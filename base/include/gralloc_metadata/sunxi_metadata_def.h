/*
 * Copyright (c) 2007-2020 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU Lesser General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * CedarC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 */

/*
*
* File : sunxi_metadata.h
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2017/04/13
*   Comment :
*
*
*/

#ifndef __SUNXI_METADATA_DEF_H__
#define __SUNXI_METADATA_DEF_H__

#include <stdint.h>

enum {
    /* hdr static metadata is available */
    SUNXI_METADATA_FLAG_HDR_SATIC_METADATA   = 0x00000001,
    /* hdr dynamic metadata is available */
    SUNXI_METADATA_FLAG_HDR_DYNAMIC_METADATA = 0x00000002,

    /* afbc header data is available */
    SUNXI_METADATA_FLAG_AFBC_HEADER          = 0x00000010,
};

struct display_master_data {

    /* display primaries */
    uint16_t display_primaries_x[3];
    uint16_t display_primaries_y[3];

    /* white_point */
    uint16_t white_point_x;
    uint16_t white_point_y;

    /* max/min display mastering luminance */
    uint32_t max_display_mastering_luminance;
    uint32_t min_display_mastering_luminance;

};

/* static metadata type 1 */
struct hdr_static_metadata {
    struct display_master_data disp_master;

    uint16_t maximum_content_light_level;//* maxCLL
    uint16_t maximum_frame_average_light_level;//* maxFALL
};

struct afbc_header {
    uint32_t signature;
    uint16_t filehdr_size;
    uint16_t version;
    uint32_t body_size;
    uint8_t ncomponents;
    uint8_t header_layout;
    uint8_t yuv_transform;
    uint8_t block_split;
    uint8_t inputbits[4];
    uint16_t block_width;
    uint16_t block_height;
    uint16_t width;
    uint16_t height;
    uint8_t left_crop;
    uint8_t top_crop;
    uint16_t block_layout;
};

/* sunxi metadata for ve and de */
struct sunxi_metadata {
    struct hdr_static_metadata hdr_smetada;
    struct afbc_header afbc_head;

    // flag must be at last always
    uint32_t flag;
};

#endif /* __SUNXI_METADATA_DEF_H__ */
