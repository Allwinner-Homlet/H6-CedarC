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
* File : h264_hal.h
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#include "h264.h"
#include "h264_dec.h"

#ifndef H264_V2_FUNC_H
#define H264_V2_FUNC_H

#ifdef __cplusplus
extern "C" {
#endif

int H264DecoderInit(DecoderInterface* pSelf,
                    VConfig* pConfig,
                    VideoStreamInfo* pVideoInfo,
                    VideoFbmInfo* pFbmInfo);
static void Destroy(DecoderInterface* pSelf);
void H264DecoderRest(DecoderInterface* pSelf);
int  H264DecoderSetSbm(DecoderInterface* pSelf, SbmInterface* pSbm, int nIndex);
Fbm* H264DecoderGetFbm(DecoderInterface* pSelf,int index);
int H264DecoderGetFbmNum(DecoderInterface* pSelf);
int  H264DecoderDecodeSbmFrame(DecoderInterface* pSelf,
                       s32               bEndOfStream,
                       s32               bDecodeKeyFrameOnly,
                       s32               bSkipBFrameIfDelay,
                       int64_t           nCurrentTimeUs);
void H264FreeMemory(H264Dec* h264Dec, H264Context* hCtx);
int H264DecoderSetExtraScaleInfo(DecoderInterface* pSelf,s32 nWidthTh, s32 nHeightTh,
                                 s32 nHorizonScaleRatio,s32 nVerticalScaleRatio);

s32 H264InitDecode(H264DecCtx* h264DecCtx, H264Dec* h264Dec, H264Context* hCtx);
s32 H264DecodeExtraData(H264Dec* h264Dec, H264Context* hCtx, u8* extraDataPtr);
s32 H264RequestBitstreamData(H264DecCtx* h264DecCtx,H264Context* hCtx);
s32 H264ProcessNaluUnit(H264DecCtx* h264DecCtx,
                            H264Context* h264Ctx ,
                            u8 nal_unit_type,
                            s32 sliceDataLen,
                            u8 decStreamIndex);
s32 H264ExecuteRefPicMarking(H264Context* hCtx, H264MmcoInfo* mmco, s32 mmcoCount);
u32 H264VeIsr(H264DecCtx* h264DecCtx);
u32 H264GetbitOffset(H264DecCtx* h264DecCtx, u8 offsetMode);

s32 H264CheckBsDmaBusy(H264DecCtx *h264DecCtx);
s32 H264SearchStartcode(H264DecCtx* h264DecCtx, H264Context* hCtx);
void H264SortDisplayFrameOrder(H264DecCtx* h264DecCtx,
                                    H264Context* hCtx,
                                    u8 bDecodeKeyFrameOnly);
void H264CongigureDisplayParameters(H264DecCtx* h264DecCtx, H264Context* hCtx);
void H264ResetDecoderParams(H264Context* hCtx);
void H264SetVbvParams(H264Dec* h264Dec,
                            H264Context* hCtx,
                            u8* startBuf,
                            u8* endBuf,
                            u32 dataLen,
                            u32 dataCtrlFlag);
void H264ConfigureAvcRegister(H264DecCtx* h264DecCtx,
                                H264Context* hCtx,
                                u8 eptbDetectEnable,
                                u32 nBitLens);
void H264ConfigureEptbDetect(H264DecCtx* h264DecCtx,
                                H264Context* hCtx,
                                u32 sliceDataBits,
                                u8 eptbDetectEnable);
//extern void H264InitRegister(H264DecCtx *h264DecCtx);
void H264InitFuncCtrlRegister(H264DecCtx *h264DecCtx);
void H264ProcessDecodeFrameBuffer(H264DecCtx* h264DecCtx,
                                H264Context* hCtx, u8 bForceReturnFrame);
s32 H264MallocFrmBuffer(H264DecCtx* h264DecCtx, H264Context* hCtx);
int H264DecoderSetPerformCmd(DecoderInterface* pSelf,
                                    enum EVDECODERSETPERFORMCMD performCmd);
int H264DecodeGetPerformInfo(DecoderInterface* pSelf,
                                    enum EVDECODERGETPERFORMCMD performCmd,
                                    VDecodePerformaceInfo **performInfo);
int  H264DecoderDecodeSbmStream(DecoderInterface* pSelf,
                       s32               bEndOfStream,
                       s32               bDecodeKeyFrameOnly,
                       s32               bSkipBFrameIfDelay,
                       int64_t           nCurrentTimeUs);


int  H264DecoderDecode(DecoderInterface* pSelf,
                              s32           bEndOfStream,
                              s32           bDecodeKeyFrameOnly,
                              s32           bSkipBFrameIfDelay,
                              int64_t       nCurrentTimeUs);


u32 H264GetFunctionStatus(H264DecCtx* h264DecCtx);
s32 H264DecodeSps(H264DecCtx* h264DecCtx, H264Context* hCtx);
u32 H264GetbitOffset(H264DecCtx* h264DecCtx, u8 offsetMode);
s32 H264DecodePps(H264DecCtx* h264DecCtx, H264Context* hCtx, s32 sliceDataLen);
s32 H264DecodeSliceHeader(H264DecCtx* h264DecCtx, H264Context* hCtx);
u32 H264GetDecodeMbNum(H264DecCtx* h264DecCtx);
u32 H264VeIsr(H264DecCtx* h264DecCtx);

void H264ConfigureBitstreamRegister(H264DecCtx *h264DecCtx,
                                    H264Context* hCtx, u32 nBitLens);
void H264DisableStartcodeDetect(H264DecCtx *h264DecCtx);
void H264SyncByte(H264DecCtx *h264DecCtx);
s32 H264CheckBsDmaBusy(H264DecCtx *h264DecCtx);
void H264EnableIntr(H264DecCtx *h264DecCtx);
void H264DisableStartcodeDetect(H264DecCtx *h264DecCtx);
void H264EnableStartcodeDetect(H264DecCtx *h264DecCtx);
void H264ReferenceRefresh(H264Context* hCtx);
void H264ConfigureSliceRegister(H264DecCtx *h264DecCtx,
                                    H264Context* hCtx, u8 decStreamIndex,u32 sliceDataLen);
 void  H264DecoderRest(DecoderInterface* pSelf);
 int H264DecodeSei(H264DecCtx* h264DecCtx, H264Context* hCtx,s32 nSliceDataLen);
 s32 H264DecodeNalHeaderExt(H264DecCtx* h264DecCtx, H264Context* hCtx);
 void H264FlushDelayedPictures(H264DecCtx* h264DecCtx, H264Context* hCtx);
 void H264ConfigureEptbDetect(H264DecCtx* h264DecCtx,
                                H264Context* hCtx, u32 sliceDataBits, u8 eptbDetectEnable);
 s32  H264SearchStartcode(H264DecCtx* h264DecCtx, H264Context* hCtx);
 void H264InitFuncCtrlRegister(H264DecCtx *h264DecCtx);

 s32 H264HoldFrameBuffer(H264DecCtx* h264DecCtx, H264Context* hCtx);
 s32 h264UpdateProcInfo(H264DecCtx* h264DecCtx, H264Context* hCtx);
 s32 H264ExecuteRefPicMarking(H264Context* hCtx, H264MmcoInfo* mmco, s32 mmcoCount);
 s32 H264ProcessErrorFrame(H264DecCtx* h264DecCtx, H264Context* hCtx);

 s32 H264DecodePictureScanType(H264DecCtx* h264DecCtx, H264Context* hCtx);
 u8 H264SwSearchStartcode(H264Context* hCtx,s32 nSearchSize);

 void H264InitFuncCtrlRegister(H264DecCtx *h264DecCtx);
 void H264SetVbvParams(H264Dec* h264Dec,
                    H264Context* hCtx,
                    u8* startBuf,
                    u8* endBuf,
                    u32 dataLen,
                    u32 dataCtrlFlag);

 void H264FlushDelayedPictures(H264DecCtx* h264DecCtx, H264Context* hCtx);
 void H264ResetDecoderParams(H264Context* hCtx);
 s32 H264HoldFrameBuffer(H264DecCtx* h264DecCtx, H264Context* hCtx);

void H264ProcessDecodeFrameBuffer(H264DecCtx* h264DecCtx,
                                H264Context* hCtx, u8 bForceReturnFrame);


 s32 H264CheckBsDmaBusy(H264DecCtx *h264DecCtx);

 void H264ConfigureBitstreamRegister(H264DecCtx *h264DecCtx, H264Context* hCtx, u32 nBitLens);
 void H264EnableIntr(H264DecCtx *h264DecCtx);
 u32 H264VeIsr(H264DecCtx* h264DecCtx);
 u32 H264GetbitOffset(H264DecCtx* h264DecCtx, u8 offsetMode);
 void H264ConfigureEptbDetect(H264DecCtx* h264DecCtx,
                            H264Context* hCtx,
                            u32 sliceDataBits,
                            u8 eptbDetectEnable);

 s32 H264ProcessNaluUnit(H264DecCtx* h264DecCtx,
                        H264Context* hCtx ,
                        u8 nalUnitType,
                        s32 sliceDataLen, u8 decStreamIndex);

 void H264SortDisplayFrameOrder(H264DecCtx* h264DecCtx,
                            H264Context* hCtx,
                            u8 bDecodeKeyFrameOnly);
 int h264UpdateProcInfo(H264DecCtx* h264DecCtx, H264Context* hCtx);
 s32 H264ExecuteRefPicMarking(H264Context* hCtx, H264MmcoInfo* mmco, s32 mmcoCount);
 s32 H264ProcessErrorFrame(H264DecCtx* h264DecCtx, H264Context* hCtx);
 void H264CongigureDisplayParameters(H264DecCtx* h264DecCtx, H264Context* hCtx);


 int H264ProcessExtraData(H264DecCtx* h264DecCtx,  H264Context* hCtx);

 s32 H264DecodeRefPicListReordering(H264DecCtx* h264DecCtx, H264Context* hCtx);
 s32 H264FillDefaultRefList(H264Context* hCtx,
                    H264PicInfo* pMajorRefFrame, u8 nDecStreamIndex);
 s32 H264DecodeRefPicMarking(H264DecCtx* h264DecCtx, H264Context* hCtx);

 void H264ConfigureSliceRegister(H264DecCtx *h264DecCtx,
                    H264Context* hCtx, u8 decStreamIndex, u32 sliceDataLen);
 void H264CongigureWeightTableRegisters(H264DecCtx* h264DecCtx, H264Context* hCtx);
 void H264DisableStartcodeDetect(H264DecCtx *h264DecCtx);
 void H264EnableStartcodeDetect(H264DecCtx *h264DecCtx);
 u32 H264GetbitOffset(H264DecCtx* h264DecCtx, u8 offsetMode);
 void H264FlushDelayedPictures(H264DecCtx* h264DecCtx, H264Context* hCtx);
 void H264ResetDecoderParams(H264Context* hCtx);

#ifdef __cplusplus
}
#endif

#endif

