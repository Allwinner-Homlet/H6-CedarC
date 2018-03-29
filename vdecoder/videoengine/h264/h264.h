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
* File : h264.h
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#ifndef h264_H
#define h264_H

#include "h264_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ENABLE_MV_BUF_OPTIMIZATION_PROGRAM (0)

    enum AVC_NALU_TYPE
    {
        NAL_SLICE            = 1,
        NAL_DPA             = 2,
        NAL_DPB             = 3,
        NAL_DPC             = 4,
        NAL_IDR_SLICE        = 5,
        NAL_SEI             = 6,
        NAL_SPS             = 7,
        NAL_PPS             = 8,
        NAL_AUD             = 9,
        NAL_END_SEQUENCE    = 10,
        NAL_END_STREAM      = 11,
        NAL_FILLER_DATA     = 12,
        NAL_HEADER_EXT1     = 14,
        NAL_SPS_EXT         = 15,
        NAL_AUXILIARY_SLICE = 19,
        NAL_HEADER_EXT2     = 20,
    };

    typedef struct GET_BIT_CONTEXT
     {
         u8 *buffer;
         u8 *buffer_end;
         s32 index;
         s32 size_in_bits;
     }GetBitContext;

    typedef struct H264_DECODER_CONTEXT
    {
        DecoderInterface    interface;
        VideoEngine*        pVideoEngine;

        VConfig             vconfig;            //* video engine configuration;
        VideoStreamInfo     videoStreamInfo;    //* video stream information;

        void*              pH264Dec;         //* h264 decoder handle
        VideoFbmInfo*      pFbmInfo;
        GetBitContext     sBitContext;

        u8                bSoftDecHeader;

        u32 (*GetBits)(void* h264DecCtx, u32 n);
        u32 (*GetUeGolomb)(void* h264DecCtx);
        u32 (*ShowBits)(void *h264DecCtx, u32 n);
        void (*InitGetBits)(void* h264DecCtx, u8 *buffer, u32 bit_size);
        s32 (*GetSeGolomb)(void* h264DecCtx);
    }H264DecCtx;

#ifdef __cplusplus
}
#endif

#endif

