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
* File : h265_func.h
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#ifndef H265_FUNC_H
#define H265_FUNC_H

#include "h265_var.h"
#include <sys/time.h>

/************************** h265 start *********************************/
s32 HevcDecInitContext(HevcContex *pHevcDec);
s32 HevcDecInitProcessSpecificData(HevcContex *pHevcDec, char *pData, s32 nDataSize);
void HevcDecDestroy(HevcContex *pHevcDec);
s32 HevcDecRequestOneFrameStream(HevcContex *pHevcDec, s32 bDecodeKeyFrameOnly);
s32 HevcDecRequestOneFrameStream2(HevcContex *pHevcDec);
s32 HevcDecRequestOneFrameStreamNew(HevcContex *pHevcDec);
s32 HevcDecReturnAllSbmStreams(HevcContex *pHevcDec);
s32 HevcDecReturnAllSbmStreamsUnflush(HevcContex *pHevcDec);
HevcDecodeFrameResult HevcDecDecodeOneFrame(HevcContex *pHevcDec, s32 bDecodeKeyFrameOnly);
void HevcDecOutputAllPictures(HevcContex *pHevcDec);
void HevcResetDecoder(HevcContex *pHevcDec);
s32 HevcCheckBitStreamType(HevcContex *pHevcDec);
/*************************** h265 end **********************************/

/************************** h265_parser start *********************************/
s32 HevcParserInitGetBits(GetBitContext *s, char *pBuffer, int nBitSize);
s32 HevcParserGetNaluType(HevcContex *pHevcDec);
s32 HevcDecodeNalVps(HevcContex *pHevcDec);
s32 HevcDecodeNalSps(HevcContex *pHevcDec);
void HevcFreePpsBuffer(HevcPPS *pPps);
s32 HevcDecodeNalPps(HevcContex *pHevcDec);
s32 HevcDecodeNalSei(HevcContex *pHevcDec);
void HevcSliceHeaderDefaultValue(HevcSliceHeader *pSh);
s32 HevcDecodeSliceHeader(HevcContex *pHevcDec);
/*************************** h265_parser end **********************************/

/************************** h265_ref start *********************************/
void HevcClearRefs(HevcContex *pHevcDec);
void HevcReturnUnusedBuffer(HevcContex *pHevcDec);
void HevcResetClearRefs(HevcContex *pHevcDec);
s32 HevcSetNewRef(HevcContex *pHevcDec, s32 nPoc);
s32 HevcConstuctFrameRps(HevcContex *pHevcDec);
s32 HevcSliceRps(HevcContex *pHevcDec);
s32 HevcOutputFrame(HevcContex *pHevcDec, s32 bFlush);
/**************************** h265_ref end *********************************/

/************************** h265_register start *********************************/
u32 HevcGetVeVersion(void);
u32 HevcHardwareFindStartCode(HevcContex *pHevcDec, s32 nSize);
void HevcInitialHwStreamDecode(HevcContex *pHevcDec);
u32 HevcGetbitsHardware(size_addr RegBaseAddr, u32 nBits);
u32 HevcGetUEHardware(size_addr RegBaseAddr);
s32 HevcGetSEHardware(size_addr RegBaseAddr);
s32 HevcDecodeOneSlice(HevcContex *pHevcDec);
s32 HevcVeStatusInfo(HevcContex *pHevcDec, s32 bVeInterruptErrorFlag);
/*************************** h265_register end **********************************/

/************************** h265_md5 start *********************************/
void HevcMd5CaculateSum(u8 *dst,  u8 *src, const int len);
/*************************** h265_md5 end **********************************/

#define HEVC_IS_IDR(eNaluType) \
    (eNaluType == HEVC_NAL_IDR_W_RADL || eNaluType == HEVC_NAL_IDR_N_LP)
#define HEVC_IS_BLA(pHevcDec) \
    (pHevcDec->eNaluType == HEVC_NAL_BLA_W_RADL || \
    pHevcDec->eNaluType == HEVC_NAL_BLA_W_LP || \
    pHevcDec->eNaluType == HEVC_NAL_BLA_N_LP)

#define HEVC_IS_IRAP(pHevcDec) \
(pHevcDec->eNaluType >= 16 && pHevcDec->eNaluType <= 23)

#define HEVC_IS_KEYFRAME(pHevcDec) \
(pHevcDec->eNaluType >= HEVC_NAL_BLA_W_LP && pHevcDec->eNaluType <= HEVC_NAL_CRA_NUT)

#define HevcSmbReadByte(ptr,i) \
ReadSbmByteIdx(ptr, pStreamInfo->pSbmBuf, pStreamInfo->pSbmBufEnd, i)

#define HevcSmbPtrPlusOne(ptr) \
SbmPtrPlusOne(&ptr, pStreamInfo->pSbmBuf, pStreamInfo->pSbmBufEnd)
/*************************** debug function start **********************/
/* These debug function is controlled by MACRO HEVC_DEBUG_FUNCTION_ENABLE */
void HevcDecRequestOneFrameStreamDebug(HevcContex *pHevcDec);
void HevcShowSliceRplInfoDebug(HevcContex *pHevcDec);
void HevcPtsListInfoDebug(HevcContex *pHevcDec, s32 nIdx, s32 bIsAdding, s32 nPoc);
void HevcShowShortRefPicSetInfoDebug(HevcShortTermRPS *pSrps, s32 nPoc);
void HevcShowRefPicSetInfoDebug(HevcRefPicList *pRps);
void HevcDecodeCheckMd5(HevcContex *pHevcDec);
void HevcSettingRegDebug(HevcContex *pHevcDec);
void HevcSaveYuvData(HevcContex *pHevcDec);
void HevcSaveSecYuvData(HevcContex *pHevcDec);
void HevcShowScalingListData(HevcScalingList *pSl, s32 bIsInSps);
void HevcSaveDecoderSliceData(HevcContex *pHevcDec);
void HevcShowMaxUsingBufferNumber(HevcContex *pHevcDec);
/**************************** debug function end  **********************/

static inline s64 HevcGetCurrentTime(void)
{
    struct timeval tv;
    s64 time;
    gettimeofday(&tv,NULL);
    time = tv.tv_sec*1000000 + tv.tv_usec;
    return time;
}

#endif  /* H265_FUNC_H */
