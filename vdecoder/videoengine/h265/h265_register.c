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
* File : h265_register.c
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#include "h265_func.h"
#include "h265_memory.h"
#include "h265_register.h"

// HEVC register variance
regHEVC_NAL_HDR               regHEVC_NalHdr_reg00;
regHEVC_SPS                   regHEVC_SPS_reg04;
regHEVC_PIC_SIZE              regHEVC_PicSize_reg08;
regHEVC_PCM_CTRL              regHEVC_PCM_Ctrl_reg0c;
regHEVC_PPS_CTRL0_VERSION01   regHEVC_PPS_Ctrl0_reg10_v01;
regHEVC_PPS_CTRL0_VERSION02   regHEVC_PPS_Ctrl0_reg10_v02;
regHEVC_PPS_CTRL1             regHEVC_PPS_Ctrl1_reg14;
regHEVC_SCALING_LIST_CTRL0    regHEVC_ScalingListCtrl0_reg18;
regHEVC_SCALING_LIST_CTRL1    regHEVC_ScalingListCtrl1_reg1c;
regHEVC_SLICE_HDR_INFO0       regHEVC_SliceHdrInfo0_reg20;
regHEVC_SLICE_HDR_INFO1_V01   regHEVC_SliceHdrInfo1_reg24_v01;
regHEVC_SLICE_HDR_INFO1_V02   regHEVC_SliceHdrInfo1_reg24_v02;
regHEVC_SLICE_HDR_INFO2       regHEVC_SliceHdrInfo2_reg28;
regHEVC_CTB_ADDR_V01          regHEVC_CTBAddr_reg2C_v01;
regHEVC_CTB_ADDR_V02          regHEVC_CTBAddr_reg2C_v02;
regHEVC_FUNC_CTRL             regHEVC_FunctionCtrol_reg30;
regHEVC_TRIGGER_TYPE          regHEVC_TriggerType_reg34;
regHEVC_FUNCTION_STATUS       regHEVC_FunctionStatus_reg38;
regHEVC_DEC_CTB_NUM           regHEVC_DecCUTNum_reg3c;
regHEVC_BITS_BASE_ADDR        regHEVC_BitsBaseAddr_reg40;
regHEVC_BITS_OFFSET           regHEVC_BitsOffset_reg44;
regHEVC_BITS_LEN              regHEVC_BitsLength_reg48;
regHEVC_BITS_END_ADDR         regHEVC_BitsEndAddr_reg4c;
regHEVC_EXTRA_CTRL            regHEVC_ExtraCtrl_reg50;
regHEVC_BUF_ADDR              regHEVC_ExtraYBuf_reg54;
regHEVC_BUF_ADDR              regHEVC_ExtraCBuf_reg58;
regHEVC_RecFrmBufIdx          regHEVC_CurRecBufIdx_reg5c;
regHEVC_BUF_ADDR              regHEVC_NeighborInfoBufAddr_reg60;
regHEVC_CTB_ADDR_V01          regHEVC_TileStartCTBAddr_reg68_v01;
regHEVC_CTB_ADDR_V02          regHEVC_TileStartCTBAddr_reg68_v02;
regHEVC_CTB_ADDR_V01          regHEVC_TileEndCTBAddr_reg6c_v01;
regHEVC_CTB_ADDR_V02          regHEVC_TileEndCTBAddr_reg6c_v02;
regHEVC_ERROR_CASE            regHEVC_ErrorCase_regb8;
regHEVC_8BIT_ADDR             regHEVC_8BIT_Addr_reg80;

regHEVC_LOWER_2BIT_ADDR_OF_FIRST_OUTPUT  regHEVC_lower_2bit_addr_first_output_reg84;
regHEVC_LOWER_2BIT_ADDR_OF_SECOND_OUTPUT regHEVC_lower_2bit_addr_second_output_reg88;
regHEVC_10BIT_CONFIGURE_REG              regHEVC_10bit_configure_reg8c;
SramFrameBufInfo              tempSramFrameBufInfo;
ReferenceList                 tempReferenceList;

#define HEVC_TRIGGER_SHOW_BITS 1
#define HEVC_TRIGGER_GET_BITS 2
#define HEVC_TRIGGER_GET_SE    4
#define HEVC_TRIGGER_GET_UE    5
#define HEVC_STCD_TYPE 0

/* return 0: on start code;
 * return len: start code bit offset, start from sbm buffer base address  */
u32 HevcHardwareFindStartCode(HevcContex *pHevcDec, s32 nSize)
{
    CEDARC_UNUSE(HevcHardwareFindStartCode);

    HevcStreamInfo *pStreamInfo = pHevcDec->pStreamInfo;
    size_addr RegBaseAddr = pHevcDec->HevcDecRegBaseAddr;
    u32 reg38Status;
    u32 nStartCodeOffset;
    volatile u32* pdwTemp = 0;
    s32 nBitLen, nBitOffset, nRet;
    regHEVC_FUNCTION_STATUS regFuctionStatusReg38_temp;

    nStartCodeOffset = 0;
    pdwTemp = (u32*)(&regHEVC_BitsEndAddr_reg4c);
    *pdwTemp = 0;
    regHEVC_BitsEndAddr_reg4c.bitstream_end_addr = pStreamInfo->pCurrentBufEndPhy >> 8;
    HevcDecWu32(RegBaseAddr + HEVC_BITS_END_ADDR_REG,
        *pdwTemp, "regHEVC_BitsEndAddr_reg4c");

    pdwTemp = (u32*)(&regHEVC_BitsLength_reg48);
    *pdwTemp = 0;
    nBitLen = nSize << 3;
    regHEVC_BitsLength_reg48.bits_len = nBitLen;
    HevcDecWu32(RegBaseAddr + HEVC_BITS_LEN_REG, *pdwTemp, "regHEVC_BitsLength_reg48");

    pdwTemp = (u32*)(&regHEVC_BitsOffset_reg44);
    *pdwTemp = 0;
    nBitOffset = pStreamInfo->nBitsOffset;
    regHEVC_BitsOffset_reg44.bits_offset = nBitOffset;
    HevcDecWu32(RegBaseAddr + HEVC_BITS_OFFSET_REG, *pdwTemp, "regHEVC_BitsOffset_reg44");

    pdwTemp = (u32*)(&regHEVC_BitsBaseAddr_reg40);
    *pdwTemp = 0;
    regHEVC_BitsBaseAddr_reg40.data_first = 1;
    regHEVC_BitsBaseAddr_reg40.data_last  = 0;
    regHEVC_BitsBaseAddr_reg40.data_valid = 1;
    regHEVC_BitsBaseAddr_reg40.bitstream_base_addr = pStreamInfo->pCurrentBufPhy >> 8;
    HevcDecWu32(RegBaseAddr + HEVC_BITS_BASE_ADDR_REG,
        *pdwTemp, "regHEVC_BitsBaseAddr_reg40");

    pdwTemp = (u32*)(&regHEVC_TriggerType_reg34);
    *pdwTemp = 0;
    regHEVC_TriggerType_reg34.trigger_type_pul = 0x7; // init_swdec
    HevcDecWu32(RegBaseAddr + HEVC_TRIGGER_TYPE_REG,
        *pdwTemp, "regHEVC_TriggerType_reg34");

    pdwTemp = (u32*)(&regHEVC_FunctionCtrol_reg30);
    *pdwTemp = 0;
    regHEVC_FunctionCtrol_reg30.finish_interrupt_en       = 1;
    regHEVC_FunctionCtrol_reg30.data_request_interrupt_en = 1;
    regHEVC_FunctionCtrol_reg30.error_interrupt_en        = 1;
    regHEVC_FunctionCtrol_reg30.startcode_detect_en          = 0;
    HevcDecWu32(RegBaseAddr + HEVC_FUNC_CTRL_REG,
        *pdwTemp, "regHEVC_FunctionCtrol_reg30");

    pdwTemp = (u32*)(&regHEVC_TriggerType_reg34);
    *pdwTemp = 0;
    regHEVC_TriggerType_reg34.trigger_type_pul = 0xc; // search start code
    HevcDecWu32(RegBaseAddr + HEVC_TRIGGER_TYPE_REG,
        *pdwTemp, "regHEVC_TriggerType_reg34");

    nRet = CdcVeWaitInterrupt(pHevcDec->pConfig->veOpsS, pHevcDec->pConfig->pVeOpsSelf);

    reg38Status = HevcDecRu32(RegBaseAddr + HEVC_FUNCTION_STATUS_REG,
        "HEVC_FUNCTION_STATUS_REG");
    regFuctionStatusReg38_temp = *((regHEVC_FUNCTION_STATUS*)(&reg38Status));
    if(nRet < 0)
    {
        loge(" hevc find start code. wait ve error ");
    }
    logd(" hevc register status: %8.8x ", reg38Status);

    /*
    pTemp = (u32*)(&reg38);
    *pTemp = 0;
    reg38.vld_data_req = 1;
    reg38.slice_dec_finish = 1;
    reg38.slice_dec_error = 1;
    */
    reg38Status &= 0x7;
    HevcDecWu32(RegBaseAddr + HEVC_FUNCTION_STATUS_REG,
        reg38Status, "regHEVC_FunctionStatus_reg38");
    logd("  hardware search start code. write status reg: %x", reg38Status);

    if(regFuctionStatusReg38_temp.vld_data_req == 1)
    {
        logd(" hardware search start code fail.... ");
        nStartCodeOffset = 0;
    }
    else if(regFuctionStatusReg38_temp.startcode_type == 1 ||
            regFuctionStatusReg38_temp.startcode_type == 2 )
    {
        nStartCodeOffset = HevcDecRu32(RegBaseAddr + HEVC_STCD_HW_BITOFFSET_REG,
            "HEVC_STCD_HW_BITOFFSET_REG");
    }
    else if(regFuctionStatusReg38_temp.startcode_type == 3)
    {
        //u32 tt;
        u32 t = HevcDecRu32(RegBaseAddr + HEVC_STCD_HW_BITOFFSET_REG,
            "HEVC_STCD_HW_BITOFFSET_REG");
        //tt = t - nBitOffset;
        logd(" hardware search start code type == 3. offset: %d", (t>>3));
        nStartCodeOffset = t;
    }
#if 1
    pdwTemp = (u32*)(&regHEVC_TriggerType_reg34);
    *pdwTemp = 0;
    regHEVC_TriggerType_reg34.trigger_type_pul = 0xd; // start code detect terminate
    HevcDecWu32(RegBaseAddr + HEVC_TRIGGER_TYPE_REG, *pdwTemp,
        "regHEVC_TriggerType_reg34");

//    pdwTemp = (u32*)(&regHEVC_DecCUTNum_reg3c);
//    *pdwTemp = 0;
//    HevcDecWu32(RegBaseAddr + HEVC_DECCTU_NUM_REG, *pdwTemp, "regHEVC_DecCUTNum_reg3c");
#endif
    return nStartCodeOffset;
}

/*
 * HevcInitialHwStreamDecode() function only used in hardware decode slice header
 * */
void HevcInitialHwStreamDecode(HevcContex *pHevcDec)
{
    HevcStreamInfo *pStreamInfo = pHevcDec->pStreamInfo;
    size_addr RegBaseAddr = pHevcDec->HevcDecRegBaseAddr;
    volatile u32* pdwTemp = 0;
    s32 nIndex, nSliceDataBitLen, nBitOffset;
    s32 nNaluDataSize = 0;

    if(pStreamInfo->pSbm->nType == SBM_TYPE_FRAME)
    {

        FramePicInfo* pStreamFramePic = pHevcDec->pStreamInfo->pCurStreamFrame;
        NaluInfo* pNaluInfo = &pStreamFramePic->pNaluInfoList[pStreamInfo->nBufIndex];
        nNaluDataSize = pNaluInfo->nDataSize;
    }
    else
    {
        nIndex = pStreamInfo->nBufIndex;
        nNaluDataSize = pStreamInfo->nDataSize[nIndex];
    }

    pdwTemp = (u32*)(&regHEVC_BitsEndAddr_reg4c);
    *pdwTemp = 0;
    regHEVC_BitsEndAddr_reg4c.bitstream_end_addr = pStreamInfo->pCurrentBufEndPhy >> 8;
    HevcDecWu32(RegBaseAddr + HEVC_BITS_END_ADDR_REG, *pdwTemp,
        "regHEVC_BitsEndAddr_reg4c");

    pdwTemp = (u32*)(&regHEVC_BitsLength_reg48);
    *pdwTemp = 0;
    nSliceDataBitLen = nNaluDataSize* 8;
//    nSliceDataBitLen = (nSliceDataBitLen + 4 + 3) & (0x3); /* todo: unkonw why ??? */
    regHEVC_BitsLength_reg48.bits_len = nSliceDataBitLen;
    HevcDecWu32(RegBaseAddr + HEVC_BITS_LEN_REG, *pdwTemp,
        "regHEVC_BitsLength_reg48");

    pdwTemp = (u32*)(&regHEVC_BitsOffset_reg44);
    *pdwTemp = 0;
    nBitOffset = pStreamInfo->nBitsOffset;
    regHEVC_BitsOffset_reg44.bits_offset = nBitOffset;
    HevcDecWu32(RegBaseAddr + HEVC_BITS_OFFSET_REG, *pdwTemp,
        "regHEVC_BitsOffset_reg44");

    pdwTemp = (u32*)(&regHEVC_BitsBaseAddr_reg40);
    *pdwTemp = 0;
    regHEVC_BitsBaseAddr_reg40.data_first = 1;
    regHEVC_BitsBaseAddr_reg40.data_last  = 1;
    regHEVC_BitsBaseAddr_reg40.data_valid = 1;
    regHEVC_BitsBaseAddr_reg40.bitstream_base_addr = pStreamInfo->pCurrentBufPhy >> 8;
    HevcDecWu32(RegBaseAddr + HEVC_BITS_BASE_ADDR_REG, *pdwTemp,
        "regHEVC_BitsBaseAddr_reg40");

    pdwTemp = (u32*)(&regHEVC_TriggerType_reg34);
    *pdwTemp = 0;
    regHEVC_TriggerType_reg34.trigger_type_pul = 0x07; // init_swdec
    HevcDecWu32(RegBaseAddr + HEVC_TRIGGER_TYPE_REG, *pdwTemp,
        "regHEVC_TriggerType_reg34");
}

u32 HevcGetbitsHardware(size_addr RegBaseAddr, u32 nBits)
{
    //volatile u32 dwVal;
    volatile u32* pdwTemp;
    regHEVC_FUNCTION_STATUS reg38;
    u32 value = 0, i;

    pdwTemp = (u32*)(&regHEVC_TriggerType_reg34);
    *pdwTemp = 0;
    regHEVC_TriggerType_reg34.trigger_type_pul = HEVC_TRIGGER_GET_BITS;
    regHEVC_TriggerType_reg34.n_bits = nBits;
    regHEVC_TriggerType_reg34.stcd_type = HEVC_STCD_TYPE;
    HevcDecWu32(RegBaseAddr + HEVC_TRIGGER_TYPE_REG, *pdwTemp,
        "regHEVC_TriggerType_reg34");

    memset(&reg38, 0, sizeof(regHEVC_FUNCTION_STATUS));
    pdwTemp = (u32*)(&reg38);
    i = 0;
    while(1)
    {
        *pdwTemp = HevcDecRu32(RegBaseAddr + HEVC_FUNCTION_STATUS_REG,
            "regHEVC_FunctionStatus_reg38");
        if(reg38.vld_busy)
        {
            i++;
            if(i > 0x200000)
            {
                logd("  HevcGetbitsHardware() over time return 0 ");
                return 0;
            }
        }
        else
        {
            value = HevcDecRu32(RegBaseAddr + HEVC_BITS_RETDATA_REG,
                "HEVC_BITS_RETDATA_REG");
            break;
        }
    }
    return value;
}

u32 HevcGetUEHardware(size_addr RegBaseAddr)
{
    //volatile u32 dwVal;
    volatile u32* pdwTemp;
    regHEVC_FUNCTION_STATUS reg38;
    u32 value = 0, i;

    pdwTemp = (u32*)(&regHEVC_TriggerType_reg34);
    *pdwTemp = 0;
    regHEVC_TriggerType_reg34.trigger_type_pul = HEVC_TRIGGER_GET_UE;
    regHEVC_TriggerType_reg34.stcd_type = HEVC_STCD_TYPE;
    HevcDecWu32(RegBaseAddr + HEVC_TRIGGER_TYPE_REG, *pdwTemp,
        "regHEVC_TriggerType_reg34");

    memset(&reg38, 0, sizeof(regHEVC_FUNCTION_STATUS));
    pdwTemp = (u32*)(&reg38);
    i = 0;
    while(1)
    {
        *pdwTemp = HevcDecRu32(RegBaseAddr + HEVC_FUNCTION_STATUS_REG,
            "regHEVC_FunctionStatus_reg38");
        if(reg38.vld_busy)
        {
            i++;
            if(i > 0x200000)
            {
                logd("  HevcGetbitsHardware() over time return 0 ");
                return 0;
            }
        }
        else
        {
            value = HevcDecRu32(RegBaseAddr + HEVC_BITS_RETDATA_REG,
                "HEVC_BITS_RETDATA_REG");
            break;
        }
    }
    return value;
}

s32 HevcGetSEHardware(size_addr RegBaseAddr)
{
    //volatile u32 dwVal;
    volatile u32* pdwTemp;
    regHEVC_FUNCTION_STATUS reg38;
    s32 value = 0, i;

    pdwTemp = (u32*)(&regHEVC_TriggerType_reg34);
    *pdwTemp = 0;
    regHEVC_TriggerType_reg34.trigger_type_pul = HEVC_TRIGGER_GET_SE;
    regHEVC_TriggerType_reg34.stcd_type = HEVC_STCD_TYPE;
    HevcDecWu32(RegBaseAddr + HEVC_TRIGGER_TYPE_REG, *pdwTemp,
        "regHEVC_TriggerType_reg34");

    memset(&reg38, 0, sizeof(regHEVC_FUNCTION_STATUS));
    pdwTemp = (u32*)(&reg38);
    i = 0;
    while(1)
    {
        *pdwTemp = HevcDecRu32(RegBaseAddr + HEVC_FUNCTION_STATUS_REG,
            "regHEVC_FunctionStatus_reg38");
        if(reg38.vld_busy)
        {
            i++;
            if(i > 0x200000)
            {
                logd("  HevcGetbitsHardware() over time return 0 ");
                return 0;
            }
        }
        else
        {
            u32 nTemp = HevcDecRu32(RegBaseAddr + HEVC_BITS_RETDATA_REG,
                "HEVC_BITS_RETDATA_REG");
            value = (s32)nTemp;
            break;
        }
    }
    return value;
}

/*
 * HevcSetStreamInfoReg() function only used in software decode slice header case;
 * */
static void HevcSetStreamInfoReg(HevcContex *pHevcDec)
{
    GetBitContext *pGb = &pHevcDec->getBit;
    HevcStreamInfo *pStreamInfo = pHevcDec->pStreamInfo;
    u32 bFlag, nShEmulationCodeNum;
    s32 nSliceDataBitLen, nBitOffset, i, nIndex, nShSize;
    s32 nShBitLen;
    size_addr RegBaseAddr = pHevcDec->HevcDecRegBaseAddr;
    volatile u32* pdwTemp = 0;
    //static u32 nMaxByteOffset = 0;

#if 0
    AdapterMemFlushCache(pStreamInfo->pDataBuf[pStreamInfo->nBufIndex],
            pStreamInfo->nDataSize[pStreamInfo->nBufIndex]);
    logd(" ---- after fush cash ok ------ ");
#endif

//    logd("RegBaseAddr: %X", RegBaseAddr);
    pdwTemp = (u32*)(&regHEVC_BitsEndAddr_reg4c);
    *pdwTemp = 0;
    regHEVC_BitsEndAddr_reg4c.bitstream_end_addr = pStreamInfo->pCurrentBufEndPhy >> 8;
    HevcDecWu32(RegBaseAddr + HEVC_BITS_END_ADDR_REG, *pdwTemp,
        "regHEVC_BitsEndAddr_reg4c");

    /* setting offset and bit length start */
    /* step 1: caculate how many emulation codes in the slice header data */
    nShEmulationCodeNum = 0;
    nShSize = (pGb->index >> 3);
    for(i = 0; i < pStreamInfo->nEmulationCodeNum; i++)
    {
        if(nShSize >= pStreamInfo->EmulationCodeIdx[i])
        {
            nShSize += 1;
            nShEmulationCodeNum += 1;
        }
    }
    /* step 2: caculate bit offset and slice data bit length */
    nIndex = pStreamInfo->nBufIndex;
    nShBitLen = pGb->index + (nShEmulationCodeNum * 8);
    /* we have decode slice header bits */
//    logd(" nShBitLen: %d ", nShBitLen);
    nSliceDataBitLen = pStreamInfo->nDataSize[nIndex] * 8 - nShBitLen;
    /* the rest data bits */
    bFlag = pStreamInfo->bHasTwoDataTrunk[nIndex];
    if(bFlag) /* current stream has two data trunk case */
    {
        logd(" ---- has two data trunk  ------  ");
        if(nShBitLen <= pStreamInfo->FirstDataTrunkSize[nIndex] * 8)
            nBitOffset = pStreamInfo->nBitsOffset + nShBitLen;
        else
            nBitOffset = nShBitLen - pStreamInfo->FirstDataTrunkSize[nIndex] * 8;
    }
    else
        nBitOffset = pStreamInfo->nBitsOffset + nShBitLen;

#if 0
    /* test code */
    nTemp = pGb->index >> 3;
    if(nMaxByteOffset < nTemp)
        nMaxByteOffset = nTemp;
    logd(" after decode slice header, nMaxByteOffset: %d,  byte offset: %d, ",
        nMaxByteOffset, nTemp);
    logd(" slice header nShEmulationCodeNum: %d, pGb->index: %d, nSliceDataBitLen: %d",
            nShEmulationCodeNum, pGb->index, nSliceDataBitLen);
    {
        char *p = pStreamInfo->ShByte;
        s32 n = (pGb->index >> 3);
        logd("slice data: index: %d , \
                data: %2.2x, %2.2x, %2.2x, %2.2x, %2.2x, %2.2x, %2.2x, %2.2x",
                n, p[n],p[n+1],p[n+2],p[n+3],p[n+4],p[n+5],p[n+6],p[n+7]);
        logd("");
        p = pStreamInfo->pDataBuf[nIndex];
        p += nShSize;
        n = 0;
        logd("slice data: index: %d , \
                data: %2.2x, %2.2x, %2.2x, %2.2x, %2.2x, %2.2x, %2.2x, %2.2x",
                nShSize,p[n],p[n+1],p[n+2],p[n+3],p[n+4],p[n+5],p[n+6],p[n+7]);
        logd(" nBitsOffset: %d sbm base: %x, buf addr: %x, dif: %d",
                pStreamInfo->nBitsOffset,
                pStreamInfo->pSbmBuf, pStreamInfo->pDataBuf[nIndex],
                pStreamInfo->pDataBuf[nIndex] - pStreamInfo->pSbmBuf);

        n = nBitOffset >> 3;
        p = pStreamInfo->pSbmBuf;
        logd("slice sbm data: nBitOffset: %d,%x , \
                data: %2.2x, %2.2x, %2.2x, %2.2x, %2.2x, %2.2x, %2.2x, %2.2x",
                nBitOffset,nBitOffset,p[n],p[n+1],p[n+2],p[n+3],
                p[n+4],p[n+5],p[n+6],p[n+7]);
        logd("");
    }

#endif

    pdwTemp = (u32*)(&regHEVC_BitsLength_reg48);
    *pdwTemp = 0;
#if 1
    nSliceDataBitLen = nSliceDataBitLen >> 3;
    nSliceDataBitLen = (nSliceDataBitLen + 4 + 3) & ~3;
    nSliceDataBitLen = nSliceDataBitLen << 3;
//    logd(" new nSliceDataBitLen: %d ", nSliceDataBitLen);
#endif
//    nSliceDataBitLen = (nSliceDataBitLen + 4 + 3) & (0x3); /* todo: unkonw why ??? */
    regHEVC_BitsLength_reg48.bits_len = nSliceDataBitLen;
    HevcDecWu32(RegBaseAddr + HEVC_BITS_LEN_REG, *pdwTemp, "regHEVC_BitsLength_reg48");

    pdwTemp = (u32*)(&regHEVC_BitsOffset_reg44);
    *pdwTemp = 0;
    regHEVC_BitsOffset_reg44.bits_offset = nBitOffset;
    HevcDecWu32(RegBaseAddr + HEVC_BITS_OFFSET_REG, *pdwTemp, "regHEVC_BitsOffset_reg44");

    /* setting offset and bit length end */

    pdwTemp = (u32*)(&regHEVC_BitsBaseAddr_reg40);
    *pdwTemp = 0;
    regHEVC_BitsBaseAddr_reg40.data_first = 1;
    regHEVC_BitsBaseAddr_reg40.data_last  = 1;
    regHEVC_BitsBaseAddr_reg40.data_valid = 1;
    regHEVC_BitsBaseAddr_reg40.bitstream_base_addr = pStreamInfo->pCurrentBufPhy >> 8;
    HevcDecWu32(RegBaseAddr + HEVC_BITS_BASE_ADDR_REG, *pdwTemp, "regHEVC_BitsBaseAddr_reg40");

    pdwTemp = (u32*)(&regHEVC_TriggerType_reg34);
    *pdwTemp = 0;
    regHEVC_TriggerType_reg34.trigger_type_pul = 0x07; // init_swdec
    HevcDecWu32(RegBaseAddr + HEVC_TRIGGER_TYPE_REG, *pdwTemp, "regHEVC_TriggerType_reg34");
}

static void HevcSetNalHeaderReg(HevcContex *pHevcDec)
{
    size_addr RegBaseAddr = pHevcDec->HevcDecRegBaseAddr;
    volatile u32* pdwTemp;
    pdwTemp = (u32*)(&regHEVC_NalHdr_reg00);
    *pdwTemp = 0;
    regHEVC_NalHdr_reg00.nal_unit_type = pHevcDec->eNaluType;
    regHEVC_NalHdr_reg00.nuh_temporal_id_plus1 = pHevcDec->nTemporalId + 1;
    HevcDecWu32(RegBaseAddr + HEVC_NAL_HDR_REG, *pdwTemp, "regHEVC_NalHdr_reg00");
}

static void HevcSetSpsReg(HevcContex *pHevcDec)
{
    size_addr RegBaseAddr = pHevcDec->HevcDecRegBaseAddr;
    HevcSPS *pSps = pHevcDec->pSps;
    volatile u32* pdwTemp;

    if(pSps == NULL)
    {
        loge(" h265 Hardware error, sps == NULL"); /* shouldn't happen */
        return;
    }
    pdwTemp = (u32*)(&regHEVC_SPS_reg04);
    *pdwTemp = 0;
    regHEVC_SPS_reg04.chroma_format_idc                 = pSps->nChromaFormatIdc;
    regHEVC_SPS_reg04.separate_color_plane_flag         = pSps->bSeparateColourPlaneFlag;
    regHEVC_SPS_reg04.bit_depth_luma_minus_8            = pSps->nBitDepthLuma - 8;
    regHEVC_SPS_reg04.bit_depth_chroma_minus_8          = pSps->nBitDepthChroma - 8;
    regHEVC_SPS_reg04.log2_min_luma_CB_size_minus3      = pSps->nLog2MinCbSize - 3;
    regHEVC_SPS_reg04.log2_diff_max_min_luma_CB_size    = pSps->nLog2DiffMaxMinCbSize;
    regHEVC_SPS_reg04.log2_min_TB_size_minus2           = pSps->nLog2MinTbSize - 2;
    regHEVC_SPS_reg04.log2_diff_max_min_TB_size         = pSps->nLog2DiffMaxMinTbSize;
    regHEVC_SPS_reg04.max_transform_depth_inter         = pSps->nMaxTransformHierarchydepthInter;
    regHEVC_SPS_reg04.max_transform_depth_intra         = pSps->nMaxTransformHierarchydepthIntra;
    regHEVC_SPS_reg04.amp_enabled_flag                  = pSps->bAmpEnableFlag;
    regHEVC_SPS_reg04.sample_adaptive_offset_en         = pSps->bSaoEnabled;
    regHEVC_SPS_reg04.sps_temporal_mvp_en_flag          = pSps->bSpsTemporalMvpEnableFlag;
    regHEVC_SPS_reg04.strong_intra_smoothing_en         = pSps->bSpsStrongIntraSmoothingenableFlag;
    regHEVC_SPS_reg04.r0                                = 0;
    HevcDecWu32(RegBaseAddr + HEVC_SPS_REG, *pdwTemp, "regHEVC_SPS_reg04");

    pdwTemp = (u32*)(&regHEVC_PicSize_reg08);
    *pdwTemp = 0;
    regHEVC_PicSize_reg08.pic_height_in_luma_samples = pSps->nHeight;
    regHEVC_PicSize_reg08.pic_width_in_luma_samples  = pSps->nWidth;
    HevcDecWu32(RegBaseAddr + HEVC_PIC_SIZE_REG, *pdwTemp, "regHEVC_PicSize_reg08");

    pdwTemp = (u32*)(&regHEVC_PCM_Ctrl_reg0c);
    *pdwTemp = 0;
    regHEVC_PCM_Ctrl_reg0c.pcm_bit_depth_luma_minus1          = pSps->pcm.nPcmBitDepthLuma - 1;
    regHEVC_PCM_Ctrl_reg0c.pcm_bit_depth_chroma_minus1        = pSps->pcm.nPcmBitdepthChroma - 1;
    regHEVC_PCM_Ctrl_reg0c.log2_min_pcm_luma_CB_size_minus3   = pSps->pcm.nLog2MinPcmCbSize - 3;
    regHEVC_PCM_Ctrl_reg0c.log2_diff_max_min_pcm_luma_CB_size = pSps->pcm.nLog2DiffMaxMinPcmCbSize;
    regHEVC_PCM_Ctrl_reg0c.pcm_loop_filter_disable_flag       = pSps->pcm.bLoopFilterDisableFlag;
    regHEVC_PCM_Ctrl_reg0c.pcm_enable_flag                    = pSps->bPcmEnableFlag;
    HevcDecWu32(RegBaseAddr + HEVC_PCM_CTRL_REG, *pdwTemp, "regHEVC_PCM_Ctrl_reg0c");
}

static void HevcSetPpsReg(HevcContex *pHevcDec)
{
    size_addr RegBaseAddr = pHevcDec->HevcDecRegBaseAddr;
    HevcPPS *pPps = pHevcDec->pPps;
    volatile u32* pdwTemp;

    if(pPps == NULL)
    {
        loge(" h265 Hardware error, pps == NULL"); /* shouldn't happen */
        return;
    }

    if(pHevcDec->nDecIpVersion == 0x31010)
    {
        pdwTemp = (u32*)(&regHEVC_PPS_Ctrl0_reg10_v02);
        *pdwTemp = 0;
        regHEVC_PPS_Ctrl0_reg10_v02.sign_data_hiding_flag         = pPps->bSignDataHidingflag;
        regHEVC_PPS_Ctrl0_reg10_v02.constrained_intra_pred_flag = pPps->bConstrainedIntraPredFlag;
        regHEVC_PPS_Ctrl0_reg10_v02.transform_skip_en             = pPps->bTransformSkipEnabledFlag;
        regHEVC_PPS_Ctrl0_reg10_v02.cu_qp_delta_en_flag         = pPps->bCuQpDeltaEnabledFlag;
        regHEVC_PPS_Ctrl0_reg10_v02.diff_cu_qp_delta_depth         = pPps->nDiffCuQpDeltaDepth;
        regHEVC_PPS_Ctrl0_reg10_v02.init_qp_minus26             = pPps->nInitQpMinus26;
        regHEVC_PPS_Ctrl0_reg10_v02.pps_cb_qp_offset             = pPps->nPpsCbQpOffset;
        regHEVC_PPS_Ctrl0_reg10_v02.pps_cr_qp_offset             = pPps->nPpsCrQpOffset;
    }
    else
    {
        pdwTemp = (u32*)(&regHEVC_PPS_Ctrl0_reg10_v01);
        *pdwTemp = 0;
        regHEVC_PPS_Ctrl0_reg10_v01.sign_data_hiding_flag         = pPps->bSignDataHidingflag;
        regHEVC_PPS_Ctrl0_reg10_v01.constrained_intra_pred_flag = pPps->bConstrainedIntraPredFlag;
        regHEVC_PPS_Ctrl0_reg10_v01.transform_skip_en             = pPps->bTransformSkipEnabledFlag;
        regHEVC_PPS_Ctrl0_reg10_v01.cu_qp_delta_en_flag         = pPps->bCuQpDeltaEnabledFlag;
        regHEVC_PPS_Ctrl0_reg10_v01.diff_cu_qp_delta_depth         = pPps->nDiffCuQpDeltaDepth;
        regHEVC_PPS_Ctrl0_reg10_v01.init_qp_minus26             = pPps->nInitQpMinus26;
        regHEVC_PPS_Ctrl0_reg10_v01.pps_cb_qp_offset             = pPps->nPpsCbQpOffset;
        regHEVC_PPS_Ctrl0_reg10_v01.pps_cr_qp_offset             = pPps->nPpsCrQpOffset;

    }
    HevcDecWu32(RegBaseAddr + HEVC_PPS_CTRL0_REG, *pdwTemp, "regHEVC_PPS_Ctrl0_reg10");

    pdwTemp = (u32*)(&regHEVC_PPS_Ctrl1_reg14);
    *pdwTemp = 0;
    regHEVC_PPS_Ctrl1_reg14.weighted_pred_flag             = pPps->bWeightedPredFlag;
    regHEVC_PPS_Ctrl1_reg14.weighted_birped_flag         = pPps->bWeightedBipredFlag;
    regHEVC_PPS_Ctrl1_reg14.transquant_bypass_en         = pPps->bTransquantBypassEnableFlag;
    regHEVC_PPS_Ctrl1_reg14.tiles_en_flag                 = pPps->bTilesEnabledFlag;
    regHEVC_PPS_Ctrl1_reg14.entropy_coding_sync_en         = pPps->bEntropyCodingSyncEnabledFlag;
    regHEVC_PPS_Ctrl1_reg14.loop_filter_accross_tiles_en = pPps->bLoopFilterAcrossTilesEnableFlag;
    regHEVC_PPS_Ctrl1_reg14.loop_filter_accross_slice_en =
        pPps->bPpsLoopFilterAcrossSlicesEnableFlag;
    regHEVC_PPS_Ctrl1_reg14.log2_parallel_merge_level_minus2 =
        pPps->nLog2ParallelMergeLevelMinus2;
    HevcDecWu32(RegBaseAddr + HEVC_PPS_CTRL1_REG, *pdwTemp, "regHEVC_PPS_Ctrl1_reg14");
}

static void HevcSetScalingListReg(HevcContex *pHevcDec)
{
    size_addr RegBaseAddr = pHevcDec->HevcDecRegBaseAddr;
    HevcSPS *pSps = pHevcDec->pSps;
    HevcPPS *pPps = pHevcDec->pPps;
    //HevcSliceHeader *pSh = &pHevcDec->SliceHeader;
    HevcScalingList *pShScalingList = NULL;
    s32 nTemp, dwTemp, use_flat_scaling_list, use_default_scaling_matrix;
    volatile u32* pdwTemp;

    use_flat_scaling_list = 0;
    use_default_scaling_matrix = 1;
    if(pSps->bScalingListEnableFlag)
    {
        pShScalingList = &pSps->ScalingList;
        use_default_scaling_matrix = 1 - pSps->ScalingList.bScalingListMatrixModified;

        if(pPps->bPpsScalingListDataPresentFlag)
        {
            pShScalingList = &pPps->ScalingList;
            if(use_default_scaling_matrix == 1)
                use_default_scaling_matrix = 1 - pPps->ScalingList.bScalingListMatrixModified;
        }
        if(!pPps->bPpsScalingListDataPresentFlag && !pSps->bSpsScalingListDataPresentFlag)
        {
            logv(" set   use_default_scaling_matrix = 1 here 01");
            use_default_scaling_matrix = 1;
        }
    }
    else
        use_flat_scaling_list = 1;

    if(pSps->bScalingListEnableFlag)
    {
        int i, n,m;
        u32 tmp_reg0 = 0,tmp_reg1 = 0;
        u8 *pScalingListCoef;

        for(i=0;i<2;i++)
        {
            int dc;
            dc = pShScalingList->SlDc[1][i];// m_scalingListDC[3][i];
            tmp_reg0 |= (dc & 0xff)<<((i+2)*8);
        }

        for(i=0;i<6;i++)
        {
            int dc;
            dc = pShScalingList->SlDc[0][i];// ->m_scalingListDC[2][i];
            if(i >= 2)
                tmp_reg1 |= (dc & 0xff)<<((i-2)*8);
            else
                tmp_reg0 |= (dc & 0xff)<<i*8;
        }
        HevcDecWu32(RegBaseAddr + HEVC_SCALER_LIST_DC_VAL0_REG,
            tmp_reg0, "HEVC_SCALER_LIST_DC_VAL0_REG");
        HevcDecWu32(RegBaseAddr + HEVC_SCALER_LIST_DC_VAL1_REG,
            tmp_reg1, "HEVC_SCALER_LIST_DC_VAL1_REG");
        nTemp = HEVC_SCALING_MATRIX_ADDR;
        HevcSramWriteAddr(RegBaseAddr, nTemp, "HEVC_SCALING_MATRIX_ADDR");
        //8x8
        dwTemp = 0;
        for(i=0;i<6;i++)
        {
            pScalingListCoef = pShScalingList->Sl[1][i];// ->m_scalingListCoef[1][i];
            for(n = 0; n < 8; n++)
            {
                for( m = 0; m < 8; m += 4)
                {
                    dwTemp = (pScalingListCoef[n+(m+3)*8]<<24) |
                        (pScalingListCoef[n+(m+2)*8]<<16) |
                        (pScalingListCoef[n+(m+1)*8]<<8) |
                        (pScalingListCoef[n+m*8]<<0);
                    HevcSramWriteData(RegBaseAddr, dwTemp,
                        "HEVC_SRAM_PORT_DATA_REG  0xE4");
                }
            }
        }

        //32x32
        for(i=0;i<2;i++)
        {
            //int dc;
            pScalingListCoef = pShScalingList->Sl[3][i];// ->m_scalingListCoef[3][i];
            for( n = 0; n < 8; n++)
            {
                for( m = 0; m < 8; m += 4)
                {
                    dwTemp = (pScalingListCoef[n+(m+3)*8]<<24) |
                        (pScalingListCoef[n+(m+2)*8]<<16) |
                        (pScalingListCoef[n+(m+1)*8]<<8) |
                        (pScalingListCoef[n+m*8]<<0);
                    HevcSramWriteData(RegBaseAddr, dwTemp, "HEVC_SRAM_PORT_DATA_REG  0xE4");
                }
            }
        }

        //16x16
        for(i=0;i<6;i++)
        {
            //int dc;
            pScalingListCoef = pShScalingList->Sl[2][i];// ->m_scalingListCoef[2][i];
            for(n = 0; n < 8; n++)
            {
                for(m = 0; m < 8; m+=4)
                {
                    dwTemp = (pScalingListCoef[n+(m+3)*8]<<24) |
                        (pScalingListCoef[n+(m+2)*8]<<16) |
                        (pScalingListCoef[n+(m+1)*8]<<8) |
                        (pScalingListCoef[n+m*8]<<0);
                    HevcSramWriteData(RegBaseAddr, dwTemp,
                        "HEVC_SRAM_PORT_DATA_REG  0xE4");
                }
            }
        }
        //4x4
        for(i=0;i<6;i++)
        {
            pScalingListCoef = pShScalingList->Sl[0][i];// ->m_scalingListCoef[0][i];
            for(n = 0; n < 4; n++)
            {
                for(m = 0; m < 4; m+=4)
                {
                    dwTemp = (pScalingListCoef[n+(m+3)*4]<<24) |
                        (pScalingListCoef[n+(m+2)*4]<<16) |
                        (pScalingListCoef[n+(m+1)*4]<<8) |
                        (pScalingListCoef[n+m*4]<<0);
                    HevcSramWriteData(RegBaseAddr, dwTemp,
                        "HEVC_SRAM_PORT_DATA_REG  0xE4");
                }
            }
        }
    }

    //4.2 set listId.
    pdwTemp = (u32*)(&regHEVC_ScalingListCtrl0_reg18);
    *pdwTemp = 0;
    regHEVC_ScalingListCtrl0_reg18.scaling_list_enabled_flag = 1 - use_flat_scaling_list;
    /* todo: scaling list */
    regHEVC_ScalingListCtrl0_reg18.use_default_scaling_matrix = use_default_scaling_matrix;
#if 0
    logd("use_flat_scaling_list: %d, use_default_scaling_matrix: %d",
        use_flat_scaling_list, use_default_scaling_matrix);
    if(use_flat_scaling_list==0 && use_default_scaling_matrix==0)
    {
        logd(" enter here.... write reg 18 ");
        regHEVC_ScalingListCtrl0_reg18.intra4x4_Y =
            pShScalingList->nRefMatrixId[0][0] & 0x7;
        regHEVC_ScalingListCtrl0_reg18.intra4x4_Cb =
            pShScalingList->nRefMatrixId[0][1] & 0x7;
        regHEVC_ScalingListCtrl0_reg18.intra4x4_Cr =
            pShScalingList->nRefMatrixId[0][2] & 0x7;
        regHEVC_ScalingListCtrl0_reg18.inter4x4_Y =
            pShScalingList->nRefMatrixId[0][3] & 0x7;
        regHEVC_ScalingListCtrl0_reg18.inter4x4_Cb =
            pShScalingList->nRefMatrixId[0][4] & 0x7;
        regHEVC_ScalingListCtrl0_reg18.inter4x4_Cr =
            pShScalingList->nRefMatrixId[0][5] & 0x7;
        regHEVC_ScalingListCtrl0_reg18.intra8x8_Y =
            pShScalingList->nRefMatrixId[1][0] & 0x7;
        regHEVC_ScalingListCtrl0_reg18.intra8x8_Cb =
            pShScalingList->nRefMatrixId[1][1] & 0x7;
        regHEVC_ScalingListCtrl0_reg18.intra8x8_Cr =
            pShScalingList->nRefMatrixId[1][2] & 0x7;
    }
    logd("  write reg18: %8.8x ", *pdwTemp);
#endif
    HevcDecWu32(RegBaseAddr + HEVC_SCALING_LIST_CTRL0_REG,
            *pdwTemp, "regHEVC_ScalingListCtrl0_reg18");
    dwTemp = HevcDecRu32(RegBaseAddr + HEVC_SCALING_LIST_CTRL0_REG,
            "regHEVC_ScalingListCtrl0_reg18");
    CEDARC_UNUSE(dwTemp);

#if 0
    /* reg1c is no used anymore */
    pdwTemp = (u32*)(&regHEVC_ScalingListCtrl1_reg1c);
    *pdwTemp = 0;
    if(use_flat_scaling_list==0 && use_default_scaling_matrix==0)
    {
        logd(" enter here.... write reg 1c  ");
        regHEVC_ScalingListCtrl1_reg1c.inter8x8_Y    =
            pShScalingList->nRefMatrixId[1][3] & 0x7;
        regHEVC_ScalingListCtrl1_reg1c.inter8x8_Cb   =
            pShScalingList->nRefMatrixId[1][4] & 0x7;
        regHEVC_ScalingListCtrl1_reg1c.inter8x8_Cr   =
            pShScalingList->nRefMatrixId[1][5] & 0x7;

        regHEVC_ScalingListCtrl1_reg1c.intra16x16_Y  =
            pShScalingList->nRefMatrixId[2][0] & 0x7;
        regHEVC_ScalingListCtrl1_reg1c.intra16x16_Cb =
            pShScalingList->nRefMatrixId[2][1] & 0x7;
        regHEVC_ScalingListCtrl1_reg1c.intra16x16_Cr =
            pShScalingList->nRefMatrixId[2][2] & 0x7;
        regHEVC_ScalingListCtrl1_reg1c.inter16x16_Y  =
            pShScalingList->nRefMatrixId[2][3] & 0x7;
        regHEVC_ScalingListCtrl1_reg1c.inter16x16_Cb =
            pShScalingList->nRefMatrixId[2][4] & 0x7;
        regHEVC_ScalingListCtrl1_reg1c.inter16x16_Cr =
            pShScalingList->nRefMatrixId[2][5] & 0x7;

        regHEVC_ScalingListCtrl1_reg1c.intra32x32_Y =
            pShScalingList->nRefMatrixId[3][0] & 0x7;
        regHEVC_ScalingListCtrl1_reg1c.inter32x32_Y =
            pShScalingList->nRefMatrixId[3][1] & 0x7;
    }
    logd("  write reg1c: %8.8x ", *pdwTemp);
    HevcDecWu32(RegBaseAddr + HEVC_SCALING_LIST_CTRL1_REG,
        *pdwTemp, "regHEVC_ScalingListCtrl1_reg1c");
    dwTemp = HevcDecRu32(RegBaseAddr + HEVC_SCALING_LIST_CTRL1_REG,
        "regHEVC_ScalingListCtrl1_reg1c");
    logd(" read 18 reg: %8.8x ", dwTemp);
#endif
}

static void HevcSetSliceHeaderReg(HevcContex *pHevcDec)
{
    size_addr RegBaseAddr = pHevcDec->HevcDecRegBaseAddr;
    HevcSliceHeader *pSh = &pHevcDec->SliceHeader;
    HevcPPS *pPps = pHevcDec->pPps;
    //HevcSPS *pSps = pHevcDec->pSps;
    volatile u32* pdwTemp;

    pdwTemp = (u32*)(&regHEVC_SliceHdrInfo0_reg20);
    *pdwTemp = 0;
    regHEVC_SliceHdrInfo0_reg20.first_slice_segment_flag       = pSh->bFirstSliceInPicFlag;
    regHEVC_SliceHdrInfo0_reg20.dependent_slice_segment_flag   = pSh->bDependentSliceSegmentFlag;
    regHEVC_SliceHdrInfo0_reg20.slice_type                     = pSh->eSliceType;
    regHEVC_SliceHdrInfo0_reg20.color_plane_id                 = 0;
    // valid when separate color enable
    regHEVC_SliceHdrInfo0_reg20.slice_temporal_mvp_enable_flag = pSh->bSliceTemporalMvpEnableFlag;
    regHEVC_SliceHdrInfo0_reg20.slice_sao_luma_flag            =
        pSh->bSliceSampleAdaptiveOffsetFlag[0];
    regHEVC_SliceHdrInfo0_reg20.slice_sao_chroma_flag          =
        pSh->bSliceSampleAdaptiveOffsetFlag[1];
    regHEVC_SliceHdrInfo0_reg20.mvd_l1_zero_flag               =
        pSh->bMvdL1ZeroFlag;
    regHEVC_SliceHdrInfo0_reg20.cabac_init_flag                =
        pSh->bCabacInitFlag;
    regHEVC_SliceHdrInfo0_reg20.collocated_from_l0_flag        =
        pSh->bCollocatedFromL0Flag;
    regHEVC_SliceHdrInfo0_reg20.collocated_ref_idx             =
        pSh->nCollocatedRefIdx;
    if(pSh->eSliceType == HEVC_I_SLICE)
    {
      regHEVC_SliceHdrInfo0_reg20.num_ref_idx0_active_minus1 = 0;
      regHEVC_SliceHdrInfo0_reg20.num_ref_idx1_active_minus1 = 0;
    }
    else
    {
      regHEVC_SliceHdrInfo0_reg20.num_ref_idx0_active_minus1 = pSh->nNumOfRefs[0] - 1;
      regHEVC_SliceHdrInfo0_reg20.num_ref_idx1_active_minus1 = pSh->nNumOfRefs[1] - 1;
    }
    regHEVC_SliceHdrInfo0_reg20.five_minus_max_num_merg_cand =
        HEVC_MRG_MAX_NUM_CANDS - pSh->nMaxNumMergeCand;
    regHEVC_SliceHdrInfo0_reg20.picture_type                 = 0;
    // 0: frame  1:top field  2:bottom field, TODO: fixme
    HevcDecWu32(RegBaseAddr + HEVC_SLICE_HDR_INFO0_REG,
            *pdwTemp, "regHEVC_SliceHdrInfo0_reg20");

    if(pSh->eSliceType != HEVC_I_SLICE)
    {
        u32 bLowDelay = 1;
        s32 i;
        HevcRefPicList *pRefPicList;
        pRefPicList = &pHevcDec->RefPicList[0];
        for(i = 0; i < pRefPicList->nNumOfRefs && bLowDelay; i++)
        {
            if(pRefPicList->List[i] > pHevcDec->nPoc)
                bLowDelay = 0;
        }
        if(pSh->eSliceType == HEVC_B_SLICE)
        {
            pRefPicList = &pHevcDec->RefPicList[1];
            for(i = 0; i < pRefPicList->nNumOfRefs && bLowDelay; i++)
            {
                if(pRefPicList->List[i] > pHevcDec->nPoc)
                    bLowDelay = 0;
            }
        }
        pSh->bIsNotBlowDelayFlag = bLowDelay;
    }

    if(pHevcDec->nDecIpVersion == 0x31010)
    {
        pdwTemp = (u32*)(&regHEVC_SliceHdrInfo1_reg24_v02);
        *pdwTemp = 0;
        regHEVC_SliceHdrInfo1_reg24_v02.slice_qp_delta                      =
            (pSh->nSliceQp - (26 + pPps->nInitQpMinus26)) & 0x7f;
        regHEVC_SliceHdrInfo1_reg24_v02.slice_cb_qp_offset                  =
            pSh->nSliceCbQpOffset;
        regHEVC_SliceHdrInfo1_reg24_v02.slice_cr_qp_offset                  =
            pSh->nSliceCrQpOffset;
        regHEVC_SliceHdrInfo1_reg24_v02.is_not_blowdelay_flag               =
            pSh->bIsNotBlowDelayFlag;
        regHEVC_SliceHdrInfo1_reg24_v02.slice_loop_filter_accross_slcies_en =
            pSh->bSliceLoopFilterAcrossSliceEnableFlag;
        regHEVC_SliceHdrInfo1_reg24_v02.slice_disable_deblocking_flag       =
            pSh->bSlicDeblockingFilterDisableFlag;
        regHEVC_SliceHdrInfo1_reg24_v02.slice_beta_offset_div2              =
            (pSh->nBetaOffset/2)/* & 0xf*/;
        regHEVC_SliceHdrInfo1_reg24_v02.slice_tc_offset_div2                =
            (pSh->nTcOffset/2)/* & 0xf*/;
    }
    else
    {
        pdwTemp = (u32*)(&regHEVC_SliceHdrInfo1_reg24_v01);
        *pdwTemp = 0;
        regHEVC_SliceHdrInfo1_reg24_v01.slice_qp_delta                      =
                (pSh->nSliceQp - (26 + pPps->nInitQpMinus26)) & 0x3f;
        regHEVC_SliceHdrInfo1_reg24_v01.slice_cb_qp_offset                  =
            pSh->nSliceCbQpOffset;
        regHEVC_SliceHdrInfo1_reg24_v01.slice_cr_qp_offset                  =
            pSh->nSliceCrQpOffset;
        regHEVC_SliceHdrInfo1_reg24_v01.is_not_blowdelay_flag               =
            pSh->bIsNotBlowDelayFlag;
        regHEVC_SliceHdrInfo1_reg24_v01.slice_loop_filter_accross_slcies_en =
            pSh->bSliceLoopFilterAcrossSliceEnableFlag;
        regHEVC_SliceHdrInfo1_reg24_v01.slice_disable_deblocking_flag       =
            pSh->bSlicDeblockingFilterDisableFlag;
        regHEVC_SliceHdrInfo1_reg24_v01.slice_beta_offset_div2              =
            (pSh->nBetaOffset/2)/* & 0xf*/;
        regHEVC_SliceHdrInfo1_reg24_v01.slice_tc_offset_div2                =
            (pSh->nTcOffset/2)/* & 0xf*/;
    }
    HevcDecWu32(RegBaseAddr + HEVC_SLICE_HDR_INFO1_REG, *pdwTemp,
        "regHEVC_SliceHdrInfo1_reg24");

    pdwTemp = (u32*)(&regHEVC_SliceHdrInfo2_reg28);
    *pdwTemp = 0;
    regHEVC_SliceHdrInfo2_reg28.luma_log2_weight_denom   = pSh->nLumaLog2WeightDenom;
    regHEVC_SliceHdrInfo2_reg28.chroma_log2_weight_denom = pSh->nChromaLog2WeightDenom;
    regHEVC_SliceHdrInfo2_reg28.num_entry_point_offsets  = pSh->nNumEntryPointOffsets;
    HevcDecWu32(RegBaseAddr + HEVC_SLICE_HDR_INFO2_REG,
        *pdwTemp, "regHEVC_SliceHdrInfo2_reg28");
}

void HevcSetOutputConfigReg(HevcContex *pHevcDec)
{
    size_addr RegBaseAddr = pHevcDec->HevcDecRegBaseAddr;
    size_addr RegTopAddr  = pHevcDec->HevcTopRegBaseAddr;
    HevcSliceHeader *pSh = &pHevcDec->SliceHeader;
    HevcControlInfo *pCi = &pHevcDec->ControlInfo;
    //VConfig *pCf = pHevcDec->pConfig;
    //HevcPPS *pPps = pHevcDec->pPps;
    HevcSPS *pSps = pHevcDec->pSps;
    u32 nTemp;
    volatile u32* pdwTemp;

    if(!pSh->bFirstSliceInPicFlag)
    {
        if(pHevcDec->nDecIpVersion == 0x31010)
        {
            pdwTemp = (u32*)(&regHEVC_CTBAddr_reg2C_v02);
            *pdwTemp = 0;
            regHEVC_CTBAddr_reg2C_v02.CTB_x = pSh->nSliceSegmentAddr % pSps->nCtbWidth;
            regHEVC_CTBAddr_reg2C_v02.CTB_y = pSh->nSliceSegmentAddr / pSps->nCtbWidth;
        }
        else
        {
            pdwTemp = (u32*)(&regHEVC_CTBAddr_reg2C_v01);
            *pdwTemp = 0;
            regHEVC_CTBAddr_reg2C_v01.CTB_x = pSh->nSliceSegmentAddr % pSps->nCtbWidth;
            regHEVC_CTBAddr_reg2C_v01.CTB_y = pSh->nSliceSegmentAddr / pSps->nCtbWidth;

        }
        logv(" not first slice, addr: (%d, %d),  nSliceSegmentAddr: %d ",
                pSh->nSliceSegmentAddr % pSps->nCtbWidth,
                pSh->nSliceSegmentAddr / pSps->nCtbWidth,
                pSh->nSliceSegmentAddr);
        HevcDecWu32(RegBaseAddr + HEVC_CTB_ADDR_REG, *pdwTemp, "regHEVC_CTBAddr_reg2C");
    }
    else /* first slice in the picture, clear CTU number */
    {
        pdwTemp = (u32*)(&regHEVC_DecCUTNum_reg3c);
        *pdwTemp = 0;
        HevcDecWu32(RegBaseAddr + HEVC_DECCTU_NUM_REG,
            *pdwTemp, "regHEVC_DecCUTNum_reg3c");
    }

    pdwTemp = (u32*)(&regHEVC_FunctionCtrol_reg30);
    *pdwTemp = 0;
    regHEVC_FunctionCtrol_reg30.tq_bypass_en              = 0; //todo: shouldn't const
    regHEVC_FunctionCtrol_reg30.vld_bypass_en             = 0; //todo: shouldn't const
    regHEVC_FunctionCtrol_reg30.finish_interrupt_en       = 1;
    regHEVC_FunctionCtrol_reg30.data_request_interrupt_en = 1;
    regHEVC_FunctionCtrol_reg30.error_interrupt_en        = 1;
    regHEVC_FunctionCtrol_reg30.write_sc_rt_pic           =
        pCi->secOutputEnabled == 1 ? 1 : 0;

    if(pHevcDec->nDecIpVersion == 0x31010)
    {
        regHEVC_FunctionCtrol_reg30.ddr_consistency_en    = 1;
    }

    //todo: shouldn't const, secode picture output
    HevcDecWu32(RegBaseAddr + HEVC_FUNC_CTRL_REG,
            *pdwTemp, "regHEVC_FunctionCtrol_reg30");

    u32 tmpread = HevcDecRu32(RegBaseAddr + HEVC_FUNC_CTRL_REG,"regHEVC_FunctionCtrol_reg30");
    logv("*********** regHEVC_FunctionCtrol_reg30 = %x",tmpread);

    nTemp = pCi->secPixelFormatReg << 30 | pCi->secFrmBufChromaSize;
    HevcDecWu32(RegTopAddr + VECORE_OUT_CHROMA_LEN_REG, nTemp, "VECORE_OUT_CHROMA_LEN_REG");

    if((pCi->secOutputEnabled == 0 && pSps->nWidth >= 2048) ||
            (pCi->secOutputEnabled == 1 && pCi->secFrmBufLumaStride >= 2048))
    {
        nTemp = HevcDecRu32(RegTopAddr + VECORE_MODESEL_REG, "VECORE_MODESEL_REG");
        nTemp |= 0x00200000;
        HevcDecWu32(RegTopAddr + VECORE_MODESEL_REG, nTemp, "VECORE_MODESEL_REG");
    }
    else
    {
        nTemp = HevcDecRu32(RegTopAddr + VECORE_MODESEL_REG, "VECORE_MODESEL_REG");
        nTemp &= ~0x00200000;
        HevcDecWu32(RegTopAddr + VECORE_MODESEL_REG, nTemp, "VECORE_MODESEL_REG");
    }

    nTemp = (pCi->priPixelFormatReg << 4) | (pCi->secPixelFormatReg);  //todo: shouldn't const
    HevcDecWu32(RegTopAddr + VECORE_PRIMARY_OUT_FORMAT_REG, nTemp, "VECORE_PRIMARY_OUT_FORMAT_REG");

    nTemp = pCi->priFrmBufChromaSize;
    HevcDecWu32(RegTopAddr + VECORE_PRI_CHROMA_BUF_LEN_REG, nTemp, "VECORE_PRI_CHROMA_BUF_LEN_REG");

    nTemp = (pCi->priFrmBufChromaStride << 16) | pCi->priFrmBufLumaStride;
    HevcDecWu32(RegTopAddr + VECORE_PRI_FRMBUF_STRIDE_REG, nTemp, "VECORE_PRI_FRMBUF_STRIDE_REG");

    nTemp = (pCi->secFrmBufChromaStride << 16) | pCi->secFrmBufLumaStride;
    HevcDecWu32(RegTopAddr + VECORE_SEC_FRMBUF_STRIDE_REG, nTemp, "VECORE_SEC_FRMBUF_STRIDE_REG");

    //9. Set Rotation/Scaling down information // 50 54 58 80
    pdwTemp = (u32*)(&regHEVC_ExtraCtrl_reg50);
    *pdwTemp = 0;
    if(pCi->bRotationEnable)
        regHEVC_ExtraCtrl_reg50.rotate_angle = pCi->nRotationDegree;
    else
        regHEVC_ExtraCtrl_reg50.rotate_angle = 0;

    if(pCi->secOutputEnabled)
    {
        regHEVC_ExtraCtrl_reg50.scale_precision =
            (pCi->nVerticalScaleDownRatio << 2) | pCi->nHorizonScaleDownRatio;
        regHEVC_ExtraCtrl_reg50.field_scale_mod = 0; //0: both field; 1: only one field
        regHEVC_ExtraCtrl_reg50.bottom_field_sel = 0;//0: top; 1: bottom
    }
    else
    {
        regHEVC_ExtraCtrl_reg50.scale_precision = 0;
        regHEVC_ExtraCtrl_reg50.field_scale_mod = 0;
        regHEVC_ExtraCtrl_reg50.bottom_field_sel = 0;
    }
    HevcDecWu32(RegBaseAddr + HEVC_SDRT_CTRL_REG, *pdwTemp, "regHEVC_ExtraCtrl_reg50");

    /************************ todo: second output frame debug ***************************/
    pdwTemp = (u32*)(&regHEVC_ExtraYBuf_reg54);
    *pdwTemp = 0;
    regHEVC_ExtraYBuf_reg54.addr = pHevcDec->pCurrDPB->SecLuamPhyAddr>> 8;
    HevcDecWu32(RegBaseAddr + HEVC_SDRT_YBUF_REG, *pdwTemp, "regHEVC_ExtraYBuf_reg54");

    pdwTemp = (u32*)(&regHEVC_ExtraCBuf_reg58);
    *pdwTemp = 0;
    regHEVC_ExtraCBuf_reg58.addr = pHevcDec->pCurrDPB->SecChromaPhyAddr>> 8;
    HevcDecWu32(RegBaseAddr + HEVC_SDRT_CBUF_REG, *pdwTemp, "regHEVC_ExtraCBuf_reg58");
    /************************ todo: second output frame debug ***************************/

    pdwTemp = (u32*)(&regHEVC_8BIT_Addr_reg80);
    *pdwTemp = 0;
    regHEVC_8BIT_Addr_reg80.sd_low8_chroma_addr =
        pHevcDec->pCurrDPB->SecChromaPhyAddr & 0xff; // todo: secode frame
    regHEVC_8BIT_Addr_reg80.pri_low8_chroma_addr =
        pHevcDec->pCurrDPB->ChromaPhyAddr & 0xff;
    HevcDecWu32(RegBaseAddr + HEVC_ENTRY_POINT_OFFSET_LOW_ADDR_REG,
        *pdwTemp, "regHEVC_8BIT_Addr_reg80");
}

#if 0
static void  HevcSetDefaultRefInfo(HevcContex *pHevcDec)
{
    HevcSliceHeader *pSh  = &pHevcDec->SliceHeader;
    size_addr RegBaseAddr = pHevcDec->HevcDecRegBaseAddr;
    s32 i           = 0;
    s32 nTemp       = 0;
    s32 nRefIndex   = -1;
    s32 nDeltPoc    = -1;
    s32 nMinDeltPoc = 0x7fffffff;
    HevcFrame *pHf  = NULL;

    //* find the ref index
    for(i = 0; i < HEVC_MAX_DPB_SIZE; i++)
    {
        pHf = &pHevcDec->DPB[i];
        if(pHf->pFbmBuffer == NULL || pHf->pFbmBuffer == pHevcDec->pCurrDPB->pFbmBuffer)
            continue;
        else
        {
            nDeltPoc = pHf->nPoc - pHevcDec->pCurrDPB->nPoc;
            if(nDeltPoc < 0)
                nDeltPoc = -nDeltPoc;
            if(nDeltPoc < nMinDeltPoc)
            {
                nMinDeltPoc = nDeltPoc;
                nRefIndex = pHf->nIndex;
            }
        }
    }
    if(nRefIndex == -1)
    {
        if(pHevcDec->pCurrDPB->nIndex == 0)
        {
            nRefIndex = pHevcDec->pCurrDPB->nIndex + 1;
        }
        else
        {
            nRefIndex = pHevcDec->pCurrDPB->nIndex - 1;
        }
    }
    logd("***** HevcSetDefaultRefInfo, nRefIndex = %d",nRefIndex);

    if(pSh->eSliceType == HEVC_I_SLICE)
        return;

    //* all refs are the same
    tempReferenceList.lt_or_st      = 0;
    tempReferenceList.top_or_bottom = 0; // todo: soppurt bottom field case
    tempReferenceList.r0            = 0;
    tempReferenceList.idx           = nRefIndex;
    tempReferenceList.r1            = 0;
    if(pSh->eSliceType == HEVC_P_SLICE || pSh->eSliceType == HEVC_B_SLICE)
    {
        nTemp = HEVC_REF_LIST_ADDR;
        HevcSramWriteAddr(RegBaseAddr, nTemp, "HEVC_SRAM_PORT_ADDR_REG  0xE0");

        for(i = 0; i < pSh->nNumOfRefs[0]; i += 4)
        {
            nTemp = 0;
            nTemp |= *((volatile unsigned int *)(&tempReferenceList));

            if(i+1 < pSh->nNumOfRefs[0])
            {
                nTemp |= (*((volatile unsigned int *)(&tempReferenceList))) << 8;
            }

            if(i+2 < pSh->nNumOfRefs[0])
            {
                nTemp |= (*((volatile unsigned int *)(&tempReferenceList))) << 16;
            }

            if(i+3 < pSh->nNumOfRefs[0])
            {
                nTemp |= (*((volatile unsigned int *)(&tempReferenceList))) << 24;
            }
            HevcSramWriteData(RegBaseAddr, nTemp, "HEVC_SRAM_PORT_DATA_REG  0xE4");
        }
    }
    if(pSh->eSliceType == HEVC_B_SLICE)
    {
        nTemp = HEVC_REF_LIST_ADDR + 16;
        HevcSramWriteAddr(RegBaseAddr, nTemp, "HEVC_SRAM_PORT_ADDR_REG  0xE0");

        for(i = 0; i < pSh->nNumOfRefs[1]; i += 4)
        {
            nTemp |= *((volatile unsigned int *)(&tempReferenceList));

            if(i+1 < pSh->nNumOfRefs[1])
            {
                nTemp |= (*((volatile unsigned int *)(&tempReferenceList))) << 8;
            }

            if(i+2 < pSh->nNumOfRefs[1])
            {
                nTemp |= (*((volatile unsigned int *)(&tempReferenceList))) << 16;
            }

            if(i+3 < pSh->nNumOfRefs[1])
            {
                nTemp |= (*((volatile unsigned int *)(&tempReferenceList))) << 24;
            }
            HevcSramWriteData(RegBaseAddr, nTemp, "HEVC_SRAM_PORT_DATA_REG  0xE4");
        }
    }
    return;
}
#endif

// configure frame buffer information and reference list information
static void HevcSetFrameBufInfoReg(HevcContex *pHevcDec)
{
#define HevcSetFrameBufInfoRegDebug 1
    size_addr RegBaseAddr = pHevcDec->HevcDecRegBaseAddr;
    HevcSliceHeader *pSh = &pHevcDec->SliceHeader;
    HevcControlInfo *pCi = &pHevcDec->ControlInfo;
    //HevcPPS *pPps = pHevcDec->pPps;
    //HevcSPS *pSps = pHevcDec->pSps;
    volatile u32* pdwTemp;
    s32 nTemp, i, nIndex;
    s32 bRefFrameWithErrorFlag = 0;

    pdwTemp = (u32*)(&regHEVC_NeighborInfoBufAddr_reg60);
    *pdwTemp = 0;
    regHEVC_NeighborInfoBufAddr_reg60.addr = pCi->pNeighbourInfoBufferPhyAddr >> 8;
    HevcDecWu32(RegBaseAddr + HEVC_NEIGHBOR_INFO_ADDR_REG,
        *pdwTemp, "regHEVC_NeighborInfoBufAddr_reg60");

    nIndex = 0;
    //1.set all DPB buffer address
#if 0
    if(1)
    {
        HevcFrame *pHf = pHevcDec->pCurrDPB;
#else
    for(i = 0; i < HEVC_MAX_DPB_SIZE; i++)
    {
        HevcFrame *pHf = &pHevcDec->DPB[i];
        if(pHf->pFbmBuffer == NULL)
            continue;
#endif
        nIndex = pHf->nIndex;

        nTemp = HEVC_FRM_BUF_INFO_ADDR + 0x20 * nIndex;
        HevcSramWriteAddr(RegBaseAddr, nTemp, "HEVC_SRAM_PORT_ADDR_REG  0xE0");

        tempSramFrameBufInfo.poc = pHf->nPoc;
        tempSramFrameBufInfo.bottom_poc = pHf->nPoc; // todo: soppurt bottom field case

        tempSramFrameBufInfo.mvInfo = pHf->MVFrameColPhyAddr >> 8;
        tempSramFrameBufInfo.bottom_mvInfo = pHf->MVFrameColPhyAddr >> 8;
        // todo: soppurt bottom field case

        tempSramFrameBufInfo.lumaBufAddr   = pHf->LuamPhyAddr >> 8;
        if(pHevcDec->bEnableAfbcFlag == 0)
            tempSramFrameBufInfo.chromaBufAddr = pHf->ChromaPhyAddr >> 8;

        HevcSramWriteData(RegBaseAddr, tempSramFrameBufInfo.poc,
            "HEVC_SRAM_PORT_DATA_REG  0xE4");             // in SRAM
        HevcSramWriteData(RegBaseAddr, tempSramFrameBufInfo.bottom_poc,
            "HEVC_SRAM_PORT_DATA_REG  0xE4");        // in SRAM
        HevcSramWriteData(RegBaseAddr, tempSramFrameBufInfo.mvInfo,
            "HEVC_SRAM_PORT_DATA_REG  0xE4");            // in DRAM
        HevcSramWriteData(RegBaseAddr, tempSramFrameBufInfo.bottom_mvInfo,
            "HEVC_SRAM_PORT_DATA_REG  0xE4");  // in DRAM
        HevcSramWriteData(RegBaseAddr, tempSramFrameBufInfo.lumaBufAddr,
            "HEVC_SRAM_PORT_DATA_REG  0xE4");    // in DRAM
        HevcSramWriteData(RegBaseAddr, tempSramFrameBufInfo.chromaBufAddr,
            "HEVC_SRAM_PORT_DATA_REG  0xE4");  // in DRAM
    }

    // 2.set cur picture index
    pdwTemp = (u32*)(&regHEVC_CurRecBufIdx_reg5c);
    *pdwTemp = 0;
    regHEVC_CurRecBufIdx_reg5c.frm_buf_idx = pHevcDec->pCurrDPB->nIndex;
    HevcDecWu32(RegBaseAddr + HEVC_CUR_REC_FRMBUF_IDX_REG, *pdwTemp,
        "regHEVC_CurRecBufIdx_reg5c");

    #if 0
    //3.set ref buffer index
    if(pHevcDec->bCurPicWithoutRefInfo == 1)
    {
        HevcSetDefaultRefInfo(pHevcDec);
        return;
    }
    #endif

    if(pSh->eSliceType == HEVC_I_SLICE)
        return;

    if(pSh->eSliceType == HEVC_P_SLICE || pSh->eSliceType == HEVC_B_SLICE)
    {
        HevcRefPicList *pRpl = &pHevcDec->RefPicList[0];
        HevcFrame *pHf;
        nTemp = HEVC_REF_LIST_ADDR;
        HevcSramWriteAddr(RegBaseAddr, nTemp, "HEVC_SRAM_PORT_ADDR_REG  0xE0");

        for(i = 0; i < pSh->nNumOfRefs[0]; i++)
        {
            logv("ref list0[%d]: poc = %d, errorFlag = %d",
                 pHevcDec->nPoc,  pRpl->Ref[i]->nPoc,  pRpl->Ref[i]->bErrorFrameFlag);
        }

    #if HEVC_ENABLE_CATCH_DDR
        pHevcDec->nCurFrameRef0 = pSh->nNumOfRefs[0];
    #endif

        for(i = 0; i < pSh->nNumOfRefs[0]; i += 4)
        {
            pHf = pRpl->Ref[i];
            nTemp = 0;
            if(pHf == NULL)
            {
                loge(" h265 setting ref reg error. frame == NULL  0  poc: %d, i: %d ",
                    pRpl->List[i], i);
                continue;
            }
            if(pHf->bErrorFrameFlag)
                bRefFrameWithErrorFlag = 1;
//            logd(" rpl[0], pSh->nNumOfRefs[0]: %d; poc: %d, i: %d ",
//                   pSh->nNumOfRefs[0], pHf->nPoc, i);
            tempReferenceList.lt_or_st      = pRpl->bIsLongTerm[i] == 1 ? 1 : 0;
            tempReferenceList.top_or_bottom = 0; // todo: soppurt bottom field case
            tempReferenceList.r0            = 0;
            tempReferenceList.idx           = pHf->nIndex;
            tempReferenceList.r1            = 0;
            nTemp |= *((volatile unsigned int *)(&tempReferenceList));

            if(i+1 < pSh->nNumOfRefs[0])
            {
                pHf = pRpl->Ref[i + 1];
                if(pHf == NULL)
                {
                    loge(" h265 setting ref reg error. frame == NULL  1 poc: %d, i: %d ",
                        pRpl->List[i + 1], i + 1);
                    continue;
                }
                if(pHf->bErrorFrameFlag)
                    bRefFrameWithErrorFlag = 1;
//                logd(" rpl[0], pSh->nNumOfRefs[0]: %d; poc: %d, i: %d ",
//                       pSh->nNumOfRefs[0], pHf->nPoc, i + 1);
                tempReferenceList.lt_or_st      = pRpl->bIsLongTerm[i + 1] == 1 ? 1 : 0;
                tempReferenceList.top_or_bottom = 0; // todo: soppurt bottom field case
                tempReferenceList.r0            = 0;
                tempReferenceList.idx           = pHf->nIndex;
                tempReferenceList.r1            = 0;
                nTemp |= (*((volatile unsigned int *)(&tempReferenceList))) << 8;
            }

            if(i+2 < pSh->nNumOfRefs[0])
            {
                pHf = pRpl->Ref[i + 2];
                if(pHf == NULL)
                {
                    loge(" h265 setting ref reg error. frame == NULL  2  poc: %d, i: %d ",
                        pRpl->List[i + 2], i + 2);
                    continue;
                }
                if(pHf->bErrorFrameFlag)
                    bRefFrameWithErrorFlag = 1;
//                logd(" rpl[0], pSh->nNumOfRefs[0]: %d; poc: %d, i: %d ",
//                       pSh->nNumOfRefs[0], pHf->nPoc, i + 2);
                tempReferenceList.lt_or_st      = pRpl->bIsLongTerm[i + 2] == 1 ? 1 : 0;
                tempReferenceList.top_or_bottom = 0; // todo: soppurt bottom field case
                tempReferenceList.r0            = 0;
                tempReferenceList.idx           = pHf->nIndex;
                tempReferenceList.r1            = 0;
                nTemp |= (*((volatile unsigned int *)(&tempReferenceList))) << 16;
            }

            if(i+3 < pSh->nNumOfRefs[0])
            {
                pHf = pRpl->Ref[i + 3];
                if(pHf == NULL)
                {
                    loge(" h265 setting ref reg error. frame == NULL  3  poc: %d, i: %d ",
                        pRpl->List[i + 3], i + 3);
                    continue;
                }
                if(pHf->bErrorFrameFlag)
                    bRefFrameWithErrorFlag = 1;
//                logd(" rpl[0], pSh->nNumOfRefs[0]: %d; poc: %d, i: %d ",
//                        pSh->nNumOfRefs[0], pHf->nPoc, i + 3);
                tempReferenceList.lt_or_st      = pRpl->bIsLongTerm[i + 3] == 1 ? 1 : 0;
                tempReferenceList.top_or_bottom = 0; // todo: soppurt bottom field case
                tempReferenceList.r0            = 0;
                tempReferenceList.idx           = pHf->nIndex;
                tempReferenceList.r1            = 0;
                nTemp |= (*((volatile unsigned int *)(&tempReferenceList))) << 24;
            }
            HevcSramWriteData(RegBaseAddr, nTemp, "HEVC_SRAM_PORT_DATA_REG  0xE4");
        }
    }
    if(pSh->eSliceType == HEVC_B_SLICE)
    {
        HevcRefPicList *pRpl = &pHevcDec->RefPicList[1];
        HevcFrame *pHf;
        nTemp = HEVC_REF_LIST_ADDR + 16;
        HevcSramWriteAddr(RegBaseAddr, nTemp, "HEVC_SRAM_PORT_ADDR_REG  0xE0");
        for(i = 0; i < pSh->nNumOfRefs[1]; i++)
        {
            logv("ref list1[%d]: poc = %d, errorFlag = %d",
                  pHevcDec->nPoc, pRpl->Ref[i]->nPoc,  pRpl->Ref[i]->bErrorFrameFlag);
        }

    #if HEVC_ENABLE_CATCH_DDR
        pHevcDec->nCurFrameRef1 = pSh->nNumOfRefs[1];
    #endif

        for(i = 0; i < pSh->nNumOfRefs[1]; i += 4)
        {
            pHf = pRpl->Ref[i];
            nTemp = 0;
            if(pHf == NULL)
            {
                loge(" h265 setting ref reg error. frame == NULL  0  poc: %d, i: %d ",
                    pRpl->List[i], i);
                continue;
            }
            if(pHf->bErrorFrameFlag)
                bRefFrameWithErrorFlag = 1;
//            logd(" rpl[1], pSh->nNumOfRefs[1]: %d; poc: %d, i: %d ",
//                    pSh->nNumOfRefs[1], pHf->nPoc, i);
            tempReferenceList.lt_or_st      = pRpl->bIsLongTerm[i] == 1 ? 1 : 0;
            tempReferenceList.top_or_bottom = 0; // todo: soppurt bottom field case
            tempReferenceList.r0            = 0;
            tempReferenceList.idx           = pHf->nIndex;
            tempReferenceList.r1            = 0;
            nTemp |= *((volatile unsigned int *)(&tempReferenceList));

            if(i+1 < pSh->nNumOfRefs[1])
            {
                pHf = pRpl->Ref[i + 1];
                if(pHf == NULL)
                {
                    loge(" h265 setting ref reg error. frame == NULL  1  poc: %d, i: %d ",
                        pRpl->List[i + 1], i + 1);
                    continue;
                }
                if(pHf->bErrorFrameFlag)
                    bRefFrameWithErrorFlag = 1;
//                logd(" rpl[1], pSh->nNumOfRefs[1]: %d; poc: %d, i: %d ",
//                        pSh->nNumOfRefs[1], pHf->nPoc, i + 1);
                tempReferenceList.lt_or_st      = pRpl->bIsLongTerm[i + 1] == 1 ? 1 : 0;
                tempReferenceList.top_or_bottom = 0; // todo: soppurt bottom field case
                tempReferenceList.r0            = 0;
                tempReferenceList.idx           = pHf->nIndex;
                tempReferenceList.r1            = 0;
                nTemp |= (*((volatile unsigned int *)(&tempReferenceList))) << 8;
            }

            if(i+2 < pSh->nNumOfRefs[1])
            {
                pHf = pRpl->Ref[i + 2];
                if(pHf == NULL)
                {
                    loge(" h265 setting ref reg error. frame == NULL  2  poc: %d, i: %d ",
                        pRpl->List[i + 2], i + 2);
                    continue;
                }
                if(pHf->bErrorFrameFlag)
                    bRefFrameWithErrorFlag = 1;
//                logd(" rpl[1], pSh->nNumOfRefs[1]: %d; poc: %d, i: %d ",
//                       pSh->nNumOfRefs[1], pHf->nPoc, i + 2);
                tempReferenceList.lt_or_st      = pRpl->bIsLongTerm[i + 2] == 1 ? 1 : 0;
                tempReferenceList.top_or_bottom = 0; // todo: soppurt bottom field case
                tempReferenceList.r0            = 0;
                tempReferenceList.idx           = pHf->nIndex;
                tempReferenceList.r1            = 0;
                nTemp |= (*((volatile unsigned int *)(&tempReferenceList))) << 16;
            }

            if(i+3 < pSh->nNumOfRefs[1])
            {
                pHf = pRpl->Ref[i + 3];
                if(pHf == NULL)
                {
                    loge(" h265 setting ref reg error. frame == NULL  3  poc: %d, i: %d ",
                        pRpl->List[i + 3], i + 3);
                    continue;
                }
                if(pHf->bErrorFrameFlag)
                    bRefFrameWithErrorFlag = 1;
//                logd(" rpl[1], pSh->nNumOfRefs[1]: %d; poc: %d, i: %d ",
//                        pSh->nNumOfRefs[1], pHf->nPoc, i + 3);
                tempReferenceList.lt_or_st      = pRpl->bIsLongTerm[i + 3] == 1 ? 1 : 0;
                tempReferenceList.top_or_bottom = 0; // todo: soppurt bottom field case
                tempReferenceList.r0            = 0;
                tempReferenceList.idx           = pHf->nIndex;
                tempReferenceList.r1            = 0;
                nTemp |= (*((volatile unsigned int *)(&tempReferenceList))) << 24;
            }
            HevcSramWriteData(RegBaseAddr, nTemp, "HEVC_SRAM_PORT_DATA_REG  0xE4");
        }
    }
    logv("bRefFrameWithErrorFlag = %d, poc = %d",bRefFrameWithErrorFlag, pHevcDec->pCurrDPB->nPoc);
    if(bRefFrameWithErrorFlag == 1)
        pHevcDec->pCurrDPB->bErrorFrameFlag = 1;
}

static void HevcSetWeightPredReg(HevcContex *pHevcDec)
{
    size_addr RegBaseAddr = pHevcDec->HevcDecRegBaseAddr;
    HevcSliceHeader *pSh = &pHevcDec->SliceHeader;
    //HevcControlInfo *pCi = &pHevcDec->ControlInfo;
    HevcPPS *pPps = pHevcDec->pPps;
    //HevcSPS *pSps = pHevcDec->pSps;
    //volatile u32* pdwTemp;
    //s32 nTemp, i, nIndex, nMvInfoSize;

    if ((pSh->eSliceType == HEVC_B_SLICE && pPps->bWeightedBipredFlag) ||
         (pSh->eSliceType == HEVC_P_SLICE && pPps->bWeightedPredFlag))
    {
        WpScalingParam  *wp[16];
        int iRefIdx,iNumRef;
        int iNbRef = (pSh->eSliceType == HEVC_B_SLICE ) ? (2) : (1);
        u32 dwTemp=0, dwTemp2= 0;

        for (iNumRef=0; iNumRef < iNbRef; iNumRef++)
        {
            s32  eRefPicList = (iNumRef ? 1 : 0);

            for (iRefIdx=0; iRefIdx < pSh->nNumOfRefs[eRefPicList]; iRefIdx++)
            {
                wp[iRefIdx] = pSh->m_weightPredTable[eRefPicList][iRefIdx];
            }

            //Y
            dwTemp2 = HEVC_WEIGHT_PRED_PARAM_ADDR+96*iNumRef;
            HevcSramWriteAddr(RegBaseAddr, dwTemp2, "HEVC_SRAM_PORT_ADDR_REG  0xE0");
//            HevcSramWriteData(RegBaseAddr, tempSramFrameBufInfo.chromaBufAddr,
//               "HEVC_SRAM_PORT_DATA_REG  0xE4");
//            SRAM_WRITE_ADDR_U32(dwTemp2);

            for (iRefIdx=0; iRefIdx < pSh->nNumOfRefs[eRefPicList]; iRefIdx += 2)
            {
                dwTemp = (((unsigned int )(wp[iRefIdx][0].iOffset& 0xff))<<8) |
                    ((unsigned int )(pSh->hevc_weight_buf[iNumRef][0][iRefIdx] & 0xff));

                if(iRefIdx+1 < pSh->nNumOfRefs[eRefPicList])
                    dwTemp |= (((unsigned int )(wp[iRefIdx+1][0].iOffset&0xff))<<24) |
                    (((unsigned int )(pSh->hevc_weight_buf[iNumRef][0][iRefIdx + 1]&0xff))<<16);

//                Reg_Write_U32(HEVC_REG_BASE, HEVC_SRAM_PORT_DATA_REG, dwTemp);
                HevcSramWriteData(RegBaseAddr, dwTemp, "HEVC_SRAM_PORT_DATA_REG  0xE4");
//                SRAM_WRITE_DATA_U32(dwTemp);
            }

            //Cb,Cr
            dwTemp2 = HEVC_WEIGHT_PRED_PARAM_ADDR+96*iNumRef+32;
//            Reg_Write_U32(HEVC_REG_BASE, HEVC_SRAM_PORT_ADDR_REG, dwTemp2);
//            SRAM_WRITE_ADDR_U32(dwTemp2);
            HevcSramWriteAddr(RegBaseAddr, dwTemp2, "HEVC_SRAM_PORT_ADDR_REG  0xE0");

            for (iRefIdx=0 ; iRefIdx< pSh->nNumOfRefs[eRefPicList] ; iRefIdx+=2)
            {
                dwTemp =  (((unsigned int )(wp[iRefIdx][1].iOffset&0xff))<<8) |
                    ((unsigned int )(pSh->hevc_weight_buf[iNumRef][1][iRefIdx]&0xff));
                dwTemp |= ((((unsigned int )(wp[iRefIdx][2].iOffset&0xff))<<24) |
                    (((unsigned int )(pSh->hevc_weight_buf[iNumRef][2][iRefIdx]&0xff)))<<16);
//                Reg_Write_U32(HEVC_REG_BASE, HEVC_SRAM_PORT_DATA_REG, dwTemp);
//                SRAM_WRITE_DATA_U32(dwTemp);
                HevcSramWriteData(RegBaseAddr, dwTemp, "HEVC_SRAM_PORT_DATA_REG  0xE4");

                if(iRefIdx+1 < pSh->nNumOfRefs[eRefPicList])
                {
                    dwTemp = (((unsigned int )(wp[iRefIdx+1][1].iOffset&0xff))<<8) |
                        (((unsigned int )(pSh->hevc_weight_buf[iNumRef][1][iRefIdx+1]&0xff)));
                    dwTemp |= (((unsigned int )(wp[iRefIdx+1][2].iOffset&0xff))<<24) |
                        (((unsigned int )(pSh->hevc_weight_buf[iNumRef][2][iRefIdx+1]&0xff))<<16);
//                    Reg_Write_U32(HEVC_REG_BASE, HEVC_SRAM_PORT_DATA_REG,dwTemp);
//                    SRAM_WRITE_DATA_U32(dwTemp);
                    HevcSramWriteData(RegBaseAddr, dwTemp, "HEVC_SRAM_PORT_DATA_REG  0xE4");
                }
            }
        }
    }
}

static void HevcSetTilesInfoReg(HevcContex *pHevcDec)
{
    size_addr RegBaseAddr = pHevcDec->HevcDecRegBaseAddr;
    u32 nTemp, nTileId;
    s32 j;
    //u32 nCtbAddrTs;
    volatile u32* pdwTemp;
    HevcSliceHeader *pSh = &pHevcDec->SliceHeader;
    //HevcSPS *pSps = pHevcDec->pSps;
    HevcPPS *pPps = pHevcDec->pPps;
    struct ScMemOpsS *_memops = pHevcDec->pConfig->memops;

    if(pSh->nNumEntryPointOffsets == 0)
    {
        /* slices in tile's case */
        /* 1. locate this slice's tile id */
        nTileId = pPps->TileIdInRs[pSh->nSliceSegmentAddr];

        if(pHevcDec->nDecIpVersion == 0x31010)
        {
            pdwTemp = (u32*)(&regHEVC_TileStartCTBAddr_reg68_v02);
            *pdwTemp = 0;
            regHEVC_TileStartCTBAddr_reg68_v02.CTB_x = pPps->TilePosInfo[nTileId].nTileStartAddrX;
            regHEVC_TileStartCTBAddr_reg68_v02.CTB_y = pPps->TilePosInfo[nTileId].nTileStartAddrY;
            HevcDecWu32(RegBaseAddr + HEVC_TILE_START_ADDR_REG,
                *pdwTemp, "regHEVC_TileStartCTBAddr_reg68");

            pdwTemp = (u32*)(&regHEVC_TileEndCTBAddr_reg6c_v02);
            *pdwTemp = 0;
            regHEVC_TileEndCTBAddr_reg6c_v02.CTB_x = pPps->TilePosInfo[nTileId].nTileEndAddrX;
            regHEVC_TileEndCTBAddr_reg6c_v02.CTB_y = pPps->TilePosInfo[nTileId].nTileEndAddrY;
            HevcDecWu32(RegBaseAddr + HEVC_TILE_END_ADDR_REG,
                *pdwTemp, "regHEVC_TileStartCTBAddr_reg68");
        }
        else
        {
            pdwTemp = (u32*)(&regHEVC_TileStartCTBAddr_reg68_v01);
            *pdwTemp = 0;
            regHEVC_TileStartCTBAddr_reg68_v01.CTB_x = pPps->TilePosInfo[nTileId].nTileStartAddrX;
            regHEVC_TileStartCTBAddr_reg68_v01.CTB_y = pPps->TilePosInfo[nTileId].nTileStartAddrY;
            HevcDecWu32(RegBaseAddr + HEVC_TILE_START_ADDR_REG,
                *pdwTemp, "regHEVC_TileStartCTBAddr_reg68");

            pdwTemp = (u32*)(&regHEVC_TileEndCTBAddr_reg6c_v01);
            *pdwTemp = 0;
            regHEVC_TileEndCTBAddr_reg6c_v01.CTB_x = pPps->TilePosInfo[nTileId].nTileEndAddrX;
            regHEVC_TileEndCTBAddr_reg6c_v01.CTB_y = pPps->TilePosInfo[nTileId].nTileEndAddrY;
            HevcDecWu32(RegBaseAddr + HEVC_TILE_END_ADDR_REG,
                *pdwTemp, "regHEVC_TileStartCTBAddr_reg68");
        }

    }
    else
    {
        /* tiles in slice's case */
        for(j = 0; j < pSh->nNumEntryPointOffsets + 1; j++)
        {
            if(j == 0)
            {
                nTileId = pPps->TileIdInRs[pSh->nSliceSegmentAddr];

                logv(" j: %d tile id: %d, start x: %d, y: %d; end x: %d, y: %d",
                        j, nTileId,
                        pPps->TilePosInfo[nTileId].nTileStartAddrX,
                        pPps->TilePosInfo[nTileId].nTileStartAddrY,
                        pPps->TilePosInfo[nTileId].nTileEndAddrX,
                        pPps->TilePosInfo[nTileId].nTileEndAddrY);

                if(pHevcDec->nDecIpVersion == 0x31010)
                {
                    pdwTemp = (u32*)(&regHEVC_TileStartCTBAddr_reg68_v02);
                    *pdwTemp = 0;
                    regHEVC_TileStartCTBAddr_reg68_v02.CTB_x
                        = pPps->TilePosInfo[nTileId].nTileStartAddrX;
                    regHEVC_TileStartCTBAddr_reg68_v02.CTB_y
                        = pPps->TilePosInfo[nTileId].nTileStartAddrY;
                    HevcDecWu32(RegBaseAddr + HEVC_TILE_START_ADDR_REG, *pdwTemp,
                        "regHEVC_TileStartCTBAddr_reg68");

                    pdwTemp = (u32*)(&regHEVC_TileEndCTBAddr_reg6c_v02);
                    *pdwTemp = 0;
                    regHEVC_TileEndCTBAddr_reg6c_v02.CTB_x
                        = pPps->TilePosInfo[nTileId].nTileEndAddrX;
                    regHEVC_TileEndCTBAddr_reg6c_v02.CTB_y
                        = pPps->TilePosInfo[nTileId].nTileEndAddrY;
                    HevcDecWu32(RegBaseAddr + HEVC_TILE_END_ADDR_REG, *pdwTemp,
                        "regHEVC_TileStartCTBAddr_reg68");
                }
                else
                {
                    pdwTemp = (u32*)(&regHEVC_TileStartCTBAddr_reg68_v01);
                    *pdwTemp = 0;
                    regHEVC_TileStartCTBAddr_reg68_v01.CTB_x
                        = pPps->TilePosInfo[nTileId].nTileStartAddrX;
                    regHEVC_TileStartCTBAddr_reg68_v01.CTB_y
                        = pPps->TilePosInfo[nTileId].nTileStartAddrY;
                    HevcDecWu32(RegBaseAddr + HEVC_TILE_START_ADDR_REG, *pdwTemp,
                        "regHEVC_TileStartCTBAddr_reg68");

                    pdwTemp = (u32*)(&regHEVC_TileEndCTBAddr_reg6c_v01);
                    *pdwTemp = 0;
                    regHEVC_TileEndCTBAddr_reg6c_v01.CTB_x
                        = pPps->TilePosInfo[nTileId].nTileEndAddrX;
                    regHEVC_TileEndCTBAddr_reg6c_v01.CTB_y
                        = pPps->TilePosInfo[nTileId].nTileEndAddrY;
                    HevcDecWu32(RegBaseAddr + HEVC_TILE_END_ADDR_REG, *pdwTemp,
                        "regHEVC_TileStartCTBAddr_reg68");
                }

            }
            else
            {
                nTileId = pPps->TileIdInRs[pSh->nSliceSegmentAddr] + j;
                logv(" j: %d tile id: %d, start x: %d, y: %d; end x: %d, y: %d, \
                        entry point offset: %d", j, nTileId,
                        pPps->TilePosInfo[nTileId].nTileStartAddrX,
                        pPps->TilePosInfo[nTileId].nTileStartAddrY,
                        pPps->TilePosInfo[nTileId].nTileEndAddrX,
                        pPps->TilePosInfo[nTileId].nTileEndAddrY,
                        pSh->EntryPointOffset[j - 1]);
                if(pPps->bEntropyCodingSyncEnabledFlag)
                    pSh->pRegEntryPointOffset[pSh->nLastEntryPointOffset + j] =
                        pSh->EntryPointOffset[j - 1];
                else
                {
                    pSh->pRegEntryPointOffset[pSh->nLastEntryPointOffset + 4*(j-1)] =
                        pSh->EntryPointOffset[j - 1];
                    nTemp = (pPps->TilePosInfo[nTileId].nTileStartAddrY << 16) |
                        pPps->TilePosInfo[nTileId].nTileStartAddrX;
                    pSh->pRegEntryPointOffset[pSh->nLastEntryPointOffset + 4*(j-1) + 1] = 0;
                    pSh->pRegEntryPointOffset[pSh->nLastEntryPointOffset + 4*(j-1) + 2] = nTemp;
                    nTemp = (pPps->TilePosInfo[nTileId].nTileEndAddrY << 16) |
                        pPps->TilePosInfo[nTileId].nTileEndAddrX;
                    pSh->pRegEntryPointOffset[pSh->nLastEntryPointOffset + 4*(j-1) + 3] = nTemp;
                    if(0)
                    {
                        s32 *p = pSh->pRegEntryPointOffset + pSh->nLastEntryPointOffset + 4*(j-1);
                        logd("pSh->pRegEntryPointOffset: %d,  %d,  %d,  %d",
                            p[0], p[1], p[2], p[3]);
                    }
                }
            }
        }
    }
    CdcMemFlushCache(_memops, pSh->pRegEntryPointOffset,
            pSh->nLastEntryPointOffset * sizeof(s32) +
            (pSh->nNumEntryPointOffsets + 1)*sizeof(s32)*4);
//    AdapterMemFlushCache(pSh->pRegEntryPointOffset,
//        HEVC_ENTRY_POINT_OFFSET_BUFFER_SIZE);

    nTemp = (pSh->pRegEntryPointOffsetPhyAddr + pSh->nLastEntryPointOffset * 4) >> 8;
    HevcDecWu32(RegBaseAddr + HEVC_ENTRY_POINT_OFFSET_ADDR_REG, nTemp,
        "regHEVC_EntryPointOffset_reg64");

    pdwTemp = (u32*)(&regHEVC_8BIT_Addr_reg80);
    /* reg80 is global, so no need the register value */
    *pdwTemp = HevcDecRu32(RegBaseAddr + HEVC_ENTRY_POINT_OFFSET_LOW_ADDR_REG,
    "regHEVC_8BIT_Addr_reg80");
    regHEVC_8BIT_Addr_reg80.entry_point_addr = (pSh->pRegEntryPointOffsetPhyAddr +
        pSh->nLastEntryPointOffset * 4) & 0xff;
    HevcDecWu32(RegBaseAddr + HEVC_ENTRY_POINT_OFFSET_LOW_ADDR_REG, *pdwTemp,
        "regHEVC_8BIT_Addr_reg80");

    if(pPps->bEntropyCodingSyncEnabledFlag)
        pSh->nLastEntryPointOffset += pSh->nNumEntryPointOffsets;
    else
        pSh->nLastEntryPointOffset += pSh->nNumEntryPointOffsets * 4;
}

void HevcInterruptQuery(HevcContex *pHevcDec)
{
    s32 reg38Status;
    size_addr RegBaseAddr = pHevcDec->HevcDecRegBaseAddr;
    regHEVC_FUNCTION_STATUS regFuctionStatusReg38_temp;
    while(1)
    {
        reg38Status = HevcDecRu32(RegBaseAddr + HEVC_FUNCTION_STATUS_REG,
                   "HEVC_FUNCTION_STATUS_REG");
        regFuctionStatusReg38_temp = *((regHEVC_FUNCTION_STATUS*)(&reg38Status));
        if(regFuctionStatusReg38_temp.slice_dec_finish == 1)
           break;
        else
           usleep(10);
    }
}
s32 HevcVeStatusInfo(HevcContex *pHevcDec, s32 bVeInterruptErrorFlag)
{
    s32 nRet = 0;
    s32 nTemp, nCtuNum, reg38Status;
    size_addr RegBaseAddr = pHevcDec->HevcDecRegBaseAddr;
    size_addr RegTopAddr  = pHevcDec->HevcTopRegBaseAddr;
    regHEVC_FUNCTION_STATUS regFuctionStatusReg38_temp;
    //volatile u32* pdwTemp;

    reg38Status = HevcDecRu32(RegBaseAddr + HEVC_FUNCTION_STATUS_REG,
        "HEVC_FUNCTION_STATUS_REG");
    regFuctionStatusReg38_temp = *((regHEVC_FUNCTION_STATUS*)(&reg38Status));

    nCtuNum = HevcDecRu32(RegBaseAddr + HEVC_DECCTU_NUM_REG,
        "regHEVC_DecCUTNum_reg3c");
    pHevcDec->debug.nCtuNum = nCtuNum;
    logv(" hevc after ve interrup, ctu num: %d, poc = %d, ctuSize = %d",
        nCtuNum, pHevcDec->nPoc, pHevcDec->pSps->nCtbSize);

    if(bVeInterruptErrorFlag == 1)
    {
        logd(" hevc after ve interrup, ctu num: %d, poc = %d, ctuSize = %d",
             nCtuNum, pHevcDec->nPoc, pHevcDec->pSps->nCtbSize);
        logd("after ve interrup, ctu num: %d, status register value: %x ",
             nCtuNum, reg38Status);
        logd("status register detail value: ");
        logd("     slice_dec_finish  = %d",regFuctionStatusReg38_temp.slice_dec_finish);
        logd("     slice_dec_error   = %d",regFuctionStatusReg38_temp.slice_dec_error);
        logd("     vld_data_req      = %d",regFuctionStatusReg38_temp.vld_data_req);
        logd("     overtime          = %d",regFuctionStatusReg38_temp.overtime);
        logd("     r0                = %d",regFuctionStatusReg38_temp.r0);
        logd("     vld_busy          = %d",regFuctionStatusReg38_temp.vld_busy);
        logd("     is_busy           = %d",regFuctionStatusReg38_temp.is_busy);
        logd("     mvp_busy          = %d",regFuctionStatusReg38_temp.mvp_busy);
        logd("     iqit_busy         = %d",regFuctionStatusReg38_temp.iqit_busy);
        logd("     mcri_busy         = %d",regFuctionStatusReg38_temp.mcri_busy);
        logd("     intra_pred_busy   = %d",regFuctionStatusReg38_temp.intra_pred_busy);
        logd("     irec_busy         = %d",regFuctionStatusReg38_temp.irec_busy);
        logd("     dblk_busy         = %d",regFuctionStatusReg38_temp.dblk_busy);
        logd("     more_data_flag    = %d",regFuctionStatusReg38_temp.more_data_flag);
        logd("     interm_busy       = %d",regFuctionStatusReg38_temp.interm_busy);
        logd("     it_busy           = %d",regFuctionStatusReg38_temp.it_busy);
        logd("     bs_dma_busy       = %d",regFuctionStatusReg38_temp.bs_dma_busy);
        logd("     wb_busy           = %d",regFuctionStatusReg38_temp.wb_busy);
        logd("     stcd_busy         = %d",regFuctionStatusReg38_temp.stcd_busy);
        logd("     startcode_type    = %d",regFuctionStatusReg38_temp.startcode_type);
        logd("     r1                = %d",regFuctionStatusReg38_temp.r1);
        HevcSettingRegDebug(pHevcDec);
    }

    if(regFuctionStatusReg38_temp.slice_dec_finish == 1)
    {
        nRet = 0;
    }
    if(regFuctionStatusReg38_temp.slice_dec_error == 1)
    {
        logw("HEVC slice dec error, poc: %d, ctuNum = %d, totalCtuNum = %d",
                pHevcDec->nPoc, nCtuNum, pHevcDec->pSps->nCtbSize);

        //logw("HEVC slice dec error, slice ctu addr: (%d, %d), status register value: %x ",
        //     regHEVC_CTBAddr_reg2C.CTB_x, regHEVC_CTBAddr_reg2C.CTB_y, reg38Status);
        nRet = -1;
        pHevcDec->bErrorFrameFlag = 1;
    }
    if(regFuctionStatusReg38_temp.vld_data_req == 1)
    {
        logd("HEVC vld data request");
        nRet = -1;
    }
    if(regFuctionStatusReg38_temp.overtime == 1)
    {
        nRet = -1;
        logd("HEVC overtime");
    }

    reg38Status = HevcDecRu32(RegBaseAddr + HEVC_FUNCTION_STATUS_REG,
        "HEVC_FUNCTION_STATUS_REG");
    reg38Status &= 0x7;
    HevcDecWu32(RegBaseAddr + HEVC_FUNCTION_STATUS_REG, reg38Status,
        "regHEVC_FunctionStatus_reg38");
//    logd("  hardware decode finish   . write status reg: %x", reg38Status);
    nTemp = HevcDecRu32(RegTopAddr + VECORE_CYCLES_COUNTER_REG,
        "VECORE_CYCLES_COUNTER_REG");
    nTemp &= ~(1<<31);
    pHevcDec->debug.nVeCounter += nTemp;

    return nRet;
}

s32 HevcDecodeOneSlice(HevcContex *pHevcDec)
{
    s32 nTemp;
    size_addr RegBaseAddr = pHevcDec->HevcDecRegBaseAddr;
    size_addr RegTopAddr  = pHevcDec->HevcTopRegBaseAddr;
    volatile u32* pdwTemp;

    /* step 1: Stream Information Register 40, 44, 48, 4c*/
//    logd(" HevcDecodeOneSlice() setting reg start..... ");
    if(pHevcDec->bHardwareGetBitsFlag == 0)
    {
#if 0
        HevcSetStreamInfoShowDebug(pHevcDec);
#endif
        HevcSetStreamInfoReg(pHevcDec);
    }

    /* step 2: nal unit Header Information Register */
    HevcSetNalHeaderReg(pHevcDec);

    /* step 3: Sps Register 04, 08, 0c*/
    HevcSetSpsReg(pHevcDec);

    /* step 4: Pps Register 10, 14 */
    HevcSetPpsReg(pHevcDec);

    /* step 5: ScalingList */
    HevcSetScalingListReg(pHevcDec);

    /* step 6: slice header reg */
    HevcSetSliceHeaderReg(pHevcDec);

    /* step 7: slice header reg */
    HevcSetOutputConfigReg(pHevcDec);

    if(pHevcDec->pPps->bTilesEnabledFlag)
        HevcSetTilesInfoReg(pHevcDec);

    HevcSetFrameBufInfoReg(pHevcDec);

    HevcSetWeightPredReg(pHevcDec);

    #if 1
            //0. set lower_2bit_addr_offset
    if(pHevcDec->b10BitStreamFlag)
    {
        HevcFrame *pTmpHf = &pHevcDec->DPB[pHevcDec->pCurrDPB->nIndex];
        logv("nLower2bitBufOffset = %d, stride = %d",
             pTmpHf->nLower2bitBufOffset,pTmpHf->nLower2bitStride);

        pdwTemp = (u32*)(&regHEVC_lower_2bit_addr_first_output_reg84);
        *pdwTemp = 0;
        regHEVC_lower_2bit_addr_first_output_reg84.first_lower_2bit_offset_addr
              = pTmpHf->nLower2bitBufOffset;
        HevcDecWu32(RegBaseAddr + HEVC_LOWER_2BIT_ADDR_OF_FIRST_OUTPUT_REG,
            *pdwTemp, "regHEVC_lower_2bit_addr_first_output_reg84");

    u32 tmptmp = HevcDecRu32(RegBaseAddr + HEVC_LOWER_2BIT_ADDR_OF_FIRST_OUTPUT_REG,
                             "regHEVC_lower_2bit_addr_first_output_reg84");
    logv("**** read register 84 value = %08x",tmptmp);
        //* set lower_2bit_addr_stride
        pdwTemp = (u32*)(&regHEVC_10bit_configure_reg8c);
        *pdwTemp = 0;
        regHEVC_10bit_configure_reg8c.lower_2bit_data_stride_first_output
            = pTmpHf->nLower2bitStride;
    //*pdwTemp = 0x00040100;
        HevcDecWu32(RegBaseAddr + HEVC_10BIT_CONFIGURE_REG,
            *pdwTemp, "regHEVC_10bit_configure_reg8c");
    }

    #endif
#if (HEVC_DEBUG_FUNCTION_ENABLE | HEVC_SHOW_REG_INFO)
    HevcSettingRegDebug(pHevcDec); /* for debug. show register value */
#endif

#if HEVC_DECODE_TIME_INFO
    pHevcDec->debug.nCurrTimeHw = HevcGetCurrentTime();
#endif
    nTemp = 0;
    HevcDecWu32(RegTopAddr + VECORE_CYCLES_COUNTER_REG, nTemp, "VECORE_CYCLES_COUNTER_REG");
    nTemp = (1 << 31);
    HevcDecWu32(RegTopAddr + VECORE_CYCLES_COUNTER_REG, nTemp, "VECORE_CYCLES_COUNTER_REG");

    pdwTemp = (u32*)(&regHEVC_TriggerType_reg34);
    regHEVC_TriggerType_reg34.trigger_type_pul = 0x08;
    regHEVC_TriggerType_reg34.stcd_type = 0;
    HevcDecWu32(RegBaseAddr + HEVC_TRIGGER_TYPE_REG, *pdwTemp, "regHEVC_TriggerType_reg34");

    logv(" HevcDecodeOneSlice() setting reg end..... ");

    return 0;
}
