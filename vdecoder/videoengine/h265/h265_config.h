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

#ifndef H265_CONFIG_H
#define H265_CONFIG_H

#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include "typedef.h"
#include "videoengine.h"
#include "veAwTopRegDef.h"

/**************** hevc const start ********************/
#define HEVC_INT_MAX 0x7fffffff
#define HEVC_INT_MINI (-(HEVC_INT_MAX-1))
#define HEVC_LOCAL_SBM_BUF_SIZE (512*1024)
#define HEVC_NEIGHBOUR_INFO_BUF_SIZE (101632 * 4 * 2)
#define HEVC_ENTRY_POINT_OFFSET_BUFFER_SIZE (4*1024)
#define HEVC_SLICE_HEADER_BYTES (1024)
#define HEVC_SEARCH_START_CODE_TIME 50

#define HEVC_MAX_SUB_LAYERS 7
#define HEVC_MAX_VPS_NUM 16
#define HEVC_MAX_SPS_NUM 32
#define HEVC_MAX_PPS_NUM 256
#define HEVC_MAX_DPB_SIZE (16+4)
#define HEVC_MAX_NALU_NUM \
(HEVC_MAX_SUB_LAYERS+HEVC_MAX_VPS_NUM+HEVC_MAX_SPS_NUM+HEVC_MAX_PPS_NUM+HEVC_MAX_DPB_SIZE)
#define HEVC_MAX_STREAM_NUM HEVC_MAX_NALU_NUM
#define HEVC_MAX_SHORT_TERM_RPS_COUNT 64
#define HEVC_MAX_REFS 16
#define HEVC_EXTRA_FBM_NUM (1+1+3+3)
#define HEVC_PTS_LIST_SIZE (HEVC_MAX_DPB_SIZE+4)
#define HEVC_MRG_MAX_NUM_CANDS 5
#define HEVC_SCALING_LIST_NUM 6

#define HEVC_FRAME_FLAG_OUTPUT    (1 << 0)
#define HEVC_FRAME_FLAG_SHORT_REF (1 << 1)
#define HEVC_FRAME_FLAG_LONG_REF  (1 << 2)

#define HEVC_MD5_PARSED        (1 << 0)
#define HEVC_FRAME_CONSTRUCTED (1 << 1)
#define HEVC_MD5_CHECK  (HEVC_MD5_PARSED | HEVC_FRAME_CONSTRUCTED)

#define HEVC_START_CODE 0x000001
#define HEVC_EMULATION_CODE 0x03
#define HEVC_PROFILE_MAIN     1
#define HEVC_PROFILE_MAIN_10   2
#define HEVC_PROFILE_MAIN_STILL_PICTURE  3
#define HEVC_EXTENDED_SAR 255
#define HEVC_SEI_UNSUPPORTED 64
#define HEVC_LIGHT_WEIGHT_ERROR    128

/***************** hevc const end *********************/

/**************** feature configure start ********************/
#define HEVC_HW_DECODE_SLICE_HEADER 1
#define HEVC_HW_SEARCH_START_CODE 0
#define HEVC_ADAPTIVE_DROP_FRAME_ENABLE 1
#define HEVC_ENABLE_CATCH_DDR 0
/****************  feature configure end  ********************/

/**************** debug configure start ********************/
/* debug configure should be zero when releasing */
#define HEVC_DECODE_TIME_INFO 0
#define HEVC_SAVE_EACH_FRAME_TIME 0
#define HEVC_BIT_RATE_INFO 0
#define HEVC_ZERO_PTS 0
#define HEVC_CLOSE_SBM_CYCLE_BUFFER 0
#define HEVC_CLOSE_DISPLAY 0
#define HEVC_ENABLE_MD5_CHECK 0
#define HEVC_ENABLE_SHA_CHECK 0
#define HEVC_SOFTWARE_REGISTER_SHOW 0
#define HEVC_ENABLE_MEMORY_LEAK_DEBUG 0
#define HEVC_PARSER_TRACE_DEBUG 0
#define HEVC_SAVE_YUV_PIC 0
#define HEVC_SAVE_SEC_YUV_PIC 0
#define HEVC_SAVE_STREAM_DATA 0
#define HEVC_SAVE_DECODER_DATA 0
#define HEVC_SAVE_SLICE_DATA 0
#define HEVC_DROP_FRAME_DEBUG 0

#define HEVC_SCALE_DOWN_DEBUG 0
#define HEVC_DISABLE_SCALE_DOWN 0

#define HEVC_SEARCH_NEXT_START_CODE_DISABLE 0

#define HEVC_PRINT_REGISTER 0
#define HEVC_DEBUG_FUNCTION_ENABLE 0
#define HEVC_SHOW_STREAM_INFO 0
#define HEVC_SHOW_RPL_INFO 0
#define HEVC_SHOW_RPS_INFO 0
#define HEVC_SHOW_REG_INFO 0
#define HEVC_SHOW_PTS_INFO 0
#define HEVC_SHOW_MAX_USING_BUFFER_NUM 0
#define HEVC_SHOW_SCALING_LIST_INFO 0

#if HEVC_PARSER_TRACE_DEBUG || \
    HEVC_HW_SEARCH_START_CODE || \
    HEVC_ENABLE_MD5_CHECK
/* add usleep() function to slow decoder speed, for debug */
#define HEVC_SLOW_DECODER 1
#endif
//#define HEVC_SLOW_DECODER 0

#if HEVC_ENABLE_MD5_CHECK
#undef HEVC_EXTRA_FBM_NUM
#define HEVC_EXTRA_FBM_NUM (HEVC_MAX_DPB_SIZE)
#endif

#if HEVC_CLOSE_SBM_CYCLE_BUFFER
#undef HEVC_LOCAL_SBM_BUF_SIZE
#define HEVC_LOCAL_SBM_BUF_SIZE (3*1024*1024)
#endif //HEVC_CLOSE_SBM_CYCLE_BUFFER

/* HEVC_FRAME_DURATION should be 16, 32, 64 or 128  */
#define HEVC_FRAME_DURATION 64
#define HEVE_VE_COUNTER_LIST 16
/****************  debug configure end  ********************/

/**************** temporary data start ********************/
#if 1
#endif /* if 1*/
/****************  temporary data end  ********************/

#endif /* H265_CONFIG_H */
