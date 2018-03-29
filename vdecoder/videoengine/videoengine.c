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
* File : videoengine.c
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <malloc.h>
#include <dlfcn.h>
#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "videoengine.h"
#include "log.h"
#include "vdecoder_config.h"
#include "DecoderList.h"
#include "DecoderTypes.h"

#include "veAdapter.h"
#include "veInterface.h"
#include "../include/sunxi_tr.h"
#include "../ve/include/veAwTopRegDef.h"

#define DEBUG_SCLALE_DOWN 0

#define VIDEO_ENGINE_WIDTH_4K        3840
#define VIDEO_ENGINE_HEIGHT_4K        2160

#define TRANSFORM_DEV_TIMEOUT  50  //* ms , tr timeout value
static DecoderInterface* CreateSpecificDecoder(VideoEngine* p);


struct VDecoderNodeS
{
    DecoderListNodeT node;
    VDecoderCreator *creator;
    int  bIsSoftDecoder;
    char desc[64]; /* specific by mime type */
    enum EVIDEOCODECFORMAT format;

};

struct VDecoderListS
{
    DecoderListT list;
    int size;
    pthread_mutex_t mutex;
};

static struct VDecoderListS gVDecoderList = {{NULL, NULL}, 0, PTHREAD_MUTEX_INITIALIZER};

int VDecoderRegister(enum EVIDEOCODECFORMAT format,
        char *desc, VDecoderCreator *creator, int bIsSoft)
{
    struct VDecoderNodeS *newVDNode = NULL, *posVDNode = NULL;

    if(desc == NULL)
    {
        loge("register decoder, type name == NULL");
        return -1;
    }
    if(strlen(desc) > 63)
    {
        loge("type name '%s' too long", desc);
        return -1;
    }

    if (gVDecoderList.size == 0)
    {
        DecoderListInit(&gVDecoderList.list);
        pthread_mutex_init(&gVDecoderList.mutex, NULL);
    }

//    pthread_mutex_lock(&gVDecoderList.mutex);

    /* check if conflict */
    DecoderListForEachEntry(posVDNode, &gVDecoderList.list, node)
    {
        if (posVDNode->format == format && strcmp(posVDNode->desc, desc) == 0)
        {
            loge("Add '%x:%s' fail! '%x:%s' already register!", \
                format, desc, format, posVDNode->desc);
            return -1;
        }
    }
    logv("1117 register %x:%s", format, desc);
    newVDNode = malloc(sizeof(*newVDNode));
    newVDNode->creator = creator;
    strncpy(newVDNode->desc, desc, 63);
    newVDNode->format = format;
    newVDNode->bIsSoftDecoder = bIsSoft;

    DecoderListAdd(&newVDNode->node, &gVDecoderList.list);
    gVDecoderList.size++;

//    pthread_mutex_unlock(&gVDecoderList.mutex);
    logw("register codec: '%x:%s' success.", format, desc);
    return 0;
}


static void enableVe(VideoEngine* pVideoEngine)
{
    if(pVideoEngine->bIsSoftwareDecoder == 0)
    {
        CdcVeLock(pVideoEngine->veOpsS, pVideoEngine->pVeOpsSelf);
        CdcVeEnableVe(pVideoEngine->veOpsS, pVideoEngine->pVeOpsSelf);
    }

}

static void disableVe(VideoEngine* pVideoEngine)
{
    if(pVideoEngine->bIsSoftwareDecoder == 0)
    {
        CdcVeDisableVe(pVideoEngine->veOpsS, pVideoEngine->pVeOpsSelf);
        CdcVeUnLock(pVideoEngine->veOpsS, pVideoEngine->pVeOpsSelf);
    }

}

VideoEngine* VideoEngineCreate(VConfig* pVConfig, VideoStreamInfo* pVideoInfo)
{
    int          ret;
    VideoEngine* pEngine;
    int chipId;

    pEngine = (VideoEngine*)malloc(sizeof(VideoEngine));
    if(pEngine == NULL)
    {
        loge("memory alloc fail, VideoEngineCreate() return fail.");
        goto error_exit;
    }
    memset(pEngine, 0, sizeof(VideoEngine));
    memcpy(&pEngine->vconfig, pVConfig, sizeof(VConfig));
    memcpy(&pEngine->videoStreamInfo, pVideoInfo, sizeof(VideoStreamInfo));

    pEngine->nTrhandle   = -1;
    pEngine->nTrChannel  = 0;
    pEngine->bInitTrFlag = 0;
    pEngine->nResetVeMode = RESET_VE_NORMAL;
    VeConfig mVeConfig;
    memset(&mVeConfig, 0, sizeof(VeConfig));
    mVeConfig.nDecoderFlag = 1;
    mVeConfig.nEncoderFlag = 0;
    mVeConfig.nFormat      = pVideoInfo->eCodecFormat;
    mVeConfig.nWidth       = pVideoInfo->nWidth;
    mVeConfig.nResetVeMode = pEngine->nResetVeMode;
    mVeConfig.nVeFreq      = pVConfig->nVeFreq;

    int veOpsType =  VE_OPS_TYPE_NORMAL;
    pEngine->veOpsS = GetVeOpsS(veOpsType);
    if(pEngine->veOpsS == NULL)
    {
        loge("get veOps failed, type = %d",veOpsType);
        goto error_exit;
    }

    pEngine->pVeOpsSelf = CdcVeInit(pEngine->veOpsS,&mVeConfig);
    if(pEngine->pVeOpsSelf == NULL)
    {
        loge("init ve failed");
        goto error_exit;
    }

    //pVConfig->veOpsS = pEngine->veOpsS;
    //pVConfig->pVeOpsSelf = pEngine->pVeOpsSelf;

    pEngine->nIcVeVersion = CdcVeGetIcVeVersion(pEngine->veOpsS, pEngine->pVeOpsSelf);
    pEngine->ndecIpVersion = (uint32_t)(pEngine->nIcVeVersion >> 32);
    logd("*** pEngine->nIcVeVersion = %llx, decIpVersion = %x",
         (long long unsigned int)pEngine->nIcVeVersion,pEngine->ndecIpVersion);

    chipId = CdcVeGetChipId(pEngine->veOpsS, pEngine->pVeOpsSelf);

    if(pEngine->nIcVeVersion == 0x3101000012010
       && pVideoInfo->eCodecFormat == VIDEO_CODEC_FORMAT_VP9
       && (chipId!=CHIP_NOTSUPPORT_VP9_HW))
    {
        pEngine->bEnableGoogleVp9Flag = 1;
        CdcVeRelease(pEngine->veOpsS, pEngine->pVeOpsSelf);

        veOpsType =  VE_OPS_TYPE_VP9;
        pEngine->veOpsS = GetVeOpsS(veOpsType);
        if(pEngine->veOpsS == NULL)
        {
            loge("get veOps failed, type = %d",veOpsType);
            goto error_exit;
        }
        pEngine->pVeOpsSelf = CdcVeInit(pEngine->veOpsS,&mVeConfig);
        if(pEngine->pVeOpsSelf == NULL)
        {
            loge("init ve failed");
            goto error_exit;
        }

    }

    pEngine->vconfig.veOpsS     = pEngine->veOpsS;
    pEngine->vconfig.pVeOpsSelf = pEngine->pVeOpsSelf;
    pVConfig->veOpsS = pEngine->veOpsS;
    pVConfig->pVeOpsSelf = pEngine->pVeOpsSelf;

    chipId = CdcVeGetChipId(pEngine->veOpsS, pEngine->pVeOpsSelf);
    if(chipId == CHIP_H3s || chipId == CHIP_H2PLUS)
    {
        logd("chipId=%d\n", chipId);
        if(pVideoInfo->nWidth >= VIDEO_ENGINE_WIDTH_4K
           || pVideoInfo->nHeight >= VIDEO_ENGINE_HEIGHT_4K)
        {
                loge("unsurpport 4k video: %dx%d",pVideoInfo->nWidth,pVideoInfo->nHeight);
                goto error_exit;
        }
    }

    if(pVideoInfo->nCodecSpecificDataLen > 0 && pVideoInfo->pCodecSpecificData != NULL)
    {
        pEngine->videoStreamInfo.pCodecSpecificData = \
            (char*)malloc(pVideoInfo->nCodecSpecificDataLen);
        if(pEngine->videoStreamInfo.pCodecSpecificData == NULL)
        {
            loge("memory alloc fail, allocate %d bytes, VideoEngineCreate() return fail.",
                    pVideoInfo->nCodecSpecificDataLen);
            goto error_exit;

        }
        memcpy(pEngine->videoStreamInfo.pCodecSpecificData,
               pVideoInfo->pCodecSpecificData,
               pVideoInfo->nCodecSpecificDataLen);
    }

    pEngine->pDecoderInterface = CreateSpecificDecoder(pEngine);
    if(pEngine->pDecoderInterface == NULL)
    {
        goto error_exit;
    }

    if(pEngine->bIsSoftwareDecoder == 0)
    {
        CdcVeLock(pEngine->veOpsS, pEngine->pVeOpsSelf);

        //* reset ve hardware and set up top level registers.
        CdcVeReset(pEngine->veOpsS, pEngine->pVeOpsSelf);

        if(pEngine->vconfig.eOutputPixelFormat == PIXEL_FORMAT_DEFAULT)
        {
            pEngine->vconfig.eOutputPixelFormat = PIXEL_FORMAT_YV12;
        }
    }

    logd("**************eCtlAfcbMode = %d",pEngine->vconfig.eCtlAfbcMode);
    //* afbc just surpport yv12
    if(pEngine->ndecIpVersion == 0x31010
       && pEngine->videoStreamInfo.eCodecFormat == VIDEO_CODEC_FORMAT_H265
       && (pEngine->vconfig.eCtlAfbcMode == ENABLE_AFBC_JUST_BIG_SIZE
           || pEngine->vconfig.eCtlAfbcMode == ENABLE_AFBC_ALL_SIZE))
    {
        pEngine->vconfig.eOutputPixelFormat = PIXEL_FORMAT_YV12;
        //*when 265 not enable afbc in small size and the format is yv12,
        //*the alignStride must be more than 16, or will appear ve-timeout
        if(pEngine->vconfig.eCtlAfbcMode == ENABLE_AFBC_JUST_BIG_SIZE)
            pEngine->vconfig.nAlignStride = 32;
    }

    pEngine->vconfig.bIsSoftDecoderFlag = pEngine->bIsSoftwareDecoder;
    //* call specific function to open decoder.
    if(pEngine->bIsSoftwareDecoder == 0)
    {
        CdcVeEnableVe(pEngine->veOpsS, pEngine->pVeOpsSelf);
    }

    pEngine->fbmInfo.bIs3DStream = pEngine->videoStreamInfo.bIs3DStream;   // added by xyliu
    pEngine->fbmInfo.bIsFrameCtsTestFlag = pEngine->videoStreamInfo.bIsFrameCtsTestFlag;//cts
    pEngine->fbmInfo.nExtraFbmBufferNum = pEngine->vconfig.nDeInterlaceHoldingFrameBufferNum +
                                          pEngine->vconfig.nDisplayHoldingFrameBufferNum +
                                          pEngine->vconfig.nRotateHoldingFrameBufferNum +
                                          pEngine->vconfig.nDecodeSmoothFrameBufferNum;
    if(pEngine->fbmInfo.nExtraFbmBufferNum > 16)
        pEngine->fbmInfo.nExtraFbmBufferNum = 16;
    if(pEngine->fbmInfo.nExtraFbmBufferNum < 0)
    {
        pEngine->fbmInfo.nExtraFbmBufferNum = 0;
        logw(" extra fbm buffer == 0 ");
    }
#if DEBUG_SCLALE_DOWN
    {
        pEngine->vconfig.bScaleDownEn = 1;
        pEngine->vconfig.nHorizonScaleDownRatio = 1;
        pEngine->vconfig.nVerticalScaleDownRatio = 1;
    }
#endif
    ret = pEngine->pDecoderInterface->Init(pEngine->pDecoderInterface,
                                           &pEngine->vconfig,
                                           &pEngine->videoStreamInfo,
                                           &pEngine->fbmInfo);


    disableVe(pEngine);

    if(ret != VDECODE_RESULT_OK)
    {
        loge("initial specific decoder fail.");
        goto error_exit;

    }
    return pEngine;

error_exit:

    if(pEngine)
    {
        if(pEngine->videoStreamInfo.pCodecSpecificData != NULL
           && pEngine->videoStreamInfo.nCodecSpecificDataLen > 0)
        {
            free(pEngine->videoStreamInfo.pCodecSpecificData);
            pEngine->videoStreamInfo.pCodecSpecificData = NULL;
        }

        if(pEngine->veOpsS)
        {
            CdcVeRelease(pEngine->veOpsS, pEngine->pVeOpsSelf);
        }
        free(pEngine);
    }
    return NULL;

}

void VideoEngineDestroy(VideoEngine* pVideoEngine)
{
    //*warning: must destory by the order: pDecoderInterface, sbm, ve

    //* close tr driver
    if(pVideoEngine->bInitTrFlag == 1)
    {
        if(pVideoEngine->nTrhandle > 0)
        {
            if(pVideoEngine->nTrChannel != 0)
            {
                size_addr arg[6] = {0};
                arg[0] = pVideoEngine->nTrChannel;
                logd("release channel start : 0x%x , %d",pVideoEngine->nTrChannel,
                                                         pVideoEngine->nTrhandle);
                if(ioctl(pVideoEngine->nTrhandle,TR_RELEASE,(void*)arg) != 0)
                {
                    loge("#### release channel failed!");
                    //return;
                }
                logd("release channel finish");
                pVideoEngine->nTrChannel = 0;
            }

            close(pVideoEngine->nTrhandle);
            pVideoEngine->nTrhandle = -1;

        }
    }

    //* close specific decoder.
    enableVe(pVideoEngine);

    pVideoEngine->pDecoderInterface->Destroy(pVideoEngine->pDecoderInterface);

    //* Destroy the stream buffer manager.
    if(pVideoEngine->pSbm[0] != NULL)
       SbmDestroy(pVideoEngine->pSbm[0]);

    if(pVideoEngine->pSbm[1] != NULL)
       SbmDestroy(pVideoEngine->pSbm[1]);

    disableVe(pVideoEngine);

    //* free codec specific data.
    if(pVideoEngine->videoStreamInfo.pCodecSpecificData != NULL &&
        pVideoEngine->videoStreamInfo.nCodecSpecificDataLen > 0)
    {
        free(pVideoEngine->videoStreamInfo.pCodecSpecificData);
        pVideoEngine->videoStreamInfo.pCodecSpecificData = NULL;
    }

    //* if use other decoder library, close the library.
    if(pVideoEngine->pLibHandle != NULL)
        dlclose(pVideoEngine->pLibHandle);

    if(pVideoEngine->veOpsS)
        CdcVeRelease(pVideoEngine->veOpsS, pVideoEngine->pVeOpsSelf);

    free(pVideoEngine);

    return;
}

int VideoEngineReopen(VideoEngine* pVideoEngine,VConfig* pVConfig, VideoStreamInfo* pVideoInfo)
{
    int ret = 0;
    int chipId = 0;

    //* close specific decoder.
    enableVe(pVideoEngine);

    //* free codec specific data.
    if(pVideoEngine->videoStreamInfo.pCodecSpecificData != NULL &&
        pVideoEngine->videoStreamInfo.nCodecSpecificDataLen > 0)
   {
           free(pVideoEngine->videoStreamInfo.pCodecSpecificData);
        pVideoEngine->videoStreamInfo.pCodecSpecificData = NULL;
   }

    pVideoEngine->pDecoderInterface->Destroy(pVideoEngine->pDecoderInterface);

    chipId = CdcVeGetChipId(pVideoEngine->veOpsS, pVideoEngine->pVeOpsSelf);
    if(chipId == CHIP_H3s || chipId == CHIP_H2PLUS)
    {
        logd("chipId=%d\n", chipId);
        if(pVideoInfo->nWidth >= VIDEO_ENGINE_WIDTH_4K
           || pVideoInfo->nHeight >= VIDEO_ENGINE_HEIGHT_4K)
        {
                loge("unsurpport 4k video: %dx%d",pVideoInfo->nWidth,pVideoInfo->nHeight);
                goto error_exit;
        }
    }

    if(pVideoInfo->nCodecSpecificDataLen > 0 && pVideoInfo->pCodecSpecificData != NULL)
    {
        pVideoEngine->videoStreamInfo.pCodecSpecificData = \
            (char*)malloc(pVideoInfo->nCodecSpecificDataLen);
        if(pVideoEngine->videoStreamInfo.pCodecSpecificData == NULL)
        {
            loge("memory alloc fail, allocate %d bytes, VideoEngineCreate() return fail.",
                    pVideoInfo->nCodecSpecificDataLen);
            goto error_exit;

        }
        memcpy(pVideoEngine->videoStreamInfo.pCodecSpecificData,
               pVideoInfo->pCodecSpecificData,
               pVideoInfo->nCodecSpecificDataLen);
    }
    else
    {
        pVideoEngine->videoStreamInfo.pCodecSpecificData = NULL;
        pVideoEngine->videoStreamInfo.nCodecSpecificDataLen = 0;
    }

    pVideoEngine->fbmInfo.nValidBufNum = 0;
    pVideoEngine->fbmInfo.pMajorDispFrame = NULL;
    pVideoEngine->fbmInfo.pMajorDecoderFrame = NULL;
    pVideoEngine->fbmInfo.pFbmFirst = NULL;
    pVideoEngine->fbmInfo.pFbmSecond = NULL;
    memset(&pVideoEngine->fbmInfo.pFbmBufInfo, 0, sizeof(FbmBufInfo));

    pVideoEngine->pDecoderInterface = CreateSpecificDecoder(pVideoEngine);
    if(pVideoEngine->pDecoderInterface == NULL)
    {
        goto error_exit;
    }

    if(pVideoEngine->bIsSoftwareDecoder == 0)
    {
        //* reset ve hardware and set up top level registers.
        CdcVeReset(pVideoEngine->veOpsS, pVideoEngine->pVeOpsSelf);

        if(pVideoEngine->vconfig.eOutputPixelFormat == PIXEL_FORMAT_DEFAULT)
        {
            pVideoEngine->vconfig.eOutputPixelFormat = PIXEL_FORMAT_YV12;
        }
    }

    ret = pVideoEngine->pDecoderInterface->Init(pVideoEngine->pDecoderInterface,
                                           &pVideoEngine->vconfig,
                                           &pVideoEngine->videoStreamInfo,
                                           &pVideoEngine->fbmInfo);
    disableVe(pVideoEngine);

    if(ret != VDECODE_RESULT_OK)
    {
        loge("initial specific decoder fail.");
        return -1;

    }
    return 0;
error_exit:
    disableVe(pVideoEngine);
    return -1;
}

void VideoEngineReset(VideoEngine* pVideoEngine)
{
    enableVe(pVideoEngine);

    pVideoEngine->pDecoderInterface->Reset(pVideoEngine->pDecoderInterface);

    disableVe(pVideoEngine);
    return;
}

int VideoEngineSetSbm(VideoEngine* pVideoEngine, SbmInterface* pSbm, int nIndex)
{
    int ret;

    enableVe(pVideoEngine);

    if(nIndex < 2)
        pVideoEngine->pSbm[nIndex] = pSbm;

    ret = pVideoEngine->pDecoderInterface->SetSbm(pVideoEngine->pDecoderInterface,
                                                  pSbm,
                                                  nIndex);
    disableVe(pVideoEngine);
    return ret;
}

int VideoEngineGetFbmNum(VideoEngine* pVideoEngine)
{
    int ret;
    ret = pVideoEngine->pDecoderInterface->GetFbmNum(pVideoEngine->pDecoderInterface);
    return ret;
}

Fbm* VideoEngineGetFbm(VideoEngine* pVideoEngine, int nIndex)
{
    Fbm* pFbm;
    pFbm = pVideoEngine->pDecoderInterface->GetFbm(pVideoEngine->pDecoderInterface, nIndex);
    return pFbm;
}


#define VIDEO_ENGINE_CREATE_DECODER(lib,function)   \
{                                                           \
    do{                                                     \
        FUNC_CREATE_DECODER pFunc;                          \
        p->pLibHandle = dlopen(lib, RTLD_NOW);   \
        if(p->pLibHandle == NULL)                           \
        {                                                   \
            pInterface = NULL;                              \
            break;                                          \
        }                                                   \
        pFunc = (FUNC_CREATE_DECODER)dlsym(p->pLibHandle, function);\
        pInterface = pFunc(p);                              \
    }while(0);                                              \
}

int VideoEngineDecode(VideoEngine* pVideoEngine,
                      int          bEndOfStream,
                      int          bDecodeKeyFrameOnly,
                      int          bDropBFrameIfDelay,
                      int64_t      nCurrentTimeUs)
{
    int ret;
    enableVe(pVideoEngine);

    ret = pVideoEngine->pDecoderInterface->Decode(pVideoEngine->pDecoderInterface,
                                                  bEndOfStream,
                                                  bDecodeKeyFrameOnly,
                                                  bDropBFrameIfDelay,
                                                  nCurrentTimeUs);

    disableVe(pVideoEngine);
    return ret;
}

typedef DecoderInterface* (*FUNC_CREATE_DECODER)(VideoEngine*);

/*
 * CheckVeVersionAndLib
 * return
 *          0, lib is ok
 *         -1, need to search next lib
 * */
#define VIDEO_ENGINE_CHECK_OK 0
#define VIDEO_ENGINE_CHECK_FAIL -1
static int CheckVeVersionAndLib(struct VDecoderNodeS *posVDNode, VideoEngine* p)
{
    int ret = VIDEO_ENGINE_CHECK_FAIL;
    int eCodecFormat;

    eCodecFormat = (int)posVDNode->format;
    switch(eCodecFormat)
    {
        case VIDEO_CODEC_FORMAT_H264:
        {
            p->fbmInfo.nDecoderNeededMiniFbmNum = H264_DECODER_NEEDED_FRAME_NUM_MINUS_REF;
            p->fbmInfo.nDecoderNeededMiniFbmNumSD = H264_DISP_NEEDED_FRAME_NUM_SCALE_DOWN;
            ret = VIDEO_ENGINE_CHECK_OK;
            break;
        }
        case VIDEO_CODEC_FORMAT_H265:
        {
            int bIsH265Soft = posVDNode->bIsSoftDecoder;
            p->fbmInfo.nDecoderNeededMiniFbmNum = H265_DECODER_NEEDED_FRAME_NUM_MINUS_REF;
            p->fbmInfo.nDecoderNeededMiniFbmNumSD = H265_DECODER_NEEDED_FRAME_NUM_MINUS_REF;
            ret = (bIsH265Soft == 0) ? VIDEO_ENGINE_CHECK_OK : VIDEO_ENGINE_CHECK_FAIL;
            break;
        }

        case VIDEO_CODEC_FORMAT_VP9:
        {
            int bIsVp9Soft = posVDNode->bIsSoftDecoder;
            if(p->bEnableGoogleVp9Flag == 1)
                ret = (bIsVp9Soft==1)? VIDEO_ENGINE_CHECK_FAIL : VIDEO_ENGINE_CHECK_OK;
            else
                ret = (bIsVp9Soft==1)? VIDEO_ENGINE_CHECK_OK : VIDEO_ENGINE_CHECK_FAIL;

            logd("****gVeVersion=%x,ret=%d,posVDNode->bIsSoftDecoder is:%d\n",
                p->ndecIpVersion, ret,posVDNode->bIsSoftDecoder);

            p->fbmInfo.nDecoderNeededMiniFbmNum = VP9_DECODER_NEEDED_FRAME_NUM;
            p->fbmInfo.nDecoderNeededMiniFbmNumSD = VP9_DECODER_NEEDED_FRAME_NUM;
            break;
        }
        default:
        {
            break;
        }
    }
    return ret;
}

static DecoderInterface* CreateSpecificDecoder(VideoEngine* p)
{
    DecoderInterface* pInterface = NULL;
    struct VDecoderNodeS *posVDNode = NULL;
    int bCheckFlag = VIDEO_ENGINE_CHECK_FAIL;

    if(gVDecoderList.size == 0)
    {
        loge("format '%x' support!", p->videoStreamInfo.eCodecFormat);
        return NULL;
    }

    DecoderListForEachEntry(posVDNode, &gVDecoderList.list, node)
    {
        if((int)posVDNode->format == p->videoStreamInfo.eCodecFormat)
        {
            bCheckFlag = CheckVeVersionAndLib(posVDNode, p);
            if(bCheckFlag == VIDEO_ENGINE_CHECK_OK)
            {
                logd("Create decoder '%x:%s'", posVDNode->format, posVDNode->desc);
                p->bIsSoftwareDecoder = posVDNode->bIsSoftDecoder;
                pInterface = posVDNode->creator(p);
                return pInterface;
            }
        }
    }

    loge("format '%x' support!", p->videoStreamInfo.eCodecFormat);
    return NULL;
}

int GetBufferSize(int ePixelFormat, int nWidth, int nHeight,
        int*nYBufferSize, int *nCBufferSize, int* nYLineStride,
        int* nCLineStride, int nAlignValue)
{
    int   nHeight16Align;
    int   nHeight32Align;
    int   nHeight64Align;
    int   nLineStride;
    int   nMemSizeY;
    int   nMemSizeC;

    nHeight16Align = (nHeight+15) & ~15;
    nHeight32Align = (nHeight+31) & ~31;
    nHeight64Align = (nHeight+63) & ~63;

    switch(ePixelFormat)
    {
        case PIXEL_FORMAT_YUV_PLANER_420:
        case PIXEL_FORMAT_YUV_PLANER_422:
        case PIXEL_FORMAT_YUV_PLANER_444:
        case PIXEL_FORMAT_YV12:
        case PIXEL_FORMAT_NV21:

            //* for decoder,
            //* height of Y component is required to be 16 aligned,
            //* for example, 1080 becomes to 1088.
            //* width and height of U or V component are both required to be 8 aligned.
            //* nLineStride should be 16 aligned.
            if(nAlignValue == 0)
            {
                nLineStride = (nWidth+15) &~ 15;
                nMemSizeY = nLineStride*nHeight16Align;
            }
            else
            {
                nLineStride = (nWidth+nAlignValue-1) &~ (nAlignValue-1);
                nHeight = (nHeight+nAlignValue-1) &~ (nAlignValue-1);
                nMemSizeY = nLineStride*nHeight;
            }
            if(ePixelFormat == PIXEL_FORMAT_YUV_PLANER_420 ||
               ePixelFormat == PIXEL_FORMAT_YV12 ||
               ePixelFormat == PIXEL_FORMAT_NV21)
                nMemSizeC = nMemSizeY>>2;
            else if(ePixelFormat == PIXEL_FORMAT_YUV_PLANER_422)
                nMemSizeC = nMemSizeY>>1;
            else
                nMemSizeC = nMemSizeY;  //* PIXEL_FORMAT_YUV_PLANER_444
            break;

        case PIXEL_FORMAT_YUV_MB32_420:
        case PIXEL_FORMAT_YUV_MB32_422:
        case PIXEL_FORMAT_YUV_MB32_444:
            //* for decoder,
            //* height of Y component is required to be 32 aligned.
            //* height of UV component are both required to be 32 aligned.
            //* nLineStride should be 32 aligned.
            nLineStride = (nWidth+63) &~ 63;
            nMemSizeY = nLineStride*nHeight32Align;

            if(ePixelFormat == PIXEL_FORMAT_YUV_MB32_420)
                nMemSizeC = nLineStride*nHeight64Align/4;
            else if(ePixelFormat == PIXEL_FORMAT_YUV_MB32_422)
                nMemSizeC = nLineStride*nHeight64Align/2;
            else
                nMemSizeC = nLineStride*nHeight64Align;
            break;

        case PIXEL_FORMAT_RGBA:
        case PIXEL_FORMAT_ARGB:
        case PIXEL_FORMAT_ABGR:
        case PIXEL_FORMAT_BGRA:
//            nLineStride = (nWidth+3) &~ 3;
            nLineStride = nWidth *4;

            nMemSizeY = nLineStride*nHeight;
            nMemSizeC = 0;

            break;

        default:
            loge("pixel format incorrect, ePixelFormat=%d", ePixelFormat);
            return -1;
    }
    if(nYBufferSize != NULL)
    {
        *nYBufferSize = nMemSizeY;
    }
    if(nCBufferSize != NULL)
    {
        *nCBufferSize = nMemSizeC;
    }
    if(nYLineStride != NULL)
    {
        *nYLineStride = nLineStride;
    }
    if(nCLineStride != NULL)
    {
        *nCLineStride = nLineStride>>1;
    }
    return 0;
}


/* /proc/[pid]/maps */

static int GetLocalPathFromProcessMaps(char *localPath, int len)
{
#define LOCAL_LIB "libvdecoder.so"
#define LOCAL_LINUX_LIB "libcdc_vdecoder.so"

    char path[512] = {0};
    char line[1024] = {0};
    FILE *file = NULL;
    char *strLibPos = NULL;
    int ret = -1;

    memset(localPath, 0x00, len);

    sprintf(path, "/proc/%d/maps", getpid());
    file = fopen(path, "r");
    if (file == NULL)
    {
        loge("fopen failure, errno(%d)", errno);
        ret = -1;
        goto out;
    }

    while (fgets(line, 1023, file) != NULL)
    {
        if ((strLibPos = strstr(line, LOCAL_LIB)) != NULL ||
            (strLibPos = strstr(line, LOCAL_LINUX_LIB)) != NULL)
        {
            char *rootPathPos = NULL;
            int localPathLen = 0;
            rootPathPos = strchr(line, '/');
            if (rootPathPos == NULL)
            {
                loge("some thing error, cur line '%s'", line);
                ret = -1;
                goto out;
            }

            localPathLen = strLibPos - rootPathPos - 1;
            if (localPathLen > len -1)
            {
                loge("localPath too long :%s ", localPath);
                ret = -1;
                goto out;
            }

            memcpy(localPath, rootPathPos, localPathLen);
            ret = 0;
            goto out;
        }
    }
    loge("Are you kidding? not found?");

out:
    if (file)
    {
        fclose(file);
    }
    return ret;
}

typedef void VDPluginEntry(void);

void AddVDPluginSingle(char *lib)
{
    void *libFd = NULL;
    if(lib == NULL)
    {
        loge(" open lib == NULL ");
        return;
    }

    libFd = dlopen(lib, RTLD_NOW);

    VDPluginEntry *PluginInit = NULL;

    if (libFd == NULL)
    {
        loge("dlopen '%s' fail: %s", lib, dlerror());
        return ;
    }

    PluginInit = dlsym(libFd, "CedarPluginVDInit");
    if (PluginInit == NULL)
    {
        logw("Invalid plugin, CedarPluginVDInit not found.");
        return;
    }
    logd("vdecoder open lib: %s", lib);
    PluginInit(); /* init plugin */
    return ;
}

/* executive when load */
//static void AddVDPlugin(void) __attribute__((constructor));
void AddVDPlugin(void)
{
    CEDARC_UNUSE(AddVDPlugin);
    char localPath[512];
    char slash[4] = "/";
    char loadLib[512];
    struct dirent **namelist = NULL;
    int num = 0, index = 0;
    int pathLen = 0;
    int ret;

    memset(localPath, 0, 512);
    memset(loadLib, 0, 512);
//scan_local_path:
    ret = GetLocalPathFromProcessMaps(localPath, 512);
    if (ret != 0)
    {
        logw("get local path failure, scan /system/lib ");
        goto scan_system_lib;
    }

    num = scandir(localPath, &namelist, NULL, NULL);
    if (num <= 0)
    {
        logw("scandir failure, errno(%d), scan /system/lib ", errno);
        goto scan_system_lib;
    }
    strcat(localPath, slash);
    pathLen = strlen(localPath);
    strcpy(loadLib, localPath);
    logw("1117 get local path: %s", localPath);
    for(index = 0; index < num; index++)
    {
        if(((strstr((namelist[index])->d_name, "libaw") != NULL) ||
            (strstr((namelist[index])->d_name, "libcdc_vd") != NULL) ||
            (strstr((namelist[index])->d_name, "librv") != NULL))
            && (strstr((namelist[index])->d_name, ".so") != NULL))
        {
            loadLib[pathLen] = '\0';
            strcat(loadLib, (namelist[index])->d_name);
            logw(" 1117 load so: %s ", loadLib);
            AddVDPluginSingle(loadLib);
        }
        free(namelist[index]);
        namelist[index] = NULL;
    }

scan_system_lib:
    // TODO: scan /system/lib

    return;
}

