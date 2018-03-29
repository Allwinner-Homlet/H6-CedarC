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
* File : h265_var.h
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#ifndef H265_VAR_H
#define H265_VAR_H

#include "h265_config.h"
//#include <hardware/sunxi_metadata_def.h>
#include "sunxi_metadata_def.h"

#define ENABLE_MV_BUF_OPTIMIZATION_PROGRAM (0)


#define HEVC_TRUE  (1)
#define HEVC_FALSE (0)

#define HEVC_MAX_FRAMES_ONE_SQUEUE (256)
#define HEVC_POC_INIT_VALUE (0x1fffffff)
typedef enum HevcDecodeFrameResult
{
    HEVC_RESULT_OK                    = 0,  // continue
    HEVC_RESULT_NULA_DATA_ERROR       = 31, // continue
    HEVC_RESULT_NO_FRAME_BUFFER       = 32, // return
    HEVC_RESULT_PICTURES_SIZE_CHANGE  = 33, // return
    HEVC_RESULT_RESET_INTRA_DECODER   = 34, // return
    HEVC_RESULT_DROP_THE_FRAME        = 35, // return
    HEVC_RESULT_INIT_FBM_FAILED       = 36  // return
}HevcDecodeFrameResult;

typedef enum HevcDecoderStep
{
    H265_STEP_REQUEST_STREAM = 0,
    H265_STEP_DECODE_FRAME,
    H265_STEP_REQUEST_FBM_BUFFER,
    H265_STEP_PROCESS_RESULT,
    H265_STEP_CRASH = 0xff,
    H265_STEP
} HevcDecoderStep;

typedef enum HevcNaluType
{
    HEVC_NAL_TRAIL_N    = 0,
    HEVC_NAL_TRAIL_R    = 1,
    HEVC_NAL_TSA_N      = 2,
    HEVC_NAL_TSA_R      = 3,
    HEVC_NAL_STSA_N     = 4,
    HEVC_NAL_STSA_R     = 5,
    HEVC_NAL_RADL_N     = 6,
    HEVC_NAL_RADL_R     = 7,
    HEVC_NAL_RASL_N     = 8,
    HEVC_NAL_RASL_R     = 9,
    HEVC_NAL_BLA_W_LP   = 16,
    HEVC_NAL_BLA_W_RADL = 17,
    HEVC_NAL_BLA_N_LP   = 18,
    HEVC_NAL_IDR_W_RADL = 19,
    HEVC_NAL_IDR_N_LP   = 20,
    HEVC_NAL_CRA_NUT    = 21,
    HEVC_NAL_VPS        = 32,
    HEVC_NAL_SPS        = 33,
    HEVC_NAL_PPS        = 34,
    HEVC_NAL_AUD        = 35,
    HEVC_NAL_EOS_NUT    = 36,
    HEVC_NAL_EOB_NUT    = 37,
    HEVC_NAL_FD_NUT     = 38,
    HEVC_NAL_SEI_PREFIX = 39,
    HEVC_NAL_SEI_SUFFIX = 40,
    HEVC_UNSPEC63          = 63
}HevcNaluType;

typedef enum HevcSliceType
{
    HEVC_B_SLICE = 0,
    HEVC_P_SLICE = 1,
    HEVC_I_SLICE = 2,
}HevcSliceType;

typedef enum HevcRPSType
{
    HEVC_ST_CURR_BEF = 0,
    HEVC_ST_CURR_AFT,
    HEVC_ST_FOLL,
    HEVC_LT_CURR,
    HEVC_LT_FOLL,
    HEVC_NB_RPS_TYPE,
}HevcRPSType;

/**************** sacling list start todo: change code style **********************/

enum ScalingListSize
{
  SCALING_LIST_4x4 = 0,
  SCALING_LIST_8x8 = 1,
  SCALING_LIST_16x16 = 2,
  SCALING_LIST_32x32 = 3,
  SCALING_LIST_SIZE_NUM = 4
};

enum COEFF_SCAN_TYPE
{
  SCAN_DIAG = 0,         ///< up-right diagonal scan
  SCAN_HOR,              ///< horizontal first scan
  SCAN_VER               ///< vertical first scan
};

/***************** sacling list end todo: change code style***********************/
typedef struct GetBitContext
{
    const char *buffer, *buffer_end;
    s32 index;
    s32 size_in_bits;
    s32 size_in_bits_plus8;
    FILE *pFileDebug;
    u32 bUseHardware;
    size_addr RegBaseAddr;
} GetBitContext;

typedef struct HevcWindow
{
    s32 bOffsetAvailable;
    s32 nLeftOffset;
    s32 nRightOffset;
    s32 nTopOffset;
    s32 nBottomOffset;
} HevcWindow;

/**
 * rational number numerator/denominator
 */
typedef struct HvecRational
{
    int num; ///< numerator
    int den; ///< denominator
} HvecRational;

typedef struct HevcVUI
{
    HvecRational Sar;
    s8 bOverscanInfoPresentFlag;
    s8 bOverScanAppropriteFlag;

    s8 bVideoSignalTypePresentFlag;
    s32 nVideoFormat;
    s8 bVideoFullRangeFlag;
    u8 bColourDescriptionPresentFlag;
    u8 nColourPrimaries;
    u8 nTransferCharacteristic;
    u8 nMatrixCoeffs;

    s8 bChromaLocInfoPresentFlag;
    s32 nChromaSampleLocTypeTopField;
    s32 nChromaSampleLocTypeBottomField;
    s8 bNeutraChromaIndicationFlag;

    s8 bFieldSeqFlag;
    s8 bFrameFieldInfoPresentFlag;

    s8 bDefaultDisplayWindowFlag;
    HevcWindow DefDispWin;

    s8 bVuiTimingInfoPresentFlag;
    u32 nVuiNumUnitsInTick;
    u32 nVuiTimeScale;
    s8 bVuiPocProportionalToTimingFlag;
    s32 nVuiNumTicksPocDiffOneMinus1;
    s8 bVuiHrdParametersPresentFlag;

    s8 bBitStreamRestrictionFlag;
    s8 bTilesFixedStructureFlag;
    s8 bMotionVectorsOverPicBoundariesFlag;
    s8 bRestrictedRefPicListsFlag;
    s32 nMinSpatialSegmentationIdc;
    s32 nMaxBytesPerPicDenom;
    s32 nMaxBitsPerMinCuDenom;
    s32 nLog2MaxMvLengthHorizontal;
    s32 nLog2MaxMvLengthVertical;
} HevcVUI;

typedef struct HevcPTLCommon
{
    u8 nProfileSpace;
    u8 bTierFlag;
    u8 nProfileIdc;
    u8 bProfileCompatibilityFlag[32];
    u8 nLevelIdc;
    u8 bProgressiveSourceFlag;
    u8 bInterlacedSourceFlag;
    u8 bNonPackedConstraintFlag;
    u8 bFrameOnlyConstraintFlag;
} HevcPTLCommon;

typedef struct HevcPTL
{
    HevcPTLCommon GeneralPtl;
    HevcPTLCommon SubLayerPtl[HEVC_MAX_SUB_LAYERS];

    u8 bSubLayerProfilePresentFlag[HEVC_MAX_SUB_LAYERS];
    u8 bSubLayerLevelPresentFlag[HEVC_MAX_SUB_LAYERS];

    s32 nSubLayerProfileSpace[HEVC_MAX_SUB_LAYERS];
    u8 bSubLayerTierFlag[HEVC_MAX_SUB_LAYERS];
    s32 nSubLayerProfileIdc[HEVC_MAX_SUB_LAYERS];
    u8 bSubLayerProfileCompatibilityFlags[HEVC_MAX_SUB_LAYERS][32];
    s32 nSubLayerLeveIdc[HEVC_MAX_SUB_LAYERS];
} HevcPTL;

typedef struct HevcScalingList
{
    /* This is a little wasteful, since sizeID 0 only needs 8 coeffs,
     * and size ID 3 only has 2 arrays, not 6. */
    u8 Sl[4][6][64];
    u8 SlDc[2][6];
    u8 nRefMatrixId[SCALING_LIST_SIZE_NUM][HEVC_SCALING_LIST_NUM];
    s32 bScalingListMatrixModified;
} HevcScalingList;

typedef struct HevcShortTermRPS
{
    s32 nNumNegativePics;
    s32 nNumDeltaPocs; /* equal to (nNumNegativePics + nNumPositivePics) */
    s32 nDeltaPoc[32];
    u8  bUsed[32];
} HevcShortTermRPS;

typedef struct HevcMD5
{
    u64 len;
    u8  block[64];
    u32 ABCD[4];
} HevcMD5;

typedef struct HevcLongTermRPS
{
    s32 nPoc[32];
    u8     bUsed[32];
    s32 nNumOfRefs;
} HevcLongTermRPS;

typedef struct HevcVPS
{
    u8 bVpsTemporalIdNestingFlag;
    s32 nVpsMaxLayers;
    s32 nVpsMaxSubLayers; ///< vps_max_temporal_layers_minus1 + 1

    HevcPTL Ptl;
    s32 bVpsSubLayerOrderingInfoPresentFlag;
    u32 nVpsMaxDecPicBuffering[HEVC_MAX_SUB_LAYERS];
    u32 nVpsNumReorderPics[HEVC_MAX_SUB_LAYERS];
    u32 nVpsMaxLatencyIncrease[HEVC_MAX_SUB_LAYERS];
    s32 nVpsMaxLayerId;
    s32 nVpsNumLayerSets; ///< vps_num_layer_sets_minus1 + 1
    u8 bVpsTimingInfoPresentFlag;
    u32 nVpsNumUnitsInTick;
    u32 nVpsTimeScale;
    u8  bVpsPocProportionalToTimingFlag;
    s32 nVpsNumTicksPocDiffOne; ///< vps_num_ticks_poc_diff_one_minus1 + 1
    s32 nVpsNumHrdParameters;
    s32 bVpsExtensionFlag;
} HevcVPS;

typedef struct HevcSPS
{
    u32 nVpsId;
    s32 nChromaFormatIdc;
    u8 bSeparateColourPlaneFlag;

    ///< output (i.e. cropped) values
    s32 output_width, output_height;
    HevcWindow OutPutWindow;
    HevcWindow PicConfWin;

    s32 nBitDepthLuma;
    s32 nBitDepthChroma;
    s32 pixel_shift;
//    enum AVPixelFormat pix_fmt;

    u32 nLog2MaxPocLsb;
    s32 bPcmEnableFlag;

    s32 nMaxSubLayers;
    struct Tem_poralL_ayer
    {
        s32 nSpsMaxDecPicBuffering;
        s32 nSpsMaxNumReoderPics;
        s32 nSpsMaxLatencyIncrease;
    } TemporalLayer[HEVC_MAX_SUB_LAYERS];

    HevcVUI vui;
    HevcPTL Ptl;

    u8 bScalingListEnableFlag;
    u8 bSpsScalingListDataPresentFlag;
    HevcScalingList ScalingList;

    u32 nNumStRps;
    HevcShortTermRPS StRps[HEVC_MAX_SHORT_TERM_RPS_COUNT];

    u8 bAmpEnableFlag;
    u8 bSaoEnabled;

    u8 bLongtermRefPicsPresentFlag;
    u16 nLtRefPicPocLsbSps[32];
    u8 bUsedByCurrPicLtSpsFlag[32];
    u8 nNumLongTermRefPicsSps;

    struct PCM
    {
        u8 nPcmBitDepthLuma;
        u8 nPcmBitdepthChroma;
        u32 nLog2MinPcmCbSize;
        u32 nLog2DiffMaxMinPcmCbSize;
        u8 bLoopFilterDisableFlag;
    } pcm;
    u8 bSpsTemporalMvpEnableFlag;
    u8 bSpsStrongIntraSmoothingenableFlag;

    u32 nLog2MinCbSize;
    u32 nLog2DiffMaxMinCbSize;
    u32 nLog2MinTbSize;
    u32 nLog2DiffMaxMinTbSize;
    u32 nLog2MaxTransfornBlockSize;
    u32 nLog2CtbSize;
    u32 nLog2MinPusize;

    s32 nMaxTransformHierarchydepthInter;
    s32 nMaxTransformHierarchydepthIntra;

    ///< coded frame dimension in various units
    s32 nWidth;
    s32 nHeight;
    s32 nCedarxLeftOffset;
    s32 nCedarxRightOffset;
    s32 nCedarxTopOffset;
    s32 nCedarxBottomOffset;

    s32 nCedarxLeftOffsetSd; /* scale down parameter */
    s32 nCedarxRightOffsetSd;
    s32 nCedarxTopOffsetSd;
    s32 nCedarxBottomOffsetSd;

    s32 nCtbWidth;
    s32 nCtbHeight;

    s32 nCtbSize;
    s32 nMinCbWidth;
    s32 nMinCbHeight;
    s32 nMinTbWidth;
    s32 nMinTbHeight;
    s32 nMinPuWidth;
    s32 nMinPuHeight;

    s32 hshift[3];
    s32 vshift[3];

    s32 nQpBdOffset;
} HevcSPS;

typedef struct HevcTilePositionInfo
{
    s16 nTileStartAddrX;
    s16 nTileStartAddrY;
    s16 nTileEndAddrX;
    s16 nTileEndAddrY;
}HevcTilePositionInfo;

typedef struct HevcPPS
{
    u32 nSpsId; ///< seq_parameter_set_id

    u8 bDependentSliceSegmentsEnabledFlag;
    u8 bOutputFlagPresentFlag;
    u8 bSignDataHidingflag;

    u8 bCabacInitPresentFlag;

    s32 nNumRefIdxL0DefaultActive; ///< num_ref_idx_l0_default_active_minus1 + 1
    s32 nNumRefIdxL1DefaultActive; ///< num_ref_idx_l1_default_active_minus1 + 1
    s32 nInitQpMinus26;

    u8 bConstrainedIntraPredFlag;
    u8 bTransformSkipEnabledFlag;

    u8 bCuQpDeltaEnabledFlag;
    s32 nDiffCuQpDeltaDepth;

    s32 nPpsCbQpOffset;
    s32 nPpsCrQpOffset;
    u8 bPpsSliceChromaQpOffsetsPresentFlag;
    u8 bWeightedPredFlag;
    u8 bWeightedBipredFlag;

    u8 bTransquantBypassEnableFlag;

    u8 bTilesEnabledFlag;
    u8 bEntropyCodingSyncEnabledFlag;

    s32 nNumTileColums;   ///< num_tile_columns_minus1 + 1
    s32 nNumTileRows;      ///< num_tile_rows_minus1 + 1
    u8 bUniformSpacingFlag;
    u8 bLoopFilterAcrossTilesEnableFlag;

    u8 bPpsLoopFilterAcrossSlicesEnableFlag;

    u8 bDeblockingFilterControlPresentFlag;
    u8 bDeblocingFilterOverrideEnableFlag;
    u8  bPpsDeblockingDisableFlag;
    s32 nPpsBetaOffset;    ///< beta_offset_div2 * 2
    s32 nPpsTcOffset;      ///< tc_offset_div2 * 2

    u8  bPpsScalingListDataPresentFlag;
    HevcScalingList ScalingList;

    u8 bListsModificationPresentFlag;
    s32 nLog2ParallelMergeLevelMinus2;
    s32 nNumExtraSliceHeaderBits;
    u8 bSliceHeaderExtentionPresentFlag;

    u8 bPpsExtensionFlag;
    u8 bPpsExtensionDataFlag;

    // Inferred parameters
    u32 *ColumnWidth;
    u32 *RowHeight;

    s32 *ColumBoundary;
    s32 *RowBoundary;
    s32 *ColumIdxX;

    s32 *CtbAddrRsToTs;
    s32 *CtbAddrTsToRs;
    s32 *TileId;          /* TileId in ts */
    s32 *TileIdInRs;     /* TileId in ts */
    HevcTilePositionInfo *TilePosInfo;
    s32 *TilePositionRs;       ///< TilePosRS
    s32 *min_cb_addr_zs;    ///< MinCbAddrZS
    s32 *min_tb_addr_zs;    ///< MinTbAddrZS
} HevcPPS;

typedef struct HevcSeiDisInfo
{
    u32 bHdrFlag;
    //s32 seiMstDisInfoPresent;
    u16 displayPrimariesX[3];
    u16 displayPrimariesY[3];
    u16 whitePointX;
    u16 whitePointY;
    u32 maxMasteringLuminance;
    u32 minMasteringLuminance;

    u8  prefTranChara;

    u16 maxContentLightLevel;
    u16 maxPicAverLightLevel;
}HevcSeiDisInfo;

typedef struct WpScalingParam
{
  // Explicit weighted prediction parameters parsed in slice header,
  // or Implicit weighted prediction parameters (8 bits depth values).
  u32        bPresentFlag;
  u32        uiLog2WeightDenom;
  s32        iWeight;
  s32        iOffset;
  // Weighted prediction scaling values built from above parameters (bitdepth scaled):
  s32         w, o, offset, shift, round;
} WpScalingParam;

typedef struct HevcSliceHeader
{
    s32 nPpsId;

    ///< address (in raster order) of the first block in the current slice segment
    s32   nSliceSegmentAddr;
    ///< address (in raster order) of the first block in the current slice
    s32   nSliceAddr;

    HevcSliceType eSliceType;

    s32 nPicOrderCntLsb;

    u8 bFirstSliceInPicFlag;
    u8 bDependentSliceSegmentFlag; //* 1: share the pre slice header info, 0: not share
    u8 bPicOutputFlag;
    u8 nColourPlaneId;

    ///< RPS coded in the slice header itself is stored here
    HevcShortTermRPS SliceRps;
    HevcShortTermRPS *pShortTermRps;
    HevcLongTermRPS LongTermRps;
    u32 ListEntryLx[2][32];

    u8 bRplModificationFlag[2];
    u8 bNoOutPutOfPriorPicsFlag;
    u8 bSliceTemporalMvpEnableFlag;

    s32 nNumOfRefs[2];

    u8 bSliceSampleAdaptiveOffsetFlag[3];
    u8 bMvdL1ZeroFlag;

    u8 bCabacInitFlag;
    u8 bSlicDeblockingFilterDisableFlag;
    u8 bSliceLoopFilterAcrossSliceEnableFlag;
    u8 bCollocatedFromL0Flag;
    u8 bCollocatedList;

    s32 nCollocatedRefIdx;

    s32 nSliceQpDelta;
    s32 nSliceCbQpOffset;
    s32 nSliceCrQpOffset;

    s32 nBetaOffset;    ///< beta_offset_div2 * 2
    s32 nTcOffset;      ///< tc_offset_div2 * 2

    u32 nMaxNumMergeCand; ///< 5 - 5_minus_max_num_merge_cand
    u8 bNextSlice;   /* next slice maybe exist */
    u8 bIsNotBlowDelayFlag;

    s32 *EntryPointOffset;
    s32 *Offset;
    s32 *Size;
    s32 nNumEntryPointOffsets;

    s32 nLastEntryPointOffset;
    s32 *pRegEntryPointOffset;
    size_addr pRegEntryPointOffsetPhyAddr;

    s32 nSliceQp;

    WpScalingParam  m_weightPredTable[2][HEVC_MAX_REFS][3];
    char      hevc_weight_buf[2][3][16];
    u8 nLumaLog2WeightDenom;
    u16 nChromaLog2WeightDenom;

    u16 LumaWeightL0[16];
    u16 ChromaWeightL0[16][2];
    u16 ChromaWeightL1[16][2];
    u16 LumaWeightL1[16];

    u16 LumaOffsetL0[16];
    u16 ChromaOffsetL0[16][2];

    u16 LumaOffsetL1[16];
    u16 ChromaOffsetL1[16][2];
    s32 nSliceCtbAddrRs;
}HevcSliceHeader;

typedef struct HevcStreamInfo
{
    VideoStreamDataInfo *pStreamList[HEVC_MAX_STREAM_NUM];
    /* a stream is added to list after processed */
    HevcNaluType eNaluTypeList[HEVC_MAX_STREAM_NUM];
    // s32 nNaluSizeList[HEVC_MAX_STREAM_NUM];
    char *pDataBuf[HEVC_MAX_NALU_NUM];
    /* an nalu start data ptr is added to list after deleting emulation code */
    s32 nDataSize[HEVC_MAX_NALU_NUM];
    /* the pDataBuf size after delete emulation code */
    u8  bIsLocalBuf[HEVC_MAX_NALU_NUM];
    s32 nBufIndex;   /* used decoding */
    VideoStreamDataInfo *pStreamTemp; /* the processing stream */
    s32 bUseLocalBufFlag;
    s32 nCurrentStreamDataSize;
    /* the rest data size in the processing stream */
    char *pCurrentStreamDataptr;
    /* data ptr in the processing stream */
    s32 nCurrentStreamNaluNum;
    /* Number of nalus in current sbm stream */
    s32 nCurrentStreamIdx;
    /* index of stream in current frame */
    s32 nCurrentDataBufIdx;
    /* index of pDataBuf in current frame; used in requesting stream */
//    s32 nStreamNum;
    s32 bFindKeyFrameOnly;
    /* at the beginning of the stream or after decoder reset, bFindKeyFrameOnly == 1 */
    s32 bStreamWithStartCode;
    /* 0: first 4 bytes is nalu lenght, mov mkv case;  1: first 4 byte is start code 0x000001 */
    s32 bSearchNextStartCode;
    s32 bCurrentFrameStartcodeFound;
    /* current frame's first nalu is found or not*/
    s32 bPtsCaculated;
    HevcNaluType eNaluType;
    /* current frame's nalu type */
    s64 nPts;

    SbmInterface* pSbm;
    char *pSbmBuf;
    // the start address of the sbm buffer
    char *pSbmBufEnd;
    // the end address of the sbm buffer
    size_addr  pSbmBufPhy;
    // the physical start address of the sbm buffer
    size_addr  pSbmBufEndPhy;
    // the physical end address of the sbm buffer
    s32 nSbmBufSize;
    // the total size of the sbm buffer

    /* todo: ShByte use pLocalBuf */
    char *pLocalBuf;
    // the start address of the local buffer,
    char *pLocalBufEnd;
    // the end address of the local buffer
    size_addr pLocalBufPhy;
    // the start address of the local buffer
    size_addr pLocalBufEndPhy;
    // the end address of the local buffer
    s32 nLocalBufSize;
    // the total size of the local buffer

    char *pExtraData;
    s32 nExtraDataSize;

    /* new framework start */
    char *ShByte;  /* slice header byte, use local sbm buffer */
    s16  EmulationCodeIdx[HEVC_SLICE_HEADER_BYTES];
    u8   bHasTwoDataTrunk[HEVC_MAX_NALU_NUM];
    s32  FirstDataTrunkSize[HEVC_MAX_NALU_NUM];
    s32  bHasTwoDataTrunkFlag;
    s32     nFirstDataTrunkSize;
    char *pAfterStartCodePtr;
    s16  nShByteSize;
    s16  nEmulationCodeNum;
    /* new framework end */

    // for hevc register
    u32  pCurrentBufPhy;
    u32  pCurrentBufEndPhy;
    s32     nCurrentBufSize;
    s32  nBitsOffset;
    FramePicInfo* pCurStreamFrame;
}HevcStreamInfo;

typedef struct HevcRefPicList
{
    struct HevcFrame *Ref[HEVC_MAX_REFS];
    s32 List[HEVC_MAX_REFS];  /* poc list */
    s32 bIsLongTerm[HEVC_MAX_REFS];
    s32 nNumOfRefs;
}HevcRefPicList;

typedef struct HevcMvBytePtr
{
    s16    nBlk0Mvlist0X;
    s16    nBlk0Mvlist0Y;
    s16    nBlk0Mvlist1X;
    s16    nBlk0Mvlist1Y;
    s32    nMvInfo;
    s16    nBlk1Mvlist0X;
    s16    nBlk1Mvlist0Y;
    s16    nBlk1Mvlist1X;
    s16    nBlk1Mvlist1Y;
}HevcMvBytePtr;


typedef struct HevcFrame
{
    VideoPicture *pFbmBuffer;
    VideoPicture *pSecFbmBuffer;
    s32 nPoc;
    s32 nSequence;
    s32 bFlags;
    s32 bKeyFrame;
    s32 bErrorFrameFlag;
    //* 1: frame that not call ve to decode because delay, also should not send to display.
    s32 bDropFlag;

    //used for register
    size_addr LuamPhyAddr;
    size_addr ChromaPhyAddr;
    size_addr SecLuamPhyAddr;
    size_addr SecChromaPhyAddr;
    s32 nIndex;
    char* pMVFrameCol;
    size_addr   MVFrameColPhyAddr;
    int nMvFrameBufIndex;
    s32 nLower2bitBufOffset;
    s32 nLower2bitStride;
}HevcFrame;

typedef struct HevcControlInfo //used for hardward register
{
    // hardware temp buffer
    void *pNeighbourInfoBuffer;
    size_addr    pNeighbourInfoBufferPhyAddr;

    // output control info
    s32    bScaleDownEnable;
    s32    priPixelFormatReg; // for register
    s32    secOutputEnabled;
    s32    secPixelFormatReg; // secondary Output Format,
    s32    secOutputFormatConfig;    // CedarX format

    s32    priFrmBufLumaStride;
    s32    priFrmBufChromaStride;
    s32    priFrmBufLumaSize;
    s32    priFrmBufChromaSize;

    s32    nHorizonScaleDownRatio;
    s32    nVerticalScaleDownRatio;
    s32       bRotationEnable;
    s32    nRotationDegree;
    s32    secFrmBufLumaStride;
    s32    secFrmBufChromaStride;
    s32    secFrmBufLumaSize;  //
    s32    secFrmBufChromaSize;  //

}HevcControlInfo;

typedef struct HevcDebug
{
    s64 nStartTime;
    s64 nCurrTime;
    s64 nCurrTimeHw;
    s32 nHwFrameNum;
    s32 nHwFrameTotal;
    float fFrameCostTimeList[HEVE_VE_COUNTER_LIST];
    s64 nCurrFrameCostTime;

    s64 nFrameDualation;
    s64 nHwTotalTime;
    s64 nSliceDualationHw;
    s64 nTotalTime;
    s64 nTotalTimeHw;
    s32 nFrameNum;

    u64 nCurrentBit;
    u64 nTotalBit;
    s64 nBitRateTotalTime;
    u64 nCurrentBitRate;
    s64 nBitRateTime;
    s32 nBitFrameNum;

    s32 nMaxUsingDpbNumber;
    s32 nMaxRefPicNumber;
    s32 nDecodeFrameNum;
    s32 nDecoderDropFrameNum;

    u32 nVeCounter;
    s32 nCtuNum;
    u32 nVeCntList[HEVE_VE_COUNTER_LIST];
    u32 nFrameBits[HEVE_VE_COUNTER_LIST];
    u32 *pAllTimeHw;
    u32 nAllTimeNum;
}HevcDebug;

typedef struct HEVCEXTRASCALEINFO
{
    s32 nWidthTh;
    s32 nHeightTh;
    s32 nHorizonScaleRatio;
    s32 nVerticalScaleRatio;
}HevcExtraScaleInf;

enum H265PERFORMINFO
{
    H265_PERFORM_NONE         = 0,
    H265_PERFORM_CALDROPFRAME = 1,
};
typedef struct HEVC_PERFORM_INF
{
    u32 performCtrlFlag;
    VDecodePerformaceInfo H265PerfInfo;
}HevcPerformInf;

typedef struct H265_MOTION_VECTOR_BUF_MANAGER
{
    char* pMVFrameCol;
    size_addr   MVFrameColPhyAddr;
    int nMvFrameBufIndex;
}HevcMvBufManager;

typedef struct H265_MOTION_VECTOR_BUF_INF
{
    HevcMvBufManager* pMVFrameColNext;
    HevcMvBufManager* pMVFrameColManager;
    void*         pParam;
}HevcMvBufInf;

typedef struct HevcContex
{
    HevcDecoderStep eDecodStep;
    VConfig *pConfig;
    VideoStreamInfo *pVideoStreamInfo;
    s32     nPoc;
    s32        nPocTid0;
    s32     bFbmInitFlag;
    HevcStreamInfo *pStreamInfo;
    GetBitContext getBit;
    HevcNaluType eNaluType;
    HevcNaluType eFirstNaluType;
    s32 nTemporalId;
    s8 sliceType;

    HevcVPS *pVps;
    HevcSPS *pSps;
    HevcPPS *pPps;
    HevcVPS *VpsList[HEVC_MAX_VPS_NUM];
    HevcSPS *SpsList[HEVC_MAX_SPS_NUM];
    HevcPPS *PpsList[HEVC_MAX_PPS_NUM];
    HevcSeiDisInfo SeiDisInfo;

    HevcSliceHeader SliceHeader;
    HevcControlInfo ControlInfo;
    s32    nSliceIdx;
    s32 bSliceInitialized;
    s32 bIsFirstPicture;  /* display the first picture immediatelys */

    u8 md5[3][16];
    u8 md5Caculate[3][16];
    s32 nMd5Frame;
    s32 bMd5AvailableFlag;

    s32 nSeqDecode;
    s32 nSeqOutput;
    s32 nMaxRa;
    s32 nNuhLayerId;
    HevcFrame DPB[HEVC_MAX_DPB_SIZE];
    VideoPicture *pCurrFrame;
    VideoPicture *pCurrSecFrame;
    HevcFrame *pCurrDPB;
    s32 nStreamFrameNum;
    s32 bFrameDurationZeroFlag;  /* for pts caculating */
    int64_t nLastStreamPts;
    int64_t nLastPts;   /* for pts caculating */
    int64_t nPts;
    int64_t nCurrentFramePts;
    int64_t PtsList[HEVC_PTS_LIST_SIZE];
    Fbm* fbm;
    Fbm* SecondFbm;
    HevcRefPicList Rps[5];
    HevcRefPicList RefPicList[2]; /* for current slice */
    size_addr HevcDecRegBaseAddr;
    size_addr HevcTopRegBaseAddr;
    u32 bHardwareGetBitsFlag;
    u32 bDropFrameAdaptiveEnable;
    s32 eDisplayPixelFormat;
    u32 nVpsMaxDecPicBuffering;
    u32 nVpsNumReorderPics;

    HevcDebug debug;
    VideoFbmInfo*  pFbmInfo;
    HevcExtraScaleInf H265ExtraScaleInfo;
    HevcPerformInf H265PerformInf;
    int nLastDispPoc;
    s32 bHadFindSpsFlag;
    s32 bHadFindPpsFlag;
    s32 bCurPicWithoutRefInfo;
    VIDEO_FRM_MV_INFO *pMvInfo;
    s32 bErrorFrameFlag;

 #if HEVC_ENABLE_CATCH_DDR
    u32 nCurFrameHWTime;
    u32 nCurFrameBW;
    u32 nLastFrameBW;
    u32 nCurFrameRef0;
    u32 nCurFrameRef1;
    u32 nPara[216000][5];
 #endif
    //* 1: send error-frame to caller and set videoPicture.bFrameErrorFlag;
    //*0: drop error-frame, reset inter decoder when error-frame is ref
    //*    to make sure all error-frame were droped.
    s32 bDisplayErrorFrameFlag;
    s32 nSqueueFramePocList [HEVC_MAX_FRAMES_ONE_SQUEUE];
    s32 b10BitStreamFlag;
    s32 bHadRemoveFile;
    s32 nDecCountHevc;
    u64 nIcVersion;
    u64 nDecIpVersion;
    u64 nCurFrameStreamSize;
    s32 bEnableAfbcFlag;
    s32 bEnableIptvFlag;
    s32 bDropPreFrameFlag;
    s32 bPreHdrFlag;

    u32 nDecPicNum;

    int  nHevcMvBufNum;
    HevcMvBufInf* pHevcMvBufEmptyQueue;
    HevcMvBufInf* pHevcMvBufInf;
}HevcContex;

#define HEVCMAX(a,b) ((a) > (b) ? (a) : (b))
#define HEVCMAX3(a,b,c) HEVCMAX(HEVCMAX(a,b),c)
#define HEVCMIN(a,b) ((a) > (b) ? (b) : (a))
#define HEVCMIN3(a,b,c) HEVCMIN(HEVCMIN(a,b),c)

#endif /* H265_VAR_H */
