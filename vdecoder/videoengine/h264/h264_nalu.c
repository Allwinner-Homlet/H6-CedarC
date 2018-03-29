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
 * File : h264_nalu.c
 * Description : h264_nalu
 * History :
 *
 */

/*
*
* File : h264_nalu.c
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


static const u8 default_scaling4[2][16]=
{
    {
        6,13,20,28,
        13,20,28,32,
        20,28,32,37,
        28,32,37,42
   },
      {
        10,14,20,24,
        14,20,24,27,
        20,24,27,30,
        24,27,30,34
    }
    };

static const u8 default_scaling8[2][64]=
{
    {
        6,10,13,16,18,23,25,27,
        10,11,16,18,23,25,27,29,
        13,16,18,23,25,27,29,31,
        16,18,23,25,27,29,31,33,
        18,23,25,27,29,31,33,36,
        23,25,27,29,31,33,36,38,
        25,27,29,31,33,36,38,40,
        27,29,31,33,36,38,40,42
    },
    {
        9,13,15,17,19,21,22,24,
        13,13,17,19,21,22,24,25,
        15,17,19,21,22,24,25,27,
        17,19,21,22,24,25,27,28,
        19,21,22,24,25,27,28,30,
        21,22,24,25,27,28,30,32,
        22,24,25,27,28,30,32,33,
        24,25,27,28,30,32,33,35
    }
    };

static const u8 zigzag_scan[16]=
{
    0+0*4, 1+0*4, 0+1*4, 0+2*4,
    1+1*4, 2+0*4, 3+0*4, 2+1*4,
    1+2*4, 0+3*4, 1+3*4, 2+2*4,
    3+1*4, 3+2*4, 2+3*4, 3+3*4,
};

static const u8 zigzag_scan8x8[64]=
{
    0+0*8, 1+0*8, 0+1*8, 0+2*8,
    1+1*8, 2+0*8, 3+0*8, 2+1*8,
    1+2*8, 0+3*8, 0+4*8, 1+3*8,
    2+2*8, 3+1*8, 4+0*8, 5+0*8,
    4+1*8, 3+2*8, 2+3*8, 1+4*8,
    0+5*8, 0+6*8, 1+5*8, 2+4*8,
    3+3*8, 4+2*8, 5+1*8, 6+0*8,
    7+0*8, 6+1*8, 5+2*8, 4+3*8,
    3+4*8, 2+5*8, 1+6*8, 0+7*8,
    1+7*8, 2+6*8, 3+5*8, 4+4*8,
    5+3*8, 6+2*8, 7+1*8, 7+2*8,
    6+3*8, 5+4*8, 4+5*8, 3+6*8,
    2+7*8, 3+7*8, 4+6*8, 5+5*8,
    6+4*8, 7+3*8, 7+4*8, 6+5*8,
    5+6*8, 4+7*8, 5+7*8, 6+6*8,
    7+5*8, 7+6*8, 6+7*8, 7+7*8,
};

//**************************************************************************************//
//**************************************************************************************//
/**
 * Returns and optionally allocates SPS / PPS structures in the supplied array 'vec'
 */
static void *H264AllocParameterSet(void **vec, const u32 id, const u32 maxCount, const u32 size)

{
    if(id >= maxCount)
    {
        return NULL;
    }

    if(vec[id] == NULL)
    {
        vec[id] = malloc(size);
        memset(vec[id], 0, size);
    }
    return vec[id];
}

static void H264DecodeScalingList(H264DecCtx* h264DecCtx, u8 *factors, s32 size,
                                const u8 *jvt_list, const u8 *fallback_list)
{
    s32 i = 0;
    s32 last = 8;
    s32 next = 8;

    const u8 *scan = (size==16)? zigzag_scan : zigzag_scan8x8;
    /* matrix not written, we use the predicted one */
    if(!h264DecCtx->GetBits((void*)h264DecCtx, 1))
    {
        memcpy((void*)factors, (void*)fallback_list, (u32)(size*sizeof(u8)));
    }
    else
    {
        for(i=0;i<size;i++)
        {
            if(next)
            {
                next = (last + h264DecCtx->GetSeGolomb((void*)h264DecCtx)) & 0xff;
            }
            if(!i && !next)
            { /* matrix not written, we use the preset one */
                memcpy((void*)factors, (void*)jvt_list, (u32)(size*sizeof(u8)));
                break;
            }
            last = factors[scan[i]] = next ? next : last;
        }
    }
}

static void H264DecodeScalingMatrices(H264DecCtx* h264DecCtx,
                                    H264SpsInfo *pSps,
                                    H264PpsInfo *pPps,
                                    int isSps,
                                    u8 (*scaling_matrix4)[16],
                                    u8 (*scaling_matrix8)[64])
{
    s32 fallbackSps = !isSps && pSps->bScalingMatrixPresent;
    if(h264DecCtx->GetBits((void*)h264DecCtx, 1))
    {
       const u8 *fallback[4] = {
                            fallbackSps? pSps->nScalingMatrix4[0] : default_scaling4[0],
                            fallbackSps? pSps->nScalingMatrix4[3] : default_scaling4[1],
                            fallbackSps? pSps->nScalingMatrix8[0] : default_scaling8[0],
                            fallbackSps? pSps->nScalingMatrix8[1] : default_scaling8[1]
                            };
        if(pPps!= NULL)
        {
            pPps->bScalingMatrixPresent = 1;
        }
        pSps->bScalingMatrixPresent |= isSps;
        H264DecodeScalingList(h264DecCtx,scaling_matrix4[0],16,
            default_scaling4[0],fallback[0]); // Intra, Y
        H264DecodeScalingList(h264DecCtx,scaling_matrix4[1],16,
            default_scaling4[0],scaling_matrix4[0]); // Intra, Cr
        H264DecodeScalingList(h264DecCtx,scaling_matrix4[2],16,
            default_scaling4[0],scaling_matrix4[1]); // Intra, Cb
        H264DecodeScalingList(h264DecCtx,scaling_matrix4[3],16,
            default_scaling4[1],fallback[1]); // Inter, Y
        H264DecodeScalingList(h264DecCtx,scaling_matrix4[4],16,
            default_scaling4[1],scaling_matrix4[3]); // Inter, Cr
        H264DecodeScalingList(h264DecCtx,scaling_matrix4[5],16,
            default_scaling4[1],scaling_matrix4[4]); // Inter, Cb

        if(isSps || pPps->bTransform8x8Mode)
        {
            H264DecodeScalingList(h264DecCtx,scaling_matrix8[0],64,
                default_scaling8[0],fallback[2]);  // Intra, Y
            H264DecodeScalingList(h264DecCtx,scaling_matrix8[1],64,
                default_scaling8[1],fallback[3]);  // Inter, Y
        }
    }
    else if(fallbackSps)
    {
        memcpy((void*)scaling_matrix4, (void*)pSps->nScalingMatrix4, 6*16*sizeof(u8));
        memcpy((void*)scaling_matrix8, (void*)pSps->nScalingMatrix8, 2*64*sizeof(u8));
    }
}
//*************************************************************************//
//*************************************************************************//

static inline void H264DecodeHrdParameters(H264DecCtx* h264DecCtx, H264SpsInfo* pSpsInf)
{
    s32 cpbCount = 0;
    s32 i = 0;

    cpbCount = h264DecCtx->GetUeGolomb((void*)h264DecCtx) + 1;
    h264DecCtx->GetBits((void*)h264DecCtx, 4);
    /* bit_rate_scale */
    h264DecCtx->GetBits((void*)h264DecCtx, 4);
    /* cpb_size_scale */

    for(i=0; i<cpbCount; i++)
    {
        h264DecCtx->GetUeGolomb((void*)h264DecCtx);
        /* bit_rate_value_minus1 */
        h264DecCtx->GetUeGolomb((void*)h264DecCtx);
        /* cpb_size_value_minus1 */
        h264DecCtx->GetBits((void*)h264DecCtx, 1);
        /* cbr_flag */
    }
    h264DecCtx->GetBits((void*)h264DecCtx, 5);
    /* initial_cpb_removal_delay_length_minus1 */
    pSpsInf->nCpbRemovalDelayLength = h264DecCtx->GetBits((void*)h264DecCtx, 5)+1;
    /* cpb_removal_delay_length_minus1 */
    pSpsInf->nDpbOutputDelayLength  = h264DecCtx->GetBits((void*)h264DecCtx, 5)+1;
    /* dpb_output_delay_length_minus1 */
    pSpsInf->nTimeOffsetLength = h264DecCtx->GetBits((void*)h264DecCtx, 5);
    /* time_offset_length */
}
s32 H264DecVuiParameters(H264DecCtx* h264DecCtx, H264Context* hCtx, H264SpsInfo* pSpsInf)
{
    u32 vuiSarWidth = 0;
    u32 vuiSarHeight = 0;
    u32 timeScale = 0;
    u32 numUnitsInTick = 0;
    u8 vuiAspectRatioIdc =0;
    u32 nNumReorderFrames = 0;
    #if 0
    u32 aspect_ratio_table[14] = {1000, 1000, 1091, 909,
                                  1455, 1212, 2182, 1818,
                                  2909, 2424, 1636, 1336,
                                  1939, 1616};
    #endif

    u8 bVideoFullRangeFlag = 0;
    u8 nMatrixCoeff = 0;
    H264Dec* h264Dec = NULL;
    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;

    if(h264DecCtx->GetBits((void*)h264DecCtx, 1) == 1)   // vui aspect ration info present flag
    {
        vuiAspectRatioIdc = h264DecCtx->GetBits((void*)h264DecCtx, 8);

        if(vuiAspectRatioIdc == 255)
        {
            vuiSarWidth = h264DecCtx->GetBits((void*)h264DecCtx, 16);
            vuiSarHeight = h264DecCtx->GetBits((void*)h264DecCtx, 16);
            if(vuiSarHeight != 0)
            {
                pSpsInf->nAspectRatio = vuiSarWidth*1000/vuiSarHeight;
            }
        }
    }

    if(h264DecCtx->GetBits((void*)h264DecCtx, 1))   // overscan info present flag
    {
        h264DecCtx->GetBits((void*)h264DecCtx, 1);   // overscan appropriate flag
    }

    if(h264DecCtx->GetBits((void*)h264DecCtx, 1))    // video signal type present flag
    {
        h264DecCtx->GetBits((void*)h264DecCtx, 3);   // video format
        bVideoFullRangeFlag = h264DecCtx->GetBits((void*)h264DecCtx, 1);   // video full range flag
        hCtx->nColorPrimary = bVideoFullRangeFlag;
        hCtx->nColorPrimary <<= 8;
        if(h264DecCtx->GetBits((void*)h264DecCtx, 1))      /* colour_description_present_flag */
        {
            h264DecCtx->GetBits((void*)h264DecCtx, 8); /* colour_primaries */
            h264DecCtx->GetBits((void*)h264DecCtx, 8); /* transfer_characteristics */
            nMatrixCoeff = h264DecCtx->GetBits((void*)h264DecCtx, 8); /* matrix_coefficients */
            hCtx->nColorPrimary |= nMatrixCoeff;
        }
    }

    if(h264DecCtx->GetBits((void*)h264DecCtx, 1))   /* chroma_location_info_present_flag */
    {
        h264DecCtx->GetUeGolomb((void*)h264DecCtx);  /* chroma_sample_location_type_top_field */
        h264DecCtx->GetUeGolomb((void*)h264DecCtx);  /* chroma_sample_location_type_bottom_field */
    }

    if(h264DecCtx->GetBits((void*)h264DecCtx, 1))    // time info present flag
    {
        numUnitsInTick = h264DecCtx->GetBits((void*)h264DecCtx, 32);  //num_units_in_tick
        //numUnitsInTick = H264GetBitsLong(h264DecCtx, 32);

        timeScale = h264DecCtx->GetBits((void*)h264DecCtx, 32);  //time_scale
        if(hCtx->vbvInfo.nFrameRate == 0)
        {
            hCtx->vbvInfo.nFrameRate = timeScale/(numUnitsInTick*2);
            logd("nFrameRate=%u", hCtx->vbvInfo.nFrameRate);
            if(hCtx->vbvInfo.nFrameRate != 0)
            {
                hCtx->vbvInfo.nPicDuration = (1000*1000);
                hCtx->vbvInfo.nPicDuration /= hCtx->vbvInfo.nFrameRate;
                h264Dec->H264PerformInf.H264PerfInfo.nFrameDuration = hCtx->vbvInfo.nPicDuration;
            }
        }
        h264DecCtx->GetBits((void*)h264DecCtx, 1);   //>fixed_frame_rate_flag
    }

    pSpsInf->bNalHrdParametersPresentFlag = h264DecCtx->GetBits((void*)h264DecCtx, 1);
    if(pSpsInf->bNalHrdParametersPresentFlag)
    {
        H264DecodeHrdParameters(h264DecCtx, pSpsInf);
    }
    pSpsInf->bVclHrdParametersPresentFlag = h264DecCtx->GetBits((void*)h264DecCtx, 1);
    if(pSpsInf->bVclHrdParametersPresentFlag)
    {
        H264DecodeHrdParameters(h264DecCtx, pSpsInf);
    }
    if(pSpsInf->bNalHrdParametersPresentFlag || pSpsInf->bVclHrdParametersPresentFlag)
    {
        pSpsInf->bLowDelayFlag = h264DecCtx->GetBits((void*)h264DecCtx, 1);
        /* low_delay_hrd_flag */
    }

    pSpsInf->bPicStructPresentFlag = h264DecCtx->GetBits((void*)h264DecCtx, 1);
    /* pic_struct_present_flag */

    pSpsInf->bBitstreamRestrictionFlag = h264DecCtx->GetBits((void*)h264DecCtx, 1);
    if(pSpsInf->bBitstreamRestrictionFlag)
    {
        h264DecCtx->GetBits((void*)h264DecCtx, 1);  /* motion_vectors_over_pic_boundaries_flag */
        h264DecCtx->GetUeGolomb((void*)h264DecCtx); /* max_bytes_per_pic_denom */
        h264DecCtx->GetUeGolomb((void*)h264DecCtx); /* max_bits_per_mb_denom */
        h264DecCtx->GetUeGolomb((void*)h264DecCtx); /* log2_max_mv_length_horizontal */
        h264DecCtx->GetUeGolomb((void*)h264DecCtx); /* log2_max_mv_length_vertical */

        nNumReorderFrames = h264DecCtx->GetUeGolomb((void*)h264DecCtx);
        h264DecCtx->GetUeGolomb((void*)h264DecCtx); /*max_dec_frame_buffering*/

        if(nNumReorderFrames > 16)
        {
            return VRESULT_ERR_FAIL;
        }
        pSpsInf->nNumReorderFrames = nNumReorderFrames;
    }
    return 0;
}

//************************************************************************//
//************************************************************************//

s32 H264DecodeSps(H264DecCtx* h264DecCtx, H264Context* hCtx)
{
    u32 i = 0;
    //u32 j = 0;
    u8 crop = 0;
    u8 nProfileIdc = 0;
    u8 vuiParamPresenFlag = 0;
    u8 frmMbsOnlyFlag = 0;
    //u16 levelIdc = 0;
    u32 nSpsId = 0;
    u32 tmp = 0;
    u32 nMbWidth = 0;
    u32 nMbHeight = 0;
    u32 nCodedFrameRatio = 0;
    H264SpsInfo* pSpsInf = NULL;
    H264Dec* h264Dec = NULL;
    //H264SpsInfo* pSpsBuf = NULL;
    H264SpsInfo* pNewSpsInf = NULL;
    u32 nMaxRcdNum = 0;
    u32 nMinRcdNum = 0xffffffff;
    u8 bConstraintSetFlags = 0;

    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;
    //add for big_array
    H264SpsInfo* pStaticSpsInf = (H264SpsInfo*)(h264Dec->dataBuffer);

    /**********************
     *profileIdc  = 44: CAVLC 4:4:4 INTRA
     *profileIdc  = 66; baseLine;
     *profileIdc  = 77; main;
     *profileIdc  = 88; extented;
     *profileIdc  = 100; high
     *profileIdc  = 110; high 10 or high 10 intra
     *profileIdc  = 122; high 4:2:2 predictive or high 4:2:2 intra
     *profileIdc  = 244; high 4:4:4 or hight 4:4:4 intra
     *profileIdc  = 118;  multiview high
     *profileIdc  = 128; stereo high
     **********************/

    nProfileIdc = h264DecCtx->GetBits((void*)h264DecCtx, 8);
    //levelIdc = h264DecCtx->GetBits((void*)h264DecCtx, 16);
    //bConstraintSetFlags = (levelIdc>>10)&0x3f;
    bConstraintSetFlags |= h264DecCtx->GetBits((void*)h264DecCtx,1) << 0;   // constraint_set0_flag
    bConstraintSetFlags |= h264DecCtx->GetBits((void*)h264DecCtx,1) << 1;   // constraint_set1_flag
    bConstraintSetFlags |= h264DecCtx->GetBits((void*)h264DecCtx,1) << 2;   // constraint_set2_flag
    bConstraintSetFlags |= h264DecCtx->GetBits((void*)h264DecCtx,1) << 3;   // constraint_set3_flag
    bConstraintSetFlags |= h264DecCtx->GetBits((void*)h264DecCtx,1) << 4;   // constraint_set4_flag
    bConstraintSetFlags |= h264DecCtx->GetBits((void*)h264DecCtx,1) << 5;   // constraint_set5_flag
    h264DecCtx->GetBits((void*)h264DecCtx,2); // reserved
    //levelIdc = h264DecCtx->GetBits((void*)h264DecCtx,8);
    h264DecCtx->GetBits((void*)h264DecCtx,8);
    nSpsId = h264DecCtx->GetUeGolomb((void*)h264DecCtx);

    pSpsInf = pStaticSpsInf;
    memset(pSpsInf, 0, sizeof(H264SpsInfo));

    if(nSpsId >= 32)
    {
        return VRESULT_ERR_FAIL;
    }

    pSpsInf->nTimeOffsetLength = 24;
    pSpsInf->bConstraintSetFlags = bConstraintSetFlags;
    pSpsInf->nProfileIdc = nProfileIdc;
    pSpsInf->bScalingMatrixPresent = 0;

    if(nProfileIdc >= 100) //high profile
    {
        tmp = h264DecCtx->GetUeGolomb((void*)h264DecCtx);
        if(tmp != 1)  //tmp:0:4:0:0 1:4:2:0 2:4:2:2,3:4:4:4
        {
            return VDECODE_RESULT_UNSUPPORTED;
        }
        if(tmp == 3)      // chroma format idc
        {
            h264DecCtx->GetBits((void*)h264DecCtx, 1);   //residual color transform flag
        }
        h264DecCtx->GetUeGolomb((void*)h264DecCtx);      // bit depth luma minus8
        h264DecCtx->GetUeGolomb((void*)h264DecCtx);      // bit depth chroma minus8
        h264DecCtx->GetBits((void*)h264DecCtx, 1);        // transform bypass
        H264DecodeScalingMatrices(h264DecCtx, pSpsInf,
                                  NULL, 1,
                                  pSpsInf->nScalingMatrix4,
                                  pSpsInf->nScalingMatrix8);
        hCtx->bScalingMatrixPresent = 1;
    }

    pSpsInf->nLog2MaxFrmNum = h264DecCtx->GetUeGolomb((void*)h264DecCtx) + 4;
    pSpsInf->nPocType = h264DecCtx->GetUeGolomb((void*)h264DecCtx);

    if(pSpsInf->nPocType == 0)
    {
        pSpsInf->nLog2MaxPocLsb = h264DecCtx->GetUeGolomb((void*)h264DecCtx)+4;
    }
    else if(pSpsInf->nPocType == 1)
    {
        pSpsInf->bDeltaPicOrderAlwaysZeroFlag = h264DecCtx->GetBits((void*)h264DecCtx,1);
        pSpsInf->nOffsetForNonRefPic = h264DecCtx->GetSeGolomb((void*)h264DecCtx);
        pSpsInf->nOffsetForTopToBottomField = h264DecCtx->GetSeGolomb((void*)h264DecCtx);
        pSpsInf->nPocCycleLength = h264DecCtx->GetUeGolomb((void*)h264DecCtx);
        //logd("***here37: pSpsInf->nPocCycleLengt=%d\n", pSpsInf->nPocCycleLength);
        if(pSpsInf->nPocCycleLength > 256)
        {
            loge("**error the nPocCycleLength is %d \n", pSpsInf->nPocCycleLength);
            return VRESULT_ERR_FAIL;
        }
        for(i=0; i<pSpsInf->nPocCycleLength;i++)
        {
            pSpsInf->nOffsetForRefFrm[i] = h264DecCtx->GetSeGolomb((void*)h264DecCtx);
        }
    }
    else if(pSpsInf->nPocType != 2)
    {
        return VRESULT_ERR_FAIL;
    }
    pSpsInf->nRefFrmCount = h264DecCtx->GetUeGolomb((void*)h264DecCtx);

    //h264DecCtx->GetBits((void*)h264DecCtx, 1);
    pSpsInf->gaps_in_frame_num_value_allowed_flag = h264DecCtx->GetBits((void*)h264DecCtx, 1);
    nMbWidth = h264DecCtx->GetUeGolomb((void*)h264DecCtx)+1;
    nMbHeight = h264DecCtx->GetUeGolomb((void*)h264DecCtx)+1;

    if(nMbWidth > 5 && nMbHeight > 4) //add for resolution change to little
    {
        if(hCtx->nMbWidth!=0 && hCtx->nMbHeight!=0)
        {
            if(nMbWidth != hCtx->nMbWidth  || nMbHeight != hCtx->nMbHeight)
            {
                if(hCtx->vbvInfo.vbv->nType != SBM_TYPE_FRAME_AVC)
                {
                    if(hCtx->vbvInfo.pVbvStreamData != NULL)
                    {
                         SbmReturnStream(hCtx->vbvInfo.vbv, hCtx->vbvInfo.pVbvStreamData);
                         hCtx->vbvInfo.pVbvStreamData = NULL;
                    }
                }
                else
                {
                     if(hCtx->pCurStreamFrame != NULL)
                     {
                         SbmReturnStream(hCtx->vbvInfo.vbv,
                            (VideoStreamDataInfo *)hCtx->pCurStreamFrame);
                         hCtx->pCurStreamFrame = NULL;
                     }
                }
                if(h264Dec->pHContext != NULL)
                {
                    H264FlushDelayedPictures(h264DecCtx, h264Dec->pHContext);
                }
                if(h264Dec->pMinorHContext != NULL)
                {
                    H264FlushDelayedPictures(h264DecCtx, h264Dec->pMinorHContext);
                }
                if(hCtx->bIsAvc == 0)
            {
                return VDECODE_RESULT_RESOLUTION_CHANGE;
            }
            else
            {
                return VDECODE_RESULT_UNSUPPORTED;
            }
        }
    }
}

    frmMbsOnlyFlag = h264DecCtx->GetBits((void*)h264DecCtx, 1);
    pSpsInf->bFrameMbsOnlyFlag = frmMbsOnlyFlag;

    pSpsInf->nMbWidth = nMbWidth;
    pSpsInf->nMbHeight = nMbHeight;

    pSpsInf->nFrmMbWidth = nMbWidth*16;
    pSpsInf->nFrmMbHeight = nMbHeight*(2-frmMbsOnlyFlag)*16;

    hCtx->nMbWidth = nMbWidth;
    hCtx->nMbHeight = nMbHeight;
    hCtx->nFrmMbWidth = pSpsInf->nFrmMbWidth;
    hCtx->nFrmMbHeight = pSpsInf->nFrmMbHeight;
    hCtx->bFrameMbsOnlyFlag = pSpsInf->bFrameMbsOnlyFlag;
    hCtx->nRefFrmCount = pSpsInf->nRefFrmCount;

    pSpsInf->bMbAff = 0;
    if(frmMbsOnlyFlag == 0)
    {
        pSpsInf->bMbAff = h264DecCtx->GetBits((void*)h264DecCtx, 1);
    }

    if(hCtx->bProgressice == 0xFF)
    {
        hCtx->bProgressice = (pSpsInf->bMbAff==1)? 0: 0xFF;
    }
    pSpsInf->bDirect8x8InferenceFlag = h264DecCtx->GetBits((void*)h264DecCtx, 1);

    pSpsInf->nCropLeft  = 0;
    pSpsInf->nCropRight = 0;
    pSpsInf->nCropTop = 0;
    pSpsInf->nCropBottom = 0;

    crop = h264DecCtx->GetBits((void*)h264DecCtx,1);
    if(crop == 1)
    {
        h264DecCtx->GetUeGolomb((void*)h264DecCtx);
        pSpsInf->nCropRight = h264DecCtx->GetUeGolomb((void*)h264DecCtx);
        h264DecCtx->GetUeGolomb((void*)h264DecCtx);
        pSpsInf->nCropBottom = h264DecCtx->GetUeGolomb((void*)h264DecCtx);
        pSpsInf->nCropRight *= 2;
        pSpsInf->nCropBottom  *= 2*(2-frmMbsOnlyFlag);
    }
    hCtx->nCropLeft = pSpsInf->nCropLeft;
    hCtx->nCropRight = pSpsInf->nCropRight;
    hCtx->nCropTop = pSpsInf->nCropTop;
    hCtx->nCropBottom = pSpsInf->nCropBottom;

    pSpsInf->nFrmRealWidth = pSpsInf->nFrmMbWidth - pSpsInf->nCropLeft - pSpsInf->nCropRight;
    pSpsInf->nFrmRealHeight = pSpsInf->nFrmMbHeight - pSpsInf->nCropTop - pSpsInf->nCropBottom;
    nCodedFrameRatio = pSpsInf->nFrmRealWidth*1000/pSpsInf->nFrmRealHeight;
     hCtx->nFrmRealWidth = pSpsInf->nFrmRealWidth;
     hCtx->nFrmRealHeight = pSpsInf->nFrmRealHeight;

    if(nCodedFrameRatio <= 1460)
    {
        pSpsInf->bCodedFrameRatio = H264_CODED_FRAME_RATIO_4_3;
    }
    else  if(nCodedFrameRatio <= 1660)
    {
        pSpsInf->bCodedFrameRatio = H264_CODED_FRAME_RATIO_14_9;
    }
    else if(nCodedFrameRatio <= 1900)
    {
        pSpsInf->bCodedFrameRatio = H264_CODED_FRAME_RATIO_16_9;
    }
    else
    {
        pSpsInf->bCodedFrameRatio = H264_CODED_FRAME_RATIO_OTHER;
    }
    pSpsInf->nAspectRatio = 1000;
    vuiParamPresenFlag = h264DecCtx->GetBits((void*)h264DecCtx, 1); // vui parameters present flag

    if(vuiParamPresenFlag == 1)
    {
        H264DecVuiParameters(h264DecCtx,hCtx, pSpsInf);
    }
#if 0
    memcpy(&hCtx->pSps, pSpsInf, sizeof(H264SpsInfo));
    hCtx->nSpsId = nSpsId;
#endif

    if(hCtx->pSpsBuffers[nSpsId]==NULL)
    {
        if(hCtx->nSpsBufferNum < 32)
        {
            hCtx->nSpsBufferIndex[hCtx->nSpsBufferNum++] = nSpsId;
        }
    }
    pNewSpsInf = (H264SpsInfo*)H264AllocParameterSet((void**)hCtx->pSpsBuffers,
                                                      nSpsId, MAX_SPS_COUNT,
                                                      sizeof(H264SpsInfo));
    if(pNewSpsInf == NULL)
    {
        hCtx->nSpsBufferNum -= 1;
        return VDECODE_RESULT_UNSUPPORTED;
    }
    hCtx->nNewSpsId = nSpsId;
    memcpy(pNewSpsInf, pSpsInf,  sizeof(H264SpsInfo));

    return VDECODE_RESULT_OK;
}

//*************************************************************************//
//**************************************************************************//

static s32 H264DecodeRbspTraiLing(u8 *src)
{
    s32 v = 0;
    s32 r = 0;

    v = *src;
    for(r=1; r<9; r++)
    {
        if(v&1)
        {
            return r;
        }
        v>>=1;
    }
    return 8;
}

s32 H264GetVLCSymbol(u8* pBuf,s32 nBitOffset, s32 nBytecount)
{
    s32 inf;
    long byteoffset=0;       // byte from start of buffer
    s32  bitoffset=0;       // bit from start of byte
    s32  bitcounter=0;
    s32  len = 0;
    u8*  curByte = NULL;
    s32  ctrBit = 0;

    byteoffset = (nBitOffset>>3);       // byte from start of buffer
    bitoffset  = (7-(nBitOffset&0x07)); // bit from start of byte
    bitcounter = 1;

    curByte = pBuf+byteoffset;
    ctrBit    = ((*curByte)>>(bitoffset)) & 0x01;
    // control bit for current bit posision

    while(ctrBit == 0)
    {                 // find leading 1 bit
        len++;
        bitcounter++;
        bitoffset--;
        bitoffset &= 0x07;
        curByte  += (bitoffset == 7);
        byteoffset+= (bitoffset == 7);
        ctrBit    = ((*curByte) >> (bitoffset)) & 0x01;
    }

    if(byteoffset + ((len + 7) >> 3) > nBytecount)
    {
        return -1;
    }

    // make infoword
    inf = 0;
    // shortest possible code is 1, then info is always 0

    while (len--)
    {
        bitoffset --;
        bitoffset &= 0x07;
        curByte  += (bitoffset == 7);
        bitcounter++;
        inf <<= 1;
        inf |= ((*curByte) >> (bitoffset)) & 0x01;
    }
    CEDARC_UNUSE(inf);
    return bitcounter;
    // return absolute offset in bit from start of frame
}

s32 H264JudgeMoreRbspData(H264Context* hCtx, u32 nSpsId)
{
    H264SpsInfo *pSpsInf = NULL;
    u8 flag1 = 0;
    u8 flag2 = 0;
    pSpsInf = hCtx->pSpsBuffers[nSpsId];

    logv("pSpsInf->nProfileIdc=%d, pSpsInf->bConstraintSetFlags=%d\n",\
        pSpsInf->nProfileIdc, pSpsInf->bConstraintSetFlags);
    flag1 = (pSpsInf->nProfileIdc==66) || (pSpsInf->nProfileIdc==77)
            || (pSpsInf->nProfileIdc == 88);
    flag2 = pSpsInf->bConstraintSetFlags&7;

    logv("********flag1=%d, flag2=%d\n", flag1, flag2);
    if(flag1 && flag2)
    {
        return 0;
    }
    return 1;
}
s32 H264CheckMoreRbspData(u8* buffer,s32 totbitoffset,s32 nTrialBits, s32 bytecount)
{
    s32  bitoffset   = (7 - (totbitoffset & 0x07));      // bit from start of byte
    s32  byteoffset = (totbitoffset >> 3);      // byte from start of buffer
    u8   curByte  = (buffer[byteoffset]);
    s32  nCtrBit     = 0;      // control bit for current bit posision
    s32  cnt         = 0;

    bytecount -= (nTrialBits/8);

    if(byteoffset>=bytecount)
    {
        return 0;
    }

    // there is more until we're in the last byte
    if(byteoffset<(bytecount - 1))
    {
        return 1;
    }

  // read one bit
    nCtrBit = ((curByte)>>(bitoffset--)) & 0x01;

  // a stop bit has to be one
    if(nCtrBit==0)
    {
      return 1;
    }

    while(bitoffset>=0 && !cnt)
    {
        cnt |= ((curByte)>> (bitoffset--)) & 0x01;   // set up control bit
    }
    logv("cnt=%d\n", cnt);
    return (cnt);
}

s32 H264DecodePps(H264DecCtx* h264DecCtx, H264Context* hCtx, s32 sliceDataLen)
{
    u32 nPpsId = 0;
    u32 nSpsId = 0;
    u32 tmp = 0;
    //u32 bitOffset = 0;
    H264PpsInfo* pPpsInf = NULL;
    H264Dec* h264Dec =  NULL;
    //s32 usedBitsLen = 0;
    s32 nByteCount = 0;
    u8* pBuf = NULL;
    u8* pBuffer = NULL;
    s32 nBitOffset = 0;
    s32 nUsedBits = 0;
    s32 i = 0;
    s32 nTriLingBits = 0;
    H264PpsInfo pStaticPpsInf;
    //H264PpsInfo* pPpsBuf = NULL;
    H264PpsInfo* pNewPpsInf = NULL;

    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;
    //add for big_array
    u8*  dataBuffer = h264Dec->dataBuffer;

    nByteCount = sliceDataLen/8;

    if((nByteCount+4) > 1024)
    {
        pBuffer = (u8*)malloc(nByteCount+4);
        if(pBuffer == NULL)
        {
            logd("malloc buffer for pBuffer error\n");
        }
    }
    else
    {
        pBuffer = dataBuffer;
    }

    if(hCtx->vbvInfo.pReadPtr+nByteCount+4 <= hCtx->vbvInfo.pVbvBufEnd)
    {
        //(*MemRead)(pBuffer,hCtx->vbvInfo.pReadPtr, nByteCount+4);
        CdcMemRead(h264Dec->memops,pBuffer, hCtx->vbvInfo.pReadPtr, nByteCount+4);
    }
    else
    {

        tmp = hCtx->vbvInfo.pVbvBufEnd-hCtx->vbvInfo.pReadPtr+1;
        CdcMemRead(h264Dec->memops, pBuffer, hCtx->vbvInfo.pReadPtr, tmp);
        CdcMemRead(h264Dec->memops, pBuffer+tmp, hCtx->vbvInfo.pVbvBuf, nByteCount+4-tmp);
    }

    if((hCtx->bDecExtraDataFlag==1) || (hCtx->bIsAvc==0))
    {
        for(i=0; i<=nByteCount; i++)
        {
            if(pBuffer[i]==0x00 && pBuffer[i+1]==0x00 && pBuffer[i+2]==0x01)
            {
                //logd("find the startcode\n");
                nByteCount = i;
                if((i>0) && pBuffer[i-1]==0x00)
                {
                   nByteCount--;
                }
                break;
            }
        }
    }

    nTriLingBits = H264DecodeRbspTraiLing(pBuffer+nByteCount-1);

    pBuf = pBuffer+1;
    nByteCount -= 1;

    nPpsId = h264DecCtx->GetUeGolomb((void*)h264DecCtx);
    pPpsInf = &pStaticPpsInf;
    memset(pPpsInf, 0, sizeof(H264PpsInfo));
    nUsedBits = H264GetVLCSymbol(pBuf, nBitOffset, nByteCount);
    nBitOffset += nUsedBits;

    tmp = h264DecCtx->GetUeGolomb((void*)h264DecCtx);

    if((nPpsId>=256) ||(tmp >= MAX_SPS_COUNT) || (hCtx->pSpsBuffers[tmp]==NULL))
    {
        logd("the sequence spsId is not equal to the spsId\n");
        if(pBuffer != dataBuffer)
        {
            free(pBuffer);
            pBuffer = NULL;
        }
        //hCtx->bNeedFindPPS = 1;
        //hCtx->bNeedFindSPS = 1;
        return VRESULT_ERR_FAIL;
    }
    nSpsId = tmp;
    nUsedBits = H264GetVLCSymbol(pBuf, nBitOffset, nByteCount);
    nBitOffset += nUsedBits;

    pPpsInf->nSpsId = tmp;
    pPpsInf->bCabac = h264DecCtx->GetBits((void*)h264DecCtx, 1);   // entropy coding mode flag
    pPpsInf->pPicOrderPresent = h264DecCtx->GetBits((void*)h264DecCtx, 1);

    nBitOffset += 2;

    pPpsInf->nSliceGroupCount = h264DecCtx->GetUeGolomb((void*)h264DecCtx)+1;

    nUsedBits = H264GetVLCSymbol(pBuf, nBitOffset, nByteCount);
    nBitOffset += nUsedBits;

    if(pPpsInf->nSliceGroupCount > 1)
    {
        if(pBuffer != dataBuffer)
        {
            free(pBuffer);
            pBuffer = NULL;
        }
        //hCtx->bNeedFindPPS = 1;
        return VRESULT_ERR_FAIL;
    }
    pPpsInf->nRefCount[0] = h264DecCtx->GetUeGolomb((void*)h264DecCtx)+1;
    // num_ref_idx_l0_active_minus1 +1
    nUsedBits = H264GetVLCSymbol(pBuf, nBitOffset, nByteCount);
    nBitOffset += nUsedBits;

    pPpsInf->nRefCount[1] = h264DecCtx->GetUeGolomb((void*)h264DecCtx)+1;
    // num_ref_idx_l1_active_minus1 +1
    nUsedBits = H264GetVLCSymbol(pBuf, nBitOffset, nByteCount);
    nBitOffset += nUsedBits;

    if(pPpsInf->nRefCount[0]>32 || pPpsInf->nRefCount[1]>32)
    {
        if(pBuffer != dataBuffer)
        {
            free(pBuffer);
            pBuffer = NULL;
        }
        //hCtx->bNeedFindPPS = 1;
        return VRESULT_ERR_FAIL;
    }

    //hCtx->nPpsId = nPpsId;

    pPpsInf->bWeightedPred = h264DecCtx->GetBits((void*)h264DecCtx, 1);
    pPpsInf->nWeightedBIpredIdc = h264DecCtx->GetBits((void*)h264DecCtx, 2);
    nBitOffset += 3;

    pPpsInf->nInitQp = h264DecCtx->GetSeGolomb((void*)h264DecCtx)+26;
    nUsedBits = H264GetVLCSymbol(pBuf, nBitOffset, nByteCount);
    nBitOffset += nUsedBits;

    h264DecCtx->GetSeGolomb((void*)h264DecCtx);
    nUsedBits = H264GetVLCSymbol(pBuf, nBitOffset, nByteCount);
    nBitOffset += nUsedBits;

    pPpsInf->nChromaQpIndexOffset[0] = h264DecCtx->GetSeGolomb((void*)h264DecCtx);
    nUsedBits = H264GetVLCSymbol(pBuf, nBitOffset, nByteCount);
    nBitOffset += nUsedBits;

    pPpsInf->bDeblockingFilterParamPresent = h264DecCtx->GetBits((void*)h264DecCtx, 1);
    pPpsInf->bConstrainedIntraPred = h264DecCtx->GetBits((void*)h264DecCtx, 1);
    pPpsInf->pRedundatPicCntPresent =  h264DecCtx->GetBits((void*)h264DecCtx, 1);
    nBitOffset += 3;

    pPpsInf->nChromaQpIndexOffset[1] = pPpsInf->nChromaQpIndexOffset[0];

    pPpsInf->bTransform8x8Mode = 0;
    memset(pPpsInf->nScalingMatrix4, 16, 6*16*sizeof(u8));
    memset(pPpsInf->nScalingMatrix8, 16, 2*64*sizeof(u8));

    if((nPpsId<MAX_PPS_COUNT) && (hCtx->pPpsBuffers[nPpsId]==NULL))
    {
        if(hCtx->nPpsBufferNum < 32)
        {
            hCtx->nPpsBufferIndex[hCtx->nPpsBufferNum++] = nPpsId;
        }
    }
    pNewPpsInf = (H264PpsInfo*)H264AllocParameterSet((void**)hCtx->pPpsBuffers,
                                                      nPpsId,
                                                      MAX_PPS_COUNT,
                                                      sizeof(H264PpsInfo));
    if(pNewPpsInf == NULL)
    {
        hCtx->nPpsBufferNum--;
        if(pBuffer != dataBuffer)
        {
            free(pBuffer);
            pBuffer = NULL;
        }
        return VDECODE_RESULT_UNSUPPORTED;
    }

    if(nBitOffset+nTriLingBits >= nByteCount*8)
    {
        memcpy(pNewPpsInf, pPpsInf, sizeof(H264PpsInfo));
        if(pBuffer != dataBuffer)
        {
            free(pBuffer);
            pBuffer = NULL;
        }
        return VDECODE_RESULT_OK;
    }

    if(H264JudgeMoreRbspData(hCtx, nSpsId) > 0)
    {
        if(H264CheckMoreRbspData(pBuf, nBitOffset, nTriLingBits, nByteCount) > 0)
        {
            pPpsInf->bTransform8x8Mode = h264DecCtx->GetBits((void*)h264DecCtx, 1);

            H264DecodeScalingMatrices(h264DecCtx,
                                      hCtx->pSpsBuffers[nSpsId],
                                      pPpsInf,
                                      0,
                                      pPpsInf->nScalingMatrix4,
                                      pPpsInf->nScalingMatrix8);
            hCtx->bScalingMatrixPresent = 1;
            pPpsInf->nChromaQpIndexOffset[1] = h264DecCtx->GetSeGolomb((void*)h264DecCtx);
        }
    }

    memcpy(pNewPpsInf, pPpsInf, sizeof(H264PpsInfo));

    if(pBuffer != dataBuffer)
    {
        free(pBuffer);
        pBuffer = NULL;
    }
    return VDECODE_RESULT_OK;
}

void H264ProcessActiveFormat(H264DecCtx* h264DecCtx, H264Context* hCtx,
                         H264SpsInfo* pCurSps, s32 nSliceDataLen)
{
    H264Dec* h264Dec = NULL;
    u32 remainDataLen = 0;
    u8* ptr = NULL;
    s32 i = 0;
    u32 nextCode = 0;
    u8 acticveFormatFlag = 0;
    u8 activeFormat = 0;
    u8* buffer = NULL;
    u8* dataBufferTemp = NULL;

    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;
    //add for big_array
    u8* dataBuffer = h264Dec->dataBuffer;

    if((pCurSps->bCodedFrameRatio != H264_CODED_FRAME_RATIO_4_3)
            && (pCurSps->bCodedFrameRatio!= H264_CODED_FRAME_RATIO_16_9))
    {
        return;
    }

    buffer = dataBuffer;

    if(nSliceDataLen > 4096)
    {
        loge(" the buffer[4096] is not enought, requstSize = %d",nSliceDataLen);
        dataBufferTemp =  (u8*)malloc(nSliceDataLen);
        if(dataBufferTemp == NULL)
        {
            loge("malloc failed!");
            abort();
        }
        buffer = dataBufferTemp;
    }
    if(hCtx->vbvInfo.pReadPtr+nSliceDataLen > hCtx->vbvInfo.pVbvBufEnd)
    {
        remainDataLen = hCtx->vbvInfo.pVbvBufEnd - hCtx->vbvInfo.pReadPtr+1;

        CdcMemRead(h264Dec->memops, buffer, hCtx->vbvInfo.pReadPtr, remainDataLen);
        CdcMemRead(h264Dec->memops, buffer+remainDataLen, \
                   hCtx->vbvInfo.pVbvBuf, nSliceDataLen-remainDataLen);
        ptr = buffer;
    }
    else
    {
        CdcMemRead(h264Dec->memops, buffer, hCtx->vbvInfo.pReadPtr, nSliceDataLen);
        ptr = buffer;
    }

    nextCode = 0xffffffff;
    for(i=0; i<nSliceDataLen; i++)
    {
        nextCode <<= 8;
        nextCode |= ptr[i];
        if(nextCode == 0x44544731)
        {
            break;
        }
    }

    if(i >= nSliceDataLen)
    {
        if(dataBufferTemp != NULL)
        {
            free(dataBufferTemp);
            dataBufferTemp = NULL;
        }
        return;
    }

    acticveFormatFlag = (ptr[i+1]>>6)& 0x01;
    if(acticveFormatFlag == 1)
    {
        activeFormat = ptr[i+2]& 0x0f;
    }

    //hCtx->nLeftOffset = 0;
    //hCtx->nRightOffset = 0;
    //hCtx->nTopOffset = 0;
    //hCtx->nBottomOffset = 0;

    switch(activeFormat)
    {
        case 2:            //box 16:9 (top)
            if(pCurSps->bCodedFrameRatio == H264_CODED_FRAME_RATIO_4_3)
            {
                hCtx->nBottomOffset = pCurSps->nFrmRealHeight-(pCurSps->nFrmRealWidth*9/16);
            }
            break;
        case 3:            //box 14:9 (top)
            if(pCurSps->bCodedFrameRatio == H264_CODED_FRAME_RATIO_4_3)
            {
                hCtx->nBottomOffset = pCurSps->nFrmRealHeight-(pCurSps->nFrmRealWidth*9/14);
            }
            else if(pCurSps->bCodedFrameRatio == H264_CODED_FRAME_RATIO_16_9)
            {
                hCtx->nLeftOffset = pCurSps->nFrmRealWidth-(pCurSps->nFrmRealHeight*14/9);
                hCtx->nLeftOffset /= 2;
                hCtx->nRightOffset = hCtx->nLeftOffset;
            }
            break;
        case 4:            //box > 16:9 (center)
            hCtx->nTopOffset =  pCurSps->nFrmRealHeight-(pCurSps->nFrmRealWidth*10/19);
            hCtx->nTopOffset /= 2;
            hCtx->nBottomOffset = hCtx->nTopOffset;
            break;
        case 9:            //4:3 (center)
            if(pCurSps->bCodedFrameRatio == H264_CODED_FRAME_RATIO_16_9)
            {
                hCtx->nLeftOffset = pCurSps->nFrmRealWidth-(pCurSps->nFrmRealHeight*4/3);
                hCtx->nLeftOffset /= 2;
                hCtx->nRightOffset = hCtx->nLeftOffset;
            }
            break;
        case 10:            //16:9 (center)
            if(pCurSps->bCodedFrameRatio == H264_CODED_FRAME_RATIO_4_3)
            {
                hCtx->nTopOffset = pCurSps->nFrmRealHeight -(pCurSps->nFrmRealWidth*9/16);
                hCtx->nTopOffset /= 2;
                hCtx->nBottomOffset = hCtx->nTopOffset;
            }
            break;
        case 11:            //14:9 (center)
            if(pCurSps->bCodedFrameRatio == H264_CODED_FRAME_RATIO_4_3)
            {
                hCtx->nTopOffset = pCurSps->nFrmRealHeight -(pCurSps->nFrmRealWidth*9/14);
                hCtx->nTopOffset /= 2;
                hCtx->nBottomOffset = hCtx->nTopOffset;
            }
            else if(pCurSps->bCodedFrameRatio == H264_CODED_FRAME_RATIO_16_9)
            {
                hCtx->nLeftOffset = pCurSps->nFrmRealWidth -(pCurSps->nFrmRealHeight*14/9);
                hCtx->nLeftOffset /= 2;
                hCtx->nRightOffset = hCtx->nLeftOffset;
            }
            break;
        case 13:            //4:3 (with shoot and protect 14:9 center)
            if(pCurSps->bCodedFrameRatio == H264_CODED_FRAME_RATIO_16_9)
            {
                hCtx->nLeftOffset = pCurSps->nFrmRealWidth -(pCurSps->nFrmRealHeight*4/3);
                hCtx->nLeftOffset /= 2;
                hCtx->nRightOffset = hCtx->nLeftOffset;
            }
            break;
        case 14:            //16:9 (with shoot and protect 14:9 center)
        case 15:            //16:9 (with shoot and protect 4:3 center)
            if(pCurSps->bCodedFrameRatio == H264_CODED_FRAME_RATIO_4_3)
            {
                hCtx->nTopOffset = pCurSps->nFrmRealHeight -(pCurSps->nFrmRealWidth*9/16);
                hCtx->nTopOffset /= 2;
                hCtx->nBottomOffset = hCtx->nTopOffset;
            }
            break;
        default:
            break;
    }

    if(dataBufferTemp != NULL)
    {
        free(dataBufferTemp);
        dataBufferTemp = NULL;
    }
    return;
}

u32 H264GetSeiCntType(H264DecCtx* h264DecCtx, s32 seiPicStruct, u8 nTimeOffsetLength)
{
    u8 i = 0;
    u8 nNumClockTs = 0;
    u32 nSeiCntType = 0;
    u8 nFullTimestampFlag;
    u8 H264_SEI_NUM_CLOCK_TS_TABLE[9] = {1, 1, 1, 2, 2, 3, 3, 2, 3};

    nNumClockTs = H264_SEI_NUM_CLOCK_TS_TABLE[seiPicStruct];

    for(i = 0; i<nNumClockTs; i++)
    {
        if(h264DecCtx->GetBits((void*)h264DecCtx, 1))
        {                /* clock_timestamp_flag */

            nSeiCntType |= 1 << h264DecCtx->GetBits((void*)h264DecCtx, 2);
            h264DecCtx->GetBits((void*)h264DecCtx, 6);                 /* nuit_field_based_flag */
            nFullTimestampFlag = h264DecCtx->GetBits((void*)h264DecCtx, 1);
            h264DecCtx->GetBits((void*)h264DecCtx, 10);

            if(nFullTimestampFlag)
            {
                h264DecCtx->GetBits((void*)h264DecCtx, 17);
            }
            else
            {
                if(h264DecCtx->GetBits((void*)h264DecCtx, 1))
                {        /* seconds_flag */
                    h264DecCtx->GetBits((void*)h264DecCtx, 6);
                    /* seconds_value range 0..59 */
                    if(h264DecCtx->GetBits((void*)h264DecCtx, 1))
                    {
                        /* minutes_flag */
                        h264DecCtx->GetBits((void*)h264DecCtx, 6);
                        /* minutes_value 0..59 */
                        if(h264DecCtx->GetBits((void*)h264DecCtx, 1))
                        /* hours_flag */
                        {
                            h264DecCtx->GetBits((void*)h264DecCtx, 5);
                            /* hours_value 0..23 */
                        }
                    }
                }
            }
            if(nTimeOffsetLength > 0)
            {
                h264DecCtx->GetBits((void*)h264DecCtx, nTimeOffsetLength); /* time_offset */
            }
       }
    }
    return nSeiCntType;
}

s32 H264DecodeSei(H264DecCtx* h264DecCtx, H264Context* hCtx,s32 nSliceDataLen)
{
    s32 nDataBitIndex = 0;
    s32 num = 0;
    s32 type = 0;
    s32 code = 0;
    s32 size = 0;
    s32 remainBits = 0;
    s32 i = 0;
    s32 n = 0;
    s32 seiPicStruct = 0;
    H264SpsInfo* pCurSps = NULL;
    u32 nSeiCntType = 0;

    if(hCtx->nNewSpsId >= MAX_SPS_COUNT)
    {
        abort();
    }
    pCurSps = hCtx->pSpsBuffers[hCtx->nNewSpsId];

    H264ProcessActiveFormat(h264DecCtx, hCtx, pCurSps, nSliceDataLen>>3);

    nSliceDataLen -= 8;

    while(nDataBitIndex+16 < nSliceDataLen)
    {
        size = 0;
        type = 0;

        do
        {
            if((nDataBitIndex+8) > nSliceDataLen)
            {
                return VDECODE_RESULT_OK;
            }
            code = h264DecCtx->GetBits((void*)h264DecCtx, 8);
            nDataBitIndex += 8;
            type += code;
        }while(code == 255);

        size=0;
        do
        {
            if((nDataBitIndex+8) > nSliceDataLen)
            {
                return VDECODE_RESULT_OK;
            }
            code = h264DecCtx->GetBits((void*)h264DecCtx, 8);
            nDataBitIndex += 8;
            size += code;
        }while(code== 255);

        if(type == 1)   //SEI_TYPE_PIC_TIMING
        {
            if(pCurSps->bNalHrdParametersPresentFlag || pCurSps->bVclHrdParametersPresentFlag)
            {
                 if((nDataBitIndex+(s32)pCurSps->nCpbRemovalDelayLength)>nSliceDataLen)
                 {
                      return VDECODE_RESULT_OK;
                 }
                 h264DecCtx->GetBits((void*)h264DecCtx, pCurSps->nCpbRemovalDelayLength);
                 nDataBitIndex += pCurSps->nCpbRemovalDelayLength;
                 if((nDataBitIndex+(s32)pCurSps->nDpbOutputDelayLength)>nSliceDataLen)
                 {
                      return VDECODE_RESULT_OK;
                 }
                 h264DecCtx->GetBits((void*)h264DecCtx, pCurSps->nDpbOutputDelayLength);
                 nDataBitIndex += pCurSps->nDpbOutputDelayLength;
            }

            if(pCurSps->bPicStructPresentFlag)
            {
                if((nDataBitIndex+4)>nSliceDataLen)
                {
                      return VDECODE_RESULT_OK;
                }
                seiPicStruct = h264DecCtx->GetBits((void*)h264DecCtx, 4);
                nDataBitIndex += 4;

                nSeiCntType = 0;
                if(seiPicStruct <= 8)  //8: SEI_PIC_STRUCT_FRAME_TRIPLING
                {
                    nSeiCntType = H264GetSeiCntType(h264DecCtx, seiPicStruct,
                                                 pCurSps->nTimeOffsetLength);
                    if((nSeiCntType&3) && (seiPicStruct<=4))
                    {
                        hCtx->bProgressice = ((nSeiCntType&2)==0);
                    }
                }
            }
            return VDECODE_RESULT_OK;
        }
        else if(type == 6)  //SEI_TYPE_RECOVERY_POINT
        {
            hCtx->nSeiRecoveryFrameCnt = h264DecCtx->GetUeGolomb((void*)h264DecCtx);
            h264DecCtx->GetBits((void*)h264DecCtx, 4);
            nDataBitIndex += 36;
            if(nDataBitIndex>nSliceDataLen)
            {
                return VDECODE_RESULT_OK;
            }
        }
        else
        {
            if((nDataBitIndex+8*size)>nSliceDataLen)
            {
                 return VDECODE_RESULT_OK;
            }
            num = 8*size/32;
            remainBits = 8*size - num*32;

            for(i=0; i<num; i++)
            {
                h264DecCtx->GetBits((void*)h264DecCtx, 32);
            }
            h264DecCtx->GetBits((void*)h264DecCtx, remainBits);
            nDataBitIndex += 8*size;
        }
        //align_get_bits(&s->gb);
        n = (-nDataBitIndex) & 7;
        if(n)
        {
               if((nDataBitIndex+n)>nSliceDataLen)
               {
                   return VDECODE_RESULT_OK;
               }
            num = n/24;
            remainBits = n- num*24;

            for(i=0; i<num; i++)
            {
                h264DecCtx->GetBits((void*)h264DecCtx, 24);
            }
            h264DecCtx->GetBits((void*)h264DecCtx, remainBits);
            nDataBitIndex += n;
        }
    }
    return VDECODE_RESULT_OK;
}

void H264InitPoc(H264Context* hCtx)
{
    u32 i = 0;
    s32 nPoc = 0;
    s32 nPocCycleCnt = 0;
    s32 expectedPoc = 0;
    s32 maxFrmNum = 0;
    s32 maxPocLsb = 0;
    s32 nFieldPoc[2] = {0};
    s32 absFrmNum = 0;
    u32 nFrmNumInnPocCycle = 0;
    s32 expectedDeltaPerPocCycle = 0;

    #define MIN(a,b) ((a) > (b) ? (b) : (a))

    maxFrmNum = 1 << hCtx->pCurSps->nLog2MaxFrmNum;

    //if(hCtx->nalUnitType == NAL_IDR_SLICE)
    if(hCtx->bIdrFrmFlag == 1)
    {
        hCtx->nFrmNumOffset = 0;
    }
    else
    {
        if(hCtx->nFrmNum < hCtx->nPrevFrmNum)
        {
            hCtx->nFrmNumOffset = hCtx->nPrevFrmNumOffset + maxFrmNum;
        }
        else
        {
            hCtx->nFrmNumOffset = hCtx->nPrevFrmNumOffset;
        }
    }

    if(hCtx->pCurSps->nPocType == 0)
    {
        maxPocLsb = 1 << hCtx->pCurSps->nLog2MaxPocLsb;
        //if(hCtx->nalUnitType == NAL_IDR_SLICE)
        if(hCtx->bIdrFrmFlag == 1)
        {
            hCtx->nPrevPocMsb = 0;
            hCtx->nPrevPocLsb = 0;
        }
        if(hCtx->nPocLsb<hCtx->nPrevPocLsb && hCtx->nPrevPocLsb-hCtx->nPocLsb>=maxPocLsb/2)
        {
            hCtx->nPocMsb = hCtx->nPrevPocMsb + maxPocLsb;
        }
        else if(hCtx->nPocLsb > hCtx->nPrevPocLsb && hCtx->nPrevPocLsb-hCtx->nPocLsb <-maxPocLsb/2)
        {
            hCtx->nPocMsb = hCtx->nPrevPocMsb - maxPocLsb;
        }
        else
        {
            hCtx->nPocMsb = hCtx->nPrevPocMsb;
        }
        nFieldPoc[0] = nFieldPoc[1] = hCtx->nPocMsb+hCtx->nPocLsb;
        if(hCtx->nPicStructure == PICT_FRAME)
        {
            nFieldPoc[1] += hCtx->nDeltaPocBottom;
        }
    }
    else if(hCtx->nPocType == 1)
    {
        absFrmNum = 0;
        if(hCtx->pCurSps->nPocCycleLength != 0)
        {
            absFrmNum = hCtx->nFrmNumOffset + hCtx->nFrmNum;
        }
        if(hCtx->nNalRefIdc==0 && absFrmNum > 0)
        {
            absFrmNum--;
        }
        expectedDeltaPerPocCycle = 0;
        for(i=0; i<hCtx->pCurSps->nPocCycleLength; i++)
        {
            expectedDeltaPerPocCycle += hCtx->pCurSps->nOffsetForRefFrm[i];
        }
        expectedPoc = 0;
        if(absFrmNum > 0)
        {
            nPocCycleCnt = (absFrmNum-1)/hCtx->pCurSps->nPocCycleLength;
            nFrmNumInnPocCycle = (absFrmNum-1)%hCtx->pCurSps->nPocCycleLength;
            expectedPoc = nPocCycleCnt * expectedDeltaPerPocCycle;
            for(i=0; i<=nFrmNumInnPocCycle; i++)
            {
                expectedPoc = expectedPoc + hCtx->pCurSps->nOffsetForRefFrm[i];
            }
        }
        if(hCtx->nNalRefIdc == 0)
        {
            expectedPoc = expectedPoc + hCtx->pCurSps->nOffsetForNonRefPic;
        }

        nFieldPoc[0] = expectedPoc + hCtx->nDeltaPoc[0];
        nFieldPoc[1] = nFieldPoc[0]+hCtx->pCurSps->nOffsetForTopToBottomField;
        if(hCtx->nPicStructure == PICT_FRAME)
        {
            nFieldPoc[1] += hCtx->nDeltaPoc[1];
        }
    }
    else
    {
        //if(hCtx->nalUnitType == NAL_IDR_SLICE)
        if(hCtx->bIdrFrmFlag == 1)
        {
            nPoc = 0;
        }
        else
        {
            if(hCtx->nNalRefIdc)
            {
                nPoc = 2*(hCtx->nFrmNumOffset+hCtx->nFrmNum);
            }
            else
            {
                nPoc = 2*(hCtx->nFrmNumOffset+hCtx->nFrmNum) -1;
            }
        }
        nFieldPoc[0] = nPoc;
        nFieldPoc[1] = nPoc;
    }

    if(hCtx->nPicStructure != PICT_BOTTOM_FIELD)
    {
        if(nFieldPoc[0]%2==1)
        {
            hCtx->nPicPocDeltaNum = 1;
        }

       // hCtx->frmBufInf.pCurPicturePtr->nFrmNum = hCtx->nCurPicNum;
        hCtx->frmBufInf.pCurPicturePtr->nFieldPoc[0] = nFieldPoc[0];
        hCtx->frmBufInf.pCurPicturePtr->nPoc = nFieldPoc[0];
        hCtx->frmBufInf.pCurPicturePtr->nFieldPicStructure[0] = hCtx->nPicStructure;
        hCtx->frmBufInf.pCurPicturePtr->nFieldNalRefIdc[0] = hCtx->nNalRefIdc;
        hCtx->frmBufInf.pCurPicturePtr->bMbAffFrame = hCtx->bMbAffFrame;
    }

    if(hCtx->nPicStructure != PICT_TOP_FIELD)
    {
        //hCtx->frmBufInf.pCurPicturePtr->nFrmNum = hCtx->nCurPicNum;
        hCtx->frmBufInf.pCurPicturePtr->nFieldPoc[1] = nFieldPoc[1];
        hCtx->frmBufInf.pCurPicturePtr->nPoc = nFieldPoc[1];
        hCtx->frmBufInf.pCurPicturePtr->nFieldPicStructure[1] = hCtx->nPicStructure;
        hCtx->frmBufInf.pCurPicturePtr->nFieldNalRefIdc[1] = hCtx->nNalRefIdc;
    }
    if(hCtx->nPicStructure == PICT_FRAME || hCtx->bFstField==0)
    {
        hCtx->frmBufInf.pCurPicturePtr->nPoc = \
            MIN(hCtx->frmBufInf.pCurPicturePtr->nFieldPoc[0],\
                hCtx->frmBufInf.pCurPicturePtr->nFieldPoc[1]);
    }
}

s32 H264GetValidBufferIndex(H264Context* hCtx, s32* bufIndex)
{
    s32 i = 0;

    if(hCtx->frmBufInf.nMaxValidFrmBufNum < 18)
    {
        for(i=hCtx->nBufIndexOffset;
            i<hCtx->frmBufInf.nMaxValidFrmBufNum+hCtx->nBufIndexOffset;
            i++)
        {
            if(hCtx->frmBufInf.picture[i].pVPicture==NULL &&
                hCtx->frmBufInf.picture[i].pScaleDownVPicture==NULL&&
                hCtx->frmBufInf.picture[i].bDropBFrame!=1)
            {
                hCtx->frmBufInf.picture[i].nReference = 0;
                hCtx->frmBufInf.picture[i].nDecodeBufIndex = i;
                hCtx->frmBufInf.picture[i].nDispBufIndex = i;

                break;
            }
        }

        if(i == hCtx->frmBufInf.nMaxValidFrmBufNum+hCtx->nBufIndexOffset)
        {
            return VDECODE_RESULT_NO_FRAME_BUFFER;
        }
    }
    else if(hCtx->nNalRefIdc == 0)
    {
        for(i=hCtx->frmBufInf.nMaxValidFrmBufNum-1; i>=hCtx->nRefFrmCount+2; i--)
        {
            if((hCtx->frmBufInf.picture[i].pVPicture==NULL) &&
                (hCtx->frmBufInf.picture[i].pScaleDownVPicture==NULL))
            {
                hCtx->frmBufInf.picture[i].nReference = 0;
                hCtx->frmBufInf.picture[i].nDecodeBufIndex = 0;
                hCtx->frmBufInf.picture[i].nDispBufIndex = i;
                if(i < 18)
                {
                    hCtx->frmBufInf.picture[i].nDecodeBufIndex = i;
                }
                break;
            }
        }
        if(i < hCtx->nRefFrmCount+2)
        {
            return VDECODE_RESULT_NO_FRAME_BUFFER;
        }
    }
    else
    {
        for(i=1; i<18; i++)
        {
            if((hCtx->frmBufInf.picture[i].pVPicture==NULL) &&
                (hCtx->frmBufInf.picture[i].pScaleDownVPicture==NULL))
            {
                hCtx->frmBufInf.picture[i].nReference = 0;
                hCtx->frmBufInf.picture[i].nDecodeBufIndex = i;
                hCtx->frmBufInf.picture[i].nDispBufIndex = i;
                break;
            }
        }
        if(i==18)
        {
            return VDECODE_RESULT_NO_FRAME_BUFFER;
        }
    }
    *bufIndex = i;
    return VDECODE_RESULT_OK;
}

s32 H264FrameStart(H264DecCtx* h264DecCtx, H264Context* hCtx)
{
    s32 i = 0;
    s32 j = 0;
    s32 k = 0;
    //s32 refFrmNum = 0;
    s32 bufIndex = 0xff;
    H264PicInfo* curPicPtr  = NULL;
    u32 minDecFrmOrder = 0xffffffff;
    u32 validBufIndex[32];
    //H264MvcContext* pMvcContext = NULL;

    if(hCtx->nCurFrmNum >= 0xffffffff)
    {
        for(i=0+hCtx->nBufIndexOffset;
            i<hCtx->frmBufInf.nMaxValidFrmBufNum+hCtx->nBufIndexOffset;
            i++)
        {
            if(hCtx->frmBufInf.picture[i].pVPicture!=NULL)
            {
               if(hCtx->frmBufInf.picture[i].nDecFrameOrder < minDecFrmOrder)
               {
                   minDecFrmOrder = hCtx->frmBufInf.picture[i].nDecFrameOrder;
               }
               validBufIndex[k] = i;
               k++;
            }
        }
        //logv("hCtx->nCurFrmNum=%d, minDecFrmOrder=%d\n",
        //hCtx->nCurFrmNum, minDecFrmOrder);
        hCtx->nCurFrmNum -= minDecFrmOrder;
        for(i=0; i<k; i++)
        {
            j = validBufIndex[i];
            hCtx->frmBufInf.picture[j].nDecFrameOrder -= minDecFrmOrder;
            //logv("index=%d, nDecFrameOrder=%d\n",
            //j, hCtx->frmBufInf.picture[j].nDecFrameOrder);
        }
    }

    if(H264GetValidBufferIndex(hCtx, &i) == VDECODE_RESULT_NO_FRAME_BUFFER)
    {
        return VDECODE_RESULT_NO_FRAME_BUFFER;
    }

    if((h264DecCtx->vconfig.bScaleDownEn==1) ||
        (h264DecCtx->vconfig.bRotationEn==1) ||
        (hCtx->bYv12OutFlag==1)||(hCtx->bNv21OutFlag==1))
    {
        hCtx->frmBufInf.picture[i].pScaleDownVPicture =
            FbmRequestBuffer(hCtx->pFbmScaledown);
        if(hCtx->frmBufInf.picture[i].pScaleDownVPicture == NULL)
        {
            return VDECODE_RESULT_NO_FRAME_BUFFER;
        }
        if((hCtx->nNalRefIdc!=0) && (hCtx->bDecodeKeyFrameOnly==0))
        {
            hCtx->frmBufInf.picture[i].pVPicture = FbmRequestBuffer(hCtx->pFbm);
            if(hCtx->frmBufInf.picture[i].pVPicture == NULL)
            {
                FbmReturnBuffer(hCtx->pFbmScaledown,
                    hCtx->frmBufInf.picture[i].pScaleDownVPicture, 0);
                hCtx->frmBufInf.picture[i].pScaleDownVPicture = NULL;
                return VDECODE_RESULT_NO_FRAME_BUFFER;
            }
        }
    }
    else
    {
        hCtx->frmBufInf.picture[i].pVPicture = FbmRequestBuffer(hCtx->pFbm);
        if(hCtx->frmBufInf.picture[i].pVPicture == NULL)
        {
            return VDECODE_RESULT_NO_FRAME_BUFFER;
        }
    }

    hCtx->frmBufInf.picture[i].bHasDispedFlag = 0;

    if(hCtx->frmBufInf.picture[i].nDecodeBufIndex !=
        hCtx->frmBufInf.picture[i].nDispBufIndex)
    {
        bufIndex = hCtx->frmBufInf.picture[i].nDecodeBufIndex;
        hCtx->frmBufInf.picture[bufIndex].pVPicture =
            hCtx->frmBufInf.picture[i].pVPicture;
    }

    hCtx->frmBufInf.pCurPicturePtr = &(hCtx->frmBufInf.picture[i]);

    curPicPtr  = hCtx->frmBufInf.pCurPicturePtr;

    curPicPtr->bDecErrorFlag = 0;
    curPicPtr->bDecFirstFieldError = 0;
    curPicPtr->bDecSecondFieldError = 0;
    curPicPtr->nPicStructure = hCtx->nPicStructure;
    curPicPtr->nPictType = hCtx->nSliceType;

    curPicPtr->bKeyFrame = (curPicPtr->nPictType==H264_I_TYPE);
    curPicPtr->nReference = 0;
    if(hCtx->nNalRefIdc != 0)
    {
        curPicPtr->nReference = hCtx->nPicStructure;
    }

    if(hCtx->frmBufInf.pCurPicturePtr->nPictType != H264_B_TYPE)
    {
        hCtx->frmBufInf.pLastPicturePtr = hCtx->frmBufInf.pNextPicturePtr;
        if(hCtx->nNalRefIdc != 0)
        {
            hCtx->frmBufInf.pNextPicturePtr = hCtx->frmBufInf.pCurPicturePtr;;
        }
    }
    return VDECODE_RESULT_OK;
}

void H264ExchangePts(int64_t* pts1, int64_t*pts2)
{
    s64 temp = 0;

    temp = *pts1;
    *pts1 = *pts2;
    *pts2 = temp;
}

void H264CalculatePicturePts( H264Context* hCtx)
{
    s32 idrFrmIndex = 0;
    s32 i = 0;
    s32 j = 0;
    s32 nPoc= 0;
    int64_t *ptsPtr = 0;
    int64_t* pPicPts = 0;
    s32 flag1 = 0;
    s32 flag2 = 0;
    int64_t nPicDuration = 0;
    s32 diffPoc = 0;
    VideoPicture* pVPicture = NULL;

    if(hCtx->frmBufInf.pCurPicturePtr->pScaleDownVPicture != NULL)
    {
        pVPicture = hCtx->frmBufInf.pCurPicturePtr->pScaleDownVPicture;
    }
    else
    {
        pVPicture = hCtx->frmBufInf.pCurPicturePtr->pVPicture;
    }

    if(pVPicture==NULL)
    {
        return;
    }
    pVPicture->nPts = hCtx->vbvInfo.nValidDataPts;
    if(hCtx->vbvInfo.nValidDataPts != H264VDEC_ERROR_PTS_VALUE)
    {
        hCtx->vbvInfo.nLastUsedValidPts = hCtx->vbvInfo.nValidDataPts;
    }

    hCtx->vbvInfo.nValidDataPts = H264VDEC_ERROR_PTS_VALUE;
    if(hCtx->bDecodeKeyFrameOnly==1)
    {
        return;
    }
    if(hCtx->nPicPocDeltaNum == 0 && hCtx->frmBufInf.pLastPicturePtr!=NULL)
    {
        hCtx->nPicPocDeltaNum = 2;
    }

    if(hCtx->vbvInfo.nPicDuration != 0)
    {
        hCtx->nEstimatePicDuration = hCtx->vbvInfo.nPicDuration;
    }
    else if(hCtx->nEstimatePicDuration == 0)
    {
        hCtx->nEstimatePicDuration = 40000;
    }

    if(pVPicture->nPts == H264VDEC_ERROR_PTS_VALUE )
    {
        if((hCtx->frmBufInf.pCurPicturePtr->nPoc == 0) || (hCtx->nPicPocDeltaNum==0))
        {
            if(hCtx->vbvInfo.nNextPicPts > 0)
            {
                pVPicture->nPts = hCtx->vbvInfo.nNextPicPts;
                hCtx->vbvInfo.nPrePicPts = pVPicture->nPts;
                hCtx->vbvInfo.nPrePicPoc = hCtx->frmBufInf.pCurPicturePtr->nPoc;
            }
            else
            {
                pVPicture->nPts = hCtx->vbvInfo.nVbvDataPts;
                hCtx->vbvInfo.nPrePicPts = pVPicture->nPts;
                hCtx->vbvInfo.nPrePicPoc = hCtx->frmBufInf.pCurPicturePtr->nPoc;
            }
        }
        else
        {
            pVPicture->nPts = hCtx->vbvInfo.nPrePicPts;
            nPicDuration = hCtx->nEstimatePicDuration/hCtx->nPicPocDeltaNum;
            pVPicture->nPts += (hCtx->frmBufInf.pCurPicturePtr->nPoc-
                                hCtx->vbvInfo.nPrePicPoc)*nPicDuration;

        }
    }
    else
    {
        if(hCtx->vbvInfo.nPrePicPoc!=0 && hCtx->vbvInfo.nPrePicPts!=0 &&
            hCtx->frmBufInf.pCurPicturePtr->nPoc!=0)
        {
              diffPoc = (hCtx->frmBufInf.pCurPicturePtr->nPoc - hCtx->vbvInfo.nPrePicPoc);
            if(diffPoc != 0)
            {
                hCtx->nEstimatePicDuration = (pVPicture->nPts - hCtx->vbvInfo.nPrePicPts);
                hCtx->nEstimatePicDuration /= diffPoc;
                hCtx->nEstimatePicDuration *= hCtx->nPicPocDeltaNum;
                hCtx->vbvInfo.nPicDuration = 0;
            }
        }
        hCtx->vbvInfo.nPrePicPts = pVPicture->nPts;
        hCtx->vbvInfo.nPrePicPoc = hCtx->frmBufInf.pCurPicturePtr->nPoc;
    }
    if(hCtx->vbvInfo.nNextPicPts < (pVPicture->nPts+hCtx->nEstimatePicDuration))
    {
        hCtx->vbvInfo.nNextPicPts = pVPicture->nPts+hCtx->nEstimatePicDuration;
    }

    if(hCtx->frmBufInf.pCurPicturePtr->nPoc==0)
    {
        return;
    }

    for(i=hCtx->nDelayedPicNum-1; i>=0; i--)
    {
        if(hCtx->frmBufInf.pDelayedPic[i]->nPoc==0)
        {
            idrFrmIndex = i;
            break;
        }
    }

    i = idrFrmIndex-1;
    ptsPtr = &pVPicture->nPts;
    nPoc = hCtx->frmBufInf.pCurPicturePtr->nPoc;

    while(1)
    {
        for(j=i+1; ; j++)
        {
            if(hCtx->frmBufInf.pDelayedPic[j]== NULL)
            {
                break;
            }

            if(hCtx->frmBufInf.pDelayedPic[j]->pVPicture == NULL)
            {
                break;
            }
            flag1 = nPoc < hCtx->frmBufInf.pDelayedPic[j]->nPoc;

            if(hCtx->frmBufInf.pDelayedPic[j]->pScaleDownVPicture!= NULL)
            {
                pPicPts = &hCtx->frmBufInf.pDelayedPic[j]->pScaleDownVPicture->nPts;
            }
            else if(hCtx->frmBufInf.pDelayedPic[j]->pVPicture!= NULL)
            {
                pPicPts = &hCtx->frmBufInf.pDelayedPic[j]->pVPicture->nPts;
            }

            flag2 = (*ptsPtr) < *pPicPts;
            if((flag1+flag2)==1)
            {
                if(nPoc!=hCtx->frmBufInf.pDelayedPic[j]->nPoc)
                {
                    H264ExchangePts(ptsPtr, pPicPts);
                }
            }
        }
        i++;
        if(hCtx->frmBufInf.pDelayedPic[i] == NULL)
        {
            break;
        }

        if(hCtx->frmBufInf.pDelayedPic[i]->pScaleDownVPicture!= NULL)
        {
            ptsPtr = &hCtx->frmBufInf.pDelayedPic[i]->pScaleDownVPicture->nPts;
        }
        else if(hCtx->frmBufInf.pDelayedPic[i]->pVPicture!= NULL)
        {
            ptsPtr = &hCtx->frmBufInf.pDelayedPic[i]->pVPicture->nPts;
        }
        nPoc = hCtx->frmBufInf.pDelayedPic[i]->nPoc;
    }
}

s32 H264CheckNewFrame(H264Context* hCtx)
{
    if(hCtx->nDecFrameStatus == H264_START_DEC_FRAME)
    {
        if(hCtx->frmBufInf.pCurPicturePtr!= NULL)
        {
            if((hCtx->bFstField==0)||(hCtx->nPicStructure==PICT_FRAME))
            {
                hCtx->nNalRefIdc = hCtx->frmBufInf.pCurPicturePtr->nNalRefIdc;
                hCtx->nSliceType = hCtx->frmBufInf.pCurPicturePtr->nPictType;
                hCtx->nFrmNum   = hCtx->frmBufInf.pCurPicturePtr->nFrmNum;
                hCtx->frmBufInf.pCurPicturePtr->bDecErrorFlag = 1;
                hCtx->frmBufInf.pCurPicturePtr->bDecSecondFieldError = 1;
                logd("here3:bDecSecondFieldError=1\n");
                hCtx->nDecFrameStatus = H264_END_DEC_FRAME;
                hCtx->bLastMbInSlice = 0;
                loge("here1: the first slice of the frame is not 0");
            }
            else if(hCtx->bFstField == 1)
            {
                loge("here2: the first slice of the frame is not 0");
                hCtx->frmBufInf.pCurPicturePtr->bDecFirstFieldError = 1;
                hCtx->frmBufInf.pCurPicturePtr->bDecErrorFlag = 1;
                hCtx->bLastMbInSlice = 0;
                logd("here4:bDecFirstFieldError=1\n");
            }
            hCtx->nCurSliceNum = 0;
            return VRESULT_DEC_FRAME_ERROR;
        }
    }
    return VDECODE_RESULT_OK;
}
s32 H264DecodeSliceHeader(H264DecCtx* h264DecCtx, H264Context* hCtx)
{
    u8  lastPicStructure = 0;
    u8 lastFieldFlag = 0;
   // u32 i  = 0;
    s32 tmp  = 0;
    u32 nPpsId = 0;
    u32 bFstMbInSlice = 0;
    u32 nSliceType    = 0;
    s32 defaultRefListDone = 0;
    s32 nLastFrmNum = 0;
    u8  nSliceTypeMap[5]= {H264_P_TYPE, H264_B_TYPE, H264_I_TYPE, H264_SP_TYPE, H264_SI_TYPE};
    H264Dec* h264Dec = NULL;
    //H264MvcContext* pMvcContext = NULL;
    s64 nPicturePts = 0;
    VideoPicture* pVpicture = NULL;
    u8 flag1 = 0;
    u8 flag2 = 0;
    u8 flag3 = 0;
    u8 flag4 = 0;
    u8 flag5 = 0;
    u32 diffFrmNum = 0;
    s32 ret = 0;
    s32 index = 0;
    H264PpsInfo* pLastPps = NULL;
    H264SpsInfo* pLastSps = NULL;
    H264MvBufInf* pH264MvBufInfNode = NULL;

    h264Dec = (H264Dec*) h264DecCtx->pH264Dec;

    bFstMbInSlice = h264DecCtx->GetUeGolomb((void*)h264DecCtx);
    flag1 = (bFstMbInSlice ==0);
    flag2 = (hCtx->bLastMbInSlice>(s32)bFstMbInSlice) &&
        (hCtx->bLastMbInSlice!=2*(s32)bFstMbInSlice);

    if(flag1 || flag2)
    {
        ret = H264CheckNewFrame(hCtx);
        if(ret != VDECODE_RESULT_OK)
        {
            return ret;
        }
        hCtx->nCurSliceNum = 0;
    }

    if((bFstMbInSlice!=0) && (hCtx->nDecFrameStatus ==H264_END_DEC_FRAME)
        && (hCtx->bLastDropBFrame==1))
    {
        return VRESULT_DROP_B_FRAME;
    }

    hCtx->bFstMbInSlice = bFstMbInSlice;
    nSliceType = h264DecCtx->GetUeGolomb((void*)h264DecCtx);

    if(nSliceType > 9)
    {
        //log("slice type is too large\n");
          logi("the slice type is invalid\n");
          if(hCtx->frmBufInf.pCurPicturePtr != NULL)
          {
              hCtx->frmBufInf.pCurPicturePtr->bDecErrorFlag = 1;
          }
        return VRESULT_ERR_FAIL;
    }
    nSliceType = (nSliceType > 4)? nSliceType-5 : nSliceType;
    //0: P slice; 1: B slice; 2: I slice; 3: SP slice; 4: SI slice
    nSliceType =  nSliceTypeMap[nSliceType];

    if(hCtx->bNeedFindIFrm == 1)
    {
        if((h264Dec->nDecStreamIndex==0) && (nSliceType!=H264_I_TYPE))
        {
              logi("need find the I frame to start decode\n");
            return VRESULT_ERR_FAIL;
        }
        else if((h264Dec->nDecStreamIndex==1)&&(nSliceType!=H264_P_TYPE))
        {
            logi("need find the I frame to start decode\n");
            return VRESULT_ERR_FAIL;
        }
        hCtx->bNeedFindIFrm = 0;
    }

    if((nSliceType == H264_I_TYPE)||(hCtx->nCurSliceNum!=0 &&
        nSliceType==hCtx->nLastSliceType))
    {
        defaultRefListDone = 1;
    }

    if(nSliceType == H264_B_TYPE && hCtx->frmBufInf.pLastPicturePtr==NULL)
    {
          logi("need drop the B frame\n");
        return VRESULT_ERR_FAIL;
    }

    hCtx->nSliceType = nSliceType;
    nPpsId = h264DecCtx->GetUeGolomb((void*)h264DecCtx);

    if(nPpsId >= 256)
    {
        logi("the ppsId is %d, larger than 256\n", nPpsId);
         if(hCtx->frmBufInf.pCurPicturePtr != NULL)
           {
             hCtx->frmBufInf.pCurPicturePtr->bDecErrorFlag = 1;
        }
        return VRESULT_ERR_FAIL;
    }

    if(hCtx->pPpsBuffers[nPpsId] == NULL)
    {
        //logd("the slice ppsid is not equal to the ppsid, nPpsId=%d,
        //hCtx->nPpsId=%d\n", nPpsId, hCtx->nPpsId);
        logi("nPpsId=%d, hCtx->pPpsBuffers[nPpsId]=NULL\n", nPpsId);
         if(hCtx->frmBufInf.pCurPicturePtr != NULL)
          {
             hCtx->frmBufInf.pCurPicturePtr->bDecErrorFlag = 1;
           }
        return VRESULT_ERR_FAIL;
    }

    pLastPps = hCtx->pCurPps;
    pLastSps = hCtx->pCurSps;
    hCtx->pCurPps = hCtx->pPpsBuffers[nPpsId];
    hCtx->pCurSps = hCtx->pSpsBuffers[hCtx->pCurPps->nSpsId];

    if((pLastPps!=NULL) && (pLastSps!=NULL))
    {
         if((pLastPps!=hCtx->pCurPps) || (pLastSps!=hCtx->pCurSps))
         {
          hCtx->bScalingMatrixPresent = 1;
         }
    }

    hCtx->pCurPps = hCtx->pPpsBuffers[nPpsId];
    hCtx->pCurSps = hCtx->pSpsBuffers[hCtx->pCurPps->nSpsId];

    nLastFrmNum = hCtx->nFrmNum;

    hCtx->nFrmNum = h264DecCtx->GetBits((void*)h264DecCtx, hCtx->pCurSps->nLog2MaxFrmNum);
    diffFrmNum = (nLastFrmNum>=hCtx->nFrmNum)?
                  (nLastFrmNum-hCtx->nFrmNum):(hCtx->nFrmNum-nLastFrmNum);

    if((hCtx->bDecodeKeyFrameOnly==0)&& (hCtx->nCurFrmNum>=2)
             &&(nLastFrmNum!=0) && (hCtx->nFrmNum!=0) && (diffFrmNum>=2))
    {
       if(!hCtx->pCurSps->gaps_in_frame_num_value_allowed_flag && hCtx->nSliceType != H264_I_TYPE)
        {
            loge("the frame is not continue:nLastFrmNum=%d, hCtx->nFrmNum=%d\n", \
                nLastFrmNum,hCtx->nFrmNum);
            ret = H264CheckNewFrame(hCtx);
            if(ret != VDECODE_RESULT_OK)
              {
                return ret;
              }
            if((hCtx->bFstField==1) && (hCtx->nPicStructure!=PICT_FRAME))
              {
                 logd("here4: the second field is missed\n");
                 hCtx->frmBufInf.pCurPicturePtr->bDecSecondFieldError = 1;
                 hCtx->frmBufInf.pCurPicturePtr->bDecErrorFlag = 1;
                 hCtx->bLastMbInSlice = 0;
                 hCtx->bFstField = 0;
                 return VRESULT_DEC_FRAME_ERROR;
               }
            if(h264DecCtx->vconfig.bDispErrorFrame==0)
               {
                   if(hCtx->vbvInfo.vbv->nType != SBM_TYPE_FRAME_AVC)
                   {
                      if(hCtx->vbvInfo.pVbvStreamData != NULL)
                       {
                         SbmFlushStream(hCtx->vbvInfo.vbv, hCtx->vbvInfo.pVbvStreamData);
                         hCtx->vbvInfo.pVbvStreamData = NULL;
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
    }

    lastPicStructure = hCtx->nPicStructure;
    hCtx->bMbAffFrame = 0;

    if(hCtx->pCurSps->bFrameMbsOnlyFlag == 1)
    {
        hCtx->nPicStructure = PICT_FRAME;
    }
    else
    {
        if(h264DecCtx->GetBits((void*)h264DecCtx, 1) == 1)
        {
            hCtx->nPicStructure = PICT_TOP_FIELD+h264DecCtx->GetBits((void*)h264DecCtx, 1) ;
        }
        else
        {
            hCtx->nPicStructure = PICT_FRAME;
            hCtx->bMbAffFrame = hCtx->pCurSps->bMbAff;
        }
    }
    if(bFstMbInSlice != 0)
    {
        if(hCtx->nPicStructure != lastPicStructure)
        {
            hCtx->nPicStructure = lastPicStructure;
        }
    }
    else if(hCtx->bFstField ==1)
    {
        if((hCtx->nPicStructure==PICT_FRAME) || (hCtx->nPicStructure==lastPicStructure))
        {
            hCtx->bFstField = 0;
            hCtx->nNalRefIdc = hCtx->frmBufInf.pCurPicturePtr->nNalRefIdc;
            hCtx->nSliceType = hCtx->frmBufInf.pCurPicturePtr->nPictType;
            hCtx->nFrmNum   = hCtx->frmBufInf.pCurPicturePtr->nFrmNum;
             if(hCtx->frmBufInf.pCurPicturePtr != NULL)
               {
                 hCtx->frmBufInf.pCurPicturePtr->bDecErrorFlag = 1;
            }
            hCtx->nDecFrameStatus = H264_END_DEC_FRAME;
            loge("the last frame is not complete");
            return VRESULT_DEC_FRAME_ERROR;
        }
    }

    lastFieldFlag = hCtx->bFstField;
    if(hCtx->nCurSliceNum == 0)
    {
        hCtx->nLastFrmNum = nLastFrmNum;

        if(hCtx->nPicStructure == PICT_FRAME)
        {
            hCtx->bFstField = 0;
        }
        else if(hCtx->bFstField == 1)
        {

            //if((hCtx->nPicStructure==PICT_FRAME) ||
            //    (hCtx->nPicStructure==lastPicStructure))
            //modified by xyliu at 2016-05-19, for liujianshuang
            flag1 = (hCtx->nPicStructure==PICT_FRAME);
            flag2 = (hCtx->nPicStructure==lastPicStructure);
            flag3 = (hCtx->nLastFrmNum == hCtx->nFrmNum);
            flag4 = (hCtx->nSliceType==hCtx->nLastSliceType);
            flag5 = (flag3==1)&&(flag4==1)&&((hCtx->nSliceType==H264_P_TYPE)
                     ||(hCtx->nSliceType==H264_I_TYPE));
            logv("flag1=%d, flag2=%d, flag3=%d, flag4=%d, flag5=%d\n",
                flag1, flag2, flag3, flag4, flag5);

            if((flag1==1)|| ((flag2==1)&& !(flag5==1 || flag3==0)))
            {
                // Previous field is unmatched.
                // Don't display it,but let i remain for nReference if marked as such.
                loge("first field and the bottom field is not matched\n");
                if((hCtx->frmBufInf.pCurPicturePtr!= NULL) &&
                    (hCtx->frmBufInf.pCurPicturePtr->pVPicture!=NULL))
                {
                    hCtx->nNalRefIdc = hCtx->frmBufInf.pCurPicturePtr->nNalRefIdc;
                    hCtx->nSliceType = hCtx->frmBufInf.pCurPicturePtr->nPictType;
                    hCtx->nFrmNum   = hCtx->frmBufInf.pCurPicturePtr->nFrmNum;
                     if(hCtx->frmBufInf.pCurPicturePtr != NULL)
                       {
                         hCtx->frmBufInf.pCurPicturePtr->bDecErrorFlag = 1;
                       }
                    #if 0
                    hCtx->nDecFrameStatus = H264_END_DEC_FRAME;
                    return VRESULT_DEC_FRAME_ERROR;
                    #endif
                }
                #if 1  //modified by xyliu for cmcc
                hCtx->nDecFrameStatus = H264_END_DEC_FRAME;
                hCtx->bFstField = 0;
                hCtx->bLastMbInSlice = 0;
                hCtx->nCurSliceNum = 0;
                return VRESULT_DEC_FRAME_ERROR;
                #endif
            }
            else
            {
                hCtx->bFstField = 0;
                if(hCtx->nLastFrmNum != hCtx->nFrmNum)
                {
                    if(hCtx->frmBufInf.pCurPicturePtr!= NULL)
                    {
                        hCtx->nNalRefIdc = hCtx->frmBufInf.pCurPicturePtr->nNalRefIdc;
                        hCtx->nSliceType = hCtx->frmBufInf.pCurPicturePtr->nPictType;
                        hCtx->nFrmNum   = hCtx->frmBufInf.pCurPicturePtr->nFrmNum;
                        hCtx->frmBufInf.pCurPicturePtr->bDecErrorFlag = 1;
                        hCtx->frmBufInf.pCurPicturePtr->bDecSecondFieldError = 1;
                        hCtx->nDecFrameStatus = H264_END_DEC_FRAME;
                        hCtx->bLastMbInSlice = 0;
                        loge("here3: the first slice of the frame is not 0");
                        hCtx->nCurSliceNum = 0;
                        return VRESULT_DEC_FRAME_ERROR;
                    }
                }
            }
        }
        else
        {
            hCtx->bFstField = (hCtx->nPicStructure!=PICT_FRAME);
        }

        if((hCtx->nCurSliceNum==0)&& ((hCtx->nPicStructure==PICT_FRAME)||
            (hCtx->bFstField==1)))
        {
            if(H264FrameStart(h264DecCtx, hCtx) == VDECODE_RESULT_NO_FRAME_BUFFER)
                //  request frame buffer and decode the frame
            {
                //fbm_print_status(hCtx->pFbm);
                logv("request buffer failed\n");
                hCtx->nPicStructure = lastPicStructure;
                hCtx->bFstField = lastFieldFlag;
                return VDECODE_RESULT_NO_FRAME_BUFFER;
            }
        }
    }

    if(hCtx->frmBufInf.pCurPicturePtr == NULL)
    {
        return VRESULT_ERR_FAIL;
    }

    hCtx->frmBufInf.pCurPicturePtr->nFrmNum = hCtx->nFrmNum;
    hCtx->frmBufInf.pCurPicturePtr->nNalRefIdc = hCtx->nNalRefIdc;
    hCtx->frmBufInf.pCurPicturePtr->nPictType = hCtx->nSliceType;
    hCtx->frmBufInf.pCurPicturePtr->bCodedFrame = (hCtx->nPicStructure == PICT_FRAME);

    hCtx->nMbX = bFstMbInSlice % hCtx->pCurSps->nMbWidth;
    hCtx->nMbY = (bFstMbInSlice/hCtx->pCurSps->nMbWidth)<<(hCtx->bMbAffFrame);

    if(hCtx->nPicStructure == PICT_FRAME)
    {
        hCtx->nCurPicNum = hCtx->nFrmNum;
        hCtx->nMaxPicNum = 1<<hCtx->pCurSps->nLog2MaxFrmNum;

    }
    else
    {
        hCtx->nCurPicNum = 2*hCtx->nFrmNum + 1;
        hCtx->nMaxPicNum = 1<<(hCtx->pCurSps->nLog2MaxFrmNum + 1);
    }

    if(hCtx->bIdrFrmFlag == 1)
    {
        tmp = h264DecCtx->GetUeGolomb((void*)h264DecCtx);  // idr_pic_id
    }

    if(hCtx->pCurSps->nPocType==0)
    {
        hCtx->nPocLsb = h264DecCtx->GetBits((void*)h264DecCtx, hCtx->pCurSps->nLog2MaxPocLsb);

        if(hCtx->pCurPps->pPicOrderPresent==1 && hCtx->nPicStructure==PICT_FRAME)
        {
            hCtx->nDeltaPocBottom = h264DecCtx->GetSeGolomb((void*)h264DecCtx);
        }
    }
    if(hCtx->pCurSps->nPocType==1 && !hCtx->pCurSps->bDeltaPicOrderAlwaysZeroFlag)
    {
        hCtx->nDeltaPoc[0] = h264DecCtx->GetSeGolomb((void*)h264DecCtx);
        if(hCtx->pCurPps->pPicOrderPresent==1 && hCtx->nPicStructure==PICT_FRAME)
        {
            hCtx->nDeltaPoc[1] = h264DecCtx->GetSeGolomb((void*)h264DecCtx);
        }
    }

    if(hCtx->nCurSliceNum==0)
    {
        H264InitPoc(hCtx);
    }

    if(hCtx->pCurPps->pRedundatPicCntPresent)
    {
        hCtx->nRedundantPicCount = h264DecCtx->GetUeGolomb((void*)h264DecCtx);
    }

    // set defaults, might be overriden a few line later
    hCtx->nRefCount[0] = hCtx->pCurPps->nRefCount[0];
    hCtx->nRefCount[1] = hCtx->pCurPps->nRefCount[1];

    hCtx->nListCount = 0;
    hCtx->bDirectSpatialMvPred = 0;
    hCtx->bNumRefIdxActiveOverrideFlag = 0;

    if(hCtx->nSliceType==H264_P_TYPE || hCtx->nSliceType==H264_SP_TYPE ||
        hCtx->nSliceType==H264_B_TYPE)
    {
        if(hCtx->nSliceType == H264_B_TYPE)
        {
            hCtx->bDirectSpatialMvPred = h264DecCtx->GetBits((void*)h264DecCtx, 1);
        }
        hCtx->bNumRefIdxActiveOverrideFlag = h264DecCtx->GetBits((void*)h264DecCtx, 1);
        if(hCtx->bNumRefIdxActiveOverrideFlag == 1)
        {
            hCtx->nRefCount[0] = h264DecCtx->GetUeGolomb((void*)h264DecCtx)+1;

            if(hCtx->nSliceType == H264_B_TYPE)
            {
                hCtx->nRefCount[1] = h264DecCtx->GetUeGolomb((void*)h264DecCtx)+1;
            }
            if(hCtx->nRefCount[0]>32 || hCtx->nRefCount[1]>32)
            {
                //log("nReference overflow\n");
                hCtx->nRefCount[0] = 1;
                hCtx->nRefCount[1] = 1;
                logv("refcount is more than 32\n");
            }
        }
        hCtx->nListCount = (hCtx->nSliceType == H264_B_TYPE)? 2 : 1;
    }

    if(!defaultRefListDone)
    {
        H264FillDefaultRefList(hCtx, h264Dec->pMajorRefFrame, h264Dec->nDecStreamIndex);
    }

    if(H264DecodeRefPicListReordering(h264DecCtx, hCtx) < 0)
    {
        logv("reference pic list reordering error\n");
    }

    if((hCtx->nCurSliceNum==0) && ((hCtx->nPicStructure==PICT_FRAME)||
        (hCtx->bFstField==1)))
    {
        hCtx->nFrameType = nSliceType;
        H264CalculatePicturePts(hCtx);
        if((hCtx->bSkipBFrameIfDelay==1) && (hCtx->nFrmMbWidth>1920) &&
            (nSliceType == H264_B_TYPE)&&(hCtx->nNalRefIdc==0)&&
            (h264DecCtx->videoStreamInfo.bIs3DStream==0))
        {
             if(hCtx->frmBufInf.pCurPicturePtr->pScaleDownVPicture != NULL)
             {
                 pVpicture = hCtx->frmBufInf.pCurPicturePtr->pScaleDownVPicture;
             }
             else
             {
                 pVpicture = hCtx->frmBufInf.pCurPicturePtr->pVPicture;
             }

            nPicturePts = pVpicture->nPts;
            //logd("the B frame is delayed, (%lld)(%lld)", nPicturePts, hCtx->nSystemTime);
               if(hCtx->nSystemTime>=nPicturePts+100000)
               {
                  logd("the B frame is delayed, drop it, \
                    pts(%lld) current_time(%lld), diff: %lld", \
                    nPicturePts, hCtx->nSystemTime, (nPicturePts - hCtx->nSystemTime));
                  if(h264Dec->H264PerformInf.performCtrlFlag & H264_PERFORM_CALDROPFRAME)
                  {
                    h264Dec->H264PerformInf.H264PerfInfo.nDropFrameNum++;
                   }
                   if(hCtx->frmBufInf.pCurPicturePtr->pScaleDownVPicture != NULL)
                   {
                       FbmReturnBuffer(hCtx->pFbmScaledown, pVpicture, 0);
                       index = hCtx->frmBufInf.pCurPicturePtr->nDispBufIndex;
                       hCtx->frmBufInf.picture[index].pScaleDownVPicture = NULL;
                       index = hCtx->frmBufInf.pCurPicturePtr->nDecodeBufIndex;
                       hCtx->frmBufInf.picture[index].pScaleDownVPicture = NULL;
                   }
                   else
                   {
                       FbmReturnBuffer(hCtx->pFbm, pVpicture, 0);
                       index = hCtx->frmBufInf.pCurPicturePtr->nDispBufIndex;
                       hCtx->frmBufInf.picture[index].pVPicture = NULL;
                       index = hCtx->frmBufInf.pCurPicturePtr->nDecodeBufIndex;
                       hCtx->frmBufInf.picture[index].pVPicture = NULL;
                   }
                   //hCtx->frmBufInf.pCurPicturePtr = NULL;
                   hCtx->frmBufInf.picture[index].bDropBFrame = 1;
                   hCtx->bDropFrame = 1;
                   hCtx->bLastDropBFrame = 1;
                   return VRESULT_DROP_B_FRAME;
               }
        }
    }
    hCtx->bLastDropBFrame = 0;
    hCtx->vbvInfo.nValidDataPts = H264VDEC_ERROR_PTS_VALUE;

    if((hCtx->pCurPps->bWeightedPred &&
        (hCtx->nSliceType==H264_P_TYPE || hCtx->nSliceType==H264_SP_TYPE)) ||
        (hCtx->pCurPps->nWeightedBIpredIdc>0 && hCtx->nSliceType==H264_B_TYPE))
    {
        H264CongigureWeightTableRegisters(h264DecCtx, hCtx);
    }

    if(hCtx->nNalRefIdc)
    {
        H264DecodeRefPicMarking(h264DecCtx, hCtx);
    }

    if(hCtx->nSliceType!=H264_I_TYPE && hCtx->nSliceType!=H264_SI_TYPE &&
        hCtx->pCurPps->bCabac)
    {
        tmp = h264DecCtx->GetUeGolomb((void*)h264DecCtx);

        if(tmp > 2)
        {
            logv("nCabacInitIdc is %d, larger than 2\n", tmp);
        }
        hCtx->nCabacInitIdc = tmp;
    }

    hCtx->nLastQscaleDiff = 0;

    tmp = hCtx->pCurPps->nInitQp + h264DecCtx->GetSeGolomb((void*)h264DecCtx);

    if(tmp > 51)
    {
        logv("nQscale is %d, larger than 51\n", tmp);

    }
    hCtx->nQscale = tmp;

    hCtx->nChromaQp[0] = 0;
    hCtx->nChromaQp[1] = 0;

    if(hCtx->nSliceType == H264_SP_TYPE)
    {
        h264DecCtx->GetBits((void*)h264DecCtx, 1);   // sp for switch flag
    }

    if(hCtx->nSliceType == H264_SP_TYPE || hCtx->nSliceType == H264_SI_TYPE)
    {
        h264DecCtx->GetSeGolomb((void*)h264DecCtx);
    }

    hCtx->bDisableDeblockingFilter = 0;
    hCtx->nSliceAlphaC0offset = 0;
    hCtx->nSliceBetaoffset = 0;

    if(hCtx->pCurPps->bDeblockingFilterParamPresent)
    {
        hCtx->bDisableDeblockingFilter  = h264DecCtx->GetUeGolomb((void*)h264DecCtx);

        if(hCtx->bDisableDeblockingFilter != 1)
        {
            hCtx->nSliceAlphaC0offset = h264DecCtx->GetSeGolomb((void*)h264DecCtx)&0x0f;
            hCtx->nSliceBetaoffset = h264DecCtx->GetSeGolomb((void*)h264DecCtx)&0x0f;
        }
    }
    hCtx->nLastSliceType = nSliceType;

    if((hCtx->pVbv->bUseNewVeMemoryProgram == 1)&&ENABLE_MV_BUF_OPTIMIZATION_PROGRAM)
    {
        if((hCtx->nCurSliceNum==0)&& ((hCtx->nPicStructure==PICT_FRAME)||
            (hCtx->bFstField==1)))
        {

            pH264MvBufInfNode =
                   (H264MvBufInf*)FIFODequeue((FiFoQueueInst**)&(hCtx->pH264MvBufEmptyQueue));

            if(pH264MvBufInfNode == NULL)
            {
                //while()
                {
                    loge("error: cannot get the empty pH264MvBufInfNode\n");
                    u32 minCalculteNum = INIT_MV_BUF_CALCULATE_NUM;
                    index = 0;
                    int i;
                    for(i=0; i<hCtx->nH264MvBufNum; i++)
                    {
                        if(hCtx->pH264MvBufInf[i].pH264MvBufManager->nCalculateNum < minCalculteNum)
                        {
                            minCalculteNum = \
								hCtx->pH264MvBufInf[i].pH264MvBufManager->nCalculateNum;
                            index = i;
                        }
                    }
                    if(minCalculteNum == INIT_MV_BUF_CALCULATE_NUM)
                    {
                        loge("the minCalculteNum is error\n");
                        abort();
                    }

                    logd("********************index=%d\n", index);

                    H264PicInfo *pCurPicturePtr = \
                        hCtx->pH264MvBufInf[index].pH264MvBufManager->pCurPicturePtr;
                    if(pCurPicturePtr != NULL)
                    {
                        if(pCurPicturePtr->pVPicture != NULL)
                        {
                            FbmReturnBuffer(hCtx->pFbm, pCurPicturePtr->pVPicture, 0);
                        }
                        if(pCurPicturePtr->pScaleDownVPicture != NULL)
                        {
                            FbmReturnBuffer(hCtx->pFbm, pCurPicturePtr->pScaleDownVPicture, 0);
                        }
                        pCurPicturePtr->nReference = 0;
                        pCurPicturePtr->pVPicture = NULL;
                        pCurPicturePtr->pScaleDownVPicture = NULL;
                        pCurPicturePtr->bHasDispedFlag = 0;
                    }
                    hCtx->pH264MvBufInf[index].pH264MvBufManager->nCalculateNum =\
                       INIT_MV_BUF_CALCULATE_NUM;
                    FIFOEnqueue((FiFoQueueInst**)&(hCtx->pH264MvBufEmptyQueue),
                        (FiFoQueueInst*)&(hCtx->pH264MvBufInf[index]));
                    pH264MvBufInfNode =
                        (H264MvBufInf*)FIFODequeue((FiFoQueueInst**)&(hCtx->pH264MvBufEmptyQueue));
                    if(pH264MvBufInfNode == NULL)
                    {
                        loge("****************error\n");
                        abort();
                    }
                }
            }
            hCtx->frmBufInf.pCurPicturePtr->nH264MvBufIndex =
                pH264MvBufInfNode->pH264MvBufManager->nH264MvBufIndex;
            hCtx->frmBufInf.pCurPicturePtr->pTopMvColBuf =
                pH264MvBufInfNode->pH264MvBufManager->pTopMvColBuf;
            hCtx->frmBufInf.pCurPicturePtr->phyTopMvColBuf =
                pH264MvBufInfNode->pH264MvBufManager->phyTopMvColBuf;
            hCtx->frmBufInf.pCurPicturePtr->pBottomMvColBuf =
                pH264MvBufInfNode->pH264MvBufManager->pBottomMvColBuf;
            hCtx->frmBufInf.pCurPicturePtr->phyBottomMvColBuf =
                pH264MvBufInfNode->pH264MvBufManager->phyBottomMvColBuf;
            pH264MvBufInfNode->pH264MvBufManager->pCurPicturePtr = hCtx->frmBufInf.pCurPicturePtr;
            pH264MvBufInfNode->pH264MvBufManager->nCalculateNum = hCtx->nCurFrmNum;
            if(hCtx->nCurFrmNum >= 0x7fffffff)
            {
                int i;
                for(i=0; i<hCtx->nH264MvBufNum; i++)
                {
                    if(hCtx->pH264MvBufInf[i].pH264MvBufManager->nCalculateNum != \
                         INIT_MV_BUF_CALCULATE_NUM)
                    {
                        hCtx->pH264MvBufInf[i].pH264MvBufManager->nCalculateNum -= 0x7fffff00;
                    }
                }
            }
        }
    }

    hCtx->nCurSliceNum += 1;
    return VDECODE_RESULT_OK;
}

s32 H264DecodePictureScanType(H264DecCtx* h264DecCtx, H264Context* hCtx)
{
    u32 nPpsId = 0;
    u32 bFstMbInSlice = 0;
    u32 nSliceType    = 0;
    H264Dec* h264Dec = NULL;
    u32 nFrmNum = 0;
    u8 nPicStructure = 0;

    h264Dec = (H264Dec*) h264DecCtx->pH264Dec;

    bFstMbInSlice = h264DecCtx->GetUeGolomb((void*)h264DecCtx);
    CEDARC_UNUSE(bFstMbInSlice);
    nSliceType = h264DecCtx->GetUeGolomb((void*)h264DecCtx);
    if(nSliceType > 9)
    {
        logd("the slice type is invalid\n");
        return VRESULT_ERR_FAIL;
    }
    nSliceType = (nSliceType > 4)? nSliceType-5 : nSliceType;

    if(hCtx->bNeedFindIFrm == 1)
    {
        if((h264Dec->nDecStreamIndex==0) && (nSliceType!=H264_I_TYPE))
        {
            logd("need find the I frame to start decode,drop the stream!!!\n");
            return VRESULT_ERR_FAIL;
        }
        else if((h264Dec->nDecStreamIndex==1)&&(nSliceType!=H264_P_TYPE))
        {
            logd("need find the I frame to start decode,drop the stream!!!\n");
            return VRESULT_ERR_FAIL;
        }
        hCtx->bNeedFindIFrm = 0;
    }
    nPpsId = h264DecCtx->GetUeGolomb((void*)h264DecCtx);

    if(nPpsId >= 256)
    {
        logi("the ppsId is %d, larger than 256\n", nPpsId);
         if(hCtx->frmBufInf.pCurPicturePtr != NULL)
           {
             hCtx->frmBufInf.pCurPicturePtr->bDecErrorFlag = 1;
        }
        return VRESULT_ERR_FAIL;
    }

    if(hCtx->pPpsBuffers[nPpsId] == NULL)
    {
        //logd("the slice ppsid is not equal to the ppsid,
        //nPpsId=%d,  hCtx->nPpsId=%d\n", nPpsId, hCtx->nPpsId);

        logi("nPpsId=%d, hCtx->pPpsBuffers[nPpsId]=NULL\n", nPpsId);
         if(hCtx->frmBufInf.pCurPicturePtr != NULL)
          {
             hCtx->frmBufInf.pCurPicturePtr->bDecErrorFlag = 1;
           }
        return VRESULT_ERR_FAIL;
    }

    hCtx->pCurPps = hCtx->pPpsBuffers[nPpsId];
    hCtx->pCurSps = hCtx->pSpsBuffers[hCtx->pCurPps->nSpsId];

    nFrmNum = h264DecCtx->GetBits((void*)h264DecCtx, hCtx->pCurSps->nLog2MaxFrmNum);
    CEDARC_UNUSE(nFrmNum);

    if(hCtx->pCurSps->bFrameMbsOnlyFlag == 1)
    {
        nPicStructure = PICT_FRAME;
    }
    else
    {
        if(h264DecCtx->GetBits((void*)h264DecCtx, 1) == 1)
        {
            nPicStructure = PICT_TOP_FIELD+h264DecCtx->GetBits((void*)h264DecCtx, 1) ;
        }
        else
        {
            nPicStructure = PICT_FRAME;
            hCtx->bMbAffFrame = hCtx->pCurSps->bMbAff;
        }
    }

    if(hCtx->bProgressice == 0xFF)
    {
        hCtx->bProgressice = ((hCtx->pCurSps->bMbAff==0)&&(nPicStructure==PICT_FRAME));
        logd("here3:hCtx->bProgressice=%d\n", hCtx->bProgressice);
    }
    return VRESULT_REDECODE_STREAM_DATA;
}

