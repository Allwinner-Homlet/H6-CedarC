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
* File : h264_dec.h
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#ifndef H264_V2_DEC_H
#define H264_V2_DEC_H

#include "h264.h"
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PICT_TOP_FIELD     1
#define PICT_BOTTOM_FIELD  2
#define PICT_FRAME         3

#define MAX_SPS_COUNT 32
#define MAX_PPS_COUNT 256
#define H264VDEC_MAX_STREAM_NUM 8
#define H264VDEC_MAX_EXTRA_DATA_LEN 1024
#define H264VDEC_ERROR_PTS_VALUE  (s64)-1 //0xFFFFFFFFFFFFFFFF

#define H264_MVC_PARAM_BUF_SIZE 32*1024
#define FIRST_SLICE_DATA 4
#define LAST_SLICE_DATA  2
#define SLICE_DATA_VALID 1
#define INIT_MV_BUF_CALCULATE_NUM 0xFFFFFFFF
#if 1    //added by xyliu for DTMB at 2015-12-16
#define DTMB_WIDTH_HEIGHT_NUM 16

typedef struct RCD_WIDTH_HEIGHT_INF
{
    u32 nRcdWidth;
    u32 nRcdHeight;
    u32 nRcdNum;
}RcdWidthHeightInf;

#endif

#define H264_P_TYPE  0 ///< Predicted
#define H264_B_TYPE  1 ///< Bi-dir predicted
#define H264_I_TYPE  2 ///< Intra
#define H264_SP_TYPE 3 ///< Switching Predicted
#define H264_SI_TYPE 4 ///< Switching Intra

#define DELAYED_PIC_REF 4

#define H264_IR_NONE       0
#define H264_IR_FINISH     1
#define H264_IR_ERR         2
#define H264_IR_DATA_REQ 4

#define H264_GET_VLD_OFFSET 0
#define H264_GET_STCD_OFFSET 1
#define MAX_MMCO_COUNT 66
#define MAX_PICTURE_COUNT 32
//#define H264_EXTENDED_DATA_LEN 32
#define H264_EXTENDED_DATA_LEN 0
#define H264_START_DEC_FRAME 1
#define H264_END_DEC_FRAME 2
#define VRESULT_DEC_FRAME_ERROR 0xFF
#define VRESULT_ERR_FAIL 0xFB
#define VRESULT_DROP_B_FRAME 0xFC
#define VRESULT_REDECODE_STREAM_DATA 0xFA

#define H264_MVC_PARAM_BUF_LEN 4096

enum AVC_DEC_STEP
{
    H264_STEP_CONFIG_VBV  = 0,
    H264_STEP_UPDATE_DATA = 1,
    H264_STEP_SEARCH_NALU = 2,
    H264_STEP_SEARCH_AVC_NALU = 3,
    H264_STEP_PROCESS_DECODE_RESULT = 4,
    H264_STEP_JUDGE_FRAME_END = 5,
    H264_STEP_REQUEST_FRAME_STREAM  = 6,
    H264_STEP_DECODE_ONE_FRAME   = 7,
    H264_STEP
};

enum AVC_FRAME_RATIO
{
    H264_CODED_FRAME_RATIO_NONE  = 0,
    H264_CODED_FRAME_RATIO_4_3   = 1,
    H264_CODED_FRAME_RATIO_14_9  = 2,
    H264_CODED_FRAME_RATIO_16_9  = 3,
    H264_CODED_FRAME_RATIO_OTHER = 4,
    H264_CODED_FRAME_RATIO_
};

enum CROP_INDEX
{
    TOP,
    RIGHT,
    BOTTOM,
    LEFT
};

typedef struct H264_VBV
{
    SbmInterface*    vbv;
    VideoStreamDataInfo* pVbvStreamData;
    u8* pVbvBuf;           // the start address of the vbv buffer
    u8* pReadPtr;          // the current read pointer of the vbv buffer
    u8* pVbvBufEnd;        // the end address of the vbv buffer
    u8* pVbvDataEndPtr;
    u8* pVbvDataStartPtr;
    s32 nCurNaluIdx;
    s32 nVbvDataSize;      // the valid data size of the vbv buffer
    u32 nVbvBufPhyAddr;
    u32 nVbvBufEndPhyAddr;
    u32 bVbvDataCtrlFlag;
    u32 nFrameRate;
    s64 nVbvDataPts;
    s64 nValidDataPts;
    s64 nPrePicPts;
    s64 nNextPicPts;
    s32 nPrePicPoc;
    s64 nPicDuration;
    s64 nLastValidPts;
    s64 nLastUsedValidPts;
}H264Vbv;

typedef struct H264_PIC_INF
{
    s32 nFieldPoc[2];           ///< h264 top/bottom POC
    s32 nPoc;                    ///< h264 frame POC
    s32 nFrmNum;              ///< h264 frame_num (raw frame_num from slice header)
    s32 nPicId;                 /**< h264 pic_num (short -> no wrap version of pic_num,
                                pic_num & max_pic_num; long -> long_pic_num) */
    s32 bLongRef;               ///< 1->long term nReference 0->short term nReference
    s32 nRefPoc[2][16];         ///< h264 POCs of the frames used as nReference
    s32 nRefCount[2];           ///< number of entries in ref_nPoc
    s32 bFrameScore;          /* */
    s32 nReference;
    u8  bKeyFrame;
    s32 nPictType;
    s32 nPicStructure;
    s32 nFieldPicStructure[2];
    u32 nDecodeBufIndex;
    u32 nDispBufIndex;
    u8* pTopMvColBuf;
    size_addr phyTopMvColBuf;
    u8* pBottomMvColBuf;
    size_addr phyBottomMvColBuf;
    s32 nH264MvBufIndex;
    VideoPicture* pVPicture;
    VideoPicture* pScaleDownVPicture;
    u8 nNalRefIdc;
    u8 nFieldNalRefIdc[2];
    u8 bMbAffFrame;
    u8 bCodedFrame;
    u32 nDecFrameOrder;
    u8  bHasDispedFlag;
    u8  bDecErrorFlag;
    u8  bDecFirstFieldError;
    u8  bDecSecondFieldError;
    u8  bDropBFrame;
//****************for mvc************************//
}H264PicInfo;

typedef struct H264_FRM_BUF_INF
{
    s32 nMaxValidFrmBufNum;
    u32 nRef1YBufAddr;
    u32 nRef1CBufAddr;
    u32 nRef1MafBufAddr;
    u32 nRef2MafBufAddr;

    u8* pMvColBuf;
    H264PicInfo picture[MAX_PICTURE_COUNT];
    H264PicInfo* pLongRef[MAX_PICTURE_COUNT];
    H264PicInfo* pShortRef[MAX_PICTURE_COUNT];
    H264PicInfo  defaultRefList[2][MAX_PICTURE_COUNT];
    H264PicInfo  refList[2][48];
    H264PicInfo* pDelayedPic[MAX_PICTURE_COUNT];
    H264PicInfo* pDelayedOutPutPic;
    H264PicInfo *pLastPicturePtr;     ///< pointer to the previous picture.
    H264PicInfo *pNextPicturePtr;     ///< pointer to the next picture (for bidir pred)
    H264PicInfo *pCurPicturePtr;      ///< pointer to the current picture
}H264FrmBufInfo;

typedef struct H264_SPS_INFO
{
    u8  bMbAff;
    u8  nPocType;
    u8  bDeltaPicOrderAlwaysZeroFlag;
    u8  bFrameMbsOnlyFlag;
    u8  bDirect8x8InferenceFlag;
    s32 nRefFrmCount;
    s32 gaps_in_frame_num_value_allowed_flag;
    u32 nMbWidth;
    u32 nMbHeight;
    u32 nFrmMbWidth;
    u32 nFrmMbHeight;
    u32 nFrmRealWidth;
    u32 nFrmRealHeight;
    u32 nCropLeft;
    u32 nCropRight;
    u32 nCropTop;
    u32 nCropBottom;
    u8 bCodedFrameRatio;
    u32 nAspectRatio;

    s32 nLog2MaxFrmNum;
    s32 bScalingMatrixPresent;
    u32 nLog2MaxPocLsb;
    s32 nOffsetForNonRefPic;
    s32 nOffsetForTopToBottomField;
    u32 nPocCycleLength;
    u32 nProfileIdc;
    u8 nScalingMatrix4[6][16];
    u8 nScalingMatrix8[2][64];
    u16 nOffsetForRefFrm[256];
    s32 bBitstreamRestrictionFlag;
    s32 nNumReorderFrames;
    u8 bLowDelayFlag;
    u8 bPicStructPresentFlag;
    u32 bNalHrdParametersPresentFlag;
    u32 bVclHrdParametersPresentFlag;
    u32 nCpbRemovalDelayLength;
    u32 nDpbOutputDelayLength;
    u32 nPicDuration;
    u8  bConstraintSetFlags;
    u8 nTimeOffsetLength;
}H264SpsInfo;

typedef struct H264_PPS_INFO
{
    u8  bScalingMatrixPresent;
    u32 nSpsId;
    s32 bCabac;
    s32 nInitQp;                ///< pic_init_qp_minus26 + 26
    u32 nSliceGroupCount;
    u32 nMbSliceGroupMapType;
    u8 pPicOrderPresent;
    u8 pRedundatPicCntPresent;
    u32 nRefCount[2];
    s32 bWeightedPred;          ///< weighted_pred_flag
    s32 nWeightedBIpredIdc;
    s32 bDeblockingFilterParamPresent;
    s32 bTransform8x8Mode;
    s32 nChromaQpIndexOffset[2];
    s32 bConstrainedIntraPred; ///< constrained_intra_pred_flag
    u8 nScalingMatrix4[6][16];
    u8 nScalingMatrix8[2][64];
}H264PpsInfo;

/**
 * Memory management control operation opcode.
 */
typedef enum H264_MMCO_OPCODE
{
    MMCO_END = 0,
    MMCO_SHORT_TO_UNUSED,
    MMCO_LONG_TO_UNUSED,
    MMCO_SHORT_TO_LONG,
    MMCO_SET_MAX_LONG,
    MMCO_RESET,
    MMCO_LONG,
}h264_mmco_opcode;

/**
 * Memory management control operation.
 */
typedef struct H264_MMCO_INFO
{
    h264_mmco_opcode opCode;
    s32 nShortPicNum;  ///< pic_num without wrapping (pic_num & max_pic_num)
    s32 bLongArg;       ///< index, pic_num, or num long refs depending on opcode
}H264MmcoInfo;

typedef struct H264_FRM_REGISTER_INFO
{
    u8  bBufUsedFlag;
    s32 nFrameBufIndex;
    s32 nFirstFieldPoc;
    s32 nSecondFieldPoc;
    s32 nFrameStruct;
    s32 nTopRefType;
    s32 nBottomRefType;
}H264FrmRegisterInfo;

typedef struct H264_MVC_SPS_EXT_PARAM
{
    u32     nNumViews;
    u32*      nViewId;
    u32*      nNumAnchorRefs[2];
    u32**     nAnchorRef[2];
    u32*      nNumNonAnchorRefs[2];
    u32**     nNonAnchorRef[2];
}H26MvcSpsExtParam;

typedef struct H264_MVC_SLICE_EXT_PARAM
{
    s16 bNonIdrFlag;
    s16 nPriorityId;
    s16 nViewId;
    s16 nTemporalId;
    s16 bAnchorPicFlag;
    s16 bInterViewFlag;
}H264MvcSliceExtParam;

typedef struct H264_MVC_CONTEXT
{
    u8*                     pMvcParamBuf;
    H264MvcSliceExtParam*   pH264MvcSliceExtParam;     // mvc parames
    //H26MvcSpsExtParam*      pH264MvcSpsExtParam;
}H264MvcContext;

typedef struct H264_DEBUG_INFO
{
    s32 nDeltaFrameNum;
    s32 nTotalFrameNum;
    int64_t nStartTime;
    int64_t nDeltaTime;
    int64_t nTotalTime;

    s32        nDeltaStreamNum;
    s32        nTotalStreamNum;
    int64_t nBRStartTime;
    int64_t nBRDeltaStartTime;
    int64_t nCurrentBits;
    int64_t nTotalBits;
}H264Debug;

typedef struct H264_MOTION_VECTOR_BUF_MANAGER
{
    u8* pTopMvColBuf;
    size_addr phyTopMvColBuf;
    u8* pBottomMvColBuf;
    size_addr phyBottomMvColBuf;
    s32 nH264MvBufIndex;
    u32 nCalculateNum;
    H264PicInfo *pCurPicturePtr;
}H264MvBufManager;

typedef struct H264_MOTION_VECTOR_BUF_INF
{
    H264MvBufManager* pNextMvBufManager;
    H264MvBufManager* pH264MvBufManager;
    void*             param;
}H264MvBufInf;

typedef struct H264_CONTEXT
{
    s8  bIsAvc;                    ///< this flag is != 0 if codec is avc1
    s32 nNalLengthSize;             ///< Number of bytes used for nal length (1, 2 or 4)
    u8* pExtraDataBuf;
    u32 nExtraDataLen;
    u8  bDecExtraDataFlag;
    int  nNalRefIdc;
    u8  nalUnitType;

    u8  bFstField;
    u8  nPicStructure;
    u8  nRedundantPicCount;
    u8  nSliceType;
    u8  nFrameType;

    s32  nDeltaPoc[2];
    u32 nListCount;
    s32 nLongRefCount;  ///< number of actual long term nReferences
    s32 nShortRefCount; ///< number of actual short term nReferences
    s32 nRefCount[2];

    u32 nCurSliceNum;                     // current slice number
    u32 nLastSliceType;
    s32 nFrmNum;
    s32 nLastFrmNum;
    u32 nCurPicNum;
    u32 nMaxPicNum;
    u32 nMbX;
    u32 nMbY;
    s32 bDirectSpatialMvPred;
    s32 bMbAffFrame;
        //s32 bMbFieldDecodingFlag;
    s32 nCabacInitIdc;
    s32 nLastQscaleDiff;
    s32 nQscale;
    s32 bDisableDeblockingFilter;         ///< disable_deblocking_filter_idc with 1<->0
    s32 nSliceAlphaC0offset;
    s32 nSliceBetaoffset;
    s32 bNumRefIdxActiveOverrideFlag;
    s32 nChromaQp[2];

    s32 nDeltaPocBottom;
    s32 bFstMbInSlice;
    s32 bLastMbInSlice;
    s32 nFrmNumOffset;
    s32 nPrevFrmNumOffset;
    s32 nPrevFrmNum;
    s32 nPrevPocLsb;
    s32 nPrevPocMsb;
    s32 nPocMsb;
    s32 nPocLsb;
    s32 nPocType;
    s32 nMmcoIndex;

    u8  bFrameMbsOnlyFlag;
    u8  bDirect8x8InferenceFlag;
    s32 nRefFrmCount;
    u32 nMbWidth;
    u32 nMbHeight;
    u32 nFrmMbWidth;
    u32 nFrmMbHeight;
    u32 nCropLeft;
    u32 nCropRight;
    u32 nCropTop;
    u32 nCropBottom;
    u32 nFrmRealWidth;
    u32 nFrmRealHeight;

#if 1
    H264SpsInfo* pCurSps;                ///< current pSps
    H264PpsInfo* pCurPps;               // current pPps
    u8          nSpsBufferNum;
    u8          nPpsBufferNum;
    u8          nSpsBufferIndex[32];
    u8          nPpsBufferIndex[32];

    H264SpsInfo *pSpsBuffers[MAX_SPS_COUNT];
    H264PpsInfo *pPpsBuffers[MAX_PPS_COUNT];
    u32          nNewSpsId;
#else
    H264SpsInfo pSps;               ///< current pSps
    H264PpsInfo pPps;               // current pPps

    u32  nSpsId;               ///< current pSps
    u32  nPpsId;
#endif

    H264MmcoInfo mmco[MAX_MMCO_COUNT];
    u32 nScalingMatrix4[6][4];
    u32 nScalingMatrix8[2][16];

    H264FrmBufInfo frmBufInf;     // the information of the frame buffer
    H264Vbv vbvInfo;           // the information of the h264 vbv buffer
    FramePicInfo* pCurStreamFrame;

    u8* pMbFieldIntraBuf;
    u8* pMbNeighborInfoBuf;
    size_addr phyMbFieldIntraBuf;
    size_addr uMbNeighBorInfo16KAlignBufPhy; //* 16K-align to pMbNeighborInfoBuf
    u32 bNeedFindIFrm;
    u32 bNeedFindPPS;
    u32 bNeedFindSPS;
    u32 bScalingMatrixPresent;
    u8  nDecFrameStatus;

    u8  bUseMafFlag;       // support maf
    u8* pMafInitBuffer;
    u32 nMafBufferSize;
    u32 nCurFrmNum;
    s32 nDelayedPicNum;
    s32 nMinDispPoc;
    //u8  bOnlyDecKeyFrame;
    u8  bProgressice;
    s64 nEstimatePicDuration;
    u8  nPicPocDeltaNum;
    u8*     pVbvBase;
    u32     nVbvSize;
    SbmInterface*    pVbv;
    Fbm*    pFbm;
    Fbm*    pFbmScaledown;
    u8      nDecStep;           // the status of the mpeg2 decoder
    u8      bNeedCheckFbmNum;
    s32     nPoc;
    u8      bIdrFrmFlag;
    s32     nBufIndexOffset;
    s32        nLeftOffset;
    s32     nRightOffset;
    s32     nTopOffset;
    s32     nBottomOffset;
    u8      bUseDramBufFlag;
    u8*     pDeblkDramBuf;
    size_addr phyDeblkDramBuf;
    u8*     pIntraPredDramBuf;
    size_addr phyIntraPredDramBuf;

    u8      bYv12OutFlag;
    u8      bNv21OutFlag;
    u8      bRefYv12DispYv12Flag;
    u8      bRefNv21DispNv21Flag;
    u32     nDispCSize;
    u32     nRefCSize;
    u32     nRefYLineStride;
    u32     nRefCLineStride;
    u32     nDispYLineStride;
    u32     nDispCLineStride;
    void*   pMvcContext;
    s32     bEndOfStream;
    s64     nSystemTime;
    s32     bSkipBFrameIfDelay;
    u8      bFrameEndFlag;
    s32     bDecodeKeyFrameOnly;

    H264Debug debug;
    s32     nColorPrimary;
    u32     nSeiRecoveryFrameCnt;

    u8      nRequestBufFailedNum;
    u8      bIs4K;
    u8      bDropFrame;
    u8      bPreDropFrame;
    u8      bLastDropBFrame;
    u32     nByteLensCurFrame;

    RcdWidthHeightInf recordWidthHeightInfo[DTMB_WIDTH_HEIGHT_NUM];
    //added by xyliu for DTMB at 2015-12-16

    u32     nDecFrameCount;

    H264MvBufInf* pH264MvBufInf;
    int  nH264MvBufNum;
    H264MvBufInf* pH264MvBufEmptyQueue;
}H264Context;

typedef struct EXTRASCALEINFO
{
    s32 nWidthTh;
    s32 nHeightTh;
    s32 nHorizonScaleRatio;
    s32 nVerticalScaleRatio;
}ExtraScaleInf;

enum H264PERFORMINFO
{
    H264_PERFORM_NONE         = 0,
    H264_PERFORM_CALDROPFRAME = 1,
};

typedef struct H264_PERFORM_INF
{
    u32 performCtrlFlag;
    VDecodePerformaceInfo H264PerfInfo;
}H264PerformInf;


#define H264_HEADER_BUF_SIZE 1*1024

typedef struct H264_DECODER
{
    u8* dataBuffer;
    u64          nVeVersion;
    size_addr    nRegisterBaseAddr;
    H264Context* pHContext;
    H264Context* pMinorHContext;
    u8           nDecStreamIndex;   // 0: major stream, 1: minor stream
    H264PicInfo* pMajorRefFrame;
    //* memory methods.

    ExtraScaleInf H264ExtraScaleInf;
    H264PerformInf H264PerformInf;
    struct ScMemOpsS *memops;
    VConfig* pConfig;
    u8 *pDecHeaderBuf;
    u32 nDecHeaderBufSize;
    u32 emulateIndex[1024];
    u32 nEmulateNum;
}H264Dec;

static inline int64_t H264GetCurrentTime(void)
{
    struct timeval tv;
    int64_t time;
    gettimeofday(&tv,NULL);
    time = tv.tv_sec*1000000 + tv.tv_usec;
    return time;
}

int H264ProcessExtraData2(H264DecCtx* h264DecCtx,  H264Context* hCtx);

s32 H264RequestOneFrameStream(H264DecCtx* h264DecCtx, H264Context* hCtx);
#define H264_DEBUG_STRUCT(a) (((a)==3) ? "FRAME" : (((a)==1) ? "TOP" : "BOTTOM"))

#ifdef __cplusplus
}
#endif

#endif
