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
* File : h264_dec.c
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
#include "stdio.h"
#include <math.h>
#include "h264_func.h"
#if H264_DEBUG_COMPUTE_PIC_MD5
#include "h265_func.h"
#endif

#ifndef AV_RB16
#   define AV_RB16(x)                           \
((((const u8*)(x))[0] << 8) |          \
 ((const u8*)(x))[1])
#endif


/*************************************************************************/
/*************************************************************************/
static void H264DebugSaveDecoderPic(VideoPicture *pPic, H264Context *hCtx);

void H264ResetDecoderParams(H264Context* hCtx)
{
    s32 i = 0;

    for(i=0; i<18; i++)
    {
        if(hCtx->frmBufInf.pDelayedPic[i] != NULL)
        {
            hCtx->frmBufInf.pDelayedPic[i]->nReference = 0;
            hCtx->frmBufInf.pDelayedPic[i]->pVPicture = NULL;
        }
    }

    if(hCtx->frmBufInf.pDelayedOutPutPic != NULL)
    {
        hCtx->frmBufInf.pDelayedOutPutPic->nReference = 0;
        hCtx->frmBufInf.pDelayedOutPutPic->pVPicture = NULL;
    }

    if(hCtx->frmBufInf.pCurPicturePtr != NULL)
    {
        hCtx->frmBufInf.pCurPicturePtr->nReference = 0;
        hCtx->frmBufInf.pCurPicturePtr->pVPicture = NULL;
    }

    H264ReferenceRefresh(hCtx);
    hCtx->bNeedFindIFrm = 1;
    hCtx->nPicStructure = 0xFF;
    hCtx->bFstField = 0xFF;
    hCtx->nCurSliceNum = 0;
    memset(hCtx->frmBufInf.defaultRefList, 0, 2*MAX_PICTURE_COUNT*sizeof(H264PicInfo));
    memset(hCtx->frmBufInf.refList, 0, 2*32*sizeof(H264PicInfo));
    hCtx->nDecStep = H264_STEP_CONFIG_VBV;

    hCtx->vbvInfo.nLastValidPts = H264VDEC_ERROR_PTS_VALUE;
    hCtx->vbvInfo.nValidDataPts = H264VDEC_ERROR_PTS_VALUE;
    hCtx->vbvInfo.nVbvDataPts = H264VDEC_ERROR_PTS_VALUE;
    hCtx->vbvInfo.nNextPicPts = H264VDEC_ERROR_PTS_VALUE;
    hCtx->vbvInfo.pVbvStreamData = NULL;
    hCtx->pCurStreamFrame = NULL;

    hCtx->nCurFrmNum = 0 ;
    hCtx->nMinDispPoc = 0;
    hCtx->nDelayedPicNum = 0;
    hCtx->nFrmNumOffset = 0;
    hCtx->nPrevFrmNumOffset = 0;
    hCtx->nPrevPocMsb = 0;
    hCtx->nPrevPocLsb = 0;
    hCtx->nDeltaPocBottom = 0;
    hCtx->frmBufInf.pDelayedOutPutPic = NULL;
    hCtx->frmBufInf.pCurPicturePtr = NULL;
    hCtx->frmBufInf.pLastPicturePtr = NULL;
    hCtx->frmBufInf.pNextPicturePtr = NULL;
    hCtx->frmBufInf.nRef2MafBufAddr = 0;
    hCtx->frmBufInf.nRef1MafBufAddr = 0;
    hCtx->vbvInfo.nNextPicPts = 0;
    hCtx->vbvInfo.nPrePicPts = 0;
    hCtx->vbvInfo.nPrePicPoc = 0;
    hCtx->vbvInfo.pVbvStreamData = NULL;
    hCtx->nDecFrameStatus = H264_END_DEC_FRAME;
    //hCtx->nColorPrimary = 0xffffffff;
    hCtx->nFrmNum = 0;
    hCtx->nLastFrmNum = 0;
    hCtx->nSeiRecoveryFrameCnt = 0;
    hCtx->vbvInfo.nLastUsedValidPts = H264VDEC_ERROR_PTS_VALUE;
}

/******************************************************************/
/*************************************  ***************************/
s32 H264InitDecode(H264DecCtx* h264DecCtx, H264Dec* h264Dec, H264Context* hCtx)
{
    h264Dec->memops  = h264DecCtx->vconfig.memops;
    h264Dec->pConfig = &h264DecCtx->vconfig;

    memset(hCtx, 0, sizeof(H264Context));

    #if 0

    size += 0x20000+2*1024;
    hCtx->pMbFieldIntraBuf = CdcMemPalloc(h264DecCtx->vconfig.memops,size,
                                            (void *)h264DecCtx->vconfig.veOpsS,
                                            h264DecCtx->vconfig.pVeOpsSelf);
    if(hCtx->pMbFieldIntraBuf == NULL)
    {
        return VDECODE_RESULT_UNSUPPORTED;
    }
    hCtx->phyMbFieldIntraBuf
        = (size_addr) CdcMemGetPhysicAddress(h264DecCtx->vconfig.memops,
                                             hCtx->pMbFieldIntraBuf);
    logv("phyMbFieldIntraBuf: vir=%p, phy=%x\n",
          hCtx->pMbFieldIntraBuf, (unsigned int)hCtx->phyMbFieldIntraBuf);

    CdcMemSet(h264DecCtx->vconfig.memops,hCtx->pMbFieldIntraBuf, 0, size);
    CdcMemFlushCache(h264DecCtx->vconfig.memops,hCtx->pMbFieldIntraBuf, size);

    s32 MbNeighborInfoBufSize = 0x4000 + 0x4000 + 0x4000;
    hCtx->pMbNeighborInfoBuf = CdcMemPalloc(h264DecCtx->vconfig.memops,MbNeighborInfoBufSize,
                                            (void *)h264DecCtx->vconfig.veOpsS,
                                            h264DecCtx->vconfig.pVeOpsSelf);
    if(hCtx->pMbNeighborInfoBuf == NULL)
    {
        return VDECODE_RESULT_UNSUPPORTED;
    }
    CdcMemSet(h264DecCtx->vconfig.memops,hCtx->pMbNeighborInfoBuf, 0, MbNeighborInfoBufSize);
    CdcMemFlushCache(h264DecCtx->vconfig.memops,hCtx->pMbNeighborInfoBuf, MbNeighborInfoBufSize);

    //*mb_neighbor_info_addr require 16k-align
    size_addr  tmpPhyaddr = (size_addr)CdcMemGetPhysicAddress(h264Dec->memops,
        (void*)hCtx->pMbNeighborInfoBuf);
    size_addr addrAlign = tmpPhyaddr%0x4000;
    if(addrAlign != 0)
    {
        tmpPhyaddr += (0x4000 - addrAlign);
    }
    hCtx->uMbNeighBorInfo16KAlignBufPhy = tmpPhyaddr;

    logv("uMbNeighBorInfo16KAlignBufPhy: vir=%p, phy=%x\n",
          hCtx->pMbNeighborInfoBuf, (unsigned int)hCtx->uMbNeighBorInfo16KAlignBufPhy);

   // logd("*** hCtx->uMbNeighBorInfo16KAlignBufPhy = %x, totalSize = %d KB",
     //    hCtx->uMbNeighBorInfo16KAlignBufPhy, MbNeighborInfoBufSize/1024);

   #endif

    hCtx->bNeedFindPPS = 1;
    hCtx->bNeedFindSPS = 1;
    hCtx->bNeedFindIFrm = 1;
    hCtx->vbvInfo.nVbvDataPts = H264VDEC_ERROR_PTS_VALUE;
    hCtx->vbvInfo.nValidDataPts = H264VDEC_ERROR_PTS_VALUE;
    hCtx->vbvInfo.nLastValidPts = H264VDEC_ERROR_PTS_VALUE;
    hCtx->vbvInfo.nFrameRate =  h264DecCtx->videoStreamInfo.nFrameRate;
    hCtx->nByteLensCurFrame  = 0;
    hCtx->bLastDropBFrame = 0;
    hCtx->bPreDropFrame = 0;
    hCtx->bDropFrame = 0;
    if(hCtx->vbvInfo.nFrameRate != 0)
    {
        if(hCtx->vbvInfo.nFrameRate>0 && hCtx->vbvInfo.nFrameRate<100)
        {
            hCtx->vbvInfo.nFrameRate *= 1000;
        }
        hCtx->vbvInfo.nPicDuration = 1000;
        hCtx->vbvInfo.nPicDuration *= (1000*1000);
        hCtx->vbvInfo.nPicDuration /= hCtx->vbvInfo.nFrameRate;
        if(hCtx->vbvInfo.nPicDuration > 80000)
        {
            hCtx->vbvInfo.nPicDuration = 33000;
        }
    }
    h264Dec->H264PerformInf.H264PerfInfo.nFrameDuration = hCtx->vbvInfo.nPicDuration;
    hCtx->nMinDispPoc = 0;
    hCtx->nDelayedPicNum = 0;
    hCtx->nPicStructure = 0xFF;
    hCtx->bFstField = 0xFF;
    hCtx->bIsAvc = 0;
    hCtx->nColorPrimary = 0xffffffff;
    hCtx->nSeiRecoveryFrameCnt = 0;
    hCtx->nRequestBufFailedNum = 0;
    hCtx->bProgressice = 0xFF;
    return 0;
}

void H264SetVbvParams(H264Dec* h264Dec,
                    H264Context* hCtx,
                    u8* startBuf,
                    u8* endBuf,
                    u32 dataLen,
                    u32 dataCtrlFlag)
{
    u32 phyAddr = 0;
    u32 highPhyAddr = 0;
    u32 lowPhyAddr = 0;
    size_addr tmpaddr = 0;


    hCtx->vbvInfo.pVbvDataStartPtr = startBuf;
    hCtx->vbvInfo.pVbvBuf = startBuf;

    hCtx->vbvInfo.pVbvBufEnd = endBuf;
    hCtx->vbvInfo.pReadPtr = startBuf;
    hCtx->vbvInfo.nVbvDataSize = dataLen+H264_EXTENDED_DATA_LEN;

    hCtx->vbvInfo.pVbvDataEndPtr = startBuf+ hCtx->vbvInfo.nVbvDataSize;
    hCtx->vbvInfo.bVbvDataCtrlFlag = dataCtrlFlag;


    tmpaddr = (size_addr)(CdcMemGetPhysicAddress(h264Dec->memops,
        (void*)hCtx->vbvInfo.pVbvBuf))&0xffffffff;
    phyAddr = (u32)(tmpaddr & 0xffffffff);
    highPhyAddr = (phyAddr>>28) & 0x0f;
    lowPhyAddr =  phyAddr & 0x0ffffff0;

    hCtx->vbvInfo.nVbvBufPhyAddr = lowPhyAddr+highPhyAddr;
    tmpaddr = (size_addr)(CdcMemGetPhysicAddress(h264Dec->memops,
        (void*)hCtx->vbvInfo.pVbvBufEnd))&0xffffffff;
    hCtx->vbvInfo.nVbvBufEndPhyAddr = (u32)(tmpaddr & 0xffffffff);
}

s32 H264ParseExtraDataSearchNaluSize(char *pData, s32 nSize)
{
    s32 i, nTemp, nNaluLen;
    s32 mask = 0xffffff;
    nTemp = -1;
    nNaluLen = -1;
    for(i = 0; i < nSize; i++)
    {
        nTemp = (nTemp << 8) | pData[i];
        if((nTemp & mask) == 0x000001)
        {
            nNaluLen = i - 3;
            break;
        }
    }
    return nNaluLen;

}

#if 0
static s32 H264ParseExtraDataDeleteEmulationCode(char *pBuf, s32 nSize)
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
        case 0x03:
            nSkipped += 1;
            break;
        default:
            if(nSkipped > 0)
                pBuf[i - nSkipped] = pBuf[i];
        }
    }
    return     (i - nSkipped);
}
#endif

s32 H264DecodeExtraDataNalu(H264DecCtx* h264DecCtx, H264Context* hCtx,
                                     u8 *pBuf, u8 nalUnitType, s32 nBitSize)
{
    u8 type = 0;

    if(h264DecCtx->bSoftDecHeader==0)
    {
        CdcVeReset(h264DecCtx->vconfig.veOpsS, h264DecCtx->vconfig.pVeOpsSelf);
        H264InitFuncCtrlRegister(h264DecCtx);

        hCtx->vbvInfo.pReadPtr = pBuf;
        if(hCtx->bIsAvc)
            H264ConfigureAvcRegister(h264DecCtx, hCtx, 1, nBitSize);
        else
            H264ConfigureEptbDetect(h264DecCtx, hCtx, nBitSize,1);//qu rongyu 0x03??
    }
    else
    {
        hCtx->vbvInfo.pReadPtr = pBuf;
        h264DecCtx->InitGetBits((void*)h264DecCtx, pBuf,nBitSize);

    }

    type = h264DecCtx->GetBits((void*)h264DecCtx, 8);

    type &= 0x1f;
    CEDARC_UNUSE(type);
    s32 ret = VDECODE_RESULT_OK;
    switch(nalUnitType)
    {
        case NAL_SPS:
        {
            ret = H264DecodeSps(h264DecCtx, hCtx);
            if(ret != VDECODE_RESULT_OK)
            {
                return ret;
            }
            hCtx->bNeedFindSPS = 0;

            break;
        }
        case NAL_PPS:
        {
            ret = H264DecodePps(h264DecCtx, hCtx, nBitSize);
            if(ret != VDECODE_RESULT_OK)
            {
                return ret;
            }
            hCtx->bNeedFindPPS = 0;
            break;
        }
        default:
            break;

    }
    return ret;
}
/***********************************************************************/
/**********************************************************************/
s32 H264DecodeExtraData(H264Dec* h264Dec, H264Context* hCtx, u8* extraDataPtr)
{
    s8 i = 0;
    s8 cnt = 0;
    u8* buf = NULL;
    u16 nalSize = 0;
    u8 bufSrc[4]={0x00, 0x00, 0x00, 0x01};

    buf = hCtx->pExtraDataBuf;
    //store the nal length size, that will be use to parse all other nals
    hCtx->nNalLengthSize = (extraDataPtr[4]&0x03) + 1;
    cnt = *(extraDataPtr+5) & 0x1f;      // Number of pSps

    extraDataPtr += 6;
    while(i<2)
    {
        for(;cnt>0; cnt--)
        {
            buf[0] = 0x00;
            buf[1] = 0x00;
            buf[2] = 0x00;
            buf[3] = 0x01;

            nalSize = AV_RB16(extraDataPtr);

            extraDataPtr += 2;
            //(*MemWrite)(buf+4,extraDataPtr, nalSize);
            CdcMemWrite(h264Dec->memops, buf+4, extraDataPtr, nalSize);
            buf += 4+nalSize;
            extraDataPtr += nalSize;
        }

        cnt = *(extraDataPtr) & 0xff;      // Number of pPps
        extraDataPtr += 1;
        i++;
    }

    hCtx->nExtraDataLen = buf - hCtx->pExtraDataBuf;
    if(hCtx->nExtraDataLen > H264VDEC_MAX_EXTRA_DATA_LEN)
    {
        //LOGD("the hCtx->nExtraDataLen is %d,
        //larger than the H264Vdec_MAX_EXTRA_DATA_LEN\n", hCtx->nExtraDataLen);
    }
    return 0;
}

#if 0
int H264ProcessExtraData2(H264DecCtx* h264DecCtx,  H264Context* hCtx)
{
    // process extra data
    H264Dec* h264Dec = NULL;
    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;
    u32 i = 0;
    s8 cnt = 0;
    s32 nalSize = 0;
    u8 nal_unit_type;

    s32 nRet = 0;
    hCtx->pExtraDataBuf = (u8*)CdcMemPalloc(h264Dec->memops,H264VDEC_MAX_EXTRA_DATA_LEN,
                                        (void *)h264DecCtx->vconfig.veOpsS,
                                        h264DecCtx->vconfig.pVeOpsSelf);
    if(hCtx->pExtraDataBuf == NULL)
    {
        return VDECODE_RESULT_UNSUPPORTED;
    }
    CdcMemSet(h264Dec->memops,hCtx->pExtraDataBuf, 0, H264VDEC_MAX_EXTRA_DATA_LEN);

    CdcMemWrite(h264Dec->memops,hCtx->pExtraDataBuf,
                h264DecCtx->videoStreamInfo.pCodecSpecificData,
                h264DecCtx->videoStreamInfo.nCodecSpecificDataLen);
    hCtx->nExtraDataLen = h264DecCtx->videoStreamInfo.nCodecSpecificDataLen;

    CdcMemFlushCache(h264Dec->memops,hCtx->pExtraDataBuf, H264VDEC_MAX_EXTRA_DATA_LEN);

    u8* pData   = hCtx->pExtraDataBuf;
    u32 nDataSize = hCtx->nExtraDataLen;
    u8* pDataOrg = (u8*)(h264DecCtx->videoStreamInfo.pCodecSpecificData);

    H264SetVbvParams(h264Dec, hCtx, pData,
                        pData+1024-1,nDataSize,
                        FIRST_SLICE_DATA|LAST_SLICE_DATA|SLICE_DATA_VALID);

    CdcVeReset(h264DecCtx->vconfig.veOpsS, h264DecCtx->vconfig.pVeOpsSelf);
    H264InitFuncCtrlRegister(h264DecCtx);

    if(pDataOrg[0] == 0x01)
    {
        hCtx->bIsAvc = 1;

        cnt = *(pDataOrg+5) & 0x1f;      // Number of pSps
        pDataOrg += 6;
        pData += 6;
        while(i<2)
        {
            for(;cnt>0; cnt--)
            {
                nalSize = AV_RB16(pDataOrg);
                pDataOrg += 2;
                pData += 2;
                nal_unit_type = (*pDataOrg) & 0x1f;
                hCtx->nNalRefIdc = ((*pDataOrg) & 0x60) ? 1 : 0;
                nRet = H264DecodeExtraDataNalu(h264DecCtx, hCtx, pData, nal_unit_type, nalSize*8);
                if(nRet != 0)
                    return nRet;
                pDataOrg += nalSize;
                pData += nalSize;

            }
            cnt = *(pDataOrg) & 0xff;      // Number of pPps
            pDataOrg += 1;
            pData += 1;

            i++;
        }
    }
    else
    {
        hCtx->bIsAvc = 0;
        s32 nTemp = -1;


        while(i < nDataSize)
        {
            nTemp <<= 8;
            nTemp |= pDataOrg[i];
            if((nTemp & 0xffffff) == 0x000001)
            {
                i += 1;
                nalSize = H264ParseExtraDataSearchNaluSize((char*)&pDataOrg[i], nDataSize - i);
                if(nalSize < 0)
                    nalSize = nDataSize - i;

                nal_unit_type = pDataOrg[i]& 0x1f;
                hCtx->nNalRefIdc = (pDataOrg[i] & 0x60) ? 1 : 0;
                nRet = H264DecodeExtraDataNalu(h264DecCtx, hCtx, pData+i, nal_unit_type, nalSize*8);
                if(nRet != 0)
                    return nRet;
                i += nalSize;
                continue;
            }
            i++;
        }
    }
    logv("H264ProcessNaluUnit, bNeedFindSPS = %d, bNeedFindPPS = %d",
        hCtx->bNeedFindSPS, hCtx->bNeedFindPPS);

    return VDECODE_RESULT_OK;
}
#else
int H264ProcessExtraData2(H264DecCtx* h264DecCtx,  H264Context* hCtx)
{
    H264Dec* h264Dec = NULL;
    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;
    u32 i = 0;
    s8 cnt = 0;
    s32 nalSize = 0;
    u8 nal_unit_type;
    s32 nRet = 0;
    hCtx->pExtraDataBuf = (u8*)CdcMemPalloc(h264Dec->memops,H264VDEC_MAX_EXTRA_DATA_LEN,
                                        (void *)h264DecCtx->vconfig.veOpsS,
                                        h264DecCtx->vconfig.pVeOpsSelf);
    if(hCtx->pExtraDataBuf == NULL)
    {
        return VDECODE_RESULT_UNSUPPORTED;
    }
    CdcMemSet(h264Dec->memops,hCtx->pExtraDataBuf, 0, H264VDEC_MAX_EXTRA_DATA_LEN);
    CdcMemWrite(h264Dec->memops,hCtx->pExtraDataBuf,
                h264DecCtx->videoStreamInfo.pCodecSpecificData,
                h264DecCtx->videoStreamInfo.nCodecSpecificDataLen);
    hCtx->nExtraDataLen = h264DecCtx->videoStreamInfo.nCodecSpecificDataLen;
    CdcMemFlushCache(h264Dec->memops,hCtx->pExtraDataBuf, H264VDEC_MAX_EXTRA_DATA_LEN);
    u8* pData   = hCtx->pExtraDataBuf;
    u32 nDataSize = hCtx->nExtraDataLen;
    u8* pDataOrg = (u8*)(h264DecCtx->videoStreamInfo.pCodecSpecificData);
    H264SetVbvParams(h264Dec, hCtx, pData,
                        pData+1024-1,nDataSize,
                        FIRST_SLICE_DATA|LAST_SLICE_DATA|SLICE_DATA_VALID);
    CdcVeReset(h264DecCtx->vconfig.veOpsS, h264DecCtx->vconfig.pVeOpsSelf);
    H264InitFuncCtrlRegister(h264DecCtx);
    if(pDataOrg[0] == 0x01)
    {
        hCtx->bIsAvc = 1;
        cnt = *(pDataOrg+5) & 0x1f;      // Number of pSps
        pDataOrg += 6;
        pData += 6;
        while(i<2)
        {
            for(;cnt>0; cnt--)
            {
                nalSize = AV_RB16(pDataOrg);
                pDataOrg += 2;
                pData += 2;
                nal_unit_type = (*pDataOrg) & 0x1f;
                hCtx->nNalRefIdc = ((*pDataOrg) & 0x60) ? 1 : 0;
                nRet = H264DecodeExtraDataNalu(h264DecCtx, hCtx, pData, nal_unit_type, nalSize*8);
                if(nRet != 0)
                    return nRet;
                pDataOrg += nalSize;
                pData += nalSize;
            }
            cnt = *(pDataOrg) & 0xff;      // Number of pPps
            pDataOrg += 1;
            pData += 1;
            i++;
        }
    }
    else
    {
        hCtx->bIsAvc = 0;
        s32 nTemp = -1;
        while(i < nDataSize)
        {
            nTemp <<= 8;
            nTemp |= pDataOrg[i];
            if((nTemp & 0xffffff) == 0x000001)
            {
                i += 1;
                nalSize = H264ParseExtraDataSearchNaluSize((char*)&pDataOrg[i], nDataSize - i);
                if(nalSize < 0)
                    nalSize = nDataSize - i;
                nal_unit_type = pDataOrg[i]& 0x1f;
                hCtx->nNalRefIdc = (pDataOrg[i] & 0x60) ? 1 : 0;
                nRet = H264DecodeExtraDataNalu(h264DecCtx, hCtx, pData+i, nal_unit_type, nalSize*8);
                if(nRet != 0)
                    return nRet;
                i += nalSize;
                continue;
            }
            i++;
        }
    }
    logd("H264ProcessNaluUnit, bNeedFindSPS = %d, bNeedFindPPS = %d",
        hCtx->bNeedFindSPS, hCtx->bNeedFindPPS);
    return VDECODE_RESULT_OK;
}
#endif

static void H264DebugDecoderSaveBitStream(H264Context* hCtx, VideoStreamDataInfo* pStream)
{
    CEDARC_UNUSE(H264DebugDecoderSaveBitStream);
    FILE *fp;
    s32 bTwoDataTrunk, nSize0;

    bTwoDataTrunk = 0;
    if(pStream == NULL)
    {
        loge(" h264 saving data error, stream == NULL");
        return;
    }
    if(pStream->pData == NULL)
    {
        loge(" h264 saving data error, \
            pStream->pData == NULL, BUT pStream->nLength: %d ", pStream->nLength);
        return;
    }

    nSize0 = pStream->nLength;
    if((u8 *)pStream->pData + pStream->nLength > hCtx->vbvInfo.pVbvBufEnd)
    {
        bTwoDataTrunk = 1;
        nSize0 = (s32)(hCtx->vbvInfo.pVbvBufEnd - (u8 *)pStream->pData) + 1;
    }
    fp = fopen(AVC_SAVE_STREAM_PATH, "ab");
    if(fp == NULL)
    {
        logd(" saving h264 stream open file error ");
    }
    else
    {
#if H264_DEBUG_SAVE_BIT_STREAM_ES
        logd(" saving h264 stream data. size: %d ", pStream->nLength);
        if(bTwoDataTrunk)
        {
            logd(" two data trunk, size0: %d, size1: %d ",
                nSize0, pStream->nLength - nSize0);
            fwrite(pStream->pData, 1, nSize0, fp);
            fwrite(hCtx->vbvInfo.pVbvBuf, 1, pStream->nLength - nSize0, fp);
        }
        else
        {
            logd("pStream->pData: %p, pStream->nLength: %d",
                pStream->pData, pStream->nLength);
            fwrite(pStream->pData, 1, pStream->nLength, fp);
        }
#else
        s32 nSavedSize = 0;
        char* pCurrentData = pStream->pData;
        char startcode[4] = {0, 0, 0, 1};
        char* pNaluBuffer = NULL;
        if(bTwoDataTrunk)
        {
            pNaluBuffer = calloc(1, pStream->nLength);
            logd(" two data trunk, size0: %d, size1: %d ",
                nSize0, pStream->nLength - nSize0);
            memcpy(pNaluBuffer, pStream->pData, nSize0);
            memcpy(pNaluBuffer + nSize0, hCtx->vbvInfo.pVbvBuf, pStream->nLength - nSize0);
            pCurrentData = pNaluBuffer;
        }

        logd("pStream->pData: %p, pStream->nLength: %d",
            pStream->pData, pStream->nLength);
        while(nSavedSize < pStream->nLength)
        {
            s32 nCurrentSize = 0;
            pCurrentData += nSavedSize;
            nCurrentSize = pCurrentData[0];
            nCurrentSize <<= 8;
            nCurrentSize |= pCurrentData[1];
            nCurrentSize <<= 8;
            nCurrentSize |= pCurrentData[2];
            nCurrentSize <<= 8;
            nCurrentSize |= pCurrentData[3];
            if(nCurrentSize < 0 || nCurrentSize > (1024*1024))
            {
                loge(" saving stream data, current slice size(%d) is out of range ",
                    nCurrentSize);
                break;
            }
            fwrite(startcode, 1, 4, fp);
            fwrite(pCurrentData+4, 1, nCurrentSize, fp);
            nSavedSize += 4;
            nSavedSize += nCurrentSize;
        }
        if(pNaluBuffer != NULL)
            free(pNaluBuffer);
#endif
        fclose(fp);
    }
}


void H264SetPicBufferInfoToVbvParams(H264Context* hCtx, FramePicInfo*   pFramePic)
{
    hCtx->vbvInfo.pVbvDataStartPtr = (u8*)(pFramePic->pSbmFrameBufferManager->pFrameBuffer);
    hCtx->vbvInfo.pVbvBuf = (u8*)(pFramePic->pSbmFrameBufferManager->pFrameBuffer);

    hCtx->vbvInfo.pVbvBufEnd = (u8*)(pFramePic->pSbmFrameBufferManager->pFrameBuffer+
          pFramePic->pSbmFrameBufferManager->nFrameBufferSize-1);
    hCtx->vbvInfo.pReadPtr   = (u8*)(pFramePic->pSbmFrameBufferManager->pFrameBuffer);
    hCtx->vbvInfo.nVbvDataSize = pFramePic->nlength;

    hCtx->vbvInfo.pVbvDataEndPtr = hCtx->vbvInfo.pVbvBuf+hCtx->vbvInfo.nVbvDataSize-1;
    hCtx->vbvInfo.bVbvDataCtrlFlag = 7;
    hCtx->vbvInfo.nVbvBufPhyAddr = pFramePic->pSbmFrameBufferManager->pPhyFrameBuffer;
    hCtx->vbvInfo.nVbvBufEndPhyAddr = pFramePic->pSbmFrameBufferManager->pPhyFrameBufferEnd;
}

s32 H264RequestOneFrameStream(H264DecCtx* h264DecCtx, H264Context* hCtx)
{
    s64 nDiffPts = 0;
    u32 nLastDecStep = 0;
    VideoStreamDataInfo *pStream = NULL;
    FramePicInfo*        pFramePic = NULL;
    H264Dec* h264Dec = NULL;
    int i;
    u8 flag1 = 0;
    u8 flag2 = 0;
    s64 diffPts = 0;
    struct ScMemOpsS *_memops     = NULL;
    s32 nValidSize = 0;
    _memops = h264DecCtx->vconfig.memops;
    int num;
request_stream:

    if(hCtx->pCurStreamFrame)
    {
        SbmFlushStream(hCtx->vbvInfo.vbv, (VideoStreamDataInfo*)hCtx->pCurStreamFrame);
        hCtx->pCurStreamFrame = NULL;
    }
    pStream = SbmRequestStream(hCtx->vbvInfo.vbv);
    if(pStream == NULL)
    {
        if(hCtx->bEndOfStream == 1)
        {
            nValidSize = SbmStreamDataSize(hCtx->vbvInfo.vbv);
            if(nValidSize == 0)
            {
                H264FlushDelayedPictures(h264DecCtx, hCtx);
                return VDECODE_RESULT_NO_BITSTREAM;
            }
            else
                return VDECODE_RESULT_CONTINUE;
        }
        return VDECODE_RESULT_NO_BITSTREAM;
    }

    pFramePic = (FramePicInfo*)pStream;

    if(pFramePic->bValidFlag == 0)
    {
        SbmFlushStream(hCtx->vbvInfo.vbv, (VideoStreamDataInfo*)pFramePic);
        goto request_stream;
    }

    hCtx->pCurStreamFrame = pFramePic;
    if(pFramePic->bVideoInfoFlag==1)
    {
        h264Dec = (H264Dec*)h264DecCtx->pH264Dec;
        pFramePic->bVideoInfoFlag = 0;
        SbmReturnStream(hCtx->vbvInfo.vbv, (VideoStreamDataInfo*)pFramePic);
        hCtx->pCurStreamFrame = NULL;

        if(h264Dec->pHContext != NULL)
        {
            H264FlushDelayedPictures(h264DecCtx, h264Dec->pHContext);
        }
        if(h264Dec->pMinorHContext != NULL)
        {
            H264FlushDelayedPictures(h264DecCtx, h264Dec->pMinorHContext);
        }
        logd("the_111,VDECODE_RESULT_RESOLUTION_CHANGE");
        return VDECODE_RESULT_RESOLUTION_CHANGE;
    }

#if H264_DEBUG_SAVE_BIT_STREAM
        FILE *fp = fopen("/data/camera/avc_bit_stream.txt", "ab");
        if(fp == NULL)
        {
            loge("***fp open fail!");
            abort();
        }
        logd("fp open success!!!");
        if((u8*)pFramePic->pDataStartAddr + pFramePic->nlength > hCtx->vbvInfo.pVbvBufEnd)
        {
            int cusSize = hCtx->vbvInfo.pVbvBufEnd - (u8*)pFramePic->pDataStartAddr + 1;
            fwrite(pFramePic->pDataStartAddr, 1, cusSize, fp);
            fwrite(hCtx->vbvInfo.pVbvBuf, 1, pFramePic->nlength - cusSize, fp);
        }
        else
        {
            logd("gqy*** length = %d", pFramePic->nlength);
            fwrite(pFramePic->pDataStartAddr, 1, pFramePic->nlength, fp);
        }
        fclose(fp);
#endif

    if((hCtx->bDecodeKeyFrameOnly==0)&&
        (pFramePic->nPts!= H264VDEC_ERROR_PTS_VALUE)&&
        (hCtx->vbvInfo.nLastValidPts>0))
    {
        nDiffPts = pFramePic->nPts-hCtx->vbvInfo.nLastValidPts;
    }
    hCtx->vbvInfo.nVbvDataSize = pFramePic->nlength;
    hCtx->vbvInfo.pVbvDataStartPtr = (u8*)pFramePic->pDataStartAddr;
    hCtx->vbvInfo.pReadPtr = (u8*)pFramePic->pDataStartAddr;
    hCtx->vbvInfo.nValidDataPts  = H264VDEC_ERROR_PTS_VALUE;
    #if 0
    if((pFramePic->nPts!=H264VDEC_ERROR_PTS_VALUE) &&
        (hCtx->vbvInfo.nVbvDataPts!=pFramePic->nPts))
    {
        hCtx->vbvInfo.nLastValidPts =  pFramePic->nPts;
        hCtx->vbvInfo.nValidDataPts = pFramePic->nPts;
    }
    #else
    if(pFramePic->nPts!=H264VDEC_ERROR_PTS_VALUE)
    {
        flag1 = (hCtx->vbvInfo.nVbvDataPts!=pFramePic->nPts);
        flag2 = (flag1==0) &&(pFramePic->nPts!=hCtx->vbvInfo.nLastUsedValidPts);
        if(flag1 || flag2)
        {
            hCtx->vbvInfo.nLastValidPts =  pFramePic->nPts;
            hCtx->vbvInfo.nValidDataPts = pFramePic->nPts;
        }
    }
    #endif
    hCtx->vbvInfo.nVbvDataPts  = pFramePic->nPts;

    hCtx->vbvInfo.nCurNaluIdx = 0;
    logv("******had reqeust stream-frame-pic");

    if(hCtx->pVbv->bUseNewVeMemoryProgram == 1)
    {
        H264SetPicBufferInfoToVbvParams(hCtx, pFramePic);
    }
    return 0;

}


/***********************************************************************/

s8 H264ComputeScaleRatio(u32 orgSize, u32 dstSize)
{
    CEDARC_UNUSE(H264ComputeScaleRatio);
    u8 scaleRatio = 0;

    if(dstSize == 0)
    {
        scaleRatio = 0;
    }
    else if(orgSize > dstSize)
    {
        scaleRatio = orgSize / dstSize;
        switch(scaleRatio)
        {
            case 0:
            case 1:
            case 2:
            {
                scaleRatio = 1;
                break;
            }
            case 3:
            case 4:
            {
                scaleRatio = 2;
                break;
            }
            default:
            {
                scaleRatio = 2; //* 1/8 scale down is not support.
                break;
            }
        }
    }
    return scaleRatio;
}

void H264ComputeOffset(H264DecCtx* h264DecCtx, H264Context* hCtx,u32 width, u32 height)
{
    u32 nCropBottom = 0;
    u32 nCropLeft = 0;
    u32 nCropTop = 0;
    u32 nCropRight = 0;

    nCropBottom = hCtx->nCropBottom;
    nCropLeft   = hCtx->nCropLeft;
    nCropTop    = hCtx->nCropTop;
    nCropRight  = hCtx->nCropRight;

    if(h264DecCtx->vconfig.nRotateDegree == 2)
     {
         hCtx->nCropTop    = nCropBottom;
        hCtx->nCropRight  = nCropLeft;
        hCtx->nCropBottom = nCropTop;
        hCtx->nCropLeft   = nCropRight;
    }
    else if(h264DecCtx->vconfig.nRotateDegree==1)
    {
         hCtx->nCropTop    = nCropLeft;
        hCtx->nCropRight  = nCropTop;
        hCtx->nCropBottom = nCropRight;
        hCtx->nCropLeft   = nCropBottom;
    }
    else if(h264DecCtx->vconfig.nRotateDegree==3)
    {
        hCtx->nCropTop    = nCropRight;
        hCtx->nCropRight  = nCropBottom;
        hCtx->nCropBottom = nCropLeft;
        hCtx->nCropLeft   = nCropTop;
    }

    if(h264DecCtx->vconfig.nHorizonScaleDownRatio != 0)
    {
        hCtx->nLeftOffset >>= h264DecCtx->vconfig.nHorizonScaleDownRatio;
        hCtx->nRightOffset >>= h264DecCtx->vconfig.nHorizonScaleDownRatio;
        hCtx->nCropLeft >>= h264DecCtx->vconfig.nHorizonScaleDownRatio;
        hCtx->nCropRight >>= h264DecCtx->vconfig.nHorizonScaleDownRatio;
    }
    if(h264DecCtx->vconfig.nVerticalScaleDownRatio != 0)
    {
        hCtx->nTopOffset >>= h264DecCtx->vconfig.nVerticalScaleDownRatio;
        hCtx->nBottomOffset >>= h264DecCtx->vconfig.nVerticalScaleDownRatio;
        hCtx->nCropTop >>= h264DecCtx->vconfig.nVerticalScaleDownRatio;
        hCtx->nCropBottom >>= h264DecCtx->vconfig.nVerticalScaleDownRatio;
    }

    hCtx->nRightOffset = width-hCtx->nRightOffset-hCtx->nCropRight;
    hCtx->nBottomOffset = height-hCtx->nBottomOffset-hCtx->nCropBottom;
    hCtx->nTopOffset += hCtx->nCropTop;
    hCtx->nLeftOffset += hCtx->nCropLeft;

    logv("2. nCropTop=%d, nCropLeft=%d, nCropRight=%d, nCropBottom=%d\n",
        hCtx->nCropTop,hCtx->nCropLeft, hCtx->nCropRight,
        hCtx->nCropBottom);

    logv("nTopOffset=%d, nLeftOffset=%d, nRightOffset=%d, nBottomOffset=%d\n",
        hCtx->nTopOffset,hCtx->nLeftOffset, hCtx->nRightOffset,
        hCtx->nBottomOffset);

}

static s32 H264MallocBuffer(H264DecCtx* h264DecCtx, H264Context* hCtx)
{
    s32 i = 0;
    u32 width = 0;
    u32 height = 0;
    u32 validFrmNum = 0;
    u32 fieldMvColBufSize= 0;
    //u32 mafBufSize = 0;
    //u8* mptr = NULL;
    //u32 nMbXNum = 0;
    //u32 nMbYNum = 0;
    //u32 j = 0;
    //u32 k = 0;
    u32 nRefFrameFormat = 0;
    u32 nDispFrameFormat = 0;
    H264Dec* h264Dec = NULL;
    u8 nAlignValue = 0;
    u32 nExtraBufNum = 7;
    FbmCreateInfo mFbmCreateInfo;
    VideoFbmInfo *pFbmInfo;
    s32 flag1 = 0;
    s32 flag2 = 0;

    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;

    flag1 = (h264Dec->H264ExtraScaleInf.nHorizonScaleRatio>0) &&
            (h264Dec->H264ExtraScaleInf.nHorizonScaleRatio<=3);
    flag2 = (h264Dec->H264ExtraScaleInf.nVerticalScaleRatio>0) &&
            (h264Dec->H264ExtraScaleInf.nVerticalScaleRatio<=3);

    width = (h264Dec->H264ExtraScaleInf.nWidthTh+63)&~63;
    height = (h264Dec->H264ExtraScaleInf.nHeightTh+63)&~63;

    if((flag1==1) || (flag2==1))
    {
        if((h264DecCtx->vconfig.nHorizonScaleDownRatio==0) &&
            (h264DecCtx->vconfig.nVerticalScaleDownRatio==0))
        {
            if(hCtx->nFrmMbWidth>=width||hCtx->nFrmMbHeight>=height)
            {
                h264DecCtx->vconfig.bScaleDownEn = 1;
                h264DecCtx->vconfig.nHorizonScaleDownRatio =
                    h264Dec->H264ExtraScaleInf.nHorizonScaleRatio;
                h264DecCtx->vconfig.nVerticalScaleDownRatio =
                    h264Dec->H264ExtraScaleInf.nVerticalScaleRatio;
            }
        }
    }

    h264DecCtx->vconfig.bRotationEn = 0;
    if(h264DecCtx->vconfig.nRotateDegree > 0)
    {
        h264DecCtx->vconfig.bRotationEn = 1;
    }
    if(h264DecCtx->vconfig.nHorizonScaleDownRatio!=0 ||
        h264DecCtx->vconfig.nVerticalScaleDownRatio!=0)
    {
        h264DecCtx->vconfig.bScaleDownEn = 1;
    }

    if(h264DecCtx->vconfig.eOutputPixelFormat == PIXEL_FORMAT_YV12)
    {
        hCtx->bRefYv12DispYv12Flag = 1;
        nRefFrameFormat = PIXEL_FORMAT_YV12;
        nDispFrameFormat = PIXEL_FORMAT_YV12;
    }
    else if(h264DecCtx->vconfig.eOutputPixelFormat == PIXEL_FORMAT_NV21)
    {
        hCtx->bRefNv21DispNv21Flag = 1;
        nRefFrameFormat = PIXEL_FORMAT_NV21;
        nDispFrameFormat = PIXEL_FORMAT_NV21;

    }
    else
    {
        hCtx->bRefYv12DispYv12Flag = 1;
        nRefFrameFormat = PIXEL_FORMAT_YV12;
        nDispFrameFormat = PIXEL_FORMAT_YV12;
    }

    if(h264DecCtx->vconfig.bRotationEn==1 ||
        h264DecCtx->vconfig.bScaleDownEn==1 ||
        hCtx->bYv12OutFlag==1 ||
        hCtx->bNv21OutFlag==1)
    {
        hCtx->bUseMafFlag = 0;
        h264DecCtx->vconfig.bSupportMaf = 0;
    }

    width = hCtx->nFrmMbWidth;
    height = hCtx->nFrmMbHeight;

    hCtx->pFbm  = NULL;
    hCtx->pFbmScaledown = NULL;

    pFbmInfo = (VideoFbmInfo *)h264DecCtx->pFbmInfo;

    nExtraBufNum = h264DecCtx->vconfig.nDisplayHoldingFrameBufferNum +
        h264DecCtx->vconfig.nRotateHoldingFrameBufferNum +
        h264DecCtx->vconfig.nDecodeSmoothFrameBufferNum;

    if(hCtx->bProgressice == 0)
        nExtraBufNum += h264DecCtx->vconfig.nDeInterlaceHoldingFrameBufferNum;
    if(hCtx->pFbm == NULL)
    {
        if((h264DecCtx->vconfig.bScaleDownEn==0) &&
            (h264DecCtx->vconfig.bRotationEn==0) &&
            (hCtx->bYv12OutFlag==0) &&
            (hCtx->bNv21OutFlag==0))
        {
            memset(&mFbmCreateInfo, 0, sizeof(FbmCreateInfo));
            mFbmCreateInfo.nFrameNum          =
                hCtx->nRefFrmCount + pFbmInfo->nDecoderNeededMiniFbmNum + nExtraBufNum;
            mFbmCreateInfo.nDecoderNeededMiniFrameNum =
                hCtx->nRefFrmCount + pFbmInfo->nDecoderNeededMiniFbmNum;
            mFbmCreateInfo.nWidth             = width;
            mFbmCreateInfo.nHeight            = height;
            mFbmCreateInfo.ePixelFormat       = nRefFrameFormat;
            mFbmCreateInfo.bThumbnailMode     = h264DecCtx->vconfig.bThumbnailMode;
           // mFbmCreateInfo.callback           = h264DecCtx->vconfig.callback;
           // mFbmCreateInfo.pUserData          = h264DecCtx->vconfig.pUserData;
            mFbmCreateInfo.bGpuBufValid       = h264DecCtx->vconfig.bGpuBufValid;
            mFbmCreateInfo.nAlignStride       = h264DecCtx->vconfig.nAlignStride;
            mFbmCreateInfo.nBufferType        = BUF_TYPE_REFERENCE_DISP;
            mFbmCreateInfo.bProgressiveFlag   = hCtx->bProgressice;
            mFbmCreateInfo.bIsSoftDecoderFlag = h264DecCtx->vconfig.bIsSoftDecoderFlag;
            mFbmCreateInfo.memops             = h264DecCtx->vconfig.memops;
            mFbmCreateInfo.veOpsS             = h264DecCtx->vconfig.veOpsS;
            mFbmCreateInfo.pVeOpsSelf         = h264DecCtx->vconfig.pVeOpsSelf;

            H264ComputeOffset(h264DecCtx, hCtx, width, height);
            h264DecCtx->pFbmInfo->pFbmBufInfo.nTopOffset = hCtx->nTopOffset;;
            h264DecCtx->pFbmInfo->pFbmBufInfo.nLeftOffset = hCtx->nLeftOffset;
            h264DecCtx->pFbmInfo->pFbmBufInfo.nRightOffset = hCtx->nRightOffset;
            h264DecCtx->pFbmInfo->pFbmBufInfo.nBottomOffset = hCtx->nBottomOffset;

            hCtx->pFbm = FbmCreate(&mFbmCreateInfo, h264DecCtx->pFbmInfo);
            if(hCtx->pFbm == NULL)
            {
                return VDECODE_RESULT_UNSUPPORTED;
            }

            nAlignValue = FbmGetAlignValue(hCtx->pFbm);

            GetBufferSize((int)nRefFrameFormat,(int)width,
                (int)height,(int*)NULL, (int*)&hCtx->nRefCSize,
                (int*)&hCtx->nRefYLineStride, (int*)&hCtx->nRefCLineStride, nAlignValue);
            hCtx->nDispCSize = hCtx->nRefCSize;
            hCtx->nDispYLineStride = hCtx->nRefYLineStride;
            hCtx->nDispCLineStride = hCtx->nRefCLineStride;

            validFrmNum = mFbmCreateInfo.nFrameNum;

            #if 0
            if(hCtx->bUseMafFlag == 1)
            {
                for(i = 0; i<(s32)validFrmNum; i++)
                {
                    hCtx->pFbm->pFrames[i].vpicture.pData3 =
                        (char*)CdcMemPalloc(h264DecCtx->vconfig.memops,hCtx->nMafBufferSize,
                                            (void *)h264DecCtx->vconfig.veOpsS,
                                            h264DecCtx->vconfig.pVeOpsSelf);
                    if(hCtx->pFbm->pFrames[i].vpicture.pData3 == NULL)
                    {
                        logd("AdapterMemPalloc for mafBuf failed!!");
                        hCtx->bUseMafFlag = 0;
                        return VDECODE_RESULT_UNSUPPORTED;
                    }
                    CdcMemFlushCache(h264DecCtx->vconfig.memops,\
                        hCtx->pFbm->pFrames[i].vpicture.pData3, hCtx->nMafBufferSize);
                }
            }
            #endif
        }
        else
        {
            int nExtraBufNumSD = 0;
            hCtx->bUseMafFlag = 0;
            memset(&mFbmCreateInfo, 0, sizeof(FbmCreateInfo));
            mFbmCreateInfo.nFrameNum          =
                hCtx->nRefFrmCount + pFbmInfo->nDecoderNeededMiniFbmNum;
            mFbmCreateInfo.nDecoderNeededMiniFrameNum =
                hCtx->nRefFrmCount + pFbmInfo->nDecoderNeededMiniFbmNum;
            mFbmCreateInfo.nWidth             = width;
            mFbmCreateInfo.nHeight            = height;
            mFbmCreateInfo.ePixelFormat       = nRefFrameFormat;
            mFbmCreateInfo.bThumbnailMode     = h264DecCtx->vconfig.bThumbnailMode;
            //mFbmCreateInfo.callback           = h264DecCtx->vconfig.callback;
            //mFbmCreateInfo.pUserData          = h264DecCtx->vconfig.pUserData;
            mFbmCreateInfo.bGpuBufValid       = h264DecCtx->vconfig.bGpuBufValid;
            mFbmCreateInfo.nAlignStride       = h264DecCtx->vconfig.nAlignStride;
            mFbmCreateInfo.nBufferType        = BUF_TYPE_ONLY_REFERENCE;
            mFbmCreateInfo.bProgressiveFlag   = hCtx->bProgressice;
            mFbmCreateInfo.bIsSoftDecoderFlag = h264DecCtx->vconfig.bIsSoftDecoderFlag;
            mFbmCreateInfo.memops             = h264DecCtx->vconfig.memops;
            mFbmCreateInfo.veOpsS             = h264DecCtx->vconfig.veOpsS;
            mFbmCreateInfo.pVeOpsSelf         = h264DecCtx->vconfig.pVeOpsSelf;

            hCtx->pFbm = FbmCreate(&mFbmCreateInfo, h264DecCtx->pFbmInfo);
            if(hCtx->pFbm == NULL)
            {
                return VDECODE_RESULT_UNSUPPORTED;
            }
            nAlignValue = FbmGetAlignValue(hCtx->pFbm);
            GetBufferSize((int)nRefFrameFormat,(int)width,(int)height,
                (int*)NULL, (int*)&hCtx->nRefCSize, (int*)&hCtx->nRefYLineStride,
                (int*)&hCtx->nRefCLineStride, nAlignValue);

            if(h264DecCtx->vconfig.nRotateDegree==1 || h264DecCtx->vconfig.nRotateDegree==3)
            {
                width =  hCtx->nFrmMbHeight;
                height = hCtx->nFrmMbWidth;
            }

            width >>= h264DecCtx->vconfig.nHorizonScaleDownRatio;
            height >>= h264DecCtx->vconfig.nVerticalScaleDownRatio;

            nExtraBufNumSD = pFbmInfo->nDecoderNeededMiniFbmNumSD;
            if(hCtx->bProgressice == 0)
                nExtraBufNumSD += h264DecCtx->vconfig.nDeInterlaceHoldingFrameBufferNum;
            memset(&mFbmCreateInfo, 0, sizeof(FbmCreateInfo));
            logw(" h264 scale down fbm buffer number need double check!  ");
            mFbmCreateInfo.nFrameNum          = nExtraBufNumSD;
            mFbmCreateInfo.nDecoderNeededMiniFrameNum =
                hCtx->nRefFrmCount + pFbmInfo->nDecoderNeededMiniFbmNum;
            mFbmCreateInfo.nWidth             = width;
            mFbmCreateInfo.nHeight            = height;
            mFbmCreateInfo.ePixelFormat       = nDispFrameFormat;
            mFbmCreateInfo.bThumbnailMode     = h264DecCtx->vconfig.bThumbnailMode;
            //mFbmCreateInfo.callback           = h264DecCtx->vconfig.callback;
            //mFbmCreateInfo.pUserData          = h264DecCtx->vconfig.pUserData;
            mFbmCreateInfo.bGpuBufValid       = h264DecCtx->vconfig.bGpuBufValid;
            mFbmCreateInfo.nAlignStride       = h264DecCtx->vconfig.nAlignStride;
            mFbmCreateInfo.nBufferType        = BUF_TYPE_ONLY_DISP;
            mFbmCreateInfo.bProgressiveFlag   = hCtx->bProgressice;
            mFbmCreateInfo.bIsSoftDecoderFlag = h264DecCtx->vconfig.bIsSoftDecoderFlag;
            mFbmCreateInfo.memops             = h264DecCtx->vconfig.memops;
            mFbmCreateInfo.veOpsS             = h264DecCtx->vconfig.veOpsS;
            mFbmCreateInfo.pVeOpsSelf         = h264DecCtx->vconfig.pVeOpsSelf;

            H264ComputeOffset(h264DecCtx, hCtx, width, height);
            h264DecCtx->pFbmInfo->pFbmBufInfo.nTopOffset = hCtx->nTopOffset;;
            h264DecCtx->pFbmInfo->pFbmBufInfo.nLeftOffset = hCtx->nLeftOffset;
            h264DecCtx->pFbmInfo->pFbmBufInfo.nRightOffset = hCtx->nRightOffset;
            h264DecCtx->pFbmInfo->pFbmBufInfo.nBottomOffset = hCtx->nBottomOffset;

            hCtx->pFbmScaledown = FbmCreate(&mFbmCreateInfo, h264DecCtx->pFbmInfo);
            if(hCtx->pFbmScaledown == NULL)
            {
                return VDECODE_RESULT_UNSUPPORTED;
            }


            if(nExtraBufNumSD > (int)(hCtx->nRefFrmCount +
                pFbmInfo->nDecoderNeededMiniFbmNum + nExtraBufNum))
                validFrmNum = nExtraBufNumSD;
            else
                validFrmNum = hCtx->nRefFrmCount +
                pFbmInfo->nDecoderNeededMiniFbmNum + nExtraBufNum;

            nAlignValue = FbmGetAlignValue(hCtx->pFbmScaledown);
            GetBufferSize((int)nDispFrameFormat,
                (int)width,(int)height,(int*)NULL,
                (int*)&hCtx->nDispCSize,
                (int*)&hCtx->nDispYLineStride,
                (int*)&hCtx->nDispCLineStride,
                nAlignValue);
        }
    }
    if(hCtx->pFbm == NULL)
    {
        return VDECODE_RESULT_UNSUPPORTED;
    }

    //****************************construct_fbm*******************************************//
    hCtx->frmBufInf.nMaxValidFrmBufNum = validFrmNum;
    memset(hCtx->frmBufInf.picture, 0, sizeof(H264PicInfo)*MAX_PICTURE_COUNT);

    fieldMvColBufSize = hCtx->nMbHeight*(2-hCtx->bFrameMbsOnlyFlag);
    fieldMvColBufSize = (fieldMvColBufSize+1)/2;
    fieldMvColBufSize = hCtx->nMbWidth*fieldMvColBufSize*32;

    if(hCtx->bDirect8x8InferenceFlag == 0)
    {
        fieldMvColBufSize *= 2;
    }
    hCtx->frmBufInf.pMvColBuf = NULL;
    fieldMvColBufSize = (fieldMvColBufSize + 1023) & (~1023);
    if((hCtx->pVbv->bUseNewVeMemoryProgram==0)||!ENABLE_MV_BUF_OPTIMIZATION_PROGRAM)
    {
        for(i=0+hCtx->nBufIndexOffset;
            i<hCtx->frmBufInf.nMaxValidFrmBufNum+hCtx->nBufIndexOffset; i++)
        {
            hCtx->frmBufInf.picture[i].pTopMvColBuf =
                (u8 *)CdcMemPalloc(h264DecCtx->vconfig.memops,fieldMvColBufSize*2,
                                                (void *)h264DecCtx->vconfig.veOpsS,
                                                h264DecCtx->vconfig.pVeOpsSelf);
            if(hCtx->frmBufInf.picture[i].pTopMvColBuf == NULL)
            {
                loge(" h264 palloc fail, picture[%d].pTopMvColBuf, \
                        size: %d; total picture: %d", \
                        i, 2*2*fieldMvColBufSize, \
                        hCtx->frmBufInf.nMaxValidFrmBufNum+hCtx->nBufIndexOffset);
                return VDECODE_RESULT_UNSUPPORTED;
            }
            hCtx->frmBufInf.picture[i].phyTopMvColBuf = (size_addr)CdcMemGetPhysicAddress
                (h264DecCtx->vconfig.memops,hCtx->frmBufInf.picture[i].pTopMvColBuf);

            logv("phyTopMvColBuf: vir=%p, phy=%x\n",
                hCtx->frmBufInf.picture[i].pTopMvColBuf,
                (unsigned int)hCtx->frmBufInf.picture[i].phyTopMvColBuf);

            hCtx->frmBufInf.picture[i].pBottomMvColBuf =
                hCtx->frmBufInf.picture[i].pTopMvColBuf + fieldMvColBufSize;

            hCtx->frmBufInf.picture[i].phyBottomMvColBuf =
                hCtx->frmBufInf.picture[i].phyTopMvColBuf+fieldMvColBufSize;
            logv("phyBottomMvColBuf: vir=%p, phy=%x\n",
                hCtx->frmBufInf.picture[i].pBottomMvColBuf,
                (unsigned int)hCtx->frmBufInf.picture[i].phyBottomMvColBuf);

            hCtx->frmBufInf.picture[i].pVPicture = NULL;
        }
    }
    else
    {
        hCtx->pH264MvBufInf = (H264MvBufInf*)malloc((hCtx->nRefFrmCount+1)*sizeof(H264MvBufInf));
        if(hCtx->pH264MvBufInf == NULL)
        {
            loge("malloc buffer for hCtx->pH264MvBufInf failed\n");
            return VDECODE_RESULT_UNSUPPORTED;
        }
        hCtx->nH264MvBufNum = hCtx->nRefFrmCount+1;

        for(i=0; i<hCtx->nRefFrmCount+1; i++)
        {
            hCtx->pH264MvBufInf[i].pH264MvBufManager = \
                  (H264MvBufManager*)malloc(sizeof(H264MvBufManager));
            if(hCtx->pH264MvBufInf[i].pH264MvBufManager == NULL)
            {
                return VDECODE_RESULT_UNSUPPORTED;
            }
            hCtx->pH264MvBufInf[i].pH264MvBufManager->nH264MvBufIndex = i;
            hCtx->pH264MvBufInf[i].pH264MvBufManager->pTopMvColBuf =
                (u8 *)CdcMemPalloc(h264DecCtx->vconfig.memops,fieldMvColBufSize*2,
                                                (void *)h264DecCtx->vconfig.veOpsS,
                                                h264DecCtx->vconfig.pVeOpsSelf);
            if(hCtx->pH264MvBufInf[i].pH264MvBufManager->pTopMvColBuf == NULL)
            {
                loge(" h264 palloc fail, picture[%d].pTopMvColBuf, \
                        size: %d; total picture: %d", \
                        i, 2*2*fieldMvColBufSize, \
                        hCtx->nRefFrmCount+1);
                return VDECODE_RESULT_UNSUPPORTED;
            }
            hCtx->pH264MvBufInf[i].pH264MvBufManager->phyTopMvColBuf = \
                (size_addr)CdcMemGetPhysicAddress  \
                (h264DecCtx->vconfig.memops, hCtx->pH264MvBufInf[i].pH264MvBufManager->pTopMvColBuf);
            hCtx->pH264MvBufInf[i].pH264MvBufManager->pBottomMvColBuf =
                hCtx->pH264MvBufInf[i].pH264MvBufManager->pTopMvColBuf + fieldMvColBufSize;

            hCtx->pH264MvBufInf[i].pH264MvBufManager->phyBottomMvColBuf =
                hCtx->pH264MvBufInf[i].pH264MvBufManager->phyTopMvColBuf+fieldMvColBufSize;

            logv("phyTopMvColBuf: vir=%p, phy=%x\n",
                hCtx->pH264MvBufInf[i].pH264MvBufManager->pTopMvColBuf,
                (unsigned int)hCtx->pH264MvBufInf[i].pH264MvBufManager->phyTopMvColBuf);
            logv("phyBottomMvColBuf: vir=%p, phy=%x\n",
                hCtx->pH264MvBufInf[i].pH264MvBufManager->pBottomMvColBuf,
                (unsigned int)hCtx->pH264MvBufInf[i].pH264MvBufManager->phyBottomMvColBuf);
            hCtx->pH264MvBufInf[i].pH264MvBufManager->nCalculateNum = INIT_MV_BUF_CALCULATE_NUM;
            FIFOEnqueue((FiFoQueueInst**)&(hCtx->pH264MvBufEmptyQueue), \
               (FiFoQueueInst*)&(hCtx->pH264MvBufInf[i]));
        }
    }

    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;

    if(hCtx->nFrmMbWidth > 2048)
    {
        hCtx->bUseDramBufFlag = 1;
        hCtx->pDeblkDramBuf = (u8 *)CdcMemPalloc(h264DecCtx->vconfig.memops,
                                            (hCtx->nMbWidth+31)*16*12,
                                            (void *)h264DecCtx->vconfig.veOpsS,
                                            h264DecCtx->vconfig.pVeOpsSelf);
        if(hCtx->pDeblkDramBuf == NULL)
        {
            return VDECODE_RESULT_UNSUPPORTED;
        }
        hCtx->phyDeblkDramBuf  = (size_addr)CdcMemGetPhysicAddress(h264DecCtx->vconfig.memops,
                                hCtx->pDeblkDramBuf);
        logv("phyDeblkDramBuf:vir=%p, phy=%x\n",
              hCtx->pDeblkDramBuf,(unsigned int)hCtx->phyDeblkDramBuf);


        hCtx->pIntraPredDramBuf = (u8 *)CdcMemPalloc(h264DecCtx->vconfig.memops,
                                            (hCtx->nMbWidth+63)*16*5,
                                            (void *)h264DecCtx->vconfig.veOpsS,
                                            h264DecCtx->vconfig.pVeOpsSelf);
        if(hCtx->pIntraPredDramBuf == NULL)
        {
            return VDECODE_RESULT_UNSUPPORTED;
        }

        hCtx->phyIntraPredDramBuf  = (size_addr)CdcMemGetPhysicAddress(h264DecCtx->vconfig.memops,
            hCtx->pIntraPredDramBuf);
        logv("phyIntraPredDramBuf:vir=%p, phy=%x\n",
              hCtx->pIntraPredDramBuf,(unsigned int)hCtx->phyIntraPredDramBuf);

        CdcMemFlushCache(h264DecCtx->vconfig.memops,
            hCtx->pDeblkDramBuf, (hCtx->nMbWidth+31)*16*12);
        CdcMemFlushCache(h264DecCtx->vconfig.memops,
            hCtx->pIntraPredDramBuf, (hCtx->nMbWidth+63)*16*5);
    }
    return VDECODE_RESULT_OK;
}

s32 H264MallocFrmBuffer(H264DecCtx* h264DecCtx, H264Context* hCtx)
{
    s32 ret = 0;
    u32 size = 0;

    if(h264DecCtx->videoStreamInfo.bIs3DStream == 1)
        hCtx->bUseMafFlag  = 0;

    #if 1
    if(hCtx->nFrmMbWidth < 2048)
    {
        size = (hCtx->nRefFrmCount+2)*0x1000+hCtx->nMbHeight*(2-hCtx->bFrameMbsOnlyFlag)*64;
    }
    else
    {
        size = (hCtx->nRefFrmCount+2)*0x4000+hCtx->nMbHeight*(2-hCtx->bFrameMbsOnlyFlag)*64;
    }

    if(size < (0x20000+2*1024))
    {
        size = 0x20000+2*1024;
    }


    hCtx->pMbFieldIntraBuf = CdcMemPalloc(h264DecCtx->vconfig.memops,size,
                                            (void *)h264DecCtx->vconfig.veOpsS,
                                            h264DecCtx->vconfig.pVeOpsSelf);
    if(hCtx->pMbFieldIntraBuf == NULL)
    {
        return VDECODE_RESULT_UNSUPPORTED;
    }
    hCtx->phyMbFieldIntraBuf
        = (size_addr) CdcMemGetPhysicAddress(h264DecCtx->vconfig.memops,
                                             hCtx->pMbFieldIntraBuf);
    logv("phyMbFieldIntraBuf: vir=%p, phy=%x\n",
          hCtx->pMbFieldIntraBuf, (unsigned int)hCtx->phyMbFieldIntraBuf);

    CdcMemSet(h264DecCtx->vconfig.memops,hCtx->pMbFieldIntraBuf, 0, size);
    CdcMemFlushCache(h264DecCtx->vconfig.memops,hCtx->pMbFieldIntraBuf, size);

    s32 MbNeighborInfoBufSize = 0x4000 + 0x4000 + 0x4000;
    hCtx->pMbNeighborInfoBuf = CdcMemPalloc(h264DecCtx->vconfig.memops,MbNeighborInfoBufSize,
                                            (void *)h264DecCtx->vconfig.veOpsS,
                                            h264DecCtx->vconfig.pVeOpsSelf);
    if(hCtx->pMbNeighborInfoBuf == NULL)
    {
        return VDECODE_RESULT_UNSUPPORTED;
    }
    CdcMemSet(h264DecCtx->vconfig.memops,hCtx->pMbNeighborInfoBuf, 0, MbNeighborInfoBufSize);
    CdcMemFlushCache(h264DecCtx->vconfig.memops,hCtx->pMbNeighborInfoBuf, MbNeighborInfoBufSize);

    //*mb_neighbor_info_addr require 16k-align
    size_addr  tmpPhyaddr = (size_addr)CdcMemGetPhysicAddress(h264DecCtx->vconfig.memops,
        (void*)hCtx->pMbNeighborInfoBuf);
    size_addr addrAlign = tmpPhyaddr%0x4000;
    if(addrAlign != 0)
    {
        tmpPhyaddr += (0x4000 - addrAlign);
    }
    hCtx->uMbNeighBorInfo16KAlignBufPhy = tmpPhyaddr;
    #endif
    ret = H264MallocBuffer(h264DecCtx, hCtx);
    return ret;
}
/***************************************************************/
/***************************************************************/

void H264SortDisplayFrameOrder(H264DecCtx* h264DecCtx,
                            H264Context* hCtx,
                            u8 bDecodeKeyFrameOnly)
{
    s32 i = 0;
    s32 minPoc = 0x7fffffff;
    s32 delayedBufIndex = 0xff;
    s32 anciDelayedBufIndex = 0xff;
    s32 nonRefFrameNum = 0;
    s32 refFrameNum = 0;
    u8  findDispFlag = 0;
    u8  flag2 = 0;
    u8  flag3 = 0;
    u8  flag4 = 0;
    u8  flag5 = 0;
    u8  bDispFrameFlag = 0;
    u32 index = 0;

    H264PicInfo* curPicInf = NULL;
    H264PicInfo* outPicInf = NULL;

    if(bDecodeKeyFrameOnly == 1)
    {
        curPicInf = hCtx->frmBufInf.pCurPicturePtr;
        if(h264DecCtx->vconfig.bRotationEn==1 ||
            h264DecCtx->vconfig.bScaleDownEn==1 ||
            hCtx->bYv12OutFlag==1 ||
            hCtx->bNv21OutFlag==1)
        {
            FbmReturnBuffer(hCtx->pFbmScaledown, curPicInf->pScaleDownVPicture, 1);
            curPicInf->pScaleDownVPicture = NULL;
            index = hCtx->frmBufInf.pCurPicturePtr->nDispBufIndex;
            if(index >= MAX_PICTURE_COUNT)
            {
                abort();
            }
            hCtx->frmBufInf.picture[index].pScaleDownVPicture = NULL;
        }
        else
        {
            FbmReturnBuffer(hCtx->pFbm, curPicInf->pVPicture, 1);
            curPicInf->pVPicture = NULL;
            index = hCtx->frmBufInf.pCurPicturePtr->nDispBufIndex;
            if(index >= MAX_PICTURE_COUNT)
            {
                abort();
            }
            hCtx->frmBufInf.picture[index].pVPicture = NULL;
        }
        if((hCtx->pVbv->bUseNewVeMemoryProgram==1) && ENABLE_MV_BUF_OPTIMIZATION_PROGRAM)
        {
            hCtx->pH264MvBufInf[curPicInf->nH264MvBufIndex].pH264MvBufManager->nCalculateNum =
                INIT_MV_BUF_CALCULATE_NUM;
            FIFOEnqueue((FiFoQueueInst**)&hCtx->pH264MvBufEmptyQueue,
            (FiFoQueueInst*) &hCtx->pH264MvBufInf[curPicInf->nH264MvBufIndex]);
        }
        curPicInf->nReference = 0;
        return;
    }

    curPicInf = hCtx->frmBufInf.pCurPicturePtr;


    if((hCtx->pVbv->bUseNewVeMemoryProgram==1) && ENABLE_MV_BUF_OPTIMIZATION_PROGRAM)
    {
        if(hCtx->nNalRefIdc == 0)
        {
            hCtx->pH264MvBufInf[curPicInf->nH264MvBufIndex].pH264MvBufManager->nCalculateNum =
                INIT_MV_BUF_CALCULATE_NUM;
            FIFOEnqueue((FiFoQueueInst**)&hCtx->pH264MvBufEmptyQueue,
            (FiFoQueueInst*) &hCtx->pH264MvBufInf[curPicInf->nH264MvBufIndex]);
        }
    }
    if(curPicInf->nDecodeBufIndex != curPicInf->nDispBufIndex)
    {
        if(curPicInf->nDecodeBufIndex >= MAX_PICTURE_COUNT)
        {
            abort();
        }
        hCtx->frmBufInf.picture[curPicInf->nDecodeBufIndex].pVPicture = NULL;
         hCtx->frmBufInf.picture[curPicInf->nDecodeBufIndex].pScaleDownVPicture = NULL;
        hCtx->frmBufInf.picture[curPicInf->nDecodeBufIndex].nReference = 0;
    }

    if(hCtx->nDelayedPicNum >= MAX_PICTURE_COUNT)
    {
        abort();
    }
    hCtx->frmBufInf.pDelayedPic[hCtx->nDelayedPicNum] = curPicInf;
    if((hCtx->nDelayedPicNum+1) >= MAX_PICTURE_COUNT)
    {
        abort();
    }
    hCtx->frmBufInf.pDelayedPic[hCtx->nDelayedPicNum+1] = NULL;
    hCtx->nDelayedPicNum++;
    hCtx->frmBufInf.pCurPicturePtr = NULL;

    //logd("i=%d, delayedPoc=%d\n",
    //curPicInf->nDecodeBufIndex,
    //hCtx->frmBufInf.pDelayedPic[hCtx->nDelayedPicNum]->nPoc);

    for(i=0; i<hCtx->nDelayedPicNum; i++)
    {
        if(hCtx->nDelayedPicNum >= MAX_PICTURE_COUNT)
        {
            abort();
        }
        if(hCtx->frmBufInf.pDelayedPic[i]->nReference==0 ||
            hCtx->frmBufInf.pDelayedPic[i]->nReference==4)
        {
            nonRefFrameNum++;
        }
        else
        {
            refFrameNum++;
        }

        if(hCtx->frmBufInf.pDelayedPic[i] == NULL)
        {
            continue;
        }
        if(hCtx->frmBufInf.pDelayedPic[i]->nPoc<minPoc)
        {
            minPoc = hCtx->frmBufInf.pDelayedPic[i]->nPoc;
            anciDelayedBufIndex = i;

            if(findDispFlag ==1)
            {
                continue;
            }
            if(hCtx->frmBufInf.pDelayedPic[i]->nPoc<=hCtx->nMinDispPoc)
            {
                if(i != 0)
                {
                    findDispFlag = 1;
                    continue;
                }
            }
            outPicInf = hCtx->frmBufInf.pDelayedPic[i];
            delayedBufIndex = i;
        }
    }

    if(hCtx->frmBufInf.pDelayedOutPutPic != NULL)
    {
        if(outPicInf->nPoc == 0)
        {
            if(minPoc < 0)
            {
                if(anciDelayedBufIndex >= MAX_PICTURE_COUNT)
                {
                    abort();
                }
                outPicInf = hCtx->frmBufInf.pDelayedPic[anciDelayedBufIndex];
                delayedBufIndex = anciDelayedBufIndex;
            }
        }

        flag2 = (outPicInf->nPoc==0);
        flag3 = ((outPicInf->nPoc-hCtx->nMinDispPoc)==hCtx->nPicPocDeltaNum);

        if(flag2==0 && flag3==0 && nonRefFrameNum==0)
        {
            flag4 = (outPicInf->nPoc-hCtx->nMinDispPoc)<0;
            flag5 = (outPicInf->nPoc-hCtx->nMinDispPoc)>32;
            if((flag4==0) && (flag5==0))
            {
                if(outPicInf->pScaleDownVPicture != NULL)
                {
                    return;
                }
                else if(refFrameNum <= hCtx->nRefFrmCount)
                {
                    return;
                }
            }
            else
            {
                loge("thre is some error:outPicInf->nPoc=%d, hCtx->nMinDispPoc=%d\n",
                    outPicInf->nPoc, hCtx->nMinDispPoc);
                hCtx->nMinDispPoc = outPicInf->nPoc-hCtx->nPicPocDeltaNum;
                return;
            }
        }
        if((flag3==1) &&
            (hCtx->nCurFrmNum==2) &&
            (hCtx->nPicPocDeltaNum==2) &&
            (hCtx->nRefFrmCount>1)&&(outPicInf->bDropBFrame != 1))
        {
            return;
        }
    }

    if(outPicInf != NULL)
    {
        outPicInf->bHasDispedFlag = 1;

        if(h264DecCtx->vconfig.bDispErrorFrame == 1)
        {
            bDispFrameFlag = 1;
        }
        else
        {
            bDispFrameFlag = (h264DecCtx->videoStreamInfo.bIs3DStream==0)?
                (outPicInf->bDecErrorFlag==0): 1;
        }
        if(h264DecCtx->vconfig.bRotationEn==1 ||
            h264DecCtx->vconfig.bScaleDownEn==1 ||
            hCtx->bYv12OutFlag==1 ||
            hCtx->bNv21OutFlag==1)
        {
            //outPicInf->pScaleDownpVPicture->nPts = outPicInf->pVPicture->nPts;
            //logv("here1:return buffer:picture=%p, index=%d, poc=%d\n",
            //outPicInf->pVPicture, outPicInf->nCurPicBufIndex, outPicInf->nPoc);
            FbmReturnBuffer(hCtx->pFbmScaledown, outPicInf->pScaleDownVPicture, bDispFrameFlag);
            if(outPicInf->nDispBufIndex >= MAX_PICTURE_COUNT)
            {
                abort();
            }
            hCtx->frmBufInf.picture[outPicInf->nDispBufIndex].pScaleDownVPicture = NULL;
            outPicInf->pScaleDownVPicture = NULL;
            if((outPicInf->nReference==0 ||
                outPicInf->nReference==4) &&
                (outPicInf->pVPicture!=NULL))
            {
                if(outPicInf->pVPicture != NULL)
                {
                    FbmReturnBuffer(hCtx->pFbm, outPicInf->pVPicture, 0);
                }
                hCtx->frmBufInf.picture[outPicInf->nDispBufIndex].pVPicture = NULL;
                outPicInf->pVPicture = NULL;
                outPicInf->nReference = 0;
            }
        }
        else if(outPicInf->nReference==0 || outPicInf->nReference==4)
        {
            //logd("return buffer:picture=%p,poc=%d\n", outPicInf->pVPicture, outPicInf->nPoc);
            if(outPicInf->pVPicture != NULL)
            {
                FbmReturnBuffer(hCtx->pFbm, outPicInf->pVPicture, bDispFrameFlag);
            }
            if(outPicInf->nDispBufIndex >= MAX_PICTURE_COUNT)
            {
                abort();
            }
            hCtx->frmBufInf.picture[outPicInf->nDispBufIndex].pVPicture = NULL;
            outPicInf->pVPicture = NULL;
            outPicInf->nReference = 0;
            outPicInf->bHasDispedFlag = 0;
        }
        else if(bDispFrameFlag==1)
        {
            //logd("share buffer:picture=%p, poc=%d\n", outPicInf->pVPicture, outPicInf->nPoc);
            if(outPicInf->pVPicture != NULL)
            {
                FbmShareBuffer(hCtx->pFbm, outPicInf->pVPicture);
            }
        }
        for(i=delayedBufIndex; i<hCtx->nDelayedPicNum; i++)
        {
            if((i+1)>=MAX_PICTURE_COUNT)
            {
                abort();
            }
            hCtx->frmBufInf.pDelayedPic[i] = hCtx->frmBufInf.pDelayedPic[i+1];
            //logd("i=%d, delayedPoc=%d\n",
            //i, hCtx->frmBufInf.pDelayedPic[hCtx->nDelayedPicNum]->nPoc);
        }
        if(i < MAX_PICTURE_COUNT)
        {
            hCtx->frmBufInf.pDelayedPic[i] = NULL;
        }
        hCtx->nMinDispPoc = outPicInf->nPoc;
        hCtx->nDelayedPicNum--;
          hCtx->frmBufInf.pDelayedOutPutPic = outPicInf;
        if(outPicInf->bDropBFrame == 1)
        {
           outPicInf->bDropBFrame = 0;
        }
    }

#if 0
    logv("\n");
    logv("end disp\n");

    for(i=0; i<18; i++)
    {
        if(hCtx->frmBufInf.pDelayedPic[i] != NULL)
        {
            logv("i=%d, index=%d, picture=%p, reference=%d, poc=%d\n",
                i, hCtx->frmBufInf.pDelayedPic[i]->nCurPicBufIndex,
                hCtx->frmBufInf.pDelayedPic[i]->pVPicture,
                hCtx->frmBufInf.pDelayedPic[i]->nReference,
                hCtx->frmBufInf.pDelayedPic[i]->nPoc);
        }
    }
#endif
}

void H264exchangeValues(s32* param1,s32* param2)
{
    CEDARC_UNUSE(H264exchangeValues);
    s32 temp = 0;
    temp = * param2;
    *param2 = *param1;
    *param1 = temp;
}

void  H264ComputeMax_Min_Avr(u8* pReadptr,s32 nSize,VideoPicture*  curFrmInfo)
{
     s32 i = 0;
     s16 Curmv_x = 0;
     s16 Curmv_y = 0;
     u64 Curmv_value = 0;
     u64 MaxMv_Value = 0;
     s16 MaxMv_Value_ture = 0;
     u64 MinMv_Value = 0;
     s16 MinMv_Value_ture = 0;
     s32 Totalmv_x = 0;
     s32 Totalmv_y = 0;
     s16 Avrmv_x = 0;
     s16 Avrmv_y = 0;
     u16 Avr_Mv = 0;
     s32 nZeroTimes = 0;
     s16 Maxmv_x = 0;
     s16 Maxmv_y = 0;
     s16 Minmv_x = 0;
     s16 Minmv_y = 0;
     for(i=0; i<nSize; i++)
     {
         Curmv_x =  *pReadptr;
         pReadptr++;
         Curmv_x += (*pReadptr&0x3F)<<8;
         Curmv_y += (*pReadptr>>6)&0x03;
         pReadptr++;
         Curmv_y += (*pReadptr)<<2;
         pReadptr++;
         Curmv_y += (*pReadptr&0x03)<<10;
         pReadptr++;
         if(Curmv_x & 0x2000)
         {
            Curmv_x = (~Curmv_x)+1;
            Curmv_x = (Curmv_x & 0x1FFF);
            Curmv_x = -Curmv_x;
         }
         if(Curmv_y & 0x800)
         {
            Curmv_y = (~Curmv_y)+1;
            Curmv_y = (Curmv_y & 0x7FF);
            Curmv_y = -Curmv_y;
         }
         Curmv_value = Curmv_x*Curmv_x + Curmv_y*Curmv_y;
         if(Curmv_value == 0)
         {
           nZeroTimes++;
         }
         Maxmv_x = Curmv_x > Maxmv_x ? Curmv_x : Maxmv_x;
         Maxmv_y = Curmv_y > Maxmv_y ? Curmv_y : Maxmv_y;
         Minmv_x = Curmv_x < Minmv_x ? Curmv_x : Minmv_x;
         Minmv_y = Curmv_y < Minmv_y ? Curmv_y : Minmv_y;
         MaxMv_Value = Curmv_value > MaxMv_Value ? Curmv_value : MaxMv_Value;
         MinMv_Value = Curmv_value < MinMv_Value ? Curmv_value : MinMv_Value;
         Totalmv_x += Curmv_x;
         Totalmv_y += Curmv_y;
         Curmv_x = Curmv_y = 0;
     }
     Avrmv_x = Totalmv_x/nSize;
     Avrmv_y = Totalmv_y/nSize;
     MaxMv_Value_ture= sqrt(MaxMv_Value);
     MinMv_Value_ture= sqrt(MinMv_Value);
     Avr_Mv =  sqrt(Avrmv_x*Avrmv_x + Avrmv_y*Avrmv_y);
     curFrmInfo->nCurFrameInfo.nMvInfo.nMaxMv_x  = Maxmv_x;
     curFrmInfo->nCurFrameInfo.nMvInfo.nMinMv_x  = Minmv_x;
     curFrmInfo->nCurFrameInfo.nMvInfo.nMaxMv_y  = Maxmv_y;
     curFrmInfo->nCurFrameInfo.nMvInfo.nMinMv_y  = Minmv_y;
     curFrmInfo->nCurFrameInfo.nMvInfo.nAvgMv_x  = Avrmv_x;
     curFrmInfo->nCurFrameInfo.nMvInfo.nAvgMv_y  = Avrmv_y;
     curFrmInfo->nCurFrameInfo.nMvInfo.nAvgMv    = Avr_Mv;
     curFrmInfo->nCurFrameInfo.nMvInfo.nMaxMv    = MaxMv_Value_ture;
     curFrmInfo->nCurFrameInfo.nMvInfo.nMinMv    = MinMv_Value_ture;
     curFrmInfo->nCurFrameInfo.nMvInfo.SkipRatio = nZeroTimes*100/nSize;
}
void H264ComputeMVParameters(H264Dec* h264Dec,H264Context* hCtx,VideoPicture*  curFrmInfo)
{
     s32 MvSize = 0;
     u8* pTopMvColBuf = NULL;
     H264PicInfo *pCurPicturePtr = NULL;
     pCurPicturePtr = hCtx->frmBufInf.pCurPicturePtr;
     pTopMvColBuf = pCurPicturePtr->pTopMvColBuf;
     MvSize = hCtx->nMbHeight;
     MvSize = (MvSize+1)/2;
     MvSize = hCtx->nMbWidth*MvSize;
     if(hCtx->nPicStructure != PICT_FRAME)
     {
         MvSize *= 2;
     }
     if(hCtx->bDirect8x8InferenceFlag == 0)
     {
         MvSize *= 2;
     }
  #if 0
     FILE *fp;
     char name[128];
     sprintf(name, "/data/camera/pic_%d.dat",hCtx->nCurFrmNum);
     fp = fopen(name, "ab");
     if(fp != NULL)
     {
         CdcMemFlushCache(h264Dec->pConfig->memops,pCurPicturePtr->pTopMvColBuf, MvSize*32);
         fwrite(pTopMvColBuf, 1, MvSize*32, fp);
         fclose(fp);
     }
  #endif
     CdcMemFlushCache(h264Dec->pConfig->memops,pCurPicturePtr->pTopMvColBuf, MvSize*32);
     H264ComputeMax_Min_Avr(pTopMvColBuf,MvSize*4,curFrmInfo);
}
void H264CongigureDisplayParameters(H264DecCtx* h264DecCtx, H264Context* hCtx)
{
    double    nVidFrmSize;
    double    nFrameRate;
    VideoPicture*  curFrmInfo = NULL;

    if((h264DecCtx->vconfig.bRotationEn== 1) || (h264DecCtx->vconfig.bScaleDownEn==1)
            ||(hCtx->bYv12OutFlag== 1) || (hCtx->bNv21OutFlag==1))
    {
        curFrmInfo = hCtx->frmBufInf.pCurPicturePtr->pScaleDownVPicture;
    }
    else
    {
        curFrmInfo = hCtx->frmBufInf.pCurPicturePtr->pVPicture;
    }
    curFrmInfo->nTopOffset      = hCtx->nTopOffset;
    curFrmInfo->nLeftOffset     = hCtx->nLeftOffset;
    curFrmInfo->nRightOffset    = hCtx->nRightOffset;
    curFrmInfo->nBottomOffset   = hCtx->nBottomOffset;

    curFrmInfo->nFrameRate        = hCtx->vbvInfo.nFrameRate;  // need update
    curFrmInfo->nAspectRatio      = 1000;
    curFrmInfo->nAspectRatio      <<= h264DecCtx->vconfig.nHorizonScaleDownRatio;
    curFrmInfo->nAspectRatio      >>= h264DecCtx->vconfig.nVerticalScaleDownRatio;

    curFrmInfo->bIsProgressive    = hCtx->bProgressice;
    curFrmInfo->bTopFieldFirst    = (hCtx->frmBufInf.pCurPicturePtr->nFieldPoc[0]<=
                                    hCtx->frmBufInf.pCurPicturePtr->nFieldPoc[1]);
    curFrmInfo->bRepeatTopField   = 0;
    curFrmInfo->ePixelFormat      = h264DecCtx->vconfig.eOutputPixelFormat;
    curFrmInfo->nStreamIndex      = 0;
    curFrmInfo->nColorPrimary     = hCtx->nColorPrimary;

    if(curFrmInfo->bIsProgressive == 0)
    {
        if(curFrmInfo->bTopFieldFirst == 1)
        {
            curFrmInfo->bTopFieldError =
                hCtx->frmBufInf.pCurPicturePtr->bDecFirstFieldError;
            curFrmInfo->bBottomFieldError =
                hCtx->frmBufInf.pCurPicturePtr->bDecSecondFieldError;
        }
        else
        {
            curFrmInfo->bTopFieldError =
                hCtx->frmBufInf.pCurPicturePtr->bDecSecondFieldError;
            curFrmInfo->bBottomFieldError =
                hCtx->frmBufInf.pCurPicturePtr->bDecFirstFieldError;
        }
    }
    else
    {
        curFrmInfo->bTopFieldError = (hCtx->frmBufInf.pCurPicturePtr->bDecFirstFieldError ||
                                      hCtx->frmBufInf.pCurPicturePtr->bDecSecondFieldError ||
                                      hCtx->frmBufInf.pCurPicturePtr->bDecErrorFlag);
        curFrmInfo->bBottomFieldError = curFrmInfo->bTopFieldError;
    }

    if(hCtx->bPreDropFrame == 1)
    {
        curFrmInfo->nCurFrameInfo.bDropPreFrame =1;
        hCtx->bPreDropFrame = 0;
        hCtx->bDropFrame = 0;
    }
    else
    {
        curFrmInfo->nCurFrameInfo.bDropPreFrame =0;
    }
    curFrmInfo->nCurFrameInfo.enVidFrmType = 0;
    switch(hCtx->nFrameType)
    {
      case H264_I_TYPE:
      {
          curFrmInfo->nCurFrameInfo.enVidFrmType = 1;
          break;
       }
       case H264_P_TYPE:
       {
          curFrmInfo->nCurFrameInfo.enVidFrmType = 2;
          break;
        }
        case H264_B_TYPE:
        {
          curFrmInfo->nCurFrameInfo.enVidFrmType = 3;
          break;
        }
     }
     if(hCtx->bIdrFrmFlag == 1)
     {
        curFrmInfo->nCurFrameInfo.enVidFrmType = 4;
     }
     curFrmInfo->nCurFrameInfo.nVidFrmDisW  = hCtx->nFrmRealWidth;
     curFrmInfo->nCurFrameInfo.nVidFrmDisH  = hCtx->nFrmRealHeight;
     curFrmInfo->nCurFrameInfo.nVidFrmQP    = hCtx->nQscale;
     curFrmInfo->nCurFrameInfo.nVidFrmPTS   = curFrmInfo->nPts/1000;
     curFrmInfo->nCurFrameInfo.nVidFrmSize  = hCtx->nByteLensCurFrame+4;
     if(hCtx->vbvInfo.nFrameRate != 0)
     {
         if(hCtx->vbvInfo.nFrameRate>0 && hCtx->vbvInfo.nFrameRate<100)
         {
             curFrmInfo->nCurFrameInfo.nFrameRate = (double)hCtx->vbvInfo.nFrameRate;
         }
         else
         {
             nFrameRate = (double)hCtx->vbvInfo.nFrameRate;
             curFrmInfo->nCurFrameInfo.nFrameRate = nFrameRate/(double)(1000.0);
         }
     }
     else
     {
         if(hCtx->nEstimatePicDuration != 0)
         {
             logv("the_444,nEstimatePicDuration=%lld",hCtx->nEstimatePicDuration);
             logv("222,nEstimatePicDuration=%0.2f",(double)hCtx->nEstimatePicDuration);
             curFrmInfo->nCurFrameInfo.nFrameRate =
                    ((double)(1000*1000))/((double)hCtx->nEstimatePicDuration);
         }
         else
         {
             logd("the_555");
             curFrmInfo->nCurFrameInfo.nFrameRate = 33.0;
         }
     }
     nVidFrmSize = (double)curFrmInfo->nCurFrameInfo.nVidFrmSize;
     nFrameRate = curFrmInfo->nCurFrameInfo.nFrameRate;
     curFrmInfo->nCurFrameInfo.nAverBitRate = nVidFrmSize*8*1000/nFrameRate;
     logv("the_264,nFrameRate=%0.2lf,nAverBitRate=%0.2lf\n",
        curFrmInfo->nCurFrameInfo.nFrameRate,curFrmInfo->nCurFrameInfo.nAverBitRate);
     hCtx->nByteLensCurFrame                = 0;
     if(h264DecCtx->vconfig.eCtlIptvMode != DISABLE_IPTV_ALL_SIZE)
     {
        if((h264DecCtx->vconfig.eCtlIptvMode==ENABLE_IPTV_ALL_SIZE) || \
           (h264DecCtx->vconfig.eCtlIptvMode==ENABLE_IPTV_JUST_SMALL_SIZE && hCtx->bIs4K==0))
        H264ComputeMVParameters(h264DecCtx->pH264Dec,hCtx, curFrmInfo);
     }
     logv("enVidFrmType=%d,nFrmRealWidth=%d,nFrmRealHeight=%d,nVidFrmQP=%d,nVidFrmPTS=%lld\n",\
        curFrmInfo->nCurFrameInfo.enVidFrmType,curFrmInfo->nCurFrameInfo.nVidFrmDisW,\
        curFrmInfo->nCurFrameInfo.nVidFrmDisH,curFrmInfo->nCurFrameInfo.nVidFrmQP,\
        curFrmInfo->nCurFrameInfo.nVidFrmPTS);
    logv("curFrmInfo=%x, curFrmInfo->bIsProgressive =%d, \
          curFrmInfo->bTopFieldError=%d, \
          curFrmInfo->bBottomFieldError=%d, \
          decErrorFlag=%d\n",
          curFrmInfo, curFrmInfo->bIsProgressive, curFrmInfo->bTopFieldError,
          curFrmInfo->bBottomFieldError, hCtx->frmBufInf.pCurPicturePtr->bDecErrorFlag);

}

//**************************************************************************//
//**************************************************************************//

void H264ProcessDecodeFrameBuffer(H264DecCtx* h264DecCtx,
                                H264Context* hCtx, u8 bForceReturnFrame)
{
    s32 i = 0;
    s32 refFrmNum = 0;
    s32 minOrderFrmIndex = 0;
    u32 minDecFrmOrder = 0xffffffff;
    s32 nDecodeFrmNum = 0;

    H264Dec* pH264Dec = NULL;
    H264PicInfo* outPicInf = NULL;

    pH264Dec = (H264Dec*)h264DecCtx->pH264Dec;

    if(bForceReturnFrame == 1)
    {
         for(i=0; i<hCtx->nDelayedPicNum; i++)
         {
             if(i >= MAX_PICTURE_COUNT)
             {
                abort();
             }
             outPicInf = hCtx->frmBufInf.pDelayedPic[i];

             if(outPicInf->nDispBufIndex >= MAX_PICTURE_COUNT)
             {
                abort();
             }
             if(hCtx->frmBufInf.pDelayedPic[i]->nReference==0 ||
                hCtx->frmBufInf.pDelayedPic[i]->nReference==4)
             {
                 FbmReturnBuffer(hCtx->pFbm, hCtx->frmBufInf.pDelayedPic[i]->pVPicture, 0);
                 hCtx->frmBufInf.picture[outPicInf->nDispBufIndex].pVPicture = NULL;
             }
             else
             {
                 hCtx->frmBufInf.picture[outPicInf->nDispBufIndex].bHasDispedFlag = 1;
             }
             if(hCtx->nMinDispPoc < outPicInf->nPoc)
             {
                 hCtx->nMinDispPoc = outPicInf->nPoc;
             }
             hCtx->frmBufInf.pDelayedPic[i] = NULL;
         }
         hCtx->nDelayedPicNum = 0;
         return;
    }

    nDecodeFrmNum = hCtx->frmBufInf.nMaxValidFrmBufNum - FbmValidPictureNum(hCtx->pFbm)-2;

    if(nDecodeFrmNum <= hCtx->nRefFrmCount)
    {
        return;
    }

    for(i=0+hCtx->nBufIndexOffset;
    i<hCtx->frmBufInf.nMaxValidFrmBufNum+hCtx->nBufIndexOffset; i++)
    {
        if(hCtx->frmBufInf.picture[i].pVPicture!=NULL &&
            (&hCtx->frmBufInf.picture[i]!= pH264Dec->pMajorRefFrame))
        {
            if((hCtx->frmBufInf.picture[i].bHasDispedFlag==1) ||
                (hCtx->frmBufInf.picture[i].pScaleDownVPicture!=NULL))
            {
                if(hCtx->frmBufInf.picture[i].nReference==0 ||
                    hCtx->frmBufInf.picture[i].nReference==4)
                {
                    FbmReturnBuffer(hCtx->pFbm, hCtx->frmBufInf.picture[i].pVPicture, 0);
                    //logd("return buffer:hCtx->frmBufInf.picture[i].pVPicture=%p\n",
                    //hCtx->frmBufInf.picture[i].pVPicture);

                    hCtx->frmBufInf.picture[i].nReference = 0;
                    hCtx->frmBufInf.picture[i].pVPicture = NULL;
                    hCtx->frmBufInf.picture[i].bHasDispedFlag = 0;
                }
                else
                {
                    refFrmNum++;
                    if(hCtx->frmBufInf.picture[i].nDecFrameOrder < minDecFrmOrder)
                    {
                        minDecFrmOrder = hCtx->frmBufInf.picture[i].nDecFrameOrder;
                        minOrderFrmIndex = i;
                    }
                }
            }
            else if(hCtx->frmBufInf.picture[i].nReference!=0 &&
                hCtx->frmBufInf.picture[i].nReference!=4)
            {
                refFrmNum++;
            }
        }
    }

    if(refFrmNum > hCtx->nRefFrmCount)
    {
        if(minOrderFrmIndex < MAX_PICTURE_COUNT)
        {
            FbmReturnBuffer(hCtx->pFbm, hCtx->frmBufInf.picture[minOrderFrmIndex].pVPicture, 0);

            hCtx->frmBufInf.picture[minOrderFrmIndex].pVPicture = NULL;
            hCtx->frmBufInf.picture[minOrderFrmIndex].nReference = 0;
            hCtx->frmBufInf.picture[minOrderFrmIndex].bHasDispedFlag = 0;
        }
    }
}

u8 H264CheckNextStartCode(H264Dec* h264Dec, H264Context* hCtx, u32 bitOffset, u8* pNewReadPtr)
{
    CEDARC_UNUSE(H264CheckNextStartCode);
    u8* pReadPtr = NULL;
    u8* pBuffer = NULL;
    u32 size = 0;
    u8  i = 0;
    u8 buffer[6];
    u32 nextCode = 0xffffffff;

    if((bitOffset>>3) > (u32)(hCtx->vbvInfo.pVbvBufEnd-hCtx->vbvInfo.pVbvBuf+1))
    {
        bitOffset -= (hCtx->vbvInfo.pVbvBufEnd-hCtx->vbvInfo.pVbvBuf+1)*8;
    }
    pReadPtr = hCtx->vbvInfo.pVbvBuf+ (bitOffset>>3);
    if(pReadPtr >= pNewReadPtr)
    {
        if(!((hCtx->vbvInfo.pReadPtr<=pReadPtr)&&(pReadPtr<hCtx->vbvInfo.pReadPtr)))
        {
            pReadPtr = pNewReadPtr;
        }
    }
    pReadPtr -= 6;
    if(pReadPtr+6 <= hCtx->vbvInfo.pVbvBufEnd)
    {
        pBuffer = pReadPtr;
    }
    else
    {
        size = hCtx->vbvInfo.pVbvBufEnd-pReadPtr+1;
        memcpy(buffer, pReadPtr, size);
        memcpy(buffer+size, hCtx->vbvInfo.pVbvBuf, 6-size);

        pBuffer = buffer;
    }

    for(i=0; i<6; i++)
    {
        nextCode <<= 8;
        nextCode |= pBuffer[i];
        if(nextCode == 0x000001)
        {
            return 1;
        }
    }
    return 0;
}


#if H264_DEBUG_COMPUTE_PIC_MD5
static void H264CaculateBufMd5(char *md5, char* src, int stride, int width, int height)
{
    char *buf;
    int y,x;
    buf = malloc(width * height);

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            buf[y * width + x] = src[x] & 0xff;
        }
        src += stride;
    }
    HevcMd5CaculateSum((u8 *)md5, (u8 *)buf, width * height);
    free(buf);
}

static void H264CaculatePictureMd5(H264PicInfo *pCurPicturePtr)
{
    VideoPicture *pPic;
    int poc, nWidth, nHeight;
    char *pSrc;
    char md5[16], *p;

    pPic = pCurPicturePtr->pVPicture;
    poc = pCurPicturePtr->nPoc;
    p = md5;
    logd(" ---------- poc: %d, md5 start -------", poc);
    AdapterMemFlushCache(pPic->pData0, pPic->nWidth*pPic->nHeight*3/2);

    nWidth = pPic->nWidth;
    nHeight = pPic->nHeight;
    pSrc = pPic->pData0;
    H264CaculateBufMd5(md5, pSrc, nWidth, nWidth, nHeight);
    logd(" %2x,%2x,%2x,%2x, %2x,%2x,%2x,%2x, %2x,%2x,%2x,%2x, %2x,%2x,%2x,%2x,",
            p[0],p[1],p[2],p[3], p[4],p[5],
            p[6],p[7], p[8],p[9],p[10],p[11],
            p[12],p[13],p[14],p[15]);

    nWidth = pPic->nWidth/2;
    nHeight = pPic->nHeight/2;
    pSrc = pPic->pData0 + pPic->nWidth*pPic->nHeight*5/4;
    H264CaculateBufMd5(md5, pSrc, nWidth, nWidth, nHeight);
    logd(" %2x,%2x,%2x,%2x, %2x,%2x,%2x,%2x, %2x,%2x,%2x,%2x, %2x,%2x,%2x,%2x,",
            p[0],p[1],p[2],p[3], p[4],p[5],
            p[6],p[7], p[8],p[9],p[10],p[11],
            p[12],p[13],p[14],p[15]);

    nWidth = pPic->nWidth/2;
    nHeight = pPic->nHeight/2;
    pSrc = pPic->pData0 + pPic->nWidth*pPic->nHeight;
    H264CaculateBufMd5(md5, pSrc, nWidth, nWidth, nHeight);
    logd(" %2x,%2x,%2x,%2x, %2x,%2x,%2x,%2x, %2x,%2x,%2x,%2x, %2x,%2x,%2x,%2x,",
            p[0],p[1],p[2],p[3], p[4],p[5],
            p[6],p[7], p[8],p[9],p[10],p[11], p[12],p[13],p[14],p[15]);
    logd(" ---------- poc: %d, md5 end -------", poc);
}
#endif //#if H264_DEBUG_COMPUTE_PIC_MD5

static void H264DebugSaveDecoderPic(VideoPicture *pPic, H264Context *hCtx)
{
    CEDARC_UNUSE(H264DebugSaveDecoderPic);
    FILE *fp = NULL;
    char name[128];
#if 0
    sprintf(name, "/data/camera/avc_%d.yuv", hCtx->nCurPicNum);
#else
    sprintf(name, "/data/camera/dcts_212.nv21");
#endif
    fp = fopen(name, "ab");
    if(fp != NULL)
    {
        u32 w = hCtx->nFrmMbWidth;
        u32 h = hCtx->nFrmMbHeight;
        logd(" saving pic number: %d size: %dx%d, format: %d, poc: %d ",
                hCtx->nCurPicNum, w, h, pPic->ePixelFormat,
                hCtx->frmBufInf.pCurPicturePtr->nPoc);
        if(pPic->ePixelFormat == PIXEL_FORMAT_YV12)
        {
            fwrite(pPic->pData0, 1, w * h, fp);
            fwrite(pPic->pData0 + w * h * 5/4, 1, w * h / 4, fp);
            fwrite(pPic->pData0 + w * h, 1, w * h / 4, fp);
        }
        else
            fwrite(pPic->pData0, 1, w * h * 3/2, fp);
        fclose(fp);
    }
    else
    {
        loge(" decoder saving picture open file fail ");
    }
}

static void H264DebugDecodeFinish(H264Context* hCtx)
{
    CEDARC_UNUSE(H264DebugDecodeFinish);
    H264Debug *debug = &hCtx->debug;
    s32 nFrameCycle = H264_DEBUG_FRAME_CYCLE;

#if 0
    /* todo: need double check */
    if(hCtx->nPicStructure != PICT_FRAME)
    {
        nFrameCycle <<= 1;
    }
#endif
    debug->nDeltaFrameNum += 1;

    if(debug->nDeltaFrameNum >= nFrameCycle)
    {
        /* caclulate fps */
        float avg, crt;
        s32 nDeltaTime, nTotalTime;
        debug->nTotalFrameNum += H264_DEBUG_FRAME_CYCLE;
        debug->nTotalTime += debug->nDeltaTime;

        nDeltaTime = (s32)(debug->nDeltaTime / 1000);
        crt = nDeltaTime * 1.0;
        crt = (H264_DEBUG_FRAME_CYCLE * 1000)/crt;

        nTotalTime = (s32)(debug->nTotalTime / 1000); /* warning: nTotalTime maybe negative */
        avg = nTotalTime * 1.0;
        avg = (debug->nTotalFrameNum * 1000)/avg;

        logd(" h264 decoder HARDWARE current speed: %.2f fps, average speed: %.2f fps", crt, avg);
        debug->nDeltaTime = 0;
        debug->nDeltaFrameNum = 0;
    }
}

s32 H264ProcessNaluUnit(H264DecCtx* h264DecCtx,
                        H264Context* hCtx ,
                        u8 nalUnitType,
                        s32 sliceDataLen, u8 decStreamIndex)
{
    s32 ret = VDECODE_RESULT_OK;
    u32 decMbNum = 0;
    u32 picSizeInMb  = 0;
    H264Dec* h264Dec = NULL;

    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;
    hCtx->nalUnitType = nalUnitType;

    if(h264Dec->nDecStreamIndex == 0)
    {
        if(hCtx->bNeedFindPPS==1 || hCtx->bNeedFindSPS==1)
        {
            if(nalUnitType!=NAL_SPS&& hCtx->bNeedFindSPS==1)
            {
                return VDECODE_RESULT_OK;
            }
            if((nalUnitType!=NAL_PPS&&nalUnitType!=NAL_SPS) && hCtx->bNeedFindPPS==1)
            {
                return VDECODE_RESULT_OK;
            }
        }
    }
    else if(h264Dec->nDecStreamIndex == 1)
    {
        if(hCtx->bNeedFindSPS==1)
        {
            if(nalUnitType!=NAL_SPS_EXT&& hCtx->bNeedFindSPS==1)
            {
                return VDECODE_RESULT_OK;
            }
            hCtx->bNeedFindSPS = 0;
            hCtx->bNeedFindPPS = 0;
        }
    }

    hCtx->bIdrFrmFlag = 0;

#if H264_DEBUG_SHOW_FPS
    if(((hCtx->nalUnitType>=NAL_SLICE)&&(hCtx->nalUnitType<=NAL_IDR_SLICE))||
            (hCtx->nalUnitType==NAL_HEADER_EXT1|| hCtx->nalUnitType==NAL_HEADER_EXT2))
    {
        hCtx->debug.nStartTime = H264GetCurrentTime();
    }
#endif
    switch(nalUnitType)
    {
        case NAL_IDR_SLICE:
        {
            hCtx->bNeedFindIFrm = 0;
            H264ReferenceRefresh(hCtx);
            hCtx->bIdrFrmFlag = 1;
        }
        case NAL_SLICE:
        {
            if(hCtx->frmBufInf.nMaxValidFrmBufNum == 0)
            {
                if(hCtx->bProgressice == 0xFF)
                {
                    ret = H264DecodePictureScanType(h264DecCtx, hCtx);
                    return ret;
                }
                ret = H264MallocFrmBuffer(h264DecCtx, hCtx);
                if(ret != VDECODE_RESULT_OK)
                {
                    logd("malloc buffer error\n");
                    return ret;
                }
            }
            ret = H264DecodeSliceHeader(h264DecCtx, hCtx);
            if(ret != VDECODE_RESULT_OK)
            {
                if(ret == VRESULT_ERR_FAIL)
                {
                    hCtx->vbvInfo.nValidDataPts = H264VDEC_ERROR_PTS_VALUE;
                    if(hCtx->frmBufInf.pCurPicturePtr != NULL)
                    {

                        hCtx->frmBufInf.pCurPicturePtr->bDecErrorFlag = 1;
                    }
                }
                return ret;
            }
            hCtx->nDecFrameStatus = H264_START_DEC_FRAME;

            H264ConfigureSliceRegister(h264DecCtx, hCtx, decStreamIndex, sliceDataLen);
            break;
        }
        case NAL_SPS:
        {
            ret = H264DecodeSps(h264DecCtx, hCtx);
            if(ret != VDECODE_RESULT_OK)
            {
                logv("decode pSps ret=%d\n", ret);
                return ret;
            }
            hCtx->bNeedFindSPS = 0;

            //* omx have no deinterlace-process, so we can mallc fbm buffer here
            //* to improve fbm-buffer delay , and without judge whether is progressice or not.
            if(h264DecCtx->vconfig.bCalledByOmxFlag==1 &&
                hCtx->frmBufInf.nMaxValidFrmBufNum == 0)
            {
                hCtx->bProgressice = 1;
                ret = H264MallocFrmBuffer(h264DecCtx, hCtx);
                if(ret != VDECODE_RESULT_OK)
                {
                    logd("malloc buffer error\n");
                    return ret;
                }
            }
            break;
        }
        case NAL_PPS:
        {
            ret = H264DecodePps(h264DecCtx, hCtx, sliceDataLen);
            if(ret != VDECODE_RESULT_OK)
            {
                return ret;
            }
            hCtx->bNeedFindPPS = 0;
            break;
        }
        case NAL_HEADER_EXT1:
        case NAL_HEADER_EXT2:
        {
            if(decStreamIndex == 0)
            {
                return VDECODE_RESULT_OK;
            }

            ret = H264DecodeNalHeaderExt(h264DecCtx, hCtx);
            if(hCtx->bIdrFrmFlag == 1)
            {
                H264ReferenceRefresh(hCtx);
            }
            if(hCtx->frmBufInf.nMaxValidFrmBufNum == 0)
            {
                if(hCtx->bProgressice == 0xFF)
                {
                    ret = H264DecodePictureScanType(h264DecCtx, hCtx);
                    return ret;
                }
                ret = H264MallocFrmBuffer(h264DecCtx, hCtx);
                if(ret != VDECODE_RESULT_OK)
                {
                    logd("malloc buffer error\n");
                    return ret;
                }
            }
            ret = H264DecodeSliceHeader(h264DecCtx, hCtx);
            if(ret != VDECODE_RESULT_OK)
            {
                logd("ret=%d\n", ret);
                return ret;
            }
            hCtx->nDecFrameStatus = H264_START_DEC_FRAME;
            H264ConfigureSliceRegister(h264DecCtx, hCtx, decStreamIndex, sliceDataLen);
            break;
        }
        case NAL_SPS_EXT:
        {
            if(decStreamIndex == 0)
            {
                return VDECODE_RESULT_OK;
            }
             hCtx->bNeedFindSPS = 0;
             ret = H264DecodeSps(h264DecCtx, hCtx);
             if(ret != VDECODE_RESULT_OK)
             {
                 logd("decode pSps extension ret=%d\n", ret);
                 return ret;
             }

             break;
        }
        case NAL_SEI:
        {
            ret = H264DecodeSei(h264DecCtx, hCtx, sliceDataLen);
            if(hCtx->nSeiRecoveryFrameCnt > 0)
            {
                 hCtx->bNeedFindIFrm = 0;
            }
            break;
        }
        //case NAL_DPA:
        //case NAL_DPB:
        //case NAL_DPC:
        default:
        {
            return VDECODE_RESULT_OK;
        }
    }

    if(((hCtx->nalUnitType>=NAL_SLICE)&&(hCtx->nalUnitType<=NAL_IDR_SLICE))||
            (hCtx->nalUnitType==NAL_HEADER_EXT1|| hCtx->nalUnitType==NAL_HEADER_EXT2))

    {
        int rett = CdcVeWaitInterrupt(h264DecCtx->vconfig.veOpsS, h264DecCtx->vconfig.pVeOpsSelf);

        H264CheckBsDmaBusy(h264DecCtx);
        u32 ve_status_reg = H264VeIsr(h264DecCtx);
        CEDARC_UNUSE(ve_status_reg);
        #if 0
        if((ve_status_reg&2) == 2)
        {
            //logd("here2: bDecErrorFlag=1\n");
            hCtx->frmBufInf.pCurPicturePtr->bDecErrorFlag = 1;
        }
        #endif
        decMbNum = H264GetDecodeMbNum(h264DecCtx);
        picSizeInMb = hCtx->nMbWidth*hCtx->nMbHeight*(2-hCtx->bFrameMbsOnlyFlag);
        if(hCtx->nPicStructure != PICT_FRAME)
        {
            picSizeInMb >>= 1;
        }
        hCtx->bLastMbInSlice = decMbNum;

        logv("decMbNum=%d, picSizeInMb=%d\n", decMbNum, picSizeInMb);
#if H264_DEBUG_SHOW_FPS
        if(1)
        {
            int64_t nFinishTime = H264GetCurrentTime();
            hCtx->debug.nDeltaTime += (nFinishTime - hCtx->debug.nStartTime);
        }
#endif

        if(decMbNum >= picSizeInMb)
        {
            // decode the frame end
            //logv("dec the frame end\n");
            if((decMbNum!=picSizeInMb) &&(decMbNum != 2*picSizeInMb))
            {
                if(hCtx->bFstField)
                {
                    hCtx->frmBufInf.pCurPicturePtr->bDecFirstFieldError = 1;
                }
                else
                {
                    hCtx->frmBufInf.pCurPicturePtr->bDecSecondFieldError = 1;
                }

                hCtx->frmBufInf.pCurPicturePtr->bDecErrorFlag = 1;
            }
            hCtx->nCurSliceNum  = 0;
            hCtx->bLastMbInSlice = 0;
            hCtx->nDecFrameStatus = H264_END_DEC_FRAME;
#if H264_DEBUG_SHOW_FPS
            H264DebugDecodeFinish(hCtx);
#endif //H264_DEBUG_SHOW_FPS

#if H264_DEBUG_SAVE_PIC
//            if(hCtx->nFrmNum < 30)
            {
                H264DebugSaveDecoderPic(hCtx->frmBufInf.pCurPicturePtr->pVPicture, hCtx);
                usleep(10*1000);
            }
#endif //H264_DEBUG_SAVE_PIC

#if H264_DEBUG_COMPUTE_PIC_MD5
            if(hCtx->nFrmNum < 30)
            {
                // print md5
                //H264CaculatePictureMd5(hCtx->frmBufInf.pCurPicturePtr);
            }
#endif //H264_DEBUG_COMPUTE_PIC_MD5

           // logv("hCtx->nCurPicNum=%d\n", hCtx->nCurFrmNum);

#if 0
            FILE * fp = NULL;
            H264PicInfo* pCurPicturePtr = NULL;
            u8 filePath[64] = {'/0'};
            if((hCtx->nCurFrmNum<10) && h264Dec->nDecStreamIndex==0)
            {
                pCurPicturePtr = hCtx->frmBufInf.pCurPicturePtr;
                sprintf((char*)filePath, "/data/camera/y_%d.dat", hCtx->nCurFrmNum);
                fp = fopen(filePath, "wb");
                fwrite(pCurPicturePtr->pVPicture->pData0, 1,1920*1088, fp);
                fclose(fp);
            }
#endif
            //hCtx->nCurFrmNum++;
            if(hCtx->frmBufInf.pCurPicturePtr == NULL)
            {
                return VRESULT_ERR_FAIL;
            }
            ret = (hCtx->frmBufInf.pCurPicturePtr->bKeyFrame==1)?
                VDECODE_RESULT_KEYFRAME_DECODED : VDECODE_RESULT_FRAME_DECODED;
        }
    }
    return ret;
}


