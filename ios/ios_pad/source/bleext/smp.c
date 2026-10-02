/*
 *   Copyright (C) 2026 GaryOderNichts
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "smp.h"
#include "stack/btm.h"
#include "stack/hci.h"
#include "stack/l2c.h"
#include "wud.h"

#include <string.h>

// This might be overkill, we'll never have this many concurrent SMP connections
// TODO close SMP connections after use
#define SMP_MAX_HANDLES 16

typedef enum {
    SMP_STATE_INIT,
    SMP_STATE_WAIT_CONNECT,
    SMP_STATE_PAIRING_REQUEST,
    SMP_STATE_PAIRING_CONFIRM,
    SMP_STATE_PAIRING_RANDOM,
    SMP_STATE_STK_START_ENCRYPT,
    SMP_STATE_LTK_START_ENCRYPT,
} SMPState;

typedef struct {
    uint8_t in_use;
    BD_ADDR addr;

    SMPState state;
    SMPCallback callback;
    void* userPointer;

    // SMP pairing state
    uint8_t mrand[16];
    uint8_t srand[16];
    uint8_t preq[7];
    uint8_t pres[7];

    // Bonding data
    uint8_t ltk[16];
    uint8_t rand[8];
    uint16_t ediv;
} SMPHandle;

static int smp_is_registered = 0;
static SMPHandle smp_handles[SMP_MAX_HANDLES];

/*--------------------*/
/* Internal functions */
/*--------------------*/

static SMPHandle* _SMP_AllocateHandle(BD_ADDR addr)
{
    // Find unused handle
    SMPHandle* handle = NULL;
    int i;
    for (i = 0; i < SMP_MAX_HANDLES; i++) {
        if (!smp_handles[i].in_use) {
            handle = &smp_handles[i];
            break;
        }
    }

    if (handle) {
        memset(handle, 0, sizeof(SMPHandle));
        handle->in_use = 1;
        memcpy(handle->addr, addr, sizeof(BD_ADDR));
    }

    return handle;
}

static SMPHandle* _SMP_GetHandleFromBDA(BD_ADDR addr)
{
    // Find the handle
    for (int i = 0; i < SMP_MAX_HANDLES; i++) {
        SMPHandle* handle = &smp_handles[i];

        if (handle->in_use && memcmp(handle->addr, addr, sizeof(BD_ADDR)) == 0) {
            return handle;
        }
    }

    return NULL;
}

static void _SMP_FreeHandle(SMPHandle* handle)
{
    handle->in_use = 0;
}

static BT_HDR* _SMP_AllocateRequest(uint32_t size)
{
    BT_HDR* buf = GKI_getbuf(sizeof(BT_HDR) + size + L2CAP_MIN_OFFSET);
    if (!buf) {
        return NULL;
    }

    buf->offset = L2CAP_MIN_OFFSET;
    buf->len = size;

    return buf;
}

/*-----------------------*/
/* SMP request functions */
/*-----------------------*/

static void _SMP_SendRequest(SMPHandle* handle, BT_HDR* buf)
{
    if (!buf) {
        DEBUG_PRINT("SMP: Trying to send NULL request\n");
        return;
    }

    if (!L2CA_SendFixedChnlData(L2CAP_SMP_CID, handle->addr, buf)) {
        // TODO do we want to disconnect in this case?
        DEBUG_PRINT("SMP: Failed to send request\n");
    }
}

static void _SMP_Send_PairingRequest(SMPHandle* handle, const uint8_t request[6])
{
    BT_HDR* buf = _SMP_AllocateRequest(7);
    if (!buf) {
        return;
    }

    uint8_t* p = (uint8_t*) (buf + 1) + buf->offset;
    p[0] = SMP_CMD_PAIRING_REQUEST;
    memcpy(p + 1, request, 6);

    _SMP_SendRequest(handle, buf);
}

static void _SMP_Send_PairingConfirm(SMPHandle* handle, const uint8_t confirm[16])
{
    BT_HDR* buf = _SMP_AllocateRequest(17);
    if (!buf) {
        return;
    }

    uint8_t* p = (uint8_t*) (buf + 1) + buf->offset;
    p[0] = SMP_CMD_PAIRING_CONFIRM;
    memcpy(p + 1, confirm, 16);

    _SMP_SendRequest(handle, buf);
}

static void _SMP_Send_PairingRandom(SMPHandle* handle, const uint8_t random[16])
{
    BT_HDR* buf = _SMP_AllocateRequest(17);
    if (!buf) {
        return;
    }

    uint8_t* p = (uint8_t*) (buf + 1) + buf->offset;
    p[0] = SMP_CMD_PAIRING_RANDOM;
    memcpy(p + 1, random, 16);

    _SMP_SendRequest(handle, buf);
}

static void _SMP_Send_IdentityInformation(SMPHandle* handle, const uint8_t irk[16])
{
    BT_HDR* buf = _SMP_AllocateRequest(17);
    if (!buf) {
        return;
    }

    uint8_t* p = (uint8_t*) (buf + 1) + buf->offset;
    p[0] = SMP_CMD_IDENTITY_INFORMATION;
    memcpy(p + 1, irk, 16);

    _SMP_SendRequest(handle, buf);
}

static void _SMP_Send_IdentityAddressInformation(SMPHandle* handle, uint8_t type, const BD_ADDR address)
{
    BT_HDR* buf = _SMP_AllocateRequest(8);
    if (!buf) {
        return;
    }

    uint8_t* p = (uint8_t*) (buf + 1) + buf->offset;
    p[0] = SMP_CMD_IDENTITY_ADDRESS_INFORMATION;
    p[1] = type;
    memcpy(p + 2, address, sizeof(BD_ADDR));

    _SMP_SendRequest(handle, buf);
}

/*-------------------------------------*/
/* SMP cryptographic toolbox functions */
/*-------------------------------------*/

// "2.2.1 Security function e"
static void _SMP_crypt_e(const uint8_t k[16], const uint8_t in[16], uint8_t out[16])
{
    // Need to reverse input and output:
    // >The most significant octet of key corresponds to key[0], the most significant
    // >octet of plaintextData corresponds to in[0] and the most significant octet of
    // >encryptedData corresponds to out[0] using the notation specified in FIPS-197.
    uint8_t krev[16];
    uint8_t inrev[16];
    uint8_t outrev[16];
    for (int i = 0; i < 16; i++) {
        krev[i] = k[15 - i];
        inrev[i] = in[15 - i];
    }

    // "AES-128-bit block cypher as defined in FIPS-197"
    // We could use "HCI_LE_Encrypt" for this, but then we have to wait for a callback without
    // getting a reference to the SMPHandle
    int* handle = createIOSCAesKeyHandle(krev, 16);
    if (handle) {
        aesEcbEncrypt(handle, inrev, 16, outrev, 16);
        destroyIOSCAesKeyHandle(handle);
    }

    // Reverse output
    for (int i = 0; i < 16; i++) {
        out[i] = outrev[15 - i];
    }
}

// "2.2.3 Confirm value generation function c1"
// c1 (k, r, preq, pres, iat, rat, ia, ra) = e(k, e(k, r XOR p1) XOR p2)
static void _SMP_crypt_c1(const uint8_t k[16], const uint8_t r[16], const uint8_t preq[7], const uint8_t pres[7],
                          uint8_t iat, const uint8_t ia[6], uint8_t rat, const uint8_t ra[6], uint8_t out[16])
{
    // p1 = pres || preq || rat' || iat'
    uint8_t p1[16];
    p1[0] = iat;
    p1[1] = rat;
    memcpy(p1 + 2, preq, 7);
    memcpy(p1 + 9, pres, 7);

    // p2 = padding || ia || ra
    uint8_t p2[16];
    reverseBDA(p2, ra);
    reverseBDA(p2 + 6, ia);
    memset(p2 + 12, 0, 4);

    // e(k, e(k, r XOR p1) XOR p2)
    for (int i = 0; i < 16; i++) {
        p1[i] ^= r[i];
    }
    _SMP_crypt_e(k, p1, p1);
    for (int i = 0; i < 16; i++) {
        p2[i] ^= p1[i];
    }
    _SMP_crypt_e(k, p2, out);
}

// "2.2.4 Key generation function s1"
static void _SMP_crypt_s1(const uint8_t k[16], const uint8_t r1[16], const uint8_t r2[16], uint8_t out[16])
{
    // r' = r1' || r2'
    uint8_t r[16];
    memcpy(r, r2, 8);
    memcpy(r + 8, r1, 8);

    // s1(k, r1, r2) = e(k, r')
    _SMP_crypt_e(k, r, out);
}

/*-----------------------*/
/* SMP pairing functions */
/*-----------------------*/

static void _SMP_PairingStart(SMPHandle* handle)
{
    static const uint8_t pairingRequest[6] = {
        SMP_IO_CAP_NOINPUTNOOUTPUT, /* IO Capability */
        SMP_OOB_DATA_NONE,          /* OOB data flag  */
        SMP_BONDING_FLAG_BONDING,   /* AuthReq */
        16,                         /* Maximum Encryption Key Size */
        SMP_KEY_DIST_ID_KEY,        /* Initiator Key Distribution */
        SMP_KEY_DIST_ENC_KEY,       /* Responder Key Distribution */
    };

    // Store pairing request for later
    handle->preq[0] = SMP_CMD_PAIRING_REQUEST;
    memcpy(handle->preq + 1, pairingRequest, sizeof(pairingRequest));

    // Send pairing request
    handle->state = SMP_STATE_PAIRING_REQUEST;
    _SMP_Send_PairingRequest(handle, pairingRequest);
}

static void _SMP_PairingConfirm(SMPHandle* handle)
{
    // Find device record
    tBTM_SEC_DEV_REC* dev = btm_find_dev(handle->addr);
    if (!dev) {
        DEBUG_PRINT("SMP: unknown dev\n");
        return;
    }

    // Generate our random
    // We could use "HCI_LE_Rand" for this, but then we have to wait for a callback without
    // getting a reference to the SMPHandle
    generateRandom(handle->mrand, sizeof(handle->mrand));

    // Use a zero key for encryption
    // We only support "2.3.5.2 Just Works", which always uses a zero key:
    // > Both devices set the TK value used in the authentication mechanism defined in Section 2.3.5.5 to zero.
    static const uint8_t TK[16] = { 0 };

    // Get mconfirm
    uint8_t confirm[16];
    _SMP_crypt_c1(TK, handle->mrand, handle->preq, handle->pres, BLE_ADDR_PUBLIC, gWBC.hostAddress,
                  dev->ble.ble_addr_type, handle->addr, confirm);

    // Send pairing confirm
    _SMP_Send_PairingConfirm(handle, confirm);
}

static void _SMP_StartEncryption(SMPHandle* handle)
{
    // Find device record for hci handle
    tBTM_SEC_DEV_REC* dev = btm_find_dev(handle->addr);
    if (!dev) {
        DEBUG_PRINT("SMP: unknown dev\n");
        return;
    }

    // Use a zero key for encryption
    // We only support "2.3.5.2 Just Works", which always uses a zero key:
    // > Both devices set the TK value used in the authentication mechanism defined in Section 2.3.5.5 to zero.
    static const uint8_t TK[16] = { 0 };

    // STK = s1(TK, Srand, Mrand)
    uint8_t stk[16];
    _SMP_crypt_s1(TK, handle->srand, handle->mrand, stk);

    // Tell controller to start encryption (using zero rand and ediv for STK)
    static const uint8_t dummy_rand[8] = { 0 };
    btsnd_hcic_ble_start_enc(dev->hci_handle, dummy_rand, 0, stk);
}

static void _SMP_StartLTKEncryption(SMPHandle* handle)
{
    // Find device record for hci handle
    tBTM_SEC_DEV_REC* dev = btm_find_dev(handle->addr);
    if (!dev) {
        DEBUG_PRINT("SMP: unknown dev\n");
        return;
    }

    // Start encryption
    btsnd_hcic_ble_start_enc(dev->hci_handle, handle->rand, handle->ediv, handle->ltk);
}

/*----------------------*/
/* L2C connect callback */
/*----------------------*/
static void _SMP_Connect_Callback(BD_ADDR bd_addr, uint8_t connected, uint16_t reason)
{
    DEBUG_PRINT("_SMP_Connect_Callback %s %d %x\n", bdaddr_to_string(bd_addr), connected, reason);

    SMPHandle* handle = _SMP_GetHandleFromBDA(bd_addr);

    if (connected) {
        if (handle) {
            if (handle->state == SMP_STATE_WAIT_CONNECT) {
                // Start the pairing process
                _SMP_PairingStart(handle);
            } else {
                DEBUG_PRINT("SMP: Invalid state\n");
            }
        } else {
            // not our connection, wait until someone tells us to start encryption
        }
    } else {
        if (handle) {
            _SMP_FreeHandle(handle);
        }
    }
}

/*-------------------------------------*/
/* SMP callbacks and response handlers */
/*-------------------------------------*/

static void _SMP_Handle_PairingResponse(SMPHandle* handle, uint8_t* p, uint16_t len)
{
    if (handle->state != SMP_STATE_PAIRING_REQUEST) {
        DEBUG_PRINT("SMP: Invalid state\n");
        return;
    }

    if (len < 6) {
        DEBUG_PRINT("SMP: Unsupported packet length\n");
        return;
    }

    // Store response
    handle->pres[0] = SMP_CMD_PAIRING_RESPONSE;
    memcpy(handle->pres + 1, p, 6);

    // Reply with pairing confirm
    handle->state = SMP_STATE_PAIRING_CONFIRM;
    _SMP_PairingConfirm(handle);
}

static void _SMP_Handle_PairingConfirm(SMPHandle* handle, uint8_t* p, uint16_t len)
{
    if (handle->state != SMP_STATE_PAIRING_CONFIRM) {
        DEBUG_PRINT("SMP: Invalid state\n");
        return;
    }

    if (len < 16) {
        DEBUG_PRINT("SMP: Unsupported packet length\n");
        return;
    }

    // We could store the received SConfirm here to confirm later, but ¯\_(ツ)_/¯

    // Reply with pairing random
    handle->state = SMP_STATE_PAIRING_RANDOM;
    _SMP_Send_PairingRandom(handle, handle->mrand);
}

static void _SMP_Handle_PairingRandom(SMPHandle* handle, uint8_t* p, uint16_t len)
{
    if (handle->state != SMP_STATE_PAIRING_RANDOM) {
        DEBUG_PRINT("SMP: Invalid state\n");
        return;
    }

    if (len < 16) {
        DEBUG_PRINT("SMP: Unsupported packet length\n");
        return;
    }

    // Store SRand
    memcpy(handle->srand, p, 16);

    // We could now verify the stored SConfirm here, but ¯\_(ツ)_/¯

    // We now have everything to calculate STK and start encryption
    handle->state = SMP_STATE_STK_START_ENCRYPT;
    _SMP_StartEncryption(handle);
}

static void _SMP_Handle_PairingFailed(SMPHandle* handle, uint8_t* p, uint16_t len)
{
    if (len < 1) {
        DEBUG_PRINT("SMP: Unsupported packet length\n");
        return;
    }

    DEBUG_PRINT("SMP: Pairing failed. Reason: 0x%x\n", *p);

    // Tell HID BLE of failure
    handle->callback(handle->addr, SMP_EVENT_FAILURE, NULL, handle->userPointer);

    // TODO Does this work?
    L2CA_RemoveFixedChnl(L2CAP_SMP_CID, handle->addr);
}

static void _SMP_Handle_EncryptionInformation(SMPHandle* handle, uint8_t* p, uint16_t len)
{
    // TODO states?

    memcpy(handle->ltk, p, sizeof(handle->ltk));
}

static void _SMP_Handle_MasterIdentification(SMPHandle* handle, uint8_t* p, uint16_t len)
{
    // TODO states?

    handle->ediv = p[0] | (p[1] << 8);
    memcpy(handle->rand, p + 2, sizeof(handle->rand));

    // TODO We just send random irk for now
    uint8_t irk[16];
    generateRandom(irk, sizeof(irk));
    _SMP_Send_IdentityInformation(handle, irk);
    _SMP_Send_IdentityAddressInformation(handle, BLE_ADDR_PUBLIC, gWBC.hostAddress);

    // Now that we have all bonding information, notify upper layer
    SMPBondingData data;
    memcpy(data.ltk, handle->ltk, 16);
    memcpy(data.rand, handle->rand, 8);
    data.ediv = handle->ediv;
    handle->callback(handle->addr, SMP_EVENT_BOND, &data, handle->userPointer);
}

/*-------------------*/
/* L2C data callback */
/*-------------------*/
static void _SMP_Data_Callback(BD_ADDR bd_addr, BT_HDR* p_buf)
{
    uint8_t* p = (uint8_t*) (p_buf + 1) + p_buf->offset;
    uint16_t len = p_buf->len;

    if (len == 0) {
        GKI_freebuf(p_buf);
        return;
    }

    uint8_t cmd = *p++;
    len--;

    DEBUG_PRINT("_SMP_Data_Callback %s %d cmd %02x\n", bdaddr_to_string(bd_addr), p_buf->len, cmd);

    SMPHandle* handle = _SMP_GetHandleFromBDA(bd_addr);
    if (!handle) {
        DEBUG_PRINT("SMP: No handle for data\n");
        GKI_freebuf(p_buf);
        return;
    }

    switch (cmd) {
    case SMP_CMD_PAIRING_RESPONSE:
        _SMP_Handle_PairingResponse(handle, p, len);
        break;
    case SMP_CMD_PAIRING_CONFIRM:
        _SMP_Handle_PairingConfirm(handle, p, len);
        break;
    case SMP_CMD_PAIRING_RANDOM:
        _SMP_Handle_PairingRandom(handle, p, len);
        break;
    case SMP_CMD_PAIRING_FAILED:
        _SMP_Handle_PairingFailed(handle, p, len);
        break;
    case SMP_CMD_ENCRYPTION_INFORMATION:
        _SMP_Handle_EncryptionInformation(handle, p, len);
        break;
    case SMP_CMD_MASTER_IDENTIFICATION:
        _SMP_Handle_MasterIdentification(handle, p, len);
        break;

    default:
        DEBUG_PRINT("SMP: Unhandled SMP cmd 0x%x\n", cmd);
        break;
    }

    GKI_freebuf(p_buf);
}

static void _SMP_Register(void)
{
    if (smp_is_registered) {
        return;
    }
    smp_is_registered = 1;

    tL2CAP_FIXED_CHNL_REG fixed_reg;
    fixed_reg.fixed_chnl_opts.mode = L2CAP_FCR_BASIC_MODE;
    fixed_reg.fixed_chnl_opts.max_transmit = 0;
    fixed_reg.fixed_chnl_opts.rtrans_tout = 0;
    fixed_reg.fixed_chnl_opts.mon_tout = 0;
    fixed_reg.fixed_chnl_opts.mps = 0;
    fixed_reg.fixed_chnl_opts.tx_win_sz = 0;
    fixed_reg.pL2CA_FixedConn_Cb = _SMP_Connect_Callback;
    fixed_reg.pL2CA_FixedData_Cb = _SMP_Data_Callback;
    fixed_reg.default_idle_tout = 60;
    L2CA_RegisterFixedChannel(L2CAP_SMP_CID, &fixed_reg);
}

/*------------------------*/
/* Public facing SMP API */
/*------------------------*/

void SMP_Init(void)
{
    _SMP_Register();
}

int SMP_StartPairing(BD_ADDR addr, SMPCallback callback, void* userPointer)
{
    SMPHandle* handle = _SMP_GetHandleFromBDA(addr);
    if (handle) {
        DEBUG_PRINT("SMP: Error attempting to connect with active handle\n");
        return 0;
    }

    handle = _SMP_AllocateHandle(addr);
    if (!handle) {
        DEBUG_PRINT("SMP: Failed to allocate handle\n");
        return 0;
    }

    // Wait for connection
    handle->state = SMP_STATE_WAIT_CONNECT;
    handle->callback = callback;
    handle->userPointer = userPointer;

    return L2CA_ConnectFixedChnl(L2CAP_SMP_CID, addr);
}

int SMP_StartEncrypt(BD_ADDR addr, SMPCallback callback, SMPBondingData* bondingData, void* userPointer)
{
    SMPHandle* handle = _SMP_GetHandleFromBDA(addr);
    if (handle) {
        DEBUG_PRINT("SMP: Error attempting to encrypt with active handle\n");
        return 0;
    }

    handle = _SMP_AllocateHandle(addr);
    if (!handle) {
        DEBUG_PRINT("SMP: Failed to allocate handle\n");
        return 0;
    }

    handle->state = SMP_STATE_LTK_START_ENCRYPT;
    handle->callback = callback;
    handle->userPointer = userPointer;

    // Copy BLE information from bonding info
    memcpy(handle->ltk, bondingData->ltk, sizeof(handle->ltk));
    memcpy(handle->rand, bondingData->rand, sizeof(handle->rand));
    handle->ediv = bondingData->ediv;

    // Start encryption
    _SMP_StartLTKEncryption(handle);
    return 1;
}

void SMP_HandleEncryptChange(BD_ADDR addr, uint8_t status, uint8_t encr_enable)
{
    SMPHandle* handle = _SMP_GetHandleFromBDA(addr);
    if (!handle) {
        DEBUG_PRINT("SMP: No handle for encryption change\n");
        return;
    }

    if (handle->state != SMP_STATE_STK_START_ENCRYPT && handle->state != SMP_STATE_LTK_START_ENCRYPT) {
        DEBUG_PRINT("SMP: Encryption change in unknown state?");
        return;
    }

    // Did we fail to start encryption?
    if (status != 0 || !encr_enable) {
        handle->callback(handle->addr, SMP_EVENT_FAILURE, NULL, handle->userPointer);
        return;
    }

    // Notify client of encryption
    handle->callback(handle->addr, SMP_EVENT_ENCRYPT, NULL, handle->userPointer);
}
