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
* File :h265_debug.c
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

void HevcDecRequestOneFrameStreamDebug(HevcContex *pHevcDec)
{

#if (HEVC_DEBUG_FUNCTION_ENABLE | HEVC_SHOW_STREAM_INFO)
    HevcStreamInfo *pStreamInfo;
    pStreamInfo = pHevcDec->pStreamInfo;
    int i;

    logd(" ");
    logd(" --------- HevcDecRequestOneFrameStreamDebug info start ----------- ");

    logd(" frame stream number: %d ", pStreamInfo->nCurrentStreamIdx + 1);
    for(i = 0; i <= pStreamInfo->nCurrentStreamIdx; i++)
    {
        logd("stream[%d]: 0x%x", i, (u32)pStreamInfo->pStreamList[i]);
    }
    logd(" frame nulu number: %d   nalu type: %d ",
        pStreamInfo->nCurrentDataBufIdx + 1, pStreamInfo->eNaluType);
    logd(" ... ");

    logd(" ---------  HevcDecRequestOneFrameStreamDebug info end  ----------- ");
    logd(" ");
#else
    CEDARC_UNUSE(pHevcDec);
#endif //(HEVC_DEBUG_FUNCTION_ENABLE | HEVC_SHOW_STREAM_INFO)
}

void HevcShowSliceRplInfoDebug(HevcContex *pHevcDec)
{

#if (HEVC_DEBUG_FUNCTION_ENABLE | HEVC_SHOW_RPL_INFO)

    HevcRefPicList *pRpl;
    s32 bIsBSlice = pHevcDec->SliceHeader.eSliceType == HEVC_B_SLICE;
    s32 i, nNum;

#if 0
    if(pHevcDec->nPoc <= 10 && pHevcDec->nPoc >= 20)
        return;
    if(pHevcDec->nMd5Frame != 11)
        return;
#endif

    logd(" ");
    logd(" -------------  ref picture list info  -------- ");
    logd(" poc: %d, nMd5Frame: %d ", pHevcDec->nPoc, pHevcDec->nMd5Frame);
    nNum = 0;
    logd(" valid frame info start ");
    for(i = 0; i < HEVC_MAX_DPB_SIZE; i++)
    {
        HevcFrame *pHf = &pHevcDec->DPB[i];
        if(pHf->bFlags)
        {
            logd(" %d: poc: %d ", i, pHf->nPoc);
            nNum++;
        }
    }
    if(pHevcDec->pSps)
    {
        s32 index = pHevcDec->pSps->nMaxSubLayers - 1;
        logd(" total valid frame: %d,  max_dec_pic_buffering: %d ",
                nNum, pHevcDec->pSps->TemporalLayer[index].nSpsMaxDecPicBuffering);
    }
    logd(" valid frame info end ");
    logd(" ");

    pRpl = &pHevcDec->RefPicList[0];
    if(pRpl->nNumOfRefs > 0)
    {

        logd(" --------- poc: %d; ref picture list 0 start ---------- ", pHevcDec->nPoc);
        for(i = 0; i < pRpl->nNumOfRefs; i++)
        {
            logd("\t\t%d:  %d,   %p", i, pRpl->List[i], pRpl->Ref[i]->pFbmBuffer);
        }
        logd(" -------------- ref picture list 0 end --------------- ");
    }
    else
    {
        logd(" ref picture list 0 maybe error. no ref picture in the list ");
    }
    if(!bIsBSlice)
    {
        logd(" This is not B slice. List 1 unavalable");
        return;
    }
    pRpl = &pHevcDec->RefPicList[1];
    if(pRpl->nNumOfRefs > 0)
    {

        logd(" --------- poc: %d; ref picture list 1 start ---------- ", pHevcDec->nPoc);
        for(i = 0; i < pRpl->nNumOfRefs; i++)
        {
            logd("\t\t%d:  %d,   %p", i, pRpl->List[i], pRpl->Ref[i]->pFbmBuffer);
        }
        logd(" -------------- ref picture list 1 end --------------- ");
    }
    else
    {
        logd(" ref picture list 1 maybe error. no ref picture in the list ");
    }
    logd(" ");
#else
    CEDARC_UNUSE(pHevcDec);
#endif //(HEVC_DEBUG_FUNCTION_ENABLE | HEVC_SHOW_RPL_INFO)
}

void HevcPtsListInfoDebug(HevcContex *pHevcDec, s32 nIdx, s32 bIsAdding, s32 nPoc)
{

#if (HEVC_DEBUG_FUNCTION_ENABLE | HEVC_SHOW_PTS_INFO)

    s32 i;
    s32 nNum = 0;
    static int64_t nPts = -1;

#if 0
    if(pHevcDec->nPoc <= 10 && pHevcDec->nPoc >= 20)
        return;
#endif

    if(nIdx < 0 || nIdx > HEVC_PTS_LIST_SIZE)
        return ;
    logd(" ");
    if(bIsAdding)
    {
        logd(" ------- add one pts to list start ------- ");
    }
    else
    {
        logd(" ------ find one pts info start ------- ");
        logd(" decoding poc: %d, display poc: %d ", pHevcDec->nPoc, nPoc);
    }
    for(i = 0; i < HEVC_PTS_LIST_SIZE; i++)
    {
        if(pHevcDec->PtsList[i] != -1)
            nNum++;
        if(i == nIdx)
        {
            if(bIsAdding)
                logd("\t\t  %d: %lld  ----> add pts", i, pHevcDec->PtsList[i]);
            else
                logd("\t\t  %d: %lld  ----> output pts", i, pHevcDec->PtsList[i]);
        }
        else
        {
            logd("\t\t%d: %lld", i, pHevcDec->PtsList[i]);
        }
    }
    if(!bIsAdding && nPts != -1)
    {
        logd(" last pts: %lld, current pts: %lld, diff: %lld ",
                nPts, pHevcDec->PtsList[nIdx], (pHevcDec->PtsList[nIdx] - nPts));
    }

    if(!bIsAdding)
        nPts = pHevcDec->PtsList[nIdx];
    logd(" valid pts number: %d", nNum);
    logd(" ------- show pts info end ------- ");
    logd(" ");
#else
    CEDARC_UNUSE(pHevcDec);
    CEDARC_UNUSE(bIsAdding);
    CEDARC_UNUSE(nIdx);
    CEDARC_UNUSE(nPoc);
#endif
}

void HevcShowShortRefPicSetInfoDebug(HevcShortTermRPS *pSrps, s32 nPoc)
{
#if (HEVC_DEBUG_FUNCTION_ENABLE | HEVC_SHOW_RPS_INFO)
    s32 i;
    s32 nNum = 0;
#if 0
    if(pHevcDec->nPoc <= 10 && pHevcDec->nPoc >= 20)
        return;
#endif
    logd("---------- poc: %d short term ref pic set info start ---------", nPoc);
    logd("nNumNegativePics: %d", pSrps->nNumNegativePics);
    logd("nNumDeltaPocs: %d", pSrps->nNumDeltaPocs);
    logd("  negative: ");
    for(i = 0; i < pSrps->nNumNegativePics; i++)
    {
        logd("is used[%d]: %d\t\tnDeltaPoc: %d", i, pSrps->bUsed[i], pSrps->nDeltaPoc[i]);
    }
    logd("  positive: ");
    for(; i < pSrps->nNumDeltaPocs; i++)
    {
        logd("is used[%d]: %d\t\tnDeltaPoc: %d", i, pSrps->bUsed[i], pSrps->nDeltaPoc[i]);
    }
    logd("--- short term ref pic set info end --- ");
#else
    CEDARC_UNUSE(pSrps);
    CEDARC_UNUSE(nPoc);
#endif
}

void HevcShowRefPicSetInfoDebug(HevcRefPicList *pRps)
{
#if (HEVC_DEBUG_FUNCTION_ENABLE | HEVC_SHOW_RPS_INFO)
     int j;
     logd("----------- 5 reference picture sets info start ------------- ");
     logd("  HEVC_ST_FOLL:  (total num: %d) ", pRps[HEVC_ST_FOLL].nNumOfRefs);
     for(j = 0; j < pRps[HEVC_ST_FOLL].nNumOfRefs; j++)
         logd("%6d ", pRps[HEVC_ST_FOLL].List[j]);
     logd(" ");

     logd("  HEVC_ST_CURR_BEF:    (total num: %d)  ", pRps[HEVC_ST_CURR_BEF].nNumOfRefs);
     for(j = 0; j < pRps[HEVC_ST_CURR_BEF].nNumOfRefs; j++)
         logd("%4d ", pRps[HEVC_ST_CURR_BEF].List[j]);
     logd(" ");

     logd("  HEVC_ST_CURR_AFT:   (total num: %d)   ", pRps[HEVC_ST_CURR_AFT].nNumOfRefs);
     for(j = 0; j < pRps[HEVC_ST_CURR_AFT].nNumOfRefs; j++)
         logd("%6d ", pRps[HEVC_ST_CURR_AFT].List[j]);
     logd(" ");

     logd("  HEVC_LT_CURR:   (total num: %d)   ", pRps[HEVC_LT_CURR].nNumOfRefs);
     for(j = 0; j < pRps[HEVC_LT_CURR].nNumOfRefs; j++)
         logd("%4d ", pRps[HEVC_LT_CURR].List[j]);
     logd(" ");

     logd("  HEVC_LT_FOLL:    (total num: %d)  ", pRps[HEVC_LT_FOLL].nNumOfRefs);
     for(j = 0; j < pRps[HEVC_LT_FOLL].nNumOfRefs; j++)
         logd("%6d ", pRps[HEVC_LT_FOLL].List[j]);
     logd(" ");
     logd("------------ 5 reference picture sets info end ------------- ");
#else
     CEDARC_UNUSE(pRps);
#endif
}

static s32 HevcMd5CheckDiff(u8 *s, u8 *d, char *name)
{
    s32 i, diff;
    diff = 0;
    for(i = 0; i < 16; i++)
        if(s[i] != d[i])
        {
            diff = 1;
            break;
        }

    if(diff == 0)
    {
#if 0
        m = s;
        logd(" %s parsed   md5: %3d %3d %3d %3d %3d %3d %3d %3d   %3d %3d %3d %3d %3d %3d %3d %3d",
                name,
                m[0],m[1],m[2],m[3],m[4],m[5],m[6],m[7],
                m[8+0],m[8+1],m[8+2],m[8+3],m[8+4],m[8+5],m[8+6],m[8+7]);
        m = d;
        logd(" %s caculate md5: %3d %3d %3d %3d %3d %3d %3d %3d   %3d %3d %3d %3d %3d %3d %3d %3d",
                name,
                m[0],m[1],m[2],m[3],m[4],m[5],m[6],m[7],
                m[8+0],m[8+1],m[8+2],m[8+3],m[8+4],m[8+5],m[8+6],m[8+7]);
#endif
        return 0;
    }
    else
    {
        u8 *m = s;
        logv(" %s parsed   md5: %3d %3d %3d %3d %3d %3d %3d %3d   %3d %3d %3d %3d %3d %3d %3d %3d",
                name,
                m[0],m[1],m[2],m[3],m[4],m[5],m[6],m[7],
                m[8+0],m[8+1],m[8+2],m[8+3],m[8+4],m[8+5],m[8+6],m[8+7]);
        m = d;
        logv(" %s caculate md5: %3d %3d %3d %3d %3d %3d %3d %3d   %3d %3d %3d %3d %3d %3d %3d %3d",
                name,
                m[0],m[1],m[2],m[3],m[4],m[5],m[6],m[7],
                m[8+0],m[8+1],m[8+2],m[8+3],m[8+4],m[8+5],m[8+6],m[8+7]);
        loge(" %s md5 error", name);
        usleep(50*1000);
        return 1;
    }
    return 0;
}

void HevcDecodeCheckMd5(HevcContex *pHevcDec)
{
    CEDARC_UNUSE(HevcDecodeCheckMd5);

    u8 *md5p, *md5c;  /* p: parsed md5; c: caculate md5 */
    s32 nRet = 0;
    if(pHevcDec->eDisplayPixelFormat != PIXEL_FORMAT_YV12)
    {
        logd(" pixel format is not YV12 ");
        return;
    }
    logv(" poc: %d, md5 check", pHevcDec->nPoc);
    /* y */
    md5p = pHevcDec->md5[0];
    md5c = pHevcDec->md5Caculate[0];
    nRet += HevcMd5CheckDiff(md5p, md5c, "luma");

    /* u */
    md5p = pHevcDec->md5[1];
    if(pHevcDec->eDisplayPixelFormat == PIXEL_FORMAT_YV12)
        md5c = pHevcDec->md5Caculate[2];
    else
        md5c = pHevcDec->md5Caculate[1];
    nRet += HevcMd5CheckDiff(md5p, md5c, "cb");

    /* v */
    md5p = pHevcDec->md5[2];
    if(pHevcDec->eDisplayPixelFormat == PIXEL_FORMAT_YV12)
        md5c = pHevcDec->md5Caculate[1];
    else
        md5c = pHevcDec->md5Caculate[2];
    nRet += HevcMd5CheckDiff(md5p, md5c, "cr");
    if(nRet == 0)
    {
        logv("       poc: %d; luma, cb, cr, md5 ok", pHevcDec->nPoc);
    }
    else
    {
        loge(" poc: %d. md5 check fail, md5 frame number: %d, decode frame: %d ",
                pHevcDec->nPoc, pHevcDec->nMd5Frame, pHevcDec->debug.nDecodeFrameNum);
    }
    pHevcDec->nMd5Frame += 1;
}

void HevcSettingRegDebug(HevcContex *pHevcDec)
{
    s32 i;
    size_addr RegBaseAddr = pHevcDec->HevcDecRegBaseAddr;
    size_addr RegTopAddr  = pHevcDec->HevcTopRegBaseAddr;
    u32 *base;
    //if(pHevcDec->nMd5Frame != 11)
    //    return;
    base = (u32 *)RegTopAddr;
    logd("---------------- num: %d poc: %d. top register infomation start ------_-----",
          pHevcDec->nDecCountHevc, pHevcDec->nPoc);
    for(i=0;i<16;i++)
    {
        logd("i=%d, 0x%08x  0x%08x  0x%08x  0x%08x ", i,
                (u32)base[4*i], (u32)base[4*i+1], (u32)base[4*i+2], (u32)base[4*i+3]);
    }
    logd("---------------- num: %d poc: %d. top register infomation end --------------",
          pHevcDec->nDecCountHevc, pHevcDec->nPoc);

    logd(" ");
    base = (u32 *)RegBaseAddr;
    logd("---------------- num: %d poc: %d. hevc register(%08x) infomation start ------",
         pHevcDec->nDecCountHevc, pHevcDec->nPoc,(int)RegBaseAddr);
    for(i=0;i<16;i++)
    {
        logd("i=%d, 0x%08x  0x%08x  0x%08x  0x%08x ", i,
                (u32)base[4*i], (u32)base[4*i+1], (u32)base[4*i+2], (u32)base[4*i+3]);
    }
    logd("---------------- num: %d poc: %d. hevc register infomation end --------------",
         pHevcDec->nDecCountHevc, pHevcDec->nPoc);
}

static void HeveSaveData(FILE *fp, char *src, s32 nStride, s32 nWidth, s32 nHeight)
{
    s32 i;
    char *p = src;
    for(i = 0; i < nHeight; i++)
    {
        fwrite(p, 1, nWidth, fp);
        p += nStride;
    }
}

void HevcSaveYuvData(HevcContex *pHevcDec)
{
    CEDARC_UNUSE(HevcSaveYuvData);

    HevcFrame *pHf = pHevcDec->pCurrDPB;
    VideoPicture *pFrame;
    s32 nWidth, nHeight, nStride, nUVStride;
    s32 nUoffset, nVoffset;
    FILE *fp;
    char *pSrc;
    if(pHf == NULL)
        return;
    pFrame = pHf->pFbmBuffer;
    if(pFrame == NULL)
        return;
    fp = fopen("/data/camera/p0.yuv", "ab");
    if(fp == NULL)
    {
        loge(" save yuv data open file fail..... ");
        return;
    }
    nUVStride = pHevcDec->ControlInfo.priFrmBufChromaStride;
    if(pHevcDec->eDisplayPixelFormat == PIXEL_FORMAT_YV12)
    {
        nUoffset = pHevcDec->ControlInfo.priFrmBufLumaSize +
                pHevcDec->ControlInfo.priFrmBufChromaSize +
                (pFrame->nLeftOffset >> 1) + nUVStride * (pFrame->nTopOffset >> 1);
        nVoffset = pHevcDec->ControlInfo.priFrmBufLumaSize +
                (pFrame->nLeftOffset >> 1) + nUVStride * (pFrame->nTopOffset >> 1);
    }
    else
    {
        nUoffset =  pHevcDec->ControlInfo.priFrmBufLumaSize +
                (pFrame->nLeftOffset >> 1) + nUVStride * (pFrame->nTopOffset >> 1);
        nVoffset = pHevcDec->ControlInfo.priFrmBufLumaSize +
                pHevcDec->ControlInfo.priFrmBufChromaSize +
                (pFrame->nLeftOffset >> 1) + nUVStride * (pFrame->nTopOffset >> 1);
    }
    /* y */
    nWidth = pFrame->nRightOffset - pFrame->nLeftOffset;
    nHeight = pFrame->nBottomOffset - pFrame->nTopOffset;
    nStride = pHevcDec->ControlInfo.priFrmBufLumaStride;
    logd(" save pic %d, width: %d,  height: %d, nStride: %d, md5frame: %d",
            pHevcDec->nPoc, nWidth, nHeight, nStride, pHevcDec->nMd5Frame);
    pSrc = pFrame->pData0 + pFrame->nLeftOffset + nStride * pFrame->nTopOffset;
    HeveSaveData(fp, pSrc, nStride, nWidth, nHeight);

    /* u */
    nStride = pHevcDec->ControlInfo.priFrmBufChromaStride;
    nWidth = nWidth >> 1;
    nHeight = nHeight >> 1;
    pSrc = pFrame->pData0 + nUoffset;
    HeveSaveData(fp, pSrc, nStride, nWidth, nHeight);

    /* v */
    pSrc = pFrame->pData0 + nVoffset;
    HeveSaveData(fp, pSrc, nStride, nWidth, nHeight);

    fclose(fp);
    logd(" save poc %d ok...... ", pHevcDec->nPoc);
}

void HevcSaveSecYuvData(HevcContex *pHevcDec)
{
    CEDARC_UNUSE(HevcSaveSecYuvData);

    HevcFrame *pHf = pHevcDec->pCurrDPB;
    VideoPicture *pFrame;
    s32 nWidth, nHeight, nStride;
    FILE *fp;
    char *pSrc;
    if(pHf == NULL)
        return;
    pFrame = pHf->pSecFbmBuffer;
    if(pFrame == NULL)
        return;
    fp = fopen("/data/camera/720p.yuv", "ab");
    if(fp == NULL)
    {
        loge(" save yuv data open file fail..... ");
        return;
    }
    /* y */
    nWidth = pFrame->nRightOffset - pFrame->nLeftOffset;
    nHeight = pFrame->nBottomOffset - pFrame->nTopOffset;
    nStride = pHevcDec->ControlInfo.secFrmBufLumaStride;
    logd(" save pic %d, width: %d,  height: %d, nStride: %d",
        pHevcDec->nPoc, nWidth, nHeight, nStride);
    pSrc = pFrame->pData0 + pFrame->nLeftOffset + nStride * pFrame->nTopOffset;
    HeveSaveData(fp, pSrc, nStride, nWidth, nHeight);

    /* u */
    nStride = pHevcDec->ControlInfo.secFrmBufChromaStride;
    nWidth = nWidth >> 1;
    nHeight = nHeight >> 1;
    pSrc = pFrame->pData0 + pHevcDec->ControlInfo.secFrmBufLumaSize +
            (pFrame->nLeftOffset >> 1) + nStride * (pFrame->nTopOffset >> 1);
    HeveSaveData(fp, pSrc, nStride, nWidth, nHeight);

    /* v */
    pSrc = pFrame->pData0 + pHevcDec->ControlInfo.secFrmBufLumaSize +
            pHevcDec->ControlInfo.secFrmBufChromaSize +
            (pFrame->nLeftOffset >> 1) + nStride * (pFrame->nTopOffset >> 1);
    HeveSaveData(fp, pSrc, nStride, nWidth, nHeight);

    logd(" save poc %d ok...... ", pHevcDec->nPoc);
    fclose(fp);
}

void HevcShowScalingListData(HevcScalingList *pSl, s32 bIsInSps)
{
#if (HEVC_DEBUG_FUNCTION_ENABLE | HEVC_SHOW_SCALING_LIST_INFO)
    logd(" scaling data list info. in sps: %d", bIsInSps);
    s32 nSizeId, nMatrixId, j;

    for(nSizeId = 0; nSizeId < 4; nSizeId++)
        for(nMatrixId = 0; nMatrixId < (nSizeId == 3 ? 2 : 6); nMatrixId++)
        {
            u8 *p = pSl->Sl[nSizeId][nMatrixId];
            logd("-------- nSizeId: %d, nMatrixId: %d",  nSizeId, nMatrixId);
            for(j = 0; j < 64; j += 8)
            {
                logd(" %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d",
                        p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
                p += 8;
            }
        }
#else
    CEDARC_UNUSE(pSl);
    CEDARC_UNUSE(bIsInSps);
#endif
}

void HevcSaveDecoderSliceData(HevcContex *pHevcDec)
{
    CEDARC_UNUSE(HevcSaveDecoderSliceData);

    FILE *fp = fopen("/data/camera/funny00.bin", "ab");
    HevcStreamInfo *pStreamInfo = pHevcDec->pStreamInfo;
    s32 i, nNum, nSize, bTwoTrunk;
    char start[4] = {0x00, 0x00, 0x00, 0x01};
//    char start[4] = {0xff, 0xff, 0xff, 0xff};
    if(fp == NULL)
    {
        logd(" save decoder data open file error ");
        return;
    }
    nNum = pStreamInfo->nCurrentDataBufIdx;
    for(i = 0; i < nNum; i++)
    {
        char *p = pStreamInfo->pDataBuf[i];
        nSize = pStreamInfo->nDataSize[i];
        bTwoTrunk = pStreamInfo->bHasTwoDataTrunk[i];
        logd(" saving data... two trunk: %d,  size: %d ", bTwoTrunk, nSize);
        fwrite(start, 1, 4, fp);
        if(bTwoTrunk)
        {
            s32 nSize0 = pStreamInfo->FirstDataTrunkSize[i];
            s32 nSize1 = nSize - nSize0;
            fwrite(p, 1, nSize0, fp);
            p = pStreamInfo->pSbmBuf;
            fwrite(p, 1, nSize1, fp);
        }
        else
        {
            fwrite(p, 1, nSize, fp);
        }
    }
    fclose(fp);
}

void HevcShowMaxUsingBufferNumber(HevcContex *pHevcDec)
{
    CEDARC_UNUSE(HevcShowMaxUsingBufferNumber);

    int i, t = 0;
    int s = 0;
    for(i = 0; i < HEVC_MAX_DPB_SIZE; i++)
        if(pHevcDec->DPB[i].pFbmBuffer != NULL)
            t += 1;
    if(pHevcDec->ControlInfo.secOutputEnabled)
    {
        for(i = 0; i < HEVC_MAX_DPB_SIZE; i++)
            if(pHevcDec->DPB[i].pSecFbmBuffer != NULL)
                s += 1;

    }
    if(t >= pHevcDec->debug.nMaxUsingDpbNumber)
    {
        int refNum = 0;
        pHevcDec->debug.nMaxUsingDpbNumber = t;
        logd("  ");
        for(i = 0; i < HEVC_MAX_DPB_SIZE; i++)
            if(pHevcDec->DPB[i].pFbmBuffer != NULL)
            {
                logd("poc: %d, buffer flags: %d  ", pHevcDec->DPB[i].nPoc, pHevcDec->DPB[i].bFlags);
                if(pHevcDec->DPB[i].bFlags & HEVC_FRAME_FLAG_SHORT_REF)
                    refNum++;
            }
        if(pHevcDec->debug.nMaxRefPicNumber < refNum)
            pHevcDec->debug.nMaxRefPicNumber = refNum;
        loge(" hevc max using buffer number: %d, \
            using second fbm num: %d, ref num: %d, max ref num: %d ",
            pHevcDec->debug.nMaxUsingDpbNumber, s, refNum,
            pHevcDec->debug.nMaxRefPicNumber);
        logd("  ");
    }
}
