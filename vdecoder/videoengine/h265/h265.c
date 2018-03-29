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
* File : h265.c
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#include "h265_config.h"
#include "h265_func.h"
#include "h265_memory.h"
#include "h265_var.h"
#include <math.h>
#include "h265_sha.h"
#define HEVC_FREE_ZERO_POINTER(pointer)  \
    if(pointer != NULL)                    \
        HevcFree(&pointer);                \

#define IS_HEVC_START_CODE(p) ((p[0]==0) && (p[1] == 0) && (p[2] == 1))

#define HEVC_NOT_ANNEX_B_EXTRA_DATA     \
                                        \
        case 0x200001:                    \
        case 0x210001:                    \
        case 0x220001:                    \
                                        \
        case 0x600001:                    \
        case 0x610001:                    \
        case 0x620001:                    \
                                        \
        case 0xa00001:                    \
        case 0xa10001:                    \
        case 0xa20001:                    \
                                        \
        case 0xe00001:                    \
        case 0xe10001:                    \
        case 0xe20001:                    \

#if 0
static inline char ReadSbmByte(char **p, char *pStart, char *pEnd)
{
    char c = **p;
    if((*p) == pEnd)
        (*p) = pStart;
    else
        (*p) += 1;
    return c;
}
#endif

static inline char ReadSbmByteIdx(char *p, char *pStart, char *pEnd, s32 i)
{
    char c = 0x0;
    if((p+i) <= pEnd)
        c = p[i];
    else
    {
        s32 d = (s32)(pEnd - p) + 1;
        c = pStart[i - d];
    }
    return c;
}

static inline void SbmPtrPlusOne(char **p, char *pStart, char *pEnd)
{
    if((*p) == pEnd)
        (*p) = pStart;
    else
        (*p) += 1;
}

static s32 HevcInitRegisterSettingInfo(HevcContex *pHevcDec)
{
    HevcControlInfo *pCi = &pHevcDec->ControlInfo;
    HevcSliceHeader *pSh = &pHevcDec->SliceHeader;
    struct ScMemOpsS *_memops = pHevcDec->pConfig->memops;
    pHevcDec->HevcDecRegBaseAddr = (size_addr)CdcVeGetGroupRegAddr(pHevcDec->pConfig->veOpsS,
                                                                   pHevcDec->pConfig->pVeOpsSelf,
                                                                   REG_GROUP_H265_DECODER);
    pHevcDec->HevcTopRegBaseAddr = (size_addr)CdcVeGetGroupRegAddr(pHevcDec->pConfig->veOpsS,
                                                                   pHevcDec->pConfig->pVeOpsSelf,
                                                                   REG_GROUP_VETOP);

    logv(" h265 decoder int. get HEVC addr: %X, top addr: %X, VE version: %llX",
            pHevcDec->HevcDecRegBaseAddr, pHevcDec->HevcTopRegBaseAddr, pHevcDec->nIcVersion);
    /*
    hardware picture output format
    000: 32x32 tile-based
    001: 128x32 tile-base (the buffer should be 4K aligned)
    010: YU12 planner format (i420)
    011: YV12 planner format
    100: NV12 planner format
    101: NV21 planner format
     */
    pHevcDec->eDisplayPixelFormat = pHevcDec->pConfig->eOutputPixelFormat;
#if HEVC_ENABLE_MD5_CHECK
    pHevcDec->eDisplayPixelFormat = PIXEL_FORMAT_YV12;
#endif
    switch(pHevcDec->eDisplayPixelFormat)
    {
    case PIXEL_FORMAT_YV12:
        pCi->priPixelFormatReg = 3;
        break;
    case PIXEL_FORMAT_NV12:
        pCi->priPixelFormatReg = 4;
        break;
    case PIXEL_FORMAT_NV21:
        pCi->priPixelFormatReg = 5;
        break;
    default:
        pCi->priPixelFormatReg = 3;
    }

    pCi->pNeighbourInfoBuffer = HevcAdapterPalloc(_memops, HEVC_NEIGHBOUR_INFO_BUF_SIZE,
                                                                (void *)pHevcDec->pConfig->veOpsS,
                                                                pHevcDec->pConfig->pVeOpsSelf);
    if(pCi->pNeighbourInfoBuffer == NULL)
    {
        loge(" h265 decoder initial palloc error. neighbour info ");
        return -1;
    }
    pCi->pNeighbourInfoBufferPhyAddr = (size_addr)CdcMemGetPhysicAddress(_memops,
        (void*)pCi->pNeighbourInfoBuffer);

    pSh->pRegEntryPointOffset =
        HevcAdapterPalloc(_memops, HEVC_ENTRY_POINT_OFFSET_BUFFER_SIZE,
                                                                (void *)pHevcDec->pConfig->veOpsS,
                                                                pHevcDec->pConfig->pVeOpsSelf);
    if(pSh->pRegEntryPointOffset == NULL)
    {
        loge(" h265 decoder initial palloc error. entry point offset info ");
        HevcAdapterFree(_memops, &pCi->pNeighbourInfoBuffer,
                                                                pHevcDec->pConfig->veOpsS,
                                                                pHevcDec->pConfig->pVeOpsSelf);
        return -1;
    }
    pSh->pRegEntryPointOffsetPhyAddr = (size_addr)CdcMemGetPhysicAddress(_memops,
        (void*)pSh->pRegEntryPointOffset);
    return 0;
}

s32 HevcDecInitContext(HevcContex *pHevcDec)
{
    HevcControlInfo *pCi = &pHevcDec->ControlInfo;
    s32 i, nRet;
    pHevcDec->pStreamInfo = HevcCalloc(1, sizeof(HevcStreamInfo));
    if(pHevcDec->pStreamInfo == NULL)
    {
        loge(" h265 decoder initial. calloc error ");
        return -1;
    }
    pHevcDec->bIsFirstPicture = 1;
    pHevcDec->pStreamInfo->bFindKeyFrameOnly = 1;
    /* The first frame should be a keyframe */
    pHevcDec->pStreamInfo->bStreamWithStartCode = -1;
    /* nalu's first 4 bytes is start code 0x001 */
    pHevcDec->pStreamInfo->bSearchNextStartCode = 1;
    /* used in more than one start code */
    pHevcDec->b10BitStreamFlag = -1;
    pHevcDec->bPreHdrFlag = -1;

    /*in a data trunck case, ts container */
    for(i = 0; i < HEVC_PTS_LIST_SIZE; i++)
        pHevcDec->PtsList[i] = -1;

    for(i = 0; i < HEVC_MAX_DPB_SIZE; i++)
        pHevcDec->DPB[i].nIndex = i;
    pHevcDec->nMaxRa = HEVC_INT_MAX;

    HevcSliceHeaderDefaultValue(&pHevcDec->SliceHeader);
    nRet = HevcInitRegisterSettingInfo(pHevcDec);
#if HEVC_SAVE_EACH_FRAME_TIME
    pHevcDec->debug.pAllTimeHw = calloc(1, 4*200*1024);
#endif //HEVC_SAVE_EACH_FRAME_TIME
    if(nRet < 0)
        return nRet;

    pCi->secOutputEnabled = pHevcDec->pConfig->bScaleDownEn ? 1 : 0;
    /* Rotation doesn't support */
    pCi->bScaleDownEnable = pHevcDec->pConfig->bScaleDownEn;

#if HEVC_SCALE_DOWN_DEBUG
    pCi->secOutputEnabled = 1;
    pCi->bScaleDownEnable = 1;
    pHevcDec->pConfig->nHorizonScaleDownRatio = 1;
    pHevcDec->pConfig->nVerticalScaleDownRatio = 1;
#endif //HEVC_SCALE_DOWN_ENABLE

#if HEVC_DISABLE_SCALE_DOWN
    pCi->secOutputEnabled = 0;
#endif //HEVC_DISABLE_SCALE_DOWN

    logd("pHevcDec->pConfig->bScaleDownEn = %d", pHevcDec->pConfig->bScaleDownEn);
    if(pCi->secOutputEnabled)
    {
        pCi->secPixelFormatReg = pCi->priPixelFormatReg;
        pCi->nHorizonScaleDownRatio = pHevcDec->pConfig->nHorizonScaleDownRatio;
        //todo:
        pCi->nVerticalScaleDownRatio = pHevcDec->pConfig->nVerticalScaleDownRatio;
        pCi->bRotationEnable = /*pHevcDec->pConfig->bRotationEn*/0;
        pCi->nRotationDegree = /*pHevcDec->pConfig->nRotateDegree*/0;
    }
    else
    {
        pCi->secPixelFormatReg = pCi->priPixelFormatReg;
        pCi->nHorizonScaleDownRatio = 0;    //todo:
        pCi->nVerticalScaleDownRatio = 0;
        pCi->bRotationEnable = 0;
        pCi->nRotationDegree = 0;
    }

    pHevcDec->nIcVersion =  CdcVeGetIcVeVersion(pHevcDec->pConfig->veOpsS,
                                                pHevcDec->pConfig->pVeOpsSelf);

    pHevcDec->nDecIpVersion = pHevcDec->nIcVersion >> 32;
    logd(" get the nIcversion = %llx, nDecIpVersion = %llx",
         pHevcDec->nIcVersion, pHevcDec->nDecIpVersion);

    return 0;
}

s32 HevcDecReturnAllSbmStreams(HevcContex *pHevcDec)
{
    HevcStreamInfo *pStreamInfo;
    pStreamInfo = pHevcDec->pStreamInfo;

    if(pStreamInfo->pSbm == NULL)
        return -1;

    if(pStreamInfo->pSbm->nType == SBM_TYPE_FRAME)
    {
        if(pHevcDec->pStreamInfo->pCurStreamFrame)
        {
            SbmFlushStream(pHevcDec->pStreamInfo->pSbm,
                           (VideoStreamDataInfo*)pHevcDec->pStreamInfo->pCurStreamFrame);
            pHevcDec->pStreamInfo->pCurStreamFrame = NULL;
        }
    }
    else
    {
        int i;
        for(i = 0; i < pStreamInfo->nCurrentStreamIdx; i++)
        {
            if(pStreamInfo->pStreamList[i] != NULL)
            {
                logv(" return sbm stream: 0x%x", (u32)pStreamInfo->pStreamList[i]);
                SbmFlushStream(pStreamInfo->pSbm, pStreamInfo->pStreamList[i]);
                pStreamInfo->pStreamList[i] = NULL;
            }
        }
        for(i = 0; i < pStreamInfo->nCurrentDataBufIdx; i++)
        {
            pStreamInfo->pDataBuf[i] = NULL;
            pStreamInfo->nDataSize[i] = 0;
        }
        pStreamInfo->nCurrentStreamIdx = 0;
        pStreamInfo->nCurrentDataBufIdx = 0;
    }
    return 0;
}

s32 HevcDecReturnAllSbmStreamsUnflush(HevcContex *pHevcDec)
{
    HevcStreamInfo *pStreamInfo;
    pStreamInfo = pHevcDec->pStreamInfo;

    if(pStreamInfo->pSbm->nType == SBM_TYPE_FRAME)
    {
        if(pHevcDec->pStreamInfo->pCurStreamFrame)
        {
            SbmReturnStream(pHevcDec->pStreamInfo->pSbm,
                           (VideoStreamDataInfo*)pHevcDec->pStreamInfo->pCurStreamFrame);
            pHevcDec->pStreamInfo->pCurStreamFrame = NULL;

        }
    }
    else
    {
        if(pStreamInfo->pStreamTemp)
        {
            SbmReturnStream(pStreamInfo->pSbm, pStreamInfo->pStreamTemp);
            pStreamInfo->pStreamTemp = NULL;
        }

        int i;
        for(i = pStreamInfo->nCurrentStreamIdx - 1; i >= 0; i--)
        {
            if(pStreamInfo->pStreamList[i] != NULL)
            {
                logv(" return sbm stream: 0x%x, i: %d, total: %d",
                        (u32)pStreamInfo->pStreamList[i], i, pStreamInfo->nCurrentStreamIdx);
                SbmReturnStream(pStreamInfo->pSbm, pStreamInfo->pStreamList[i]);
                pStreamInfo->pStreamList[i] = NULL;
            }
        }
        for(i = 0; i < pStreamInfo->nCurrentDataBufIdx; i++)
        {
            pStreamInfo->pDataBuf[i] = NULL;
            pStreamInfo->nDataSize[i] = 0;
        }
        pStreamInfo->nCurrentStreamIdx = 0;
        pStreamInfo->nCurrentDataBufIdx = 0;
    }
    return 0;
}

static s32 HevcCaculateFrameDuration(HevcContex *pHevcDec, int64_t nCurrentPts)
{
    VideoStreamDataInfo *pStream = NULL;
    HevcStreamInfo *pStreamInfo = pHevcDec->pStreamInfo;
    s32 nCount = 200;
    s32 nFrameNum, bTwoDataTrunk, i, bFirstSliceSegment;
    char *pData;
    HevcNaluType eNaluType;
    int64_t nDeltaPts = 0;
    do{
        pStream = SbmRequestStream(pStreamInfo->pSbm);
        if(pStream != NULL)
        {
            if(pStream->nLength == 0 || pStream->pData == NULL)
                SbmFlushStream(pStreamInfo->pSbm, pStream);
            else
                break;
        }
        usleep(1000);
        nCount--;
    }while(nCount > 0);
    if(nCount <= 0 || (pStream && pStream->nPts <= 0))
    {
        if(pStream)
            SbmReturnStream(pStreamInfo->pSbm, pStream);
        return 0;
    }
    pData = pStream->pData;
    /* step 1: get nDeltaPts */
    nDeltaPts = pStream->nPts - nCurrentPts;
    /* step 2: get frame num */
    /*   step 2.1: get data */
    bTwoDataTrunk = (pStream->pData + pStream->nLength > pStreamInfo->pSbmBufEnd);
    if(bTwoDataTrunk)
    {
        s32 nSize = (pStream->nLength + 7) & ~7;
        s32 nSize0 = (s32)(pStreamInfo->pSbmBufEnd - pStream->pData) + 1;
        pData = HevcCalloc(nSize, 1);
        if(pData == NULL)
        {
            loge(" h265 decoder calloc memory fail. caculate frame duration ");
            SbmReturnStream(pStreamInfo->pSbm, pStream);
            return 0;
        }
        memcpy(pData, pStream->pData, nSize0);
        memcpy(pData + nSize0, pStreamInfo->pSbmBuf, pStream->nLength - nSize0);
    }
    /*   step 2.2: search each nalu and get frame number */
    nFrameNum = 0;
    for(i = 0; i < pStream->nLength; i++)
    {
        if(pData[i] == 0 && pData[i + 1] == 0 && pData[i + 2] == 1)
        {
            /* */
            eNaluType = (pData[i + 2 + 1] & 0x7e) >> 1;
            if(eNaluType <= HEVC_NAL_CRA_NUT)
            {
                bFirstSliceSegment = (pData[i + 2 + 3] >> 7);
                if(bFirstSliceSegment)
                    nFrameNum += 1;
            }
        }
    }
    if(nFrameNum == 0 && pStream->nPts != -1)
    {
        loge(" maybe error, get a valid pts, \
            but there is no frame data in the current stream data ");
        nFrameNum = 1;
    }
    pHevcDec->pVideoStreamInfo->nFrameDuration = nDeltaPts / nFrameNum;
    if(pHevcDec->pVideoStreamInfo->nFrameDuration > 50000)
    {
        pHevcDec->pVideoStreamInfo->nFrameDuration = 33000;
    }
    logv(" HevcCaculateFrameDuration. get frame number: %d, nFrameDuration: %d ",
            nFrameNum, pHevcDec->pVideoStreamInfo->nFrameDuration);
    pHevcDec->H265PerformInf.H265PerfInfo.nFrameDuration =
        pHevcDec->pVideoStreamInfo->nFrameDuration;
    SbmReturnStream(pStreamInfo->pSbm, pStream);
    if(bTwoDataTrunk)
        HevcFree(&pData);
    return 1;
}

static s32 DecoderRequestOneSbmStream2(HevcContex *pHevcDec)
{
    VideoStreamDataInfo *pStream = NULL;
    HevcStreamInfo *pStreamInfo = pHevcDec->pStreamInfo;
    s32 nCounter = 50;
//    logd(" DecoderRequestOneSbmStream2() start...... ");
    while(nCounter > 0)
    {
        pStream = SbmRequestStream(pStreamInfo->pSbm);
        if(pStream == NULL)
        {
            return VDECODE_RESULT_NO_BITSTREAM;
        }
        if(pStream->nLength == 0 || pStream->pData == NULL)
        {
            logw("invalid stream data: pData[%p], len[%d], ListIndex[%d]",
                pStream->pData, pStream->nLength,pStreamInfo->nCurrentStreamIdx);
            if(pStreamInfo->nCurrentStreamIdx == 0)
            {
                SbmFlushStream(pStreamInfo->pSbm, pStream);
            }
            else
            {
                //*cannot flush immediately when streamList is not empty
                pStreamInfo->pStreamList[pStreamInfo->nCurrentStreamIdx] = pStream;
                pStreamInfo->nCurrentStreamIdx += 1;
                if(pStreamInfo->nCurrentStreamIdx >= (HEVC_MAX_STREAM_NUM - 1))
                {
                    logw("Hevc maybe error. Too many sbm streams in current frame. list is full");
                    HevcDecReturnAllSbmStreams(pHevcDec);
                    return VDECODE_RESULT_NO_BITSTREAM;
                }
            }
            nCounter--;
            continue;
        }
        //*step 1. process sbm-cycle-buffer
        if(pStream->pData + pStream->nLength > pStreamInfo->pSbmBufEnd)
        {
            s32 nSize0 = (s32)(pStreamInfo->pSbmBufEnd - pStream->pData) + 1;
            pStreamInfo->nFirstDataTrunkSize = nSize0;
            pStreamInfo->bHasTwoDataTrunkFlag = 1;
//            logd("request stream two data chunk,
//                  SIZE0: %d, total size: %d", nSize0, pStream->nLength);
#if HEVC_CLOSE_SBM_CYCLE_BUFFER
            pStreamInfo->bUseLocalBufFlag = 1;
            pStreamInfo->bHasTwoDataTrunkFlag = 0;
            pStreamInfo->nFirstDataTrunkSize = pStream->nLength;
            logd("----- use local buffer, SIZE0: %d, total size: %d",
                nSize0, pStream->nLength);
#else
            pStreamInfo->bUseLocalBufFlag = 0;
#endif
            if(pStreamInfo->bUseLocalBufFlag)
            {
                memcpy(pStreamInfo->pLocalBuf, pStream->pData, nSize0);
                memcpy(pStreamInfo->pLocalBuf + nSize0,
                    pStreamInfo->pSbmBuf, pStream->nLength - nSize0);
                pStreamInfo->pCurrentStreamDataptr = pStreamInfo->pLocalBuf;
            }
            else
                pStreamInfo->pCurrentStreamDataptr = pStream->pData;

            logv(" has two data trunk. total size: %d, size0: %d ",
                pStream->nLength, nSize0);
#if HEVC_SAVE_STREAM_DATA
#define HEVC_SAVE_STREAM_PATH "/data/camera/funny.h265"
            if(1)
            {
                FILE *fp = fopen(HEVC_SAVE_STREAM_PATH, "ab");
                if(fp == NULL)
                {
                    logd(" save h265 stream open file error ");
                }
                else
                {
                    logd(" two data trunk ");
                    fwrite(pStream->pData, 1, nSize0, fp);
                    fwrite(pStreamInfo->pSbmBuf, 1, pStream->nLength - nSize0, fp);
                    fclose(fp);
                }
            }
#endif
        }
        else
        {
#if HEVC_CLOSE_SBM_CYCLE_BUFFER
            pStreamInfo->bUseLocalBufFlag = 0;
#endif
            pStreamInfo->pCurrentStreamDataptr = pStream->pData;
            pStreamInfo->nFirstDataTrunkSize = pStream->nLength;
            pStreamInfo->bHasTwoDataTrunkFlag = 0;
//            logd(" current nalu total size: %d ", pStream->nLength);
#if HEVC_SAVE_STREAM_DATA
            if(1)
            {
                FILE *fp = fopen(HEVC_SAVE_STREAM_PATH, "ab");
                if(fp == NULL)
                {
                    logd(" save h265 stream open file error ");
                }
                else
                {
                    fwrite(pStream->pData, 1, pStream->nLength, fp);
                    fclose(fp);
                }
            }
#endif
        }
        pStreamInfo->nCurrentStreamDataSize = pStream->nLength;
        pStreamInfo->pStreamTemp = pStream;
        logv(" request one stream, \
                data size: %d pts: %lld. start code found: %d,  pts caculat: %d",
                pStream->nLength, pStream->nPts,
                pHevcDec->pStreamInfo->bCurrentFrameStartcodeFound,
                pHevcDec->pStreamInfo->bPtsCaculated);
        //*step 2: caculate the pts and duration
        if(pHevcDec->pStreamInfo->bPtsCaculated == 0)
        {
            /* step 2.1: get the stream pts, if stream pts is unvalid, caculate one */
            if(pStream->nPts != -1)
            {
                /* todo: if pStream->nPst is error? */
                logv(" get stream pts: %lld, current pts: %lld, diff: %lld  ------ ", \
                pStream->nPts,pHevcDec->nPts, pStream->nPts - pHevcDec->nPts);
                if(pHevcDec->pVideoStreamInfo->nFrameDuration == 0 &&
                    pStreamInfo->bStreamWithStartCode)
                    HevcCaculateFrameDuration(pHevcDec, pStream->nPts);
                pHevcDec->nPts = pStream->nPts;
            }
            else
            {
                if(pHevcDec->pVideoStreamInfo->nFrameDuration != 0)
                    pHevcDec->nPts += pHevcDec->pVideoStreamInfo->nFrameDuration;
                else
                    pHevcDec->nPts += 33300;
            }
            pHevcDec->pStreamInfo->bPtsCaculated = 1;
            /* step 2.2: caculate one frame duration */
            /* for pts caculation start */
//            pHevcDec->bFrameDurationZeroFlag = 0;
            /* maybe we needn't caculate the duration each time */
            if(pHevcDec->bFrameDurationZeroFlag && pHevcDec->nLastStreamPts != -1)
            {
                int64_t nDeltaPts = pHevcDec->nPts - pHevcDec->nLastStreamPts;
                if(nDeltaPts > 0)
                {
                    if(pHevcDec->nStreamFrameNum > 1)
                    {
                        pHevcDec->pVideoStreamInfo->nFrameDuration =
                            nDeltaPts / pHevcDec->nStreamFrameNum;
                        if(pHevcDec->pVideoStreamInfo->nFrameDuration > 50000)
                        {
                            pHevcDec->pVideoStreamInfo->nFrameDuration = 33000;
                        }
                        logv(" nDeltaPts: %lld, nStreamFrameNum: %d get nFrameDuration: %d ",
                                nDeltaPts, pHevcDec->nStreamFrameNum,
                                pHevcDec->pVideoStreamInfo->nFrameDuration);
                         pHevcDec->H265PerformInf.H265PerfInfo.nFrameDuration =
                            pHevcDec->pVideoStreamInfo->nFrameDuration;
                    }
                }
            }
            pHevcDec->nStreamFrameNum = 0;
            pHevcDec->nLastStreamPts = pHevcDec->nPts;
            /* for pts caculation end */
        }
        pStreamInfo->nCurrentStreamNaluNum = 0;

        return VDECODE_RESULT_OK;
    }
    return VDECODE_RESULT_NO_BITSTREAM;
}

/* olny search 4 bytes return after start code index. if not foud, return -1 */
static s32 DecoderSearchStartCode4bytes(HevcStreamInfo *pStreamInfo)
{
    CEDARC_UNUSE(DecoderSearchStartCode4bytes);

    s32 i, nIndex;
    char *pBuf = pStreamInfo->pCurrentStreamDataptr;
    nIndex = -1;
    if(pStreamInfo->bHasTwoDataTrunkFlag)
    {
        s32 nSize0 = pStreamInfo->nFirstDataTrunkSize;
        char *p = pBuf;
        char *p1 = pBuf + 1;
        if(nSize0 > 4)
        {
            if(IS_HEVC_START_CODE(p))
            {
                pStreamInfo->pAfterStartCodePtr = p + 3;
                nIndex = 3;
            }
            else if(IS_HEVC_START_CODE(p1))
            {
                pStreamInfo->pAfterStartCodePtr = p + 4;
                nIndex = 4;
            }
        }
        else
        {
            /* step 1.2: two data trunk and first data trunk size less than 4
             * this case is very rare..... */
            char pTmp[4], *pT, *pT1;
            loge(" Wow!!! Rare case... hevc start code in the cycle buffer. \
                first data trunk size: %d ", nSize0);
            for(i = 0; i < nSize0; i++)
                pTmp[i] = p[i];
            for( ; i < 4; i++)
                pTmp[i] = pStreamInfo->pSbmBuf[i];
            pT = pTmp;
            pT1 = &pTmp[1];
            if(IS_HEVC_START_CODE(pT))
            {
                if(nSize0 == 3)
                    pStreamInfo->pAfterStartCodePtr = pStreamInfo->pSbmBuf;
                else
                    pStreamInfo->pAfterStartCodePtr = pStreamInfo->pSbmBuf + (3 - nSize0);
                nIndex = 3;
            }
            else if(IS_HEVC_START_CODE(pT1))
            {
                pStreamInfo->pAfterStartCodePtr = pStreamInfo->pSbmBuf + (4 - nSize0);
                nIndex = 4;
            }
        }
    }
    else /* one data trunk case */
    {
        char *p = pBuf;
        char *p1 = pBuf + 1;
        if(IS_HEVC_START_CODE(p))
        {
            pStreamInfo->pAfterStartCodePtr = p + 3;
            nIndex = 3;
        }
        else if(IS_HEVC_START_CODE(p1))
        {
            pStreamInfo->pAfterStartCodePtr = p + 4;
            nIndex = 4;
        }
    }
    return nIndex;
}

static s32 DecoderSearchStreamDataStartCode2(HevcContex *pHevcDec, s32 *nIndex)
{
/* Search current data's nalu start code, get the value data's location */
    HevcStreamInfo *pStreamInfo = pHevcDec->pStreamInfo;
    s32 nNaluSize = -1;
    s32 nTemporalId = 0;
    if(pStreamInfo->bStreamWithStartCode == 0)
    {
        s32 nSize;
        if(pStreamInfo->bHasTwoDataTrunkFlag &&
            (pStreamInfo->pCurrentStreamDataptr) + 6 > pStreamInfo->pSbmBufEnd)
        {
            char pBuf[6];
            s32  nSize0 = pStreamInfo->pSbmBufEnd - pStreamInfo->pCurrentStreamDataptr + 1;
            s32 i;
            for(i = 0; i < nSize0; i++)
                pBuf[i] = pStreamInfo->pCurrentStreamDataptr[i];
            for(; i < 6; i++)
                pBuf[i] = pStreamInfo->pSbmBuf[i - nSize0];
            nSize = (pBuf[0] << 24) | (pBuf[1] << 16) | (pBuf[2] << 8) | pBuf[3];
            pStreamInfo->pAfterStartCodePtr = pStreamInfo->pSbmBuf + (4 - nSize0);
            nTemporalId     = pBuf[5] & 0x3; // HevcGetNBits(3, gb, "nuh_temporal_id_plus1") - 1;
        }
        else
        {
            char *pBuf = pStreamInfo->pCurrentStreamDataptr;
            nSize = (pBuf[0] << 24) | (pBuf[1] << 16) | (pBuf[2] << 8) | pBuf[3];
            pStreamInfo->pAfterStartCodePtr = pBuf + 4;
            nTemporalId     = pBuf[5] & 0x3; // HevcGetNBits(3, gb, "nuh_temporal_id_plus1") - 1;
        }
        *nIndex = 4;
        nNaluSize = nSize;
        if(nTemporalId < 1)
            nNaluSize = -1;
        if(nSize > 10*1024*1024)
        {
            loge(" stream size error. size: %d", nSize);
            nNaluSize = -1;
        }
        return nNaluSize;
    }
    else
    {
#if HEVC_HW_SEARCH_START_CODE
        s32 bFlag, nStartCodeOffset, bFoundFlag, i;
        s32 nNaluSize = pStreamInfo->nCurrentStreamDataSize;
        char *pBuf = pStreamInfo->pCurrentStreamDataptr;
        /* step 1: if the first four byte contains start code, no need to use hardware */
        /* the first four byte includes start code, hardware searching's offset is not correct */
        /* step 1.1 two data trunk case */
        bFoundFlag = DecoderSearchStartCode4bytes(pStreamInfo);
        if(bFoundFlag != -1)
        {
            logd(" hevc first four byte contains start code. search nalu size: %d  ", nNaluSize);
            *nIndex = bFoundFlag;
            return 0;
        }
        /* step 2: start hardware search start code */
        if(0)
        {
            char *t = pStreamInfo->pCurrentStreamDataptr;
            logd("before hw search first start code ,  %x, %x, %x, %x, %x, %x, %x, %x ",
                    t[0],t[1],t[2],t[3],  t[4],t[5],t[6],t[7]);
        }
#if 0
        if(1)
        {
            u32 addr = (u32)pBuf;
            addr = (addr + 3) & ~3;
            pBuf = (char *)addr;
        }
#endif
        if(pStreamInfo->bUseLocalBufFlag)
        {
            pStreamInfo->pCurrentBufPhy = pStreamInfo->pLocalBufPhy;
            pStreamInfo->pCurrentBufEndPhy = pStreamInfo->pLocalBufEndPhy;
            pStreamInfo->nBitsOffset = (pBuf - pStreamInfo->pLocalBuf) * 8;
        }
        else
        {
            pStreamInfo->pCurrentBufPhy = pStreamInfo->pSbmBufPhy;
            pStreamInfo->pCurrentBufEndPhy = pStreamInfo->pSbmBufEndPhy;
            pStreamInfo->nBitsOffset = (pBuf - pStreamInfo->pSbmBuf) * 8;
        }
        logd(" hardware find first start code start..... data size: %d ", nNaluSize);
        nStartCodeOffset = (s32)HevcHardwareFindStartCode(pHevcDec, nNaluSize);

        if(nStartCodeOffset == 0) /* on start code is found */
        {
            logd(" hw: no start code is found ");
            return -1;
        }
        else
        {
            u32 nIdx, nSize0, nConsumedSize, nCounter;
            u32 nBufferOverFlow;
            char *pSc;
            /* nStartCodeOffset is bit offset starts from sbm buffer base address */
            /* we need to adjust start code's ptr, as the return offset is not so accurate */
            /* the return offset may have 4 bytes error */
            if(nStartCodeOffset > pStreamInfo->nBitsOffset)
            {
                /* case 1: one data trunk; or two data trunk, and start code is in the first */
                nConsumedSize = nStartCodeOffset - pStreamInfo->nBitsOffset;
                nIdx = nConsumedSize >> 3;
                nConsumedSize = nConsumedSize >> 3; /* convert to byte offset */
                /* software adjust ptr */
                if(nIdx > 4)
                {
                    pSc = pBuf + nIdx - 4;
                    nConsumedSize -= 4;
                }
                else
                    pSc = pBuf;
                nCounter = 0;
                while(!(IS_HEVC_START_CODE(pSc)))
                {
                    pSc += 1;
                    nConsumedSize += 1;
                    nCounter += 1;
                    if(nCounter >= 8 || pSc >= (pStreamInfo->pSbmBufEnd - 3))
                    {
                        logw(" Hevc search start code maybe error!! shouldn't happen case ");
                        logw(" Software adjust start code ptr fail. case 1 ");
                        break;
                    }
                }
                pStreamInfo->pAfterStartCodePtr = pSc + 3; /* this is after start code ptr */
                (*nIndex) = nConsumedSize + 3;
            }
            else
            {
                /* case 2: two data trunk, and start code is in the second */
                nIdx = nStartCodeOffset >> 3;
                nSize0 = pStreamInfo->pSbmBufEnd - pBuf + 1;
                loge(" two data trunk case. nSize0: %d ", nSize0);
                nConsumedSize = nSize0 + (nStartCodeOffset >> 3);
                /* software adjust ptr */
                if(nIdx > 4)
                {
                    pSc = pStreamInfo->pSbmBuf + nIdx - 4;
                    nConsumedSize -= 4;
                }
                else
                    pSc = pStreamInfo->pSbmBuf;
                nCounter = 0;
                while(!(IS_HEVC_START_CODE(pSc)))
                {
                    pSc += 1;
                    nConsumedSize += 1;
                    nCounter += 1;
                    if(nCounter >= 8)
                    {
                        logw(" Hevc search start code maybe error!! shouldn't happen case.  ");
                        logw(" Software adjust start code ptr fail. case 2 ");
                        break;
                    }
                }
                pStreamInfo->pAfterStartCodePtr =  pSc + 3;/* this is after start code ptr */
                (*nIndex) = nConsumedSize + 3;
            }
#if 1
            if(1) /* debug */
            {
                char *t = pStreamInfo->pAfterStartCodePtr;
                t -= 3;
                logd(" first start code,  %x, %x, %x, %x, %x, %x, %x, %x  \
                    consum size: %d, idx: %d",
                    t[0],t[1],t[2],t[3],  t[4],t[5],t[6],t[7],  nConsumedSize, nIdx);
            }

#endif
        }
        return 0;

#else //HEVC_HW_SEARCH_START_CODE
        if(pStreamInfo->bHasTwoDataTrunkFlag)
        {
            char pBuf[3];
            char *ptr = pStreamInfo->pCurrentStreamDataptr;
            s32 nSize = pStreamInfo->nCurrentStreamDataSize - 3;
//            logd("bHasTwoDataTrunkFlag: %x, %x, %x, %x, %x, %x, %x, %x ",
//                    ptr[0],ptr[1],ptr[2],ptr[3],ptr[4],ptr[5],ptr[6],ptr[7]);
            logv("bHasTwoDataTrunk pSbmBuf: %p, pSbmBufEnd: %p, curr: %p, diff: %d ",
                    pStreamInfo->pSbmBuf, pStreamInfo->pSbmBufEnd, ptr,
                    (u32)(pStreamInfo->pSbmBufEnd - ptr));
            while(nSize > 0)
            {
                pBuf[0] = HevcSmbReadByte(ptr, 0);
                pBuf[1] = HevcSmbReadByte(ptr, 1);
                pBuf[2] = HevcSmbReadByte(ptr, 2);
                if(pBuf[0] == 0 && pBuf[1] == 0 && pBuf[2] == 1)
                {
                    nNaluSize = 0; //return 0, means there is not nalu size info
                    (*nIndex) += 3; //so that buf[0] is the actual data, not start code
                    HevcSmbPtrPlusOne(ptr);
                    HevcSmbPtrPlusOne(ptr);
                    HevcSmbPtrPlusOne(ptr);
                    pStreamInfo->pAfterStartCodePtr = ptr;
                    /* record the after start code ptr */
                    return 0;
                }
                HevcSmbPtrPlusOne(ptr);
                ++(*nIndex);
                --nSize;
            }
//            logd(" search start code.bHasTwoDataTrunkFlag. nIndex: %d ", *nIndex);
        }
        else
        {
            char *pBuf = pStreamInfo->pCurrentStreamDataptr;
            s32 nSize = pStreamInfo->nCurrentStreamDataSize - 3;
            while(nSize > 0)
            {
                if(pBuf[0] == 0 && pBuf[1] == 0 && pBuf[2] == 1)
                {
                    nNaluSize = 0; //return 0, means there is not nalu size info
                    (*nIndex) += 3; //so that buf[0] is the actual data, not start code
                    pStreamInfo->pAfterStartCodePtr = pBuf + 3;
                    /* record the after start code ptr */
                    return 0;
                }
                ++pBuf;
                ++(*nIndex);
                --nSize;
            }
        }
#endif //HEVC_HW_SEARCH_START_CODE
        return -1;
    }
    return -1;

}

static void DecoderReturnOneSbmStream(HevcStreamInfo *pStreamInfo)
{
    if(pStreamInfo->pStreamTemp != NULL)
        SbmFlushStream(pStreamInfo->pSbm, pStreamInfo->pStreamTemp);
    pStreamInfo->nCurrentBufSize = 0;
    pStreamInfo->pCurrentStreamDataptr = NULL;
    pStreamInfo->nCurrentStreamDataSize = 0;
    pStreamInfo->pStreamTemp = NULL;
}

static s32 HevcDecoderAddPtsToList(HevcContex *pHevcDec, VideoStreamDataInfo *pStream)
{
    s32 i;
    int64_t *pList;

    if(pStream == NULL && pHevcDec->pStreamInfo->pSbm->nType == SBM_TYPE_STREAM)
        return -1;
    /* step 1: caculate a valid pts, us */

    if(pHevcDec->nPts == pHevcDec->nLastPts)
    {
        logd(" _____caculate pts case . duration: %d ", pHevcDec->pVideoStreamInfo->nFrameDuration);
        if(pHevcDec->pVideoStreamInfo->nFrameDuration == 0)
            pHevcDec->nPts += 33300;
        else
            pHevcDec->nPts += pHevcDec->pVideoStreamInfo->nFrameDuration;
    }

    logv("add pts: %lld to list;  last pts: %lld, diff: %lld",
            pHevcDec->nPts, pHevcDec->nLastPts, (pHevcDec->nPts - pHevcDec->nLastPts));
    pHevcDec->nLastPts = pHevcDec->nPts;

    pHevcDec->nStreamFrameNum += 1;
    pHevcDec->pStreamInfo->bPtsCaculated = 0;
    /* step 2: add pts to list */
    pList = pHevcDec->PtsList;
    for(i = 0; i < HEVC_PTS_LIST_SIZE; i++)
    {
        if(pHevcDec->nPts == pList[i])
        {
            logw(" h265 pts list error. duplicate pts: %lld", (long long int)pHevcDec->nPts);
            return -1;
        }
        if(pList[i] == -1)
        {
            pList[i] = pHevcDec->nPts;
            return i;
        }
    }
    logw(" h265 pts list is full ");
    return -1;
}

static void HevcDecoderDeleteOnePtsFromList(HevcContex *pHevcDec, int64_t nPts)
{
    s32 i;
    s32 bFound = 0;
    for(i = 0; i < HEVC_PTS_LIST_SIZE; i++)
    {
        if(pHevcDec->PtsList[i] == nPts)
        {
            pHevcDec->PtsList[i] = -1;
            bFound = 1;
            break;
        }
    }
    if(bFound == 0)
    {
        logv(" hevc decoder delete one pts fail. Maybe had deleted. Can not find pts: %lld ", nPts);
    }
}

static s32 DecoderDeleteEmulationCode(char **pOutBuf, s32 *pOutputSize, char *pBuf, s32 nSize)
{
    CEDARC_UNUSE(DecoderDeleteEmulationCode);
    s32 i, nSkipped;
    s32 nTemp = -1;
    const u32 mask = 0xFFFFFF;

    *pOutputSize = 0;
    nSkipped = 0;
    *pOutBuf = pBuf;

    for(i = 0; i < nSize; i++)
    {
        nTemp = (nTemp << 8) | pBuf[i];
        switch(nTemp & mask)
        {
        case HEVC_START_CODE:
            *pOutputSize = i - 2 - nSkipped;
            return i - 2;
        case HEVC_EMULATION_CODE:
            nSkipped += 1;
            break;
        default:
            if(nSkipped > 0)
                pBuf[i - nSkipped] = pBuf[i];
        }
    }
//    logd(" there is no start code int the current data anymore. data size: %d", i);
    *pOutputSize = i - nSkipped;
    return i;
}

/* get the ended ptr of current nal unit, return the actual nal unit size */
static s32 HevcFindNextNaluStartCode(HevcContex *pHevcDec, s32 nNaluSize)
{
    HevcStreamInfo *pStreamInfo = pHevcDec->pStreamInfo;
    char *pBuf = pStreamInfo->pCurrentStreamDataptr;
    s32 i, nConsumedSize;
    s32 nSize;

    if(pStreamInfo->bStreamWithStartCode == 0)
    {
        if(pStreamInfo->bHasTwoDataTrunkFlag)
        {
            pStreamInfo->pCurrentStreamDataptr += nNaluSize;
            if(pStreamInfo->pCurrentStreamDataptr > pStreamInfo->pSbmBufEnd)
            {
                s32 nExtraSize = pStreamInfo->pCurrentStreamDataptr - (pStreamInfo->pSbmBufEnd + 1);
                pStreamInfo->pCurrentStreamDataptr = pStreamInfo->pSbmBuf + nExtraSize;
//                logd(" HevcFindNextNaluStartCode().
//                        first size: %d,  nalu size: %d  nExtraSize: %d",
//                        pStreamInfo->nFirstDataTrunkSize, nNaluSize, nExtraSize);
            }
        }
        else
            pStreamInfo->pCurrentStreamDataptr += nNaluSize;
        return nNaluSize;
    }

    if(pStreamInfo->bSearchNextStartCode == 0)
        nConsumedSize = nNaluSize;
    /* only one start code in the data trunk, one nal unit */
    else
    {
        /* maybe ts container */
#if HEVC_HW_SEARCH_START_CODE
        s32 bFlag;
        s32 nStartCodeOffset;
#if 0
        if(1)
        {
            size_addr addr = (size_addr)pBuf;
            addr = (addr + 3) & ~3;
            pBuf = (char *)addr;
        }
#endif
        if(pStreamInfo->bUseLocalBufFlag)
        {
            pStreamInfo->pCurrentBufPhy = pStreamInfo->pLocalBufPhy;
            pStreamInfo->pCurrentBufEndPhy = pStreamInfo->pLocalBufEndPhy;
            pStreamInfo->nBitsOffset = (pBuf - pStreamInfo->pLocalBuf) * 8;
        }
        else
        {
            pStreamInfo->pCurrentBufPhy = pStreamInfo->pSbmBufPhy;
            pStreamInfo->pCurrentBufEndPhy = pStreamInfo->pSbmBufEndPhy;
            pStreamInfo->nBitsOffset = (pBuf - pStreamInfo->pSbmBuf) * 8;
        }
        logd(" hardware find next start code start..... data size: %d ", nNaluSize);
        nStartCodeOffset = (s32)HevcHardwareFindStartCode(pHevcDec, nNaluSize);
        if(nStartCodeOffset == 0) /* on start code is found */
        {
            logd(" hw: no start code is found, current size(%d) is the nal unit size ",
                nNaluSize);
            nConsumedSize = nNaluSize;
        }
        else
        {
            u32 nIndex, nSize0, nCounter;
            char *pSc;
            if(nStartCodeOffset > pStreamInfo->nBitsOffset)
            {
                /* case 1: only one data trunk, or two data trunk, but start code is in the first */
                nConsumedSize = nStartCodeOffset - pStreamInfo->nBitsOffset;
                nIndex = nConsumedSize >> 3;
                nConsumedSize = nConsumedSize >> 3; /* byte offset */

                /* software adjust ptr */
                if(nIndex > 4)
                {
                    pSc = pBuf + nIndex - 4;
                    nConsumedSize -= 4;
                }
                else
                    pSc = pBuf;
                nCounter = 0;
                while(!(IS_HEVC_START_CODE(pSc)))
                {
                    pSc += 1;
                    nConsumedSize += 1;
                    nCounter += 1;
                    if(nCounter >= 8 || pSc >= (pStreamInfo->pSbmBufEnd - 3))
                    {
                        logw(" Hevc search start code maybe error!! shouldn't happen case ");
                        logw(" Software adjust start code ptr fail. case 1 ");
                        break;
                    }
                }
                pStreamInfo->pCurrentStreamDataptr = pSc;
            }
            else
            {
                /* case 2: two data trunk and the start code is in the second */
                nIndex = nStartCodeOffset >> 3;
                nSize0 = pStreamInfo->pSbmBufEnd - pBuf + 1;
                loge(" next start code. two data trunk case. nSize0: %d ", nSize0);
                nConsumedSize = nSize0 + (nStartCodeOffset >> 3);
                /* software adjust ptr */
                if(nIndex > 4)
                {
                    pSc = pStreamInfo->pSbmBuf + nIndex - 4;
                    nConsumedSize -= 4;
                }
                else
                    pSc = pStreamInfo->pSbmBuf;
                nCounter = 0;
                while(!(IS_HEVC_START_CODE(pSc)))
                {
                    pSc += 1;
                    nConsumedSize += 1;
                    nCounter += 1;
                    if(nCounter >= 8)
                    {
                        logw(" Hevc search start code maybe error!! shouldn't happen case.  ");
                        logw(" Software adjust start code ptr fail. case 2 ");
                        break;
                    }
                }
                pStreamInfo->pCurrentStreamDataptr = pSc;
            }
#if 1
            if(1)
            {
                char *t = pStreamInfo->pCurrentStreamDataptr;
                t -= 4;
                logd(" start code, before: %x, %x, %x, %x   after: %x, %x, %x, %x",
                        t[0],t[1],t[2],t[3],  t[4],t[5],t[6],t[7]);
            }

#endif
        }
        logd(" find start code finish. nConsumedSize: %d  ", nConsumedSize);
#else // HEVC_HW_SEARCH_START_CODE
        nConsumedSize = 0;
        if(pStreamInfo->bHasTwoDataTrunkFlag)
        {
            char *ptr = pStreamInfo->pCurrentStreamDataptr;
            char pBuf[3];
            nSize = pStreamInfo->nCurrentStreamDataSize - 3;
            for(i = 0; i < nSize; i++)
            {
                pBuf[0] = HevcSmbReadByte(ptr, 0);
                pBuf[1] = HevcSmbReadByte(ptr, 1);
                pBuf[2] = HevcSmbReadByte(ptr, 2);
                if(pBuf[0] == 0 && pBuf[1] == 0 && pBuf[2] == 1)
                {
                    pStreamInfo->pCurrentStreamDataptr = ptr;
                    nConsumedSize = i;
                    break;
                }
                HevcSmbPtrPlusOne(ptr);
            }
            if(nConsumedSize == 0)
                nConsumedSize = nNaluSize;
        }
        else
        {
            char *pBuf = pStreamInfo->pCurrentStreamDataptr;
            nSize = pStreamInfo->nCurrentStreamDataSize - 3;
            for(i = 0; i < nSize; i++)
                if(pBuf[i] == 0 && pBuf[i + 1] == 0 && pBuf[i + 2] == 1)
                {
                    pStreamInfo->pCurrentStreamDataptr = pBuf + i;
                    nConsumedSize = i;
                    break;
                }
            if(nConsumedSize == 0)
            {
                nConsumedSize = nNaluSize;
            }
//            logd(" find the next start code, comsum size: %d, nalu size: %d",
//                   nConsumedSize, nNaluSize);
        }
#if 0
            if(1)
            {
                char *t = pStreamInfo->pCurrentStreamDataptr;
                t -= 4;
                logd(" after search next start code, before: %x, %x, %x, %x \
                        after: %x, %x, %x, %x",
                        t[0],t[1],t[2],t[3],  t[4],t[5],t[6],t[7]);
            }
#endif
        return nConsumedSize;
#endif //HEVC_HW_SEARCH_START_CODE
    }
    return nConsumedSize;
}

void  HevcSetPicBufferInfoToVbvParams(HevcContex *pHevcDec, FramePicInfo* pFramePic)
{
    HevcStreamInfo *pStreamInfo = NULL;

    pStreamInfo = pHevcDec->pStreamInfo;
    pStreamInfo->pSbmBuf = (char *)pFramePic->pSbmFrameBufferManager->pFrameBuffer;
    pStreamInfo->nSbmBufSize = pFramePic->pSbmFrameBufferManager->nFrameBufferSize;
    pStreamInfo->pSbmBufEnd = pStreamInfo->pSbmBuf + pStreamInfo->nSbmBufSize - 1;
    pStreamInfo->pSbmBufPhy = pFramePic->pSbmFrameBufferManager->pPhyFrameBuffer;
    pStreamInfo->pSbmBufEndPhy = pFramePic->pSbmFrameBufferManager->pPhyFrameBufferEnd;
}

s32 HevcDecRequestOneFrameStreamNew(HevcContex *pHevcDec)
{
    HevcStreamInfo *pStreamInfo = pHevcDec->pStreamInfo;

    VideoStreamDataInfo *pStream = NULL;
    FramePicInfo*        pFramePic = NULL;

request_stream:

    pStream = SbmRequestStream(pStreamInfo->pSbm);
    if(pStream == NULL)
    {
        return VDECODE_RESULT_NO_BITSTREAM;
    }

    pFramePic = (FramePicInfo*)pStream;
    if(pFramePic->bValidFlag == 0)
    {
        SbmFlushStream(pStreamInfo->pSbm, (VideoStreamDataInfo*)pFramePic);
        goto request_stream;
    }

    if(pStreamInfo->pCurStreamFrame)
        SbmFlushStream(pStreamInfo->pSbm, (VideoStreamDataInfo*)pFramePic);

    pStreamInfo->pCurStreamFrame = pFramePic;
    if(pStreamInfo->pCurStreamFrame->nPts != -1)
        pHevcDec->nPts = pStreamInfo->pCurStreamFrame->nPts;

    pHevcDec->nCurFrameStreamSize = pFramePic->nlength;
    logv("had reqeust stream-frame-pic");
    HevcDecoderAddPtsToList(pHevcDec, NULL);

    if(pHevcDec->pStreamInfo->pSbm->bUseNewVeMemoryProgram == 1)
    {
        HevcSetPicBufferInfoToVbvParams(pHevcDec,pFramePic);
    }

    return 0;

}

s32 HevcDecRequestOneFrameStream2(HevcContex *pHevcDec)
{
    HevcStreamInfo *pStreamInfo;
    s32 bKeyFrameOnly = 0;
    s32 nNaluDataSize;
    s32 nAfterStartCodeIdx;
    s32 nCounter = HEVC_SEARCH_START_CODE_TIME;
    HevcNaluType eNaluType = HEVC_UNSPEC63;
    s32 bFirstSliceSegment;
    s32 nConsumedSize;
    char *pCurrentDataPtr;
    s32 nRet, nIdx;

    pStreamInfo = pHevcDec->pStreamInfo;
    bKeyFrameOnly = pStreamInfo->bFindKeyFrameOnly;

    while(1)
    {
        nAfterStartCodeIdx = 0;
//        usleep(500*1000);  /* used in function debug */
        /* step 1: if current stream is NULL or data isn't enough, request one stream*/
        if(pStreamInfo->nCurrentStreamDataSize < 5 || pStreamInfo->pCurrentStreamDataptr == NULL)
        {
            if(pStreamInfo->pStreamTemp != NULL)
            {
                if(pStreamInfo->nCurrentStreamIdx == 0)
                    DecoderReturnOneSbmStream(pStreamInfo);
                else if(pStreamInfo->nCurrentStreamIdx > 0 &&
                    pStreamInfo->pStreamList[pStreamInfo->nCurrentStreamIdx - 1] !=
                    pStreamInfo->pStreamTemp)
                {
                    pStreamInfo->pStreamList[pStreamInfo->nCurrentStreamIdx] =
                        pStreamInfo->pStreamTemp;
                    pStreamInfo->nCurrentStreamIdx += 1;
                    pStreamInfo->pStreamTemp = NULL;
                }
            }
            nRet = DecoderRequestOneSbmStream2(pHevcDec);
            if(nRet == VDECODE_RESULT_NO_BITSTREAM)
            {
                return nRet;
            }
            logv(" request one stream successfully: 0x%p", pStreamInfo->pStreamTemp);
        }
        else
        {
            logv(" there is still enough data, no need to request a new stream data, \
                data lenght: %d", pStreamInfo->nCurrentStreamDataSize);
        }
//        usleep(1000*1000);
        /* step 2: find one nal unit's start code */
        nNaluDataSize = DecoderSearchStreamDataStartCode2(pHevcDec, &nAfterStartCodeIdx);
        pStreamInfo->nFirstDataTrunkSize -= nAfterStartCodeIdx;
        if(pStreamInfo->nFirstDataTrunkSize < 0)
            pStreamInfo->nFirstDataTrunkSize = 0;

        if(nNaluDataSize >= 0) /* means this data trunk has value nalu start code */
        {
            pStreamInfo->nCurrentStreamNaluNum += 1;
            nCounter = HEVC_SEARCH_START_CODE_TIME;
            logv(" find a start code, search nalu number: %d", \
                pStreamInfo->nCurrentStreamNaluNum);
        }
        else /* no start code is found in the rest data, request one stream again */
        {
            if(pStreamInfo->nCurrentDataBufIdx == 0 &&
                /* 1. This stream has no valid data  */
               pStreamInfo->nCurrentStreamIdx == 0)
               /* 2. And no streams in the list yet */
            {
                /* So we can return current stream   */
//                loge(" h265 decoder request frame's nalu error.
//                       There is no start code in the stream data ");
//                logd(" stream nalu number: %d, buf idx: %d, stream idx: %d ",
//                        pStreamInfo->nCurrentStreamNaluNum,
//                        pStreamInfo->nCurrentDataBufIdx,
//                        pStreamInfo->nCurrentStreamIdx);
                /* todo: if we have a few stream now,
                we shouldnot return the current stream immediately */
                pStreamInfo->nCurrentDataBufIdx = 0;
                DecoderReturnOneSbmStream(pStreamInfo);
            }
            else
            {
                /* means there are valid data in this stream, or stream list is no empty,
                 * as we should return sbm stream in order, but previous streams is valid.
                 * So we have no choice but adding current stream to list,
                 although it is invalid. */
                logv(" no start code in the rest data, add stream to list,\
                        continue to request stream ");
                pStreamInfo->pStreamList[pStreamInfo->nCurrentStreamIdx] =
                        pStreamInfo->pStreamTemp;
                pStreamInfo->nCurrentStreamIdx += 1;
                if(pStreamInfo->nCurrentStreamIdx >= HEVC_MAX_STREAM_NUM)
                {
                    logw(" Hevc maybe error. Too many sbm stream in current frame. \
                        Stream List is full ");
                    HevcDecReturnAllSbmStreams(pHevcDec);
                }
                pStreamInfo->pStreamTemp = NULL;
                pStreamInfo->pCurrentStreamDataptr = NULL;
                pStreamInfo->nCurrentStreamDataSize = 0;
            }
            nCounter--;
            if(nCounter > 0) /* avoid infinite loop */
                continue;
            else
            {
                logw(" h265 decoder cannot find start code within %d frame stream data",
                     HEVC_SEARCH_START_CODE_TIME);
                pStreamInfo->pStreamList[pStreamInfo->nCurrentStreamIdx] =
                        pStreamInfo->pStreamTemp;
                pStreamInfo->nCurrentStreamIdx += 1;
                pStreamInfo->pStreamTemp = NULL;
                pStreamInfo->pCurrentStreamDataptr = NULL;
                pStreamInfo->nCurrentStreamDataSize = 0;
                HevcDecReturnAllSbmStreams(pHevcDec);
                return VDECODE_RESULT_NO_BITSTREAM;
            }
        }

        /* step 3: by now, we have found a start code, if this start code belongs to the
         * next frame, then we have got all nal units in current frame, return.
         */
        eNaluType = (pStreamInfo->pAfterStartCodePtr[0] & 0x7e) >> 1;
        logv(" get eNaluType %d, nAfterStartCodeIdx: %d", eNaluType, nAfterStartCodeIdx);
        if(bKeyFrameOnly)
        {
            if(eNaluType != HEVC_NAL_IDR_W_RADL && eNaluType != HEVC_NAL_VPS &&
                eNaluType != HEVC_NAL_IDR_N_LP && eNaluType != HEVC_NAL_SPS &&
                eNaluType != HEVC_NAL_CRA_NUT && eNaluType != HEVC_NAL_PPS)
            {
                /* current nal unit is not the keyframe start flag */
                logv(" key frame only flag, continue search next nalu, \
                        nalu type: %d, nAfterStartCodeIdx: %d, size: %d",
                        eNaluType, nAfterStartCodeIdx, pStreamInfo->nCurrentStreamDataSize);
                if(pStreamInfo->bStreamWithStartCode == 0) /* mov, mkv case */
                {
                    if(pStreamInfo->bHasTwoDataTrunkFlag) /* two data trunk case */
                    {
                        pStreamInfo->pCurrentStreamDataptr += (nAfterStartCodeIdx + nNaluDataSize);
                        if(pStreamInfo->pCurrentStreamDataptr > pStreamInfo->pSbmBufEnd)
                        {
                            s32 nExtraSize = pStreamInfo->pCurrentStreamDataptr -
                                (pStreamInfo->pSbmBufEnd + 1);
                            pStreamInfo->pCurrentStreamDataptr =
                                pStreamInfo->pSbmBuf + nExtraSize;
                        }
                    }
                    else
                        pStreamInfo->pCurrentStreamDataptr +=
                            (nAfterStartCodeIdx + nNaluDataSize);

                    pStreamInfo->nCurrentStreamDataSize -=
                        (nAfterStartCodeIdx + nNaluDataSize);
                }
                else
                {
                    pStreamInfo->nCurrentStreamDataSize -= nAfterStartCodeIdx;
                    pStreamInfo->pCurrentStreamDataptr = pStreamInfo->pAfterStartCodePtr;
                }
                if(pStreamInfo->bSearchNextStartCode && eNaluType <= HEVC_NAL_BLA_N_LP)
                {
                    if(pStreamInfo->bHasTwoDataTrunkFlag)
                    {
                        char *ptr = pStreamInfo->pAfterStartCodePtr;
                        HevcSmbPtrPlusOne(ptr);
                        HevcSmbPtrPlusOne(ptr);
                        bFirstSliceSegment = (ptr[0] >> 7);
                        logv("bHasTwoDataTrunk 000 pSbmBuf: %p, pSbmBufEnd: %p,  \
                                curr: %p, diff: %d ",
                                pStreamInfo->pSbmBuf, pStreamInfo->pSbmBufEnd, ptr,
                                (u32)(pStreamInfo->pSbmBufEnd - ptr));
                    }
                    else
                        bFirstSliceSegment = (pStreamInfo->pAfterStartCodePtr[2] >> 7);
                    if(bFirstSliceSegment) /* in this case, we need to caculate pts */
                    {
//                        pHevcDec->nLastStreamPts = pHevcDec->nPts;
                        pHevcDec->nStreamFrameNum += 1;
                    }
                }
                pHevcDec->pStreamInfo->bPtsCaculated = 0;
                continue; /* attention: add stream to stream list if necessary */
            }
            else if(eNaluType == HEVC_NAL_IDR_W_RADL ||
                    eNaluType == HEVC_NAL_IDR_N_LP ||
                    eNaluType == HEVC_NAL_CRA_NUT)
            {
                /* found a keyframe */
//                logd(" found a key frame's start nalu,
//                       delete the key frame only flag,
//                       just search next frame's start nalu ");
                bKeyFrameOnly = 0;
                pStreamInfo->bFindKeyFrameOnly = 0;
            }
        }
        bFirstSliceSegment = 0;
        if((eNaluType >= HEVC_NAL_VPS && eNaluType <= HEVC_NAL_AUD) ||
            eNaluType == HEVC_NAL_SEI_PREFIX /*||
            (eNaluType >= 41 && eNaluType <= 44) || (eNaluType >= 48 && eNaluType <= 63)*/)
        {
            /* Begining of access unit, needn't bFirstSliceSegment */
            if(pStreamInfo->bCurrentFrameStartcodeFound == 1)
            {
//                logd(" the next nalu is the Begining of access unit ");
                pStreamInfo->bCurrentFrameStartcodeFound = 0;
                if(pStreamInfo->eNaluType == HEVC_NAL_IDR_W_RADL ||
                    pStreamInfo->eNaluType == HEVC_NAL_IDR_N_LP ||
                    pStreamInfo->eNaluType == HEVC_NAL_CRA_NUT)
                    return 2; /* return key frame */
                else
                    return 1;
            }
        }
        else if(eNaluType <= HEVC_NAL_RASL_R ||
            (eNaluType >= HEVC_NAL_BLA_W_LP && eNaluType <= HEVC_NAL_CRA_NUT))
        {
            /* frame nalu, check if this is the start nal unit */
            if(pStreamInfo->bHasTwoDataTrunkFlag)
            {
                char *ptr = pStreamInfo->pAfterStartCodePtr;
                HevcSmbPtrPlusOne(ptr);
                HevcSmbPtrPlusOne(ptr);
                bFirstSliceSegment = (ptr[0] >> 7);
                logv("bHasTwoDataTrunk 111 pSbmBuf: %p, pSbmBufEnd: %p, \
                        curr: %p, diff: %d ",
                        pStreamInfo->pSbmBuf, pStreamInfo->pSbmBufEnd, ptr,
                        (u32)(pStreamInfo->pSbmBufEnd - ptr));
            }
            else
                bFirstSliceSegment = (pStreamInfo->pAfterStartCodePtr[2] >> 7);
            if(bFirstSliceSegment == 1)
            {
//                logd(" find first slice segment in a frame ");
                if(pStreamInfo->bCurrentFrameStartcodeFound == 0)
                {
                    pStreamInfo->eNaluType = eNaluType;
                    pStreamInfo->bCurrentFrameStartcodeFound = 1;
//                    logd(" This is the current frame's first slice segment,
//                           we need to find the next frame's start nalu  ");
                    nIdx = HevcDecoderAddPtsToList(pHevcDec, pStreamInfo->pStreamTemp);
                    HevcPtsListInfoDebug(pHevcDec, nIdx, 1, 0);
                }
                else
                {
//                    logd(" This is next frame's first slice segment,
//                           find a whole frame' nalu, return");
                    pStreamInfo->bCurrentFrameStartcodeFound = 0;
                    if(pStreamInfo->eNaluType == HEVC_NAL_IDR_W_RADL ||
                        pStreamInfo->eNaluType == HEVC_NAL_IDR_N_LP ||
                        pStreamInfo->eNaluType == HEVC_NAL_CRA_NUT)
                        return 2; /* return key frame */
                    else
                        return 1;
                }
            }
        }

        /* step 4: find the ending(next start code) of current nal unit,
         *  add current nalu data to list */
//        logd(" nCurrentStreamDataSize %d, nAfterStartCodeIdx: %d bStreamWithStartCode: %d",
//                pStreamInfo->nCurrentStreamDataSize,
//                nAfterStartCodeIdx, pStreamInfo->bStreamWithStartCode);
#if 0
        p = pStreamInfo->pAfterStartCodePtr;
        logd(" before find next start code data: %x, %x, %x, %x, %x, %x, %x, %x",
            p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
        for(i = 0; i < 10; i++)
        {
            p += 8;
            logd("%x, %x, %x, %x, %x, %x, %x, %x",
                p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
        }
#endif
        pStreamInfo->nCurrentStreamDataSize -= nAfterStartCodeIdx;
        pStreamInfo->pCurrentStreamDataptr = pStreamInfo->pAfterStartCodePtr;
        if(pStreamInfo->bStreamWithStartCode == 1)
            /* start code is 0x001, set searched data size */
            nNaluDataSize = pStreamInfo->nCurrentStreamDataSize;
        pCurrentDataPtr = pStreamInfo->pCurrentStreamDataptr;
        nConsumedSize = HevcFindNextNaluStartCode(pHevcDec, nNaluDataSize);
//        logd(" --------- add data to list. idx: %d, size: %d, type: %d ",
//                pStreamInfo->nCurrentDataBufIdx, nConsumedSize, eNaluType);

        pStreamInfo->pDataBuf[pStreamInfo->nCurrentDataBufIdx] = pCurrentDataPtr;

        pStreamInfo->bHasTwoDataTrunk[pStreamInfo->nCurrentDataBufIdx] =
            pStreamInfo->bHasTwoDataTrunkFlag;
        pStreamInfo->FirstDataTrunkSize[pStreamInfo->nCurrentDataBufIdx] =
            pStreamInfo->nFirstDataTrunkSize;
        pStreamInfo->bIsLocalBuf[pStreamInfo->nCurrentDataBufIdx] =
            pStreamInfo->bUseLocalBufFlag; /* reserve for future use */
        pStreamInfo->eNaluTypeList[pStreamInfo->nCurrentDataBufIdx] = eNaluType;
        pStreamInfo->nDataSize[pStreamInfo->nCurrentDataBufIdx++] = nConsumedSize;

        if(pStreamInfo->nCurrentDataBufIdx >= (HEVC_MAX_STREAM_NUM - 1))
        {
            logw(" maybe error. Too many nal units in current frame. \
                Stream List is full ");
            logd("pStreamInfo->nCurrentDataBufIdx: %d, \
                    pStreamInfo->nFirstDataTrunkSize: %d, \
                    nConsumedSize: %d, eNaluType: %d ",
                    pStreamInfo->nCurrentDataBufIdx,
                    pStreamInfo->nFirstDataTrunkSize, nConsumedSize, eNaluType);
            HevcDecReturnAllSbmStreams(pHevcDec);
            return VDECODE_RESULT_NO_BITSTREAM;
        }

        pStreamInfo->nCurrentStreamDataSize -= nConsumedSize;
        if(pStreamInfo->bHasTwoDataTrunkFlag)
        {
            pStreamInfo->nFirstDataTrunkSize -=
                (pStreamInfo->pStreamTemp->nLength - pStreamInfo->nCurrentStreamDataSize);
            if(pStreamInfo->nFirstDataTrunkSize < 0)
            {
                pStreamInfo->nFirstDataTrunkSize = pStreamInfo->pStreamTemp->nLength;
            }
        }
        if(pStreamInfo->nCurrentStreamDataSize <= 5)
        {
            logv(" add one stream to list, pStreamInfo: %p ", pStreamInfo->pStreamTemp);
            pStreamInfo->pStreamList[pStreamInfo->nCurrentStreamIdx] =
                pStreamInfo->pStreamTemp;
            pStreamInfo->pStreamTemp = NULL;
            pStreamInfo->nCurrentStreamIdx += 1;
            if(pStreamInfo->nCurrentStreamIdx >= (HEVC_MAX_STREAM_NUM - 1))
            {
                logw(" Hevc maybe error. Too many sbm streams in current frame. \
                    Stream List is full ");
                logd("pStreamInfo->nCurrentDataBufIdx: %d, \
                        pStreamInfo->nFirstDataTrunkSize: %d, \
                        nConsumedSize: %d, eNaluType: %d ",
                        pStreamInfo->nCurrentDataBufIdx,
                        pStreamInfo->nFirstDataTrunkSize, nConsumedSize, eNaluType);
                HevcDecReturnAllSbmStreams(pHevcDec);
            }
        }

        /*
         pStreamInfo->nCurrentStreamIdx and  pStreamInfo->nCurrentDataBufIdx
         should be set to 0 after the current picture is display
         */
    }
    return 0;
}

static void HevcGetPreprocessData(HevcContex *pHevcDec, s32 nIdx)
{
    s32 i, nSize, nBufSize, bTwoTrunk, nTemp, nSkipped;
    HevcStreamInfo *pStreamInfo = pHevcDec->pStreamInfo;
    HevcNaluType eNaluType;
    const s32 mask = 0xffffff;
    char *pData;
    char *pBuf;
    FramePicInfo* pStreamFramePic = NULL;
    NaluInfo* pNaluInfo = NULL;

    pStreamInfo->ShByte = pStreamInfo->pLocalBuf;
    pData = pStreamInfo->ShByte;

    if(pStreamInfo->pSbm->nType == SBM_TYPE_FRAME)
    {

        pStreamFramePic = pHevcDec->pStreamInfo->pCurStreamFrame;
        pNaluInfo = &pStreamFramePic->pNaluInfoList[nIdx];
        pBuf = pNaluInfo->pDataBuf;
        nBufSize = pNaluInfo->nDataSize;
        eNaluType = pNaluInfo->nType;
        if((pBuf + nBufSize) > pStreamInfo->pSbmBufEnd)
            bTwoTrunk = 1;
        else
            bTwoTrunk = 0;
    }
    else
    {
        pBuf = pStreamInfo->pDataBuf[nIdx];
        nBufSize = pStreamInfo->nDataSize[nIdx];
        bTwoTrunk = pStreamInfo->bHasTwoDataTrunk[nIdx]; // used for sbm register
        eNaluType = pStreamInfo->eNaluTypeList[nIdx];
    }

//    logd("  processe data buf. idx: %d, size: %d, type: %d, pBuf: %x, two truk: %d",
//            nIdx, nBufSize, eNaluType, (u32)pBuf, bTwoTrunk);
    /* step 1: copy data */
    nSize = nBufSize;
    if(eNaluType <= HEVC_NAL_CRA_NUT)
    {
        if(nBufSize < HEVC_SLICE_HEADER_BYTES)
            nSize = nBufSize;
        else
            nSize = HEVC_SLICE_HEADER_BYTES;
    }
    if(nSize > pStreamInfo->nLocalBufSize)
    {
        logw("warning: h265 stream data size(%d) > local buffer size(%d), \
            nalu type: %d", nSize, pStreamInfo->nLocalBufSize, eNaluType);
        nSize = pStreamInfo->nLocalBufSize;
    }

    if(!bTwoTrunk)
    {
        memcpy(pData, pBuf, nSize);
    }
    else
    {
        s32 nSize0 = 0;
        if(pStreamInfo->pSbm->nType == SBM_TYPE_FRAME)
        {
            nSize0 = pStreamInfo->pSbmBufEnd - pBuf;
        }
        else
        {
            nSize0 = pStreamInfo->FirstDataTrunkSize[nIdx];
        }

        if(nSize <= nSize0)
            memcpy(pData, pBuf, nSize);
        else /* nSize > nSize0 */
        {
            char *pStart = pStreamInfo->pSbmBuf;
            memcpy(pData, pBuf, nSize0);
            memcpy(pData + nSize0, pStart, nSize - nSize0);
        }
    }
    pStreamInfo->nShByteSize = nSize;

    /* step 2: delet emulation code */
    nSkipped = 0;
    nTemp = -1;
    pStreamInfo->nEmulationCodeNum = 0;
    for(i = 0; i < nSize; i++)
    {
        nTemp = (nTemp << 8) | pData[i];
        switch(nTemp & mask)
        {
        case HEVC_EMULATION_CODE:
            pStreamInfo->EmulationCodeIdx[pStreamInfo->nEmulationCodeNum] = i;
            pStreamInfo->nEmulationCodeNum += 1;
            nSkipped += 1;
            break;
        default:
            if(nSkipped > 0)
                pData[i - nSkipped] = pData[i];
        }
    }
//    logd(" current nalu  pStreamInfo->nEmulationCodeNum: %d, eNaluType",
//            pStreamInfo->nEmulationCodeNum, eNaluType);
}

static void HevcCaculateBufMd5(u8 *md5, char* src, int stride, int width, int height)
{
    char *buf;
    int y,x;
    buf = HevcMalloc(width * height);

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            buf[y * width + x] = src[x] & 0xff;
        }
        src += stride;
    }
    HevcMd5CaculateSum(md5, (u8 *)buf, width * height);
    HevcFree(&buf);
}

static void HevcCaculatePictureMd5(HevcContex *pHevcDec)
{
    CEDARC_UNUSE(HevcCaculatePictureMd5);

    HevcFrame *pHf = pHevcDec->pCurrDPB;
    VideoPicture *pFrame;
    s32 nWidth, nHeight, nStride;
    char *pSrc;
    struct ScMemOpsS *_memops = pHevcDec->pConfig->memops;
    if(pHf == NULL)
        return;
    pFrame = pHf->pFbmBuffer;
    if(pFrame == NULL)
        return;

    CdcMemFlushCache(_memops, pFrame->pData0, pHevcDec->ControlInfo.priFrmBufLumaSize +
            2*pHevcDec->ControlInfo.priFrmBufChromaSize);
    /* y */
    nWidth = pHevcDec->pSps->nWidth;
    nHeight = pHevcDec->pSps->nHeight;
    nStride = pHevcDec->ControlInfo.priFrmBufLumaStride;
    logv(" caculat md5: width: %d,  height: %d, nStride: %d", nWidth, nHeight, nStride);
    pSrc = pFrame->pData0; /* there is no offset parameter in the data used to caculate md5 */
    HevcCaculateBufMd5(pHevcDec->md5Caculate[0], pSrc, nStride, nWidth, nHeight);

    /* u */
    nStride = pHevcDec->ControlInfo.priFrmBufChromaStride;
    nWidth = nWidth >> 1;
    nHeight = nHeight >> 1;
    pSrc = pFrame->pData1;
    HevcCaculateBufMd5(pHevcDec->md5Caculate[1], pSrc, nStride, nWidth, nHeight);

    /* v */
    pSrc = pFrame->pData1 + pHevcDec->ControlInfo.priFrmBufChromaSize;
    HevcCaculateBufMd5(pHevcDec->md5Caculate[2], pSrc, nStride, nWidth, nHeight);
}

static void HevcClearPocList(HevcContex *pHevcDec)
{
    s32 i;
    for(i = 0; i < HEVC_MAX_FRAMES_ONE_SQUEUE; i++)
        pHevcDec->nSqueueFramePocList[i] = HEVC_POC_INIT_VALUE;
}

static void HevcCaculatePictureSha(HevcContex *pHevcDec)
{
    CEDARC_UNUSE(HevcCaculatePictureSha);

    HevcFrame *pHf = pHevcDec->pCurrDPB;
    VideoPicture *pFrame;
    s32 nWidth, nHeight, nStride;
    unsigned char *pSrc;
    struct ScMemOpsS *_memops = pHevcDec->pConfig->memops;
    if(pHf == NULL)
        return;
    pFrame = pHf->pFbmBuffer;
    if(pFrame == NULL)
        return;

    pHevcDec->nDecCountHevc++;
    pFrame->nDebugCount = pHevcDec->nDecCountHevc;
    SHA1_CTX shaCtx;
    unsigned char sha[20]={0};
    unsigned char shaData[4096*2] = {0};
    int i/*, j*/;
    FILE* fp = NULL;
    unsigned char* pShaData = NULL;

    if(pHevcDec->bHadRemoveFile == 0)
    {
        //remove("/tmp/h265Sha.txt");
        pHevcDec->bHadRemoveFile = 1;
    }

    fp = fopen("/tmp/h265Sha.txt", "ab+");
    logd("***** caculate sha: fp = %p",fp);

    memset(&shaCtx, 0, sizeof(SHA1_CTX));
    SHA1Init(&shaCtx);

    logd("***** caculate sha: flush cache, size = %d, decCount = %d",
        pFrame->nBufSize, pHevcDec->nDecCountHevc);
    CdcMemFlushCache(_memops, pFrame->pData0, pFrame->nBufSize);

    logd("***** caculate sha: bEnableAfbcFlag = %d, b10BitPicFlag = %d",
         pHevcDec->bEnableAfbcFlag, pFrame->b10BitPicFlag);
    if(pHevcDec->bEnableAfbcFlag/* && pFrame->b10BitPicFlag*/)
    {
        int nAfbcSize = pFrame->nAfbcSize;
        logd("***** caculate sha(afbc): nAfbcSize = %d",nAfbcSize);
        /* afbc */
        pShaData = (unsigned char *)malloc(nAfbcSize);
        if(pShaData == NULL)
        {
            loge("*** malloc pShaData failed, size = %d",nAfbcSize);
            return ;
        }
        pSrc = (unsigned char *)pFrame->pData0;
        memcpy(pShaData, pSrc, nAfbcSize);

        SHA1Update(&shaCtx, pShaData, nAfbcSize);

        #if 0
        if(pFrame->nDebugCount < 10)
        {
            char name[128];
            FILE *fptmp;

            sprintf(name, "/mnt/install/pict_%dx%d_%d.dat",
                    pFrame->nWidth, pFrame->nHeight,pFrame->nDebugCount);
            logd("*** save afbc data: size = %d, w&h = %d, %d",
                  nAfbcSize, pFrame->nWidth, pFrame->nHeight);
            fptmp = fopen(name, "ab");
            if(fptmp != NULL)
            {

                int nSaveLen = nAfbcSize;

                fwrite(pFrame->pData0, 1, nSaveLen, fptmp);
                fclose(fptmp);
            }
            else
            {
                loge("saving picture open file error, frame number: %d", pFrame->nDebugCount);
            }
        }
        #endif

        //pSrc = pFrame->pData0 + pFrame->nLower2BitBufOffset;
        //SHA1Update(&shaCtx, pSrc, pFrame->nLower2BitBufSize);
    }
    else if(pFrame->b10BitPicFlag)
    {

        /* y */
        nWidth = pHevcDec->pSps->nWidth;
        nHeight = pHevcDec->pSps->nHeight;
        nStride = pHevcDec->ControlInfo.priFrmBufLumaStride;
        logd(" caculat sha(*10bit*): width: %d,  height: %d, nStride: %d",
              nWidth, nHeight, nStride);
        /* there is no offset parameter in the data used to caculate md5 */
        pSrc = (unsigned char *)pFrame->pData0;
        for(i = 0; i < nHeight; i++)
        {
            memcpy(shaData, pSrc, nWidth);
            SHA1Update(&shaCtx, shaData, nWidth);
            pSrc += nStride;
        }

        /* u */
        nStride = pHevcDec->ControlInfo.priFrmBufChromaStride;
        nWidth = nWidth >> 1;
        nHeight = nHeight >> 1;

        pSrc = (unsigned char *)(pFrame->pData1 + pHevcDec->ControlInfo.priFrmBufChromaSize);
        for(i = 0; i < nHeight; i++)
        {
            memcpy(shaData, pSrc, nWidth);
            SHA1Update(&shaCtx, shaData, nWidth);
            pSrc += nStride;
        }

        /* v */
        pSrc = (unsigned char *)pFrame->pData1;
        for(i = 0; i < nHeight; i++)
        {
            memcpy(shaData, pSrc, nWidth);
            SHA1Update(&shaCtx, shaData, nWidth);
            pSrc += nStride;
        }

        #if 1
        /* low 2 bit */

        //* y
        pSrc = (unsigned char *)(pFrame->pData0 + pFrame->nLower2BitBufOffset);

        nWidth = (pHevcDec->pSps->nWidth+3)>>2;
        nHeight = pHevcDec->pSps->nHeight;
        nStride = pFrame->nLower2BitBufStride;
        logd("*****compute lower2Bit sha: pSrc = %p, size = %d, offset = %d",
             pSrc,pFrame->nLower2BitBufSize, pFrame->nLower2BitBufOffset);
        logd(" caculat sha(10bit) -- lower 2: width: %d,  height: %d, nStride: %d",
             nWidth, nHeight, nStride);

        for(i = 0; i < nHeight*3/2; i++)
        {
            memcpy(shaData, pSrc, nWidth);
            SHA1Update(&shaCtx, shaData, nWidth);
            pSrc += nStride;
        }
        #endif

    }
    else
    {

        /* y */
        nWidth = pHevcDec->pSps->nWidth;
        nHeight = pHevcDec->pSps->nHeight;
        nStride = pHevcDec->ControlInfo.priFrmBufLumaStride;
        logd(" caculat sha(8bit): y width: %d,  height: %d, nStride: %d", nWidth, nHeight, nStride);
        pSrc = (unsigned char *)pFrame->pData0;
        for(i = 0; i < nHeight; i++)
        {
            memcpy(shaData, pSrc, nWidth);
            SHA1Update(&shaCtx, shaData, nWidth);
            pSrc += nStride;
        }

        nStride = pHevcDec->ControlInfo.priFrmBufChromaStride;
        nWidth = nWidth >> 1;
        nHeight = nHeight >> 1;

        /* u */
        pSrc = (unsigned char *)(pFrame->pData1 + pHevcDec->ControlInfo.priFrmBufChromaSize);
        logd(" caculat sha(8bit): u width: %d,  height: %d, nStride: %d, priFrmBufChromaSize = %d",
              nWidth, nHeight, nStride, pHevcDec->ControlInfo.priFrmBufChromaSize);
        for(i = 0; i < nHeight; i++)
        {
            memcpy(shaData, pSrc, nWidth);
            SHA1Update(&shaCtx, shaData, nWidth);
            pSrc += nStride;
        }

        /* v */
        pSrc = (unsigned char *)pFrame->pData1;
        logd(" caculat sha(8bit): v width: %d,  height: %d, nStride: %d", nWidth, nHeight, nStride);
        for(i = 0; i < nHeight; i++)
        {
            memcpy(shaData, pSrc, nWidth);
            SHA1Update(&shaCtx, shaData, nWidth);
            pSrc += nStride;
        }

    }

    SHA1Final(sha, &shaCtx);

    if(pShaData)
        free(pShaData);

    logd("Sha: poc(%d), %02x%02x%02x%02x%02x%02x%02x%02x%02x%\
02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
        pHevcDec->nPoc,sha[0],sha[1],sha[2],sha[3],sha[4],sha[5],sha[6],sha[7],sha[8],sha[9],
        sha[10],sha[11],sha[12],sha[13],sha[14],sha[15],sha[16],sha[17],sha[18],sha[19]);

    if(fp != NULL)
    {
        for(i = 0; i < 20; i++)
        {
            fprintf(fp, "%02x", sha[i]);
        }

        fprintf(fp, "\n");
        fclose(fp);
    }
}
static s32 HevcFrameStart(HevcContex *pHevcDec)
{
    s32 nRet;
//    logd(" HevcFrame Start function start ");
    pHevcDec->eFirstNaluType = pHevcDec->eNaluType;
    pHevcDec->SliceHeader.nLastEntryPointOffset = 0;
//    memset(pHevcDec->SliceHeader.pRegEntryPointOffset, 0, HEVC_ENTRY_POINT_OFFSET_BUFFER_SIZE);

    //*check whether the poc is normal
    s32 i;
    for( i = 0; i < HEVC_MAX_FRAMES_ONE_SQUEUE; i++)
    {
    #if 0
        if(pHevcDec->nSqueueFramePocList[i] == pHevcDec->nPoc)
        {
            s32 bFlush = 1;
            HevcOutputFrame(pHevcDec, bFlush);
            HevcResetClearRefs(pHevcDec);
            break;
        }

    #else
        if(pHevcDec->nSqueueFramePocList[i] == pHevcDec->nPoc )
        {
            s32 bFlush = 1;
            logw("*** poc have in prePoc list, \
pocs should be different in one queue, clear refs ****");
            HevcOutputFrame(pHevcDec, bFlush);
            HevcResetClearRefs(pHevcDec);
            if(HEVC_IS_KEYFRAME(pHevcDec))
            {
                HevcClearPocList(pHevcDec);
                break;
            }
            else
            {
               break;//should do something
            }
        }
       #endif

    }

    nRet = HevcConstuctFrameRps(pHevcDec);
    if(nRet != HEVC_RESULT_OK)
    {
        logw(" construct reference picture set failed , ret = %d",nRet);
        return nRet;
    }

    //* reqeust fbm buffer after HevcConstuctFrameRps. so we can reduce one fbm buffer
    nRet = HevcSetNewRef(pHevcDec, pHevcDec->nPoc);
    if(nRet == HEVC_RESULT_NO_FRAME_BUFFER || nRet == HEVC_RESULT_INIT_FBM_FAILED)
    {
        return nRet;
    }

    pHevcDec->debug.nDecodeFrameNum++;
    pHevcDec->debug.nVeCounter = 0;
#if HEVC_SHOW_MAX_USING_BUFFER_NUM
    HevcShowMaxUsingBufferNumber(pHevcDec);
#endif //HEVC_SHOW_MAX_USING_BUFFER_NUM
    return HEVC_RESULT_OK;
}

static void DecoderDebugShowVeCounterList(u32 *l, char *name)
{
    if(l == NULL)
        return;
    logd("%s list[ 0-7 ]: %11u %11u %11u %11u  %11u %11u %11u %11u ", name,
            l[0],l[1],l[2],l[3],l[4],l[5],l[6],l[7]);
    logd("%s list[ 8-15]: %11u %11u %11u %11u  %11u %11u %11u %11u ", name,
            l[0+8],l[1+8],l[2+8],l[3+8],l[4+8],l[5+8],l[6+8],l[7+8]);
}
#define HEVC_DEBUG_SWAP(a,b,tmp) {(tmp)=(a);(a)=(b);(b)=(tmp);}
static int DecoderDebugAddCounterAndBitsToList(u32 *listc, u32 *listb, u32 nCounter, u32 nFrameBits)
{
    u32 i, tmp;
    if(listb == NULL || listc == NULL)
        return -1;
    if(nCounter < listc[0])
        return -1;
    listc[0] = nCounter;
    listb[0] = nFrameBits;
    for(i = 0; i < HEVE_VE_COUNTER_LIST - 1; i++)
    {
         if(listc[i + 1] < nCounter)
         {
             HEVC_DEBUG_SWAP(listc[i],listc[i + 1], tmp);
             HEVC_DEBUG_SWAP(listb[i],listb[i + 1], tmp);
         }
         else
             return i;
    }
    return i;
}
static void DecoderDebugShowFrameTimeList(float *l)
{
    if(l == NULL)
        return;
    logd("ve frameTime list[ 0-7 ]: %.2f, %.2f, %.2f, %.2f,  %.2f, %.2f, %.2f, %.2f ",
            l[0],l[1],l[2],l[3],l[4],l[5],l[6],l[7]);
    logd("ve frameTime  list[ 8-15]: %.2f, %.2f, %.2f, %.2f,  %.2f, %.2f, %.2f, %.2f ",
            l[0+8],l[1+8],l[2+8],l[3+8],l[4+8],l[5+8],l[6+8],l[7+8]);
}
static void DecoderDebugAddFrameTimeToList(float *list, float fTime)
{
    int i;
    float tmp;
    if(list == NULL)
        return;
    if(fTime < list[0])
        return;
    list[0] = fTime;
    for(i = 0; i < HEVE_VE_COUNTER_LIST - 1; i++)
    {
         if(list[i + 1] < fTime)
         {
             HEVC_DEBUG_SWAP(list[i], list[i + 1], tmp);
         }
         else
             return;
    }
}
static void DecoderDebugCaculateTimeInfo(HevcContex *pHevcDec)
{
    CEDARC_UNUSE(DecoderDebugCaculateTimeInfo);

    s32 nDeltaNum, i;
    s64 nTime, nTimeDiff;
    nDeltaNum = HEVC_FRAME_DURATION;
    nTime = HevcGetCurrentTime();
    nTimeDiff = nTime - pHevcDec->debug.nCurrTimeHw;
    pHevcDec->debug.nCurrFrameCostTime += nTimeDiff;
    pHevcDec->debug.nSliceDualationHw += nTimeDiff;
    if(pHevcDec->debug.nCtuNum >= pHevcDec->pSps->nCtbSize)
    {
        float fCurrFrameTime;
        u32 nFrameBits = 0;
        u32 nCurrFrameTime = pHevcDec->debug.nCurrFrameCostTime;
        fCurrFrameTime = nCurrFrameTime/(1000.0);
        logv(" pHevcDec->debug.nCurrFrameCostTime: %lld , \
            fCurrFrameTime: %.2f, nTimeDiff: %lld", \
            pHevcDec->debug.nCurrFrameCostTime, fCurrFrameTime, nTimeDiff);
        pHevcDec->debug.nCurrFrameCostTime = 0;
#if HEVC_SAVE_EACH_FRAME_TIME
        if(1)
        {
            pHevcDec->debug.pAllTimeHw[pHevcDec->debug.nAllTimeNum] = nCurrFrameTime;
            pHevcDec->debug.nAllTimeNum++;
            return;
        }
#endif
        for(i = 0; i < pHevcDec->pStreamInfo->nCurrentDataBufIdx; i++)
             nFrameBits += (pHevcDec->pStreamInfo->nDataSize[i] << 3);
        DecoderDebugAddFrameTimeToList(pHevcDec->debug.fFrameCostTimeList, fCurrFrameTime);
        DecoderDebugAddCounterAndBitsToList(pHevcDec->debug.nVeCntList,
            pHevcDec->debug.nFrameBits, pHevcDec->debug.nVeCounter, nFrameBits);
        pHevcDec->debug.nHwFrameNum += 1;
    }
    if(pHevcDec->debug.nHwFrameNum >= nDeltaNum)
    {
        float fps, avg;
        pHevcDec->debug.nHwTotalTime += pHevcDec->debug.nSliceDualationHw;
        pHevcDec->debug.nHwFrameTotal += pHevcDec->debug.nHwFrameNum;
        fps = pHevcDec->debug.nSliceDualationHw / 1000.0;
        avg = pHevcDec->debug.nHwTotalTime / 1000.0;
        loge(" hardware decode one frame, decode frame: %d, \
                average time: %.2f ms, speed: %.2f fps, average speed: %.2f fps",
                pHevcDec->debug.nFrameNum, fps / nDeltaNum,
                (nDeltaNum *1000) / fps,
                ((float)pHevcDec->debug.nHwFrameTotal)*1000.0 / avg);
        pHevcDec->debug.nHwFrameNum = 0;
        pHevcDec->debug.nSliceDualationHw = 0;
        DecoderDebugShowVeCounterList(pHevcDec->debug.nFrameBits, "frame bit");
        DecoderDebugShowVeCounterList(pHevcDec->debug.nVeCntList, " ve counter");
        DecoderDebugShowFrameTimeList(pHevcDec->debug.fFrameCostTimeList);
    }
}
static void HevcCaculateMvValue(HevcContex *pHevcDec)
{
    if(pHevcDec->pCurrDPB == NULL)
    {
        loge("***pCurrDPB is NULL");
        return;
    }

    HevcFrame *pHf = pHevcDec->pCurrDPB;
    if(pHevcDec->pMvInfo == NULL)
    {
        pHevcDec->pMvInfo = (VIDEO_FRM_MV_INFO *)malloc(sizeof(VIDEO_FRM_MV_INFO));
    }
    memset(pHevcDec->pMvInfo, 0, sizeof(VIDEO_FRM_MV_INFO));
    VIDEO_FRM_MV_INFO *pMvInfo = pHevcDec->pMvInfo;
    s32 nMvTotalSizeX = 0;
    s32 nMvTotalSizeY = 0;

    struct ScMemOpsS *_memops = pHevcDec->pConfig->memops;
    CdcMemFlushCache(_memops, pHf->pMVFrameCol, (pHevcDec->pSps->nCtbSize*160 + 1024));
    logv("gqy*** pHf->pMVFrameCol= %p", pHf->pMVFrameCol);
    HevcMvBytePtr *ptr = (HevcMvBytePtr *)(pHf->pMVFrameCol +  256);

    int ctuSize = 1 << pHevcDec->pSps->nLog2CtbSize;
    logv("***ctuSize = %d", ctuSize);
    int ctuNum = pHevcDec->pSps->nCtbSize;

    logv("ctuNum1 = %d, ctuNum2 = %d", ctuNum, pHevcDec->debug.nCtuNum);

    int i;
    int mvZeroCount = 0;
    int blockNum = 0;
    u64 nQuaNum = 0;
    u64 nMaxMvQuaNum = 0;
    u64 nMinMvQuaNum = 0;
    switch(ctuSize)
    {
     case 16:
        blockNum = 1;
        break;
     case 32:
        blockNum = 4;
        break;
     case 64:
        blockNum = 16;
        break;
     default:
        loge("***the ctuSize is error");
        return;
    }


#if 0
   if(pHevcDec->ncount == 2)
   {
       int j;
       int k=0;

        FILE *fp = fopen("/data/camera/mvdui.dat", "ab");
        for(i=0; i<ctuNum; i++)
        {
           fprintf(fp,"***********************ctu: %d*********************\n", k++);
           for(j=0; j<blockNum; j++)
          {
            fprintf(fp, "block%d list0(%d, %d) , list1(%d, %d)\n",j,
                (s16)(ptr->nBlk0Mvlist0X), (s16)(ptr->nBlk0Mvlist0Y),
                (s16)(ptr->nBlk0Mvlist1X), (s16)(ptr->nBlk0Mvlist1Y));

            j++;
            if(j>blockNum)
               break;
            fprintf(fp, "block%d list0(%d, %d) , list1(%d, %d)\n",j,
                (s16)(ptr->nBlk1Mvlist0X), (s16)(ptr->nBlk1Mvlist0Y),
                (s16)(ptr->nBlk1Mvlist1X), (s16)(ptr->nBlk1Mvlist1Y));

             ptr++;
          }
          fprintf(fp, "\n");
        }
        fclose(fp);
    }
#endif

    pMvInfo->nMaxMv_x = (s16)ptr->nBlk0Mvlist0X;

    pMvInfo->nMaxMv_y = (s16)ptr->nBlk0Mvlist0Y;

    pMvInfo->nMinMv_x = (s16)ptr->nBlk0Mvlist0X;

    pMvInfo->nMinMv_y = (s16)ptr->nBlk0Mvlist0Y;

    nQuaNum = ptr->nBlk0Mvlist0X * ptr->nBlk0Mvlist0X + ptr->nBlk0Mvlist0Y * ptr->nBlk0Mvlist0Y;
    nMaxMvQuaNum = nQuaNum;
    nMinMvQuaNum = nQuaNum;

    for(i=0; i<(ctuNum*blockNum/2); i++)
    {
        int flag1 = 0, flag2 = 0;

       /************************    block0        ******************************/
        //logv("blok0list0x = %d, blok0list0y = %d", ptr->nBlk0Mvlist0X, ptr->nBlk0Mvlist0Y);
        nMvTotalSizeX += (s16)ptr->nBlk0Mvlist0X;
        nMvTotalSizeY += (s16)ptr->nBlk0Mvlist0Y;
        if((s16)ptr->nBlk0Mvlist0X > pMvInfo->nMaxMv_x)
            pMvInfo->nMaxMv_x = (s16)ptr->nBlk0Mvlist0X;
        if((s16)ptr->nBlk0Mvlist0Y > pMvInfo->nMaxMv_y)
            pMvInfo->nMaxMv_y = (s16)ptr->nBlk0Mvlist0Y;
        if((s16)ptr->nBlk0Mvlist0X < pMvInfo->nMinMv_x)
            pMvInfo->nMinMv_x = (s16)ptr->nBlk0Mvlist0X;
        if((s16)ptr->nBlk0Mvlist0Y < pMvInfo->nMinMv_y)
            pMvInfo->nMinMv_y = (s16)ptr->nBlk0Mvlist0Y;
        flag1 = ((s16)ptr->nBlk0Mvlist0X == 0 && (s16)ptr->nBlk0Mvlist0Y == 0)?1:0;
        nQuaNum = ptr->nBlk0Mvlist0X * ptr->nBlk0Mvlist0X +  \
                    ptr->nBlk0Mvlist0Y * ptr->nBlk0Mvlist0Y;
        if(nMaxMvQuaNum < nQuaNum)
            nMaxMvQuaNum = nQuaNum;
        if(nMinMvQuaNum > nQuaNum)
            nMinMvQuaNum = nQuaNum;

        //logv("blok0list1x = %d, blok0list1y = %d",
         //     (s16)ptr->nBlk0Mvlist1X, (s16)ptr->nBlk0Mvlist1Y);
        nMvTotalSizeX += (s16)ptr->nBlk0Mvlist1X;
        nMvTotalSizeY += (s16)ptr->nBlk0Mvlist1Y;
        if((s16)ptr->nBlk0Mvlist1X > pMvInfo->nMaxMv_x)
            pMvInfo->nMaxMv_x = (s16)ptr->nBlk0Mvlist1X;
        if((s16)ptr->nBlk0Mvlist1Y > pMvInfo->nMaxMv_y)
            pMvInfo->nMaxMv_y = (s16)ptr->nBlk0Mvlist1Y;
        if((s16)ptr->nBlk0Mvlist1X < pMvInfo->nMinMv_x)
            pMvInfo->nMinMv_x = (s16)ptr->nBlk0Mvlist1X;
        if((s16)ptr->nBlk0Mvlist1Y < pMvInfo->nMinMv_y)
            pMvInfo->nMinMv_y = (s16)ptr->nBlk0Mvlist1Y;
        flag2 = ((s16)ptr->nBlk0Mvlist1X == 0 && (s16)ptr->nBlk0Mvlist1Y == 0)?1:0;
        if(flag1 && flag2)
            mvZeroCount++;
        nQuaNum = ptr->nBlk0Mvlist1X * ptr->nBlk0Mvlist1X + \
                     ptr->nBlk0Mvlist1Y * ptr->nBlk0Mvlist1Y;
        if(nMaxMvQuaNum < nQuaNum)
            nMaxMvQuaNum = nQuaNum;
        if(nMinMvQuaNum > nQuaNum)
            nMinMvQuaNum = nQuaNum;


        /***********************   blockInfo       ******************************/

        /************************    block1        ******************************/

        //logv("blok1list0x = %d, blok1list0y = %d",
        //     (s16)ptr->nBlk1Mvlist0X, (s16)ptr->nBlk1Mvlist0Y);
        nMvTotalSizeX += (s16)ptr->nBlk1Mvlist0X;
        nMvTotalSizeY += (s16)ptr->nBlk1Mvlist0Y;
        if((s16)ptr->nBlk1Mvlist0X > pMvInfo->nMaxMv_x)
            pMvInfo->nMaxMv_x = (s16)ptr->nBlk1Mvlist0X;
        if((s16)ptr->nBlk1Mvlist0Y > pMvInfo->nMaxMv_y)
            pMvInfo->nMaxMv_y = (s16)ptr->nBlk1Mvlist0Y;
        if((s16)ptr->nBlk1Mvlist0X < pMvInfo->nMinMv_x)
            pMvInfo->nMinMv_x = (s16)ptr->nBlk1Mvlist0X;
        if((s16)ptr->nBlk1Mvlist0Y < pMvInfo->nMinMv_y)
            pMvInfo->nMinMv_y = (s16)ptr->nBlk1Mvlist0Y;
        flag1 = ((s16)ptr->nBlk1Mvlist0X == 0 && (s16)ptr->nBlk1Mvlist0Y == 0)?1:0;
        nQuaNum = ptr->nBlk1Mvlist0X * ptr->nBlk1Mvlist0X + \
                     ptr->nBlk1Mvlist0Y * ptr->nBlk1Mvlist0Y;
        if(nMaxMvQuaNum < nQuaNum)
            nMaxMvQuaNum = nQuaNum;
        if(nMinMvQuaNum > nQuaNum)
            nMinMvQuaNum = nQuaNum;


        //logv("blok1list1x = %d, blok1list1y = %d",
        //     (s16)ptr->nBlk1Mvlist1X, (s16)ptr->nBlk1Mvlist1Y);
        nMvTotalSizeX += (s16)ptr->nBlk1Mvlist1X;
        nMvTotalSizeY += (s16)ptr->nBlk1Mvlist1Y;
        if((s16)ptr->nBlk1Mvlist1X > pMvInfo->nMaxMv_x)
            pMvInfo->nMaxMv_x = (s16)ptr->nBlk1Mvlist1X;
        if((s16)ptr->nBlk1Mvlist1Y > pMvInfo->nMaxMv_y)
            pMvInfo->nMaxMv_y = (s16)ptr->nBlk1Mvlist1Y;
        if((s16)ptr->nBlk1Mvlist1X < pMvInfo->nMinMv_x)
            pMvInfo->nMinMv_x = (s16)ptr->nBlk1Mvlist1X;
        if((s16)ptr->nBlk1Mvlist1Y < pMvInfo->nMinMv_y)
            pMvInfo->nMinMv_y = (s16)ptr->nBlk1Mvlist1Y;
        flag2 = ((s16)ptr->nBlk1Mvlist1X == 0 && (s16)ptr->nBlk1Mvlist1Y == 0)?1:0;
        if(flag1 && flag2)
            mvZeroCount++;
        nQuaNum = ptr->nBlk1Mvlist1X * ptr->nBlk1Mvlist1X + \
                     ptr->nBlk1Mvlist1X * ptr->nBlk1Mvlist1X;
        if(nMaxMvQuaNum < nQuaNum)
            nMaxMvQuaNum = nQuaNum;
        if(nMinMvQuaNum > nQuaNum)
            nMinMvQuaNum = nQuaNum;


        ptr++;
    }

    pMvInfo->nAvgMv_x = nMvTotalSizeX/(ctuNum* blockNum * 2);
    pMvInfo->nAvgMv_y = nMvTotalSizeY/(ctuNum* blockNum * 2);
    pMvInfo->nMaxMv = sqrt(nMaxMvQuaNum);
    pMvInfo->nMinMv = sqrt(nMinMvQuaNum);
    nQuaNum = pMvInfo->nAvgMv_x * pMvInfo->nAvgMv_x + pMvInfo->nAvgMv_y * pMvInfo->nAvgMv_y;
    pMvInfo->nAvgMv   = sqrt(nQuaNum);
    pMvInfo->SkipRatio = mvZeroCount * 100 /(ctuNum*blockNum);
    //logd("gqy*** nAvgMv_x = %d, nAvgMv_y= %d, nMaxMv_x = %d,
          //nMinMv_x = %d, nMaxMv_y = %d, nMinMv_y = %d, SkipRatio = %d",
    //  pMvInfo->nAvgMv_x, pMvInfo->nAvgMv_y, pMvInfo->nMaxMv_x,
    //  pMvInfo->nMinMv_x, pMvInfo->nMaxMv_y, pMvInfo->nMinMv_y, pMvInfo->SkipRatio);
    if(pHevcDec->ControlInfo.secOutputEnabled)
        memcpy(&pHf->pSecFbmBuffer->nCurFrameInfo.nMvInfo, pMvInfo, sizeof(VIDEO_FRM_MV_INFO));
    else
        memcpy(&pHf->pFbmBuffer->nCurFrameInfo.nMvInfo, pMvInfo, sizeof(VIDEO_FRM_MV_INFO));

}

static void HevcSavePicInfo(HevcContex *pHevcDec)
{
    double nFrameDuration;
    double nVidFrmSize;
    double nFrameRate;
    if(pHevcDec->pCurrDPB == NULL)
    {
        loge("***pCurrDPB is NULL");
        return;
    }
    HevcFrame *pHf = pHevcDec->pCurrDPB;
    if(pHevcDec->ControlInfo.secOutputEnabled == 0)
    {
        if(pHevcDec->sliceType == HEVC_I_SLICE)
            pHf->pFbmBuffer->nCurFrameInfo.enVidFrmType = VIDEO_FORMAT_TYPE_I;
        else if(HEVC_IS_IDR(pHevcDec->eNaluType))
            pHf->pFbmBuffer->nCurFrameInfo.enVidFrmType = VIDEO_FORMAT_TYPE_IDR;
        else if(pHevcDec->sliceType == HEVC_B_SLICE)
            pHf->pFbmBuffer->nCurFrameInfo.enVidFrmType = VIDEO_FORMAT_TYPE_B;
        else if(pHevcDec->sliceType == HEVC_P_SLICE)
            pHf->pFbmBuffer->nCurFrameInfo.enVidFrmType = VIDEO_FORMAT_TYPE_P;

        logv("gqy*** enVidfrmType = %d", pHf->pFbmBuffer->nCurFrameInfo.enVidFrmType);
        pHf->pFbmBuffer->nCurFrameInfo.nVidFrmSize = pHevcDec->nCurFrameStreamSize;
        if(pHevcDec->pVideoStreamInfo->nFrameRate)
        {
            pHf->pFbmBuffer->nCurFrameInfo.nFrameRate =
                    (double)pHevcDec->pVideoStreamInfo->nFrameRate/(double)1000.0;
        }
        else
        {
            if(pHevcDec->pVideoStreamInfo->nFrameDuration)
            {
                nFrameDuration = (double)pHevcDec->pVideoStreamInfo->nFrameDuration;
            }
            else
            {
                nFrameDuration = (double)33333;
            }
            logv("the_444,nFrameDuration=%0.2lf",nFrameDuration);
            pHf->pFbmBuffer->nCurFrameInfo.nFrameRate = (double)(1000*1000)/nFrameDuration;
        }
        nVidFrmSize = (double)pHf->pFbmBuffer->nCurFrameInfo.nVidFrmSize;
        nFrameRate  = pHf->pFbmBuffer->nCurFrameInfo.nFrameRate;
        pHf->pFbmBuffer->nCurFrameInfo.nAverBitRate = nVidFrmSize*8*1000/nFrameRate;
        logv("the_265,nFrameRate=%0.2lf,nAverBitRate=%0.2lf",
            pHf->pFbmBuffer->nCurFrameInfo.nFrameRate,pHf->pFbmBuffer->nCurFrameInfo.nAverBitRate);
        if(pHevcDec->bDropPreFrameFlag)
        {
            pHf->pFbmBuffer->nCurFrameInfo.bDropPreFrame = 1;
            pHevcDec->bDropPreFrameFlag = 0;
        }
    }
    else
    {
        if(pHevcDec->sliceType == HEVC_I_SLICE)
            pHf->pSecFbmBuffer->nCurFrameInfo.enVidFrmType = VIDEO_FORMAT_TYPE_I;
        else if(HEVC_IS_IDR(pHevcDec->eNaluType))
            pHf->pSecFbmBuffer->nCurFrameInfo.enVidFrmType = VIDEO_FORMAT_TYPE_IDR;
        else if(pHevcDec->sliceType == HEVC_B_SLICE)
            pHf->pSecFbmBuffer->nCurFrameInfo.enVidFrmType = VIDEO_FORMAT_TYPE_B;
        else if(pHevcDec->sliceType == HEVC_P_SLICE)
            pHf->pSecFbmBuffer->nCurFrameInfo.enVidFrmType = VIDEO_FORMAT_TYPE_P;
        pHf->pSecFbmBuffer->nCurFrameInfo.nVidFrmSize = pHevcDec->nCurFrameStreamSize;
        if(pHevcDec->bDropPreFrameFlag)
        {
            pHf->pSecFbmBuffer->nCurFrameInfo.bDropPreFrame = 1;
            pHevcDec->bDropPreFrameFlag = 0;
        }
    }
    if(pHevcDec->bEnableIptvFlag)
        HevcCaculateMvValue(pHevcDec);

}

static HevcDecodeFrameResult DecoderDecodeOneNalu(HevcContex *pHevcDec, s32 nNaluIndex)
{
    char *pBuf;
    s32 i, nBufSize, bFlag;
    s32 nRet = HEVC_RESULT_OK;
    HevcDecodeFrameResult eReturnValue = HEVC_RESULT_OK;
    s32 bVeInterruptErrorFlag = 0;
    HevcNaluType eNaluType;
    HevcStreamInfo *pStreamInfo = pHevcDec->pStreamInfo;
    FramePicInfo* pStreamFramePic = NULL;
    NaluInfo* pNaluInfo = NULL;
    s32 bISLocalBufFlag = 0;

    pStreamInfo->nBufIndex = nNaluIndex;
    if(pStreamInfo->pSbm->nType == SBM_TYPE_FRAME)
    {
        pStreamFramePic = pHevcDec->pStreamInfo->pCurStreamFrame;
        pNaluInfo = &pStreamFramePic->pNaluInfoList[nNaluIndex];
        eNaluType = pNaluInfo->nType;
        pBuf = pNaluInfo->pDataBuf;
        bISLocalBufFlag = 0;
    }
    else
    {
        eNaluType = pStreamInfo->eNaluTypeList[nNaluIndex];
        pBuf = pStreamInfo->pDataBuf[nNaluIndex];
        bISLocalBufFlag = pStreamInfo->bIsLocalBuf[nNaluIndex];
    }

    if(bISLocalBufFlag)
    {
        pStreamInfo->pCurrentBufPhy = pStreamInfo->pLocalBufPhy;
        pStreamInfo->pCurrentBufEndPhy = pStreamInfo->pLocalBufEndPhy;
        pStreamInfo->nBitsOffset = (pBuf - pStreamInfo->pLocalBuf) * 8;
        logv("DecoderDecodeOneNalu() use local buffer.  ");
    }
    else
    {
        pStreamInfo->pCurrentBufPhy = pStreamInfo->pSbmBufPhy;
        pStreamInfo->pCurrentBufEndPhy = pStreamInfo->pSbmBufEndPhy;
        pStreamInfo->nBitsOffset = (pBuf - pStreamInfo->pSbmBuf) * 8;
    }
#if HEVC_BIT_RATE_INFO
    pHevcDec->debug.nCurrentBit += (pStreamInfo->nDataSize[nNaluIndex] << 3);
#endif

#if 0
    if(1)
    {
        s32 n;
        char *p;
        logd(" 000000  pStreamInfo->nBitsOffset: %d ", pStreamInfo->nBitsOffset);
        logd(" buf addr: %x, buf size: %d ", (size_addr)pBuf, pStreamInfo->nDataSize[nNaluIndex]);
        n = pStreamInfo->nBitsOffset/8;
        p = pStreamInfo->pSbmBuf;
        logd("sbm data: index: %d , data: %2.2x, %2.2x, %2.2x, %2.2x, %2.2x, %2.2x, %2.2x, %2.2x",
                n,p[n],p[n+1],p[n+2],p[n+3],p[n+4],p[n+5],p[n+6],p[n+7]);
        for(i = 0; i < 4; i++)
        {
            n += 8;
            logd("sbm data: index: %d , \
                    data: %2.2x, %2.2x, %2.2x, %2.2x, %2.2x, %2.2x, %2.2x, %2.2x",
                    n,p[n],p[n+1],p[n+2],p[n+3],p[n+4],p[n+5],p[n+6],p[n+7]);
        }
        logd("");
        p = pBuf;
        n = 0;
        logd("buf data: data: %2.2x, %2.2x, %2.2x, %2.2x, %2.2x, %2.2x, %2.2x, %2.2x",
                p[n],p[n+1],p[n+2],p[n+3],p[n+4],p[n+5],p[n+6],p[n+7]);
        n += 8;
        logd("buf data: data: %2.2x, %2.2x, %2.2x, %2.2x, %2.2x, %2.2x, %2.2x, %2.2x",
                p[n],p[n+1],p[n+2],p[n+3],p[n+4],p[n+5],p[n+6],p[n+7]);
        logd("");
    }

#endif
#if HEVC_HW_DECODE_SLICE_HEADER
    if(eNaluType <= HEVC_NAL_CRA_NUT)
        pHevcDec->bHardwareGetBitsFlag = 1;
    /* if this picture slice, use hardware decode */
    else
        pHevcDec->bHardwareGetBitsFlag = 0;
#else
    pHevcDec->bHardwareGetBitsFlag = 0;
#endif

    logv("pHevcDec->bHardwareGetBitsFlag = %d",pHevcDec->bHardwareGetBitsFlag);

    if(pHevcDec->bHardwareGetBitsFlag)
    {
        pHevcDec->getBit.bUseHardware = 1;
        pHevcDec->getBit.RegBaseAddr = pHevcDec->HevcDecRegBaseAddr;
        HevcInitialHwStreamDecode(pHevcDec);
//        logd("---------------  hardware decod nalu  ------------------ ");
    }
    else
    {
        pHevcDec->getBit.bUseHardware = 0;
        HevcGetPreprocessData(pHevcDec, nNaluIndex);
        pBuf = pStreamInfo->ShByte;
        nBufSize = pStreamInfo->nShByteSize;
//        logd(" --------decoder nalu, type: %d, size: %d, idx: %d ",
//             eNaluType, nBufSize, nNaluIndex);
        nRet = HevcParserInitGetBits(&pHevcDec->getBit,
                pStreamInfo->ShByte, pStreamInfo->nShByteSize * 8);
//        logd("   ---   ---  software decod nalu  -----   -----    ");
        if(nRet < 0)
        {
            if(eNaluType >= HEVC_NAL_AUD || nBufSize <= 0)
            {
                return HEVC_RESULT_OK;
            }
            else
            {
                logw(" h265 decoder initial Get Bits error. \
                      nalu type: %d,data size: %d, idx: %d", eNaluType, nBufSize, nNaluIndex);
                goto HevcDecodeNaluErrorExit;
            }
        }
    }

    nRet = HevcParserGetNaluType(pHevcDec);
    if(nRet < 0)
    {
        //char *p = pNaluInfo->pDataBuf;
        loge(" h265 decoder get nalu type error. parser nalu type: %d", eNaluType);
        //logd(" current nalu data: %x, %x, %x, %x,  %x, %x, %x, %x ",
        //        p[0],p[1],p[2],p[3],  p[4],p[5],p[6],p[7]);
        goto HevcDecodeNaluErrorExit;
    }

    //*re init values of poc list
    if(HEVC_IS_IDR(eNaluType))
    {
        for(i = 0; i < HEVC_MAX_FRAMES_ONE_SQUEUE; i++)
        {
            pHevcDec->nSqueueFramePocList[i] = HEVC_POC_INIT_VALUE;
        }
    }
    switch(pHevcDec->eNaluType)
    {
    case HEVC_NAL_VPS:
        nRet = HevcDecodeNalVps(pHevcDec);
        if(nRet < 0)
        {
            loge(" h265 decoder decode nal vps error.");
            goto HevcDecodeNaluErrorExit;
        }
        else if(nRet == HEVC_RESULT_PICTURES_SIZE_CHANGE)
            eReturnValue = HEVC_RESULT_PICTURES_SIZE_CHANGE;
        else
            eReturnValue = HEVC_RESULT_OK;
        break;
    case HEVC_NAL_SPS:
        nRet = HevcDecodeNalSps(pHevcDec);
        if(nRet < 0)
        {
            loge(" h265 decoder decode nal sps error.");
            goto HevcDecodeNaluErrorExit;
        }
        else if(nRet == HEVC_RESULT_PICTURES_SIZE_CHANGE)
            eReturnValue = HEVC_RESULT_PICTURES_SIZE_CHANGE;
        else
            eReturnValue = HEVC_RESULT_OK;

        break;
    case HEVC_NAL_PPS:
        nRet = HevcDecodeNalPps(pHevcDec);
        if(nRet < 0)
        {
            loge(" h265 decoder decode nal pps error.");
            goto HevcDecodeNaluErrorExit;
        }
        eReturnValue = HEVC_RESULT_OK;
        break;
    case HEVC_NAL_SEI_PREFIX:
    case HEVC_NAL_SEI_SUFFIX:
        HevcDecodeNalSei(pHevcDec);
        eReturnValue = HEVC_RESULT_OK;
        break;
    case HEVC_NAL_TRAIL_N:
    case HEVC_NAL_TRAIL_R:
    case HEVC_NAL_TSA_N:
    case HEVC_NAL_TSA_R:
    case HEVC_NAL_STSA_N:
    case HEVC_NAL_STSA_R:
    case HEVC_NAL_BLA_W_LP:
    case HEVC_NAL_BLA_W_RADL:
    case HEVC_NAL_BLA_N_LP:
    case HEVC_NAL_IDR_W_RADL:
    case HEVC_NAL_IDR_N_LP:
    case HEVC_NAL_CRA_NUT:
    case HEVC_NAL_RADL_N:
    case HEVC_NAL_RADL_R:
    case HEVC_NAL_RASL_N:
    case HEVC_NAL_RASL_R:
        //* drop the slice stream data when had no sps,pps. no need decode it
        if(pHevcDec->bHadFindSpsFlag == HEVC_FALSE
           || pHevcDec->bHadFindPpsFlag == HEVC_FALSE)
        {
            HevcDecoderDeleteOnePtsFromList(pHevcDec, pHevcDec->nLastPts);
            return HEVC_RESULT_DROP_THE_FRAME;
        }

        nRet = HevcDecodeSliceHeader(pHevcDec);
        logv(" after slice header, current frame poc = %d, type: %d",
            pHevcDec->nPoc, pHevcDec->eNaluType);
        if(nRet == HEVC_RESULT_PICTURES_SIZE_CHANGE)
        {
            return HEVC_RESULT_PICTURES_SIZE_CHANGE;
        }
        else if(nRet < 0)
        {
            //* we should decode continue even if slice_header_error,
            //* becase application maybe need display error frame.
            logw(" h265 decoder decode nal slice header error.");
            pHevcDec->bErrorFrameFlag = 1;
        }

        if(pHevcDec->nMaxRa == HEVC_INT_MAX)
        {
            if(pHevcDec->eNaluType == HEVC_NAL_CRA_NUT ||HEVC_IS_BLA(pHevcDec))
                pHevcDec->nMaxRa = pHevcDec->nPoc;
            else
                if(HEVC_IS_IDR(pHevcDec->eNaluType))
                    pHevcDec->nMaxRa = HEVC_INT_MINI;
        }
        if((pHevcDec->eNaluType == HEVC_NAL_RASL_R ||
            pHevcDec->eNaluType == HEVC_NAL_RASL_N) &&
                pHevcDec->nPoc <= pHevcDec->nMaxRa)
        {
            logd(" no need to decode poc: %d, md5 frame: %d ",
                pHevcDec->nPoc, pHevcDec->nMd5Frame);
            HevcDecoderDeleteOnePtsFromList(pHevcDec, pHevcDec->nLastPts);
            return HEVC_RESULT_DROP_THE_FRAME;
        }
        else
        {
            if(pHevcDec->eNaluType == HEVC_NAL_RASL_R &&
                pHevcDec->nPoc > pHevcDec->nMaxRa)
                pHevcDec->nMaxRa = HEVC_INT_MINI;
        }
        logv("bFirstSliceInPicFlag = %d, sliceType = %d",
              pHevcDec->SliceHeader.bFirstSliceInPicFlag, pHevcDec->SliceHeader.eSliceType);
        if(pHevcDec->SliceHeader.bFirstSliceInPicFlag)
        {
            nRet = HevcFrameStart(pHevcDec);
            if(nRet == HEVC_RESULT_DROP_THE_FRAME)
            {
                HevcDecoderDeleteOnePtsFromList(pHevcDec, pHevcDec->nLastPts);
                return HEVC_RESULT_DROP_THE_FRAME;
            }
            else if(nRet == HEVC_RESULT_NO_FRAME_BUFFER
                    || nRet == HEVC_RESULT_RESET_INTRA_DECODER
                    || nRet == HEVC_RESULT_INIT_FBM_FAILED)
            {
                return (HevcDecodeFrameResult)nRet;
            }
            else if(nRet < 0)
            {
                loge(" h265 decode frame start error ");
                goto HevcDecodeNaluErrorExit;
            }
        }
        else if(pHevcDec->pCurrDPB == NULL)
        {
            loge(" h265 error.  current frame first slice is missing ");
            goto HevcDecodeNaluErrorExit;
        }

        if(pHevcDec->eNaluType != pHevcDec->eFirstNaluType)
        {
            loge(" h265 error. nal type change (between slices) in the same frame ");
            goto HevcDecodeNaluErrorExit;
        }

        if(!pHevcDec->SliceHeader.bDependentSliceSegmentFlag &&
                pHevcDec->SliceHeader.eSliceType != HEVC_I_SLICE)
        {
            nRet = HevcSliceRps(pHevcDec);
            if(nRet < 0)
            {
                loge(" h265 error. construct current slice ref list error ");
                pHevcDec->bErrorFrameFlag = 1;
                //goto HevcDecodeNaluErrorExit;
            }
        }
        /* decode slice hardware */

        if(pHevcDec->pCurrDPB->bDropFlag == 1)
        {
            logv(" Drop one slice poc: %d ", pHevcDec);
            //* this case means not decode the delay B-frame, but the frame still
            //* in the DPB, so do not call HevcDecoderDeleteOnePtsFromList here.
            return HEVC_RESULT_DROP_THE_FRAME;
        }

     #if HEVC_ENABLE_CATCH_DDR
        u32 value_1=0;
        CdcVeWriteValue(pHevcDec->pConfig->veOpsS,         \
             pHevcDec->pConfig->pVeOpsSelf, (((0x9C>>2)<<16) | 0x03));
     #endif
        HevcDecodeOneSlice(pHevcDec);
        //usleep(50*1000);
     #if HEVC_ENABLE_CATCH_DDR
        s64 HW_time1 = HevcGetCurrentTime();
     #endif

        nRet = CdcVeWaitInterrupt(pHevcDec->pConfig->veOpsS, pHevcDec->pConfig->pVeOpsSelf);

     #if HEVC_ENABLE_CATCH_DDR
        s64 HW_time2 = HevcGetCurrentTime();
        s64 HW_waste = (HW_time2-HW_time1)/1000;
     #endif
        logv(" CdcVeWaitInterrupt() ret: %d ", nRet);
        if(nRet < 0)
        {
            loge(" h265 wait ve interrupt error ");
            bVeInterruptErrorFlag = 1;
        }
      #if HEVC_ENABLE_CATCH_DDR
        value_1 = CdcVeReadValue(pHevcDec->pConfig->veOpsS, pHevcDec->pConfig->pVeOpsSelf,0xA8>>2);
        pHevcDec->nCurFrameHWTime += (u32)HW_waste;
        pHevcDec->nCurFrameBW += value_1 - pHevcDec->nLastFrameBW;
        pHevcDec->nLastFrameBW = value_1;
     #endif

        nRet = HevcVeStatusInfo(pHevcDec, bVeInterruptErrorFlag);
        logv("ve status: ret = %d, poc = %d",nRet,pHevcDec->nPoc);
        if(nRet < 0)
        {
            loge(" h265 decode one slice hardware error ");
            eReturnValue = HEVC_RESULT_NULA_DATA_ERROR;
        }
#if HEVC_ENABLE_MD5_CHECK
        pHevcDec->bMd5AvailableFlag |= HEVC_FRAME_CONSTRUCTED;
#endif //HEVC_ENABLE_MD5_CHECK

#if HEVC_SAVE_SEC_YUV_PIC
        if(pHevcDec->debug.nHwFrameNum <= 60)
        {
            HevcSaveSecYuvData(pHevcDec);
        }
#endif //HEVC_SAVE_SEC_YUV_PIC

#if HEVC_DECODE_TIME_INFO
        DecoderDebugCaculateTimeInfo(pHevcDec);
#endif    //HEVC_DECODE_TIME_INFO
        break;
    case HEVC_NAL_EOS_NUT:
    case HEVC_NAL_EOB_NUT:
        pHevcDec->nSeqDecode = (pHevcDec->nSeqDecode + 1) & 0xff;
        pHevcDec->nMaxRa = HEVC_INT_MAX;
        eReturnValue = HEVC_RESULT_OK;
        break;
    case HEVC_NAL_AUD:
    case HEVC_NAL_FD_NUT:
        eReturnValue = HEVC_RESULT_OK;
        break;
    default:
        logv(" skipped nalu");
    }
//    logd(" end of DecoderDecodeOneNalu() ");
    return eReturnValue;
HevcDecodeNaluErrorExit:
    return HEVC_RESULT_NULA_DATA_ERROR;
}

HevcDecodeFrameResult HevcDecDecodeOneFrame(HevcContex *pHevcDec, s32 bDecodeKeyFrameOnly)
{
    s32 i;
    HevcDecodeFrameResult eReturnValue = HEVC_RESULT_OK;
    int bSkipNonKeyFrame = 0;

    pHevcDec->pCurrDPB = NULL;
    pHevcDec->bErrorFrameFlag = 0;
    pHevcDec->sliceType = -1;
#if HEVC_BIT_RATE_INFO
    if(pHevcDec->debug.nBitRateTime == 0)
        pHevcDec->debug.nBitRateTime = HevcGetCurrentTime();
#endif
#if HEVC_SAVE_DECODER_DATA
    HevcSaveDecoderSliceData(pHevcDec);
#endif //HEVC_SAVE_DECODER_DATA
    logv(" HevcDecDecodeOneFrame(). start.  buf number: %d, md5 frame: %d",
            pHevcDec->pStreamInfo->nCurrentDataBufIdx, pHevcDec->nMd5Frame);
    s32 nNaluNum = 0;
    FramePicInfo* pStreamFramePic = pHevcDec->pStreamInfo->pCurStreamFrame;

    if(pHevcDec->pStreamInfo->pSbm->nType == SBM_TYPE_FRAME)
    {
        nNaluNum = pStreamFramePic->nCurNaluIdx;
    }
    else
    {
        nNaluNum = pHevcDec->pStreamInfo->nCurrentDataBufIdx;
    }
    logv("*** ntype = %d, num = %d",pHevcDec->pStreamInfo->pSbm->nType, nNaluNum);

    if(bDecodeKeyFrameOnly || pHevcDec->pStreamInfo->bFindKeyFrameOnly)
        bSkipNonKeyFrame = 1;

    logv("*** bSkipNonKeyFrame = %d, %d, %d, nNaluNum = %d",bSkipNonKeyFrame,
         bDecodeKeyFrameOnly, pHevcDec->pStreamInfo->bFindKeyFrameOnly, nNaluNum);
    for(i = 0; i < nNaluNum; i++)
    {
        HevcNaluType eCurNaluType;
        s32          nNaluDataSize;
        if(pHevcDec->pStreamInfo->pSbm->nType == SBM_TYPE_FRAME)
        {
            eCurNaluType = pStreamFramePic->pNaluInfoList[i].nType;
            nNaluDataSize = pStreamFramePic->pNaluInfoList[i].nDataSize;
        }
        else
        {
            eCurNaluType  =  pHevcDec->pStreamInfo->eNaluTypeList[i];
            nNaluDataSize = pHevcDec->pStreamInfo->nDataSize[i];
        }

        logv("*** eCurNaluType = %d, i = %d, num = %d",
            eCurNaluType, i, nNaluNum);
        if(eCurNaluType == HEVC_NAL_IDR_W_RADL
           || eCurNaluType == HEVC_NAL_IDR_N_LP || eCurNaluType == HEVC_NAL_CRA_NUT)
        {
            //* this is nalu of key frame
            if(pHevcDec->pStreamInfo->bFindKeyFrameOnly)
                pHevcDec->pStreamInfo->bFindKeyFrameOnly = 0;
        }
        else if(eCurNaluType < HEVC_NAL_IDR_W_RADL && bSkipNonKeyFrame)
        {
            logv("skip the non-keyframe, flag: %d, %d, naluType = %d",
                  bDecodeKeyFrameOnly, pHevcDec->pStreamInfo->bFindKeyFrameOnly,
                  eCurNaluType);
            HevcDecoderDeleteOnePtsFromList(pHevcDec, pHevcDec->nLastPts);
            continue;
        }

        if(eCurNaluType <= 21)
        {
            logv("    buf: %d, size: %d, nalu type: %d ",
                    i, nNaluDataSize,
                    eCurNaluType);
        }
        if(nNaluDataSize > 10*1024*1024
            || eCurNaluType > HEVC_UNSPEC63)
        {
            loge("h265 decode nalu error. nalu type: %d, size: %d, index: %d",
                    eCurNaluType,nNaluDataSize, i);
            usleep(1000);
            continue;
        }
        logv("***** eCurNaluType = %d",eCurNaluType);
        eReturnValue = DecoderDecodeOneNalu(pHevcDec, i);

        if(eReturnValue == HEVC_RESULT_PICTURES_SIZE_CHANGE
           || eReturnValue == HEVC_RESULT_NO_FRAME_BUFFER
           || eReturnValue == HEVC_RESULT_RESET_INTRA_DECODER
           || eReturnValue == HEVC_RESULT_DROP_THE_FRAME
           || eReturnValue == HEVC_RESULT_INIT_FBM_FAILED)
        {
            return eReturnValue;
        }
        else if(eReturnValue == HEVC_RESULT_NULA_DATA_ERROR)
        {
            logw("h265 decode nalu error, continue decode next nalu! nalu type: %d, size: %d",
                    eCurNaluType,nNaluDataSize);
            logw("*******pHevcDec->pCurrFrame = %p,i = %d", pHevcDec->pCurrFrame, i);
        }
    }

    if(pHevcDec->pSps && pHevcDec->debug.nCtuNum < pHevcDec->pSps->nCtbSize)
    {
        logw(" ctuNum not rignt: %d, %d", pHevcDec->debug.nCtuNum, pHevcDec->pSps->nCtbSize);
        pHevcDec->bErrorFrameFlag = 1;
    }

    if(pHevcDec->pCurrDPB && pHevcDec->bErrorFrameFlag == 1)
    {

        pHevcDec->pCurrDPB->bErrorFrameFlag = 1;

    }
    if(pHevcDec->pCurrDPB)
    {
        //* save poc in current squeue poc list
        s32 nMinPoc = -1;
        s32 nMinPocIndex = 0;
        for(i = 0; i < HEVC_MAX_FRAMES_ONE_SQUEUE; i++)
        {
            if(pHevcDec->nSqueueFramePocList[i] < nMinPoc || nMinPoc == -1)
            {
                nMinPoc = pHevcDec->nSqueueFramePocList[i];
                nMinPocIndex = i;
            }
            if(pHevcDec->nSqueueFramePocList[i] == HEVC_POC_INIT_VALUE)
                break;
        }

        if(i < HEVC_MAX_FRAMES_ONE_SQUEUE)
            pHevcDec->nSqueueFramePocList[i] = pHevcDec->pCurrDPB->nPoc;
        else
            pHevcDec->nSqueueFramePocList[nMinPocIndex] = pHevcDec->pCurrDPB->nPoc;
        logv(" decode one frame : poc = %d, errorFlag = %d",
               pHevcDec->pCurrDPB->nPoc, pHevcDec->pCurrDPB->bErrorFrameFlag);
        HevcSavePicInfo(pHevcDec);

    }
#if HEVC_ENABLE_MD5_CHECK
    if((pHevcDec->bMd5AvailableFlag & HEVC_MD5_CHECK) == HEVC_MD5_CHECK)
    {
        if(pHevcDec->nMd5Frame == 0)
        {
            logd(" MD5 check enable .................... ");
        }
        HevcCaculatePictureMd5(pHevcDec);
        HevcDecodeCheckMd5(pHevcDec);
        pHevcDec->bMd5AvailableFlag = 0;
    }
#else
    pHevcDec->bMd5AvailableFlag = 0;
#endif // HEVC_ENABLE_MD5_CHECK

#if HEVC_SAVE_YUV_PIC
        if(pHevcDec->nMd5Frame <= 30)
            HevcSaveYuvData(pHevcDec);
#endif //HEVC_SAVE_YUV_PIC

#if HEVC_BIT_RATE_INFO
    if(1)
    {
        HevcDebug *debug = &pHevcDec->debug;
        debug->nBitFrameNum += 1;
        if(debug->nBitFrameNum >= HEVC_FRAME_DURATION)
        {
            s64 nBitRate, nAvgBitRate;
            s64 nTime = HevcGetCurrentTime() - debug->nBitRateTime;
            debug->nBitRateTotalTime += nTime;
            debug->nTotalBit += debug->nCurrentBit;
            nAvgBitRate =
                (debug->nTotalBit * 1000*1000) / (debug->nBitRateTotalTime * 1024);
            nBitRate = (debug->nCurrentBit * 1000*1000) / (nTime * 1024);
            logd(" fps HEVC Bit Rate Information. \
                current bit rate:  %lld kbps, average bitRate: %lld kbps ",
                nBitRate, nAvgBitRate);
            debug->nCurrentBit = 0;
            debug->nBitFrameNum = 0;
            debug->nBitRateTime = HevcGetCurrentTime();
        }
    }

#endif
#if(HEVC_ENABLE_SHA_CHECK)
    HevcCaculatePictureSha(pHevcDec);
#endif
    return eReturnValue;
}

void HevcDecOutputAllPictures(HevcContex *pHevcDec)
{
    s32 nRet = 1;
    while(nRet)
    {
        nRet = HevcOutputFrame(pHevcDec, 1);
        usleep(100); /* todo: maybe no need */
    }

}

static s32 HevcCheckIsNalu(char *pBuf, s32 nSize)
{
    s32 ret = 0;
    if(nSize <= 2)
       return ret;
    s32 nForbiddenBit = pBuf[0] >> 7; //* read 1 bits
    s32 naluType      = (pBuf[0] & 0x7e) >> 1; //*read 6 bits
    s32 nTemporalId   = pBuf[1] & 0x7;//* read 3 bits
    if(nTemporalId >= 1 && nForbiddenBit == 0 && (naluType >= 0 && naluType <= 40))
    {
        ret = 1;
    }
    return ret;
}

static s32 HevcParseExtraDataDeleteEmulationCode(char *pBuf, s32 nSize)
{
    s32 i;
    s32 nSkipped = 0;
    s32 nTemp = -1;
    const u32 mask = 0xFFFFFF;
    for(i = 0; i < nSize; i++)
    {
        nTemp = (nTemp << 8) | pBuf[i];
        switch(nTemp & mask)
        {
        case HEVC_EMULATION_CODE:
            nSkipped += 1;
            break;
        default:
            if(nSkipped > 0)
                pBuf[i - nSkipped] = pBuf[i];
        }
    }
    return     (i - nSkipped);
}

static s32 HevcParseExtraDataNalu(HevcContex *pHevcDec, char *pBuf, s32 nDataSize)
{
    s32 nDataTrunkLen, nRet;

#if HEVC_SAVE_DECODER_DATA
    pHevcDec->pStreamInfo->nCurrentDataBufIdx = 1;
    pHevcDec->pStreamInfo->pDataBuf[0] = pBuf;
    pHevcDec->pStreamInfo->nDataSize[0] = nDataSize;
    pHevcDec->pStreamInfo->bHasTwoDataTrunk[0] = 0;
    HevcSaveDecoderSliceData(pHevcDec);
#endif //HEVC_SAVE_DECODER_DATA

    nDataTrunkLen = HevcParseExtraDataDeleteEmulationCode(pBuf, nDataSize);
    HevcParserInitGetBits(&pHevcDec->getBit, pBuf, nDataTrunkLen * 8);
    nRet = HevcParserGetNaluType(pHevcDec);
    switch(pHevcDec->eNaluType)
    {
    case HEVC_NAL_VPS:
        nRet = HevcDecodeNalVps(pHevcDec);
        if(nRet < 0)
        {
            loge(" h265 decoder decode nal vps error. Extra Data");
            return nRet;
        }
        break;
    case HEVC_NAL_SPS:
        nRet = HevcDecodeNalSps(pHevcDec);
        if(nRet < 0)
        {
            loge(" h265 decoder decode nal sps error. Extra Data");
            return nRet;
        }
        break;
    case HEVC_NAL_PPS:
        nRet = HevcDecodeNalPps(pHevcDec);
        if(nRet < 0)
        {
            loge(" h265 decoder decode nal pps error. Extra Data");
            return nRet;
        }
        break;
    default:
        break;
    }
    return nRet;
}

static s32 HevcParseExtraDataSearchNaluSize(char *pData, s32 nSize)
{
    s32 i, nTemp, nNaluLen;
    s32 mask = 0xffffff;
    nTemp = -1;
    nNaluLen = -1;
    for(i = 0; i < nSize; i++)
    {
        nTemp = (nTemp << 8) | pData[i];
        if((nTemp & mask) == HEVC_START_CODE)
        {
            nNaluLen = i - 3;
            break;
        }
    }
    return nNaluLen;
}

static s32 HevcParseExtraData(HevcContex *pHevcDec, char *pData, s32 nDataSize)
{
    s32 i, nDataTrunkLen, nTemp, nRet, bBufCallocFlag;
    HevcStreamInfo *pSi = pHevcDec->pStreamInfo;
    char *pBuf;
    nRet = 0;
    bBufCallocFlag = 0;
    if(pSi->pLocalBuf == NULL)
    {
        pBuf = HevcCalloc(1, 1024);
        if(pBuf == NULL)
        {
            logd(" HevcParseExtraData calloc fail. ");
            return -1;
        }
        bBufCallocFlag = 1;
    }
    else
    {
        pBuf = pSi->pLocalBuf;
    }
    nTemp = -1;
    for(i = 0; i < nDataSize; )
    {
        nTemp <<= 8;
        nTemp |= pData[i];
        switch(nTemp & 0xffffff)
        {
        HEVC_NOT_ANNEX_B_EXTRA_DATA
            logv("****stream type: without startCode");
            /* a data trunk */
            nDataTrunkLen = (pData[i + 1] << 8) | pData[i + 2];
            memcpy(pBuf, &pData[i + 3], nDataTrunkLen);
            i += 3;
            i += nDataTrunkLen;
            nRet = HevcParseExtraDataNalu(pHevcDec, pBuf, nDataTrunkLen);
            if(nRet < 0)
                return nRet;
            /* nalu's first 4 bytes is nalu lenght */
            pHevcDec->pStreamInfo->bStreamWithStartCode = 0;
            pHevcDec->pStreamInfo->bSearchNextStartCode = 0;
            break;
        case 0x000001:
            logv("****stream type: with startCode");
            /* ts container's extra data starts with 0x000001 */
            i += 1;
            nDataTrunkLen = HevcParseExtraDataSearchNaluSize(&pData[i], nDataSize - i);
            if(nDataTrunkLen < 0)
            {
                if(HevcCheckIsNalu(&pData[i], nDataSize - i))
                {
                    nDataTrunkLen = nDataSize - i;
                }
                else
                {
                    logw("****maybe it is not the stream-packet");
                    continue;
                }
            }
            memcpy(pBuf, &pData[i], nDataTrunkLen);
            i += nDataTrunkLen;
            nRet = HevcParseExtraDataNalu(pHevcDec, pBuf, nDataTrunkLen);
            if(nRet < 0)
                return nRet;
            /* nalu's first 4 bytes is nalu lenght */
            pHevcDec->pStreamInfo->bStreamWithStartCode = 1;
            pHevcDec->pStreamInfo->bSearchNextStartCode = 1;
            /* todo: process ts container's extra data */
            break;
        default:
             logd("****can not find the stream type!");
             i++;
            break;
        }
    }
    if(bBufCallocFlag)
        HevcFree(&pBuf);
    return nRet;
}

s32 HevcDecInitProcessSpecificData(HevcContex *pHevcDec, char *pData, s32 nDataSize)
{
    s32 nTemp;
    HevcStreamInfo *pSi = pHevcDec->pStreamInfo;

    nTemp = (nDataSize + 31) & ~31;
    pSi->pExtraData = HevcCalloc(nTemp, 1);
    if(pSi->pExtraData == NULL)
    {
        loge(" h265 initial. process extra data calloc faile ");
        return -1;
    }
    memcpy(pSi->pExtraData, pData, nDataSize);
    return HevcParseExtraData(pHevcDec, pData, nDataSize);

#if 0
    FILE *fp = fopen("/data/camera/extra.dat", "wb");
    if(fp == NULL)
    {
        logd(" initial data open file error ");
    }
    fwrite(pData, 1, nDataSize, fp);
    fclose(fp);
#endif
    return 0;
}

static s32 checkStreamTypeWithoutStartCode(HevcContex *pHevcDec,
                                           VideoStreamDataInfo *pStream)
{
    HevcStreamInfo *pStreamInfo   = pHevcDec->pStreamInfo;
    const s32 nForbiddenBitValue  = 0;
    const s32 nTemporalIdMinValue = 1;
    char *pBuf = NULL;
    char tmpBuf[6] = {0};
    s32 nTemporalId      = -1;
    s32 nForbiddenBit    = -1;
    s32 nDataSize   = -1;
    s32 nRemainSize = -1;
    s32 nRet = -1;

    s32 nHadProcessLen = 0;
    pBuf = pStream->pData;
    while(nHadProcessLen < pStream->nLength)
    {
        nRemainSize = pStream->nLength-nHadProcessLen;
        tmpBuf[0] = HevcSmbReadByte(pBuf, 0);
        tmpBuf[1] = HevcSmbReadByte(pBuf, 1);
        tmpBuf[2] = HevcSmbReadByte(pBuf, 2);
        tmpBuf[3] = HevcSmbReadByte(pBuf, 3);
        tmpBuf[4] = HevcSmbReadByte(pBuf, 4);
        tmpBuf[5] = HevcSmbReadByte(pBuf, 5);
        nDataSize = (tmpBuf[0] << 24) | (tmpBuf[1] << 16) | (tmpBuf[2] << 8) | tmpBuf[3];
        nForbiddenBit = tmpBuf[4] >> 7; //* read 1 bits
        nTemporalId   = tmpBuf[5] & 0x7;//* read 3 bits
        if(nDataSize > (nRemainSize - 4)
           || nDataSize < 0
           || nTemporalId < nTemporalIdMinValue
           || nForbiddenBit != nForbiddenBitValue)
        {
            logd("check stream type fail: nDataSize[%d], streamSize[%d], nTempId[%d], fobBit[%d]",
                 nDataSize, (pStream->nLength-nHadProcessLen),nTemporalId,nForbiddenBit);
            nRet = -1;
            break;
        }
        logv("*** nDataSize = %d, nRemainSize = %d, proceLen = %d, totalLen = %d",
            nDataSize, nRemainSize,
            nHadProcessLen,pStream->nLength);

        if(nDataSize == (nRemainSize - 4))
        {
            nRet = 0;
            break;
        }

        nHadProcessLen += nDataSize + 4;
        pBuf = pStream->pData + nHadProcessLen;
        if(pBuf - pStreamInfo->pSbmBufEnd >0)
        {
            pBuf = pStreamInfo->pSbmBuf + (pBuf - pStreamInfo->pSbmBufEnd);
        }
    }

    return nRet;
}

s32 HevcCheckBitStreamType(HevcContex *pHevcDec)
{
    HevcStreamInfo *pStreamInfo  = pHevcDec->pStreamInfo;
    VideoStreamDataInfo *pStream = NULL;
    const s32 nTsStreamType       = 0x000001;
    const s32 nForbiddenBitValue  = 0;
    const s32 nTemporalIdMinValue = 1;
    const s32 nUpLimitCount       = 50;
    s32 nReqeustCounter  = 0;
    s32 nCheck4BitsValue = -1;
    s32 nTemporalId      = -1;
    s32 nForbiddenBit    = -1;
    char *pBuf = NULL;
    char tmpBuf[6] = {0};
    s32 nRet = VDECODE_RESULT_NO_BITSTREAM;
    s32 nHadCheckBytesLen = 0;

    while(nReqeustCounter < nUpLimitCount)
    {
        nReqeustCounter++;
        pStream = SbmRequestStream(pStreamInfo->pSbm);
        if(pStream == NULL)
        {
            nRet = VDECODE_RESULT_NO_BITSTREAM;
            break;
        }
        if(pStream->nLength == 0 || pStream->pData == NULL)
        {
            SbmFlushStream(pStreamInfo->pSbm, pStream);
            pStream = NULL;
            continue;
        }
        //*1. process sbm-cycle-buffer case
        pBuf = pStream->pData;

checkBitStreamType_ReadByte:

        if((nHadCheckBytesLen + 6) > pStream->nLength)
        {
            SbmFlushStream(pStreamInfo->pSbm, pStream);
            pStream = NULL;
            continue;
        }
        tmpBuf[0] = HevcSmbReadByte(pBuf, nHadCheckBytesLen + 0);
        tmpBuf[1] = HevcSmbReadByte(pBuf, nHadCheckBytesLen + 1);
        tmpBuf[2] = HevcSmbReadByte(pBuf, nHadCheckBytesLen + 2);
        tmpBuf[3] = HevcSmbReadByte(pBuf, nHadCheckBytesLen + 3);
        tmpBuf[4] = HevcSmbReadByte(pBuf, nHadCheckBytesLen + 4);
        tmpBuf[5] = HevcSmbReadByte(pBuf, nHadCheckBytesLen + 5);

        nCheck4BitsValue = (tmpBuf[0] << 24) | (tmpBuf[1] << 16) | (tmpBuf[2] << 8) | tmpBuf[3];
        if(nCheck4BitsValue == 0) //*compatible for the case: 00 00 00 00 00 00 00 01
        {
            nHadCheckBytesLen++;
            goto checkBitStreamType_ReadByte;
        }

        if(nCheck4BitsValue == nTsStreamType)
        {
            nForbiddenBit = tmpBuf[4] >> 7; //* read 1 bits
            nTemporalId   = tmpBuf[5] & 0x7;//* read 3 bits
            if(nTemporalId >= nTemporalIdMinValue && nForbiddenBit == nForbiddenBitValue)
            {
                pStreamInfo->bStreamWithStartCode = 1;
            }
            else
            {
                nHadCheckBytesLen += 4;
                goto checkBitStreamType_ReadByte;
            }
        }
        else if((nCheck4BitsValue >> 8) == nTsStreamType)
        {
            nForbiddenBit = tmpBuf[3] >> 7; //* read 1 bits
            nTemporalId   = tmpBuf[4] & 0x7;//* read 3 bits
            if(nTemporalId >= nTemporalIdMinValue && nForbiddenBit == nForbiddenBitValue)
            {
                pStreamInfo->bStreamWithStartCode = 1;
            }
            else
            {
                nHadCheckBytesLen += 3;
                goto checkBitStreamType_ReadByte;
            }

        }
        else if(nCheck4BitsValue < pStream->nLength)
        {
            s32 nRetTmp = checkStreamTypeWithoutStartCode(pHevcDec, pStream);
            if(nRetTmp == 0)
            {
                pStreamInfo->bStreamWithStartCode = 0;
            }
            else
            {
                nHadCheckBytesLen += 4;
                goto checkBitStreamType_ReadByte;
            }

        }
        else
        {
            nHadCheckBytesLen += 4;
            goto checkBitStreamType_ReadByte;
        }

        logd("result: nCheck4BitsValue[%d], bStreamWithStartCode[%d], nHadCheckBytesLen[%d]",
            nCheck4BitsValue,pStreamInfo->bStreamWithStartCode, nHadCheckBytesLen);
        //*continue reqeust stream from sbm when if judge the stream type
        if(pStreamInfo->bStreamWithStartCode == -1)
        {
            SbmFlushStream(pStreamInfo->pSbm, pStream);
            continue;
        }
        else
        {
            //* judge stream type successfully, return.
            SbmReturnStream(pStreamInfo->pSbm, pStream);
            nRet = 0;
            break;
        }
    }

    return nRet;
}

void HevcResetDecoder(HevcContex *pHevcDec)
{
    int i;
    HevcStreamInfo *pStreamInfo = pHevcDec->pStreamInfo;
    HevcResetClearRefs(pHevcDec);
    HevcDecReturnAllSbmStreams(pHevcDec);
    DecoderReturnOneSbmStream(pStreamInfo);
    pStreamInfo->nCurrentDataBufIdx = 0;
    pStreamInfo->bFindKeyFrameOnly = 1; /* The first frame should be a keyframe */
    for(i = 0; i < HEVC_PTS_LIST_SIZE; i++)
        pHevcDec->PtsList[i] = -1;
    for(i = 0; i < HEVC_NB_RPS_TYPE; i++)
        pHevcDec->Rps[i].nNumOfRefs = 0;

    HevcClearPocList(pHevcDec);

    if(pHevcDec->debug.nDecoderDropFrameNum)
    {
         logd(" decoder info: decode: %d, dorp: %d",
                 pHevcDec->debug.nDecodeFrameNum, pHevcDec->debug.nDecoderDropFrameNum);
    }
    pHevcDec->SliceHeader.pShortTermRps = NULL;
    pHevcDec->nPts = 0;
    pHevcDec->nLastPts = -1;
    pHevcDec->nLastStreamPts = -1;
    pHevcDec->pStreamInfo->bPtsCaculated = 0;
    pHevcDec->eDecodStep = 0;
    //pHevcDec->b10BitStreamFlag = -1;
    //pHevcDec->bPreHdrFlag = -1;
    if((pHevcDec->pStreamInfo->pSbm->bUseNewVeMemoryProgram == 1)&&
        ENABLE_MV_BUF_OPTIMIZATION_PROGRAM)
    {
        HevcMvBufInf* pHevcMvBufNode = NULL;
        while(1)
        {
            pHevcMvBufNode = (HevcMvBufInf*)FIFODequeue((FiFoQueueInst**)&
               pHevcDec->pHevcMvBufEmptyQueue);
            if(pHevcMvBufNode == NULL)
            {
                break;
            }
            usleep(1);
        }
        for(i=0; i<pHevcDec->nHevcMvBufNum; i++)
        {
            FIFOEnqueue((FiFoQueueInst**)&pHevcDec->pHevcMvBufEmptyQueue,
                (FiFoQueueInst*)&pHevcDec->pHevcMvBufInf[i]);
        }
    }
}

static void HevcDestroySbmInfo(HevcContex *pHevcDec)
{
    HevcStreamInfo *pStreamInfo = pHevcDec->pStreamInfo;
    struct ScMemOpsS *_memops = pHevcDec->pConfig->memops;
    int i = 0;

    for(i = 0; i <HEVC_MAX_STREAM_NUM; i++)
    {
        if(pStreamInfo->pStreamList[i] != NULL)
        {
            SbmFlushStream(pStreamInfo->pSbm, pStreamInfo->pStreamList[i]);
            pStreamInfo->pStreamList[i] = NULL;
        }
    }

    if(pStreamInfo->pStreamTemp != NULL)
        SbmFlushStream(pStreamInfo->pSbm, pStreamInfo->pStreamTemp);
    HevcAdapterFree(_memops, &pStreamInfo->pLocalBuf,
                                                                pHevcDec->pConfig->veOpsS,
                                                                pHevcDec->pConfig->pVeOpsSelf);
    HevcFree(&pStreamInfo->pExtraData);
}

static void HevcDestroyFrameBuf(HevcContex *pHevcDec)
{
    HevcFrame *pHf;
    struct ScMemOpsS *_memops = pHevcDec->pConfig->memops;
    s32 i;
    if((pHevcDec->pStreamInfo->pSbm->bUseNewVeMemoryProgram==0)||     \
        !ENABLE_MV_BUF_OPTIMIZATION_PROGRAM)
    {
        for(i = 0; i < HEVC_MAX_DPB_SIZE; i++)
        {
            pHf = &pHevcDec->DPB[i];
            if(pHf->pMVFrameCol != NULL)
                HevcAdapterFree(_memops, &pHf->pMVFrameCol,
                                pHevcDec->pConfig->veOpsS,
                                pHevcDec->pConfig->pVeOpsSelf);
        }
    }
    else
    {
        if(pHevcDec->pHevcMvBufInf != NULL)
        {
            for(i = 0; i < pHevcDec->nHevcMvBufNum; i++)
            {
                if(pHevcDec->pHevcMvBufInf[i].pMVFrameColManager != NULL)
                {
                    HevcAdapterFree(_memops,     \
                        &(pHevcDec->pHevcMvBufInf[i].pMVFrameColManager->pMVFrameCol),
                    pHevcDec->pConfig->veOpsS,
                    pHevcDec->pConfig->pVeOpsSelf);
                }
                free(pHevcDec->pHevcMvBufInf[i].pMVFrameColManager);
                pHevcDec->pHevcMvBufInf[i].pMVFrameColManager = NULL;
            }

            free(pHevcDec->pHevcMvBufInf);
            pHevcDec->pHevcMvBufInf = NULL;
         }
    }
}

void HevcDecDestroy(HevcContex *pHevcDec)
{
    s32 i;
    HevcSliceHeader *pSh;
    struct ScMemOpsS *_memops = NULL;
    if(pHevcDec == NULL)
        return ;

    if(pHevcDec->pConfig->bSetProcInfoEnable == 1)
    {
        CdcVeStopProcInfo(pHevcDec->pConfig->veOpsS,
                          pHevcDec->pConfig->pVeOpsSelf,
                          pHevcDec->pConfig->nChannelNum);
    }

    _memops = pHevcDec->pConfig->memops;
    HevcDestroySbmInfo(pHevcDec);
    HevcDestroyFrameBuf(pHevcDec);
    HevcFree(&pHevcDec->pStreamInfo);

    if(pHevcDec->fbm)
        FbmDestroy(pHevcDec->fbm);
    if(pHevcDec->SecondFbm)
        FbmDestroy(pHevcDec->SecondFbm);

    for(i = 0; i < HEVC_MAX_VPS_NUM; i++) /* vps list */
        if(pHevcDec->VpsList[i] != NULL)
            HevcFree(&pHevcDec->VpsList[i]);
    for(i = 0; i < HEVC_MAX_SPS_NUM; i++) /* sps list */
        if(pHevcDec->SpsList[i] != NULL)
            HevcFree(&pHevcDec->SpsList[i]);
    for(i = 0; i < HEVC_MAX_PPS_NUM; i++) /* pps list */
        if(pHevcDec->PpsList[i] != NULL)
        {
            HevcFreePpsBuffer(pHevcDec->PpsList[i]);
            HevcFree(&pHevcDec->PpsList[i]);
        }
    HevcAdapterFree(_memops, &pHevcDec->SliceHeader.pRegEntryPointOffset,
                                                                pHevcDec->pConfig->veOpsS,
                                                                pHevcDec->pConfig->pVeOpsSelf);
    HevcAdapterFree(_memops, &pHevcDec->ControlInfo.pNeighbourInfoBuffer,
                                                                pHevcDec->pConfig->veOpsS,
                                                                pHevcDec->pConfig->pVeOpsSelf);

    if(pHevcDec->pMvInfo != NULL)
    {
        HevcFree(&pHevcDec->pMvInfo);
    }

#if HEVC_SAVE_EACH_FRAME_TIME
    loge("before saving frame time. frame number: %d ", pHevcDec->debug.nAllTimeNum);
    if(pHevcDec->debug.pAllTimeHw != NULL)
    {
        FILE *fp = fopen("/data/camera/frameTime.txt", "wt");
        if(fp != NULL)
        {
            u32 j;
            loge(" saving frame time. frame number: %d ", pHevcDec->debug.nAllTimeNum);
            for(j = 0; j < pHevcDec->debug.nAllTimeNum; j++)
            {
                fprintf(fp,"%u\n", pHevcDec->debug.pAllTimeHw[j]);
            }
            fclose(fp);
        }
        else
        {
             loge("save frame time open file error");
        }
        HevcFree(&pHevcDec->debug.pAllTimeHw);
    }
#endif //HEVC_SAVE_EACH_FRAME_TIME
    /* slice header */
    pSh = &pHevcDec->SliceHeader;
    if(pSh->EntryPointOffset)
        HevcFree(&pSh->EntryPointOffset);
    if(pSh->Size)
        HevcFree(&pSh->Size);
    if(pSh->Offset)
        HevcFree(&pSh->Offset);

    if(pHevcDec->pSps)
        HevcFree(&pHevcDec->pSps);

}

#undef IS_HEVC_START_CODE

