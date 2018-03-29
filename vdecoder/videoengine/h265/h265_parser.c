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
* File : h265_parser.c
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#include "h265_parser.h"
#include "h265_func.h"
#include "h265_memory.h"

const u32 gSigLastScanCG32x32[64] =
{
    0,  8,  1,  16, 9,  2,  24, 17,
    10, 3,  32, 25, 18, 11, 4,  40,
    33, 26, 19, 12, 5,  48, 41, 34,
    27, 20, 13, 6,  56, 49, 42, 35,
    28, 21, 14, 7,  57, 50, 43, 36,
    29, 22, 15, 58, 51, 44, 37, 30,
    23, 59, 52, 45, 38, 31, 60, 53,
    46, 39, 61, 54, 47, 62, 55, 63
};

const u32 gSigLastScan_0_1[16] =
{
    0,  4,  1,  8,
    5,  2,  12, 9,
    6,  3,  13, 10,
    7,  14, 11, 15
};

#if HEVC_PARSER_TRACE_DEBUG

#define HevcGetNBits(lenght, gb, name)     HevcGetNBitsDebug(gb, lenght, name)
#define HevcGetNBitsL(lenght, gb, name)     HevcGetNBitsLongDebug(gb, lenght, name)
#define HevcGetOneBit(gb, name)             HevcGetOneBitDebug(gb, name)
#define HevcGetUe(gb, name)                HevcGetUeDebug(gb, name)
#define HevcGetSe(gb, name)                HevcGetSeDebug(gb, name)
#define HevcSkipBits(gb, nBits, name)            HevcSkipBitsDebug(gb, nBits, name)
#define HevcCabacTrace(gb, message)        HevcCabacTraceFunc(gb, message)
#define HevcGetBitsLeft(gb)                get_bits_left(gb)
#define HevcShowBits(gb, nBits)            show_bits(gb, nBits)

static inline u32 HevcGetNBitsDebug(GetBitContext *gb, s32 nLenght, char *name)
{
    u32 nTemp = 0;
    if(gb->bUseHardware)
        nTemp = HevcGetbitsHardware(gb->RegBaseAddr, nLenght);
    else
        nTemp = get_bits(gb, nLenght);
    logd("get %d bits \t\t%s = %d \t\tindex: %d ",  nLenght, name, nTemp, gb->index);
    return nTemp;
}

static inline u32 HevcGetNBitsLongDebug(GetBitContext *gb, s32 nLenght, char *name)
{
    u32 nTemp = 0;
    if(gb->bUseHardware)
        nTemp = HevcGetbitsHardware(gb->RegBaseAddr, nLenght);
    else
        nTemp = get_bits_long(gb, nLenght);
    logd("get %d bits L \t\t%s = %d \t\tindex: %d ",  nLenght, name, nTemp, gb->index);
    return nTemp;
}

static inline u32 HevcGetOneBitDebug(GetBitContext *gb, char *name)
{
    u32 nTemp = 0;
    if(gb->bUseHardware)
        nTemp = HevcGetbitsHardware(gb->RegBaseAddr, 1);
    else
        nTemp = get_bits1(gb);
    logd("get 1 bit  \t\t%s = %d  \t\tindex: %d ", name, nTemp, gb->index);
    return nTemp;
}

static inline u32 HevcGetUeDebug(GetBitContext *gb, char *name)
{
    u32 nTemp = 0;
    if(gb->bUseHardware)
        nTemp = HevcGetUEHardware(gb->RegBaseAddr);
    else
        nTemp = get_ue_golomb_long(gb);
//    u32 nTemp = get_ue_golomb(gb);
    logd("get ue golomb \t\t%s = %d \t\tindex: %d ", name, nTemp, gb->index);
    return nTemp;
}

static inline s32 HevcGetSeDebug(GetBitContext *gb, char *name)
{
    s32 nTemp;
    if(gb->bUseHardware)
        nTemp = HevcGetSEHardware(gb->RegBaseAddr);
    else
        nTemp = get_se_golomb(gb);
//    u32 nTemp = get_se_golomb(gb);
    logd("get se golomb  \t\t %s = %d \t\tindex: %d ", name, nTemp, gb->index);
    return nTemp;
}

static inline void HevcSkipBitsDebug(GetBitContext *gb, s32 nBits, char *name)
{
    u32 nTemp = 0;
    if(gb->bUseHardware)
        nTemp = HevcGetbitsHardware(gb->RegBaseAddr, nBits);
    else
        nTemp = get_bits_long(gb, nBits);
    logd(" %s skip bits: %d \t\tindex: %d ", name, nBits, gb->index);
}

static inline void HevcCabacTraceFunc(GetBitContext *gb, char *message)
{
    /* fprintf(gb->pFileDebug, "%s\n", message); */
    logd(" %s \t\tindex: %d ", message, gb->index);
}

#else  /* HEVC_PARSER_TRACE_DEBUG */
#define HevcGetNBits(lenght, gb, name)     HevcGetNBitsNormal(gb, lenght)
#define HevcGetNBitsL(lenght, gb, name)     HevcGetNBitsLongNormal(gb, lenght)
#define HevcGetOneBit(gb, name)             HevcGetOneBitNormal(gb)
#define HevcGetUe(gb, name)                HevcGetUeNormal(gb)
#define HevcGetSe(gb, name)                HevcGetSeNormal(gb)
#define HevcSkipBits(gb, nBits, name)            HevcSkipBitsNormal(gb, nBits)
#define HevcCabacTrace(gb, message)
#define HevcGetBitsLeft(gb)                get_bits_left(gb)
#define HevcShowBits(gb, nBits)            show_bits(gb, nBits)

static inline u32 HevcGetNBitsNormal(GetBitContext *gb, s32 nLenght)
{
    u32 nRet = 0;
    if(gb->bUseHardware)
        nRet = HevcGetbitsHardware(gb->RegBaseAddr, nLenght);
    else
        nRet = get_bits(gb, nLenght);
    return nRet;
}

static inline u32 HevcGetNBitsLongNormal(GetBitContext *gb, s32 nLenght)
{
    u32 nRet = 0;
    if(gb->bUseHardware)
        nRet = HevcGetbitsHardware(gb->RegBaseAddr, nLenght);
    else
        nRet = get_bits_long(gb, nLenght);
    return nRet;
}

static inline u32 HevcGetOneBitNormal(GetBitContext *gb)
{
    u32 nRet = 0;
    if(gb->bUseHardware)
        nRet = HevcGetbitsHardware(gb->RegBaseAddr, 1);
    else
        nRet = get_bits1(gb);
    return nRet;
}

static inline u32 HevcGetUeNormal(GetBitContext *gb)
{
    u32 nRet = 0;
    if(gb->bUseHardware)
        nRet = HevcGetUEHardware(gb->RegBaseAddr);
    else
        nRet = get_ue_golomb_long(gb);
    return nRet;
//    return get_ue_golomb(gb);
}

static inline s32 HevcGetSeNormal(GetBitContext *gb)
{
    s32 nRet = 0;
    if(gb->bUseHardware)
        nRet = HevcGetSEHardware(gb->RegBaseAddr);
    else
        nRet = get_se_golomb(gb);
    return nRet;
}

static inline void HevcSkipBitsNormal(GetBitContext *gb, s32 nBits)
{
    if(gb->bUseHardware)
        HevcGetbitsHardware(gb->RegBaseAddr, nBits);
    else
        skip_bits(gb, nBits);
}

#endif  /* HEVC_PARSER_TRACE_DEBUG */

s32 HevcParserInitGetBits(GetBitContext *s, char *pBuffer, int nBitSize)
{
    int nBufferSize;
    int nRet = 0;

    if (nBitSize > HEVC_INT_MAX - 7 || nBitSize <= 0)
    {
        nBufferSize = nBitSize = 0;
        pBuffer = NULL;
        nRet = -1;
    }
    nBufferSize = (nBitSize + 7) >> 3;
    s->buffer = pBuffer;
    s->size_in_bits = nBitSize;
    s->size_in_bits_plus8 = nBitSize + 8;
    s->buffer_end = pBuffer + nBufferSize;
    s->index = 0;
    return nRet;
}

static s32 HevcMoreRbspData(GetBitContext *gb)
{
    return HevcGetBitsLeft(gb) > 0 && HevcShowBits(gb, 8) != 0x80;
}

s32 HevcParserGetNaluType(HevcContex *pHevcDec)
{
    GetBitContext *gb = &pHevcDec->getBit;
    s32  nNuhLayerId = 0;
    s32 nTemp;

    nTemp = HevcGetOneBit(gb, "forbidden_zero_bit");
    if(nTemp != 0)
    {
        loge(" h265 decoder error. forbidden_zero_bit != 0 ");
        return -1;
    }
    pHevcDec->eNaluType     = HevcGetNBits(6, gb, "nal_unit_type");
    nNuhLayerId                = HevcGetNBits(6, gb, "nuh_layer_id");
    pHevcDec->nTemporalId     = HevcGetNBits(3, gb, "nuh_temporal_id_plus1") - 1;
    pHevcDec->nNuhLayerId = nNuhLayerId;
    if(pHevcDec->nTemporalId < 0)
    {
        loge(" h265 decoder error. nuh_temporal_id_plus1 < 0 ");
        logd( "nal_unit_type: %d, nuh_layer_id: %dtemporal_id: %d",
                pHevcDec->eNaluType, nNuhLayerId, pHevcDec->nTemporalId);
        //usleep(500*1000); //debug
        return -1;
    }
//    logd( "nal_unit_type: %d, nuh_layer_id: %dtemporal_id: %d",
//            pHevcDec->eNaluType, nNuhLayerId, pHevcDec->nTemporalId);
    return nNuhLayerId == 0;

}

static s32 HevcParsePTL(HevcContex *pHevcDec, HevcPTL *pPTL, s32 nVpsMaxSubLayers)
{
    s32 i, j;
    GetBitContext *pGb;

    pGb = &pHevcDec->getBit;
    pPTL->GeneralPtl.nProfileSpace = HevcGetNBits(2, pGb, "general_profile_space");
    pPTL->GeneralPtl.bTierFlag = HevcGetOneBit(pGb, "general_tier_flag");
    pPTL->GeneralPtl.nProfileIdc = HevcGetNBits(5, pGb, "general_profile_idc");
    if(pPTL->GeneralPtl.nProfileIdc == HEVC_PROFILE_MAIN)
    {
        logv("h265 stream type: 8 bit. before = %d",pHevcDec->b10BitStreamFlag);
        if(pHevcDec->b10BitStreamFlag == 1)
        {
            logd("*** from 10bit to 8bit, call back size change");
            pHevcDec->b10BitStreamFlag = 0;
            return HEVC_RESULT_PICTURES_SIZE_CHANGE;
        }
        pHevcDec->b10BitStreamFlag = 0;

    }
    else if(pPTL->GeneralPtl.nProfileIdc == HEVC_PROFILE_MAIN_10)
    {
        logv("h265 stream type: 10 bit. before = %d",pHevcDec->b10BitStreamFlag);
        if(pHevcDec->b10BitStreamFlag == 0)
        {
            logd("*** from 8bit to 10bit , call back size change");
            pHevcDec->b10BitStreamFlag = 1;
            return HEVC_RESULT_PICTURES_SIZE_CHANGE;
        }
        pHevcDec->b10BitStreamFlag = 1;
        logv(" This is HEVC main 10 profile bitstream.");
        //return -1;
    }
    else if(pPTL->GeneralPtl.nProfileIdc == HEVC_PROFILE_MAIN_STILL_PICTURE)
    {
        pHevcDec->b10BitStreamFlag = 0;
        logd(" This is HEVC main still picture profile bitstream ");
    }
    else
    {
        pHevcDec->b10BitStreamFlag = 0;
//        logd(" There is not profile information in the bitstream ");
    }
    for(j = 0; j < 32; j++)
        pPTL->GeneralPtl.bProfileCompatibilityFlag[j] =
        HevcGetOneBit(pGb, "general_profile_compatibility_flag[j]");
    pPTL->GeneralPtl.bProgressiveSourceFlag =
        HevcGetOneBit(pGb, "general_progressive_source_flag");
    pPTL->GeneralPtl.bInterlacedSourceFlag =
        HevcGetOneBit(pGb, "general_interlaced_source_flag");
    pPTL->GeneralPtl.bNonPackedConstraintFlag =
        HevcGetOneBit(pGb, "general_non_packed_constraint_flag");
    pPTL->GeneralPtl.bFrameOnlyConstraintFlag =
        HevcGetOneBit(pGb, "general_frame_only_constraint_flag");
    if(pPTL->GeneralPtl.bFrameOnlyConstraintFlag == 0)
    {
        logv(" pPTL->GeneralPtl.bFrameOnlyConstraintFlag: %d ",
            pPTL->GeneralPtl.bFrameOnlyConstraintFlag);
    }

    HevcSkipBits(pGb, 16, "reserved_zero_44bits 16bits");
    HevcSkipBits(pGb, 16, "reserved_zero_44bits 16bits");
    HevcSkipBits(pGb, 12, "reserved_zero_44bits 12bits"); /* reserved_zero_44bits */
    pPTL->GeneralPtl.nLevelIdc = HevcGetNBits(8, pGb, "general_level_idc");

    for(i = 0; i < nVpsMaxSubLayers - 1; i++)
    {
        pPTL->bSubLayerProfilePresentFlag[i] =
            HevcGetOneBit(pGb, "sub_layer_profile_present_flag[i]");
        pPTL->bSubLayerLevelPresentFlag[i] =
            HevcGetOneBit(pGb, "sub_layer_level_present_flag[i]");
    }
    if(nVpsMaxSubLayers - 1 > 0)
        for(i = nVpsMaxSubLayers - 1; i < 8; i++)
            HevcSkipBits(pGb, 2, "reserved_zero_2bits");  /* reserved_zero_2bits */
    for(i = 0; i < nVpsMaxSubLayers - 1; i++)
    {
        if(pPTL->bSubLayerProfilePresentFlag[i])
        {
            pPTL->SubLayerPtl[i].nProfileSpace =
                HevcGetNBits(2, pGb, "sub_layer_profile_space[i]");
            pPTL->SubLayerPtl[i].bTierFlag =
                HevcGetOneBit(pGb, "sub_layer_tier_flag[i]");
            pPTL->SubLayerPtl[i].nProfileIdc =
                HevcGetNBits(5, pGb, "sub_layer_profile_idc[i]");
            for( j = 0; j < 32; j++ )
                pPTL->SubLayerPtl[i].bProfileCompatibilityFlag[j] =
                HevcGetOneBit(pGb, "general_profile_compatibility_flag[i][j]");
            pPTL->SubLayerPtl[i].bProgressiveSourceFlag =
                HevcGetOneBit(pGb, "sub_layer_progressive_source_flag[i]");
            pPTL->SubLayerPtl[i].bInterlacedSourceFlag =
                HevcGetOneBit(pGb, "sub_layer_interlaced_source_flag[i]");
            pPTL->SubLayerPtl[i].bNonPackedConstraintFlag =
                HevcGetOneBit(pGb, "sub_layer_non_packed_constraint_flag[i]");
            pPTL->SubLayerPtl[i].bFrameOnlyConstraintFlag =
                HevcGetOneBit(pGb, "sub_layer_frame_only_constraint_flag[i]");
            if(pPTL->SubLayerPtl[i].bFrameOnlyConstraintFlag == 0)
            {
                loge("pPTL->SubLayerPtl[%d].bFrameOnlyConstraintFlag: %d",
                    i, pPTL->SubLayerPtl[i].bFrameOnlyConstraintFlag);
            }
            HevcSkipBits(pGb, 16, "sub_layer_reserved_zero_44bits[i] 16 bits");
            HevcSkipBits(pGb, 16, "sub_layer_reserved_zero_44bits[i] 16 bits");
            HevcSkipBits(pGb, 12, "sub_layer_reserved_zero_44bits[i] 12 bits");
            /* sub_layer_reserved_zero_44bits[i] */
        }
        if(pPTL->bSubLayerLevelPresentFlag[i])
            pPTL->SubLayerPtl[i].nLevelIdc =
            HevcGetNBits(8, pGb, "sub_layer_level_idc[i]");
    }
    return 0;
}

static void HevcParseSubLayerHrd(HevcContex *pHevcDec, s32 nNbCpb, s32 bSubpicParamPresent)
{
    GetBitContext *pGb = &pHevcDec->getBit;
    s32 i;
    for( i = 0; i  <=  nNbCpb; i++ )
    {
        HevcGetUe(pGb, "bit_rate_value_minus1[i]");
        HevcGetUe(pGb, "cpb_size_value_minus1[i]");
        if(bSubpicParamPresent)
        {
            HevcGetUe(pGb, "cpb_size_du_value_minus1[i]");
            HevcGetUe(pGb, "bit_rate_du_value_minus1[i]");
        }
        HevcGetOneBit(pGb, "cbr_flag[i]");
    }
}

static s32 HevcParseHRD(HevcContex *pHevcDec, s32 bCprmsPresentFlag, s32 nMaxSubLayers)
{
    GetBitContext *pGb = &pHevcDec->getBit;
    s32 bNalParamPresent = 0, bVclParamPresent = 0;
    s32 bSubpicParamPresent = 0;
    s32 i;

    if(bCprmsPresentFlag)
    {
        bNalParamPresent = HevcGetOneBit(pGb, "nal_hrd_parameters_present_flag");
        bVclParamPresent = HevcGetOneBit(pGb, "vcl_hrd_parameters_present_flag");
        if(bNalParamPresent || bVclParamPresent)
        {
            bSubpicParamPresent = HevcGetOneBit(pGb, "sub_pic_hrd_params_present_flag");
            if(bSubpicParamPresent)
            {
                HevcGetNBits(8, pGb, "tick_divisor_minus2");
                HevcGetNBits(5, pGb, "du_cpb_removal_delay_increment_length_minus1");
                HevcGetOneBit(pGb, "sub_pic_cpb_params_in_pic_timing_sei_flag");
                HevcGetNBits(5, pGb, "dpb_output_delay_du_length_minus1");
            }
            HevcGetNBits(4, pGb, "bit_rate_scale");
            HevcGetNBits(4, pGb, "cpb_size_scale");
            if(bSubpicParamPresent)
                HevcGetNBits(4, pGb, "cpb_size_du_scale");
            HevcGetNBits(5, pGb, "initial_cpb_removal_delay_length_minus1");
            HevcGetNBits(5, pGb, "au_cpb_removal_delay_length_minus1");
            HevcGetNBits(5, pGb, "dpb_output_delay_length_minus1");
        }
    }
    for( i = 0; i  <=  nMaxSubLayers - 1; i++ )
    {
        s32 bLowDelay = 0, bFixedRate = 0;
        u32 nNbCpb = 0;
        bFixedRate = HevcGetOneBit(pGb, "fixed_pic_rate_general_flag[i]");
        if(!bFixedRate)
            bFixedRate = HevcGetOneBit(pGb, "fixed_pic_rate_within_cvs_flag[i]");
           if(bFixedRate)
            bFixedRate = HevcGetUe(pGb, "elemental_duration_in_tc_minus1[i]");
           else
               bLowDelay = HevcGetOneBit(pGb, "low_delay_hrd_flag[i]");
           if(!bLowDelay)
               nNbCpb = HevcGetUe(pGb, "cpb_cnt_minus1[i]");
           if(bNalParamPresent)
               HevcParseSubLayerHrd(pHevcDec, nNbCpb, bSubpicParamPresent);
           if(bVclParamPresent)
               HevcParseSubLayerHrd(pHevcDec, nNbCpb, bSubpicParamPresent);
    }
    return 0;
}

static s32 HevcParseShortTermRefSet(HevcContex *pHevcDec,
        HevcShortTermRPS *pRps, HevcSPS *pSps, s32 bIsSliceHeader)
{
    GetBitContext *pGb = &pHevcDec->getBit;
    s32 bRpsPredict, nDeltaPoc;
    s32 i, k, k0, k1;

    bRpsPredict = 0;
    k = 0;
    k0 = 0;
    k1 = 0;
    if(pRps != pSps->StRps && pSps->nNumStRps)
        bRpsPredict = HevcGetOneBit(pGb, "inter_ref_pic_set_prediction_flag");
    if(bRpsPredict)
    {
        const HevcShortTermRPS *pRpsRidx;
        s32 nDeltaRps, nAbsDeltaRps;
        s32 bUseDeltaFlag = 0;
        s32 bDeltaRpsSign;
        if(bIsSliceHeader)
        {
            u32 nDeltaIdx = HevcGetUe(pGb, "delta_idx_minus1") + 1;
            if(nDeltaIdx > pSps->nNumStRps)
            {
                loge(" h265 decode short rps error. In slice header");
                return -1;
            }
            pRpsRidx = &pSps->StRps[pSps->nNumStRps - nDeltaIdx];
        }
        else
            pRpsRidx = &pSps->StRps[pRps - pSps->StRps - 1];

        bDeltaRpsSign = HevcGetOneBit(pGb, "delta_rps_sign");
        nAbsDeltaRps = HevcGetUe(pGb, "abs_delta_rps_minus1") + 1;
        nDeltaRps = (1 - (bDeltaRpsSign << 1)) * nAbsDeltaRps;
        for(i = 0; i <= pRpsRidx->nNumDeltaPocs; i++)
        {
            s32 bUsed = HevcGetOneBit(pGb, "used_by_curr_pic_flag[j]");
            pRps->bUsed[k] = bUsed;
            if(!bUsed)
                bUseDeltaFlag = HevcGetOneBit(pGb, "use_delta_flag[j]");
            if(bUsed || bUseDeltaFlag)
            {
                if(i < pRpsRidx->nNumDeltaPocs)
                    nDeltaPoc = nDeltaRps + pRpsRidx->nDeltaPoc[i];
                else
                    nDeltaPoc = nDeltaRps;
                pRps->nDeltaPoc[k] = nDeltaPoc;
                if(nDeltaPoc < 0)
                    k0++;
                else
                    k1++;
                k++;
            }
        }
        pRps->nNumDeltaPocs = k;
        pRps->nNumNegativePics = k0;
        // sort in increasing order (smallest first)
        if(pRps->nNumDeltaPocs != 0)
        {
            s32 bUsed, nTemp;
            for(i = 1; i < pRps->nNumDeltaPocs; i++)
            {
                nDeltaPoc = pRps->nDeltaPoc[i];
                bUsed = pRps->bUsed[i];
                for(k = i - 1; k >= 0; k--)
                {
                    nTemp = pRps->nDeltaPoc[k];
                    if(nDeltaPoc < nTemp)
                    {
                        pRps->nDeltaPoc[k + 1] = nTemp;
                        pRps->bUsed[k + 1] = pRps->bUsed[k];
                        pRps->nDeltaPoc[k] = nDeltaPoc;
                        pRps->bUsed[k] = bUsed;

                    }
                }
            }
        }
        if((pRps->nNumNegativePics >> 1) != 0)
        {
            s32 bUsed;
            k = pRps->nNumNegativePics - 1;
            // flip the negative values to largest first
            for(i = 0; i < pRps->nNumNegativePics >> 1; i++)
            {
                nDeltaPoc = pRps->nDeltaPoc[i];
                bUsed = pRps->bUsed[i];
                pRps->nDeltaPoc[i] = pRps->nDeltaPoc[k];
                pRps->bUsed[i] = pRps->bUsed[k];
                pRps->nDeltaPoc[k] = nDeltaPoc;
                pRps->bUsed[k] = bUsed;
                k--;
            }
        }
    }
    else /* if(bRpsPredict) */
    {
        s32 nPrev, nNumPositivePics;
        pRps->nNumNegativePics = HevcGetUe(pGb, "num_negative_pics");
        nNumPositivePics = HevcGetUe(pGb, "num_positive_pics");
        if(pRps->nNumNegativePics >= HEVC_MAX_REFS ||
                nNumPositivePics >= HEVC_MAX_REFS )
        {
            loge("h265 decode short rps error. Short term ref is out of range. \
                positive: %d, negative: %d",
                nNumPositivePics, pRps->nNumNegativePics);
            return -1;
        }
        pRps->nNumDeltaPocs = pRps->nNumNegativePics + nNumPositivePics;
        if(pRps->nNumDeltaPocs)
        {
            nPrev = 0;
            for(i = 0; i < pRps->nNumNegativePics; i++)
            {
                nDeltaPoc = HevcGetUe(pGb, "delta_poc_s0_minus1[i]") + 1;
                nPrev -= nDeltaPoc;
                pRps->nDeltaPoc[i] = nPrev;
                pRps->bUsed[i] = HevcGetOneBit(pGb, "used_by_curr_pic_s0_flag[i]");
            }
            nPrev = 0;
            for(i = 0; i < nNumPositivePics; i++)
            {
                nDeltaPoc = HevcGetUe(pGb, "delta_poc_s1_minus1[i]") + 1;
                nPrev += nDeltaPoc;
                pRps->nDeltaPoc[pRps->nNumNegativePics + i] = nPrev;
                pRps->bUsed[pRps->nNumNegativePics + i] =
                    HevcGetOneBit(pGb, "used_by_curr_pic_s1_flag[i]");
            }
        }
    }

    return 0;
}

static s32 HevcParseLongTermRefSet(HevcContex *pHevcDec, HevcLongTermRPS *pLt)
{
    GetBitContext *pGb = &pHevcDec->getBit;
    HevcSPS *pSps = pHevcDec->pSps;
    s32 nMaxPocLsb = (1 << pSps->nLog2MaxPocLsb);
    s32 nPrvDeltaMsb = 0;
    s32 nNumOfLtSps = 0;
    s32 nNumOfLtSh = 0;
    s32 nDeltaPocMsbCycleLt = 0;
    s32 i, nTemp;

    if(!pHevcDec->pSps->bLongtermRefPicsPresentFlag)
        return 0;
    if(pSps->nNumLongTermRefPicsSps > 0)
        nNumOfLtSps = HevcGetUe(pGb, "num_long_term_sps");
    nNumOfLtSh = HevcGetUe(pGb, "num_long_term_pics");
    if(nNumOfLtSps + nNumOfLtSh > 32)
    {
        loge(" Slice Header Long term rps error ");
        return -1;
    }
    pLt->nNumOfRefs = nNumOfLtSps + nNumOfLtSh;
    for (i = 0; i < pLt->nNumOfRefs; i++)
    {
        s32 bDeltaPocMsbPresent;
        if(i < nNumOfLtSps)
        {
            s32 nLtIdxSps = 0;
            if(pSps->nNumLongTermRefPicsSps > 1)
            {
                nTemp = HevcCeilLog2(pSps->nNumLongTermRefPicsSps);
                nLtIdxSps = HevcGetNBitsL(nTemp, pGb, "lt_idx_sps[i]");
            }
            pLt->nPoc[i] = pSps->nLtRefPicPocLsbSps[nLtIdxSps];
            pLt->bUsed[i] = pSps->bUsedByCurrPicLtSpsFlag[nLtIdxSps];
        }
        else
        {
            pLt->nPoc[i] = HevcGetNBitsL(pSps->nLog2MaxPocLsb, pGb, "poc_lsb_lt[i]");
            pLt->bUsed[i] = HevcGetOneBit(pGb, "used_by_curr_pic_s0_flag[i]");
        }
        bDeltaPocMsbPresent = HevcGetOneBit(pGb, "delta_poc_msb_present_flag[i]");
        if(bDeltaPocMsbPresent)
        {
            nDeltaPocMsbCycleLt = HevcGetUe(pGb, "delta_poc_msb_cycle_lt[i]");
            if(i && i != nNumOfLtSps)
                nDeltaPocMsbCycleLt += nPrvDeltaMsb;
            pLt->nPoc[i] += pHevcDec->nPoc -
                nDeltaPocMsbCycleLt * nMaxPocLsb - pHevcDec->SliceHeader.nPicOrderCntLsb;
        }
        else
        {
            /*  reset nPrvDeltaMsb for first LTRP from slice header if MSB not present*/
            if(i == (pLt->nNumOfRefs - nNumOfLtSh))
                nDeltaPocMsbCycleLt = 0;
        }
        nPrvDeltaMsb = nDeltaPocMsbCycleLt;
    }
    return 0;
}

static void HevcParseVuiParameters(HevcContex *pHevcDec, HevcSPS *pSps)
{
    GetBitContext *pGb = &pHevcDec->getBit;
    HevcVUI *pVui = &pSps->vui;
    s32 bSarPresent;

    bSarPresent = HevcGetOneBit(pGb, "aspect_ratio_info_present_flag");
    if(bSarPresent)
    {
        s32 nAspectRatioIdc;
        nAspectRatioIdc = HevcGetNBits(8, pGb, "aspect_ratio_idc");
        if(nAspectRatioIdc == HEVC_EXTENDED_SAR)
        {
            pVui->Sar.num = HevcGetNBits(16, pGb, "sar_width");
            pVui->Sar.den = HevcGetNBits(16, pGb, "sar_height");
        }
        else
            pVui->Sar = HevcVuiSar[nAspectRatioIdc];
    }
    pVui->bOverscanInfoPresentFlag = HevcGetOneBit(pGb, "overscan_info_present_flag");
    if(pVui->bOverscanInfoPresentFlag)
        pVui->bOverScanAppropriteFlag = HevcGetOneBit(pGb, "overscan_appropriate_flag");
    pVui->bVideoSignalTypePresentFlag = HevcGetOneBit(pGb, "video_signal_type_present_flag");
    if(pVui->bVideoSignalTypePresentFlag)
    {
        pVui->nVideoFormat = HevcGetNBits(3, pGb, "video_format");
        pVui->bVideoFullRangeFlag = HevcGetOneBit(pGb, "video_full_range_flag");
        pVui->bColourDescriptionPresentFlag = HevcGetOneBit(pGb, "colour_description_present_flag");
        /*
        if (vui->video_full_range_flag && sps->pix_fmt == AV_PIX_FMT_YUV420P)
            sps->pix_fmt = AV_PIX_FMT_YUVJ420P;
         * */
        if(pVui->bColourDescriptionPresentFlag)
        {
            pVui->nColourPrimaries = HevcGetNBits(8, pGb, "colour_primaries");
            pVui->nTransferCharacteristic = HevcGetNBits(8, pGb, "transfer_characteristics");
            pVui->nMatrixCoeffs = HevcGetNBits(8, pGb, "matrix_coeffs");
        }
    }
    pVui->bChromaLocInfoPresentFlag = HevcGetOneBit(pGb, "chroma_loc_info_present_flag");
    if(pVui->bChromaLocInfoPresentFlag)
    {
        pVui->nChromaSampleLocTypeTopField =
            HevcGetUe(pGb, "chroma_sample_loc_type_top_field");
        pVui->nChromaSampleLocTypeBottomField =
            HevcGetUe(pGb, "chroma_sample_loc_type_bottom_field");
    }
    pVui->bNeutraChromaIndicationFlag =
        HevcGetOneBit(pGb, "neutral_chroma_indication_flag");
    pVui->bFieldSeqFlag = HevcGetOneBit(pGb, "field_seq_flag");
    pVui->bFrameFieldInfoPresentFlag =
        HevcGetOneBit(pGb, "frame_field_info_present_flag");
    pVui->bDefaultDisplayWindowFlag =
        HevcGetOneBit(pGb, "default_display_window_flag");
    pVui->DefDispWin.bOffsetAvailable = pVui->bDefaultDisplayWindowFlag;
    if(pVui->bDefaultDisplayWindowFlag)
    {
        pVui->DefDispWin.nLeftOffset = HevcGetUe(pGb, "def_disp_win_left_offset") * 2;
        pVui->DefDispWin.nRightOffset = HevcGetUe(pGb, "def_disp_win_right_offset") * 2;
        pVui->DefDispWin.nTopOffset = HevcGetUe(pGb, "def_disp_win_top_offset") * 2;
        pVui->DefDispWin.nBottomOffset = HevcGetUe(pGb, "def_disp_win_bottom_offset") * 2;
        logv(" picture vui offset. top: %d, bottom: %d, left: %d, right: %d ",
                pVui->DefDispWin.nTopOffset, pVui->DefDispWin.nBottomOffset,
                pVui->DefDispWin.nLeftOffset, pVui->DefDispWin.nRightOffset);
    }
    pVui->bVuiTimingInfoPresentFlag = HevcGetOneBit(pGb, "vui_timing_info_present_flag");
    if(pVui->bVuiTimingInfoPresentFlag)
    {
        pVui->nVuiNumUnitsInTick = HevcGetNBitsL(32, pGb, "vui_num_units_in_tick");
        pVui->nVuiTimeScale = HevcGetNBitsL(32, pGb, "vui_time_scale");
        pVui->bVuiPocProportionalToTimingFlag =
            HevcGetOneBit(pGb, "vui_poc_proportional_to_timing_flag");
        if(pVui->bVuiPocProportionalToTimingFlag)
            pVui->nVuiNumTicksPocDiffOneMinus1 =
            HevcGetUe(pGb, "vui_num_ticks_poc_diff_one_minus1");
        pVui->bVuiHrdParametersPresentFlag =
            HevcGetOneBit(pGb, "vui_hrd_parameters_present_flag");
        if(pVui->bVuiHrdParametersPresentFlag)
            HevcParseHRD(pHevcDec, 1, pSps->nMaxSubLayers);
    }
    pVui->bBitStreamRestrictionFlag =
        HevcGetOneBit(pGb, "bitstream_restriction_flag");
    if(pVui->bBitStreamRestrictionFlag)
    {
        pVui->bTilesFixedStructureFlag =
            HevcGetOneBit(pGb, "tiles_fixed_structure_flag");
        pVui->bMotionVectorsOverPicBoundariesFlag =
            HevcGetOneBit(pGb, "motion_vectors_over_pic_boundaries_flag");
        pVui->bRestrictedRefPicListsFlag =
            HevcGetOneBit(pGb, "restricted_ref_pic_lists_flag");

        pVui->nMinSpatialSegmentationIdc =
            HevcGetUe(pGb, "min_spatial_segmentation_idc");
        pVui->nMaxBytesPerPicDenom =
            HevcGetUe(pGb, "max_bytes_per_pic_denom");
        pVui->nMaxBitsPerMinCuDenom =
            HevcGetUe(pGb, "max_bits_per_min_cu_denom");
        pVui->nLog2MaxMvLengthHorizontal =
            HevcGetUe(pGb, "log2_max_mv_length_horizontal");
        pVui->nLog2MaxMvLengthVertical =
            HevcGetUe(pGb, "log2_max_mv_length_vertical");
    }
}

static void HevcScalingListDefaultData(HevcScalingList *pSl)
{
    s32 i, j;
    for(i = 0; i < 6; i++)
    {
        // 4x4 default is 16
        memset(pSl->Sl[0][i], 16, 16);
        pSl->SlDc[0][i] = 16; // default for 16x16
        pSl->SlDc[1][i] = 16; // default for 32x32
    }
    memcpy(pSl->Sl[1][0], DefaultScalingListIntra, 64);
    memcpy(pSl->Sl[1][1], DefaultScalingListIntra, 64);
    memcpy(pSl->Sl[1][2], DefaultScalingListIntra, 64);
    memcpy(pSl->Sl[1][3], DefaultScalingListInter, 64);
    memcpy(pSl->Sl[1][4], DefaultScalingListInter, 64);
    memcpy(pSl->Sl[1][5], DefaultScalingListInter, 64);
    memcpy(pSl->Sl[2][0], DefaultScalingListIntra, 64);
    memcpy(pSl->Sl[2][1], DefaultScalingListIntra, 64);
    memcpy(pSl->Sl[2][2], DefaultScalingListIntra, 64);
    memcpy(pSl->Sl[2][3], DefaultScalingListInter, 64);
    memcpy(pSl->Sl[2][4], DefaultScalingListInter, 64);
    memcpy(pSl->Sl[2][5], DefaultScalingListInter, 64);
    memcpy(pSl->Sl[3][0], DefaultScalingListIntra, 64);
    memcpy(pSl->Sl[3][1], DefaultScalingListInter, 64);

    for(i = 0; i < 4; i++)
        for(j = 0; j < (i == 3 ? 2 : 6); j++)
            pSl->nRefMatrixId[i][j] = j;
/**/
}

static s32 HevcParseScalingListData(HevcContex *pHevcDec, HevcScalingList *pSl)
{
    GetBitContext *pGb = &pHevcDec->getBit;
    u8 bScalingListPredModeFlag[4][6];
    s32 ScalingListDcCoef[2][6];
    s32 i, nSizeId, nMatrixId;
    u32 nPos;

    pSl->bScalingListMatrixModified = 0;
    for(nSizeId = 0; nSizeId < 4; nSizeId++)
        for(nMatrixId = 0; nMatrixId < (nSizeId == 3 ? 2 : 6); nMatrixId++)
        {
            bScalingListPredModeFlag[nSizeId][nMatrixId] =
                HevcGetOneBit(pGb, "scaling_list_pred_mode_flag[sizeId][matrixId]");
            logv("  bScalingListPredModeFlag[%d][%d] = %d ",
                    nSizeId, nMatrixId,
                    bScalingListPredModeFlag[nSizeId][nMatrixId]);
            if(!bScalingListPredModeFlag[nSizeId][nMatrixId])
            {
                s32 nDelta =
                    HevcGetUe(pGb, "scaling_list_pred_matrix_id_delta[sizeId][matrixId]");
                /* Only need to handle non-zero delta. Zero means default,
                 * which should already be in the arrays. */
                if(nDelta)
                {
                    // Copy from previous array.
                    if(nMatrixId < nDelta)
                    {
                        /* may be data error */
                        loge(" h265 decode scaling list error. invalid data: %d", nDelta);
                        nDelta = (nMatrixId - 1) >= 0 ? (nMatrixId - 1) : 0;
                    }
                    pSl->nRefMatrixId[nSizeId][nMatrixId] = nMatrixId - nDelta;
                    memcpy(pSl->Sl[nSizeId][nMatrixId],
                        pSl->Sl[nSizeId][nMatrixId - nDelta], nSizeId > 0 ? 64 : 16);
                    if(nSizeId > 1)
                        pSl->SlDc[nSizeId - 2][nMatrixId] =
                            pSl->SlDc[nSizeId - 2][nMatrixId - nDelta];
                }
            }
            else
            {
                s32 nNextCoef, nCoefNum, nScalingListDeltaCoef;
                const u32 *Scan  = (nSizeId == 0) ? gSigLastScan_0_1 :  gSigLastScanCG32x32;
                nNextCoef = 8;
                nCoefNum = HEVCMIN(64, 1 << (4 + (nSizeId << 1)));
                if(nSizeId > 1)
                {
                    ScalingListDcCoef[nSizeId - 2][nMatrixId] =
                        HevcGetSe(pGb, "scaling_list_dc_coef_minus8[sizeId − 2][matrixId]") + 8;
                    nNextCoef = ScalingListDcCoef[nSizeId - 2][nMatrixId];
                    pSl->SlDc[nSizeId - 2][nMatrixId] = nNextCoef;
                }
                for(i = 0; i < nCoefNum; i++)
                {
                    nPos = Scan[i];
                    nScalingListDeltaCoef = HevcGetSe(pGb, "scaling_list_delta_coef");
                    nNextCoef = (nNextCoef + nScalingListDeltaCoef + 256) % 256;
                    pSl->Sl[nSizeId][nMatrixId][nPos] = nNextCoef;
                }
                pSl->bScalingListMatrixModified = 1;
            }
        }

    return 0;
}

static s32 HevcComputeNumOfRefs(HevcContex *pHevcDec)
{
    s32 i, nRet = 0;
    HevcShortTermRPS *pSrps = pHevcDec->SliceHeader.pShortTermRps;
    HevcLongTermRPS *pLrps = &pHevcDec->SliceHeader.LongTermRps;

    if(pHevcDec->SliceHeader.eSliceType == HEVC_I_SLICE)
    {
        logd(" HevcComputeNumOfRefs() I_SLICE ");
        return 0;
    }
//    logd(" HevcComputeNumOfRefs(). pSrps->nNumDeltaPocs = %d pSrps->nNumNegativePics= %d",
//            pSrps->nNumDeltaPocs, pSrps->nNumNegativePics);
    if(pSrps)
    {
        for(i = 0; i < pSrps->nNumNegativePics; i++)
            nRet += !!pSrps->bUsed[i];
        for( ; i < pSrps->nNumDeltaPocs; i++)
            nRet += !!pSrps->bUsed[i];
    }
    if(pLrps)
    {
        for(i = 0; i < pLrps->nNumOfRefs; i++)
            nRet += !!pLrps->bUsed[i];
    }

    return nRet;
}

static void HevcInitWpScaling(HevcSliceHeader *pSh, s32 bitDepthY, s32 bitDepthC)
{
    s32 e, i, yuv;
    s32 bitDepth;

    for(e = 0; e < 2; e++)
    {
        for (i = 0; i < HEVC_MAX_REFS; i++)
        {
            for (yuv = 0; yuv < 3; yuv++)
            {
                WpScalingParam  *pwp = &(pSh->m_weightPredTable[e][i][yuv]);
                if (!pwp->bPresentFlag)
                {
                    // Inferring values not present :
                    pwp->iWeight = (1 << pwp->uiLog2WeightDenom);
                    pwp->iOffset = 0;
                }
                pwp->w      = pwp->iWeight;
                bitDepth = yuv ? bitDepthC : bitDepthY; // from sps
                pwp->o      = pwp->iOffset << (bitDepth-8);
                pwp->shift  = pwp->uiLog2WeightDenom;
                pwp->round  = (pwp->uiLog2WeightDenom>=1) ? (1 << (pwp->uiLog2WeightDenom-1)) : (0);
            }
        }
    }
}

static void HevcParseWeightTable2(HevcContex *pHevcDec)
{
    GetBitContext *pGb = &pHevcDec->getBit;
    HevcSliceHeader *pSh = &pHevcDec->SliceHeader;
    WpScalingParam  *wp;
    u32            bChroma     = 1; // color always present in HEVC ?
    HevcSliceType  eSliceType  = pSh->eSliceType;
    s32            iNbRef      = (eSliceType == HEVC_B_SLICE ) ? (2) : (1);
    u32            uiLog2WeightDenomLuma, uiLog2WeightDenomChroma;
    u32            uiTotalSignalledWeightFlags = 0;
    s32            iDeltaDenom;
    s32            iNumRef;

    uiLog2WeightDenomChroma = 0;
    HevcCabacTrace(pGb, "----- decode weight table start -----");
    uiLog2WeightDenomLuma = HevcGetUe(pGb, "luma_log2_weight_denom");
    pSh->nLumaLog2WeightDenom = uiLog2WeightDenomLuma;
    if(bChroma)
    {
        iDeltaDenom = HevcGetSe(pGb, "delta_chroma_log2_weight_denom");
        uiLog2WeightDenomChroma = (u32)(iDeltaDenom + uiLog2WeightDenomLuma);
        pSh->nChromaLog2WeightDenom = uiLog2WeightDenomChroma;
    }

    for (iNumRef = 0; iNumRef < iNbRef; iNumRef++)
    {
        s32  eRefPicList = (iNumRef ? 1 : 0);
        s32 iRefIdx;
        for (iRefIdx=0 ; iRefIdx < pSh->nNumOfRefs[eRefPicList]; iRefIdx++)
        {
            wp = pSh->m_weightPredTable[eRefPicList][iRefIdx];
            wp[0].uiLog2WeightDenom = uiLog2WeightDenomLuma;
            wp[1].uiLog2WeightDenom = uiLog2WeightDenomChroma;
            wp[2].uiLog2WeightDenom = uiLog2WeightDenomChroma;
            wp[0].bPresentFlag = HevcGetOneBit(pGb, "luma_weight_lx_flag[i]");
            uiTotalSignalledWeightFlags += wp[0].bPresentFlag;
        }
        if (bChroma)
        {
            u32  uiCode;
            s32  iRefIdx;
            for ( iRefIdx=0; iRefIdx < pSh->nNumOfRefs[eRefPicList]; iRefIdx++ )
            {
                wp = pSh->m_weightPredTable[eRefPicList][iRefIdx];

                uiCode = HevcGetOneBit(pGb, "chroma_weight_l0_flag[i]");
                wp[1].bPresentFlag = ( uiCode == 1 );
                wp[2].bPresentFlag = ( uiCode == 1 );
                uiTotalSignalledWeightFlags += 2*wp[1].bPresentFlag;
            }
        }
        for(iRefIdx=0 ; iRefIdx < pSh->nNumOfRefs[eRefPicList]; iRefIdx++)
        {
            wp = pSh->m_weightPredTable[eRefPicList][iRefIdx];
            if(wp[0].bPresentFlag )
            {
                s32 iDeltaWeight;
                iDeltaWeight = HevcGetSe(pGb, "delta_luma_weight_l0[i]"); // delta_luma_weight_lX
                pSh->hevc_weight_buf[iNumRef][0][iRefIdx] = iDeltaWeight & 0xff;
                wp[0].iWeight = (iDeltaWeight + (1<<wp[0].uiLog2WeightDenom));
                wp[0].iOffset =HevcGetSe(pGb, "luma_offset_l0[i]");; // luma_offset_lX
            }
            else
            {
                wp[0].iWeight = (1 << wp[0].uiLog2WeightDenom);
                wp[0].iOffset = 0;
                pSh->hevc_weight_buf[iNumRef][0][iRefIdx] = 0;
            }
            if (bChroma)
            {
                s32 j;
                s32 iDeltaWeight, iDeltaChroma, pred;
                if(wp[1].bPresentFlag)
                {
                    for(j=1 ; j<3 ; j++ )
                    {
                        iDeltaWeight = HevcGetSe(pGb, "delta_chroma_weight_lx[i][j]");
                        pSh->hevc_weight_buf[iNumRef][j][iRefIdx] = iDeltaWeight & 0xff;
                        wp[j].iWeight = (iDeltaWeight + (1<<wp[1].uiLog2WeightDenom));
                        iDeltaChroma = HevcGetSe(pGb, "delta_chroma_offset_lx[i][j]");
                        // delta_chroma_offset_lX
                        pred = (128 - ((128*wp[j].iWeight)>>(wp[j].uiLog2WeightDenom)));
                        wp[j].iOffset = HevcClip((iDeltaChroma + pred), -128, 127);
                    }
                }
                else
                {
                    for (j=1; j<3; j++ )
                    {
                        wp[j].iWeight = (1 << wp[j].uiLog2WeightDenom);
                        wp[j].iOffset = 0;
                        pSh->hevc_weight_buf[iNumRef][j][iRefIdx] = 0;
                    }
                }
            }
        }
        for (iRefIdx = pSh->nNumOfRefs[eRefPicList]; iRefIdx < HEVC_MAX_REFS; iRefIdx++)
        {
            wp = pSh->m_weightPredTable[eRefPicList][iRefIdx];
            wp[0].bPresentFlag = 0;
            wp[1].bPresentFlag = 0;
            wp[2].bPresentFlag = 0;
        }
    }
    CEDARC_UNUSE(uiTotalSignalledWeightFlags);
    HevcCabacTrace(pGb, "----- decode weight table end -----");
}

#if 0
static void HevcParseWeightTable(HevcContex *pHevcDec)
{
    GetBitContext *pGb;
    HevcSliceHeader *pSh;
    s32 i, j, nTemp;
    u8 bLumaWeightL0Flag[16];
    u8 bChromaWeightL0Flag[16];
    u8 bLumaWeightL1Flag[16];
    u8 bChromaWeightL1Flag[16];

    pGb = &pHevcDec->getBit;
    pSh = &pHevcDec->SliceHeader;

    pSh->nLumaLog2WeightDenom = HevcGetUe(pGb, "luma_log2_weight_denom");
    if(pHevcDec->pSps->nChromaFormatIdc != 0)
    {
        s32 nDelta = HevcGetSe(pGb, "delta_chroma_log2_weight_denom");
        pSh->nChromaLog2WeightDenom = HevcClip(pSh->nLumaLog2WeightDenom + nDelta, 0, 7);
    }
    for(i = 0; i < pSh->nNumOfRefs[0]; i++)
    {
        bLumaWeightL0Flag[i] = HevcGetOneBit(pGb, "luma_weight_l0_flag[i]");
        if(!bLumaWeightL0Flag[i])
        {
            pSh->LumaWeightL0[i] = 1 << pSh->nLumaLog2WeightDenom;
            pSh->LumaOffsetL0[i] = 0;
        }
    }
    if(pHevcDec->pSps->nChromaFormatIdc != 0)
    {
        for(i = 0; i < pSh->nNumOfRefs[0]; i++)
            bChromaWeightL0Flag[i] = HevcGetOneBit(pGb, "chroma_weight_l0_flag[i]");
    }
    else
    {
        for(i = 0; i < pSh->nNumOfRefs[0]; i++)
            bChromaWeightL0Flag[i] = 0;
    }
    for(i = 0; i < pSh->nNumOfRefs[0]; i++)
    {
        if(bLumaWeightL0Flag[i])
        {
            s32 nDeltaLumaWeightL0 = HevcGetSe(pGb, "delta_luma_weight_l0[i]");
            pSh->LumaWeightL0[i] = (1 << pSh->nLumaLog2WeightDenom) + nDeltaLumaWeightL0;
            pSh->LumaOffsetL0[i] = HevcGetSe(pGb, "luma_offset_l0[i]");
        }
        if(bChromaWeightL0Flag[i])
        {
            for(j = 0; j < 2; j++)
            {
                s32 nDeltaChromaWeightL0 = HevcGetSe(pGb, "delta_chroma_weight_l0[i][j]");
                s32 nDeltaChromaOffsetL0 = HevcGetSe(pGb, "delta_chroma_offset_l0[i][j]");
                pSh->ChromaWeightL0[i][j]
                    = (1 << pSh->nChromaLog2WeightDenom) + nDeltaChromaWeightL0;
                nTemp = (nDeltaChromaOffsetL0 - ((128 * pSh->ChromaWeightL0[i][j]) >>
                    pSh->nChromaLog2WeightDenom) + 128);
                pSh->ChromaOffsetL0[i][j] = HevcClip(nTemp, -128, 127);
            }
        }
        else
        {
            pSh->ChromaWeightL0[i][0] = 1 << pSh->nChromaLog2WeightDenom;
            pSh->ChromaOffsetL0[i][0] = 0;
            pSh->ChromaWeightL0[i][1] = 1 << pSh->nChromaLog2WeightDenom;
            pSh->ChromaOffsetL0[i][1] = 0;
        }
    }
    if(pSh->eSliceType == HEVC_B_SLICE)
    {
        for(i = 0; i < pSh->nNumOfRefs[1]; i++)
        {
            bLumaWeightL1Flag[i] = HevcGetOneBit(pGb, "luma_weight_l1_flag[i]");
            if(!bLumaWeightL1Flag[i])
            {
                pSh->LumaWeightL1[i] = 1 << pSh->nLumaLog2WeightDenom;
                pSh->LumaOffsetL1[i] = 0;
            }
        }
        if(pHevcDec->pSps->nChromaFormatIdc != 0)
        {
            for(i = 0; i < pSh->nNumOfRefs[1]; i++)
                bChromaWeightL1Flag[i] = HevcGetOneBit(pGb, "chroma_weight_l1_flag[i]");
        }
        else
        {
            for(i = 0; i < pSh->nNumOfRefs[1]; i++)
                bChromaWeightL1Flag[i] = 0;
        }
        for(i = 0; i < pSh->nNumOfRefs[1]; i++)
        {
            if(bLumaWeightL1Flag[i])
            {
                s32 nDeltaLumaWeightL1 = HevcGetSe(pGb, "delta_luma_weight_l1[i]");
                pSh->LumaWeightL1[i] = (1 << pSh->nLumaLog2WeightDenom) + nDeltaLumaWeightL1;
                pSh->LumaOffsetL1[i] = HevcGetSe(pGb, "luma_offset_l1[i]");
            }
            if(bChromaWeightL1Flag[i])
            {
                for(j = 0; j < 2; j++)
                {
                    s32 nDeltaChromaWeightL1 = HevcGetSe(pGb, "delta_chroma_weight_l1[i][j]");
                    s32 nDeltaChromaOffsetL1 = HevcGetSe(pGb, "delta_chroma_offset_l1[i][j]");
                    pSh->ChromaWeightL1[i][j] =
                        (1 << pSh->nChromaLog2WeightDenom) + nDeltaChromaWeightL1;
                    nTemp = (nDeltaChromaOffsetL1 - ((128 * pSh->ChromaWeightL1[i][j]) >>
                        pSh->nChromaLog2WeightDenom) + 128);
                    pSh->ChromaOffsetL1[i][j] = HevcClip(nTemp, -128, 127);
                }
            }
            else
            {
                pSh->ChromaWeightL1[i][0] = 1 << pSh->nChromaLog2WeightDenom;
                pSh->ChromaOffsetL1[i][0] = 0;
                pSh->ChromaWeightL1[i][1] = 1 << pSh->nChromaLog2WeightDenom;
                pSh->ChromaOffsetL1[i][1] = 0;
            }
        }
    }
}
#endif

s32 HevcDecodeNalVps(HevcContex *pHevcDec)
{
#define HEVC_DECODE_VPS_ERROR()          \
    if(pVps != NULL)                \
    {                                \
        HevcFree(&pVps);            \
        return -1;                    \
    }

    HevcVPS *pVps;
    GetBitContext *pGb;
    s32 i, j, nRet;
    s32 nVpsId = 0;
    s32 nTemp = 0;

    pGb = &pHevcDec->getBit;
    pVps = HevcCalloc(1, sizeof(HevcVPS));
    if(pVps == NULL)
    {
        loge(" h265 decode nal vps calloc fail");
        HEVC_DECODE_VPS_ERROR();
    }

    HevcCabacTrace(pGb, "-------- decode nal vps start --------");

    nVpsId = HevcGetNBits(4, pGb, "vps_video_parameter_set_id");
    logv(" compatibility debug. vps id: %d ", nVpsId);
    if(nVpsId > HEVC_MAX_VPS_NUM)
    {
        loge(" h265 decode nal vps, vps id larger than MAX_VPS_NUM ");
        HEVC_DECODE_VPS_ERROR();
    }
    nTemp = HevcGetNBits(2, pGb, "vps_reserved_three_2bits");
    if(nTemp != 3)
    {
        loge(" h265 decode nal vps, vps_reserved_three_2bits != 3 ");
        HEVC_DECODE_VPS_ERROR();
    }
    pVps->nVpsMaxLayers = HevcGetNBits(6, pGb, "vps_max_layers_minus1") + 1;
    pVps->nVpsMaxSubLayers = HevcGetNBits(3, pGb, "vps_max_sub_layers_minus1") + 1;
    pVps->bVpsTemporalIdNestingFlag = HevcGetOneBit(pGb, "vps_temporal_id_nesting_flag");
    nTemp = HevcGetNBits(16, pGb, "vps_reserved_0xffff_16bits");
    if(nTemp != 0xffff)
    {
        loge(" h265 decode nal vps, vps_reserved_0xffff_16bits != 0xffff ");
        HEVC_DECODE_VPS_ERROR();
    }
    if(pVps->nVpsMaxSubLayers > HEVC_MAX_SUB_LAYERS)
    {
        loge(" h265 decode nal vps, vps_max_sub_layers > 7 ");
        HEVC_DECODE_VPS_ERROR();
    }
    nRet = HevcParsePTL(pHevcDec, &pVps->Ptl, pVps->nVpsMaxSubLayers);
    if(nRet < 0)
    {
        loge(" h265 decode nal vps, The profile is unsupport ");
        HEVC_DECODE_VPS_ERROR();
    }
    else if(nRet == HEVC_RESULT_PICTURES_SIZE_CHANGE)
        return nRet;

    pVps->bVpsSubLayerOrderingInfoPresentFlag =
        HevcGetOneBit(pGb, "bVpsSubLayerOrderingInfoPresentFlag");
    for(i = (pVps->bVpsSubLayerOrderingInfoPresentFlag ? 0 : pVps->nVpsMaxSubLayers - 1);
            i <= pVps->nVpsMaxSubLayers - 1; i++)
    {
        pVps->nVpsMaxDecPicBuffering[i] =
            HevcGetUe(pGb, "vps_max_dec_pic_buffering_minus1[i]") + 1;
        pVps->nVpsNumReorderPics[i] =
            HevcGetUe(pGb, "vps_max_num_reorder_pics[i]");
        pVps->nVpsMaxLatencyIncrease[i] =
            HevcGetUe(pGb, "vps_max_latency_increase_plus1[i]") - 1;
        pHevcDec->nVpsMaxDecPicBuffering = pVps->nVpsMaxDecPicBuffering[i];
        pHevcDec->nVpsNumReorderPics = pVps->nVpsNumReorderPics[i];
        if(pVps->nVpsMaxDecPicBuffering[i] > HEVC_MAX_DPB_SIZE)
        {
            loge(" h265 decode nal vps, vps_max_dec_pic_buffering[i] > HEVC_MAX_DPB_SIZE ");
            HEVC_DECODE_VPS_ERROR();
        }
        if(pVps->nVpsNumReorderPics[i] > (pVps->nVpsMaxDecPicBuffering[i] - 1))
        {
            loge(" h265 decode nal vps, \
                vps_max_num_reorder_pics(%d) > vps_max_dec_pic_buffering(%d) ",
                    pVps->nVpsNumReorderPics[i], pVps->nVpsMaxDecPicBuffering[i]);
            pVps->nVpsMaxDecPicBuffering[i] = pVps->nVpsNumReorderPics[i] + 1;
//            HEVC_DECODE_VPS_ERROR();
        }
    }
    pVps->nVpsMaxLayerId = HevcGetNBits(6, pGb, "vps_max_layer_id");
    pVps->nVpsNumLayerSets = HevcGetUe(pGb, "vps_num_layer_sets_minus1") + 1;
    for(i = 1; i < pVps->nVpsNumLayerSets; i++)
        for(j = 0; j <= pVps->nVpsMaxLayerId; j++)
            HevcSkipBits(pGb, 1, "layer_id_included_flag[i][j]");
    /* layer_id_included_flag[i][j] */
    pVps->bVpsTimingInfoPresentFlag = HevcGetOneBit(pGb, "vps_timing_info_present_flag");
    if(pVps->bVpsTimingInfoPresentFlag)
    {
        pVps->nVpsNumUnitsInTick = HevcGetNBitsL(32, pGb, "vps_num_units_in_tick");
        pVps->nVpsTimeScale = HevcGetNBitsL(32, pGb, "vps_time_scale");
        pVps->bVpsPocProportionalToTimingFlag =
            HevcGetOneBit(pGb, "vps_poc_proportional_to_timing_flag");
        if(pVps->bVpsPocProportionalToTimingFlag)
            pVps->nVpsNumTicksPocDiffOne =
                    HevcGetUe(pGb, "vps_num_ticks_poc_diff_one_minus1") + 1;
        pVps->nVpsNumHrdParameters = HevcGetUe(pGb, "vps_num_hrd_parameters");
        for(i = 0; i < pVps->nVpsNumHrdParameters; i++)
        {
            s32 bCprmsPresentFlag = 1;
            HevcGetUe(pGb, "hrd_layer_set_idx[i]");
            if(i > 0)
                bCprmsPresentFlag = HevcGetOneBit(pGb, "cprms_present_flag[i]");
            HevcParseHRD(pHevcDec, bCprmsPresentFlag, pVps->nVpsMaxSubLayers);
        }
    }
    pVps->bVpsExtensionFlag = HevcGetOneBit(pGb, "vps_extension_flag");

    if(pVps->bVpsExtensionFlag)
    {
        logw(" vps extension flag == 1 ");
        // HevcVpsExtension();
    }
    if(pHevcDec->VpsList[nVpsId] != NULL)
    {
        nTemp = memcmp(pHevcDec->VpsList[nVpsId], pVps, sizeof(HevcVPS));
        if(!nTemp)
        {
            HevcFree(&pVps);  /* the same vps, no need to change original one */
        }
        else
        {
            HevcFree(&pHevcDec->VpsList[nVpsId]);
            pHevcDec->VpsList[nVpsId] = pVps;
        }
    }
    else
        pHevcDec->VpsList[nVpsId] = pVps;
    HevcCabacTrace(pGb, "-------- decode nal vps end --------");
    return 0;
#undef HEVC_DECODE_VPS_ERROR
}

static void HevcSpsDefaultValue(HevcSPS *pSps)
{
    CEDARC_UNUSE(pSps);

#if 0
    s32 i;
    HevcSPS *ppSps = pSps;
    pSps->nMaxSubLayers = 1;
    pSps->nWidth = 352;
    pSps->nHeight = 288;
    pSps->pcm.nLog2DiffMaxMinPcmCbSize = 2;
    pSps->pcm.nLog2DiffMaxMinPcmCbSize = 3;
    pSps->pcm.nPcmBitDepthLuma = 8;
    pSps->pcm.nPcmBitdepthChroma = 8;
    pSps->nBitDepthChroma = 8;
    pSps->nBitDepthLuma = 8;
    pSps->nLog2MaxTransfornBlockSize = 5;
    for (i = 0; i < HEVC_MAX_SUB_LAYERS; i++)
        pSps->TemporalLayer[i].nSpsMaxDecPicBuffering = 1;

    if(pSPS->m_scalingList != NULL)
    {
        destroyScalingListCoef(pSPS->m_scalingList);
    }

    pSPS->m_scalingList = h265_calloc(1, sizeof(ScalingList));
    createScalingList(pSPS->m_scalingList);
    setDefaultScalingList(pSPS->m_scalingList);
    initVUI(&(pSPS->m_vuiParameters));
#endif
}

s32 HevcDecodeNalSps(HevcContex *pHevcDec)
{
#define HEVC_DECODE_SPS_ERROR()          \
    if(pSps != NULL)                \
    {                                \
        HevcFree(&pSps);            \
        return -1;                    \
    }
    GetBitContext *pGb;
    s32 i, nSpsId, nRet, bFlag, nStart;
    HevcSPS *pSps;

    pGb = &pHevcDec->getBit;
    pSps = HevcCalloc(1, sizeof(HevcSPS));
    if(pSps == NULL)
    {
        loge(" h265 decode nal sps. calloc fail ");
        return -1;
    }
    HevcSpsDefaultValue(pSps);
    HevcCabacTrace(pGb, "-------- decode nal sps start --------");
    pSps->nVpsId = HevcGetNBits(4, pGb, "sps_video_parameter_set_id");
    if(pSps->nVpsId >= HEVC_MAX_SPS_NUM)
    {
        loge(" h265 decode nal sps. vps id > 32 ");
        HEVC_DECODE_SPS_ERROR();
    }
    if(pHevcDec->VpsList[pSps->nVpsId] == NULL)
    {
        loge(" h265 decode nal sps. vpslist[%d] == NULL  ", pSps->nVpsId);
        HEVC_DECODE_SPS_ERROR();
    }
    pSps->nMaxSubLayers = HevcGetNBits(3, pGb, "sps_max_sub_layers_minus1") + 1;
    if(pSps->nMaxSubLayers > HEVC_MAX_SUB_LAYERS)
    {
        loge(" h265 decode nal sps. max sub layer(%d) > 7, out of range  ", pSps->nMaxSubLayers);
        HEVC_DECODE_SPS_ERROR();
    }
    HevcGetOneBit(pGb, "sps_temporal_id_nesting_flag");
    nRet = HevcParsePTL(pHevcDec, &pSps->Ptl, pSps->nMaxSubLayers);
    if(nRet < 0)
    {
        loge(" h265 decode nal sps, The profile is unsupport ");
        HEVC_DECODE_SPS_ERROR();
    }
    else if(nRet == HEVC_RESULT_PICTURES_SIZE_CHANGE)
        return nRet;

    nSpsId = HevcGetUe(pGb, "sps_seq_parameter_set_id");
    logv(" compatibility debug.    sps id: %d ", nSpsId);
    if(nSpsId > HEVC_MAX_SPS_NUM)
    {
        loge(" h265 decode nal sps, sps id(%d) > 32, id is out of range ", nSpsId);
        HEVC_DECODE_SPS_ERROR();
    }
    pSps->nChromaFormatIdc = HevcGetUe(pGb, "chroma_format_idc");
    if(pSps->nChromaFormatIdc != 1)
    {
        loge(" h265 decode nal sps, chroma_format_idc(%d) != 1, not support ",
            pSps->nChromaFormatIdc);
        HEVC_DECODE_SPS_ERROR();
    }
    if(pSps->nChromaFormatIdc == 3)
        pSps->bSeparateColourPlaneFlag = HevcGetOneBit(pGb, "separate_colour_plane_flag");
    /* todo */
    pSps->nWidth = HevcGetUe(pGb, "pic_width_in_luma_samples");
    pSps->nHeight = HevcGetUe(pGb, "pic_height_in_luma_samples");
    bFlag = HevcGetOneBit(pGb, "conformance_window_flag");
    pSps->PicConfWin.bOffsetAvailable = bFlag;
    if(bFlag)
    {
        pSps->PicConfWin.nLeftOffset = HevcGetUe(pGb, "conf_win_left_offset") * 2;
        pSps->PicConfWin.nRightOffset = HevcGetUe(pGb, "conf_win_right_offset") * 2;
        pSps->PicConfWin.nTopOffset = HevcGetUe(pGb, "conf_win_top_offset") * 2;
        pSps->PicConfWin.nBottomOffset = HevcGetUe(pGb, "conf_win_bottom_offset") * 2;
        if((pSps->PicConfWin.nLeftOffset +
                pSps->PicConfWin.nRightOffset +
                pSps->PicConfWin.nTopOffset +
                pSps->PicConfWin.nBottomOffset))
        {
            logv(" picture conformance offset. top: %d, bottom: %d, left: %d, right: %d ",
                    pSps->PicConfWin.nTopOffset, pSps->PicConfWin.nBottomOffset,
                    pSps->PicConfWin.nLeftOffset, pSps->PicConfWin.nRightOffset);
        }
        pSps->OutPutWindow = pSps->PicConfWin;
    }
    pSps->nBitDepthLuma = HevcGetUe(pGb, "bit_depth_luma_minus8") + 8;
    pSps->nBitDepthChroma = HevcGetUe(pGb, "bit_depth_chroma_minus8") + 8;
    if(pSps->nBitDepthLuma != pSps->nBitDepthChroma)
    {
        logw("265 decode nal sps, luam bit depth != chroma bit depth");
    }
    pSps->nLog2MaxPocLsb = HevcGetUe(pGb, "log2_max_pic_order_cnt_lsb_minus4") + 4;
    if(pSps->nLog2MaxPocLsb > 16)
    {
        loge(" h265 decode nal sps, log2_max_pic_order_cnt_lsb(%d) > 16, out of range ",
            pSps->nLog2MaxPocLsb);
        HEVC_DECODE_SPS_ERROR();
    }
    bFlag = HevcGetOneBit(pGb, "sps_sub_layer_ordering_info_present_flag");
    nStart = bFlag ? 0 : pSps->nMaxSubLayers - 1;
    for(i = nStart ; i <= pSps->nMaxSubLayers - 1; i++)
    {
        pSps->TemporalLayer[i].nSpsMaxDecPicBuffering =
            HevcGetUe(pGb, "sps_max_dec_pic_buffering_minus1[i]") + 1;
        pSps->TemporalLayer[i].nSpsMaxNumReoderPics =
            HevcGetUe(pGb, "sps_max_num_reorder_pics[i]");
        pSps->TemporalLayer[i].nSpsMaxLatencyIncrease =
            HevcGetUe(pGb, "sps_max_latency_increase_plus1[i]");
        if(pSps->TemporalLayer[i].nSpsMaxDecPicBuffering > HEVC_MAX_DPB_SIZE)
        {
            loge("h265 decode nal sps, sps_max_dec_pic_buffering(%d) > 16, out of range",
                    pSps->TemporalLayer[i].nSpsMaxDecPicBuffering);
            HEVC_DECODE_SPS_ERROR();
        }
        if(pSps->TemporalLayer[i].nSpsMaxNumReoderPics >
            pSps->TemporalLayer[i].nSpsMaxDecPicBuffering -1)
        {
            if(pSps->TemporalLayer[i].nSpsMaxNumReoderPics > HEVC_MAX_DPB_SIZE)
            {
                loge("h265 decode nal sps, sps_max_num_reorder_pics(%d) > 16, out of range",
                        pSps->TemporalLayer[i].nSpsMaxNumReoderPics);
                HEVC_DECODE_SPS_ERROR();
            }
            pSps->TemporalLayer[i].nSpsMaxDecPicBuffering =
                pSps->TemporalLayer[i].nSpsMaxNumReoderPics + 1;
        }
        logv(" sps max buffering: %d", pSps->TemporalLayer[i].nSpsMaxDecPicBuffering);
        if(!bFlag)
            for(i = 0; i < nStart; i++)
            {
                pSps->TemporalLayer[i].nSpsMaxDecPicBuffering =
                    pSps->TemporalLayer[nStart].nSpsMaxDecPicBuffering;
                pSps->TemporalLayer[i].nSpsMaxNumReoderPics =
                    pSps->TemporalLayer[nStart].nSpsMaxNumReoderPics;
                pSps->TemporalLayer[i].nSpsMaxLatencyIncrease =
                    pSps->TemporalLayer[nStart].nSpsMaxLatencyIncrease;
            }
    }
    pSps->nLog2MinCbSize = HevcGetUe(pGb, "log2_min_luma_coding_block_size_minus3") + 3;
    pSps->nLog2DiffMaxMinCbSize = HevcGetUe(pGb, "log2_diff_max_min_luma_coding_block_size");
    pSps->nLog2MinTbSize = HevcGetUe(pGb, "log2_min_transform_block_size_minus2") + 2;
    pSps->nLog2DiffMaxMinTbSize = HevcGetUe(pGb, "log2_diff_max_min_transform_block_size");

    pSps->nMaxTransformHierarchydepthInter = HevcGetUe(pGb, "max_transform_hierarchy_depth_inter");
    pSps->nMaxTransformHierarchydepthIntra = HevcGetUe(pGb, "nMaxTransformHierarchydepthIntra");

    pSps->bScalingListEnableFlag = HevcGetOneBit(pGb, "scaling_list_enabled_flag");
    if(pSps->bScalingListEnableFlag)
    {
        HevcScalingListDefaultData(&pSps->ScalingList);
        pSps->bSpsScalingListDataPresentFlag =
            HevcGetOneBit(pGb, "sps_scaling_list_data_present_flag");
        if(pSps->bSpsScalingListDataPresentFlag)
        {
            logv(" scaling list data sps____  id: %d", nSpsId);
            HevcScalingListDefaultData(&pSps->ScalingList);
            HevcParseScalingListData(pHevcDec, &pSps->ScalingList);
            HevcShowScalingListData(&pSps->ScalingList, 1);
        }
    }
    pSps->bAmpEnableFlag = HevcGetOneBit(pGb, "amp_enabled_flag");
    pSps->bSaoEnabled = HevcGetOneBit(pGb, "sample_adaptive_offset_enabled_flag");
    pSps->bPcmEnableFlag = HevcGetOneBit(pGb, "pcm_enabled_flag");
    if(pSps->bPcmEnableFlag)
    {
        pSps->pcm.nPcmBitDepthLuma =
            HevcGetNBits(4, pGb, "pcm_sample_bit_depth_luma_minus1") + 1;
        pSps->pcm.nPcmBitdepthChroma =
            HevcGetNBits(4, pGb, "pcm_sample_bit_depth_chroma_minus1") + 1;
        pSps->pcm.nLog2MinPcmCbSize =
            HevcGetUe(pGb, "log2_min_pcm_luma_coding_block_size_minus3") + 3;
        pSps->pcm.nLog2DiffMaxMinPcmCbSize =
            HevcGetUe(pGb, "log2_diff_max_min_pcm_luma_coding_block_size");
        pSps->pcm.bLoopFilterDisableFlag =
            HevcGetOneBit(pGb, "pcm_loop_filter_disabled_flag");
    }
    else
    {
        pSps->pcm.nPcmBitDepthLuma = 8;
        pSps->pcm.nPcmBitdepthChroma = 8;
        pSps->pcm.nLog2MinPcmCbSize = 3;
        pSps->pcm.nLog2DiffMaxMinPcmCbSize = 2;
    }

    pSps->nNumStRps = HevcGetUe(pGb, "num_short_term_ref_pic_sets");
    if(pSps->nNumStRps > HEVC_MAX_SHORT_TERM_RPS_COUNT)
    {
        loge(" h265 nal sps. the num of short term ref(%d) is out of range ",
            pSps->nNumStRps);
        HEVC_DECODE_SPS_ERROR();
    }
    for( i = 0; i < (s32)pSps->nNumStRps; i++)
    {
        HevcParseShortTermRefSet(pHevcDec, &pSps->StRps[i], pSps, 0);
        HevcShowShortRefPicSetInfoDebug(&pSps->StRps[i], -1);
    }
    pSps->bLongtermRefPicsPresentFlag = HevcGetOneBit(pGb, "long_term_ref_pics_present_flag");
    if(pSps->bLongtermRefPicsPresentFlag)
    {
        pSps->nNumLongTermRefPicsSps = HevcGetUe(pGb, "num_long_term_ref_pics_sps");
        for( i = 0; i < pSps->nNumLongTermRefPicsSps; i++ )
        {
            pSps->nLtRefPicPocLsbSps[i] =
                HevcGetNBits(pSps->nLog2MaxPocLsb, pGb, "lt_ref_pic_poc_lsb_sps[i]");
            pSps->bUsedByCurrPicLtSpsFlag[i] =
                HevcGetOneBit(pGb, "used_by_curr_pic_lt_sps_flag[i]");
        }
    }
    pSps->bSpsTemporalMvpEnableFlag = HevcGetOneBit(pGb, "sps_temporal_mvp_enabled_flag");
    pSps->bSpsStrongIntraSmoothingenableFlag =
        HevcGetOneBit(pGb, "strong_intra_smoothing_enabled_flag");
    bFlag = HevcGetOneBit(pGb, "vui_parameters_present_flag");
    if(bFlag)
    {
        HevcParseVuiParameters(pHevcDec, pSps);
#if 0
        pSps->OutPutWindow.nTopOffset += pSps->vui.DefDispWin.nTopOffset;
        pSps->OutPutWindow.nBottomOffset += pSps->vui.DefDispWin.nBottomOffset;
        pSps->OutPutWindow.nLeftOffset += pSps->vui.DefDispWin.nLeftOffset;
        pSps->OutPutWindow.nRightOffset += pSps->vui.DefDispWin.nRightOffset;
#endif
    }

    /* for CedarX display setting */
    pSps->nCedarxTopOffset = pSps->OutPutWindow.nTopOffset;
    pSps->nCedarxLeftOffset = pSps->OutPutWindow.nLeftOffset;
    pSps->nCedarxBottomOffset =
        pSps->nHeight - (pSps->OutPutWindow.nTopOffset + pSps->OutPutWindow.nBottomOffset);
    pSps->nCedarxRightOffset =
        pSps->nWidth - (pSps->OutPutWindow.nLeftOffset + pSps->OutPutWindow.nRightOffset);
    if(pHevcDec->ControlInfo.bScaleDownEnable)
    {
        s32 nHSd = pHevcDec->ControlInfo.nHorizonScaleDownRatio;
        s32 nVSd = pHevcDec->ControlInfo.nVerticalScaleDownRatio;
        pSps->nCedarxTopOffsetSd = pSps->OutPutWindow.nTopOffset >> nVSd;
        pSps->nCedarxLeftOffsetSd = pSps->OutPutWindow.nLeftOffset >> nHSd;
        pSps->nCedarxBottomOffsetSd = (pSps->nHeight >> nVSd) -
            ((pSps->OutPutWindow.nTopOffset + pSps->OutPutWindow.nBottomOffset) >> nVSd);
        pSps->nCedarxRightOffsetSd = (pSps->nWidth >> nHSd) -
            ((pSps->OutPutWindow.nLeftOffset + pSps->OutPutWindow.nRightOffset) >> nHSd);
        logv(" nHsd: %d, nVsd: %d;  top: %d, bottom: %d, left: %d, right: %d", nHSd, nVSd,
                pSps->nCedarxTopOffsetSd, pSps->nCedarxBottomOffsetSd,
                pSps->nCedarxLeftOffsetSd, pSps->nCedarxRightOffsetSd);
    }

    HevcGetOneBit(pGb, "sps_extension_flag");

    /********/
    pSps->nLog2CtbSize = pSps->nLog2MinCbSize + pSps->nLog2DiffMaxMinCbSize;
    pSps->nLog2MinPusize = pSps->nLog2MinCbSize - 1;
    pSps->nCtbWidth  = (pSps->nWidth  + (1 << pSps->nLog2CtbSize) - 1) >> pSps->nLog2CtbSize;
    pSps->nCtbHeight = (pSps->nHeight + (1 << pSps->nLog2CtbSize) - 1) >> pSps->nLog2CtbSize;
    pSps->nCtbSize = pSps->nCtbWidth * pSps->nCtbHeight;
//    logd(" h265 ctb w: %d, ctb h: %d, ctb size: %d",
//    pSps->nCtbWidth, pSps->nCtbHeight, pSps->nCtbSize);

    pSps->nMinCbWidth = pSps->nWidth >> pSps->nLog2MinCbSize;
    pSps->nMinCbHeight = pSps->nHeight >> pSps->nLog2MinCbSize;
    pSps->nMinTbWidth = pSps->nWidth >> pSps->nLog2MinTbSize;
    pSps->nMinTbHeight = pSps->nHeight >> pSps->nLog2MinTbSize;
    pSps->nMinPuWidth = pSps->nWidth >> pSps->nLog2MinPusize;
    pSps->nMinPuHeight = pSps->nHeight >> pSps->nLog2MinPusize;
    pSps->nQpBdOffset = 6 * (pSps->nBitDepthLuma - 8);

    if(pHevcDec->SpsList[nSpsId] != NULL)
    {
        nRet = memcmp(pSps, pHevcDec->SpsList[nSpsId], sizeof(HevcSPS));
        if(!nRet)
        {
            HevcFree(&pSps); /* The same sps, no need to change original one */
        }
        else
        {
            logv(" new sps ...... ");
            HevcFree(&pHevcDec->SpsList[nSpsId]);
            pHevcDec->SpsList[nSpsId] = pSps;
        }
    }
    else
        pHevcDec->SpsList[nSpsId] = pSps;
    pHevcDec->bHadFindSpsFlag = HEVC_TRUE;
    #if 1  //for cts test
    if((pHevcDec->SpsList[nSpsId]!= NULL) &&
        (pHevcDec->SpsList[nSpsId]->nWidth<64) &&
        (pHevcDec->SpsList[nSpsId]->nHeight<64))
    {
        pHevcDec->bHadFindSpsFlag = HEVC_FALSE;
    }
    #endif
    HevcCabacTrace(pGb, "-------- decode nal sps end --------");
    return 0;
#undef HEVC_DECODE_SPS_ERROR
}

static void HevcPpsDefaultValue(HevcPPS *pPps)
{
#if 0
    pPps->nNumRefIdxL0DefaultActive = 1;
    pPps->nNumRefIdxL1DefaultActive = 1;
#endif
    pPps->bLoopFilterAcrossTilesEnableFlag = 1;
    pPps->nNumTileColums = 1;
    pPps->nNumTileRows = 1;
    pPps->bUniformSpacingFlag = 1;

//    pPps->nPpsBetaOffset = 2*0xd;
//    pPps->nPpsTcOffset = 2*0xd;
#if 0
    if(pPPS->m_scalingList != NULL)
    {
        destroyScalingListCoef(pPPS->m_scalingList);
        h265_free_p(&(pPPS->m_scalingList));
    }

    pPPS->m_scalingList = h265_calloc(1, sizeof(ScalingList));
    createScalingList(pPPS->m_scalingList);
    setDefaultScalingList(pPPS->m_scalingList);
#endif
}

void HevcFreePpsBuffer(HevcPPS *pPps)
{
    if(pPps)
    {
        HevcFree(&pPps->RowHeight);
        HevcFree(&pPps->ColumnWidth);
        HevcFree(&pPps->ColumnWidth);
        HevcFree(&pPps->ColumBoundary);
        HevcFree(&pPps->RowBoundary);
        HevcFree(&pPps->ColumIdxX);
        HevcFree(&pPps->CtbAddrRsToTs);
        HevcFree(&pPps->CtbAddrTsToRs);
        HevcFree(&pPps->TileId);
        HevcFree(&pPps->TileIdInRs);
        HevcFree(&pPps->TilePositionRs);
        HevcFree(&pPps->TilePosInfo);
    }
}

s32 HevcDecodeNalPps(HevcContex *pHevcDec)
{
#define HEVC_DECODE_PPS_ERROR()      \
    if(pPps != NULL)                \
    {                                \
        HevcFree(&pPps);            \
        return -1;                    \
    }
#define HEVC_DECODE_PPS_ERROR2()      \
    if(pPps != NULL)                \
    {                                \
        HevcFree(&pPps->RowHeight);    \
        HevcFree(&pPps->ColumnWidth);\
        HevcFree(&pPps);            \
        return -1;                    \
    }
#define HEVC_DECODE_PPS_ERROR3()      \
    if(pPps != NULL)                \
    {                                \
        HevcFree(&pPps->RowHeight);    \
        HevcFree(&pPps->ColumnWidth);\
        HevcFree(&pPps->ColumBoundary); \
        HevcFree(&pPps->RowBoundary);   \
        HevcFree(&pPps->ColumIdxX);      \
        HevcFree(&pPps->CtbAddrRsToTs);  \
        HevcFree(&pPps->CtbAddrTsToRs);  \
        HevcFree(&pPps->TileId);    \
        HevcFree(&pPps->TileIdInRs);    \
        HevcFree(&pPps->TilePosInfo);    \
        HevcFree(&pPps);            \
        return -1;                    \
    }

    GetBitContext *pGb;
    HevcPPS *pPps;
    HevcSPS *pSps;
    s32 i, j, nPpsId, nTileId;
    s32 nCtbAddrRs, x, y;

    pGb = &pHevcDec->getBit;
    pPps = HevcCalloc(1, sizeof(HevcPPS));
    if(pPps == NULL)
    {
        loge(" h265 decode pps error. Calloc memory fail");
        return -1;
    }
    HevcPpsDefaultValue(pPps);
    HevcCabacTrace(pGb, "-------- decode nal pps start --------");
    nPpsId = HevcGetUe(pGb, "pps_pic_parameter_set_id");
    logv(" compatibility debug.    -pps id: %d ", nPpsId);
    if(nPpsId > HEVC_MAX_PPS_NUM)
    {
        loge(" h265 decode pps error. pps_id(%d) > 256, out of range", nPpsId);
        HEVC_DECODE_PPS_ERROR();
    }
    pPps->nSpsId = HevcGetUe(pGb, "pps_seq_parameter_set_id");
    if(pPps->nSpsId > HEVC_MAX_SPS_NUM)
    {
        loge(" h265 decode pps error. sps_id(%d) > 256, out of range", pPps->nSpsId);
        HEVC_DECODE_PPS_ERROR();
    }
    if(pHevcDec->SpsList[pPps->nSpsId] == NULL)
    {
        loge(" h265 decode pps error. sps[%d] == NULL", pPps->nSpsId);
        HEVC_DECODE_PPS_ERROR();
    }
    pSps = pHevcDec->SpsList[pPps->nSpsId];
    pPps->bDependentSliceSegmentsEnabledFlag =
        HevcGetOneBit(pGb, "dependent_slice_segments_enabled_flag");
    pPps->bOutputFlagPresentFlag = HevcGetOneBit(pGb, "output_flag_present_flag");
    pPps->nNumExtraSliceHeaderBits = HevcGetNBits(3, pGb, "num_extra_slice_header_bits");
    pPps->bSignDataHidingflag = HevcGetOneBit(pGb, "sign_data_hiding_enabled_flag");
    pPps->bCabacInitPresentFlag = HevcGetOneBit(pGb, "cabac_init_present_flag");
    pPps->nNumRefIdxL0DefaultActive = HevcGetUe(pGb, "num_ref_idx_l0_default_active_minus1") + 1;
    pPps->nNumRefIdxL1DefaultActive = HevcGetUe(pGb, "num_ref_idx_l1_default_active_minus1") + 1;
    pPps->nInitQpMinus26 = HevcGetSe(pGb, "init_qp_minus26");
    pPps->bConstrainedIntraPredFlag = HevcGetOneBit(pGb, "constrained_intra_pred_flag");
    pPps->bTransformSkipEnabledFlag = HevcGetOneBit(pGb, "transform_skip_enabled_flag");
    pPps->bCuQpDeltaEnabledFlag = HevcGetOneBit(pGb, "cu_qp_delta_enabled_flag");
    if(pPps->bCuQpDeltaEnabledFlag)
        pPps->nDiffCuQpDeltaDepth = HevcGetUe(pGb, "diff_cu_qp_delta_depth");
    pPps->nPpsCbQpOffset = HevcGetSe(pGb, "pps_cb_qp_offset");
    if(pPps->nPpsCbQpOffset < -12 || pPps->nPpsCbQpOffset > 12)
    {
        loge(" h265 decode pps error. pPps->nPpsCbQpOffset(%d) if out of range ",
            pPps->nPpsCbQpOffset);
        HEVC_DECODE_PPS_ERROR();
    }
    pPps->nPpsCrQpOffset = HevcGetSe(pGb, "pps_cr_qp_offset");
    if(pPps->nPpsCrQpOffset < -12 || pPps->nPpsCrQpOffset > 12)
    {
        loge(" h265 decode pps error. pPps->nPpsCrQpOffset(%d) if out of range ",
            pPps->nPpsCrQpOffset);
        HEVC_DECODE_PPS_ERROR();
    }
    pPps->bPpsSliceChromaQpOffsetsPresentFlag =
        HevcGetOneBit(pGb, "pps_slice_chroma_qp_offsets_present_flag");
    pPps->bWeightedPredFlag = HevcGetOneBit(pGb, "weighted_pred_flag");
    pPps->bWeightedBipredFlag = HevcGetOneBit(pGb, "weighted_bipred_flag");
    pPps->bTransquantBypassEnableFlag = HevcGetOneBit(pGb, "transquant_bypass_enabled_flag");
    pPps->bTilesEnabledFlag = HevcGetOneBit(pGb, "tiles_enabled_flag");
    pPps->bEntropyCodingSyncEnabledFlag = HevcGetOneBit(pGb, "entropy_coding_sync_enabled_flag");

    if(pPps->bTilesEnabledFlag)
    {
        logv(" h265 debug. tile enalbe ");
        pPps->nNumTileColums = HevcGetUe(pGb, "num_tile_columns_minus1") + 1;
        pPps->nNumTileRows = HevcGetUe(pGb, "num_tile_rows_minus1") + 1;
        pPps->ColumnWidth = HevcCalloc(pPps->nNumTileColums, sizeof(u32));
        pPps->RowHeight = HevcCalloc(pPps->nNumTileRows, sizeof(u32));
        if(pPps->ColumnWidth == NULL || pPps->RowHeight == NULL)
        {
            loge(" h265 decode pps. calloc array fail ");
            HEVC_DECODE_PPS_ERROR2();
        }
        pPps->bUniformSpacingFlag = HevcGetOneBit(pGb, "uniform_spacing_flag");
        if(!pPps->bUniformSpacingFlag)
        {
            s32 nSum = 0;
            for(i = 0; i < pPps->nNumTileColums - 1; i++)
            {
                pPps->ColumnWidth[i] = HevcGetUe(pGb, "column_width_minus1[i]") + 1;
                nSum += pPps->ColumnWidth[i];
            }
            if(nSum > pSps->nCtbWidth)
            {
                loge(" h265 decode pps error. tile width is out of range ");
                HEVC_DECODE_PPS_ERROR2();
            }
            pPps->ColumnWidth[pPps->nNumTileColums - 1] = pSps->nCtbWidth - nSum;
            nSum = 0;
            for(i = 0; i < pPps->nNumTileRows - 1; i++)
            {
                pPps->RowHeight[i] = HevcGetUe(pGb, "row_height_minus1[i]") + 1;
                nSum += pPps->RowHeight[i];
            }
            if(nSum > pSps->nCtbHeight)
            {
                loge(" h265 decode pps error. tile heights is out of range ");
                HEVC_DECODE_PPS_ERROR2();
            }
            pPps->RowHeight[pPps->nNumTileRows - 1] = pSps->nCtbHeight - nSum;
        }
        pPps->bLoopFilterAcrossTilesEnableFlag =
            HevcGetOneBit(pGb, "loop_filter_across_tiles_enabled_flag");
    }
    pPps->bPpsLoopFilterAcrossSlicesEnableFlag =
        HevcGetOneBit(pGb, "pps_loop_filter_across_slices_enabled_flag");
    pPps->bDeblockingFilterControlPresentFlag =
        HevcGetOneBit(pGb, "deblocking_filter_control_present_flag");
    if(pPps->bDeblockingFilterControlPresentFlag)
    {
        pPps->bDeblocingFilterOverrideEnableFlag =
            HevcGetOneBit(pGb, "deblocking_filter_override_enabled_flag");
        pPps->bPpsDeblockingDisableFlag =
            HevcGetOneBit(pGb, "pps_deblocking_filter_disabled_flag");
        if(!pPps->bPpsDeblockingDisableFlag)
        {
            pPps->nPpsBetaOffset = HevcGetSe(pGb, "pps_beta_offset_div2") * 2;
            pPps->nPpsTcOffset = HevcGetSe(pGb, "pps_tc_offset_div2") * 2;
            if(pPps->nPpsBetaOffset/2 < -6 || pPps->nPpsBetaOffset/2 > 6)
            {
                loge(" h265 decode pps error. pps_beta_offset_div2(%d) is out of range ",
                    pPps->nPpsBetaOffset/2);
                HEVC_DECODE_PPS_ERROR2();
            }
            if(pPps->nPpsTcOffset/2 < -6 || pPps->nPpsTcOffset/2 > 6)
            {
                loge(" h265 decode pps error. pps_tc_offset_div2(%d) is out of range ",
                    pPps->nPpsTcOffset);
                HEVC_DECODE_PPS_ERROR2();
            }
        }
    }

    pPps->bPpsScalingListDataPresentFlag =
        HevcGetOneBit(pGb, "pps_scaling_list_data_present_flag");
    if(pPps->bPpsScalingListDataPresentFlag)
    {
        logv(" scaling list data pps.....   id: %d", nPpsId);
        HevcScalingListDefaultData(&pPps->ScalingList);
        HevcParseScalingListData(pHevcDec, &pPps->ScalingList);
        HevcShowScalingListData(&pPps->ScalingList, 0);
    }
    pPps->bListsModificationPresentFlag =
        HevcGetOneBit(pGb, "lists_modification_present_flag");
    pPps->nLog2ParallelMergeLevelMinus2 =
        HevcGetUe(pGb, "log2_parallel_merge_level_minus2");
    pPps->bSliceHeaderExtentionPresentFlag =
        HevcGetOneBit(pGb, "slice_segment_header_extension_present_flag");
    pPps->bPpsExtensionFlag = HevcGetOneBit(pGb, "pps_extension_flag");

    if(!pPps->bTilesEnabledFlag)
        goto HevcDecodePpsExit;
    // Inferred parameters
    logv("bTilesEnabled. tile colum: %d, tile rows: %d ",
            pPps->nNumTileColums, pPps->nNumTileRows);
    pPps->ColumBoundary = HevcCalloc(pPps->nNumTileColums + 1, sizeof(u32));
    pPps->RowBoundary = HevcCalloc(pPps->nNumTileRows + 1, sizeof(u32));
    pPps->ColumIdxX = HevcCalloc(pSps->nCtbWidth, sizeof(s32));
    if(!pPps->ColumBoundary ||
        !pPps->RowBoundary ||
        !pPps->ColumIdxX)
    {
        loge(" h265 decode pps calloc error, tile context 0");
        HEVC_DECODE_PPS_ERROR3();
    }
    if(pPps->bUniformSpacingFlag)
    {
        if(!pPps->ColumnWidth)
        {
            pPps->ColumnWidth = HevcCalloc(pPps->nNumTileColums, sizeof(u32));
            pPps->RowHeight = HevcCalloc(pPps->nNumTileColums, sizeof(u32));
        }
        if(!pPps->ColumnWidth || !pPps->RowHeight)
        {
            loge(" h265 decode pps calloc error, tile context 1");
            HEVC_DECODE_PPS_ERROR3();
        }
        for(i = 0; i < pPps->nNumTileColums; i++)
            pPps->ColumnWidth[i] = ((i + 1) * pSps->nCtbWidth) / pPps->nNumTileColums -
                                           (i * pSps->nCtbWidth) / pPps->nNumTileColums;
        for(i = 0; i < pPps->nNumTileRows; i++)
            pPps->RowHeight[i] = ((i + 1) * pSps->nCtbHeight) / pPps->nNumTileRows -
                                           (i * pSps->nCtbHeight) / pPps->nNumTileRows;
    }
    pPps->ColumBoundary[0] = 0;
    for(i = 0; i < pPps->nNumTileColums; i++)
        pPps->ColumBoundary[i + 1] = pPps->ColumBoundary[i] + pPps->ColumnWidth[i];

    pPps->RowBoundary[0] = 0;
    for(i = 0; i < pPps->nNumTileRows; i++)
        pPps->RowBoundary[i + 1] = pPps->RowBoundary[i] + pPps->RowHeight[i];

    for(i = 0, j = 0; i < pSps->nCtbWidth; i++)
    {
        if(i > pPps->ColumBoundary[j])
            j++;
        pPps->ColumIdxX[i] = j;
    }

    pPps->CtbAddrRsToTs = HevcCalloc(pSps->nCtbSize, sizeof(s32));
    pPps->CtbAddrTsToRs = HevcCalloc(pSps->nCtbSize, sizeof(s32));
    pPps->TileId = HevcCalloc(pSps->nCtbSize, sizeof(s32));
    pPps->TileIdInRs = HevcCalloc(pSps->nCtbSize, sizeof(s32));
    pPps->TilePosInfo = HevcCalloc(pPps->nNumTileRows * pPps->nNumTileColums,
                         sizeof(HevcTilePositionInfo));

    if(!pPps->CtbAddrRsToTs ||
        !pPps->CtbAddrTsToRs ||
        !pPps->TilePosInfo ||
        !pPps->TileId || !pPps->TileIdInRs)
    {
        loge(" h265 decode pps calloc error, tile context 1");
        HEVC_DECODE_PPS_ERROR3();
    }

    for(nCtbAddrRs = 0; nCtbAddrRs < pSps->nCtbSize; nCtbAddrRs++)
    {
        s32 nTbX = nCtbAddrRs % pSps->nCtbWidth;
        s32 nTbY = nCtbAddrRs / pSps->nCtbWidth;
        s32 nTileX = 0;
        s32 nTileY = 0;
        s32 nVal = 0;

        /* find current nCtbAddrRs in which tile colum */
        for(i = 0; i < pPps->nNumTileColums; i++)
            if(nTbX < pPps->ColumBoundary[i + 1])
            {
                nTileX = i;
                break;
            }
        /* find current nCtbAddrRs in which tile row */
        for(i = 0; i < pPps->nNumTileRows; i++)
            if(nTbY < pPps->RowBoundary[i + 1])
            {
                nTileY = i;
                break;
            }
        /* caculate nCtbAddrRs's tile scan position */
        /*     1. find the current tile start addr in raster scan */
        for(i = 0; i < nTileX; i++)
            nVal += pPps->RowHeight[nTileY] * pPps->ColumnWidth[i];
        for(i = 0; i < nTileY; i++)
            nVal += pSps->nCtbWidth * pPps->RowHeight[i];
        /*     2. find nCtbAddrRs's position in current tile */
        nVal += (nTbY - pPps->RowBoundary[nTileY]) * pPps->ColumnWidth[nTileX] +
                nTbX - pPps->ColumBoundary[nTileX];

        pPps->CtbAddrRsToTs[nCtbAddrRs] = nVal;
        pPps->CtbAddrTsToRs[nVal] = nCtbAddrRs;
    }

    for(j = 0, nTileId = 0; j < pPps->nNumTileRows; j++)
        for(i = 0; i < pPps->nNumTileColums; i++, nTileId++)
        {
            pPps->TilePosInfo[nTileId].nTileStartAddrX = pPps->ColumBoundary[i];
            pPps->TilePosInfo[nTileId].nTileStartAddrY = pPps->RowBoundary[j];
            pPps->TilePosInfo[nTileId].nTileEndAddrX = pPps->ColumBoundary[i + 1] - 1;
            pPps->TilePosInfo[nTileId].nTileEndAddrY = pPps->RowBoundary[j + 1] - 1;
            for(y = pPps->RowBoundary[j]; y < pPps->RowBoundary[j + 1]; y++)
                for(x = pPps->ColumBoundary[i]; x < pPps->ColumBoundary[i + 1]; x++)
                {
                    pPps->TileIdInRs[y * pSps->nCtbWidth + x] = nTileId;
                    pPps->TileId[pPps->CtbAddrRsToTs[y * pSps->nCtbWidth + x]] = nTileId;
                }
        }

    pPps->TilePositionRs = HevcCalloc(nTileId, sizeof(s32));
    if(!pPps->TilePositionRs)
    {
        loge(" h265 decode pps calloc error, tile context 2");
        HEVC_DECODE_PPS_ERROR3();
    }

    for(j = 0; j < pPps->nNumTileRows; j++)
        for(i = 0; i < pPps->nNumTileColums; i++)
            pPps->TilePositionRs[j * pPps->nNumTileColums + i] =
                    pPps->RowBoundary[j] * pSps->nCtbWidth + pPps->ColumBoundary[i];

HevcDecodePpsExit:
    if(pHevcDec->PpsList[nPpsId] != NULL)
    {
        HevcPPS *pTempPps = pHevcDec->PpsList[nPpsId];
        HevcFreePpsBuffer(pTempPps);
        HevcFree(&pHevcDec->PpsList[nPpsId]);
        logv(" free pps(%d) ", nPpsId);
    }
    pHevcDec->PpsList[nPpsId] = pPps;
    pHevcDec->bHadFindPpsFlag = HEVC_TRUE;
    HevcCabacTrace(pGb, "-------- decode nal pps end --------");
    return 0;
#undef HEVC_DECODE_PPS_ERROR
#undef HEVC_DECODE_PPS_ERROR2
#undef HEVC_DECODE_PPS_ERROR3
}

static s32 HevcComputePoc(HevcContex *pHevcDec, s32 nPicOrderCntLsb)
{
    s32 nMaxPocLsb  = 1 << pHevcDec->pSps->nLog2MaxPocLsb;
    s32 nPrevPocLsb = pHevcDec->nPocTid0 % nMaxPocLsb;
    s32 nPrevPocMsb = pHevcDec->nPocTid0 - nPrevPocLsb;
    s32 nPocMsb;

    if (nPicOrderCntLsb < nPrevPocLsb && nPrevPocLsb - nPicOrderCntLsb >= nMaxPocLsb / 2)
        nPocMsb = nPrevPocMsb + nMaxPocLsb;
    else if (nPicOrderCntLsb > nPrevPocLsb && nPicOrderCntLsb - nPrevPocLsb > nMaxPocLsb / 2)
        nPocMsb = nPrevPocMsb - nMaxPocLsb;
    else
        nPocMsb = nPrevPocMsb;

    // For BLA picture types, POCmsb is set to 0.
    if (pHevcDec->eNaluType == HEVC_NAL_BLA_W_LP   ||
        pHevcDec->eNaluType == HEVC_NAL_BLA_W_RADL ||
        pHevcDec->eNaluType == HEVC_NAL_BLA_N_LP)
        nPocMsb = 0;

    return nPocMsb + nPicOrderCntLsb;
}

static void HevcParseSeiPicHash(HevcContex *pHevcDec)
{
    s32 i, cIdx;
    u8 nHashType;
    GetBitContext *pGb = &pHevcDec->getBit;

    nHashType = HevcGetNBits(8, pGb, "hash_type");
    for(cIdx = 0; cIdx < 3; cIdx++)
    {
        if(nHashType == 0)
        {
            for(i = 0; i < 16; i++)
                pHevcDec->md5[cIdx][i] = HevcGetNBits(8, pGb, "picture_md5[cIdx][i]");
            pHevcDec->bMd5AvailableFlag |= HEVC_MD5_PARSED;
        }
        else if(nHashType == 1)
            HevcGetNBits(16, pGb, "picture_crc[cIdx]");
        else if(nHashType == 2)
            HevcGetNBitsL(32, pGb, "picture_checksum[cIdx]");
    }
    /*
    HevcGetUe(pGb, "");
    HevcGetSe(pGb, "");
    HevcGetOneBit(pGb, "");
    HevcGetNBits(6, pGb, "");
     * */
}

#if 0
static s32 HevcActiveParameterSets(HevcContex  *pHevcDec)
{
    GetBitContext *pGb = &pHevcDec->getBit;
    u32 nSpsIdsMinus1;
    int i;
    //u32 activeSeqParaSetId;
    HevcGetNBits(4, pGb, "active_video_parameter_set_id");
    HevcGetOneBit(pGb, "self_contained_cvs_flag");
    HevcGetOneBit(pGb, "num_sps_ids_minus1");
    nSpsIdsMinus1 = HevcGetUe(pGb, "num_sps_ids_minus1");

    if(nSpsIdsMinus1 < 0 || nSpsIdsMinus1 > 15)
    {
        loge("num_sps_ids_minus1 = %d, is invalid", nSpsIdsMinus1);
        return -1;
    }
    for(i = 0; i <= nSpsIdsMinus1; i++)
    {
        HevcGetUe(pGb, "active_seq_parameter_set_id[i]");
    }
    return 0;
}
#endif
static void HevcParseSeiMstDisColVol(HevcContex *pHevcDec)
{
    s32 c;
    GetBitContext *pGb = &pHevcDec->getBit;
    for(c = 0; c < 3; c++)
    {
        pHevcDec->SeiDisInfo.displayPrimariesX[c]
            = HevcGetNBits(16, pGb, "display_primariex_x[c]");
        pHevcDec->SeiDisInfo.displayPrimariesY[c]
            = HevcGetNBits(16, pGb, "display_primariex_y[c]");
    }
    pHevcDec->SeiDisInfo.whitePointX = HevcGetNBits(16, pGb, "white_point_x");
    pHevcDec->SeiDisInfo.whitePointY = HevcGetNBits(16, pGb, "white_point_y");
    pHevcDec->SeiDisInfo.maxMasteringLuminance
        = HevcGetNBits(32, pGb, "max_display_mastering_luminance");
    pHevcDec->SeiDisInfo.minMasteringLuminance
        = HevcGetNBits(32, pGb, "min_display_mastering_luminance");

    // As this SEI message comes before the first frame that references it,
    // initialize the flag to 2 and decrement on IRAP access unit so it
    // persists for the coded video sequence (e.g., between two IRAPs)
    //pHevcDec->SeiDisInfo.seiMstDisInfoPresent = 2;
}

static s32 HevcParseNalSeiMessage(HevcContex *pHevcDec)
{
    GetBitContext *pGb = &pHevcDec->getBit;
    s32 nPlayLoadType, nPlayLoadSize, nByte;
    nPlayLoadType = 0;
    nPlayLoadSize = 0;

    nByte = 0xff;
    while(nByte == 0xff)
    {
        nByte = HevcGetNBits(8, pGb, "ff_byte");
        nPlayLoadType += nByte;
    }
    nByte = 0xff;
    while(nByte == 0xff)
    {
        nByte = HevcGetNBits(8, pGb, "ff_byte");
        nPlayLoadSize += nByte;
    }
    if(pHevcDec->eNaluType == HEVC_NAL_SEI_PREFIX)
    {
        logv("Sei decoding %d", nPlayLoadType);
        if(nPlayLoadType == 256)
        {
            /* decode_nal_sei_decoded_picture_hash */
            HevcParseSeiPicHash(pHevcDec);
        }
        else if(nPlayLoadType == 137)
        {
            /*decode_mastering_display_colour_volume*/
            pHevcDec->SeiDisInfo.bHdrFlag = 1;
            HevcParseSeiMstDisColVol(pHevcDec);
        }
        else if(nPlayLoadType == 144)
        {
        /*decode_content_light_level*/
            pHevcDec->SeiDisInfo.bHdrFlag = 1;
            pHevcDec->SeiDisInfo.maxContentLightLevel
                = HevcGetNBits(16, pGb, "max_content_light_level");
            pHevcDec->SeiDisInfo.maxPicAverLightLevel
                = HevcGetNBits(16, pGb, "max_pic_average_light_level");
        }
        else if(nPlayLoadType == 147)
        {
            pHevcDec->SeiDisInfo.prefTranChara
                = HevcGetNBits(8, pGb, "preferred_transfer_characteristics");
        }
        else if(nPlayLoadType == 45)
        {
            /* decode_nal_sei_frame_packing_arrangement */
            HevcSkipBits(pGb, 8 * nPlayLoadSize, " SeiPlayLoadTye45 ");
        }
        else if(nPlayLoadType == 1)
        {
            /* decode_pic_timing */
            HevcSkipBits(pGb, 8 * nPlayLoadSize, " SeiPlayLoadTye1 ");
        }
        else if(nPlayLoadType == 129)
        {
            /* active_parameter_sets */
    #if 1
            HevcSkipBits(pGb, 8 * nPlayLoadSize, " SeiPlayLoadTye129 ");
    #else
            return HevcActiveParameterSets(pHevcDec);
        #endif
        }
        else
        {
            HevcSkipBits(pGb, 8 * nPlayLoadSize, " unsupported ");
        logv("Skipped PREFIX SEI %d", nPlayLoadType);
            return HEVC_SEI_UNSUPPORTED;
        }
    }
    else
    {
        if (nPlayLoadType == 132)
        {
            /* decode_nal_sei_decoded_picture_hash; */
            HevcParseSeiPicHash(pHevcDec);
        }
        else
        {
            HevcSkipBits(pGb, 8 * nPlayLoadSize, " SeiPlayLoadTyeUnknowed ");
        }
    }

    return 0;
}

s32 HevcDecodeNalSei(HevcContex *pHevcDec)
{
    s32 nRet;
    GetBitContext *pGb = &pHevcDec->getBit;
    do{
        nRet = HevcParseNalSeiMessage(pHevcDec);
        if(nRet < 0)
        {
            loge(" h265 decode SEI nal error ");
            return -1;
        }
       /*
         else if(nRet == HEVC_SEI_UNSUPPORTED)
        {
            return 0;
        }
      */
    }while(HevcMoreRbspData(pGb));
    return 0;
}

void HevcSliceHeaderDefaultValue(HevcSliceHeader *pSh)
{
    pSh->eSliceType = HEVC_I_SLICE;
    pSh->bCollocatedFromL0Flag = 1;
    pSh->nMaxNumMergeCand = 5;
//    pSh->nChromaLog2WeightDenom = 5;
//    pSh->nLumaLog2WeightDenom = 5;
    /* other value is 0 */
}

s32 HevcDecodeSliceHeader(HevcContex *pHevcDec)
{
    GetBitContext *pGb;
    s32 i, j, nTemp, nRet;
    s32 bIsSAOEnabled, bIsDBFEnabled;
    HevcSliceHeader *pSh;
    HevcSPS *pCurSps = NULL;

    pSh = &pHevcDec->SliceHeader;
    pGb = &pHevcDec->getBit;

    HevcCabacTrace(pGb, "-------- decode slice header start --------");

    pSh->bFirstSliceInPicFlag = HevcGetOneBit(pGb,"first_slice_segment_in_pic_flag");

    if(pSh->bFirstSliceInPicFlag)
    {
        pSh->bCabacInitFlag = 0;
        pSh->bCollocatedFromL0Flag = 1;
        pSh->nNumEntryPointOffsets = 0;
        pSh->nSliceCbQpOffset = 0;
        pSh->nSliceCrQpOffset = 0;
        pSh->nMaxNumMergeCand = HEVC_MRG_MAX_NUM_CANDS;
        pSh->bSliceTemporalMvpEnableFlag = 1;
        pSh->nCollocatedRefIdx = 0;
        pSh->bIsNotBlowDelayFlag = 0;
    }
    if ((HEVC_IS_IDR(pHevcDec->eNaluType) || HEVC_IS_BLA(pHevcDec)) && pSh->bFirstSliceInPicFlag)
    {
        pHevcDec->nSeqDecode = (pHevcDec->nSeqDecode + 1) & 0xff;
        pHevcDec->nMaxRa = HEVC_INT_MAX;
        if(HEVC_IS_IDR(pHevcDec->eNaluType))
        {
            HevcClearRefs(pHevcDec);
        }
    }

    pSh->bNoOutPutOfPriorPicsFlag = 0;
    if(pHevcDec->eNaluType >= 16 && pHevcDec->eNaluType <= 23)
        pSh->bNoOutPutOfPriorPicsFlag = HevcGetOneBit(pGb,"no_output_of_prior_pics_flag");
    /*
     if (s->nal_unit_type == NAL_CRA_NUT && s->last_eos == 1)
        sh->no_output_of_prior_pics_flag = 1;
     */
    pSh->nPpsId = HevcGetUe(pGb, "slice_pic_parameter_set_id");
    if(pSh->nPpsId >= HEVC_MAX_PPS_NUM || pHevcDec->PpsList[pSh->nPpsId] == NULL)
    {
        loge(" h265 decode slice header error. pps id(%d) out of range ", pSh->nPpsId);
        return -1;
    }
    if(!pSh->bFirstSliceInPicFlag && pHevcDec->pPps != pHevcDec->PpsList[pSh->nPpsId])
    {
        loge(" h265 decode slice header error. pps change between slices ");
    }

    pHevcDec->pPps = pHevcDec->PpsList[pSh->nPpsId];
    pCurSps = pHevcDec->SpsList[pHevcDec->pPps->nSpsId];
    if(pCurSps == NULL)
    {
        loge("*** the sps is null when had pps");
        return -1;
    }

    if(pHevcDec->pSps == NULL)
    {
        pHevcDec->pSps = HevcCalloc(1, sizeof(HevcSPS));
        if(pHevcDec->pSps == NULL)
        {
            loge("***malloc failed***");
            return -1;
        }
        memcpy(pHevcDec->pSps, pCurSps, sizeof(HevcSPS));
        HevcClearRefs(pHevcDec);
        pHevcDec->nSeqDecode = (pHevcDec->nSeqDecode + 1) & 0xff;
        pHevcDec->nMaxRa = HEVC_INT_MAX;
        pHevcDec->bPreHdrFlag = pHevcDec->SeiDisInfo.bHdrFlag;
    }
    else
    {
        if(memcmp(pHevcDec->pSps, pCurSps, sizeof(HevcSPS)) != 0)
        {
            s32 nPreWidht  = pHevcDec->pSps->nWidth;
            s32 nPreHeight = pHevcDec->pSps->nHeight;
            s32 nIndex = pHevcDec->pSps->nMaxSubLayers - 1;
            s32 nPreMaxNumReoderPics = pHevcDec->pSps->TemporalLayer[nIndex].nSpsMaxNumReoderPics;
            s32 nPreMaxDecPicBuffers = pHevcDec->pSps->TemporalLayer[nIndex].nSpsMaxDecPicBuffering;

            memcpy(pHevcDec->pSps, pCurSps, sizeof(HevcSPS));
            nIndex = pHevcDec->pSps->nMaxSubLayers - 1;
            s32 nMaxNumReoderPics = pHevcDec->pSps->TemporalLayer[nIndex].nSpsMaxNumReoderPics;
            s32 nMaxDecPicBuffers = pHevcDec->pSps->TemporalLayer[nIndex].nSpsMaxDecPicBuffering;

            logv("pre ref buffer num: %d, %d, cur: %d, %d",
                  nPreMaxDecPicBuffers, nPreMaxNumReoderPics,
                  nMaxDecPicBuffers, nMaxNumReoderPics);

            if(pHevcDec->bPreHdrFlag != -1
               && pHevcDec->bPreHdrFlag != pHevcDec->SeiDisInfo.bHdrFlag)
            {
                logd(" hdr-flag change, call back size change: %d, %d",
                     pHevcDec->bPreHdrFlag, pHevcDec->SeiDisInfo.bHdrFlag);
                pHevcDec->bPreHdrFlag = pHevcDec->SeiDisInfo.bHdrFlag;
                return HEVC_RESULT_PICTURES_SIZE_CHANGE;
            }
            pHevcDec->bPreHdrFlag = pHevcDec->SeiDisInfo.bHdrFlag;

            if(nPreWidht != pHevcDec->pSps->nWidth
               || nPreHeight != pHevcDec->pSps->nHeight)
            {
                logw(" h265 picture size change. old size: %dx%d, new size: %dx%d ",
                        nPreWidht, nPreHeight,
                        pHevcDec->pSps->nWidth,pHevcDec->pSps->nHeight);
                return HEVC_RESULT_PICTURES_SIZE_CHANGE;
            }

            if(nPreMaxNumReoderPics != nMaxNumReoderPics
               || nPreMaxDecPicBuffers != nMaxDecPicBuffers)
            {
                logw("h265 ref buffer num change. old: %d, %d, new : %d, %d",
                     nPreMaxDecPicBuffers, nPreMaxNumReoderPics,
                     nMaxDecPicBuffers, nMaxNumReoderPics);
                return HEVC_RESULT_PICTURES_SIZE_CHANGE;
            }
        }
    }

    pSh->bDependentSliceSegmentFlag = 0;
    if(!pSh->bFirstSliceInPicFlag)
    {
        s32 nSliceAddressLength;
        if(pHevcDec->pPps->bDependentSliceSegmentsEnabledFlag)
            pSh->bDependentSliceSegmentFlag = HevcGetOneBit(pGb,"dependent_slice_segment_flag");
        nSliceAddressLength = HevcCeilLog2(pHevcDec->pSps->nCtbWidth * pHevcDec->pSps->nCtbHeight);
        pSh->nSliceSegmentAddr = HevcGetNBits(nSliceAddressLength, pGb, "slice_segment_address");
        if(pSh->nSliceSegmentAddr > pHevcDec->pSps->nCtbSize)
        {
            loge(" h265 decode slice header error. slice_segment_address is out of range ");
        }
        if(!pSh->bDependentSliceSegmentFlag)
        {
            pSh->nSliceAddr = pSh->nSliceSegmentAddr;
            pHevcDec->nSliceIdx++;
        }
    }
    else
    {
        pSh->nSliceAddr = pSh->nSliceSegmentAddr = 0;
        pHevcDec->nSliceIdx = 0;
        pHevcDec->bSliceInitialized = 0;
    }
    pSh->bNextSlice = !pSh->bDependentSliceSegmentFlag;
    if(!pSh->bDependentSliceSegmentFlag)
    {
        pHevcDec->bSliceInitialized = 0;
        for(i = 0; i < pHevcDec->pPps->nNumExtraSliceHeaderBits; i++)
            HevcSkipBits(pGb, 1, "slice_reserved_flag[i]");
        pSh->eSliceType = HevcGetUe(pGb, "slice_type");

        logv("******* pSh->eSliceType = %d",pSh->eSliceType);
        if(pSh->eSliceType != HEVC_B_SLICE &&
           pSh->eSliceType != HEVC_P_SLICE &&
           pSh->eSliceType != HEVC_I_SLICE)
        {
            loge(" h265 decode slice header error. unknown slice type : %d", pSh->eSliceType);
            pHevcDec->sliceType = -1;

        }
        else
        {
            switch(pSh->eSliceType)
            {
                case HEVC_B_SLICE:
                     pHevcDec->sliceType = HEVC_B_SLICE;
                     break;
                case HEVC_P_SLICE:
                     if(pHevcDec->sliceType == HEVC_B_SLICE)
                 pHevcDec->sliceType = HEVC_B_SLICE;
                     else
                 pHevcDec->sliceType = HEVC_P_SLICE;
                     break;
                case HEVC_I_SLICE:
                     if(pHevcDec->sliceType == HEVC_B_SLICE)
                         pHevcDec->sliceType = HEVC_B_SLICE;
                     else if(pHevcDec->sliceType == HEVC_P_SLICE)
                         pHevcDec->sliceType = HEVC_P_SLICE;
                     else
                         pHevcDec->sliceType = HEVC_I_SLICE;
                     break;
            }
        }
        logv("******* pSh->eSliceType = %d, pHevcDec->sliceType = %d ",
               pSh->eSliceType, pHevcDec->sliceType);

        /*
        if (!s->decoder_id && IS_IRAP(s) && sh->slice_type != HEVC_I_SLICE) {
            av_log(s->avctx, AV_LOG_ERROR, "Inter slices in an IRAP frame.\n");
            return AVERROR_INVALIDDATA;
        }
         * */
        pSh->bPicOutputFlag = 1;
        if(pHevcDec->pPps->bOutputFlagPresentFlag)
            pSh->bPicOutputFlag = HevcGetOneBit(pGb,"pic_output_flag");
        if(pHevcDec->pSps->bSeparateColourPlaneFlag)
            pSh->nColourPlaneId = HevcGetNBits(2, pGb, "colour_plane_id");
        if(!HEVC_IS_IDR(pHevcDec->eNaluType))
        {
            s32 bShortTermRefPicSetSpsFlag, nPoc;
            pSh->nPicOrderCntLsb =
                HevcGetNBits(pHevcDec->pSps->nLog2MaxPocLsb, pGb, "slice_pic_order_cnt_lsb");
            nPoc = HevcComputePoc(pHevcDec, pSh->nPicOrderCntLsb);
            if(!pSh->bFirstSliceInPicFlag && nPoc != pHevcDec->nPoc)
            {
                logw(" h265 decoder slice header. POC change between slices in the same picture ");
                nPoc = pHevcDec->nPoc;
            }
            pHevcDec->nPoc = nPoc;

            bShortTermRefPicSetSpsFlag = HevcGetOneBit(pGb,"short_term_ref_pic_set_sps_flag");
            if(!bShortTermRefPicSetSpsFlag)
            {
                HevcParseShortTermRefSet(pHevcDec, &pSh->SliceRps, pHevcDec->pSps, 1);
                pSh->pShortTermRps = &pSh->SliceRps;
//                logd(" slice header: after HevcParseShortTermRefSet() ");
            }
            else
            {
                s32 nBits, nRpsIdx;
                nBits = HevcCeilLog2(pHevcDec->pSps->nNumStRps);
                nRpsIdx = nBits > 0 ? HevcGetNBits(nBits, pGb, "short_term_ref_pic_set_idx") : 0;
                if(nRpsIdx >= HEVC_MAX_SHORT_TERM_RPS_COUNT)
                {
                        logw("the nRpsIdx is overflow: nRpsIdx = %d, max = %d ",
                                 nRpsIdx, HEVC_MAX_SHORT_TERM_RPS_COUNT - 1);
                        return -1;
                }
                pSh->pShortTermRps = &pHevcDec->pSps->StRps[nRpsIdx];
//                logd(" slice header: pSh->pShortTermRps =
//                     &pHevcDec->pSps->StRps[%d], poc: %d", nRpsIdx, pHevcDec->nPoc);
            }
            HevcShowShortRefPicSetInfoDebug(pSh->pShortTermRps, pHevcDec->nPoc);
            nRet = HevcParseLongTermRefSet(pHevcDec, &pSh->LongTermRps);
            if(nRet < 0)
            {
                loge(" h265 decode slice header. decode long term rps error ");
            }

            if(pHevcDec->pSps->bSpsTemporalMvpEnableFlag)
                pSh->bSliceTemporalMvpEnableFlag =
                    HevcGetOneBit(pGb,"slice_temporal_mvp_enabled_flag");
            else
                pSh->bSliceTemporalMvpEnableFlag = 0;
        }
        else /* if !IDR */
        {
            pSh->pShortTermRps = NULL;
            pSh->LongTermRps.nNumOfRefs = 0;
            pSh->bSliceTemporalMvpEnableFlag = 0;
            pHevcDec->nPoc = 0;
        }
        if (pHevcDec->nTemporalId == 0 &&
            pHevcDec->eNaluType != HEVC_NAL_TRAIL_N &&
            pHevcDec->eNaluType != HEVC_NAL_TSA_N   &&
            pHevcDec->eNaluType != HEVC_NAL_STSA_N  &&
            pHevcDec->eNaluType != HEVC_NAL_RADL_N  &&
            pHevcDec->eNaluType != HEVC_NAL_RADL_R  &&
            pHevcDec->eNaluType != HEVC_NAL_RASL_N  &&
            pHevcDec->eNaluType != HEVC_NAL_RASL_R)
            pHevcDec->nPocTid0 = pHevcDec->nPoc;

        if(pHevcDec->pSps->bSaoEnabled)
        {
            pSh->bSliceSampleAdaptiveOffsetFlag[0] =
                HevcGetOneBit(pGb,"slice_sao_luma_flag");
            pSh->bSliceSampleAdaptiveOffsetFlag[1] =
            pSh->bSliceSampleAdaptiveOffsetFlag[2] =
                HevcGetOneBit(pGb,"slice_sao_chroma_flag");
        }

        pSh->nNumOfRefs[0] = pSh->nNumOfRefs[1] = 0;
        pSh->bCabacInitFlag = 0;
        if(pSh->eSliceType == HEVC_P_SLICE || pSh->eSliceType == HEVC_B_SLICE)
        {
            s32 nNumRefs, bFlag;
            pSh->nNumOfRefs[0] = pHevcDec->pPps->nNumRefIdxL0DefaultActive;
            if(pSh->eSliceType == HEVC_B_SLICE)
                pSh->nNumOfRefs[1] = pHevcDec->pPps->nNumRefIdxL1DefaultActive;
            bFlag = HevcGetOneBit(pGb,"num_ref_idx_active_override_flag");
            if(bFlag)
            {
                pSh->nNumOfRefs[0] = HevcGetUe(pGb, "num_ref_idx_l0_active_minus1") + 1;
                if(pSh->eSliceType == HEVC_B_SLICE)
                    pSh->nNumOfRefs[1] = HevcGetUe(pGb, "num_ref_idx_l0_active_minus1") + 1;
            }
            if(pSh->nNumOfRefs[0] > HEVC_MAX_REFS || pSh->nNumOfRefs[1] > HEVC_MAX_REFS)
            {
                loge(" h265 slice header error. The ref pictures num is out of range. \
                    L0: %d, L1: %d ",
                    pSh->nNumOfRefs[0], pSh->nNumOfRefs[1]);
            }
            pSh->bRplModificationFlag[0] = 0;
            pSh->bRplModificationFlag[1] = 0;

            nNumRefs = HevcComputeNumOfRefs(pHevcDec);
            if(nNumRefs == 0)
            {
                loge(" h265 decode slice header error. \
                    Number of refs == 0 in P or B slice ");
            }
            if(pHevcDec->pPps->bListsModificationPresentFlag && nNumRefs > 1)
            {
                s32 nBitsOfRefs = HevcCeilLog2(nNumRefs);
                pSh->bRplModificationFlag[0] =
                    HevcGetOneBit(pGb,"ref_pic_list_modification_flag_l0");
                if(pSh->bRplModificationFlag[0])
                    for(i = 0; i < pSh->nNumOfRefs[0]; i++)
                        pSh->ListEntryLx[0][i] =
                        HevcGetNBits(nBitsOfRefs, pGb, "list_entry_l0[i]");
                if(pSh->eSliceType == HEVC_B_SLICE)
                {
                    pSh->bRplModificationFlag[1] =
                        HevcGetOneBit(pGb,"ref_pic_list_modification_flag_l1");
                    if(pSh->bRplModificationFlag[1])
                        for(i = 0; i < pSh->nNumOfRefs[1]; i++)
                            pSh->ListEntryLx[1][i] =
                                HevcGetNBits(nBitsOfRefs, pGb, "list_entry_l0[i]");
                }
            }
            if(pSh->eSliceType == HEVC_B_SLICE)
                pSh->bMvdL1ZeroFlag = HevcGetOneBit(pGb,"mvd_l1_zero_flag");
            if(pHevcDec->pPps->bCabacInitPresentFlag)
                pSh->bCabacInitFlag = HevcGetOneBit(pGb,"cabac_init_flag");

            if(pSh->bSliceTemporalMvpEnableFlag)
            {
                pSh->bCollocatedList = 0;
                if(pSh->eSliceType == HEVC_B_SLICE)
                {
                    pSh->bCollocatedFromL0Flag =
                        HevcGetOneBit(pGb,"collocated_from_l0_flag");
                    pSh->bCollocatedList = !pSh->bCollocatedFromL0Flag;
                }
                else
                    pSh->bCollocatedFromL0Flag = 1;

                if(pSh->eSliceType != HEVC_I_SLICE &&
                    ((pSh->bCollocatedFromL0Flag == 1 && pSh->nNumOfRefs[0] > 1) ||
                     (pSh->bCollocatedFromL0Flag == 0 && pSh->nNumOfRefs[1] > 1)))
                {
                    pSh->nCollocatedRefIdx = HevcGetUe(pGb, "collocated_ref_idx");
                    if(pSh->nCollocatedRefIdx > pSh->nNumOfRefs[pSh->bCollocatedList])
                    {
                        loge(" h265 decode slice header error. \
                            Collocated ref idx(%d) invalid ",
                            pSh->nCollocatedRefIdx);
                    }
                }
                else
                {
                    pSh->nCollocatedRefIdx = 0;
                }
            }
            if((pHevcDec->pPps->bWeightedPredFlag && pSh->eSliceType == HEVC_P_SLICE) ||
                (pHevcDec->pPps->bWeightedBipredFlag && pSh->eSliceType == HEVC_B_SLICE))
            {
//                HevcParseWeightTable(pHevcDec);
                HevcParseWeightTable2(pHevcDec);
                HevcInitWpScaling(&pHevcDec->SliceHeader,
                    pHevcDec->pSps->nBitDepthLuma, pHevcDec->pSps->nBitDepthChroma);
            }

            pSh->nMaxNumMergeCand = HEVC_MRG_MAX_NUM_CANDS -
                HevcGetUe(pGb, "five_minus_max_num_merge_cand");
            if(pSh->nMaxNumMergeCand < 1 || pSh->nMaxNumMergeCand > 5)
            {
                loge(" h265 decode slice header error. \
                    merge candidates number(%d) invalid ",
                    pSh->nMaxNumMergeCand);
            }
        }
        else
        {
            pSh->nCollocatedRefIdx = 0;
            if(pSh->bSliceTemporalMvpEnableFlag)
                pSh->bCollocatedFromL0Flag = 1;
        }
        pSh->nSliceQpDelta = HevcGetSe(pGb, "slice_qp_delta");
        if(pHevcDec->pPps->bPpsSliceChromaQpOffsetsPresentFlag)
        {
            pSh->nSliceCbQpOffset = HevcGetSe(pGb, "slice_cb_qp_offset");
            pSh->nSliceCrQpOffset = HevcGetSe(pGb, "slice_cr_qp_offset");
        }
        if(pHevcDec->pPps->bDeblockingFilterControlPresentFlag)
        {
            s32 bDeblokingFilterOverrideFlag = 0;
            if(pHevcDec->pPps->bDeblocingFilterOverrideEnableFlag)
                bDeblokingFilterOverrideFlag =
                HevcGetOneBit(pGb,"deblocking_filter_override_flag");
            if(bDeblokingFilterOverrideFlag)
            {
                pSh->bSlicDeblockingFilterDisableFlag =
                    HevcGetOneBit(pGb,"slice_deblocking_filter_disabled_flag");
                if(!pSh->bSlicDeblockingFilterDisableFlag)
                {
                    pSh->nBetaOffset = HevcGetSe(pGb, "slice_beta_offset_div2") * 2;
                    pSh->nTcOffset = HevcGetSe(pGb, "slice_tc_offset_div2") * 2;
                }
            }
            else
            {
                pSh->bSlicDeblockingFilterDisableFlag =
                    pHevcDec->pPps->bPpsDeblockingDisableFlag;
                pSh->nBetaOffset = pHevcDec->pPps->nPpsBetaOffset;
                pSh->nTcOffset = pHevcDec->pPps->nPpsTcOffset;
            }
        }
        else
        {
            pSh->bSlicDeblockingFilterDisableFlag =0;
            pSh->nBetaOffset = 0;
            pSh->nTcOffset = 0;
        }

        bIsSAOEnabled = (!pHevcDec->pSps->bSaoEnabled) ? 0 :
                            (pSh->bSliceSampleAdaptiveOffsetFlag[0] ||
                            pSh->bSliceSampleAdaptiveOffsetFlag[1]);
        bIsDBFEnabled = !pSh->bSlicDeblockingFilterDisableFlag;
        if(pHevcDec->pPps->bPpsLoopFilterAcrossSlicesEnableFlag &&
            (bIsSAOEnabled || bIsDBFEnabled))
            pSh->bSliceLoopFilterAcrossSliceEnableFlag =
            HevcGetOneBit(pGb,"slice_loop_filter_across_slices_enabled_flag");
        else
            pSh->bSliceLoopFilterAcrossSliceEnableFlag =
                    pHevcDec->pPps->bPpsLoopFilterAcrossSlicesEnableFlag;
    } /* if(!pSh->bDependentSliceSegmentFlag) */
    else if(!pHevcDec->bSliceInitialized)
    {
        loge(" h265 decoder slice header error. Independent slice segment missing ");
    }
    if(pHevcDec->pPps->bTilesEnabledFlag || pHevcDec->pPps->bEntropyCodingSyncEnabledFlag)
    {
        s32 nPreNumEntryPointOffsets = pSh->nNumEntryPointOffsets;
        pSh->nNumEntryPointOffsets = HevcGetUe(pGb, "num_entry_point_offsets");
        if(pHevcDec->pPps->bEntropyCodingSyncEnabledFlag)
        {
            if(pSh->nNumEntryPointOffsets > pHevcDec->pSps->nCtbHeight ||
                pSh->nNumEntryPointOffsets < 0)
            {
                loge(" h265 decode slice error.  \
                    num_entry_point_offsets(%d) out of range",
                    pSh->nNumEntryPointOffsets);
            }
        }
        else
        {
            if(pSh->nNumEntryPointOffsets > pHevcDec->pSps->nCtbSize ||
                pSh->nNumEntryPointOffsets < 0)
            {
                loge(" h265 decode slice error.  \
                    num_entry_point_offsets(%d) out of range",
                    pSh->nNumEntryPointOffsets);
            }
        }

        if(pSh->nNumEntryPointOffsets > 0)
        {
            s32 nOffsetLen, nSegments, nRest;
            if(nPreNumEntryPointOffsets < pSh->nNumEntryPointOffsets)
            {
                HevcFree(&pSh->EntryPointOffset);
                HevcFree(&pSh->Size);
                HevcFree(&pSh->Offset);
                pSh->EntryPointOffset =
                    HevcCalloc(pSh->nNumEntryPointOffsets, sizeof(s32));
                pSh->Size = HevcCalloc(pSh->nNumEntryPointOffsets, sizeof(s32));
                pSh->Offset = HevcCalloc(pSh->nNumEntryPointOffsets, sizeof(s32));
                if(pSh->EntryPointOffset == NULL ||
                    pSh->Size == NULL ||
                    pSh->Offset == NULL)
                {
                    loge(" h265 decode slice error. calloc memory fail");
                }
            }
            nOffsetLen = HevcGetUe(pGb, "offset_len_minus1") + 1;
            nSegments = nOffsetLen >> 4;
            nRest = (nOffsetLen & 15);
            for(i = 0; i < pSh->nNumEntryPointOffsets; i++)
            {
                nTemp = 0;
                for(j = 0; j < nSegments; j++)
                {
                    nTemp <<= 16;
                    nTemp +=
                        HevcGetNBits(16, pGb, "entry_point_offset_minus1[i] 16 bits");
                }
                if(nRest)
                {
                    nTemp <<= nRest;
                    nTemp +=
                        HevcGetNBits(nRest, pGb, "entry_point_offset_minus1[i] 16 bits");
                }
                pSh->EntryPointOffset[i] = nTemp + 1;
            }
        }
    }
    if(pHevcDec->pPps->bSliceHeaderExtentionPresentFlag)
    {
        nTemp = HevcGetUe(pGb, "slice_segment_header_extension_length");
        for(i = 0; i < nTemp; i++)
            HevcSkipBits(pGb, 8, "slice_segment_header_extension_data_byte");
        /* slice_segment_header_extension_data_byte */
    }
    /* ************** End of Slice Header Parsing ************** */
    HevcCabacTrace(pGb, "-------- decode slice header end --------");

    pSh->nSliceQp = 26 + pHevcDec->pPps->nInitQpMinus26 + pSh->nSliceQpDelta;
    if(pSh->nSliceQp > 51 || pSh->nSliceQp < -pHevcDec->pSps->nQpBdOffset)
    {
        loge(" h265 decode slice header error. qp(%d) out of range [%d, 51] ",
                pSh->nSliceQp, -pHevcDec->pSps->nQpBdOffset);
    }

    pSh->nSliceCtbAddrRs = pSh->nSliceSegmentAddr;
    if(pSh->nSliceCtbAddrRs == 0 && pSh->bDependentSliceSegmentFlag)
    {
        loge(" h265 decode slice header error. slice segment addr error ");
    }
    pHevcDec->bSliceInitialized = 1;

    return 0;
}

static void tmp(void)
{
#if 1

#endif

#if 1

#endif

}

