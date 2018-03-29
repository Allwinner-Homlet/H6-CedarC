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
* File : h265_drv.c
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#include "h265_drv.h"
#include "h265_memory.h"
#include "h265_var.h"
#include "h265_func.h"

static int  H265DecoderInit(DecoderInterface* pSelf,
                         VConfig* pConfig,
                         VideoStreamInfo* pVideoInfo,
                         VideoFbmInfo* pFbmInfo);
void        H265DecoderResetHw(DecoderInterface* pSelf);
static int  H265DecoderSetSbm(DecoderInterface* pSelf, SbmInterface* pSbm, int nIndex);
static int  H265DecoderGetFbmNum(DecoderInterface* pSelf);
static Fbm* H265DecoderGetFbm(DecoderInterface* pSelf, int nIndex);
static int  H265DecoderDecode(DecoderInterface* pSelf,
                            int               bEndOfStream,
                            int               bDecodeKeyFrameOnly,
                            int               bSkipBFrameIfDelay,
                            int64_t           nCurrentTimeUs);
static void Destroy(DecoderInterface* pSelf);

int H265DecoderSetExtraScaleInfo(DecoderInterface* pSelf,s32 nWidthTh, s32 nHeightTh,
                                 s32 nHorizonScaleRatio,s32 nVerticalScaleRatio);

int H265DecoderSetPerformCmd(DecoderInterface* pSelf,enum EVDECODERSETPERFORMCMD performCmd);
int H265DecodeGetPerformInfo(DecoderInterface* pSelf,enum EVDECODERGETPERFORMCMD performCmd,
                             VDecodePerformaceInfo** performInfo);
DecoderInterface* CreateH265Decoder(VideoEngine* p)
{
    H265DecCtx *pH265Ctx = NULL;

#if HEVC_ENABLE_MEMORY_LEAK_DEBUG
    HevcMeroryDebugInit();
#endif //HEVC_ENABLE_MEMORY_LEAK_DEBUG

    pH265Ctx = (H265DecCtx*)HevcMalloc(sizeof(H265DecCtx));
    if(pH265Ctx == NULL)
        return NULL;

    memset(pH265Ctx, 0, sizeof(H265DecCtx));

    pH265Ctx->pVideoEngine        = p;
    pH265Ctx->interface.Init      = H265DecoderInit;
    pH265Ctx->interface.Reset     = H265DecoderResetHw;
    pH265Ctx->interface.SetSbm    = H265DecoderSetSbm;
    pH265Ctx->interface.GetFbmNum = H265DecoderGetFbmNum;
    pH265Ctx->interface.GetFbm    = H265DecoderGetFbm;
    pH265Ctx->interface.Decode    = H265DecoderDecode;
    pH265Ctx->interface.Destroy   = Destroy;
    pH265Ctx->interface.SetExtraScaleInfo = H265DecoderSetExtraScaleInfo;
    pH265Ctx->interface.SetPerformCmd = H265DecoderSetPerformCmd;
    pH265Ctx->interface.GetPerformInfo = H265DecodeGetPerformInfo;
    logv(" h265hw CreateH265Decoder() ok......");
    return &pH265Ctx->interface;
}

void CedarPluginVDInit(void)
{
    int ret;
    ret = VDecoderRegister(VIDEO_CODEC_FORMAT_H265, "h265", CreateH265Decoder, 0);

    if (0 == ret)
    {
        logi("register h265 decoder success!");
    }
    else
    {
        loge("register h265 decoder failure!!!");
    }
    return ;
}

static int  H265DecoderInit(DecoderInterface* pSelf, VConfig* pConfig,
    VideoStreamInfo* pVideoInfo, VideoFbmInfo* pFbmInfo)
{
    H265DecCtx *pH265Ctx = (H265DecCtx *)pSelf;
    HevcContex *pHevcDec;
    s32 nRet = 0;
    s32 i = 0;
    if(pH265Ctx == NULL)
    {
        loge(" h265 decoder initial error. context pointer is null");
        return 0;
    }
    memcpy(&pH265Ctx->vconfig, pConfig, sizeof(VConfig));
    memcpy(&pH265Ctx->videoStreamInfo, pVideoInfo, sizeof(VideoStreamInfo));
    pH265Ctx->pFbmInfo = pFbmInfo;

    pHevcDec = (HevcContex *)HevcCalloc(1, sizeof(HevcContex));
    logv("pVideoInfo->eCodecFormat: %x", pVideoInfo->eCodecFormat);
    if(pHevcDec == NULL)
    {
        loge(" h265 decoder calloc faill, get a null pointer");
        return 0;
    }
    pHevcDec->pConfig = &pH265Ctx->vconfig;
    pHevcDec->pVideoStreamInfo = &pH265Ctx->videoStreamInfo;
    pHevcDec->bDisplayErrorFrameFlag = pConfig->bDispErrorFrame;
    logd("bDisplayErrorFrameFlag = %d", pHevcDec->bDisplayErrorFrameFlag);
    nRet = HevcDecInitContext(pHevcDec);
    if(nRet < 0)
    {
        loge(" h265 decoder initial error.....");
        pHevcDec->eDecodStep = H265_STEP_CRASH;
        return VDECODE_RESULT_UNSUPPORTED;
    }
    logd("*** specificData = %p, len = %d",
        pH265Ctx->videoStreamInfo.pCodecSpecificData,
        pH265Ctx->videoStreamInfo.nCodecSpecificDataLen);
    if(pH265Ctx->videoStreamInfo.pCodecSpecificData != NULL)
    {
        logv(" h265 decoder process pCodecSpecificData  ");
        nRet = HevcDecInitProcessSpecificData(pHevcDec,
                pH265Ctx->videoStreamInfo.pCodecSpecificData,
                pH265Ctx->videoStreamInfo.nCodecSpecificDataLen);
        if(nRet < 0)
        {
            loge(" h265 decoder process initial data error.....");
            pHevcDec->eDecodStep = H265_STEP_CRASH;
            return VDECODE_RESULT_UNSUPPORTED;
        }
    }
#if HEVC_SEARCH_NEXT_START_CODE_DISABLE
    pHevcDec->pStreamInfo->bSearchNextStartCode = 0; /* todo: just use as test */
#endif //HEVC_SEARCH_NEXT_START_CODE_DISABLE
    pHevcDec->eDecodStep = H265_STEP_REQUEST_STREAM;
    pHevcDec->bFrameDurationZeroFlag = (pH265Ctx->videoStreamInfo.nFrameDuration == 0);
    pHevcDec->nLastPts = -1;
    pHevcDec->nLastStreamPts = -1;
    pHevcDec->bFbmInitFlag = 0;

 #if HEVC_ENABLE_CATCH_DDR
    pHevcDec->nCurFrameHWTime = 0;
    pHevcDec->nLastFrameBW    = 0;
    pHevcDec->nCurFrameBW     = 0;
    pHevcDec->nCurFrameRef0   = 0;
    pHevcDec->nCurFrameRef1   = 0;
 #endif

    pH265Ctx->pHevcDec = pHevcDec;
    memset(&pHevcDec->H265PerformInf, 0, sizeof(HevcPerformInf));

    if(pVideoInfo->nFrameRate != 0)
    {
        if(pVideoInfo->nFrameRate>0 && pVideoInfo->nFrameRate<200)
        {
            pVideoInfo->nFrameRate = 1000*pVideoInfo->nFrameRate;
            pH265Ctx->videoStreamInfo.nFrameRate = pVideoInfo->nFrameRate;
        }
        pH265Ctx->videoStreamInfo.nFrameDuration =
            (1000*1000*1000)/pVideoInfo->nFrameRate;
        if(pH265Ctx->videoStreamInfo.nFrameDuration > 80000)
        {
            pH265Ctx->videoStreamInfo.nFrameDuration = 33000;
        }
        pHevcDec->H265PerformInf.H265PerfInfo.nFrameDuration =
            pH265Ctx->videoStreamInfo.nFrameDuration;
    }

    for(i = 0; i < HEVC_MAX_FRAMES_ONE_SQUEUE; i++)
    {
        pHevcDec->nSqueueFramePocList[i] = HEVC_POC_INIT_VALUE;
    }
    return VDECODE_RESULT_OK;
}

void H265DecoderResetHw(DecoderInterface* pSelf)
{
    H265DecCtx *pH265Ctx = (H265DecCtx *)pSelf;
    HevcContex *pHevcDec;
    logv(" calling H265DecoderResetHw() ...... ");

    if(pH265Ctx == NULL)
        return;
    pHevcDec = pH265Ctx->pHevcDec;
    if(pHevcDec == NULL)
        return;
    HevcResetDecoder(pHevcDec);
//    usleep(30*1000);
    pHevcDec->H265PerformInf.performCtrlFlag = 0;
    pHevcDec->H265PerformInf.H265PerfInfo.nDropFrameNum = 0;
}

static int H265DecoderSetSbm(DecoderInterface* pSelf, SbmInterface* pSbm, int nIndex)
{
    H265DecCtx *pH265Ctx = (H265DecCtx *)pSelf;
    HevcStreamInfo *pStreamInfo;
    size_addr pPhyAddr;
    struct ScMemOpsS *_memops = NULL;

    s32 nRet = 0;

    if(nIndex != 0)
    {
        return VDECODE_RESULT_UNSUPPORTED;
    }
    if(pH265Ctx == NULL ||
       pH265Ctx->pHevcDec == NULL ||
       pH265Ctx->pHevcDec->pStreamInfo == NULL )
    {
        loge(" h265 decoder set sbm error. context pointer is null");
        return VDECODE_RESULT_UNSUPPORTED;
    }
    _memops = pH265Ctx->vconfig.memops;
    pStreamInfo = pH265Ctx->pHevcDec->pStreamInfo;
    pStreamInfo->pSbm = pSbm;
    pStreamInfo->pSbmBuf = (char *)SbmBufferAddress(pSbm);
    pStreamInfo->nSbmBufSize = SbmBufferSize(pSbm);
    pStreamInfo->pSbmBufEnd = pStreamInfo->pSbmBuf + pStreamInfo->nSbmBufSize - 1;

    if(pH265Ctx->pHevcDec->pStreamInfo->pSbm->bUseNewVeMemoryProgram == 0)
    {
        pPhyAddr = (size_addr)CdcMemGetPhysicAddress(_memops, (void*)pStreamInfo->pSbmBuf);
        pStreamInfo->pSbmBufPhy = pPhyAddr;

        pPhyAddr = (size_addr)CdcMemGetPhysicAddress(_memops, (void*)pStreamInfo->pSbmBufEnd);
        pStreamInfo->pSbmBufEndPhy = pPhyAddr;
    }

    pStreamInfo->pLocalBuf = HevcAdapterPalloc(_memops, HEVC_LOCAL_SBM_BUF_SIZE,
                                                                (void *)pH265Ctx->vconfig.veOpsS,
                                                                pH265Ctx->vconfig.pVeOpsSelf);
    if(pStreamInfo->pLocalBuf == NULL)
    {
        nRet = HEVC_LOCAL_SBM_BUF_SIZE;
        loge(" H265DecoderSetSbm palloc error, size: %d ", nRet);
        return VDECODE_RESULT_UNSUPPORTED;
    }
    pStreamInfo->nLocalBufSize = HEVC_LOCAL_SBM_BUF_SIZE;
    pStreamInfo->pLocalBufEnd = pStreamInfo->pLocalBuf + pStreamInfo->nLocalBufSize - 1;
    pPhyAddr = (size_addr)CdcMemGetPhysicAddress(_memops,
        (void*)pStreamInfo->pLocalBuf);
    pStreamInfo->pLocalBufPhy = pPhyAddr;
    pPhyAddr = (size_addr)CdcMemGetPhysicAddress(_memops,
        (void*)pStreamInfo->pLocalBufEnd);
    pStreamInfo->pLocalBufEndPhy = pPhyAddr;

    return VDECODE_RESULT_OK;
}

static int H265DecoderGetFbmNum(DecoderInterface* pSelf)
{
    CEDARC_UNUSE(pSelf);
    return 1; /* h265 doesn't have 3D yet */
}

static Fbm* H265DecoderGetFbm(DecoderInterface* pSelf, int nIndex)
{
    H265DecCtx *pH265Ctx = (H265DecCtx *)pSelf;
    HevcContex *pHevcDec;

    if(nIndex != 0)
    {
        return NULL;
    }
    if(pH265Ctx == NULL)
        return NULL;
    pHevcDec = pH265Ctx->pHevcDec;
    if(pHevcDec != NULL)
    {
        if(pHevcDec->ControlInfo.secOutputEnabled)
        {
            logv(" get second fbm ok, index: %d ", nIndex);
            return pHevcDec->SecondFbm;
        }
        else
            return pHevcDec->fbm;
    }
    else
    {
        logw(" h265 decoder get fbm error. Maybe decoder hadn't initialized");
    }
    return NULL;
}

int H265DecoderSetExtraScaleInfo(DecoderInterface* pSelf,s32 nWidthTh, s32 nHeightTh,
                                 s32 nHorizonScaleRatio,s32 nVerticalScaleRatio)
{
    H265DecCtx *pH265Ctx = (H265DecCtx *)pSelf;
    HevcContex *pHevcDec;

    pHevcDec = pH265Ctx->pHevcDec;

    pHevcDec->H265ExtraScaleInfo.nWidthTh = nWidthTh;
    pHevcDec->H265ExtraScaleInfo.nHeightTh = nHeightTh;
    pHevcDec->H265ExtraScaleInfo.nHorizonScaleRatio = nHorizonScaleRatio;
    pHevcDec->H265ExtraScaleInfo.nVerticalScaleRatio = nVerticalScaleRatio;
    return 0;
}

int H265DecoderSetPerformCmd(DecoderInterface* pSelf,enum EVDECODERSETPERFORMCMD performCmd)
{
    H265DecCtx *pH265Ctx = (H265DecCtx *)pSelf;
    HevcContex *pHevcDec = NULL;
    int ret = 0;

    pHevcDec = pH265Ctx->pHevcDec;

    switch(performCmd)
    {
        case VDECODE_SETCMD_DEFAULT:
        {
            ret = -1;
            break;
        }
        case VDECODE_SETCMD_START_CALDROPFRAME:
        {
            pHevcDec->H265PerformInf.performCtrlFlag |= H265_PERFORM_CALDROPFRAME;
            pHevcDec->H265PerformInf.H265PerfInfo.nDropFrameNum = 0;
            ret = 0;
            break;
        }
        case VDECODE_SETCMD_STOP_CALDROPFRAME:
        {
            pHevcDec->H265PerformInf.performCtrlFlag &= ~H265_PERFORM_CALDROPFRAME;
            pHevcDec->H265PerformInf.H265PerfInfo.nDropFrameNum = 0;
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

int H265DecodeGetPerformInfo(DecoderInterface* pSelf,enum EVDECODERGETPERFORMCMD performCmd,
                             VDecodePerformaceInfo** performInfo)
{
    H265DecCtx *pH265Ctx = (H265DecCtx *)pSelf;
    HevcContex *pHevcDec = NULL;
    int ret = 0;

    pHevcDec = pH265Ctx->pHevcDec;

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
            *performInfo  = &(pHevcDec->H265PerformInf.H265PerfInfo);
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

static int updateProcInfo(HevcContex *pHevcDec)
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

    mVeProcDecInfo.nChannelNum = pHevcDec->pConfig->nChannelNum;

    if(pHevcDec->b10BitStreamFlag == 1)
        sprintf(mVeProcDecInfo.nProfile, "%s", "main10");
    else
        sprintf(mVeProcDecInfo.nProfile, "%s", "main");

    mVeProcDecInfo.nWidth  = pHevcDec->pSps->nWidth;
    mVeProcDecInfo.nHeight = pHevcDec->pSps->nHeight;

    nProcInfoLen += sprintf(tmpBuf + nProcInfoLen,
            "***************channel[%d]: h265 dec info**********************\n",
            mVeProcDecInfo.nChannelNum);

    nProcInfoLen += sprintf(tmpBuf + nProcInfoLen,
            "profile: %s, widht: %d, height: %d\n",
            mVeProcDecInfo.nProfile, mVeProcDecInfo.nWidth, mVeProcDecInfo.nHeight);

    logv("proc info: len = %d, %s",nProcInfoLen, tmpBuf);
    CdcVeSetProcInfo(pHevcDec->pConfig->veOpsS,
                     pHevcDec->pConfig->pVeOpsSelf,
                     tmpBuf, nProcInfoLen, mVeProcDecInfo.nChannelNum);


    free(tmpBuf);
    return 0;
}

/****************************************************************************/
//********************************************** ****************************/

static int H265DecoderDecode(DecoderInterface* pSelf,
                            int               bEndOfStream,
                            int               bDecodeKeyFrameOnly,
                            int               bSkipBFrameIfDelay,
                            int64_t           nCurrentTimeUs)
{
    H265DecCtx *pH265Ctx = (H265DecCtx *)pSelf;
    HevcContex *pHevcDec;
    s32 nRet;
    s32 nGetPictre = 0;
    s32 nOutPutRet = 0;
    s32 nValidSize = 0;

#if HEVC_SLOW_DECODER
    usleep(200*1000);
    logd("   ");
    logd("  --------------  a new frame start --------------- ");
#endif
    if(pH265Ctx == NULL)
        return VDECODE_RESULT_UNSUPPORTED;
    pHevcDec = pH265Ctx->pHevcDec;
    if(pHevcDec == NULL)
    {
        loge(" h265 decoder decoding error. Maybe decoder hadn't initial.... ");
        return VDECODE_RESULT_UNSUPPORTED;
    }
    pHevcDec->pFbmInfo = pH265Ctx->pFbmInfo;
    if(pHevcDec->eDecodStep == H265_STEP_CRASH)
    {
        usleep(2*1000);
        return VDECODE_RESULT_UNSUPPORTED;
    }

    if(pHevcDec->bFbmInitFlag == 1)
    {
        nOutPutRet = HevcOutputFrame(pHevcDec, 0);
        if(nOutPutRet != 0)
        {
            logv("get one extra picture to show..............  ");
            return VDECODE_RESULT_OK;
        }
    }

    if(pHevcDec->eDecodStep == H265_STEP_REQUEST_STREAM)
    {
        pHevcDec->nCurFrameStreamSize = 0;
        if(pHevcDec->pStreamInfo->pSbm->nType == SBM_TYPE_FRAME)
        {
            nRet = HevcDecRequestOneFrameStreamNew(pHevcDec);
        }
        else
        {
            if(pHevcDec->pStreamInfo->bStreamWithStartCode == -1)
            {
                nRet = HevcCheckBitStreamType(pHevcDec);
                if(nRet != 0)
                    return nRet;
            }
            nRet = HevcDecRequestOneFrameStream2(pHevcDec);
        }

        if(nRet == VDECODE_RESULT_NO_BITSTREAM)
        {
            if(bEndOfStream == 0)
                return VDECODE_RESULT_NO_BITSTREAM;
            else if(bEndOfStream && pHevcDec->pStreamInfo->nCurrentDataBufIdx == 0)
            {
                nValidSize = SbmStreamDataSize(pHevcDec->pStreamInfo->pSbm);
                if(nValidSize == 0)
                {
                    logd(" no bit stream and bEndOfStream == 1, \
                        h265 decoder should output all pictures ");
                    HevcDecOutputAllPictures(pHevcDec);
                    return VDECODE_RESULT_NO_BITSTREAM;
                }
                else
                    return VDECODE_RESULT_CONTINUE;
            }

        }
        if(nRet == VDECODE_RESULT_UNSUPPORTED)
        {
            loge(" h265 decoder cannot get a whole frame data, maybe something wrong... ");
            pHevcDec->eDecodStep = H265_STEP_CRASH;
            return VDECODE_RESULT_UNSUPPORTED;
        }
        pHevcDec->eDecodStep = H265_STEP_DECODE_FRAME;
        HevcDecRequestOneFrameStreamDebug(pHevcDec);
    }

    if(pHevcDec->eDecodStep == H265_STEP_DECODE_FRAME)
    {
//        logd(" enter H265_STEP_PARSE_DATA,  next will be H265_STEP_PROCESS_REGISTER");
        /* todo: if fbm is not initial,
           we should initial fbm right after we get the width and height.
          And request fbm buffe.
          */
        CdcVeReset(pH265Ctx->vconfig.veOpsS, pH265Ctx->vconfig.pVeOpsSelf);
        nRet = HevcDecDecodeOneFrame(pHevcDec, bDecodeKeyFrameOnly);
        if(nRet == HEVC_RESULT_PICTURES_SIZE_CHANGE)
        {
            s32 nTryTime = 200;
            logd(" Picture size change.. ");
            HevcDecReturnAllSbmStreamsUnflush(pHevcDec);
            nRet = 0;
            do{
                nRet = HevcOutputFrame(pHevcDec, 1);
                usleep(500);
                nTryTime--;
            }while(nRet != 0 && nTryTime > 0);
            return VDECODE_RESULT_RESOLUTION_CHANGE;
        }
        else if(nRet == HEVC_RESULT_RESET_INTRA_DECODER)
        {
            logd(" reset h265 intra decoder ");
            HevcResetDecoder(pHevcDec);
            pHevcDec->H265PerformInf.performCtrlFlag = 0;
            pHevcDec->H265PerformInf.H265PerfInfo.nDropFrameNum = 0;
            pHevcDec->eDecodStep = H265_STEP_REQUEST_STREAM;
            return VDECODE_RESULT_OK;
        }
        else if(nRet == HEVC_RESULT_DROP_THE_FRAME)
        {
            HevcDecReturnAllSbmStreams(pHevcDec);
            pHevcDec->bDropPreFrameFlag = 1;
            pHevcDec->eDecodStep = H265_STEP_REQUEST_STREAM;
            return VDECODE_RESULT_OK;
        }
        else if(nRet == HEVC_RESULT_NO_FRAME_BUFFER)
        {
            return VDECODE_RESULT_NO_FRAME_BUFFER;
        }
        else if(nRet == HEVC_RESULT_INIT_FBM_FAILED || nRet < 0)
        {
            loge(" h265 decode frame error, return unsupport, waiting for exit , ret = %d",nRet);
            HevcDecReturnAllSbmStreams(pHevcDec);
            pHevcDec->eDecodStep = H265_STEP_CRASH;
            return VDECODE_RESULT_UNSUPPORTED;
        }
        else
        {
            if(nRet != HEVC_RESULT_OK && nRet != HEVC_RESULT_NULA_DATA_ERROR)
            {
                logw("call HevcDecDecodeOneFrame: the nRet(%d) is not defined",nRet);
            }
            //* it means had decoded one frame when pCurrDPB is not null
            HevcFrame *pHf = pHevcDec->pCurrDPB;
            if(pHf != NULL && pHf->bDropFlag == 0)
            {
                if(pHf->bKeyFrame)
                    nGetPictre = VDECODE_RESULT_KEYFRAME_DECODED;
                else
                    nGetPictre = VDECODE_RESULT_FRAME_DECODED;
            }
            pHevcDec->eDecodStep = H265_STEP_PROCESS_RESULT;
        }
    }

    if(pHevcDec->eDecodStep == H265_STEP_PROCESS_RESULT)
    {
        pHevcDec->nDecPicNum++;
        if(pHevcDec->pConfig->bSetProcInfoEnable == 1
           && pHevcDec->nDecPicNum%pHevcDec->pConfig->nSetProcInfoFreq == 0)
        {
            updateProcInfo(pHevcDec);
        }
        /* wait for hardware interrupt */
//        logd(" enter H265_STEP_PROCESS_RESULT,  next will be H265_STEP_REQUEST_STREAM");
        HevcDecReturnAllSbmStreams(pHevcDec);
        pHevcDec->eDecodStep = H265_STEP_REQUEST_STREAM;

    #if HEVC_ENABLE_CATCH_DDR
        if(pHevcDec->pCurrDPB)
        {
            logd("the_113,nDecPicNum=%d,nCurFrameHWTime=%d,nCurFrameBW=%d,  \
                nCurFrameRef0=%d,nCurFrameRef1=%d\n",  \
                pHevcDec->nDecPicNum,pHevcDec->nCurFrameHWTime,pHevcDec->nCurFrameBW,  \
                pHevcDec->nCurFrameRef0,pHevcDec->nCurFrameRef1);
            HevcFrame *pHf = pHevcDec->pCurrDPB;
            pHevcDec->nPara[pHevcDec->nDecPicNum][0] = pHevcDec->nCurFrameHWTime;
            pHevcDec->nPara[pHevcDec->nDecPicNum][1] = pHevcDec->nCurFrameBW;
            pHevcDec->nPara[pHevcDec->nDecPicNum][2] = pHevcDec->nCurFrameRef0;
            pHevcDec->nPara[pHevcDec->nDecPicNum][3] = pHevcDec->nCurFrameRef1;
            pHevcDec->nPara[pHevcDec->nDecPicNum][4] = pHf->pFbmBuffer->nCurFrameInfo.nVidFrmSize;
        }
        pHevcDec->nCurFrameHWTime = 0;
        pHevcDec->nCurFrameBW     = 0;
        pHevcDec->nCurFrameRef0   = 0;
        pHevcDec->nCurFrameRef1   = 0;
    #endif

        HevcOutputFrame(pHevcDec, 0);
        int64_t nRange = 100*1000; // 100 ms
        if(pHevcDec->nCurrentFramePts != 0 &&
           //pHevcDec->nCurrentFramePts < (nCurrentTimeUs - nRange) &&
           pHevcDec->nCurrentFramePts < (nCurrentTimeUs+20*1000) &&
           bSkipBFrameIfDelay)
            pHevcDec->bDropFrameAdaptiveEnable = 1;
        else
            pHevcDec->bDropFrameAdaptiveEnable = 0;

#if !HEVC_ADAPTIVE_DROP_FRAME_ENABLE
        pHevcDec->bDropFrameAdaptiveEnable = 0;
#endif

#if HEVC_DROP_FRAME_DEBUG
        pHevcDec->bDropFrameAdaptiveEnable = 1;
#endif //HEVC_DROP_FRAME_DEBUG

#if HEVC_DECODE_TIME_INFO
        if(0 && pHevcDec->pCurrDPB)
    {
        s64 nTime, nTimeDiff;
        s32 nDeltaNum = HEVC_FRAME_DURATION - 1;
        pHevcDec->debug.nFrameNum += 1;
        if((pHevcDec->debug.nFrameNum & nDeltaNum) == 0)
        {
            float f, avg;
            nTime = HevcGetCurrentTime();
            pHevcDec->debug.nFrameDualation = nTime - pHevcDec->debug.nCurrTime;
            pHevcDec->debug.nCurrTime = nTime;
            f = (float)(pHevcDec->debug.nFrameDualation/1000.0);

            nTimeDiff = nTime - pHevcDec->debug.nStartTime;
            avg = (float)(nTimeDiff / 1000.0);
            avg = avg / 1000.0;
            f = f / 1000.0;
            loge(" hevc decoder. decode frame: %d, current speed: \
                %.2f fps, average speed: %.2f fps",
                pHevcDec->debug.nFrameNum, (nDeltaNum + 1)/f,
                ((float)pHevcDec->debug.nFrameNum)/avg);
            logd(" current pts: %lld, current time: %lld ", \
                pHevcDec->nCurrentFramePts, nCurrentTimeUs);
            pHevcDec->debug.nFrameDualation = 0;
            if(pHevcDec->pCurrDPB != NULL)
            {
                s32 nDecW, nDecH;
                nDecW = pHevcDec->pSps->nWidth;
                nDecH = pHevcDec->pSps->nHeight;
                if(pHevcDec->ControlInfo.secOutputEnabled)
                {
                    s32 w, h;
                    w = nDecW >> pHevcDec->ControlInfo.nHorizonScaleDownRatio;
                    h = nDecH >> pHevcDec->ControlInfo.nVerticalScaleDownRatio;
                    logd(" hevc current display picture size: %dx%d, \
                        decoded picture size: %dx%d, scale down enable",
                            w, h, nDecW, nDecH);
                }
                else
                {
                    logd(" hevc current display picture size: %dx%d,  \
                        decoded picture size: %dx%d",
                            nDecW, nDecH, nDecW, nDecH);
                }
            }
        }
    }
#endif    //HEVC_DECODE_TIME_INFO

    }
    return nGetPictre;
}

static u32 nDestory = 0;
static void Destroy(DecoderInterface* pSelf)
{
    H265DecCtx* pH265Ctx = (H265DecCtx*)pSelf;
    if(pH265Ctx == NULL)
        return ;

    #if HEVC_ENABLE_CATCH_DDR
        u32 i,j;
        char name[128];
        double BW,BW_temp;
        nDestory++;
        HevcContex *pHevcDec = pH265Ctx->pHevcDec;
        if(pHevcDec == NULL)
        {
            loge(" h265 decoder decoding error. Maybe decoder hadn't initial.... ");
            return ;
        }
        sprintf(name, "/data/camera/h265_ka_%d.txt", nDestory);
        FILE *fp = fopen(name, "wt");
        if(fp != NULL)
        {
            logw(" saving frame number: %d ", pHevcDec->nDecPicNum);
            fprintf(fp,"nDecPicNum,nCurFrameHWTime,nCurFrameBW(KB),nCurFrameBW(GB),  \
                nCurFrameRef0,nCurFrameRef1,nCurFrameDataSize");
            fprintf(fp,"\n");
            for(i = 0; i < pHevcDec->nDecPicNum; i++)
            {
                fprintf(fp,"%8u,", i);
                for(j=0;j<5;j++)
                {
                   if(j==0)
                   {
                      fprintf(fp,"%8u,", pHevcDec->nPara[i][j]);
                   }
                   else if(j==1)
                   {
                      fprintf(fp,"%8u,", pHevcDec->nPara[i][j]);
                      BW_temp = (double)(pHevcDec->nPara[i][j]);
                      BW = BW_temp/((double)1024.0)*((double)60.0)/((double)1024.0);
                      fprintf(fp,"%.2f,", BW);
                   }
                   else if(j==2 || j==3 || j==4)
                   {
                      fprintf(fp,"%8u,", pHevcDec->nPara[i][j]);
                   }
                }
                fprintf(fp,"\n");
            }
            fclose(fp);
        }
        else
        {
             loge("save frame number open file error");
        }
    #endif
    logv(" decoder destroy start ............. ");
    HevcDecDestroy(pH265Ctx->pHevcDec);
    HevcFree(&pH265Ctx->pHevcDec);
    HevcFree(&pH265Ctx);

#if HEVC_ENABLE_MEMORY_LEAK_DEBUG
    HevcMeroryLeakDebugInfo();
    HevcMeroryDebugClose();
#endif //HEVC_ENABLE_MEMORY_LEAK_DEBUG

    logv(" h265hw calling Destroy() ......");
}

