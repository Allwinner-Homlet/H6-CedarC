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
* File : h265_ref.c
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#include "h265_func.h"
#include "h265_memory.h"

static void controlEnableAfbc(HevcContex *pHevcDec)
{
    //* make policy of enable afbc rely on widht and height
    //* just chip/0x31010 surpport afbc
    if(pHevcDec->nDecIpVersion == 0x31010)
    {
        if(pHevcDec->pConfig->eCtlAfbcMode == DISABLE_AFBC_ALL_SIZE)
        {
            pHevcDec->bEnableAfbcFlag = 0;
        }
        else if(pHevcDec->pConfig->eCtlAfbcMode == ENABLE_AFBC_ALL_SIZE)
        {
            pHevcDec->bEnableAfbcFlag = 1;
        }
        else if(pHevcDec->pConfig->eCtlAfbcMode == ENABLE_AFBC_JUST_BIG_SIZE)
        {
            if(pHevcDec->pSps->nWidth >= AFBC_ENABLE_SIZE_WIDTH
               || pHevcDec->pSps->nHeight >= AFBC_ENABLE_SIZE_HEIGHT)
            {
                pHevcDec->bEnableAfbcFlag = 1;
            }
            else
            {
                pHevcDec->bEnableAfbcFlag = 0;
            }
        }
        else
        {
            pHevcDec->bEnableAfbcFlag = 0;
        }
    }
    else
    {
        pHevcDec->bEnableAfbcFlag = 0;
    }
}


static void controlEnableIptv(HevcContex *pHevcDec)
{
    if(pHevcDec->pConfig->eCtlIptvMode == DISABLE_IPTV_ALL_SIZE)
    {
        pHevcDec->bEnableIptvFlag = 0;
    }
    else if(pHevcDec->pConfig->eCtlIptvMode == ENABLE_IPTV_ALL_SIZE)
    {
        pHevcDec->bEnableIptvFlag = 1;
    }
    else if(pHevcDec->pConfig->eCtlIptvMode == ENABLE_IPTV_JUST_SMALL_SIZE)
    {
        if(pHevcDec->pSps->nWidth >= AFBC_ENABLE_SIZE_WIDTH
           || pHevcDec->pSps->nHeight >= AFBC_ENABLE_SIZE_HEIGHT)
        {
            pHevcDec->bEnableIptvFlag = 0;
        }
        else
        {
            pHevcDec->bEnableIptvFlag = 1;
        }
    }
    else
    {
        pHevcDec->bEnableIptvFlag = 0;
    }


}

static void HevcMarkRef(HevcFrame *pHf, s32 bFlag)
{
    pHf->bFlags &= ~(HEVC_FRAME_FLAG_LONG_REF | HEVC_FRAME_FLAG_SHORT_REF);
    pHf->bFlags |= bFlag;
}

static void HevcUnrefFrame(HevcContex *pHevcDec, HevcFrame *pHf, s32 bFlags)
{
    if(pHf->pFbmBuffer == NULL)
        return;
    pHf->bFlags &= ~bFlags;
    if(!pHf->bFlags)
    {
        /* return fbm buffer */
        if(pHevcDec->fbm != NULL && pHf->pFbmBuffer != NULL)
        {
//            logd(" return one first fbm buffer");
            FbmReturnBuffer(pHevcDec->fbm, pHf->pFbmBuffer, 0);
            if((pHevcDec->pStreamInfo->pSbm->bUseNewVeMemoryProgram == 1)&&       \
                ENABLE_MV_BUF_OPTIMIZATION_PROGRAM)
            {
                FIFOEnqueue((FiFoQueueInst**)&pHevcDec->pHevcMvBufEmptyQueue,
                    (FiFoQueueInst *)&(pHevcDec->pHevcMvBufInf[pHf->nMvFrameBufIndex]));
            }
        }
        pHf->pFbmBuffer = NULL;
        if(pHevcDec->ControlInfo.secOutputEnabled)
        {
            if(pHevcDec->SecondFbm != NULL && pHf->pSecFbmBuffer != NULL)
            {
                FbmReturnBuffer(pHevcDec->SecondFbm, pHf->pSecFbmBuffer, 0);
//                logd(" return one second fbm buffer -----");
            }
            pHf->pSecFbmBuffer = NULL;
        }
    }
}

static HevcFrame *HevcFindRefIdx(HevcContex *pHevcDec, s32 nPoc)
{
    s32 i;
    s32 LtMask = (1 << pHevcDec->pSps->nLog2MaxPocLsb) - 1;

    for(i = 0; i < HEVC_MAX_DPB_SIZE; i++)
    {
        HevcFrame *pHf = &pHevcDec->DPB[i];
        if(pHf->pFbmBuffer && (pHf->nSequence == pHevcDec->nSeqDecode))
        {
            if(pHf->nPoc == nPoc)
                return pHf;
        }
    }

    for(i = 0; i < HEVC_MAX_DPB_SIZE; i++)
    {
        HevcFrame *pHf = &pHevcDec->DPB[i];
        if(pHf->pFbmBuffer && (pHf->nSequence == pHevcDec->nSeqDecode))
        {
            if((pHf->nPoc & LtMask) == nPoc)
                return pHf;
        }
    }
#if 0
    logw(" h265 decoder could not find reference picture with poc == %d, \
            current poc: %d, slice type: %d ",
            nPoc, pHevcDec->nPoc, pHevcDec->SliceHeader.eSliceType);
    for(i = 0; i < HEVC_MAX_DPB_SIZE; i++)
    {
        HevcFrame *pHf = &pHevcDec->DPB[i];
        if(pHf->nPoc == nPoc)
        {
            logd(" DPB buffer list. poc == %d, index: %d, pHf->pFbmBuffer: %x ",
                    nPoc, i, (u32)pHf->pFbmBuffer);
            logd(" pHf->nSequence: %d, pHevcDec->nSeqDecode: %d ",
                pHf->nSequence, pHevcDec->nSeqDecode);
        }
    }
#endif
    return NULL;
}

/* add a reference picture with the given poc to the list and mark it as used in DPB */
static s32 HevcAddCandidateRef(HevcContex *pHevcDec,
        HevcRefPicList *pRpsList, s32 nPoc, s32 bFlags)
{
    HevcFrame *pHf = HevcFindRefIdx(pHevcDec, nPoc);
    if(pHf == NULL)
    {
        if(pHevcDec->eNaluType == HEVC_NAL_RASL_N || pHevcDec->eNaluType == HEVC_NAL_RASL_R)
        {
            logw("******* drop the RASL nalu as miss ref_frame");
            return HEVC_RESULT_DROP_THE_FRAME;
        }

        if(HEVC_IS_KEYFRAME(pHevcDec))
           logd("this is a key frame, poc: %d, before frame poc: %d", pHevcDec->nPoc, nPoc);
        else
           logw(" ref frame missing, current poc: %d, ref poc: %d ",
                pHevcDec->nPoc, nPoc);
        if(nPoc == 0)
        {
            logd("*** clear refs as miss IDR frame ****");
            s32 bFlush = 1;
            HevcOutputFrame(pHevcDec, bFlush);
            HevcResetClearRefs(pHevcDec);
        }
        return HEVC_RESULT_OK;
    }

    logv("*** add ref: poc = %d, error flag = %d, displayFlag = %d, bflag = %d",
         pHf->nPoc, pHf->bErrorFrameFlag, pHevcDec->bDisplayErrorFrameFlag, bFlags);
    if(pHf->bErrorFrameFlag == 1 && pHevcDec->bDisplayErrorFrameFlag == 0
       && (bFlags == HEVC_FRAME_FLAG_SHORT_REF || bFlags == HEVC_FRAME_FLAG_LONG_REF))
    {
        logw("ref_frame is error, reset decoder to deocde from keyFrame");
        return HEVC_RESULT_RESET_INTRA_DECODER;
    }

    pRpsList->List[pRpsList->nNumOfRefs] = pHf->nPoc;
    pRpsList->Ref[pRpsList->nNumOfRefs] = pHf;
    pRpsList->nNumOfRefs++;
//    logd("pRpsList->nNumOfRefs: %d", pRpsList->nNumOfRefs);
    HevcMarkRef(pHf, bFlags);
    return HEVC_RESULT_OK;
}

void HevcClearRefs(HevcContex *pHevcDec)
{
    s32 i;
    s32 bFlags = HEVC_FRAME_FLAG_SHORT_REF | HEVC_FRAME_FLAG_LONG_REF;
    for (i = 0; i < HEVC_MAX_DPB_SIZE; i++)
    {
        HevcUnrefFrame(pHevcDec, &pHevcDec->DPB[i], bFlags);
    }
}

void HevcReturnUnusedBuffer(HevcContex *pHevcDec)
{
    CEDARC_UNUSE(HevcReturnUnusedBuffer);

    s32 i;
    for (i = 0; i < HEVC_MAX_DPB_SIZE; i++)
    {
        HevcUnrefFrame(pHevcDec, &pHevcDec->DPB[i], 0);
    }
}

void HevcResetClearRefs(HevcContex *pHevcDec)
{
    s32 i;
    s32 bFlags = -1;  /* clear all picture */
    for (i = 0; i < HEVC_MAX_DPB_SIZE; i++)
    {
        HevcUnrefFrame(pHevcDec, &pHevcDec->DPB[i], bFlags);
    }
}

/* return the pts index */
static inline s32 HevcFindAnAvailablePts(int64_t *PtsList)
{
    s32 i, nMinIdx;
    int64_t nPts = 0x7fffffffffffffff;
    nMinIdx = -1;
    for(i = 0; i < HEVC_PTS_LIST_SIZE; i++)
    {
        if(PtsList[i] >= 0)
        {
            if(PtsList[i] < nPts)
            {
                nPts = PtsList[i];
                nMinIdx = i;
            }
        }
    }
    return nMinIdx;
}

static s32 HevcInitialFBM(HevcContex *pHevcDec)
{
    s32 i, nFbmNum;
    s32 nWidth, nHeight;
    s32 nScaleWidth, nScaleHeight;
    HevcControlInfo *pCi = &pHevcDec->ControlInfo;
    int nAlignValue = 0;
    int nBufferType = 0;
    int nProgressiveFlag = 1;
    u32 nMaxDecPicBuffering = 0;
    u32 nMaxNumReoderPics = 0;
    u32 nExtraBufNum = 5;
    FbmCreateInfo mFbmCreateInfo;
    s32 flag1 = 0;
    s32 flag2 = 0;
    s32 nWidthTh = 0;
    s32 nHeightTh = 0;
    struct ScMemOpsS *_memops = pHevcDec->pConfig->memops;

    nFbmNum = 8; /* giving an initial number */
    if(pHevcDec->pSps == NULL)
    {
        loge(" initial fbm buffer error. sps == NULL ");
        return -1;
    }

    if(pHevcDec->b10BitStreamFlag == 1 && pHevcDec->nDecIpVersion != 0x31010)
    {
        loge("the chip(%llx) is not support 10bit-stream",pHevcDec->nIcVersion);
        return -1;
    }

    nWidth = (pHevcDec->pSps->nWidth + 31) & ~31;
    nHeight = (pHevcDec->pSps->nHeight + 31) & ~31;
    flag1 = (pHevcDec->H265ExtraScaleInfo.nHorizonScaleRatio>0) &&
            (pHevcDec->H265ExtraScaleInfo.nHorizonScaleRatio<=3);
    flag2 = (pHevcDec->H265ExtraScaleInfo.nVerticalScaleRatio>0) &&
            (pHevcDec->H265ExtraScaleInfo.nVerticalScaleRatio<=3);
    nWidthTh = (pHevcDec->H265ExtraScaleInfo.nWidthTh+63) & ~63;
    nHeightTh = (pHevcDec->H265ExtraScaleInfo.nHeightTh+63) & ~63;

    if((flag1==1)||(flag2==1))
    {
        if((pHevcDec->pConfig->nHorizonScaleDownRatio==0) &&
            (pHevcDec->pConfig->nVerticalScaleDownRatio==0))
        {
            if(nWidth>=nWidthTh || nHeight>=nHeightTh)
            {
                pHevcDec->pConfig->nHorizonScaleDownRatio =
                    pHevcDec->H265ExtraScaleInfo.nHorizonScaleRatio;
                pHevcDec->pConfig->nVerticalScaleDownRatio =
                    pHevcDec->H265ExtraScaleInfo.nVerticalScaleRatio;
                pHevcDec->pConfig->bScaleDownEn = 1;
            }
        }
    }

    nScaleWidth = 0;
    nScaleHeight = 0;

    i = pHevcDec->pSps->nMaxSubLayers - 1;
    nMaxNumReoderPics = pHevcDec->pSps->TemporalLayer[i].nSpsMaxNumReoderPics;
    nMaxDecPicBuffering = pHevcDec->pSps->TemporalLayer[i].nSpsMaxDecPicBuffering;
    nMaxDecPicBuffering = HEVCMAX(pHevcDec->nVpsMaxDecPicBuffering, nMaxDecPicBuffering);
    nMaxNumReoderPics = HEVCMAX(pHevcDec->nVpsNumReorderPics, nMaxNumReoderPics);

    nExtraBufNum = pHevcDec->pConfig->nDisplayHoldingFrameBufferNum +
        pHevcDec->pConfig->nRotateHoldingFrameBufferNum +
        pHevcDec->pConfig->nDecodeSmoothFrameBufferNum;
    logd("*** nExtraBufNum[%d] = displayHoldNum[%d] + rotateHoldNum[%d] + smoothNum[%d]",
         nExtraBufNum,pHevcDec->pConfig->nDisplayHoldingFrameBufferNum,
         pHevcDec->pConfig->nRotateHoldingFrameBufferNum,
         pHevcDec->pConfig->nDecodeSmoothFrameBufferNum);
#if 0
    if(pFbmInfo->nExtraFbmBufferNum < pFbmInfo->nDecoderNeededMiniFbmNum)
        pFbmInfo->nExtraFbmBufferNum = pFbmInfo->nDecoderNeededMiniFbmNum;

    if(pCi->secOutputEnabled)
        nFbmNum = nMaxDecPicBuffering + pFbmInfo->nDecoderNeededMiniFbmNum;
    else
        nFbmNum = nMaxDecPicBuffering + pFbmInfo->nDecoderNeededMiniFbmNum + nExtraBufNum;
#else
    if(pCi->secOutputEnabled)
    {
        nFbmNum = nMaxDecPicBuffering;
        logd("**(two output -- 1) nFbmNum[%d] = maxDPBNum[%d]",
            nFbmNum,nMaxDecPicBuffering);
    }
    else
    {
        nFbmNum = nMaxDecPicBuffering + nMaxNumReoderPics + nExtraBufNum;
        logd("**(one output) nFbmNum[%d] = maxDPBNum[%d] + maxReoderNum[%d] + extraNum[%d]",
            nFbmNum,nMaxDecPicBuffering,nMaxNumReoderPics,nExtraBufNum);
    }
#endif
    logd(" h265 initial fbm number: %d, reference pictures: %d. \
            reorder pictures: %d, real_w: %d, real_h: %d", \
            nFbmNum,nMaxDecPicBuffering,
            nMaxNumReoderPics,
            pHevcDec->pSps->nWidth, pHevcDec->pSps->nHeight);

    if(pCi->secOutputEnabled == 1)
    {
        nBufferType = BUF_TYPE_ONLY_REFERENCE;
    }
    else
    {
        nBufferType = BUF_TYPE_REFERENCE_DISP;
    }

    int bAdjustDramSpeedFlag = 0;
    if(pHevcDec->nDecIpVersion == 0x31010)
    {
        if(pHevcDec->pSps->nWidth >= ENABLE_HIGH_CHANNAL_SIZE_WIDTH
           || pHevcDec->pSps->nHeight >= ENABLE_HIGH_CHANNAL_SIZE_HEIGHT)
        {
            bAdjustDramSpeedFlag = 1;
        }
    }

    CdcVeSetAdjustDramSpeedFlag(pHevcDec->pConfig->veOpsS,
                                pHevcDec->pConfig->pVeOpsSelf,
                                bAdjustDramSpeedFlag);

    controlEnableAfbc(pHevcDec);
    controlEnableIptv(pHevcDec);

    CdcVeSetEnableAfbcFlag(pHevcDec->pConfig->veOpsS,
                           pHevcDec->pConfig->pVeOpsSelf,
                           pHevcDec->bEnableAfbcFlag);
    CdcVeEnableVe(pHevcDec->pConfig->veOpsS,pHevcDec->pConfig->pVeOpsSelf);
    logd("*************** pHevcDec->bEnableAfbcFlag = %d", pHevcDec->bEnableAfbcFlag);

    memset(&mFbmCreateInfo, 0, sizeof(FbmCreateInfo));

    if(pCi->secOutputEnabled)
        mFbmCreateInfo.nDecoderNeededMiniFrameNum = nMaxDecPicBuffering;
    else
        mFbmCreateInfo.nDecoderNeededMiniFrameNum = nMaxDecPicBuffering + nMaxNumReoderPics;

    mFbmCreateInfo.nFrameNum          = nFbmNum;
    mFbmCreateInfo.nWidth             = pHevcDec->pSps->nWidth/*nWidth*/;
    mFbmCreateInfo.nHeight            = pHevcDec->pSps->nHeight/*nHeight*/;
    mFbmCreateInfo.ePixelFormat       = pHevcDec->eDisplayPixelFormat;
    mFbmCreateInfo.bThumbnailMode     = pHevcDec->pConfig->bThumbnailMode;
    //mFbmCreateInfo.callback           = pHevcDec->pConfig->callback;
    //mFbmCreateInfo.pUserData          = pHevcDec->pConfig->pUserData;
    mFbmCreateInfo.bGpuBufValid       = pHevcDec->pConfig->bGpuBufValid;
    mFbmCreateInfo.nAlignStride       = pHevcDec->pConfig->nAlignStride;
    mFbmCreateInfo.nBufferType        = nBufferType;
    mFbmCreateInfo.bProgressiveFlag   = nProgressiveFlag;
    mFbmCreateInfo.bIsSoftDecoderFlag = pHevcDec->pConfig->bIsSoftDecoderFlag;
    mFbmCreateInfo.memops             = pHevcDec->pConfig->memops;
    mFbmCreateInfo.b10BitStreamFlag   = pHevcDec->b10BitStreamFlag;
    mFbmCreateInfo.veOpsS             = pHevcDec->pConfig->veOpsS;
    mFbmCreateInfo.pVeOpsSelf         = pHevcDec->pConfig->pVeOpsSelf;

    logd("b10BitStreamFlag = %d, %d",mFbmCreateInfo.b10BitStreamFlag, pHevcDec->b10BitStreamFlag);
    pHevcDec->pFbmInfo->pFbmBufInfo.bAfbcModeFlag = pHevcDec->bEnableAfbcFlag;
    pHevcDec->pFbmInfo->pFbmBufInfo.bHdrVideoFlag = pHevcDec->SeiDisInfo.bHdrFlag;

    pHevcDec->fbm = FbmCreate(&mFbmCreateInfo, pHevcDec->pFbmInfo);

    if(pHevcDec->fbm == NULL)
    {
        loge(" h265 initial fbm error. width: %d, height: %d, fbm num: %d ",
                nWidth, nHeight, nFbmNum);
        return -1;
    }
#if HEVC_DECODE_TIME_INFO
    pHevcDec->debug.nStartTime = HevcGetCurrentTime();
    pHevcDec->debug.nCurrTime = pHevcDec->debug.nStartTime;
#endif //HEVC_DECODE_TIME_INFO
    nAlignValue = FbmGetAlignValue(pHevcDec->fbm);
    GetBufferSize(pHevcDec->eDisplayPixelFormat, pHevcDec->pSps->nWidth,
            pHevcDec->pSps->nHeight,
            &pCi->priFrmBufLumaSize,
            &pCi->priFrmBufChromaSize, &pCi->priFrmBufLumaStride,
            &pCi->priFrmBufChromaStride, nAlignValue);

    if(pCi->secOutputEnabled)
    {
        i = pHevcDec->pSps->nMaxSubLayers - 1;
        nFbmNum = nMaxNumReoderPics + nExtraBufNum;
        nScaleWidth = ((pHevcDec->pSps->nWidth>>pCi->nHorizonScaleDownRatio) + 31) & ~31;
        nScaleHeight = ((pHevcDec->pSps->nHeight>>pCi->nVerticalScaleDownRatio) + 31) & ~31;

        logd("**(two output -- 2) nFbmNum[%d] = maxReorderNum[%d] + nExtraBufNum[%d]",
            nFbmNum,nMaxNumReoderPics,nExtraBufNum);

        memset(&mFbmCreateInfo, 0, sizeof(FbmCreateInfo));
        mFbmCreateInfo.nFrameNum          = nFbmNum;
        mFbmCreateInfo.nDecoderNeededMiniFrameNum = /*nMaxDecPicBuffering*/ nMaxDecPicBuffering;
        mFbmCreateInfo.nWidth             = nScaleWidth;
        mFbmCreateInfo.nHeight            = nScaleHeight;
        mFbmCreateInfo.ePixelFormat       = pHevcDec->eDisplayPixelFormat;
        mFbmCreateInfo.bThumbnailMode     = pHevcDec->pConfig->bThumbnailMode;
        //mFbmCreateInfo.callback           = pHevcDec->pConfig->callback;
        //mFbmCreateInfo.pUserData          = pHevcDec->pConfig->pUserData;
        mFbmCreateInfo.bGpuBufValid       = pHevcDec->pConfig->bGpuBufValid;
        mFbmCreateInfo.nAlignStride       = pHevcDec->pConfig->nAlignStride;
        mFbmCreateInfo.nBufferType        = BUF_TYPE_ONLY_DISP;
        mFbmCreateInfo.bProgressiveFlag   = nProgressiveFlag;
        mFbmCreateInfo.bIsSoftDecoderFlag = pHevcDec->pConfig->bIsSoftDecoderFlag;
        mFbmCreateInfo.memops             = pHevcDec->pConfig->memops;
        pHevcDec->pFbmInfo->pFbmBufInfo.bAfbcModeFlag = 0;
        pHevcDec->pFbmInfo->pFbmBufInfo.bHdrVideoFlag = 0;
        mFbmCreateInfo.veOpsS             = pHevcDec->pConfig->veOpsS;
        mFbmCreateInfo.pVeOpsSelf         = pHevcDec->pConfig->pVeOpsSelf;

        pHevcDec->SecondFbm = FbmCreate(&mFbmCreateInfo, pHevcDec->pFbmInfo);

        if(pHevcDec->SecondFbm == NULL)
        {
            loge(" h265 initial secode fbm error. width: %d, height: %d, fbm num: %d ",
                    nScaleWidth, nScaleHeight, nFbmNum);
            return -1;
        }
        logd(" second fbm initial num: %d,  width: %d, height: %d, reorder pictures number: %d",
                nFbmNum, nScaleWidth, nScaleHeight,
                pHevcDec->pSps->TemporalLayer[i].nSpsMaxNumReoderPics);

        nAlignValue = FbmGetAlignValue(pHevcDec->SecondFbm);
        GetBufferSize(pHevcDec->eDisplayPixelFormat, nScaleWidth,
                nScaleHeight, &pCi->secFrmBufLumaSize,
                &pCi->secFrmBufChromaSize, &pCi->secFrmBufLumaStride,
                &pCi->secFrmBufChromaStride, nAlignValue);
    }
    if((pHevcDec->pStreamInfo->pSbm->bUseNewVeMemoryProgram == 0)||     \
        !ENABLE_MV_BUF_OPTIMIZATION_PROGRAM)
    {
        for(i = 0; i < HEVC_MAX_DPB_SIZE; i++)
        {
            HevcFrame *pHf = &pHevcDec->DPB[i];
            if(pHf->pMVFrameCol == NULL)
            {
                pHf->pMVFrameCol = HevcAdapterPalloc(_memops,
                    pHevcDec->pSps->nCtbSize*160 + 1024,
                    (void *)pHevcDec->pConfig->veOpsS,
                    pHevcDec->pConfig->pVeOpsSelf);/* todo *160 */
                if(pHf->pMVFrameCol == NULL)
                {
                    loge(" h265 decoder palloc error, mv frame col, nCtbSize: %d ",
                        pHevcDec->pSps->nCtbSize*160);
                    return -1;
                }
                pHf->MVFrameColPhyAddr = (size_addr)CdcMemGetPhysicAddress(_memops,
                    (void*)pHf->pMVFrameCol);
                // todo: soppurt bottom fiel case
            }
        }
    }
    else
    {
        pHevcDec->pHevcMvBufInf = (HevcMvBufInf*)malloc(nMaxDecPicBuffering*sizeof(HevcMvBufInf));
        if(pHevcDec->pHevcMvBufInf == NULL)
        {
            loge("malloc memory for pHevcDec->pHevcMvBufInf  failed\n");
            return -1;
        }

        pHevcDec->nHevcMvBufNum = (s32)(nMaxDecPicBuffering+1);

        for(i=0; i<(s32)pHevcDec->nHevcMvBufNum; i++)
         {
             pHevcDec->pHevcMvBufInf[i].pMVFrameColManager =      \
                (HevcMvBufManager*)malloc(sizeof(HevcMvBufManager));
             if(pHevcDec->pHevcMvBufInf[i].pMVFrameColManager == NULL)
             {
                return -1;
             }
             pHevcDec->pHevcMvBufInf[i].pMVFrameColManager->pMVFrameCol = HevcAdapterPalloc(_memops,
                    pHevcDec->pSps->nCtbSize*160 + 1024,
                    (void *)pHevcDec->pConfig->veOpsS,
                    pHevcDec->pConfig->pVeOpsSelf);/* todo *160 */
            if(pHevcDec->pHevcMvBufInf[i].pMVFrameColManager->pMVFrameCol == NULL)
            {
                loge(" h265 decoder palloc error, mv frame col, nCtbSize: %d ",
                        pHevcDec->pSps->nCtbSize*160);
                return -1;
            }
            pHevcDec->pHevcMvBufInf[i].pMVFrameColManager->MVFrameColPhyAddr =
                  (size_addr)CdcMemGetPhysicAddress(_memops,
                    (void*)pHevcDec->pHevcMvBufInf[i].pMVFrameColManager->pMVFrameCol);
            pHevcDec->pHevcMvBufInf[i].pMVFrameColManager->nMvFrameBufIndex = i;
            FIFOEnqueue((FiFoQueueInst **)&(pHevcDec->pHevcMvBufEmptyQueue),
                  (FiFoQueueInst *)&(pHevcDec->pHevcMvBufInf[i]));
        }
    }


#if 0
    logd(" ------------------------  picture info start ----------------------");
    logd(" width: %d,  height: %d,  align w: %d, h: %d, ctb_w: %d, ctb_h: %d",
            pHevcDec->pSps->nWidth, pHevcDec->pSps->nHeight, nWidth, nHeight,
            (1 << pHevcDec->pSps->nLog2CtbSize), (1 << pHevcDec->pSps->nLog2CtbSize));
    logd(" nMaxDecPicBuffering: %d, reorder pictures: %d  ",
            nMaxDecPicBuffering,
            nMaxNumReoderPics);
    logd(" luma stride: %d, chroma stride: %d, display format: %d",
            pCi->priFrmBufLumaStride, pCi->priFrmBufChromaStride, pHevcDec->eDisplayPixelFormat);
    logd(" luma size: %d, chroma size: %d", pCi->priFrmBufLumaSize, pCi->priFrmBufChromaSize);
    if(pCi->secOutputEnabled)
    {
        logd(" scale down enable. scale down width: %d, scale dowm height: %d",
                nScaleWidth, nScaleHeight);
    }
    logd("-------------------------  picture info end  ----------------------");
#endif
    return HEVC_RESULT_OK;
}

static void HevcInitAfbcInfo(HevcContex *pHevcDec, HevcFrame *pRef,
                             struct afbc_header *pAfbcHeader,
                             u8 nTopCrop, u8 nHeaderLayout,
                             u8 nYuvTransform, u8 nBlockSplit, u8 nMbSize)
{
    //pAfbcHeader->signature = 'A' | 'F'<<8 | 'B'<<16 | 'C'<<24;
    pAfbcHeader->signature    = 0x43424641;//AFBC -> CBFA
    pAfbcHeader->filehdr_size = AFBC_FILEHEADER_SIZE;
    pAfbcHeader->version      = AFBC_VERSION;
    pAfbcHeader->body_size    = pRef->pFbmBuffer->nAfbcSize;

    pAfbcHeader->ncomponents = 3;//y:ncomponents[0] = 1; uv:ncomponents[1] = 2;

    pAfbcHeader->header_layout = nHeaderLayout;
    pAfbcHeader->yuv_transform = nYuvTransform;
    pAfbcHeader->block_split   = nBlockSplit;

    if(pHevcDec->b10BitStreamFlag)
    {
        pAfbcHeader->inputbits[0] = 10;//y
        pAfbcHeader->inputbits[1] = 10;//u
        pAfbcHeader->inputbits[2] = 10;//v
        pAfbcHeader->inputbits[3] =  0;
    }
    else
    {
        pAfbcHeader->inputbits[0] = 8;//y
        pAfbcHeader->inputbits[1] = 8;//u
        pAfbcHeader->inputbits[2] = 8;//v
        pAfbcHeader->inputbits[3] = 0;
    }
    pAfbcHeader->left_crop     = 0;
    pAfbcHeader->top_crop      = nTopCrop;
    pAfbcHeader->block_width   = (pHevcDec->pSps->nWidth + 0/*left_crop*/ + nMbSize - 1) / nMbSize;
    pAfbcHeader->block_height  = (pHevcDec->pSps->nHeight + nTopCrop + nMbSize - 1) / nMbSize;
    pAfbcHeader->width         = pHevcDec->pSps->nWidth;
    pAfbcHeader->height        = pHevcDec->pSps->nHeight;
    pAfbcHeader->block_layout  = 0;//file_message:(0x)xx00 -> xx =00;
}

s32 HevcSetNewRef(HevcContex *pHevcDec, s32 nPoc)
{
    s32 i, nRet;
    HevcFrame *pRef;
    s32 bPocErrorFlag = 0;
    struct display_master_data dispMaster;
    struct afbc_header afbcHeader;
    struct sunxi_metadata mMetaData;

    for(i = 0; i < HEVC_MAX_DPB_SIZE; i++)
    {
        HevcFrame *pHf = &pHevcDec->DPB[i];
        if(pHf->pFbmBuffer && pHf->nSequence == pHevcDec->nSeqDecode &&
                pHf->nPoc == nPoc && !pHevcDec->nNuhLayerId)
        {
            logw(" current POC(%d) is in DPB, clear refs as maybe lost large stream-data", nPoc);
            bPocErrorFlag = 1;
            s32 bFlush = 1;
            HevcOutputFrame(pHevcDec, bFlush);
            HevcResetClearRefs(pHevcDec);
        }
    }
    if(pHevcDec->bFbmInitFlag == 0)
    {
        s32 nTryTime = 0;
        nRet = HevcInitialFBM(pHevcDec);
        if(nRet < 0)
            return HEVC_RESULT_INIT_FBM_FAILED;

        pHevcDec->bFbmInitFlag = 1;

        /* Maybe need some time to get a fbm buffer, we set the max time is 1 second */
        do{
            pHevcDec->pCurrFrame = FbmRequestBuffer(pHevcDec->fbm);
            if(pHevcDec->pCurrFrame == NULL)
                usleep(1000);
            nTryTime++;
        }while(nTryTime < 1000 && pHevcDec->pCurrFrame == NULL);
        if(nTryTime >= 1000 && pHevcDec->pCurrFrame == NULL)
        {
            loge(" h265 decoder cost request fbm buffer timeout ");
        }
        if(pHevcDec->pCurrFrame == NULL)
        {
            logw(" first fbm requests no buffer after inition");
            return HEVC_RESULT_NO_FRAME_BUFFER;
        }
        if(pHevcDec->ControlInfo.secOutputEnabled)
        {
            s32 nTryTime = 0;
            do{
            pHevcDec->pCurrSecFrame = FbmRequestBuffer(pHevcDec->SecondFbm);
                if(pHevcDec->pCurrSecFrame == NULL)
                    usleep(1000);
                nTryTime++;
            }while(nTryTime < 1000 && pHevcDec->pCurrSecFrame  == NULL);
            if(nTryTime >= 1000 && pHevcDec->pCurrSecFrame == NULL)
            {
                loge(" h265 decoder cost request second fbm buffer timeout ");
            }
            if(pHevcDec->pCurrSecFrame == NULL)
            {
                logw(" second fbm requests no buffer after inition");
                return HEVC_RESULT_NO_FRAME_BUFFER;
            }
        }
    }

    //* reqeust fbm buffer here after construct ref
    if(pHevcDec->pCurrFrame == NULL)
    {
        pHevcDec->pCurrFrame = FbmRequestBuffer(pHevcDec->fbm);
        if(pHevcDec->pCurrFrame == NULL)
        {
            logv(" no frame buffer first fbm");
            return HEVC_RESULT_NO_FRAME_BUFFER;
        }
    }
    if(pHevcDec->ControlInfo.secOutputEnabled)
    {
        if(pHevcDec->pCurrSecFrame == NULL)
            pHevcDec->pCurrSecFrame = FbmRequestBuffer(pHevcDec->SecondFbm);
        if(pHevcDec->pCurrSecFrame == NULL)
        {
            logv(" no frame buffer second fbm ");
            return HEVC_RESULT_NO_FRAME_BUFFER;
        }
    }

    for(i = 0; i < HEVC_MAX_DPB_SIZE; i++)
    {
        pRef = &pHevcDec->DPB[i];
        if(pRef->pFbmBuffer == NULL)
            break;
    }
    if(i == HEVC_MAX_DPB_SIZE)
    {
        loge(" h265 DPB buffer is full ");
    }
    pRef->pFbmBuffer = pHevcDec->pCurrFrame;

    if((pHevcDec->pStreamInfo->pSbm->bUseNewVeMemoryProgram==1)&&ENABLE_MV_BUF_OPTIMIZATION_PROGRAM)
    {
          HevcMvBufInf* pHrvcMbBufNode =
           (HevcMvBufInf*)FIFODequeue((FiFoQueueInst **)&pHevcDec->pHevcMvBufEmptyQueue);
          if(pHrvcMbBufNode == NULL)
          {
                while(1)
                {
                    loge("error: cannot get the empty pHrvcMbBufNode\n");
                }
          }

          pRef->MVFrameColPhyAddr = pHrvcMbBufNode->pMVFrameColManager->MVFrameColPhyAddr;
          pRef->pMVFrameCol       = pHrvcMbBufNode->pMVFrameColManager->pMVFrameCol;
          pRef->nMvFrameBufIndex  = pHrvcMbBufNode->pMVFrameColManager->nMvFrameBufIndex;
    }
    pHevcDec->pCurrFrame = NULL;
    if(HEVC_IS_KEYFRAME(pHevcDec))
        pRef->bKeyFrame = 1;
    else
        pRef->bKeyFrame = 0;
    pRef->nPoc = nPoc;
    pRef->bFlags = HEVC_FRAME_FLAG_OUTPUT | HEVC_FRAME_FLAG_SHORT_REF;
    if(pHevcDec->SliceHeader.bPicOutputFlag == 0)
        pRef->bFlags &= ~(HEVC_FRAME_FLAG_OUTPUT);
    pRef->nSequence = pHevcDec->nSeqDecode;


    pRef->pFbmBuffer->nCurFrameInfo.nVidFrmQP = pHevcDec->SliceHeader.nSliceQp;
    pRef->pFbmBuffer->nCurFrameInfo.nVidFrmDisW = \
         pRef->pFbmBuffer->nRightOffset - pRef->pFbmBuffer->nLeftOffset;
    pRef->pFbmBuffer->nCurFrameInfo.nVidFrmDisH = \
         pRef->pFbmBuffer->nBottomOffset - pRef->pFbmBuffer->nTopOffset;
    pRef->pFbmBuffer->nTopOffset = pHevcDec->pSps->nCedarxTopOffset;
    pRef->pFbmBuffer->nBottomOffset = pHevcDec->pSps->nCedarxBottomOffset;
    pRef->pFbmBuffer->nLeftOffset = pHevcDec->pSps->nCedarxLeftOffset;
    pRef->pFbmBuffer->nRightOffset = pHevcDec->pSps->nCedarxRightOffset;
    logv(" offset: top: %d, bottom: %d, left: %d, right: %d, \
            display w: %d, display h: %d ",
            pRef->pFbmBuffer->nTopOffset, pRef->pFbmBuffer->nBottomOffset,
            pRef->pFbmBuffer->nLeftOffset, pRef->pFbmBuffer->nRightOffset,
            pRef->pFbmBuffer->nRightOffset - pRef->pFbmBuffer->nLeftOffset,
            pRef->pFbmBuffer->nBottomOffset - pRef->pFbmBuffer->nTopOffset);

    /* todo: pRef->pFbmBuffer->nWidth and pRef->pFbmBuffer->nHeight no need to write a value,
     * otherwise the picture showing would error. */
    pRef->pFbmBuffer->ePixelFormat = pHevcDec->eDisplayPixelFormat;
    pRef->LuamPhyAddr = (size_addr)pRef->pFbmBuffer->phyYBufAddr;
    pRef->ChromaPhyAddr = pRef->pFbmBuffer->phyCBufAddr;
    pRef->nLower2bitBufOffset = pRef->pFbmBuffer->nLower2BitBufOffset;
    pRef->nLower2bitStride    = pRef->pFbmBuffer->nLower2BitBufStride;

    pRef->pFbmBuffer->video_full_range_flag    = pHevcDec->pSps->vui.bVideoFullRangeFlag;
    pRef->pFbmBuffer->colour_primaries         = pHevcDec->pSps->vui.nColourPrimaries;
    if(pHevcDec->SeiDisInfo.prefTranChara == 18)
        pHevcDec->pSps->vui.nTransferCharacteristic = 18;
    pRef->pFbmBuffer->transfer_characteristics = pHevcDec->pSps->vui.nTransferCharacteristic;
    pRef->pFbmBuffer->matrix_coeffs            = pHevcDec->pSps->vui.nMatrixCoeffs;

    /*fill the struct of hdr_static_metadata*/
    memcpy(dispMaster.display_primaries_x, pHevcDec->SeiDisInfo.displayPrimariesX, 3*sizeof(u16));
    memcpy(dispMaster.display_primaries_y, pHevcDec->SeiDisInfo.displayPrimariesY, 3*sizeof(u16));
    dispMaster.max_display_mastering_luminance = pHevcDec->SeiDisInfo.maxMasteringLuminance;
    dispMaster.min_display_mastering_luminance = pHevcDec->SeiDisInfo.minMasteringLuminance;
    dispMaster.white_point_x = pHevcDec->SeiDisInfo.whitePointX;
    dispMaster.white_point_y = pHevcDec->SeiDisInfo.whitePointY;

    mMetaData.hdr_smetada.disp_master = dispMaster;
    mMetaData.hdr_smetada.maximum_content_light_level =
        pHevcDec->SeiDisInfo.maxContentLightLevel;
    mMetaData.hdr_smetada.maximum_frame_average_light_level =
        pHevcDec->SeiDisInfo.maxPicAverLightLevel;

   /*fill the struct of afbc_header*/
    u8 topCrop = 4;
    u8 headerLayout = 1;
    u8 yuvTransform = 0;
    u8 blockSplit = 1;
    u8 mbSize = 16;
    HevcInitAfbcInfo(pHevcDec, pRef, &afbcHeader,
                     topCrop, headerLayout,
                     yuvTransform, blockSplit, mbSize);
    mMetaData.afbc_head = afbcHeader;
    mMetaData.flag = 0;
    if(pHevcDec->bEnableAfbcFlag)
        mMetaData.flag |= SUNXI_METADATA_FLAG_AFBC_HEADER;
    if(pHevcDec->SeiDisInfo.bHdrFlag)
        mMetaData.flag |= SUNXI_METADATA_FLAG_HDR_SATIC_METADATA;

    if(pRef->pFbmBuffer->pMetaData && pHevcDec->ControlInfo.secOutputEnabled != 1)
    {
        logv("pMetaData = %p, is not NULL", pRef->pFbmBuffer->pMetaData);
        memset(pRef->pFbmBuffer->pMetaData, 0, sizeof(struct sunxi_metadata));
        memcpy(pRef->pFbmBuffer->pMetaData, (void *)&mMetaData, sizeof(struct sunxi_metadata));
    }
    if(pHevcDec->ControlInfo.secOutputEnabled)
    {
        pRef->pSecFbmBuffer = pHevcDec->pCurrSecFrame;
        pRef->SecLuamPhyAddr = (size_addr)pRef->pSecFbmBuffer->phyYBufAddr;
        pRef->SecChromaPhyAddr = (size_addr)pRef->pSecFbmBuffer->phyCBufAddr;
#if 0
        pRef->pFbmBuffer->nTopOffset = pHevcDec->pSps->nCedarxTopOffsetSd;
        pRef->pFbmBuffer->nBottomOffset = pHevcDec->pSps->nCedarxBottomOffsetSd;
        pRef->pFbmBuffer->nLeftOffset = pHevcDec->pSps->nCedarxLeftOffsetSd;
        pRef->pFbmBuffer->nRightOffset = pHevcDec->pSps->nCedarxRightOffsetSd;
        logd(" Scale down offset: top: %d, bottom: %d, left: %d, right: %d ",
                pRef->pFbmBuffer->nTopOffset, pRef->pFbmBuffer->nBottomOffset,
                pRef->pFbmBuffer->nLeftOffset, pRef->pFbmBuffer->nRightOffset);
#else
        pRef->pSecFbmBuffer->nTopOffset = pHevcDec->pSps->nCedarxTopOffsetSd;
        pRef->pSecFbmBuffer->nBottomOffset = pHevcDec->pSps->nCedarxBottomOffsetSd;
        pRef->pSecFbmBuffer->nLeftOffset = pHevcDec->pSps->nCedarxLeftOffsetSd;
        pRef->pSecFbmBuffer->nRightOffset = pHevcDec->pSps->nCedarxRightOffsetSd;
        logv(" Scale down offset: top: %d, bottom: %d, left: %d, right: %d ",
                pRef->pSecFbmBuffer->nTopOffset, pRef->pSecFbmBuffer->nBottomOffset,
                pRef->pSecFbmBuffer->nLeftOffset, pRef->pSecFbmBuffer->nRightOffset);
#endif

        pRef->pSecFbmBuffer->nCurFrameInfo.nVidFrmQP = pHevcDec->SliceHeader.nSliceQp;
        pRef->pSecFbmBuffer->nCurFrameInfo.nVidFrmDisW = \
             pRef->pSecFbmBuffer->nRightOffset - pRef->pSecFbmBuffer->nLeftOffset;
        pRef->pSecFbmBuffer->nCurFrameInfo.nVidFrmDisH = \
             pRef->pSecFbmBuffer->nBottomOffset - pRef->pSecFbmBuffer->nTopOffset;

        pRef->pSecFbmBuffer->ePixelFormat = pHevcDec->eDisplayPixelFormat;
        pHevcDec->pCurrSecFrame = NULL;
    }
    if(bPocErrorFlag == 1)
        pRef->bErrorFrameFlag = 1;
    else
        pRef->bErrorFrameFlag = 0;
    pHevcDec->pCurrDPB = pRef;

    if(pHevcDec->bDropFrameAdaptiveEnable)
    {
        switch(pHevcDec->eNaluType)
        {
        case HEVC_NAL_TRAIL_N:
        case HEVC_NAL_TSA_N:
        case HEVC_NAL_STSA_N:
        case HEVC_NAL_RADL_N:
        case HEVC_NAL_RASL_N:
            pRef->bDropFlag = 1;
            pHevcDec->debug.nDecoderDropFrameNum++;
            logw(" hevc decoder drop one frame. poc: %d ", pHevcDec->nPoc);

            if(pHevcDec->H265PerformInf.performCtrlFlag & H265_PERFORM_CALDROPFRAME)
            {
                pHevcDec->H265PerformInf.H265PerfInfo.nDropFrameNum++;
            }
            break;
        default:
            pRef->bDropFlag = 0;
        }
    }
    else
        pRef->bDropFlag = 0;
    return HEVC_RESULT_OK;
}

s32 HevcConstuctFrameRps(HevcContex *pHevcDec)
{
    const HevcShortTermRPS *pShortRps = pHevcDec->SliceHeader.pShortTermRps;
    HevcLongTermRPS  *pLongRps  = &pHevcDec->SliceHeader.LongTermRps;
    HevcRefPicList *pRps = pHevcDec->Rps;
    s32 i, nRet;

    if(!pShortRps)
    {
        pRps[0].nNumOfRefs = pRps[1].nNumOfRefs = 0;
        if(!pLongRps || (pLongRps && pLongRps->nNumOfRefs == 0))
            return HEVC_RESULT_OK;
    }
    /* clear the reference flags on all frames  */
//    logd(" setting rps start ... pRps[0].nNumOfRefs: %d,  pRps[1].nNumOfRefs, %d",
//            pRps[0].nNumOfRefs, pRps[1].nNumOfRefs);
    for(i = 0; i < HEVC_MAX_DPB_SIZE; i++)
    {
        HevcFrame *pRef = &pHevcDec->DPB[i];
        if(pRef == NULL)
            continue;
        HevcMarkRef(pRef, 0);
    }
    for (i = 0; i < HEVC_NB_RPS_TYPE; i++)
        pRps[i].nNumOfRefs = 0;
    /* short refs */
    for(i = 0; pShortRps && i < pShortRps->nNumDeltaPocs; i++)
    {
        s32 nPoc = pHevcDec->nPoc + pShortRps->nDeltaPoc[i];
        s32 nListIdx;
        if(!pShortRps->bUsed[i])
            nListIdx = HEVC_ST_FOLL;
        else if(i < pShortRps->nNumNegativePics)
            nListIdx = HEVC_ST_CURR_BEF;
        else
            nListIdx = HEVC_ST_CURR_AFT;

        nRet = HevcAddCandidateRef(pHevcDec, &pRps[nListIdx], nPoc, HEVC_FRAME_FLAG_SHORT_REF);
        if(nRet != 0)
            return nRet;
    }

    /* long refs */
    for(i = 0; pLongRps && i < pLongRps->nNumOfRefs; i++)
    {
        s32 nPoc = pLongRps->nPoc[i];
        s32 nListIdx = pLongRps->bUsed[i] ? HEVC_LT_CURR : HEVC_LT_FOLL;

        nRet = HevcAddCandidateRef(pHevcDec, &pRps[nListIdx], nPoc, HEVC_FRAME_FLAG_LONG_REF);
        if(nRet != 0)
            return nRet;
    }
    for(i = 0; i < HEVC_MAX_DPB_SIZE; i++)
        HevcUnrefFrame(pHevcDec, &pHevcDec->DPB[i], 0);

    HevcShowRefPicSetInfoDebug(pRps);

    return HEVC_RESULT_OK;
}

s32 HevcSliceRps(HevcContex *pHevcDec)
{
    HevcSliceHeader *pSh = &pHevcDec->SliceHeader;
    s32 i, j;
    s32 nNumOfList = pSh->eSliceType == HEVC_B_SLICE ? 2 : 1;
    s32 nListIdx;

//    logd(" HevcSliceRps start ");
    pHevcDec->bCurPicWithoutRefInfo = 0;
    if(!(pHevcDec->Rps[HEVC_ST_CURR_BEF].nNumOfRefs +
            pHevcDec->Rps[HEVC_ST_CURR_AFT].nNumOfRefs +
            pHevcDec->Rps[HEVC_LT_CURR].nNumOfRefs))
    {
        loge(" h265 construct ref pic list error. ref frame number == 0 ");
        pHevcDec->bCurPicWithoutRefInfo = 1;
        return -1;
        //return 0;
    }
    for(nListIdx = 0; nListIdx < nNumOfList; nListIdx++)
    {
        /* The order of the elements is
         * HEVC_ST_CURR_BEF --> HEVC_ST_CURR_AFT --> HEVC_LT_CURR for the L0 and
         * HEVC_ST_CURR_AFT --> HEVC_ST_CURR_BEF --> HEVC_LT_CURR for the L1 */
        s32 CandLists[3] = {nListIdx ? HEVC_ST_CURR_AFT : HEVC_ST_CURR_BEF,
                            nListIdx ? HEVC_ST_CURR_BEF : HEVC_ST_CURR_AFT,
                            HEVC_LT_CURR};
        HevcRefPicList *pRpl;
        HevcRefPicList RplTemp;
        memset(&RplTemp, 0, sizeof(HevcRefPicList));
        pRpl = &pHevcDec->RefPicList[nListIdx];
        pRpl->nNumOfRefs = 0;

        /* concatenate the candidate lists for the current frame */
//        logd(" pSh->nNumOfRefs[%d]: %d ", nListIdx, pSh->nNumOfRefs[nListIdx]);
        if(pSh->nNumOfRefs[nListIdx] > HEVC_MAX_REFS)
        {
            pHevcDec->pCurrDPB->bDropFlag = 1;
            return -1;
        }
        while(RplTemp.nNumOfRefs < pSh->nNumOfRefs[nListIdx])
        {
            for(i = 0; i < 3; i++)
            {
                HevcRefPicList *pRps = &pHevcDec->Rps[CandLists[i]];
                for(j = 0; j < pRps->nNumOfRefs && RplTemp.nNumOfRefs < HEVC_MAX_REFS; j++)
                {
                    RplTemp.List[RplTemp.nNumOfRefs] = pRps->List[j];
                    RplTemp.Ref[RplTemp.nNumOfRefs] = pRps->Ref[j];
                    RplTemp.bIsLongTerm[RplTemp.nNumOfRefs] = (i == 2);
                    RplTemp.nNumOfRefs++;
                }
            }
        }

        /* reorder the references if necessary */
//        logd(" pSh->bRplModificationFlag[%d]: %d ",
//         nListIdx, pSh->bRplModificationFlag[nListIdx]);
        if(pSh->bRplModificationFlag[nListIdx])
        {
            for(i = 0; i < pSh->nNumOfRefs[nListIdx]; i++)
            {
                s32 nIdx = pSh->ListEntryLx[nListIdx][i];
                if(/*!s->decoder_id &&*/ nIdx >= RplTemp.nNumOfRefs)
                {
                    loge(" h265 construct ref pic list error. Modification index invalid ");
                    pHevcDec->bCurPicWithoutRefInfo = 1;
                    return -1;
                }
                pRpl->List[i] = RplTemp.List[nIdx];
                pRpl->Ref[i] = RplTemp.Ref[nIdx];
                pRpl->bIsLongTerm[i] = RplTemp.bIsLongTerm[nIdx];
                pRpl->nNumOfRefs++;
            }
        }
        else
        {
            memcpy(pRpl, &RplTemp, sizeof(HevcRefPicList));
            pRpl->nNumOfRefs = HEVCMIN(pRpl->nNumOfRefs, pSh->nNumOfRefs[nListIdx]);
        }
        /*
        if(pSh->nCollocatedList == nListIdx &&
                pSh->nCollocatedRefIdx == pRpl->nNumOfRefs)
            pHevcDec->pCurrDPB->pCollocatedRef = pRpl->Ref[pSh->nCollocatedRefIdx];
        */
    }
    HevcShowSliceRplInfoDebug(pHevcDec);
//    logd(" end of HevcSliceRps");
    return HEVC_RESULT_OK;
}

s32 HevcOutputFrame(HevcContex *pHevcDec, s32 bFlush)
{
    u32 nNumOutput;
    s32 nMinPoc;
    s32 i, nMinIdx, nRet, nPtsIdx;
    u32 nNumOfValidPic = 0;
    nRet = 0;
    u32 nSpsMaxNumReoderPics = 0;
    int index = 0;

    //* not check output frame when had no sps,pps
    if(pHevcDec->bHadFindSpsFlag == HEVC_FALSE
       || pHevcDec->bHadFindPpsFlag == HEVC_FALSE
       || pHevcDec->pSps == NULL)
    {
        return HEVC_RESULT_OK;
    }
    index = pHevcDec->pSps->nMaxSubLayers - 1;
    nSpsMaxNumReoderPics = pHevcDec->pSps->TemporalLayer[index].nSpsMaxNumReoderPics;
    //index = pHevcDec->pSps->nMaxSubLayers - 1;
    //nSpsMaxDecPicBuffering = pHevcDec->pSps->TemporalLayer[index].nSpsMaxDecPicBuffering + 1;

    do{
        nNumOutput = 0;
        nMinPoc = HEVC_INT_MAX;
        nMinIdx = 0;
        nNumOfValidPic = 0;

        for(i = 0; i < HEVC_MAX_DPB_SIZE; i++)
        {
            HevcFrame *pHf = &pHevcDec->DPB[i];
            if((pHf->bFlags & HEVC_FRAME_FLAG_OUTPUT) &&
                pHf->nSequence == pHevcDec->nSeqOutput)
            {
                nNumOutput++;
                if(pHf->nPoc < nMinPoc)
                {
                    nMinPoc = pHf->nPoc;
                    nMinIdx = i;
                }
            }
            if(pHf->bFlags)
                nNumOfValidPic++;
        }

        s32 nNeedOutputFlag = 0;
        if(bFlush == 1
           || pHevcDec->nSeqOutput != pHevcDec->nSeqDecode
           || pHevcDec->bIsFirstPicture == 1
           || nNumOutput > nSpsMaxNumReoderPics
           || (nMinPoc - pHevcDec->nLastDispPoc) == 1)
        {
           nNeedOutputFlag = 1;
        }

        if(nNeedOutputFlag == 0)
            return HEVC_RESULT_OK;

        pHevcDec->nLastDispPoc = nMinPoc;
        if(nNumOutput)
        {
            HevcFrame *pHf = &pHevcDec->DPB[nMinIdx];
            pHf->bFlags &= ~(HEVC_FRAME_FLAG_OUTPUT);
            /* todo: add pts contral */
            nPtsIdx = HevcFindAnAvailablePts(pHevcDec->PtsList);
            HevcPtsListInfoDebug(pHevcDec, nPtsIdx, 0, pHf->nPoc);
            if(nPtsIdx >= 0)
            {
                pHf->pFbmBuffer->nPts = pHevcDec->PtsList[nPtsIdx];

                pHf->pFbmBuffer->nCurFrameInfo.nVidFrmPTS = pHf->pFbmBuffer->nPts;
                pHevcDec->PtsList[nPtsIdx] = -1;
            }
            else
            {
                logd("can not find pts , pts list is empty");
                pHf->pFbmBuffer->nPts = 0;
            }
            logv(" last pts: %lld, current pts: %lld, diff: %lld",
                    pHevcDec->nCurrentFramePts, pHf->pFbmBuffer->nPts,
                    pHf->pFbmBuffer->nPts - pHevcDec->nCurrentFramePts);
            pHevcDec->nCurrentFramePts = pHf->pFbmBuffer->nPts;

#if HEVC_ZERO_PTS
            pHf->pFbmBuffer->nPts = 0;
#endif //HEVC_ZERO_PTS

            s32 bDisplayFlag = 1;
            if(pHf->bDropFlag == 1 || HEVC_CLOSE_DISPLAY == 1
               || (pHevcDec->bDisplayErrorFrameFlag == 0 && pHf->bErrorFrameFlag == 1))
            {
                bDisplayFlag = 0;
            }
            //* we should set the bFrameErrorFlag and send errorFrame
            //* when caller require display errorFrame,
            //* or, we should make sure every frame is right which send to caller.
            if(pHevcDec->bDisplayErrorFrameFlag == 1)
            {
                pHf->pFbmBuffer->bFrameErrorFlag = pHf->bErrorFrameFlag;
            }
            logv("*** errorFlag = %d, poc = %d",pHf->pFbmBuffer->bFrameErrorFlag, pHf->nPoc);

            if(pHevcDec->ControlInfo.secOutputEnabled)
            {
                pHf->pSecFbmBuffer->nPts = pHf->pFbmBuffer->nPts;
                pHf->pSecFbmBuffer->nCurFrameInfo.nVidFrmPTS = pHf->pFbmBuffer->nPts;
                if(bDisplayFlag == 1)
                    FbmReturnBuffer(pHevcDec->SecondFbm, pHf->pSecFbmBuffer, 1);
                else
                    FbmReturnBuffer(pHevcDec->SecondFbm, pHf->pSecFbmBuffer, 0);
                logv(" share second fbm. width: %d, height: %d, picture: %x, \
                        top: %d, bottom: %d, left: %d, right: %d, stride: %d",
                        pHf->pSecFbmBuffer->nWidth, pHf->pSecFbmBuffer->nHeight,
                        (u32)pHf->pSecFbmBuffer,
                        pHf->pSecFbmBuffer->nTopOffset, pHf->pSecFbmBuffer->nBottomOffset,
                        pHf->pSecFbmBuffer->nLeftOffset, pHf->pSecFbmBuffer->nRightOffset,
                        pHf->pSecFbmBuffer->nLineStride);
                pHf->pSecFbmBuffer = NULL;
            }
            else
            {
                logv(" share first fbm. width: %d, height: %d, picture: %x, \
                        top: %d, bottom: %d, left: %d, right: %d, stride: %d",
                        pHf->pFbmBuffer->nWidth, pHf->pFbmBuffer->nHeight,
                        (u32)pHf->pFbmBuffer,
                        pHf->pFbmBuffer->nTopOffset, pHf->pFbmBuffer->nBottomOffset,
                        pHf->pFbmBuffer->nLeftOffset, pHf->pFbmBuffer->nRightOffset,
                        pHf->pFbmBuffer->nLineStride);

                if(bDisplayFlag == 1)
                    FbmShareBuffer(pHevcDec->fbm, pHf->pFbmBuffer);
            }
            if(pHevcDec->bIsFirstPicture)
                pHevcDec->bIsFirstPicture = 0; /* clear first picture flag */

            if(pHf->bKeyFrame)
                nRet = VDECODE_RESULT_KEYFRAME_DECODED;
            else
                nRet = VDECODE_RESULT_FRAME_DECODED;
            logv(" share fbm. poc: %d, is keyframe: %d,  pts: %lld, fbm: %p",
                    pHf->nPoc, pHf->bKeyFrame, pHf->pFbmBuffer->nPts, pHf->pFbmBuffer);
            return nRet;
        }
        if(pHevcDec->nSeqOutput != pHevcDec->nSeqDecode)
            pHevcDec->nSeqOutput = (pHevcDec->nSeqOutput + 1) & 0xff;
        else
            break;
    }while(1);

    return HEVC_RESULT_OK;
}

