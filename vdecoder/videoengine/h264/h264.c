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
 * File : h264.c
 * Description : h264
 * History :
 *
 */

/*
*
* File : h264.c
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#include "h264_dec.h"
#include "h264_hal.h"
#include "h264_func.h"


//*******************************************************************//
//*******************************************************************//

DecoderInterface* CreateH264Decoder(VideoEngine* p)
{
    H264DecCtx* h264DecCtx = NULL;

    h264DecCtx = (H264DecCtx*)malloc(sizeof(H264DecCtx));
    if(h264DecCtx == NULL)
    {
        return NULL;
    }

    memset(h264DecCtx, 0, sizeof(H264DecCtx));

    h264DecCtx->pVideoEngine        = p;
    h264DecCtx->interface.Init      = H264DecoderInit;
    h264DecCtx->interface.Reset     = H264DecoderRest;
    h264DecCtx->interface.SetSbm    = H264DecoderSetSbm;
    h264DecCtx->interface.GetFbmNum = H264DecoderGetFbmNum;
    h264DecCtx->interface.GetFbm    = H264DecoderGetFbm;
    h264DecCtx->interface.Decode = H264DecoderDecode;
    h264DecCtx->interface.Destroy   = Destroy;
    h264DecCtx->interface.SetExtraScaleInfo = H264DecoderSetExtraScaleInfo;
    h264DecCtx->interface.SetPerformCmd = H264DecoderSetPerformCmd;
    h264DecCtx->interface.GetPerformInfo = H264DecodeGetPerformInfo;
    return &h264DecCtx->interface;
}

void CedarPluginVDInit(void)
{
    CEDARC_UNUSE(CedarPluginVDInit);
    int ret;
    ret = VDecoderRegister(VIDEO_CODEC_FORMAT_H264, "h264", CreateH264Decoder, 0);

    if (0 == ret)
    {
        logi("register h264 decoder success!");
    }
    else
    {
        loge("register h264 decoder failure!!!");
    }
    return ;
}

//*******************************************************************//
//*******************************************************************//

static void H264DebugSaveExtraData(H264Context* hCtx)
{
    CEDARC_UNUSE(H264DebugSaveExtraData);
    char *buf = (char *)hCtx->pExtraDataBuf;
    s32 nSize = hCtx->nExtraDataLen;
    FILE *fp = fopen(AVC_SAVE_STREAM_PATH, "ab");
    if(fp != NULL)
    {
        fwrite(buf, 1, nSize, fp);
        fclose(fp);
        logd(" h264 decoder saving extra data ok, size: %d", nSize);
    }
    else
    {
        loge(" h264 decoder saving extra data open file error ");
    }
}

//************************************************************************************/
//      Name:        ve_open                                                          //
//      Prototype:    Handle ve_open (vconfig_t* config, videoStreamInfo_t* stream_info); //
//      Function:    Start up the VE CSP.                                             //
//      Return:    A handle of the VE device.                                           //
//      Input:    vconfig_t* config, the configuration for the VE CSP.                 //
//************************************************************************************/

int H264ProcessExtraData(H264DecCtx* h264DecCtx,  H264Context* hCtx)
{
    // process extra data
    H264Dec* h264Dec = NULL;
    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;

    hCtx->pExtraDataBuf = (u8*)CdcMemPalloc(h264Dec->memops,H264VDEC_MAX_EXTRA_DATA_LEN,
                                        (void *)h264DecCtx->vconfig.veOpsS,
                                        h264DecCtx->vconfig.pVeOpsSelf);
    if(hCtx->pExtraDataBuf == NULL)
    {
        return VDECODE_RESULT_UNSUPPORTED;
    }
    CdcMemSet(h264Dec->memops,hCtx->pExtraDataBuf, 0, H264VDEC_MAX_EXTRA_DATA_LEN);
    CdcMemFlushCache(h264Dec->memops,hCtx->pExtraDataBuf, H264VDEC_MAX_EXTRA_DATA_LEN);
    if(h264DecCtx->videoStreamInfo.pCodecSpecificData[0] == 0x01)
    {
        hCtx->bIsAvc = 1;
        if(h264DecCtx->videoStreamInfo.nCodecSpecificDataLen >= 7)
        {
            H264DecodeExtraData(h264Dec, hCtx, (u8*)h264DecCtx->videoStreamInfo.pCodecSpecificData);
#if H264_DEBUG_SAVE_BIT_STREAM
            H264DebugSaveExtraData(hCtx);
#endif
        }
    }
    else
    {
        hCtx->bIsAvc = 0;
        if(h264DecCtx->videoStreamInfo.nCodecSpecificDataLen > H264VDEC_MAX_EXTRA_DATA_LEN)
        {
            //LOGD("the extra data len is %d,
            //larger than the H264VDEC_MAX_EXTRA_DATA_LEN",
            //h264DecCtx->vStreamInfo.init_data_len);
        }
        CdcMemWrite(h264Dec->memops,hCtx->pExtraDataBuf,
                    h264DecCtx->videoStreamInfo.pCodecSpecificData,
                    h264DecCtx->videoStreamInfo.nCodecSpecificDataLen);
        hCtx->nExtraDataLen = h264DecCtx->videoStreamInfo.nCodecSpecificDataLen;
    }
    CdcMemFlushCache(h264Dec->memops,hCtx->pExtraDataBuf, H264VDEC_MAX_EXTRA_DATA_LEN);
    return VDECODE_RESULT_OK;
}

s32 H264PallocBufferBeforeDecode(H264DecCtx* h264DecCtx, H264Context* pHContext)
{
    s32 ret = VDECODE_RESULT_OK;
    if(pHContext->frmBufInf.nMaxValidFrmBufNum == 0)
    {
        pHContext->nFrmMbHeight = (h264DecCtx->videoStreamInfo.nHeight+15)&~15;
        pHContext->nFrmMbWidth = (h264DecCtx->videoStreamInfo.nWidth+15)&~15;
        pHContext->bFrameMbsOnlyFlag = 1;
        pHContext->nMbHeight = pHContext->nFrmMbHeight/16;
        pHContext->nMbWidth = pHContext->nFrmMbWidth/16;
        pHContext->bProgressice = 1;

        ret = H264MallocFrmBuffer(h264DecCtx, pHContext);
        if(ret != VDECODE_RESULT_OK)
        {
            logd("*********malloc buffer error\n");
            return ret;
        }
    }
    return ret;
}

int H264DecoderInit(DecoderInterface* pSelf,
                    VConfig* pConfig,
                    VideoStreamInfo* pVideoInfo,
                    VideoFbmInfo* pFbmInfo)
{
    H264DecCtx* h264DecCtx = NULL;
    H264Dec*h264DecHandle = NULL;
    H264MvcContext* pMVCContext = NULL;
    s32 ret = 0;

    h264DecCtx = (H264DecCtx*)pSelf;

    memcpy(&h264DecCtx->vconfig, pConfig, sizeof(VConfig));
    memcpy(&h264DecCtx->videoStreamInfo, pVideoInfo, sizeof(VideoStreamInfo));
    h264DecCtx->pFbmInfo = pFbmInfo;           //added by xyliu

    if(h264DecCtx->vconfig.bDisable3D == 1)
    {
        h264DecCtx->videoStreamInfo.bIs3DStream = 0;
    }

    h264DecHandle = (H264Dec*)malloc(sizeof(H264Dec));
    if(h264DecHandle == NULL)
    {
        goto h264_open_error;
    }
    memset(h264DecHandle, 0, sizeof(H264Dec));

    h264DecCtx->pH264Dec = (void*)h264DecHandle;

    //add for big_array
    h264DecHandle->dataBuffer = (u8*)malloc(4096*sizeof(u8));
    if(h264DecHandle->dataBuffer == NULL)
    {
        goto h264_open_error;
    }

    h264DecHandle->nRegisterBaseAddr = (size_addr)CdcVeGetGroupRegAddr(h264DecCtx->vconfig.veOpsS,
                                                            h264DecCtx->vconfig.pVeOpsSelf,
                                                            REG_GROUP_H264_DECODER);
    h264DecHandle->nVeVersion =  CdcVeGetIcVeVersion(h264DecCtx->vconfig.veOpsS,
                                                     h264DecCtx->vconfig.pVeOpsSelf);

    h264DecHandle->pHContext = (H264Context*)malloc(sizeof(H264Context));
    if(h264DecHandle->pHContext == NULL)
    {
        //LOGD("malloc memory for h264Dec->pHContext failed\n");
        goto h264_open_error;
    }

    if(H264InitDecode(h264DecCtx, h264DecHandle, h264DecHandle->pHContext) < 0)
    {
        goto h264_open_error;
    }

    H264ResetDecoderParams(h264DecHandle->pHContext);
    if(h264DecCtx->videoStreamInfo.pCodecSpecificData!= NULL)
    {
        h264DecHandle->pHContext->bDecExtraDataFlag = 1;
    }

    if(h264DecCtx->videoStreamInfo.bIs3DStream == 1)
    {
        h264DecHandle->pMinorHContext = (H264Context*)malloc(sizeof(H264Context));
        if(h264DecHandle->pMinorHContext == NULL)
        {
            goto h264_open_error;
        }
        if(H264InitDecode(h264DecCtx,h264DecHandle, h264DecHandle->pMinorHContext) < 0)
        {
            goto h264_open_error;
        }
        pMVCContext = malloc(sizeof(H264MvcContext));
        if(pMVCContext == NULL)
        {
            goto h264_open_error;
        }
        pMVCContext->pMvcParamBuf = malloc(sizeof(H264MvcSliceExtParam));
        if(pMVCContext->pMvcParamBuf == NULL)
        {
            goto h264_open_error;
        }

        h264DecHandle->pMinorHContext->pMvcContext = (void*)pMVCContext;

        H264ResetDecoderParams(h264DecHandle->pMinorHContext);
        h264DecHandle->pMinorHContext->bDecExtraDataFlag = 0;
        h264DecHandle->pMinorHContext->nExtraDataLen = 0;
        h264DecHandle->pMinorHContext->pExtraDataBuf = NULL;
        h264DecHandle->pMinorHContext->bIsAvc        = h264DecHandle->pHContext->bIsAvc;
    }

/********************************************************************************
 ******************** H264PallocBufferBeforeDecode onlay used by xuqi***********/
    if((h264DecCtx->vconfig.bSupportPallocBufBeforeDecode==1) &&
        (h264DecCtx->videoStreamInfo.nWidth>0) &&
        (h264DecCtx->videoStreamInfo.nHeight>0))
    {
        ret = H264PallocBufferBeforeDecode(h264DecCtx, h264DecHandle->pHContext);
        if(ret != VDECODE_RESULT_OK)
        {
            return ret;
        }
        if(h264DecCtx->videoStreamInfo.bIs3DStream == 1)
        {
            ret = H264PallocBufferBeforeDecode(h264DecCtx, h264DecHandle->pMinorHContext);
            if(ret != VDECODE_RESULT_OK)
            {
                return ret;
            }
        }
    }

    //added by xyliu at 2017-08-21 for decode sps pps and slice header by software
    h264DecCtx->bSoftDecHeader = 0;

    if(h264DecCtx->bSoftDecHeader == 0)
    {
        h264DecCtx->GetBits = H264GetBits;
        h264DecCtx->GetUeGolomb = H264GetUeGolomb;
        h264DecCtx->ShowBits = H264ShowBits;
        h264DecCtx->GetSeGolomb = H264GetSeGolomb;
        h264DecCtx->InitGetBits = NULL;
    }
    else
    {
        h264DecCtx->GetBits = H264GetBitsSw;
        h264DecCtx->GetUeGolomb = H264GetUeGolombSw;
        h264DecCtx->ShowBits = H264ShowBitsSw;
        h264DecCtx->GetSeGolomb = H264GetSeGolombSw;
        h264DecCtx->InitGetBits = H264InitGetBitsSw;
        h264DecHandle->pDecHeaderBuf = malloc(H264_HEADER_BUF_SIZE);
        if(h264DecHandle->pDecHeaderBuf == NULL)
        {
            goto h264_open_error;
        }
        h264DecHandle->nDecHeaderBufSize = H264_HEADER_BUF_SIZE;
    }
    return VDECODE_RESULT_OK;

h264_open_error:
    if(h264DecHandle != NULL)
    {
        if(h264DecHandle->pHContext != NULL)
        {
            H264FreeMemory(h264DecHandle, h264DecHandle->pHContext);
        }
        if(h264DecHandle->pMinorHContext != NULL)
        {
            H264FreeMemory(h264DecHandle, h264DecHandle->pMinorHContext);
        }
        free(h264DecHandle);
        h264DecHandle = NULL;
        h264DecCtx->pH264Dec = NULL;
    }
    if(h264DecCtx != NULL)
    {
        free(h264DecCtx);
        h264DecCtx = NULL;
    }
    return VDECODE_RESULT_UNSUPPORTED;
}

/*******************************************************************************/
//       Name:    ve_close                                                                  //
//       Prototype: vresult_e ve_close (u8 flush_pictures);                    //
//       Function:    Close the VE CSP.                                                     //
//       Return:    VDECODE_RESULT_OK: success.                                                  //
//         VRESULT_ERR_LIBRARY_NOT_OPEN: the CSP is not opened yet.                         //
/*******************************************************************************/

void H264FlushDelayedPictures(H264DecCtx* h264DecCtx, H264Context* hCtx)
{
    s32 outIdx=0;
    s32 i = 0;
    s32 bDispFrameFlag = 0;

    while(1)
    {
        H264PicInfo* outPicPtr = hCtx->frmBufInf.pDelayedPic[0];
        outIdx = 0;

        if(outPicPtr != NULL)
        {
            for(i=1; hCtx->frmBufInf.pDelayedPic[i] &&
                !hCtx->frmBufInf.pDelayedPic[i]->bKeyFrame; i++)
            {
                if((hCtx->frmBufInf.pDelayedPic[i]->pVPicture==NULL) &&
                    (hCtx->frmBufInf.pDelayedPic[i]->pScaleDownVPicture==NULL))
                {
                    hCtx->frmBufInf.pDelayedPic[i] = NULL;
                }
                else if(hCtx->frmBufInf.pDelayedPic[i]->nPoc < outPicPtr->nPoc)
                {
                    outPicPtr = hCtx->frmBufInf.pDelayedPic[i];
                    outIdx = i;
                }
            }
        }
//        hCtx->frmBufInf.pDelayedPic[i] = NULL;
        for(i=outIdx; hCtx->frmBufInf.pDelayedPic[i]; i++)
        {
            if((i+1) >= MAX_PICTURE_COUNT)
            {
                abort();
            }
            hCtx->frmBufInf.pDelayedPic[i] = hCtx->frmBufInf.pDelayedPic[i+1];
        }

        if(outPicPtr != NULL)
        {
            if(h264DecCtx->vconfig.bDispErrorFrame == 1)
            {
                bDispFrameFlag = 1;
            }
            else
            {
                bDispFrameFlag = (h264DecCtx->videoStreamInfo.bIs3DStream==0)?
                    (outPicPtr->bDecErrorFlag==0): 1;
            }
            if(outPicPtr->pScaleDownVPicture != NULL)
            {
                FbmReturnBuffer(hCtx->pFbmScaledown,
                    outPicPtr->pScaleDownVPicture,
                    bDispFrameFlag);
                outPicPtr->pScaleDownVPicture = NULL;
                if(outPicPtr->nDispBufIndex >= MAX_PICTURE_COUNT)
                {
                    abort();
                }
                hCtx->frmBufInf.picture[outPicPtr->nDispBufIndex].pScaleDownVPicture = NULL;
            }
            else if(outPicPtr->pVPicture != NULL)
            {
                FbmReturnBuffer(hCtx->pFbm, outPicPtr->pVPicture, bDispFrameFlag);
                if(outPicPtr->nDispBufIndex >= MAX_PICTURE_COUNT)
                {
                    abort();
                }
                hCtx->frmBufInf.picture[outPicPtr->nDispBufIndex].pVPicture = NULL;
                outPicPtr->pVPicture = NULL;
            }
        }
        else
        {
            break;
        }
    }
    for(i=0; i<MAX_PICTURE_COUNT; i++)
    {
        hCtx->frmBufInf.pDelayedPic[i] = NULL;
    }
    hCtx->nDelayedPicNum = 0;
    for(i=0+hCtx->nBufIndexOffset;
        i<hCtx->frmBufInf.nMaxValidFrmBufNum+hCtx->nBufIndexOffset; i++)
    {
        if(i >= MAX_PICTURE_COUNT)
        {
            abort();
        }
        if(hCtx->frmBufInf.picture[i].pVPicture!=NULL)
        {
            FbmReturnBuffer(hCtx->pFbm, hCtx->frmBufInf.picture[i].pVPicture, 0);
        }
        if(hCtx->frmBufInf.picture[i].pScaleDownVPicture != NULL)
        {
            FbmReturnBuffer(hCtx->pFbmScaledown,
                               hCtx->frmBufInf.picture[i].pScaleDownVPicture, 0);
        }
        hCtx->frmBufInf.picture[i].pVPicture = NULL;
        hCtx->frmBufInf.picture[i].pScaleDownVPicture = NULL;
        hCtx->frmBufInf.picture[i].nReference = 0;
        hCtx->frmBufInf.picture[i].nDecFrameOrder = 0;
    }
    return;
}

void H264FreeMemory(H264Dec* h264Dec, H264Context* hCtx)
{
    s32 i = 0;

    if(hCtx->pDeblkDramBuf != NULL)
    {
        CdcMemPfree(h264Dec->memops,hCtx->pDeblkDramBuf,
                                        (void *)h264Dec->pConfig->veOpsS,
                                        h264Dec->pConfig->pVeOpsSelf);
        hCtx->pDeblkDramBuf = NULL;
    }
    if(hCtx->pIntraPredDramBuf != NULL)
    {
        CdcMemPfree(h264Dec->memops,hCtx->pIntraPredDramBuf,
                                        (void *)h264Dec->pConfig->veOpsS,
                                        h264Dec->pConfig->pVeOpsSelf);
        hCtx->pIntraPredDramBuf = NULL;
    }
    if(hCtx->pExtraDataBuf != NULL)
    {
        CdcMemPfree(h264Dec->memops,hCtx->pExtraDataBuf,
                                        (void *)h264Dec->pConfig->veOpsS,
                                        h264Dec->pConfig->pVeOpsSelf);
        hCtx->pExtraDataBuf = NULL;
    }
    if(hCtx->pMbFieldIntraBuf !=NULL)
    {
        CdcMemPfree(h264Dec->memops,hCtx->pMbFieldIntraBuf,
                                        (void *)h264Dec->pConfig->veOpsS,
                                        h264Dec->pConfig->pVeOpsSelf);
        hCtx->pMbFieldIntraBuf = NULL;
    }
    if(hCtx->pMbNeighborInfoBuf != NULL)
    {
        CdcMemPfree(h264Dec->memops,hCtx->pMbNeighborInfoBuf,
                                        (void *)h264Dec->pConfig->veOpsS,
                                        h264Dec->pConfig->pVeOpsSelf);
        hCtx->pMbNeighborInfoBuf = NULL;
    }

    if(hCtx->frmBufInf.pMvColBuf != NULL)
    {
        CdcMemPfree(h264Dec->memops,hCtx->frmBufInf.pMvColBuf,
                                        (void *)h264Dec->pConfig->veOpsS,
                                        h264Dec->pConfig->pVeOpsSelf);
        hCtx->frmBufInf.pMvColBuf = NULL;
    }

    if((hCtx->pVbv->bUseNewVeMemoryProgram == 0)||!ENABLE_MV_BUF_OPTIMIZATION_PROGRAM)
    {
        for(i=0+hCtx->nBufIndexOffset;
            i<hCtx->frmBufInf.nMaxValidFrmBufNum+hCtx->nBufIndexOffset; i++)
        {
            if(i >= MAX_PICTURE_COUNT)
            {
                abort();
            }
            if(hCtx->frmBufInf.picture[i].pTopMvColBuf != NULL)
            {
                CdcMemPfree(h264Dec->memops,hCtx->frmBufInf.picture[i].pTopMvColBuf,
                                            (void *)h264Dec->pConfig->veOpsS,
                                            h264Dec->pConfig->pVeOpsSelf);
                hCtx->frmBufInf.picture[i].pTopMvColBuf = NULL;
            }
        }
    }
    else
    {
        if(hCtx->pH264MvBufInf != NULL)
         {
            for(i=0;i<hCtx->nH264MvBufNum; i++)
            {
                if(hCtx->pH264MvBufInf[i].pH264MvBufManager != NULL)
                {
                    if(hCtx->pH264MvBufInf[i].pH264MvBufManager->pTopMvColBuf != NULL)
                    {
                        CdcMemPfree(h264Dec->memops,
                            hCtx->pH264MvBufInf[i].pH264MvBufManager->pTopMvColBuf,
                                             (void *)h264Dec->pConfig->veOpsS,
                                             h264Dec->pConfig->pVeOpsSelf);
                        hCtx->pH264MvBufInf[i].pH264MvBufManager->pTopMvColBuf = NULL;
                    }
                    free(hCtx->pH264MvBufInf[i].pH264MvBufManager);
                    hCtx->pH264MvBufInf[i].pH264MvBufManager = NULL;
                 }
            }
            free(hCtx->pH264MvBufInf);
            hCtx->pH264MvBufInf = NULL;
         }
    }
    if(hCtx->pMafInitBuffer != NULL)
    {
        CdcMemPfree(h264Dec->memops,hCtx->pMafInitBuffer,
                                        (void *)h264Dec->pConfig->veOpsS,
                                        h264Dec->pConfig->pVeOpsSelf);
        hCtx->pMafInitBuffer = NULL;
    }

    if(hCtx->bUseMafFlag == 1)
    {
        for(i=0+hCtx->nBufIndexOffset;
            i<hCtx->frmBufInf.nMaxValidFrmBufNum+hCtx->nBufIndexOffset; i++)
        {
            if(hCtx->pFbm->pFrames[i].vpicture.pData3 != NULL)
            {
                CdcMemPfree(h264Dec->memops,hCtx->pFbm->pFrames[i].vpicture.pData3,
                                        (void *)h264Dec->pConfig->veOpsS,
                                        h264Dec->pConfig->pVeOpsSelf);
                hCtx->pFbm->pFrames[i].vpicture.pData3 = NULL;
            }
        }
    }

    if(hCtx->pFbm != NULL)
    {
        FbmDestroy(hCtx->pFbm);
        hCtx->pFbm = NULL;
    }
    if(hCtx->pFbmScaledown != NULL)
    {
        FbmDestroy(hCtx->pFbmScaledown);
        hCtx->pFbmScaledown = NULL;
    }

    if(hCtx->pMvcContext != NULL)
    {
        H264MvcContext* pMVCContext = NULL;
        pMVCContext = (H264MvcContext*)hCtx->pMvcContext;

        if(pMVCContext->pMvcParamBuf != NULL)
        {
            free(pMVCContext->pMvcParamBuf);
        }
        pMVCContext->pMvcParamBuf = NULL;
        free(pMVCContext);
        pMVCContext = NULL;
        hCtx->pMvcContext = NULL;
    }
    for(i=0; i<hCtx->nSpsBufferNum; i++)
    {
        free(hCtx->pSpsBuffers[hCtx->nSpsBufferIndex[i]]);
        hCtx->pSpsBuffers[hCtx->nSpsBufferIndex[i]] = NULL;
    }
    for(i=0; i<hCtx->nPpsBufferNum; i++)
    {
        free(hCtx->pPpsBuffers[hCtx->nPpsBufferIndex[i]]);
        hCtx->pPpsBuffers[hCtx->nPpsBufferIndex[i]] = NULL;
    }
    free(hCtx);
}

static void Destroy(DecoderInterface* pSelf)
{
    H264DecCtx* h264DecCtx = NULL;
    H264Dec* h264DecHandle = NULL;

    h264DecCtx = (H264DecCtx*)pSelf;

    if(h264DecCtx == NULL)
    {
        return;
    }

    h264DecHandle = (H264Dec*)h264DecCtx->pH264Dec;

    if(h264DecHandle != NULL)
    {
        if(h264DecHandle->pHContext != NULL && h264DecHandle->pHContext->vbvInfo.vbv != NULL)
        {
            if(h264DecHandle->pHContext->vbvInfo.vbv->nType != SBM_TYPE_FRAME_AVC)
            {
                if(h264DecHandle->pHContext->vbvInfo.pVbvStreamData != NULL)
                {
                    SbmFlushStream(h264DecHandle->pHContext->vbvInfo.vbv,
                        h264DecHandle->pHContext->vbvInfo.pVbvStreamData);
                    h264DecHandle->pHContext->vbvInfo.pVbvStreamData = NULL;
                    h264DecHandle->pHContext->vbvInfo.nVbvDataSize = 0;
                }
            }
            else
            {
                 if(h264DecHandle->pHContext->pCurStreamFrame != NULL)
                {
                    SbmFlushStream(h264DecHandle->pHContext->vbvInfo.vbv,
                     (VideoStreamDataInfo *)h264DecHandle->pHContext->pCurStreamFrame);
                    h264DecHandle->pHContext->pCurStreamFrame = NULL;
                }
            }
            H264FreeMemory(h264DecHandle, h264DecHandle->pHContext);
        }
        if(h264DecHandle->pMinorHContext != NULL &&
            h264DecHandle->pMinorHContext->vbvInfo.vbv != NULL)
        {
            if(h264DecHandle->pMinorHContext->vbvInfo.vbv->nType != SBM_TYPE_FRAME_AVC)
            {
                if(h264DecHandle->pMinorHContext->vbvInfo.pVbvStreamData != NULL)
                {
                    SbmFlushStream(h264DecHandle->pMinorHContext->vbvInfo.vbv,
                        h264DecHandle->pMinorHContext->vbvInfo.pVbvStreamData);
                    h264DecHandle->pMinorHContext->vbvInfo.pVbvStreamData = NULL;
                    h264DecHandle->pMinorHContext->vbvInfo.nVbvDataSize = 0;
                }
            }
            else
            {
                 if(h264DecHandle->pMinorHContext->pCurStreamFrame != NULL)
                {
                    SbmFlushStream(h264DecHandle->pMinorHContext->vbvInfo.vbv,
                        (VideoStreamDataInfo *)h264DecHandle->pMinorHContext->pCurStreamFrame);
                    h264DecHandle->pMinorHContext->pCurStreamFrame = NULL;
                }
            }
            H264FreeMemory(h264DecHandle, h264DecHandle->pMinorHContext);
        }
         // add for big_array;
        free(h264DecHandle->dataBuffer);
        h264DecHandle->dataBuffer = NULL;
        if(h264DecHandle->pDecHeaderBuf != NULL)
        {
            free(h264DecHandle->pDecHeaderBuf);
            h264DecHandle->pDecHeaderBuf = NULL;
        }
        free(h264DecHandle);
        h264DecHandle = NULL;
    }
    free(h264DecCtx);
    h264DecCtx = NULL;
}

/*************************************************************************/
//          Name:    ve_reset                                            //
//          Prototype: vresult_e ve_reset (u8 flush_pictures)            //
//          Function:  Reset the VE CSP.                                 //
//          Return:    VDECODE_RESULT_OK: success.                       //
//                   VRESULT_ERR_LIBRARY_NOT_OPEN: the CSP is not opened yet.//
/****************************************************************************/

void  H264DecoderRest(DecoderInterface* pSelf)
{
    H264DecCtx* h264DecCtx = NULL;
    H264Dec* h264DecHandle = NULL;

    h264DecCtx = (H264DecCtx*)pSelf;

    if(h264DecCtx == NULL)
    {
        return;
    }

    CdcVeReset(h264DecCtx->vconfig.veOpsS, h264DecCtx->vconfig.pVeOpsSelf);

    h264DecHandle = (H264Dec*)h264DecCtx->pH264Dec;
    if(h264DecHandle != NULL)
    {
        h264DecHandle->nDecStreamIndex = 0;
        if(h264DecHandle->pHContext != NULL && h264DecHandle->pHContext->vbvInfo.vbv != NULL)
        {
            if(h264DecHandle->pHContext->vbvInfo.vbv->nType != SBM_TYPE_FRAME_AVC)
            {
                if(h264DecHandle->pHContext->vbvInfo.pVbvStreamData != NULL)
                {
                    SbmFlushStream(h264DecHandle->pHContext->vbvInfo.vbv,
                        h264DecHandle->pHContext->vbvInfo.pVbvStreamData);
                    h264DecHandle->pHContext->vbvInfo.pVbvStreamData = NULL;
                    h264DecHandle->pHContext->vbvInfo.nVbvDataSize = 0;
                }
            }
            else
            {
                 if(h264DecHandle->pHContext->pCurStreamFrame != NULL)
                {
                    SbmFlushStream(h264DecHandle->pHContext->vbvInfo.vbv,
                      (VideoStreamDataInfo *)h264DecHandle->pHContext->pCurStreamFrame);
                    h264DecHandle->pHContext->pCurStreamFrame = NULL;
                }
            }
            H264FlushDelayedPictures(h264DecCtx, h264DecHandle->pHContext);
            H264ResetDecoderParams(h264DecHandle->pHContext);
        }
        if(h264DecHandle->pMinorHContext != NULL &&
            h264DecHandle->pMinorHContext->vbvInfo.vbv != NULL)
        {
            if(h264DecHandle->pMinorHContext->vbvInfo.vbv->nType != SBM_TYPE_FRAME_AVC)
            {
                if(h264DecHandle->pMinorHContext->vbvInfo.pVbvStreamData != NULL)
                {
                    SbmFlushStream(h264DecHandle->pMinorHContext->vbvInfo.vbv,
                        h264DecHandle->pMinorHContext->vbvInfo.pVbvStreamData);
                    h264DecHandle->pMinorHContext->vbvInfo.pVbvStreamData = NULL;
                    h264DecHandle->pMinorHContext->vbvInfo.nVbvDataSize = 0;
                }
            }
            else
            {
                 if(h264DecHandle->pMinorHContext->pCurStreamFrame != NULL)
                {
                    SbmFlushStream(h264DecHandle->pMinorHContext->vbvInfo.vbv,
                       (VideoStreamDataInfo *)h264DecHandle->pMinorHContext->pCurStreamFrame);
                    h264DecHandle->pMinorHContext->pCurStreamFrame = NULL;
                }
            }
            H264FlushDelayedPictures(h264DecCtx, h264DecHandle->pMinorHContext);
            H264ResetDecoderParams(h264DecHandle->pMinorHContext);
        }
        h264DecHandle->H264PerformInf.performCtrlFlag = 0;
        h264DecHandle->H264PerformInf.H264PerfInfo.nDropFrameNum = 0;
    }
}

/**********************************************************************************/
//   Name:    ve_set_vbv
//   Prototype: vresult_e ve_set_vbv (u8* vbv_buf, u32 vbv_size, Handle ve);
//   Function: Set VBV's bitstream buffer base address and buffer size to the CSP..
//   Return:   VDECODE_RESULT_OK: success.
//             VRESULT_ERR_LIBRARY_NOT_OPEN: the CSP is not opened yet.
/**********************************************************************************/

int  H264DecoderSetSbm(DecoderInterface* pSelf, SbmInterface* pSbm, int nIndex)
{
    H264Dec*h264DecHandle = NULL;
    H264DecCtx* h264DecCtx = NULL;
    h264DecCtx = (H264DecCtx*)pSelf;

    h264DecHandle = (H264Dec*)h264DecCtx->pH264Dec;

    if(nIndex == 0)
    {
        if(h264DecHandle->pHContext)
        {
            h264DecHandle->pHContext->pVbv            = pSbm;
            h264DecHandle->pHContext->pVbvBase         = (u8*)SbmBufferAddress(pSbm);
            h264DecHandle->pHContext->nVbvSize         = SbmBufferSize(pSbm);
            h264DecHandle->pHContext->vbvInfo.vbv   = pSbm;
        }
    }
    else
    {
        if(h264DecHandle->pMinorHContext)
        {
            h264DecHandle->pMinorHContext->pVbv            = pSbm;
            h264DecHandle->pMinorHContext->pVbvBase     = (u8*)SbmBufferAddress(pSbm);
            h264DecHandle->pMinorHContext->nVbvSize     = SbmBufferSize(pSbm);
            h264DecHandle->pMinorHContext->vbvInfo.vbv  = pSbm;
        }
    }
    return VDECODE_RESULT_OK;
}

/******************************************************************************/
//     Name:    ve_get_fbm
//     Prototype: Handle get_fbm (void);
//     Function: Get a handle of the FBM instance, in which pictures for display are stored.
//     Return:   Not NULL Handle: handle of the FBM instance.
//               NULL Handle: FBM module is not initialized yet.
/******************************************************************************/

Fbm* H264DecoderGetFbm(DecoderInterface* pSelf, int index)
{
    H264DecCtx* h264DecCtx = NULL;
    H264Dec*h264Dec = NULL;

    h264DecCtx = (H264DecCtx*)pSelf;

    if(h264DecCtx == NULL)
    {
        return NULL;
    }
    else
    {
        h264Dec =  (H264Dec*)h264DecCtx->pH264Dec;
        if(h264Dec->pHContext->frmBufInf.nMaxValidFrmBufNum == 0)
        {
            return NULL;
        }
        else
        {
            H264Context* hCtx = NULL;
            if(index == 0)
            {
                hCtx = h264Dec->pHContext;
            }
            else
            {
                hCtx = h264Dec->pMinorHContext;
            }
            if(hCtx == NULL)
            {
                return NULL;
            }

            if(hCtx->pFbmScaledown == 0)
            {
                return  hCtx->pFbm;
            }
            return hCtx->pFbmScaledown;
        }
    }
    return NULL;
}

int H264DecoderGetFbmNum(DecoderInterface* pSelf)
{
    H264DecCtx* h264DecCtx = NULL;
    H264Context* hCtx =  NULL;
    H264Context* minorHCtx =  NULL;

    h264DecCtx = (H264DecCtx*)pSelf;

    hCtx = ((H264Dec*)h264DecCtx->pH264Dec)->pHContext;
    minorHCtx = ((H264Dec*)h264DecCtx->pH264Dec)->pMinorHContext;

    if(h264DecCtx->videoStreamInfo.bIs3DStream==1)
    {
        return 2;
    }
    return 1;
}

int H264DecoderSetExtraScaleInfo(DecoderInterface* pSelf,s32 nWidthTh, s32 nHeightTh,
                                 s32 nHorizonScaleRatio,s32 nVerticalScaleRatio)
{

     H264DecCtx* h264DecCtx = NULL;
     H264Dec* h264Dec = NULL;

     h264DecCtx = (H264DecCtx*)pSelf;
     h264Dec = (H264Dec*)h264DecCtx->pH264Dec;

     h264Dec->H264ExtraScaleInf.nWidthTh             = nWidthTh;
     h264Dec->H264ExtraScaleInf.nHeightTh             = nHeightTh;
     h264Dec->H264ExtraScaleInf.nHorizonScaleRatio  = nHorizonScaleRatio;
     h264Dec->H264ExtraScaleInf.nVerticalScaleRatio = nVerticalScaleRatio;

     logd("nWidthTh=%d, nHeightTh=%d,nHorizonScaleRatio=%d,nVerticalScaleRatio=%d\n",
             nWidthTh, nHeightTh,nHorizonScaleRatio,nVerticalScaleRatio);
     return 0;
}

int H264DecoderSetPerformCmd(DecoderInterface* pSelf,enum EVDECODERSETPERFORMCMD performCmd)
{
    H264DecCtx* h264DecCtx = NULL;
    H264Dec* h264Dec = NULL;
    int ret = 0;

    h264DecCtx = (H264DecCtx*)pSelf;
    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;

    switch(performCmd)
    {
        case VDECODE_SETCMD_DEFAULT:
        {
            ret = -1;
            break;
        }
        case VDECODE_SETCMD_START_CALDROPFRAME:
        {
            h264Dec->H264PerformInf.performCtrlFlag |= H264_PERFORM_CALDROPFRAME;
            h264Dec->H264PerformInf.H264PerfInfo.nDropFrameNum = 0;
            ret = 0;
            break;
        }
        case VDECODE_SETCMD_STOP_CALDROPFRAME:
        {
            h264Dec->H264PerformInf.performCtrlFlag &= ~H264_PERFORM_CALDROPFRAME;
            h264Dec->H264PerformInf.H264PerfInfo.nDropFrameNum = 0;
            ret = 0;
            break;
        }
        default:
        {
            ret = -1;
            break;
        }
    }
    return ret;
}

int H264DecodeGetPerformInfo(DecoderInterface* pSelf,enum EVDECODERGETPERFORMCMD performCmd,
                             VDecodePerformaceInfo** performInfo)
{
    H264DecCtx* h264DecCtx = NULL;
    H264Dec* h264Dec = NULL;
    int ret = 0;
    h264DecCtx = (H264DecCtx*)pSelf;
    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;

    switch(performCmd)
    {
        case VDECODE_GETCMD_DEFAULT:
        {
            ret = -1;
            break;
        }
        case VDECODE_GETCMD_DROPFRAME_INFO:
        {
            ret = 0;
            *performInfo  = &(h264Dec->H264PerformInf.H264PerfInfo);
            break;
        }
        default:
        {
            ret = -1;
            break;
        }
    }
    return ret;
}

/****************************************************************************
   Name:    ve_decode
   Prototype: vresult_e decode (VideoStreamDataInfo* stream, u8 keyframe_only,
                               u8 skip_bframe, u32 cur_time);
   Function: Decode one bitstream frame.
   INput:    VideoStreamDataInfo* stream: start address of the stream frame.
             u8 keyframe_only:   tell the CSP to decode key frame only;
             u8 skip_bframe:  tell the CSP to skip B frame if it is overtime;
             u32 cur_time:current time, used to compare with PTS when decoding B frame;
   Return:   VDECODE_RESULT_OK:             decode stream success but no frame decoded;
             VDECODE_RESULT_FRAME_DECODED:  one common frame decoded;
             VDECODE_RESULT_KEYFRAME_DECODED:    one key frame decoded;
             VRESULT_ERR_FAIL:           decode stream fail;
             VRESULT_ERR_INVALID_PARAM:  either stream or ve is NULL;
             VRESULT_ERR_INVALID_STREAM: some error data in the stream, decode fail;
             VRESULT_ERR_NO_MEMORY:      allocate memory fail in this method;
             VRESULT_ERR_NO_FRAMEBUFFER: request empty frame buffer fail in this method;
             VRESULT_ERR_UNSUPPORTED:    stream format is unsupported by this version of VE CSP;
             VRESULT_ERR_LIBRARY_NOT_OPEN:  'open' has not been successfully called yet.
*******************************************************************************/

s32 H264ProcessErrorFrame(H264DecCtx* h264DecCtx, H264Context* hCtx)
{
    if(hCtx->frmBufInf.pCurPicturePtr->bDecErrorFlag == 1)
    {
        if(h264DecCtx->videoStreamInfo.bIs3DStream == 1)
        {
            return VDECODE_RESULT_OK;
        }
        else
        {
            if(hCtx->nNalRefIdc != 0)  // the frame is a reference frame
            {
                hCtx->bPreDropFrame = 1;
                if(hCtx->vbvInfo.vbv->nType != SBM_TYPE_FRAME_AVC)
                {
                    if(hCtx->vbvInfo.pVbvStreamData != NULL)
                    {
                        SbmFlushStream(hCtx->vbvInfo.vbv, hCtx->vbvInfo.pVbvStreamData);
                        hCtx->vbvInfo.pVbvStreamData = NULL;
                        hCtx->vbvInfo.nVbvDataSize = 0;
                    }
                }
                else
                {
                    if(hCtx->pCurStreamFrame != NULL)
                    {
                        SbmFlushStream(hCtx->vbvInfo.vbv,
                           (VideoStreamDataInfo *)hCtx->pCurStreamFrame);
                        hCtx->pCurStreamFrame = NULL;
                    }
                }
                H264FlushDelayedPictures(h264DecCtx, hCtx);
                H264ResetDecoderParams(hCtx);
                return VDECODE_RESULT_CONTINUE;
            }
        }
        hCtx->bDropFrame = 1;
    }
    return VDECODE_RESULT_OK;
}

int h264UpdateProcInfo(H264DecCtx* h264DecCtx, H264Context* hCtx)
{
    char* tmpBuf = NULL;
    int   nTmpBufLen = 1024;
    VeProcDecInfo mVeProcDecInfo;
    int   nProcInfoLen = 0;

    memset(&mVeProcDecInfo, 0, sizeof(VeProcDecInfo));

    tmpBuf = (char*)malloc(nTmpBufLen);
    if(tmpBuf == NULL)
    {
        loge("malloc for tmpBuf failed, size = %d",nTmpBufLen);
        return -1;
    }
    memset(tmpBuf, 0, nTmpBufLen);

    mVeProcDecInfo.nChannelNum = h264DecCtx->vconfig.nChannelNum;

    sprintf(mVeProcDecInfo.nProfile, "%s", "main");

    mVeProcDecInfo.nWidth  = hCtx->nFrmMbWidth;
    mVeProcDecInfo.nHeight = hCtx->nFrmMbHeight;

    nProcInfoLen += sprintf(tmpBuf + nProcInfoLen,
            "***************channel[%d]: h264 dec info**********************\n",
            mVeProcDecInfo.nChannelNum);

    nProcInfoLen += sprintf(tmpBuf + nProcInfoLen,
            "profile: %s, widht: %d, height: %d\n",
            mVeProcDecInfo.nProfile, mVeProcDecInfo.nWidth, mVeProcDecInfo.nHeight);

    logv("proc info: len = %d, %s",nProcInfoLen, tmpBuf);
    CdcVeSetProcInfo(h264DecCtx->vconfig.veOpsS,
                     h264DecCtx->vconfig.pVeOpsSelf,
                     tmpBuf, nProcInfoLen, mVeProcDecInfo.nChannelNum);


    free(tmpBuf);
    return 0;
}

s32 H264HoldFrameBuffer(H264DecCtx* h264DecCtx, H264Context* hCtx)
{
    s32 i = 0;
    s32 nUsedBufferNum = 0;
    s32 maxFrameNum = 0;

    for(i=0+hCtx->nBufIndexOffset;
        i<hCtx->frmBufInf.nMaxValidFrmBufNum+hCtx->nBufIndexOffset; i++)
    {
        if(hCtx->frmBufInf.picture[i].pVPicture != NULL)
        {
            nUsedBufferNum++;
        }
    }

    maxFrameNum = (s32)(hCtx->nRefFrmCount+1+
        h264DecCtx->vconfig.nDecodeSmoothFrameBufferNum);

    if(nUsedBufferNum>=maxFrameNum)
    {
        logd("error: H264HoldFrameBuffer, nUsedBufferNum=%d, maxFrameNum=%d\n",\
            nUsedBufferNum,maxFrameNum);
        return 1;
    }
    return 0;
}


 int  H264DecoderDecode(DecoderInterface* pSelf,
                               s32           bEndOfStream,
                               s32           bDecodeKeyFrameOnly,
                               s32           bSkipBFrameIfDelay,
                               int64_t       nCurrentTimeUs)
{
    H264DecCtx* h264DecCtx = NULL;
    H264Dec* h264Dec = NULL;
    H264Context* hCtx = NULL;
    s32 ret = 0;

    h264DecCtx = (H264DecCtx*)pSelf;
    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;
    hCtx = h264Dec->pHContext;

    if(hCtx->pVbv->nType == SBM_TYPE_FRAME_AVC)
    {
        ret = H264DecoderDecodeSbmFrame(pSelf,bEndOfStream,bDecodeKeyFrameOnly,
            bSkipBFrameIfDelay,nCurrentTimeUs);
    }
    else
    {
       ret = VDECODE_RESULT_OK;;
    }
    return ret;

}


s32 H264DecodeProcessData(H264DecCtx* h264DecCtx, H264Context* hCtx,
                                    u8 nalUnitType, u32 sliceLen)
{
    H264Dec* h264Dec = NULL;
    u32 remainLength  = 0;
    u32 nextCode = 0;
    u32 i = 0;
    u32 index = 0;
    u32 length = 0;

    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;

    if(h264DecCtx->bSoftDecHeader == 0)
    {
        if(nalUnitType == 6)
        {
            H264CheckBsDmaBusy(h264DecCtx);
            CdcVeReset(h264DecCtx->vconfig.veOpsS, h264DecCtx->vconfig.pVeOpsSelf);
        }
        if(hCtx->bIsAvc)
        {
            H264ConfigureAvcRegister(h264DecCtx, hCtx, 1, sliceLen*8);
        }
        else
            H264ConfigureEptbDetect(h264DecCtx, hCtx, sliceLen*8, 1);
    }
    else if(h264DecCtx->bSoftDecHeader == 1)
    {
        if((nalUnitType==NAL_SLICE)|| (nalUnitType==NAL_IDR_SLICE)
            ||(nalUnitType==NAL_HEADER_EXT1)||(nalUnitType==NAL_HEADER_EXT2))
        {
            if(sliceLen >= H264_HEADER_BUF_SIZE)
            {
                sliceLen = H264_HEADER_BUF_SIZE;
            }
        }

         if(sliceLen > h264Dec->nDecHeaderBufSize)
         {
            free(h264Dec->pDecHeaderBuf);
            h264Dec->pDecHeaderBuf = (u8*)malloc(sliceLen+1024);
            if(h264Dec->pDecHeaderBuf == NULL)
            {
                loge("malloc buffer for h264Dec->pDecHeaderBuf is failed\n");
                abort();
            }
            h264Dec->nDecHeaderBufSize = sliceLen+1024;
         }

         if(hCtx->vbvInfo.pReadPtr+sliceLen > hCtx->vbvInfo.pVbvBufEnd)
         {
            remainLength = hCtx->vbvInfo.pVbvBufEnd-hCtx->vbvInfo.pReadPtr+1;
         }
         else
         {
            remainLength = sliceLen;
         }
         memcpy(h264Dec->pDecHeaderBuf,hCtx->vbvInfo.pReadPtr,remainLength);
         memcpy(h264Dec->pDecHeaderBuf+remainLength,hCtx->vbvInfo.pVbvBuf,sliceLen-remainLength);

        //check the 0x03
        h264Dec->nEmulateNum = 0;
        nextCode =  0xFFFFFFFF;
        for(i=0; i<sliceLen; i++)
        {
            nextCode <<= 8;
            nextCode |= h264Dec->pDecHeaderBuf[i];
            if((nextCode&0x00FFFFFF) == 0x000003)
            {
                h264Dec->emulateIndex[h264Dec->nEmulateNum] = i;
                h264Dec->nEmulateNum++;
                logd("h264Dec->nEmulateNum=%d, i=%d\n", h264Dec->nEmulateNum, i);
            }
        }


        if(h264Dec->nEmulateNum > 0)
        {
           logv("origin data\n");
           for(i=0; i<8; i++)
           {
                logv("data=%x %x %x %x %x %x %x %x\n",h264Dec->pDecHeaderBuf[8*i+0],
                h264Dec->pDecHeaderBuf[8*i+1],h264Dec->pDecHeaderBuf[8*i+2],
                h264Dec->pDecHeaderBuf[8*i+3],h264Dec->pDecHeaderBuf[8*i+4],
                h264Dec->pDecHeaderBuf[8*i+5],h264Dec->pDecHeaderBuf[8*i+6],
                h264Dec->pDecHeaderBuf[8*i+7]);
            }

            h264Dec->emulateIndex[h264Dec->nEmulateNum] = sliceLen;
            index = h264Dec->emulateIndex[0];
            for(i=0; i<h264Dec->nEmulateNum; i++)
            {
                length = h264Dec->emulateIndex[i+1]-h264Dec->emulateIndex[i]-1;
                memcpy(h264Dec->pDecHeaderBuf+index,
                    h264Dec->pDecHeaderBuf+h264Dec->emulateIndex[i]+1,length);

                logv("length=%d, index=%d\n", length, index);
                index += length;

            }

            logv("after data\n");
            for(i=0; i<8; i++)
            {
                logv("data=%x %x %x %x %x %x %x %x\n",h264Dec->pDecHeaderBuf[8*i+0],
                    h264Dec->pDecHeaderBuf[8*i+1],h264Dec->pDecHeaderBuf[8*i+2],
                    h264Dec->pDecHeaderBuf[8*i+3],h264Dec->pDecHeaderBuf[8*i+4],
                    h264Dec->pDecHeaderBuf[8*i+5],h264Dec->pDecHeaderBuf[8*i+6],
                    h264Dec->pDecHeaderBuf[8*i+7]);
            }
        }

        h264DecCtx->InitGetBits((void*)h264DecCtx, h264Dec->pDecHeaderBuf, sliceLen*8);
     }
     return 0;
}

int  H264DecoderDecodeSbmFrame(DecoderInterface* pSelf,
                            s32           bEndOfStream,
                            s32           bDecodeKeyFrameOnly,
                            s32           bSkipBFrameIfDelay,
                            int64_t       nCurrentTimeUs)
{

    H264DecCtx* h264DecCtx = NULL;
    H264Dec* h264Dec = NULL;
    H264Context* hCtx = NULL;
    s32  nal_unit_type  = 0;
    s32 ret = 0;
    u8  bForceRetunFrame = 0;
    u8 bSliceFlag=0;
    u8 bExtSliceFlag=0;
    u32 sliceLen = 0;

    h264DecCtx = (H264DecCtx*)pSelf;
    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;

    if(h264Dec->nDecStreamIndex == 0)
    {
        hCtx =     h264Dec->pHContext;
        hCtx->nBufIndexOffset = 0;
    }
    else
    {
        hCtx =     h264Dec->pMinorHContext;
        hCtx->nBufIndexOffset = 9;
    }
    if(hCtx == NULL)
    {
        return VDECODE_RESULT_OK;
    }
    hCtx->bEndOfStream = bEndOfStream;
    hCtx->nSystemTime = nCurrentTimeUs;
    hCtx->bSkipBFrameIfDelay = bSkipBFrameIfDelay;
    hCtx->bDecodeKeyFrameOnly = bDecodeKeyFrameOnly;

     //**************step 1*****************************************//
     if(hCtx->nDecStep == H264_STEP_CONFIG_VBV)
     {
         if(hCtx->bDecExtraDataFlag==1)
         {
            ret = H264ProcessExtraData2(h264DecCtx, hCtx);
            #if 0
            if(ret != VDECODE_RESULT_OK)
            #else
            if(ret == VDECODE_RESULT_UNSUPPORTED)
            #endif
            {
                return ret;
            }
            hCtx->bDecExtraDataFlag = 0;
         }
         CdcVeReset(h264DecCtx->vconfig.veOpsS, h264DecCtx->vconfig.pVeOpsSelf);
         H264InitFuncCtrlRegister(h264DecCtx);

         hCtx->bNeedCheckFbmNum = 1;

         if(hCtx->pVbv->bUseNewVeMemoryProgram == 0)
         {
            H264SetVbvParams(h264Dec, hCtx, hCtx->pVbvBase,
                         hCtx->pVbvBase+hCtx->nVbvSize-1, 0,
                         FIRST_SLICE_DATA|LAST_SLICE_DATA|SLICE_DATA_VALID);
         }
         hCtx->nDecStep = H264_STEP_UPDATE_DATA;
     }
     if(hCtx->nDecStep == H264_STEP_UPDATE_DATA)
     {
         if(hCtx->bNeedCheckFbmNum == 1)
         {
             hCtx->bNeedFindIFrm |= bDecodeKeyFrameOnly;
             hCtx->bDecodeKeyFrameOnly = bDecodeKeyFrameOnly;
             if(hCtx->pFbm && FbmEmptyBufferNum(hCtx->pFbm) == 0)
             {
                 return VDECODE_RESULT_NO_FRAME_BUFFER;
             }
             hCtx->nRequestBufFailedNum = 0;
             if(hCtx->pFbmScaledown && FbmEmptyBufferNum(hCtx->pFbmScaledown) == 0)
             {
                 return VDECODE_RESULT_NO_FRAME_BUFFER;
             }
             hCtx->bNeedCheckFbmNum = 0;
          }
        #if 1
         if(hCtx->pCurStreamFrame == NULL)
         {
             ret = H264RequestOneFrameStream(h264DecCtx, hCtx);
             if(ret != 0)
                 return ret;
         }
       #endif
         CdcVeReset(h264DecCtx->vconfig.veOpsS, h264DecCtx->vconfig.pVeOpsSelf);
         H264InitFuncCtrlRegister(h264DecCtx);

         hCtx->nDecStep = H264_STEP_DECODE_ONE_FRAME;
     }

    while(hCtx->nDecStep == H264_STEP_DECODE_ONE_FRAME)
    {
         if(hCtx->pCurStreamFrame == NULL)
         {
             ret = H264RequestOneFrameStream(h264DecCtx, hCtx);
             if(ret != 0)
                 return ret;
         }
         FramePicInfo* pCurStreamFrame = hCtx->pCurStreamFrame;

         u32 nCurNaluIdx = hCtx->vbvInfo.nCurNaluIdx;
         sliceLen = pCurStreamFrame->pNaluInfoList[nCurNaluIdx].nDataSize;


         hCtx->vbvInfo.pReadPtr = (u8*)pCurStreamFrame->pNaluInfoList[nCurNaluIdx].pDataBuf;

         nal_unit_type = pCurStreamFrame->pNaluInfoList[nCurNaluIdx].nType;
         hCtx->nNalRefIdc = pCurStreamFrame->pNaluInfoList[nCurNaluIdx].nNalRefIdc;

         H264DecodeProcessData(h264DecCtx, hCtx, nal_unit_type, sliceLen);

         h264DecCtx->GetBits((void*)h264DecCtx, 8);
         logv("2222 nal_unit_type=%d,  slice size: %d, nalRefIdc = %d",
            nal_unit_type, sliceLen, hCtx->nNalRefIdc);
         ret = H264ProcessNaluUnit(h264DecCtx, hCtx, nal_unit_type, sliceLen*8,
                                     h264Dec->nDecStreamIndex);

         hCtx->nByteLensCurFrame += sliceLen;

         if((nal_unit_type==6) && (h264DecCtx->bSoftDecHeader==0))
         {
             /* after decoding SEI, next nalu may have VE timeout;
                      * so we need to reset ve; */
             CdcVeReset(h264DecCtx->vconfig.veOpsS, h264DecCtx->vconfig.pVeOpsSelf);
         }
         if((ret==VDECODE_RESULT_KEYFRAME_DECODED) || (ret==VDECODE_RESULT_FRAME_DECODED))
         {
             if(hCtx->vbvInfo.nCurNaluIdx != hCtx->pCurStreamFrame->nCurNaluIdx - 1)
             {
                 loge("*** something is erro, nCurNaluIdx = %d, totalNalu = %d",
                 hCtx->vbvInfo.nCurNaluIdx, hCtx->pCurStreamFrame->nCurNaluIdx - 1);
                 abort();
             }
             else if(hCtx->pCurStreamFrame != NULL)
             {
                 SbmFlushStream(hCtx->vbvInfo.vbv,(VideoStreamDataInfo *)hCtx->pCurStreamFrame);
                 hCtx->pCurStreamFrame = NULL;
             }
             hCtx->nDecStep = H264_STEP_PROCESS_DECODE_RESULT;
             break;
         }
         if(hCtx->bIsAvc == 0)
         {
             if(ret== VRESULT_ERR_FAIL)
             {
                 bSliceFlag = ((nal_unit_type==NAL_SLICE)||(nal_unit_type==NAL_IDR_SLICE));
                 bExtSliceFlag = ((h264Dec->nDecStreamIndex==1) &&
                         ((nal_unit_type==NAL_HEADER_EXT1)||(nal_unit_type==NAL_HEADER_EXT2)));
                 if((bSliceFlag==1) || (bExtSliceFlag==1))
                 {
                     hCtx->bFrameEndFlag = 0;
                 }
                 //continue;
             }
             else if(ret!= VDECODE_RESULT_OK && ret!= VRESULT_DROP_B_FRAME)
             {
                 if(ret == VRESULT_DEC_FRAME_ERROR)
                 {
                     hCtx->nDecStep = H264_STEP_PROCESS_DECODE_RESULT;
                     ret = VDECODE_RESULT_FRAME_DECODED;
                     hCtx->nDecFrameStatus = H264_END_DEC_FRAME;
                     if(hCtx->pCurStreamFrame != NULL)
                     {
                         SbmReturnStream(hCtx->vbvInfo.vbv,\
                             (VideoStreamDataInfo *)hCtx->pCurStreamFrame);
                         hCtx->pCurStreamFrame = NULL;
                     }
                     break;
                  }
                 else if(ret == VDECODE_RESULT_NO_FRAME_BUFFER)
                 {
                     hCtx->nDecStep = H264_STEP_UPDATE_DATA;
                 }
                 else if(ret == VRESULT_REDECODE_STREAM_DATA)
                 {
                     hCtx->nDecStep = H264_STEP_UPDATE_DATA;
                     ret = VDECODE_RESULT_OK;
                 }
                 return ret;
             }
         }
         else
         {
             if((ret!= VDECODE_RESULT_OK)&&(ret!= VRESULT_ERR_FAIL)&&(ret!=VRESULT_DROP_B_FRAME))
             {
                 if(ret == VRESULT_DEC_FRAME_ERROR)
                  {
                     hCtx->nDecStep = H264_STEP_PROCESS_DECODE_RESULT;
                     ret = VDECODE_RESULT_FRAME_DECODED;
                     hCtx->nDecFrameStatus = H264_END_DEC_FRAME;
                     if(hCtx->pCurStreamFrame != NULL)
                     {
                         SbmReturnStream(hCtx->vbvInfo.vbv,\
                            (VideoStreamDataInfo *)hCtx->pCurStreamFrame);
                         hCtx->pCurStreamFrame = NULL;
                     }
                     break;
                  }
                 else if(ret == VDECODE_RESULT_NO_FRAME_BUFFER)
                 {
                     hCtx->nDecStep = H264_STEP_UPDATE_DATA;
                 }
                 else if(ret == VRESULT_REDECODE_STREAM_DATA)
                 {
                     hCtx->nDecStep = H264_STEP_UPDATE_DATA;
                     ret = VDECODE_RESULT_OK;
                 }
                 return ret;
             }

         }

         if(ret == VRESULT_DROP_B_FRAME)
         {
             if(hCtx->frmBufInf.pCurPicturePtr != NULL)
             {
                 hCtx->bPreDropFrame = 1;
                 H264SortDisplayFrameOrder(h264DecCtx, hCtx, bDecodeKeyFrameOnly);
             }
             if(hCtx->pCurStreamFrame != NULL)
             {
                 SbmFlushStream(hCtx->vbvInfo.vbv,(VideoStreamDataInfo *)hCtx->pCurStreamFrame);
                 hCtx->pCurStreamFrame = NULL;
             }
             hCtx->nDecStep = H264_STEP_UPDATE_DATA;
             return VDECODE_RESULT_OK;
         }

         hCtx->vbvInfo.nCurNaluIdx++;

         if(hCtx->vbvInfo.nCurNaluIdx == hCtx->pCurStreamFrame->nCurNaluIdx)
         {
              if(hCtx->pCurStreamFrame != NULL)
             {
                 SbmFlushStream(hCtx->vbvInfo.vbv,(VideoStreamDataInfo *)hCtx->pCurStreamFrame);
                 hCtx->pCurStreamFrame = NULL;
             }
             logv("the curFrameSteam can not decode one frame");
             //hCtx->nDecStep = H264_STEP_REQUEST_FRAME_STREAM;
         }
    }

     //**************************************************************//
    //**************step 4*****************************************//
    if(hCtx->nDecStep == H264_STEP_PROCESS_DECODE_RESULT)
    {
        if((ret==VDECODE_RESULT_KEYFRAME_DECODED) || (ret==VDECODE_RESULT_FRAME_DECODED))
        {
            hCtx->nDecFrameCount++;
            if(h264DecCtx->vconfig.bSetProcInfoEnable == 1
               && hCtx->nDecFrameCount%h264DecCtx->vconfig.nSetProcInfoFreq == 0)
            {
                h264UpdateProcInfo(h264DecCtx, hCtx);
            }
            if(hCtx->frmBufInf.pCurPicturePtr->bDecErrorFlag == 1)
            {
                if((h264DecCtx->vconfig.bThumbnailMode==1) &&
                    (h264DecCtx->vconfig.bDispErrorFrame==0))
                {
                    ret = VDECODE_RESULT_OK;
                }
            }
            hCtx->nPrevFrmNumOffset = hCtx->nFrmNumOffset;
            hCtx->nPrevFrmNum = hCtx->nFrmNum;
            hCtx->nPrevPocMsb = hCtx->nPocMsb;
            hCtx->nPrevPocLsb = hCtx->nPocLsb;
            hCtx->frmBufInf.pCurPicturePtr->nPictType = hCtx->nSliceType;

            if(hCtx->nNalRefIdc != 0)
            {
                hCtx->nPrevPocMsb = hCtx->nPocMsb;
                hCtx->nPrevPocLsb = hCtx->nPocLsb;
                H264ExecuteRefPicMarking(hCtx, hCtx->mmco, hCtx->nMmcoIndex);
            }

            if(hCtx->bFstField == 1)
            {
                // waiting fot the second field
                ret = VDECODE_RESULT_OK;
                hCtx->bNeedCheckFbmNum = 0;
            }
            else
            {
                VideoPicture* curFrmInfo = NULL;

                if((h264DecCtx->vconfig.bRotationEn== 1) ||
                    (h264DecCtx->vconfig.bScaleDownEn==1)||
                    (hCtx->bYv12OutFlag== 1) ||
                    (hCtx->bNv21OutFlag==1))
                {
                    curFrmInfo = hCtx->frmBufInf.pCurPicturePtr->pScaleDownVPicture;
                }
                else
                {
                    curFrmInfo = hCtx->frmBufInf.pCurPicturePtr->pVPicture;
                }
#if 1  // added for liweihai by xyliu

                if(h264DecCtx->vconfig.bDispErrorFrame==0)
                {
                    if(H264ProcessErrorFrame(h264DecCtx, hCtx) != VDECODE_RESULT_OK)
                    {
                        return VDECODE_RESULT_CONTINUE;
                    }
                }

#endif

                if((hCtx->frmBufInf.pCurPicturePtr!= NULL)&&(curFrmInfo!= NULL))
                {
                    hCtx->frmBufInf.pCurPicturePtr->nDecFrameOrder = hCtx->nCurFrmNum;

                    hCtx->nCurFrmNum++;
                    H264CongigureDisplayParameters(h264DecCtx, hCtx);
                    if(hCtx->bDropFrame == 1)
                    {
                        hCtx->bPreDropFrame = 1;
                    }
                    if(h264DecCtx->videoStreamInfo.bIs3DStream == 0)
                    {
                        H264SortDisplayFrameOrder(h264DecCtx, hCtx, bDecodeKeyFrameOnly);
                    }
                    else
                    {
                        if(h264Dec->nDecStreamIndex == 1)
                        {
                            H264Context* majorHCtx = h264Dec->pHContext;

                            H264SortDisplayFrameOrder(h264DecCtx, majorHCtx, bDecodeKeyFrameOnly);
                            //logd("hCtx->nCurFrmNum=%d,
                            //majorHCtx->frmBufInf.pCurPicturePt->nPoc=%d\n",
                            //hCtx->nCurFrmNum, majorHCtx->frmBufInf.pCurPicturePtr->nPoc);

                            H264SortDisplayFrameOrder(h264DecCtx, hCtx, bDecodeKeyFrameOnly);
                        }
                        else
                        {
                            h264Dec->pMajorRefFrame = hCtx->frmBufInf.pCurPicturePtr;
                        }
                        h264Dec->nDecStreamIndex = 1 - h264Dec->nDecStreamIndex;
                    }
                }
                hCtx->bNeedCheckFbmNum = 1;
                //decode the second frame
            }
            hCtx->nDecStep = H264_STEP_UPDATE_DATA;
        }
    }
    return ret;
}

