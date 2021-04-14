/******************************************************************************
 * vtoydump_linux.c  ---- Dump ventoy os parameters in Linux
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <dirent.h>

#include <vtoydump.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif
#if defined(_dragon_fly) || defined(_free_BSD) || defined(_QNX)
#define MMAP_FLAGS          MAP_SHARED
#else
#define MMAP_FLAGS          MAP_PRIVATE
#endif

#define SEARCH_MEM_START 0x80000
#define SEARCH_MEM_LEN   0x20000

#define SYS_EFI  "/sys/firmware/efi"
#define VENTOY_OS_EFIVAR   VENTOY_VAR_NAME"-77772020-2e77-6576-6e74-6f792e6e6574"
#define VENTOY_SYS_ACPI    "/sys/firmware/acpi/tables/VTOY"

ventoy_image_location * ventoy_get_location_by_lsexfat(const char *diskname, int part, const char *filename);

static int format = 0;
int verbose = 0;
ventoy_guid vtoy_guid = VENTOY_GUID;
static const char *vtoy_fs_type[] = 
{
    "exfat", "ntfs", "ext", "xfs", "udf", "fat"
};

int vtoy_os_param_from_acpi(ventoy_os_param *param)
{
    int fd;
    acpi_table_header acpi;

    debug("vtoy_os_param_from_acpi %s\n", VENTOY_SYS_ACPI);

    if (access(VENTOY_SYS_ACPI, F_OK) < 0)
    {
        debug("%s acpi table NOT exist\n", "VTOY");
        return 1;
    }

    memset(param, 0, sizeof(ventoy_os_param));

    fd = open(VENTOY_SYS_ACPI, O_RDONLY | O_BINARY);
    if (fd >= 0)
    {
        read(fd, &acpi, sizeof(acpi_table_header));
        read(fd, param, sizeof(ventoy_os_param));
        close(fd);

        if (0 == vtoy_check_os_param(param))
        {
            return 0;
        }

        return 1;
    }
    else
    {
        debug("failed to open VTOY acpi table %d\n", errno);
        return 1;
    }
}

int vtoy_os_param_from_efivar(ventoy_os_param *param)
{
    int fd;
    int len;
    int newfmt = 1;
    const char *path = NULL;

    path = SYS_EFI"/efivars/"VENTOY_OS_EFIVAR;
    debug("vtoy_os_param_from_efivar %s\n", path);

    if (access(path, F_OK) < 0)
    {
        debug("%s NOT exist\n", path);

        path = SYS_EFI"/vars/"VENTOY_OS_EFIVAR"/data";
        if (access(path, F_OK) < 0)
        {
            debug("%s NOT exist\n", path);
            return 1;
        }

        newfmt = 0;
    }

    fd = open(path, O_RDONLY | O_BINARY);
    if (fd >= 0)
    {
        if (newfmt)
        {
            /* skip the 4 bytes attribute */
            read(fd, &newfmt, 4);
        }
    
        len = read(fd, param, sizeof(ventoy_os_param));
        close(fd);

        if (len == sizeof(ventoy_os_param))
        {
            if (0 == vtoy_check_os_param(param))
            {
                return 0;
            }
        }
        else
        {
            debug("read %s fail, %d\n", path, len);
        }

        memset(param, 0, sizeof(ventoy_os_param));
        return 1;
    }
    else
    {
        debug("failed to open %s %d\n", path, errno);
        return 1;
    }
}

int vtoy_os_param_from_phymem(ventoy_os_param *param)
{
    int i = 0;
    int fd = 0;
    int rc = 1;
    char *mapbuf = NULL;

    fd = open("/dev/mem", O_RDONLY | O_BINARY);
    if (fd < 0)
    {
        fprintf(stderr, "Failed to open memory device /dev/mem %d\n", errno);
        return errno;
    }

    mapbuf = (char *)mmap(NULL, SEARCH_MEM_LEN, PROT_READ, MMAP_FLAGS, fd, SEARCH_MEM_START);
    if (mapbuf == NULL || (uint32_t)(unsigned long)mapbuf == 0xFFFFFFFF)
    {
        fprintf(stderr, "mmap failed, NULL %d  %p\n", errno, mapbuf);
        close(fd);
        return errno;
    }

    debug("map memory 0x%x at %p\n", SEARCH_MEM_START, mapbuf);

    for (i = 0; i < SEARCH_MEM_LEN; i += 16)
    {
        if (0 == vtoy_check_os_param((ventoy_os_param *)(mapbuf + i)))
        {
            debug("find ventoy os pararm at %p offset %d phymem:0x%08x\n", mapbuf + i, i, SEARCH_MEM_START + i);
            memcpy(param, mapbuf + i, sizeof(ventoy_os_param));
            rc = 0;
            break;
        }
    }

    munmap(mapbuf, SEARCH_MEM_LEN);
    close(fd);

    return rc;
}

static int vtoy_get_disk_guid(const char *diskname, uint8_t *vtguid, uint8_t *vtsig)
{
    int i = 0;
    int fd = 0;
    char devdisk[256] = {0};

    snprintf(devdisk, sizeof(devdisk) - 1, "/dev/%s", diskname);
    
    fd = open(devdisk, O_RDONLY | O_BINARY);
    if (fd >= 0)
    {
        lseek(fd, 0x180, SEEK_SET);
        read(fd, vtguid, 16);
        lseek(fd, 0x1b8, SEEK_SET);
        read(fd, vtsig, 4);
        close(fd);

        debug("GUID for %s: <", devdisk);
        for (i = 0; i < 16; i++)
        {
            debug("%02x", vtguid[i]);
        }
        debug(">\n");
        
        return 0;
    }
    else
    {
        debug("failed to open %s %d\n", devdisk, errno);
        return errno;
    }
}

static unsigned long long vtoy_get_disk_size_in_byte(const char *disk)
{
    int fd;
    int rc;
    unsigned long long size = 0;
    char diskpath[256] = {0};
    char sizebuf[64] = {0};

    // Try 1: get size from sysfs
    snprintf(diskpath, sizeof(diskpath) - 1, "/sys/block/%s/size", disk);
    if (access(diskpath, F_OK) >= 0)
    {
        debug("get disk size from sysfs for %s\n", disk);
        
        fd = open(diskpath, O_RDONLY | O_BINARY);
        if (fd >= 0)
        {
            read(fd, sizebuf, sizeof(sizebuf));
            size = strtoull(sizebuf, NULL, 10);
            close(fd);
            return (size * 512);
        }
    }
    else
    {
        debug("%s not exist \n", diskpath);
    }

    // Try 2: get size from ioctl
    snprintf(diskpath, sizeof(diskpath) - 1, "/dev/%s", disk);
    fd = open(diskpath, O_RDONLY);
    if (fd >= 0)
    {
        debug("get disk size from ioctl for %s\n", disk);
        rc = ioctl(fd, BLKGETSIZE64, &size);
        if (rc == -1)
        {
            size = 0;
            debug("failed to ioctl %d\n", rc);
        }
        close(fd);
    }
    else
    {
        debug("failed to open %s %d\n", diskpath, errno);
    }

    debug("disk %s size %llu bytes\n", disk, (unsigned long long)size);
    return size;
}

static int vtoy_is_possible_blkdev(const char *name)
{
    if (name[0] == '.')
    {
        return 0;
    }

    /*
     * Obviously, these devices are not ventoy disk.
     * /dev/ramX  /dev/loopX  /dev/dm-X  /dev/srX
     */
    if (strncmp(name, "ram", 3) == 0 ||
        strncmp(name, "loop", 4) == 0 ||
        strncmp(name, "dm-", 3) == 0 ||
        strncmp(name, "sr", 2) == 0)
    {
        return 0;
    }
    
    return 1;
}

static int vtoy_find_disk_by_size(unsigned long long size, char *diskname, int buflen)
{
    unsigned long long cursize = 0;
    DIR* dir = NULL;
    struct dirent* p = NULL;
    int rc = 0;

    dir = opendir("/sys/block");
    if (!dir)
    {
        return 0;
    }
    
    while ((p = readdir(dir)) != NULL)
    {
        if (!vtoy_is_possible_blkdev(p->d_name))
        {
            debug("disk %s is filted by name\n", p->d_name);
            continue;
        }
    
        cursize = vtoy_get_disk_size_in_byte(p->d_name);
        debug("disk %s size %llu\n", p->d_name, (unsigned long long)cursize);
        if (cursize == size)
        {
            snprintf(diskname, buflen, "%s", p->d_name);
            rc++;
        }
    }
    closedir(dir);
    return rc;    
}

static int vtoy_find_disk_by_guid(ventoy_os_param *param, char *diskname, int buflen)
{
    int rc = 0;
    int count = 0;
    DIR* dir = NULL;
    struct dirent* p = NULL;
    uint8_t vtsig[4];
    uint8_t vtguid[16];

    dir = opendir("/sys/block");
    if (!dir)
    {
        return 0;
    }
    
    while ((p = readdir(dir)) != NULL)
    {
        if (!vtoy_is_possible_blkdev(p->d_name))
        {
            debug("disk %s is filted by name\n", p->d_name);        
            continue;
        }
    
        memset(vtguid, 0, sizeof(vtguid));
        rc = vtoy_get_disk_guid(p->d_name, vtguid, vtsig);
        if (rc == 0 && memcmp(vtguid, param->vtoy_disk_guid, 16) == 0 &&
            memcmp(vtsig, param->vtoy_disk_signature, 4) == 0)
        {
            snprintf(diskname, buflen, "%s", p->d_name);
            count++;
        }
    }
    closedir(dir);
    
    return count;    
}

static int vtoy_check_device(ventoy_os_param *param, const char *device)
{
    uint8_t vtguid[16] = {0};
    uint8_t vtsig[4] = {0};

    debug("vtoy_check_device for <%s>\n", device);

    vtoy_get_disk_guid(device, vtguid, vtsig);

    if (memcmp(vtguid, param->vtoy_disk_guid, 16) == 0 &&
        memcmp(vtsig, param->vtoy_disk_signature, 4) == 0)
    {
        debug("<%s> is right ventoy disk\n", device);
        return 0;
    }
    else
    {
        debug("<%s> is NOT right ventoy disk\n", device);
        return 1;
    }
}

int vtoy_find_disk(ventoy_os_param *param, char *diskname, int buflen)
{
    int cnt = 0;

    cnt = vtoy_find_disk_by_size(param->vtoy_disk_size, diskname, buflen);
    debug("find disk by size %llu, cnt=%d...\n", (unsigned long long)param->vtoy_disk_size, cnt);
    if (1 == cnt)
    {
        if (vtoy_check_device(param, diskname) != 0)
        {
            cnt = 0;
        }
    }
    else
    {
        cnt = vtoy_find_disk_by_guid(param, diskname, buflen);
        debug("find disk by guid cnt=%d...\n", cnt);
    }
    
    if (cnt > 1)
    {
        fprintf(stderr, "More than one disk found, Indistinguishable\n");
        return 1;
    }
    else if (cnt == 0)
    {
        fprintf(stderr, "No ventoy disk found\n");
        return 1;
    }
    else
    {
        return 0;
    }
}

static ventoy_image_location * ventoy_get_location_by_phymem(ventoy_os_param *param)
{
    int fd = 0;
    char *mapbuf = NULL;
    ventoy_image_location *location = NULL;

    debug("get image location by phymem\n");

    if (param->vtoy_img_location_addr == 0 || param->vtoy_img_location_len == 0)
    {
        return NULL;
    }

    location = (ventoy_image_location *)malloc(param->vtoy_img_location_len);
    if (!location)
    {
        return NULL;
    }
    
    fd = open("/dev/mem", O_RDONLY | O_BINARY);
    if (fd < 0)
    {
        debug("Failed to open memory device /dev/mem %d\n", errno);
        return NULL;
    }

    mapbuf = (char *)mmap(NULL, param->vtoy_img_location_len, PROT_READ, MMAP_FLAGS, fd, param->vtoy_img_location_addr);
    if (mapbuf == NULL || (uint32_t)(unsigned long)mapbuf == 0xFFFFFFFF)
    {
        debug("mmap failed, NULL %d 0x%lx %p\n", errno, (unsigned long)param->vtoy_img_location_addr, mapbuf);
        close(fd);
        return NULL;
    }
    debug("map memory 0x%llx at %p\n", (unsigned long long)param->vtoy_img_location_addr, mapbuf);

    memcpy(location, mapbuf, param->vtoy_img_location_len);

    munmap(mapbuf, param->vtoy_img_location_len);
    close(fd);

    return location;
}

static ventoy_image_location * ventoy_get_location_by_acpi(ventoy_os_param *param)
{
    int fd = 0;
    ventoy_os_param acpiparam;
    ventoy_image_location *location = NULL;

    debug("get image location by acpi\n");

    if (vtoy_os_param_from_acpi(&acpiparam))
    {
        return NULL;
    }

    debug("param->vtoy_img_location: [0x%lx %u]\n", (unsigned long)param->vtoy_img_location_addr, param->vtoy_img_location_len);
    debug("acpiparam.vtoy_img_location: [0x%lx %u]\n", (unsigned long)acpiparam.vtoy_img_location_addr, acpiparam.vtoy_img_location_len);

    if (acpiparam.vtoy_img_location_len == 0)
    {
        return NULL;
    }

    location = (ventoy_image_location *)malloc(acpiparam.vtoy_img_location_len);
    if (!location)
    {
        return NULL;
    }
    
    fd = open(VENTOY_SYS_ACPI, O_RDONLY | O_BINARY);
    if (fd < 0)
    {
        debug("Failed to open %s %d\n", VENTOY_SYS_ACPI, errno);
        free(location);
        return NULL;
    }

    lseek(fd, sizeof(acpi_table_header) + sizeof(ventoy_os_param), SEEK_SET);
    read(fd, location, acpiparam.vtoy_img_location_len);

    close(fd);

    return location;
}

int vtoy_print_image_location(ventoy_os_param *param, char *diskname)
{
    int i;
    int fd = 0;
    int partflag = 0;
    unsigned long long start;
    unsigned long long count;
    unsigned long long partstart;
    ventoy_image_location *location = NULL;
    ventoy_image_disk_region *region = NULL;
    char dmdisk[256] = {0};
    char sysstart[256] = {0};
    char valuebuf[64] = {0};

    snprintf(dmdisk, sizeof(dmdisk) - 1, "/dev/%s", diskname);
    
    /*
     * Ventoy save a copy of image disk location data to phy memory before load.
     * But in some cases the phymem can not be read in userspace.
     * For example CONFIG_DEVKMEM disabled, or CONFIG_STRICT_DEVMEM enabled.
     * In that case, we directly parse the file system and get the image location.
     *
     */
    
    location = ventoy_get_location_by_phymem(param);
    if (!location)
    {
        location = ventoy_get_location_by_acpi(param);
    }

    if (!location)
    {
        if (param->vtoy_disk_part_type == 0)
        {
            debug("get image location by fs tool\n");
            snprintf(dmdisk, sizeof(dmdisk) - 1, "/dev/%s%d", diskname, param->vtoy_disk_part_id);
            location = ventoy_get_location_by_lsexfat(diskname, param->vtoy_disk_part_id, param->vtoy_img_path);
        }
    }

    if (!location)
    {
        fprintf(stderr, "Failed to find image location\n");
        return 1;
    }

    if (memcmp(&vtoy_guid, &location->guid, sizeof(ventoy_guid)))
    {
        free(location);
        fprintf(stderr, "Image location data corrupted\n");
        return 1;
    }

    if (format == 1)
    {
        printf("=== ventoy image location ===\n");
    }

    if (strstr(diskname, "nvme") || strstr(diskname, "mmc") || strstr(diskname, "nbd"))
    {
        partflag = 1;
        snprintf(sysstart, sizeof(sysstart) - 1, "/sys/class/block/%sp%u/start", diskname, param->vtoy_disk_part_id);
        
    }
    else
    {
        snprintf(sysstart, sizeof(sysstart) - 1, "/sys/class/block/%s%u/start", diskname, param->vtoy_disk_part_id);
    }

    partstart = 2048;
    if (access(sysstart, F_OK) >= 0)
    {
        debug("get part start from sysfs for %s\n", sysstart);
        
        fd = open(sysstart, O_RDONLY | O_BINARY);
        if (fd >= 0)
        {
            read(fd, valuebuf, sizeof(valuebuf));
            partstart = strtoull(valuebuf, NULL, 10);
            close(fd);
        }
    }
    else
    {
        debug("%s not exist \n", sysstart);
    }

    /* print location in dmsetup table format */
    for (i = 0; i < (int)location->region_count; i++)
    {
        region = location->regions + i;
        start = region->image_start_sector;
        count = region->image_sector_count;

        if (partflag)
        {
            printf("%llu %llu linear %sp%u %llu\n", 
                   start * location->image_sector_size / location->disk_sector_size,
                   count * location->image_sector_size / location->disk_sector_size,
                   dmdisk, param->vtoy_disk_part_id,
                   (unsigned long long)region->disk_start_sector - partstart);
        }
        else
        {
            printf("%llu %llu linear %s%u %llu\n", 
                   start * location->image_sector_size / location->disk_sector_size,
                   count * location->image_sector_size / location->disk_sector_size,
                   dmdisk, param->vtoy_disk_part_id,
                   (unsigned long long)region->disk_start_sector - partstart);
        }
    }

    free(location);
    return 0;
}

int vtoy_print_os_param(ventoy_os_param *param, char *diskname)
{
    const char *fs = "unknown";

    if (param->vtoy_disk_part_type < sizeof(vtoy_fs_type) / sizeof(vtoy_fs_type[0]))
    {
        fs = vtoy_fs_type[param->vtoy_disk_part_type];
    }

    printf("=== ventoy runtime data ===\n");
    printf("disk name : /dev/%s\n", diskname);
    printf("disk size : %llu\n", (unsigned long long)param->vtoy_disk_size);
    printf("disk part : %u\n", param->vtoy_disk_part_id);
    printf("filesystem: %s\n", fs);
    printf("image size: %llu\n", (unsigned long long)param->vtoy_img_size);
    printf("image path: %s\n", param->vtoy_img_path);

    return 0;
}

void print_usage(void)
{
    /*
    * Parameters Description:
    * none      print ventoy runtime data
    * -l        print ventoy runtime data and image location table
    * -L        only print image location table (used to generate dmsetup table)
    * -v        be verbose
    */

    printf("Usage: vtoydump [ -lL ] [ -v ]\n");
    printf("  none   Only print ventoy runtime data\n");
    printf("  -l     Print ventoy runtime data and image location table\n");
    printf("  -L     Only print image location table (used to generate dmsetup table)\n");
    printf("  -c     Check whether ventoy runtime data exist\n");
    printf("  -v     Verbose, print additional debug info\n");
    printf("  -h     Print this help info\n");
    printf("\n");
}


int main(int argc, char **argv)
{
    int rc;
    int ch;
    int check = 0;
    char diskname[256] = { 0 };
    ventoy_os_param param;

    while ((ch = getopt(argc, argv, "l::L::c::v::h::")) != -1)
    {
        if (ch == 'l')
        {
            format = 1;
        }
        else if (ch == 'L')
        {
            format = 2;
        }
        else if (ch == 'c')
        {
            check = 1;
        }
        else if (ch == 'v')
        {
            verbose = 1;
        }
        else if (ch == 'h')
        {
            print_usage();
            return 0;
        }
        else
        {
            return 1;
        }
    }

    memset(&param, 0, sizeof(ventoy_os_param));

    rc = vtoy_os_param_from_acpi(&param);

    if (rc)
    {
        if (access(SYS_EFI, F_OK) >= 0)
        {
            debug("current is efi system, get os pararm from efivar\n");
            rc = vtoy_os_param_from_efivar(&param);
        }
        else
        {
            debug("current is legacy bios system, get os pararm from phymem\n");
            rc = vtoy_os_param_from_phymem(&param);
        }
    }

    if (rc)
    {
        fprintf(stderr, "ventoy runtime data not found\n");
        return rc;            
    }    

    if (check == 1)
    {
        return 0;
    }

    rc = vtoy_find_disk(&param, diskname, (int)(sizeof(diskname)-1));
    if (rc == 0)
    {
        if (format == 0 || format == 1)
        {
            vtoy_print_os_param(&param, diskname);
        }

        if (format == 1 || format == 2)
        {
            vtoy_print_image_location(&param, diskname);
        }
    }

    return rc;
}
