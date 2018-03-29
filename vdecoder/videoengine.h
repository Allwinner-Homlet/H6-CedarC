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
* File : videoengine.h
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#ifndef VIDEOENGINE_H
#define VIDEOENGINE_H

#include "vdecoder.h"
#include "sbm.h"
#include "fbm.h"

/******************* decoder const fbm buffer start *********************/
/* each decoder's mini number
 h264 h265 decoder's mini number need to add reference picture number;
H264_DECODER_NEEDED_FRAME_NUM_MINUS_REF (1)
H264_DISP_NEEDED_FRAME_NUM_SCALE_DOWN (8)
H265_DECODER_NEEDED_FRAME_NUM_MINUS_REF (2)
 * */

#define VP9_DECODER_NEEDED_FRAME_NUM (4+0)
#define H264_DECODER_NEEDED_FRAME_NUM_MINUS_REF (1+0)
#define H264_DISP_NEEDED_FRAME_NUM_SCALE_DOWN (8+0)
#define H265_DECODER_NEEDED_FRAME_NUM_MINUS_REF (3+0)
/******************* decoder const fbm buffer end  *********************/

typedef struct DECODERINTERFACE DecoderInterface;
struct DECODERINTERFACE
{
    int  (*Init)(DecoderInterface* pSelf,
                 VConfig* pConfig,
                 VideoStreamInfo* pVideoInfo,
                 VideoFbmInfo* pFbmInfo);
    void (*Reset)(DecoderInterface* pSelf);
    int  (*SetSbm)(DecoderInterface* pSelf, SbmInterface* pSbm, int nIndex);
    int  (*GetFbmNum)(DecoderInterface* pSelf);
    Fbm* (*GetFbm)(DecoderInterface* pSelf, int nIndex);
    int  (*Decode)(DecoderInterface* pSelf,
                   int               bEndOfStream,
                   int               bDecodeKeyFrameOnly,
                   int               bSkipBFrameIfDelay,
                   int64_t           nCurrentTimeUs);
    void (*Destroy)(DecoderInterface* pSelf);
    int (*SetSpecialData)(DecoderInterface* pSelf, void *pArg);
    int (*SetExtraScaleInfo)(DecoderInterface* pSelf,
                             int nWidthTh,
                             int nHeightTh,
                             int nHorizonScaleRatio,
                             int nVerticalScaleRatio);
    int(*SetPerformCmd)(DecoderInterface* pSelf,
                        enum EVDECODERSETPERFORMCMD performCmd);
    int(*GetPerformInfo)(DecoderInterface* pSelf,
                         enum EVDECODERGETPERFORMCMD performCmd,
                         VDecodePerformaceInfo** performInfo);
};

typedef struct VIDEOENGINE
{
    VConfig           vconfig;
    VideoStreamInfo   videoStreamInfo;
    void*             pLibHandle;
    DecoderInterface* pDecoderInterface;
    int               bIsSoftwareDecoder;
    VideoFbmInfo      fbmInfo;
    u8                nResetVeMode;
    VeOpsS*           veOpsS;
    void*             pVeOpsSelf;
    uint64_t          nIcVeVersion;
    uint32_t          ndecIpVersion;

    SbmInterface*       pSbm[2];
    s32               bEnableGoogleVp9Flag;

    int nTrhandle;
    int nTrChannel;
    int bInitTrFlag;

}VideoEngine;

typedef DecoderInterface *VDecoderCreator(VideoEngine *);

int VDecoderRegister(enum EVIDEOCODECFORMAT format,
                  char *desc,
                  VDecoderCreator *creator,
                  int bIsSoft);

VideoEngine* VideoEngineCreate(VConfig* pVConfig,
                            VideoStreamInfo* pVideoInfo);

void VideoEngineDestroy(VideoEngine* pVideoEngine);

void VideoEngineReset(VideoEngine* pVideoEngine);

int VideoEngineSetSbm(VideoEngine* pVideoEngine, SbmInterface* pSbm, int nIndex);

int VideoEngineGetFbmNum(VideoEngine* pVideoEngine);

Fbm* VideoEngineGetFbm(VideoEngine* pVideoEngine, int nIndex);

int VideoEngineDecode(VideoEngine* pVideoEngine,
                      int          bEndOfStream,
                      int          bDecodeKeyFrameOnly,
                      int          bDropBFrameIfDelay,
                      int64_t      nCurrentTimeUs);

int VideoEngineRotatePicture(VideoEngine* pVideoEngine,
                                   VideoPicture* pPictureIn,
                                   VideoPicture* pPictureOut,
                                   int           nRotateDegree);

int GetBufferSize(int ePixelFormat,
               int nWidth,
               int nHeight,
               int* nYBufferSize,
               int *nCBufferSize,
               int* nYLineStride,
               int* nCLineStride,
               int nAlignValue);

int VideoEngineReopen(VideoEngine* pVideoEngine,
                            VConfig* pVConfig, VideoStreamInfo* pVideoInfo);

#endif

