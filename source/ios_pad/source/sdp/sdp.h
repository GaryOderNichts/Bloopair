/*
 *   Copyright (C) 2021 GaryOderNichts
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
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

#pragma once

#include <imports.h>

#define SDP_MAX_ATTR_LEN            400

/* Maximum UUID size - 16 bytes, and structure to hold any type of UUID. */
#define MAX_UUID_SIZE              16
typedef struct
{
#define LEN_UUID_16     2
#define LEN_UUID_32     4
#define LEN_UUID_128    16

    uint16_t          len;

    union
    {
        uint16_t      uuid16;
        uint32_t      uuid32;
        uint8_t       uuid128[MAX_UUID_SIZE];
    } uu;

} tBT_UUID;

/* Define a structure to hold the discovered service information. */
typedef struct
{
    union
    {
        uint8_t       u8;                         /* 8-bit integer            */
        uint16_t      u16;                        /* 16-bit integer           */
        uint32_t      u32;                        /* 32-bit integer           */
        uint8_t       array[4];                   /* Variable length field    */
        struct t_sdp_disc_attr *p_sub_attr;     /* Addr of first sub-attr (list)*/
    } v;

} tSDP_DISC_ATVAL;

typedef struct t_sdp_disc_attr
{
    struct t_sdp_disc_attr *p_next_attr;        /* Addr of next linked attr     */
    uint16_t                attr_id;            /* Attribute ID                 */
    uint16_t                attr_len_type;      /* Length and type fields       */
    tSDP_DISC_ATVAL         attr_value;         /* Variable length entry data   */
} tSDP_DISC_ATTR;

typedef struct t_sdp_disc_rec
{
    tSDP_DISC_ATTR          *p_first_attr;      /* First attribute of record    */
    struct t_sdp_disc_rec   *p_next_rec;        /* Addr of next linked record   */
    uint32_t                time_read;          /* The time the record was read */
    uint8_t                 remote_bd_addr[6];  /* Remote BD address            */
} tSDP_DISC_REC;

typedef struct
{
    uint32_t        mem_size;                   /* Memory size of the DB        */
    uint32_t        mem_free;                   /* Memory still available       */
    tSDP_DISC_REC   *p_first_rec;               /* Addr of first record in DB   */
    uint16_t        num_uuid_filters;           /* Number of UUIds to filter    */
    tBT_UUID        uuid_filters[3];            /* UUIDs to filter      */
    uint16_t        num_attr_filters;           /* Number of attribute filters  */
    uint16_t        attr_filters[13];           /* Attributes to filter */
    uint8_t         *p_free_mem;                /* Pointer to free memory       */
}tSDP_DISCOVERY_DB;

#define UUID_SERVCLASS_PNP_INFORMATION          0X1200  /* Device Identification */

/* Device Identification (DI)
*/
#define ATTR_ID_SPECIFICATION_ID                0x0200
#define ATTR_ID_VENDOR_ID                       0x0201
#define ATTR_ID_PRODUCT_ID                      0x0202
#define ATTR_ID_PRODUCT_VERSION                 0x0203
#define ATTR_ID_PRIMARY_RECORD                  0x0204
#define ATTR_ID_VENDOR_ID_SOURCE                0x0205

extern uint8_t (*const SDP_InitDiscoveryDb)(tSDP_DISCOVERY_DB *p_db, uint32_t len, uint16_t num_uuid, tBT_UUID *p_uuid_list, uint16_t num_attr, uint16_t *p_attr_list);
extern uint8_t (*const SDP_ServiceSearchRequest)(uint8_t *p_bd_addr, tSDP_DISCOVERY_DB *p_db, void* p_cb);
extern uint8_t (*const HID_HostGetSDPRecord)(uint8_t *addr, tSDP_DISCOVERY_DB *p_db, uint32_t db_len, void* p_cb);
extern tSDP_DISC_ATTR* (*const SDP_FindAttributeInRec)(tSDP_DISC_REC *p_rec, uint16_t attr_id);
extern tSDP_DISC_REC* (*const SDP_FindServiceUUIDInDb)(tSDP_DISCOVERY_DB *p_db, tBT_UUID *p_uuid, tSDP_DISC_REC *p_start_rec);

uint16_t SDP_DiDiscover(uint8_t* remote_device, tSDP_DISCOVERY_DB *p_db, uint32_t len, void *p_cb);
