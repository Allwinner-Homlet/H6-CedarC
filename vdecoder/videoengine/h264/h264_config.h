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
* File : h264_config.h
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#ifndef H264_V2_CONFIG_H
#define H264_V2_CONFIG_H

#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include "typedef.h"
#include "videoengine.h"
#include "veAwTopRegDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define H264_DEBUG_SAVE_BIT_STREAM 0
#define H264_DEBUG_SAVE_BIT_STREAM_ES 0

#define H264_DEBUG_SAVE_PIC 0
#define H264_DEBUG_SHOW_FPS 0
#define H264_DEBUG_SHOW_BIT_RATE 0
#define H264_DEBUG_FRAME_CYCLE 32

#define H264_DEBUG_PRINT_REF_LIST 0
#define H264_DEBUG_PRINTF_REGISTER 0
#define H264_DEBUG_COMPUTE_PIC_MD5 0

#define AVC_SAVE_STREAM_PATH "/data/camera/avcstream.dat"

#ifdef __cplusplus
}
#endif

#endif

