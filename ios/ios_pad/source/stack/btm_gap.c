#include "bt_api.h"
#include "btm.h"
#include "l2c.h"
#include "device_info.h"

uint8_t (*const real_btm_ble_is_discoverable)(BD_ADDR bda, uint8_t evt_type, uint8_t* p) = (void*) 0x11f0cab0;
uint8_t btm_ble_is_discoverable_hook(BD_ADDR bda, uint8_t evt_type, uint8_t* p)
{
    uint8_t* data;
    uint8_t dataLen;
    uint8_t len;
    char nameBuf[BTM_BLE_ADV_DATA_LEN_MAX + 1] = {};
    uint16_t appearance = 0;
    uint8_t mfrDataLen = 0;
    const uint8_t* mfrData = NULL;

    // TODO this is currently to prevent a race between WUD disconnecting the device and reconnecting
    // HID BLE getting the disconnection event, so we prevent from scanning for a device that might be still connected
    if (BTM_IsAclConnectionUp(bda)) {
        return 0;
    }

    dataLen = *p;
    if (dataLen != 0 && dataLen < BTM_BLE_CACHE_ADV_DATA_MAX) {
        if ((data = BTM_CheckAdvData(p + 1, BTM_BLE_AD_TYPE_NAME_CMPL, &len))) {
            memcpy(nameBuf, data, MIN(len, BTM_BLE_ADV_DATA_LEN_MAX));
        }

        if ((data = BTM_CheckAdvData(p + 1, BTM_BLE_AD_TYPE_MANU, &len))) {
            mfrDataLen = MIN(len, BTM_BLE_ADV_DATA_LEN_MAX);
            mfrData = data;
        }

        if ((data = BTM_CheckAdvData(p + 1, BTM_BLE_AD_TYPE_APPEARANCE, &len))) {
            appearance = (data[1] << 8) | data[0];
        }
    }

    DEBUG_PRINT("btm_ble_is_discoverable_hook: %s %s %04x\n", bdaddr_to_string(bda), nameBuf, appearance);

    if (real_btm_ble_is_discoverable(bda, evt_type, p)) {
        if (appearance == 0x03c4 || // Appearance: Gamepad (0x03c4)
            appearance == 0x03c2) { // Appearance: Mouse (0x03c2) [Steam Controller]
            return 1;
        }

        if (mfrData && mfrDataLen >= 9) {
            uint16_t manufacturerId = mfrData[0] | (mfrData[1] << 8);
            // Nintendo (Switch 2 controllers)
            // https://github.com/ndeadly/switch2_controller_research/blob/master/bluetooth_interface.md#manufacturer-data-format
            if (manufacturerId == 0x0553) {
                uint16_t vendorId = mfrData[5] | (mfrData[6] << 8);
                uint16_t productId = mfrData[7] | (mfrData[8] << 8);

                // Add vendor and product id to device info
                DeviceInfo* info = DeviceInfo_GetOrAllocate(bda);
                if (!info) {
                    return 0;
                }

                info->magic = MAGIC_SWITCH2;
                info->vendor_id = vendorId;
                info->product_id = productId;
                return 1;
            }
        }

        return 0;
    }

    return 0;
}
