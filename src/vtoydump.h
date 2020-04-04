/******************************************************************************
 * vtoydump.h  ---- Dump ventoy os parameters 
 *
 * Copyright (c) 2020, longpanda <admin@ventoy.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __VTOYDUMP_H__
#define __VTOYDUMP_H__

#define IS_DIGIT(x) ((x) >= '0' && (x) <= '9')

#define VENTOY_VAR_NAME      "VentoyOsParam"

#define VENTOY_GUID { 0x77772020, 0x2e77, 0x6576, { 0x6e, 0x74, 0x6f, 0x79, 0x2e, 0x6e, 0x65, 0x74 }}
#define VENTOY_GUID_STR "{77772020-2e77-6576-6e74-6f792e6e6574}"

#pragma pack(1)

typedef struct ventoy_guid
{
    uint32_t   data1;
    uint16_t   data2;
    uint16_t   data3;
    uint8_t    data4[8];
}ventoy_guid;


typedef struct ventoy_image_disk_region
{
    uint32_t   image_sector_count; /* image sectors contained in this region (in 2048) */
    uint32_t   image_start_sector; /* image sector start (in 2048) */
    uint64_t   disk_start_sector;  /* disk sector start (in 512) */
}ventoy_image_disk_region;

typedef struct ventoy_image_location
{
    ventoy_guid  guid;
    
    /* image sector size, currently this value is always 2048 */
    uint32_t   image_sector_size;

    /* disk sector size, normally the value is 512 */
    uint32_t   disk_sector_size;

    uint32_t   region_count;
    
    /*
     * disk region data
     * If the image file has more than one fragments in disk, 
     * there will be more than one region data here.
     * You can calculate the region count by 
     */
    ventoy_image_disk_region regions[1];

    /* ventoy_image_disk_region regions[2~region_count-1] */
}ventoy_image_location;

typedef struct ventoy_os_param
{
    ventoy_guid    guid;             // VENTOY_GUID
    uint8_t        chksum;           // checksum

    uint8_t   vtoy_disk_guid[16];
    uint64_t  vtoy_disk_size;       // disk size in bytes
    uint16_t  vtoy_disk_part_id;    // begin with 1
    uint16_t  vtoy_disk_part_type;  // 0:exfat   1:ntfs  other: reserved
    char      vtoy_img_path[384];   // It seems to be enough, utf-8 format
    uint64_t  vtoy_img_size;        // image file size in bytes

    /* 
     * Ventoy will write a copy of ventoy_image_location data into runtime memory
     * this is the physically address and length of that memory.
     * Address 0 means no such data exist.
     * Address will be aligned by 4KB.
     *
     */
    uint64_t  vtoy_img_location_addr;
    uint32_t  vtoy_img_location_len;

    uint64_t  vtoy_reserved[4];     // Internal use by ventoy

    uint8_t   reserved[31];
}ventoy_os_param;

#pragma pack()

extern int verbose;
extern ventoy_guid vtoy_guid;
#define debug(fmt, ...) if(verbose) printf(fmt, ##__VA_ARGS__)

#define check_opt(c) (argv[ch][0] == '-' && argv[ch][1] == (c))

#ifdef WIN32
#define INLINE_DEC __forceinline
#else
#define INLINE_DEC static inline
#endif

INLINE_DEC int vtoy_check_os_param(ventoy_os_param *param)
{
    uint32_t i;
    uint8_t  chksum = 0;
    uint8_t *buf = (uint8_t *)param;

    if (memcmp(&param->guid, &vtoy_guid, sizeof(ventoy_guid)))
    {
        return 1;
    }

    for (i = 0; i < sizeof(ventoy_os_param); i++)
    {
        chksum += buf[i];
    }

    if (chksum)
    {
        debug("Invalid checksum 0x%02x\n", chksum);
        return 1;
    }

    return 0;
}

int vtoy_is_efi_system(void);
int vtoy_check_os_param(ventoy_os_param *param);
int vtoy_os_param_from_efivar(ventoy_os_param *param);
int vtoy_os_param_from_phymem(ventoy_os_param *param);
int vtoy_find_disk(ventoy_os_param *param, char *diskname, int buflen);
int vtoy_print_os_param(ventoy_os_param *param, char *diskname);
int vtoy_print_image_location(ventoy_os_param *param, char *diskname);

#endif

