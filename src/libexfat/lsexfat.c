/******************************************************************************
 * lsexfat.c  ---- list file blocklist in fs
 *
 * Copyright (c) 2020, longpanda <admin@ventoy.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <exfat.h>
#include <vtoydump.h>

static int g_image_max_region = 0;
static ventoy_image_location *g_image_location = NULL;

static int realloc_image_location(int max_region)
{
    size_t len;
    ventoy_guid guid = VENTOY_GUID;
    ventoy_image_location *new_location = NULL;

    len = sizeof(ventoy_image_location) + sizeof(ventoy_image_disk_region) * (max_region - 1);
    new_location = malloc(len);
    if (!new_location)
    {
        return -1;
    }

    if (g_image_location)
    {
        len = sizeof(ventoy_image_location) + sizeof(ventoy_image_disk_region) * (g_image_max_region - 1);
        memcpy(new_location, g_image_location, len);
        free(g_image_location);
    }
    else
    {
        memcpy(&new_location->guid, &guid, sizeof(ventoy_guid));
        new_location->image_sector_size = 2048;
        new_location->disk_sector_size = 512;
        new_location->region_count = 0;
    }

    g_image_location = new_location;
    g_image_max_region = max_region;
    
    return 0;
}

static ssize_t exfat_add_disk_region(size_t size, off_t offset)
{
    int rc = 0;
    off_t last_disk_sector_count = 0;
    ventoy_image_disk_region *last_region = NULL;
    ventoy_image_disk_region *cur_region = NULL;

    if (size == 0)
    {
        return 0;
    }

    if (g_image_location->region_count == 0)
    {
        cur_region = g_image_location->regions;
        cur_region->image_start_sector = 0;
        cur_region->image_sector_count = size / 2048;
        cur_region->disk_start_sector  = offset / 512;
        g_image_location->region_count = 1;
        return size;
    }

    last_region = g_image_location->regions + g_image_location->region_count - 1;
    last_disk_sector_count = (off_t)last_region->image_sector_count * 4;
    
    if (last_region->disk_start_sector + last_disk_sector_count == (offset / 512))
    {
        last_region->image_sector_count += size / 2048;
        return size;
    }
    
    if (g_image_location->region_count == g_image_max_region)
    {
        rc = realloc_image_location(g_image_max_region * 2);
        if (rc)
        {
            return -1;
        }

        /* must update last_region */
        last_region = g_image_location->regions + g_image_location->region_count - 1;
    }

    cur_region = last_region + 1;
    cur_region->image_start_sector = last_region->image_start_sector + last_region->image_sector_count;
    cur_region->image_sector_count = size / 2048;
    cur_region->disk_start_sector  = offset / 512;
    
    g_image_location->region_count++;

    return size;
}

static int exfat_get_image_location(struct exfat *ef, struct exfat_node *node)
{
    off_t left_size = 0;
	off_t cur_size = 0;
    off_t cur_offset = 0;
    off_t last_size = 0;
    off_t last_offset = 0;
	cluster_t cluster;

    realloc_image_location(1024);

	cluster = exfat_advance_cluster(ef, node, 0);
	if (CLUSTER_INVALID(*ef->sb, cluster))
	{
		exfat_error("invalid cluster 0x%x", cluster);
		return -EIO;
	}

	for (left_size = node->size; left_size > 0; left_size -= cur_size)
	{
		if (CLUSTER_INVALID(*ef->sb, cluster))
		{
			exfat_error("invalid cluster 0x%x while reading", cluster);
			return -EIO;
		}

        cur_size = MIN(CLUSTER_SIZE(*ef->sb), left_size);
        cur_offset = exfat_c2o(ef, cluster);

        if (last_offset + last_size == cur_offset)
        {
            last_size += cur_size;
        }
        else
        {
            exfat_add_disk_region(last_size, last_offset);
            last_size = cur_size;
            last_offset = cur_offset;
        }

		cluster = exfat_next_cluster(ef, node, cluster);
	}

    exfat_add_disk_region(last_size, last_offset);

	return 0;
}

ventoy_image_location * ventoy_get_location_by_lsexfat(const char *diskname, int part, const char *filename)
{
    int rc;
    char diskpart[128] = {0};
    struct exfat ef;
    struct exfat_node *node;

    snprintf(diskpart, sizeof(diskpart) - 1, "/dev/%s%d", diskname, part);

    rc = exfat_mount(&ef, diskpart, "ro");
    if (rc)
    {
        fprintf(stderr, "Failed to mount exfat fs %d\n", rc);
        return NULL;
    }

    rc = exfat_lookup(&ef, &node, filename);
    if (rc == 0)
    {
        rc = exfat_get_image_location(&ef, node);
        exfat_put_node(&ef, node);
    }
    else
    {
        fprintf(stderr, "Failed to find %s in exfat fs %d\n", filename, rc);
    }

    exfat_unmount(&ef);

    return rc ? NULL : g_image_location;
}

