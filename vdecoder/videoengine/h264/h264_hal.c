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
* File : h264_hal.c
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#include "h264_hal.h"
#include "h264_dec.h"
#include "h264_func.h"

#define H264_WEIGHT_PREDPARA_LEN    768        //32*3*2*4 bytes 0x300
#define H264_QUANT_MATRIX_LEN        224        //(4*6+16*2)*4 bytes 0xe0
#define H264_REFLIST_LEN            72        //36*2 bytes
#define H264_FRMBUF_INFO_LEN        576        //8*4*18 bytes 0x240

#define H264_WEIGHT_PREDPARA_ADDR    0x000
#define H264_FRMBUF_INFO_ADDR        0x400
#define H264_QUANT_MATRIX_ADDR        0x800
#define H264_REFLIST_ADDR            H264_FRMBUF_INFO_ADDR+H264_FRMBUF_INFO_LEN

#if H264_DEBUG_PRINTF_REGISTER
#define PRINT_REG logd
#else
#define PRINT_REG logv
#endif

u32 H264GetBitCountSw(void* param);

void H264PrintRegister(H264DecCtx *h264DecCtx)
{
    CEDARC_UNUSE(H264PrintRegister);
    u32 i = 0;
    u32* reg_base = NULL;
    H264Dec* h264Dec = NULL;
    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;
    H264Context* hCtx = NULL;
    s32 nCurPoc;

    if(h264Dec->nDecStreamIndex == 0)
        hCtx = h264Dec->pHContext;
    else
        hCtx = h264Dec->pMinorHContext;

    if(hCtx->nPicStructure != PICT_FRAME)
    {
        nCurPoc = hCtx->frmBufInf.pCurPicturePtr->nFieldPoc[hCtx->nPicStructure==PICT_BOTTOM_FIELD];
    }
    else
    {
        nCurPoc = hCtx->frmBufInf.pCurPicturePtr->nPoc;
    }

    logd(" ---h264 reg info, nCurFrmNum: %d, header frame num: %d, poc: %d ----",
            hCtx->nCurFrmNum, hCtx->nFrmNum, nCurPoc);
#if 0
    //logv("**************top register\n");
    reg_base = ve_get_reglist( REG_GROUP_VETOP);

    for(i=0;i<16;i++)
    {
        //logv("%02x, 0x%08x 0x%08x 0x%08x 0x%08x \n",
        //      i, reg_base[4*i],reg_base[4*i+1],reg_base[4*i+2],reg_base[4*i+3]);
    }
    //logv("\n");
#endif

    reg_base = (u32*)h264Dec->nRegisterBaseAddr;
    //logv("********* reg_base =%p, %p\n",
    //reg_base, ve_get_reglist( REG_GROUP_H264_DECODER));
    if(1)
    {
        u8* p = hCtx->vbvInfo.pReadPtr;
        logd(" bit stream[ 0- 7]: %x, %x, %x, %x,   %x, %x, %x, %x",
                p[0],p[1],p[2],p[3],  p[4],p[5],p[6],p[7]);
        p += 8;
        logd(" bit stream[ 8-15]: %x, %x, %x, %x,   %x, %x, %x, %x",
                p[0],p[1],p[2],p[3],  p[4],p[5],p[6],p[7]);
        p += 8;
        logd(" bit stream[16-23]: %x, %x, %x, %x,   %x, %x, %x, %x",
                p[0],p[1],p[2],p[3],  p[4],p[5],p[6],p[7]);
        p += 8;
        logd(" bit stream[24-31]: %x, %x, %x, %x,   %x, %x, %x, %x",
                p[0],p[1],p[2],p[3],  p[4],p[5],p[6],p[7]);
        p += 8;
        logd(" bit stream[32-39]: %x, %x, %x, %x,   %x, %x, %x, %x",
                p[0],p[1],p[2],p[3],  p[4],p[5],p[6],p[7]);
        p += 8;
        logd(" bit stream[40-47]: %x, %x, %x, %x,   %x, %x, %x, %x",
                p[0],p[1],p[2],p[3],  p[4],p[5],p[6],p[7]);
        p += 8;
        logd(" bit stream[48-55]: %x, %x, %x, %x,   %x, %x, %x, %x",
                p[0],p[1],p[2],p[3],  p[4],p[5],p[6],p[7]);
        logd("  ");
    }

    for(i=0;i<16;i++)
    {
        logd("%02x, 0x%08x 0x%08x 0x%08x 0x%08x \n",
                i, reg_base[4*i],reg_base[4*i+1], reg_base[4*i+2],reg_base[4*i+3]);
    }
    logd("  ");
    logd("  ");
    logd("  ");
}

void H264InitFuncCtrlRegister(H264DecCtx *h264DecCtx)
{
    H264Dec* h264Dec = NULL;
    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;

    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_FUNC_CTRL, (1<<10)|(1<<25));
}

s32 H264CheckBsDmaBusy(H264DecCtx *h264DecCtx)
{
    #define BS_DMA_BUSY (1<<22)
    #define CHECK_BUSY_TIME_OUT    (1000000)
    volatile u32 dwVal;
    H264Dec* h264Dec = NULL;
    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;
    int nWaitTime = 0;
    s32 ret = 0;

    if(h264Dec->nVeVersion != 0x3101000012010)
    {
        while(1)
        {
            nWaitTime++;
            vdec_readuint32(h264Dec->nRegisterBaseAddr+REG_FUNC_STATUS);
            if(!(dwVal & BS_DMA_BUSY))
            {
                break;
            }
            else if(nWaitTime > CHECK_BUSY_TIME_OUT)
            {
                ret = -1;
                break;
            }
        }
        return ret;
    }
    else
    {
        return 0;
    }
}

void H264EnableIntr(H264DecCtx *h264DecCtx)
{
    volatile u32 dwVal;
    H264Dec* h264Dec = NULL;
    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;

    vdec_readuint32(h264Dec->nRegisterBaseAddr+REG_FUNC_CTRL);
    dwVal |= 7;
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_FUNC_CTRL, dwVal);
}

u32 H264GetbitOffset(H264DecCtx* h264DecCtx, u8 offsetMode)
{
    H264Dec* h264Dec = NULL;
    volatile u32 dwVal = 0;

    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;
    if(offsetMode ==  H264_GET_VLD_OFFSET)
    {
        vdec_readuint32(h264Dec->nRegisterBaseAddr+REG_VLD_OFFSET);
    }
    else if(offsetMode ==  H264_GET_STCD_OFFSET)
    {
        vdec_readuint32(h264Dec->nRegisterBaseAddr+REG_STCD_OFFSET);
    }
    return dwVal;
}

/***************************************************************************/
/***************************************************************************/

void H264ConfigureBitstreamRegister(H264DecCtx *h264DecCtx, H264Context* hCtx, u32 nBitLens)
{
    H264Dec* h264Dec = NULL;
    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;
    H264CheckBsDmaBusy(h264DecCtx);

    #if 1  //added by xyliu for ve iommu
    if(hCtx->vbvInfo.pReadPtr < hCtx->vbvInfo.pVbvBuf)
    {
        hCtx->vbvInfo.pReadPtr = hCtx->vbvInfo.pVbvBuf;
        loge("*****error1:hCtx->vbvInfo.pReadPtr=%p,hCtx->vbvInfo.pVbvBuf=%p\n",
            hCtx->vbvInfo.pReadPtr,hCtx->vbvInfo.pVbvBuf);
    }
    if(hCtx->vbvInfo.pReadPtr > hCtx->vbvInfo.pVbvBufEnd)
    {
        hCtx->vbvInfo.pReadPtr = hCtx->vbvInfo.pVbvBufEnd;
        loge("*****error2:hCtx->vbvInfo.pReadPtr=%p,hCtx->vbvInfo.pVbvBufEnd=%p\n",
            hCtx->vbvInfo.pReadPtr,hCtx->vbvInfo.pVbvBufEnd);
    }
    #endif
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_VLD_OFFSET,
        (hCtx->vbvInfo.pReadPtr-hCtx->vbvInfo.pVbvBuf)*8);
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_VLD_BIT_LENGTH, nBitLens);

    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_VLD_END_ADDR,
        hCtx->vbvInfo.nVbvBufEndPhyAddr);
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_VLD_BITSTREAM_ADDR,
        (hCtx->vbvInfo.bVbvDataCtrlFlag<<28)|hCtx->vbvInfo.nVbvBufPhyAddr);
}


void H264ConfigureEptbDetect(H264DecCtx* h264DecCtx,
                            H264Context* hCtx,
                            u32 sliceDataBits,
                            u8 eptbDetectEnable)
{
    volatile u32 dwVal;
    H264Dec* h264Dec = NULL;

    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;

    H264CheckBsDmaBusy(h264DecCtx);

    if(eptbDetectEnable == 1)
    {
        vdec_readuint32(h264Dec->nRegisterBaseAddr+REG_FUNC_CTRL);
        dwVal &= ~EPTB_DETECTION_BY_PASS;
        vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_FUNC_CTRL, dwVal);
    }

    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_VLD_OFFSET,
        (hCtx->vbvInfo.pReadPtr-hCtx->vbvInfo.pVbvBuf)*8);
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_VLD_BIT_LENGTH,
        sliceDataBits);
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_VLD_END_ADDR,
        hCtx->vbvInfo.nVbvBufEndPhyAddr);
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_VLD_BITSTREAM_ADDR,
        (hCtx->vbvInfo.bVbvDataCtrlFlag<<28)|hCtx->vbvInfo.nVbvBufPhyAddr);
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_TRIGGER_TYPE, INIT_SWDEC);
}
/*****************************************************************************/
/*****************************************************************************/

void H264ConfigureReconMvcolBufRegister(H264Dec* h264Dec,
                                    H264PicInfo* pCurPicturePtr,
                                    u32 nYBufferOffset,
                                    u32 nCBufferOffset)
{
    volatile u32* pdwTemp;
    size_addr tmpaddr=0;

    //0xe0
    pdwTemp = (u32*)(&sram_port_rw_offset_rege0);
    *pdwTemp = 0;
    sram_port_rw_offset_rege0.sram_addr =
        H264_FRMBUF_INFO_ADDR+pCurPicturePtr->nDecodeBufIndex*32+12;
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_OFFSET,*pdwTemp);
    PRINT_REG("0xe0: %x\n", *pdwTemp);

    // configure luma buffer addr, chroma buffer addr, topmv_coladdr, botmv_coladdr
    pdwTemp = (u32*)(&sram_port_rw_data_rege4);
    *pdwTemp = 0;
    sram_port_rw_data_rege4.sram_data =
        (u32)pCurPicturePtr->pVPicture->phyYBufAddr+nYBufferOffset;
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_DATA,*pdwTemp);
    PRINT_REG("0xe4: %x\n", *pdwTemp);

    pdwTemp = (u32*)(&sram_port_rw_data_rege4);
    *pdwTemp = 0;
    sram_port_rw_data_rege4.sram_data = (u32)pCurPicturePtr->pVPicture->phyCBufAddr+nCBufferOffset;
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_DATA,*pdwTemp);
    PRINT_REG("0xe4: %x\n", *pdwTemp);

    pdwTemp = (u32*)(&sram_port_rw_data_rege4);
    *pdwTemp = 0;
    #if 0
    tmpaddr = (size_addr)CdcMemGetPhysicAddress(h264Dec->memops,
        (void*)pCurPicturePtr->pTopMvColBuf);
    #else
    tmpaddr = (size_addr)pCurPicturePtr->phyTopMvColBuf;
    #endif
    sram_port_rw_data_rege4.sram_data = (u32)(tmpaddr);

    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_DATA,*pdwTemp);
    PRINT_REG("0xe4:%x\n", *pdwTemp);

    pdwTemp = (u32*)(&sram_port_rw_data_rege4);
    *pdwTemp = 0;
    #if 0
    tmpaddr = (size_addr)CdcMemGetPhysicAddress(h264Dec->memops,
        (void*)pCurPicturePtr->pBottomMvColBuf);
    #else
     tmpaddr = (size_addr)pCurPicturePtr->phyBottomMvColBuf;
    #endif
    sram_port_rw_data_rege4.sram_data = (u32)(tmpaddr);
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_DATA,*pdwTemp);
    PRINT_REG("0xe4:%x\n", *pdwTemp);
}

void H264ConfigureFrameInfoRegister(H264Dec* h264Dec, H264PicInfo* pCurPicturePtr)
{
    volatile u32* pdwTemp;
    H264Context* hCtx = NULL;
    hCtx =     h264Dec->pHContext;

    if(hCtx->nPicStructure == PICT_FRAME)
    {
        //0xe0
        pdwTemp = (u32*)(&sram_port_rw_offset_rege0);
        *pdwTemp = 0;
        sram_port_rw_offset_rege0.sram_addr =
            H264_FRMBUF_INFO_ADDR+pCurPicturePtr->nDecodeBufIndex*32;
        vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_OFFSET,*pdwTemp);
        PRINT_REG("0xe0:%x\n", *pdwTemp);

        pdwTemp = (u32*)(&sram_port_rw_data_rege4);
        *pdwTemp = 0;
        sram_port_rw_data_rege4.sram_data = pCurPicturePtr->nFieldPoc[0];
        vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_DATA,*pdwTemp);
        PRINT_REG("0xe4:%x\n", *pdwTemp);

        pdwTemp = (u32*)(&sram_port_rw_data_rege4);
        *pdwTemp = 0;
        sram_port_rw_data_rege4.sram_data = pCurPicturePtr->nFieldPoc[1];
        vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_DATA,*pdwTemp);
        PRINT_REG("0xe4:%x\n", *pdwTemp);

        pdwTemp = (u32*)(&frame_struct_ref_info_rege4);
        *pdwTemp = 0;
        frame_struct_ref_info_rege4.frm_struct  = hCtx->bMbAffFrame? 2: 0;
        frame_struct_ref_info_rege4.top_ref_type = hCtx->nNalRefIdc ? 0: 2;
        frame_struct_ref_info_rege4.bot_ref_type = hCtx->nNalRefIdc ? 0: 2;
        vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_DATA,*pdwTemp);
        PRINT_REG("0xe4:%x\n", *pdwTemp);
    }
    else if(hCtx->nPicStructure == PICT_TOP_FIELD)
    {
        //0xe0
        pdwTemp = (u32*)(&sram_port_rw_offset_rege0);
        *pdwTemp = 0;
        sram_port_rw_offset_rege0.sram_addr =
            H264_FRMBUF_INFO_ADDR+pCurPicturePtr->nDecodeBufIndex*32;
        vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_OFFSET,*pdwTemp);
        PRINT_REG("0xe0:%x\n", *pdwTemp);

        pdwTemp = (u32*)(&sram_port_rw_data_rege4);
        *pdwTemp = 0;
        sram_port_rw_data_rege4.sram_data = pCurPicturePtr->nFieldPoc[0];
        vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_DATA,*pdwTemp);
        PRINT_REG("0xe4:%x\n", *pdwTemp);

        pdwTemp = (u32*)(&sram_port_rw_offset_rege0);
        *pdwTemp = 0;
        sram_port_rw_offset_rege0.sram_addr =
            H264_FRMBUF_INFO_ADDR+pCurPicturePtr->nDecodeBufIndex*32+8;
        vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_OFFSET,*pdwTemp);
        PRINT_REG("0xe0:%x\n", *pdwTemp);

        pdwTemp = (u32*)(&frame_struct_ref_info_rege4);
        *pdwTemp = 0;
        frame_struct_ref_info_rege4.frm_struct = 1;
        frame_struct_ref_info_rege4.top_ref_type = hCtx->nNalRefIdc ? 0: 2;
        vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_DATA,*pdwTemp);
        PRINT_REG("0xe4:%x\n", *pdwTemp);
    }
    else if(hCtx->nPicStructure == PICT_BOTTOM_FIELD)
    {
        //0xe0
        pdwTemp = (u32*)(&sram_port_rw_offset_rege0);
        *pdwTemp = 0;
        sram_port_rw_offset_rege0.sram_addr =
            H264_FRMBUF_INFO_ADDR+pCurPicturePtr->nDecodeBufIndex*32+4;
        vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_OFFSET,*pdwTemp);
        PRINT_REG("0xe0:%x\n", *pdwTemp);

        pdwTemp = (u32*)(&sram_port_rw_data_rege4);
        *pdwTemp = 0;
        sram_port_rw_data_rege4.sram_data = pCurPicturePtr->nFieldPoc[1];
        vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_DATA,*pdwTemp);
        PRINT_REG("0xe4:%x\n", *pdwTemp);

        pdwTemp = (u32*)(&frame_struct_ref_info_rege4);
        *pdwTemp = 0;
        frame_struct_ref_info_rege4.frm_struct = 1;
        frame_struct_ref_info_rege4.bot_ref_type = hCtx->nNalRefIdc ? 0: 2;;
        vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_DATA,*pdwTemp);
        PRINT_REG("0xe4:%x\n", *pdwTemp);
    }

    if((hCtx->bRefYv12DispYv12Flag==1) || (hCtx->bRefNv21DispNv21Flag==1))
    {
        vetop_reglist_t* vetop_reg_list;
        vetop_reg_list = (vetop_reglist_t*)CdcVeGetGroupRegAddr(h264Dec->pConfig->veOpsS,
                                                                h264Dec->pConfig->pVeOpsSelf,
                                                                REG_GROUP_VETOP);

        vetop_reg_list->_ec_pri_output_format.primary_output_format =
                (hCtx->bRefYv12DispYv12Flag==1)? 3: 5;
        //1:32*32 2:YUV 3:YVU Planner Format.
        vetop_reg_list->_c4_pri_chroma_buf_len.pri_chroma_buf_len    =
                hCtx->nRefCSize;
        vetop_reg_list->_c8_pri_buf_line_stride.pri_luma_line_stride =
                hCtx->nRefYLineStride;
        vetop_reg_list->_c8_pri_buf_line_stride.pri_chroma_line_stride =
                hCtx->nRefCLineStride;
    }

}

void H264CalCulateRefFrameRegisterValue(H264FrmRegisterInfo *pFrmRegisterInfo,
                                    H264PicInfo* pCurPicturePtr,
                                    u8 pPicStructure,
                                    u8 nReference)
{
    if(nReference == PICT_FRAME)
    {
        pFrmRegisterInfo->nFirstFieldPoc = pCurPicturePtr->nFieldPoc[0];
        pFrmRegisterInfo->nSecondFieldPoc = pCurPicturePtr->nFieldPoc[1];
        pFrmRegisterInfo->nFrameStruct = 1;

        if(pPicStructure == PICT_FRAME)
        {
            pFrmRegisterInfo->nFrameStruct  = pCurPicturePtr->bMbAffFrame? 2: 0;
            //hCtx->bMbAffFrame? 2: 0;
        }
        pFrmRegisterInfo->nTopRefType = pCurPicturePtr->nFieldNalRefIdc[0]? 0: 2;
        //hCtx->nNalRefIdc ? 0: 2;
        pFrmRegisterInfo->nBottomRefType = pCurPicturePtr->nFieldNalRefIdc[1]? 0: 2;
        //hCtx->nNalRefIdc ? 0: 2;
    }
    else if(nReference== PICT_TOP_FIELD)
    {
        pFrmRegisterInfo->nFirstFieldPoc = pCurPicturePtr->nFieldPoc[0];
        pFrmRegisterInfo->nFrameStruct = 1;
        pFrmRegisterInfo->nTopRefType = pCurPicturePtr->nFieldNalRefIdc[0]? 0: 2;
        //hCtx->nNalRefIdc ? 0: 2;
    }
    else if(nReference == PICT_BOTTOM_FIELD)
    {
        pFrmRegisterInfo->nSecondFieldPoc = pCurPicturePtr->nFieldPoc[1];
        pFrmRegisterInfo->nFrameStruct = 1;
        pFrmRegisterInfo->nBottomRefType = pCurPicturePtr->nFieldNalRefIdc[1]? 0: 2;
        //hCtx->nNalRefIdc ? 0: 2;
    }
    pFrmRegisterInfo->nFrameStruct = pCurPicturePtr->bCodedFrame ?
        (pCurPicturePtr->bMbAffFrame? 2: 0) : 1; //hCtx->bMbAffFrame? 2: 0;
}

void H264ConfigureRefFrameRegister(H264Dec* h264Dec, H264FrmRegisterInfo *pFrmRegisterInfo)
{
    volatile u32* pdwTemp;
    //0xe0
    pdwTemp = (u32*)(&sram_port_rw_offset_rege0);
    *pdwTemp = 0;
    sram_port_rw_offset_rege0.sram_addr =
        H264_FRMBUF_INFO_ADDR+ pFrmRegisterInfo->nFrameBufIndex*32;
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_OFFSET,*pdwTemp);

    PRINT_REG("0xe0:%x\n", *pdwTemp);

    pdwTemp = (u32*)(&sram_port_rw_data_rege4);
    *pdwTemp = 0;
    sram_port_rw_data_rege4.sram_data = pFrmRegisterInfo->nFirstFieldPoc;
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_DATA,*pdwTemp);
    PRINT_REG("0xe4:%x\n", *pdwTemp);

    pdwTemp = (u32*)(&sram_port_rw_data_rege4);
    *pdwTemp = 0;
    sram_port_rw_data_rege4.sram_data = pFrmRegisterInfo->nSecondFieldPoc;
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_DATA,*pdwTemp);
    PRINT_REG("0xe4:%x\n", *pdwTemp);

    pdwTemp = (u32*)(&frame_struct_ref_info_rege4);
    *pdwTemp = 0;
    frame_struct_ref_info_rege4.frm_struct  = pFrmRegisterInfo->nFrameStruct;
    frame_struct_ref_info_rege4.top_ref_type = pFrmRegisterInfo->nTopRefType;
    frame_struct_ref_info_rege4.bot_ref_type = pFrmRegisterInfo->nBottomRefType;
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_DATA,*pdwTemp);
    PRINT_REG("0xe4:%x\n", *pdwTemp);
}

void H264ConfigureRefListRegister(H264Dec* h264Dec, H264Context* hCtx)
{
    u32 value = 0;
    u32 i= 0;
    s32 j = 0;
    s32 k = 0;
    s32 n = 0;
    s32 refPicNum[18]={0};
    H264PicInfo* pCurPicturePtr = NULL;
    H264FrmRegisterInfo pFrmRegisterInfo[32];
    u8 validFrameBufIndex[32];
    //u8 validFrameBufNum = 0;
    //u8 bRefFrmErrorFlag = 0;

    memset(pFrmRegisterInfo, 0, 32*sizeof(H264FrmRegisterInfo));
    //logv("hCtx->nListCount=%d, nRefCount[0]=%d, nRefCount[1]=%d\n",
    //hCtx->nListCount, hCtx->nRefCount[0], hCtx->nRefCount[1]);

    for(i=0; i<hCtx->nListCount; i++)
    {
        if(i>=2)
        {
            continue;
        }

        j = 0;
        for(; j<hCtx->nRefCount[i]; j++)
        {
            if(j>=48)
            {
                continue;
            }
            if(hCtx->frmBufInf.refList[i][j].pVPicture != NULL)
            {
                //bRefFrmErrorFlag += hCtx->frmBufInf.refList[i][j].bDecErrorFlag;
                k = hCtx->frmBufInf.refList[i][j].nDecodeBufIndex;
                pCurPicturePtr = &hCtx->frmBufInf.refList[i][j];
                pFrmRegisterInfo[k].nFrameBufIndex = k;

                H264CalCulateRefFrameRegisterValue(&pFrmRegisterInfo[k],pCurPicturePtr,
                               hCtx->frmBufInf.refList[i][j].nPicStructure,
                               hCtx->frmBufInf.refList[i][j].nReference);

                validFrameBufIndex[n] = k;
                if(pFrmRegisterInfo[k].bBufUsedFlag == 0)
                {
                    n++;
                }
                pFrmRegisterInfo[k].bBufUsedFlag = 1;
            }
        }
    }

    //hCtx->frmBufInf.pCurPicturePtr->bDecErrorFlag =
    //(bRefFrmErrorFlag>0)? 1: hCtx->frmBufInf.pCurPicturePtr->bDecErrorFlag;

    for(j=0; j<n; j++)
    {
        k = validFrameBufIndex[j];
        H264ConfigureRefFrameRegister(h264Dec, &pFrmRegisterInfo[k]);
    }

    for(i=0; i<hCtx->nListCount; i++)
    {
        if(i>=2)
        {
            continue;
        }
        memset(refPicNum, 0, 18*sizeof(u32));
        j = hCtx->nRefCount[i]-1;

        for(;j>=0; j--)
        {
            if(j>=48)
            {
                continue;
            }
            if(hCtx->frmBufInf.refList[i][j].pVPicture != NULL)
            {
                k = hCtx->frmBufInf.refList[i][j].nDecodeBufIndex;
                value = (k<<1)|(hCtx->frmBufInf.refList[i][j].nPicStructure==PICT_BOTTOM_FIELD);
                refPicNum[j/4] = (refPicNum[j/4]<<8) |value;
            }
        }

        if(hCtx->nRefCount[i]>0)
        {
            volatile u32* pdwTemp;
            //0xe0
            pdwTemp = (u32*)(&sram_port_rw_offset_rege0);
            *pdwTemp = 0;
            vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_OFFSET,
                H264_REFLIST_ADDR+36*i);
            PRINT_REG("0xe0:%x", H264_REFLIST_ADDR+36*i);

            for(k=0; k<=(hCtx->nRefCount[i]/4); k++)
            {
                pdwTemp = (u32*)(&sram_port_rw_data_rege4);
                *pdwTemp = 0;
                vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_DATA,
                    refPicNum[k]);
                PRINT_REG("0xe4:%x", refPicNum[k]);
            }
        }
    }
#if H264_DEBUG_PRINT_REF_LIST
    if(hCtx->nSliceType != H264_I_TYPE)
    {
        s32 i;
        s32 nCurPoc;
        if(hCtx->nPicStructure != PICT_FRAME)
        {
            s32 index = hCtx->nPicStructure==PICT_BOTTOM_FIELD;
            nCurPoc = hCtx->frmBufInf.pCurPicturePtr->nFieldPoc[index];
        }
        else
        {
            nCurPoc = hCtx->frmBufInf.pCurPicturePtr->nPoc;
        }
        logd("-------poc: %d, %s ref list 0 info ------", nCurPoc,
                H264_DEBUG_STRUCT(hCtx->nPicStructure));
        for(i=0; i<hCtx->nRefCount[0]; i++)
        {
            if(hCtx->frmBufInf.refList[0][i].pVPicture != NULL)
            {
                logd("     %d, poc: %d, frame num: %d, nPicStructure: %s",
                        i, hCtx->frmBufInf.refList[0][i].nPoc,
                        hCtx->frmBufInf.refList[0][i].nFrmNum,
                        H264_DEBUG_STRUCT(hCtx->frmBufInf.refList[0][i].nPicStructure));
            }
        }
        if(hCtx->nSliceType == H264_B_TYPE)
        {
            logd("-------poc: %d, %s ref list 1 info ------", nCurPoc,
                    (hCtx->nPicStructure == 0) ? "FRAME" :
                        ((hCtx->nPicStructure == 1) ? "TOP" : "BOTTOM"));
            for(i=0; i<hCtx->nRefCount[1]; i++)
            {
                if(hCtx->frmBufInf.refList[1][i].pVPicture != NULL)
                {
                    logd("     %d, poc: %d, frame num: %d, nPicStructure: %s",
                            i, hCtx->frmBufInf.refList[1][i].nPoc,
                            hCtx->frmBufInf.refList[1][i].nFrmNum,
                            H264_DEBUG_STRUCT(hCtx->frmBufInf.refList[1][i].nPicStructure));
                }
            }
            logd(" ");
        }
        logd("     -------------- reference list end ------------------");
        logd(" ");
    }
#endif
}

void H264ConvertScalingMatrices( H264Context* hCtx,
                            u8 (*nScalingMatrix4)[16], u8 (*nScalingMatrix8)[64])
{
    s32 i = 0;
    s32 j = 0;

    for(i=0; i<6; i++)
    {
        for(j=0; j<4; j++)
        {
            hCtx->nScalingMatrix4[i][j] = (nScalingMatrix4[i][4*j+3]<<24) |
                                            (nScalingMatrix4[i][4*j+2]<<16) |
                                            (nScalingMatrix4[i][4*j+1]<<8)|
                                            (nScalingMatrix4[i][4*j+0]);
        }
    }

    for(i=0; i<2; i++)
    {
        for(j=0; j<16; j++)
        {
            hCtx->nScalingMatrix8[i][j] = (nScalingMatrix8[i][4*j+3]<<24) |
                                            (nScalingMatrix8[i][4*j+2]<<16) |
                                            (nScalingMatrix8[i][4*j+1]<<8)|
                                            (nScalingMatrix8[i][4*j+0]);
        }
    }
}

void H264ConfigureQuantMatrixRegister(H264DecCtx *h264DecCtx, H264Context* hCtx)
{
    s32 i = 0;
    s32 j = 0;
    H264Dec* h264Dec = NULL;
    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;

    if(hCtx->bScalingMatrixPresent == 1)
    {
        if(hCtx->pCurPps->bScalingMatrixPresent == 1)
        {
            H264ConvertScalingMatrices(hCtx,
                hCtx->pCurPps->nScalingMatrix4,
                hCtx->pCurPps->nScalingMatrix8);
        }
        else if(hCtx->pCurSps->bScalingMatrixPresent == 1)
        {
            H264ConvertScalingMatrices(hCtx,
                hCtx->pCurSps->nScalingMatrix4,
                hCtx->pCurSps->nScalingMatrix8);
        }
        hCtx->bScalingMatrixPresent = 0;
    }

    for(i=0; i<=1; i++)
    {
        vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_OFFSET,
            H264_QUANT_MATRIX_ADDR+i*64);
        PRINT_REG("0xe0: %x\n", H264_QUANT_MATRIX_ADDR+i*64);

        for(j=0; j<16; j++)
        {
            vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_DATA,
                hCtx->nScalingMatrix8[i][j]);
            PRINT_REG("0xe4: %x\n", hCtx->nScalingMatrix8[i][j]);
        }
    }

    for(i=0; i<6; i++)
    {
        vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_OFFSET,
            H264_QUANT_MATRIX_ADDR+0x80+i*16);
        PRINT_REG("0xe0: %x\n", H264_QUANT_MATRIX_ADDR+0x80+i*16);

        vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_DATA,
            hCtx->nScalingMatrix4[i][0]);
        vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_DATA,
            hCtx->nScalingMatrix4[i][1]);
        vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_DATA,
            hCtx->nScalingMatrix4[i][2]);
        vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_DATA,
            hCtx->nScalingMatrix4[i][3]);
        PRINT_REG("0xe4: %x\n", hCtx->nScalingMatrix4[i][0]);
        PRINT_REG("0xe4: %x\n", hCtx->nScalingMatrix4[i][1]);
        PRINT_REG("0xe4: %x\n", hCtx->nScalingMatrix4[i][2]);
        PRINT_REG("0xe4: %x\n", hCtx->nScalingMatrix4[i][3]);
    }
    return;
}

void H264ConfigureScaleRotateRegister(H264DecCtx *h264DecCtx,H264Dec* h264Dec, H264Context* hCtx)
{
    volatile u32* pdwTemp;
    volatile u32 dwVal;
    vetop_reglist_t* vetop_reg_list;

    //congigure 0x40 register
    pdwTemp = (u32*)(&sd_rotate_ctrl_reg40);
    *pdwTemp = 0;
    sd_rotate_ctrl_reg40.rot_angle = h264DecCtx->vconfig.nRotateDegree;
#if 0
    if((h264DecCtx->vconfig.nRotateDegree==1)|| (h264DecCtx->vconfig.nRotateDegree==3))
    {
        sd_rotate_ctrl_reg40.scale_precision |= (h264DecCtx->vconfig.nHorizonScaleDownRatio<<2);
        sd_rotate_ctrl_reg40.scale_precision |= (h264DecCtx->vconfig.nVerticalScaleDownRatio);
    }
    else
#endif
    {
        sd_rotate_ctrl_reg40.scale_precision |= (h264DecCtx->vconfig.nVerticalScaleDownRatio<<2);
        sd_rotate_ctrl_reg40.scale_precision |= (h264DecCtx->vconfig.nHorizonScaleDownRatio);
    }
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SD_ROTATE_CTRL,*pdwTemp);

    //congigure 0x44 register
    pdwTemp = (u32*)(&sd_rotate_buf_addr_reg44);
    *pdwTemp = 0;
    sd_rotate_buf_addr_reg44.sd_rotate_luma_buf_addr =
        (u32)hCtx->frmBufInf.pCurPicturePtr->pScaleDownVPicture->phyYBufAddr+
        FbmGetBufferOffset(hCtx->pFbmScaledown, 1);
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SD_ROTATE_LUMA_BUF_ADDR,*pdwTemp);

    //congigure 0x48 register
    pdwTemp = (u32*)(&sd_rotate_chroma_buf_addr_reg48);
    *pdwTemp = 0;
    sd_rotate_chroma_buf_addr_reg48.sd_rotate_chroma_buf_addr =
        (u32)hCtx->frmBufInf.pCurPicturePtr->pScaleDownVPicture->phyCBufAddr+
        FbmGetBufferOffset(hCtx->pFbmScaledown, 0);
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SD_ROTATE_CHROMA_BUF_ADDR,*pdwTemp);

    vdec_readuint32(h264Dec->nRegisterBaseAddr+REG_FUNC_CTRL);
    dwVal |= WRITE_SCALE_ROTATED_PIC;
    if(hCtx->nNalRefIdc > 0)
    {
        dwVal &= ~NOT_WRITE_RECONS_PIC;
    }
    else
    {
        dwVal |= NOT_WRITE_RECONS_PIC;
    }
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_FUNC_CTRL, dwVal);

    vetop_reg_list = (vetop_reglist_t*)CdcVeGetGroupRegAddr(h264DecCtx->vconfig.veOpsS,
                                                            h264DecCtx->vconfig.pVeOpsSelf,
                                                            REG_GROUP_VETOP);
    vetop_reg_list->_e8_chroma_buf_len0.output_data_format = 0;

    if(hCtx->bYv12OutFlag == 1)
    {
        vetop_reg_list->_e8_chroma_buf_len0.output_data_format = 3;
        //1:32*32 2:YUV 3:YVU Planner Format.
        vetop_reg_list->_e8_chroma_buf_len0.chroma_align_mode = 1;
        //default.0->16 bytes;1->8 bytes.
        vetop_reg_list->_e8_chroma_buf_len0.sdrt_chroma_buf_len = hCtx->nDispCSize;
    }
    else if(hCtx->bNv21OutFlag == 1)
    {
        vetop_reg_list->_e8_chroma_buf_len0.output_data_format = 1;
        vetop_reg_list->_ec_pri_output_format.primary_output_format = 0;
        //default 0:32*32 tile
        vetop_reg_list->_ec_pri_output_format.second_special_output_format = 5;
        //0x100:NV12 planner format.0x101:NU12
    }
    else if(hCtx->bRefYv12DispYv12Flag==1 || hCtx->bRefNv21DispNv21Flag==1)
    {
        vetop_reg_list->_ec_pri_output_format.primary_output_format =
            (hCtx->bRefYv12DispYv12Flag==1)? 3 : 5;
        //1:32*32 2:YUV 3:YVU Planner Format.
        vetop_reg_list->_c4_pri_chroma_buf_len.pri_chroma_buf_len         =
            hCtx->nRefCSize;
        vetop_reg_list->_c8_pri_buf_line_stride.pri_luma_line_stride      =
            hCtx->nRefYLineStride;
        vetop_reg_list->_c8_pri_buf_line_stride.pri_chroma_line_stride   =
            hCtx->nRefCLineStride;
        vetop_reg_list->_cc_sec_buf_line_stride.sec_luma_line_stride     =
            hCtx->nDispYLineStride;
        vetop_reg_list->_cc_sec_buf_line_stride.sec_chroma_line_stride   =
            hCtx->nDispCLineStride;
        vetop_reg_list->_e8_chroma_buf_len0.output_data_format =
            (hCtx->bRefYv12DispYv12Flag==1)? 3:1;//1:32*32 2:YUV 3:YVU Planner Format.
        vetop_reg_list->_e8_chroma_buf_len0.sdrt_chroma_buf_len         =
            hCtx->nDispCSize;
        vetop_reg_list->_ec_pri_output_format.second_special_output_format =
            (hCtx->bRefYv12DispYv12Flag==1)? 3:5;  //1:32*32 2:YUV 3:YVU Plan
    }
}

#if 0
void H264ConfigMafRegisters(H264Dec* h264Dec, H264Context* hCtx)
{
    u32 tmp = 0;
    u32 val = 0;
    size_addr tmpaddr = 0;
    vetop_reglist_t* vetop_reg_list;
    vetop_reg_list = (vetop_reglist_t*)CdcVeGetGroupRegAddr(h264Dec->pConfig->veOpsS,
                                                            h264Dec->pConfig->pVeOpsSelf,
                                                            REG_GROUP_VETOP);

    tmp = 0;
    val = (hCtx->bFstField==1 || hCtx->nPicStructure == PICT_FRAME);
    tmp |= (val << 31);
    val = hCtx->bUseMafFlag;
    tmp |= (val << 30);
    tmp |= 0x5622;
    //average_shifter_c=0x05; average_shifter_y=0x06;
    //motion_to_still_threshold=0x02;still_to_motion_threshold=0x02
    SetRegValue(vetop_reg_list->_30_maf_ctrl, tmp);

    tmp = 0x190f1009;
    //max_clip_value_c = 0x19;min_clip_value_c = 0x0f;
    //max_clip_value_y = 0x10;min_clip_value_y = 0x09
    SetRegValue(vetop_reg_list->_34_maf_th_clip, tmp);
    hCtx->frmBufInf.pCurPicturePtr->pVPicture->bMafValid = hCtx->bUseMafFlag;
    hCtx->frmBufInf.pCurPicturePtr->pVPicture->nMafFlagStride = 0x40;
    hCtx->frmBufInf.pCurPicturePtr->pVPicture->bPreFrmValid = 1;
    hCtx->frmBufInf.pCurPicturePtr->pVPicture->pMafData =
        hCtx->frmBufInf.pCurPicturePtr->pVPicture->pData3;

    if(hCtx->frmBufInf.nRef2MafBufAddr== 0)
    {
         memcpy(hCtx->frmBufInf.pCurPicturePtr->pVPicture->pData3,
            hCtx->pMafInitBuffer, hCtx->nMafBufferSize);
         CdcMemFlushCache(h264Dec->memops,
            hCtx->frmBufInf.pCurPicturePtr->pVPicture->pData3, hCtx->nMafBufferSize);
         hCtx->frmBufInf.pCurPicturePtr->pVPicture->bPreFrmValid = 0;
         if(hCtx->bFstField==0 || hCtx->nPicStructure == PICT_FRAME)
          {
             tmpaddr = (size_addr)CdcMemGetPhysicAddress(h264Dec->memops,
                (void*)hCtx->pMafInitBuffer)&0x7ffffffffLL;
             hCtx->frmBufInf.nRef1MafBufAddr = (u32)(tmpaddr & 0xffffffff);
          }
    }
    else
    {
        SetRegValue(vetop_reg_list->_38_maf_ref1_luma_addr,
            hCtx->frmBufInf.nRef1YBufAddr);

        SetRegValue(vetop_reg_list->_3c_maf_ref1_chroma_addr,
            hCtx->frmBufInf.nRef1CBufAddr);

        SetRegValue(vetop_reg_list->_44_ref1_maf_addr,
            hCtx->frmBufInf.nRef1MafBufAddr);
        SetRegValue(vetop_reg_list->_48_ref2_maf_addr,
            hCtx->frmBufInf.nRef2MafBufAddr);
        tmpaddr = (size_addr)CdcMemGetPhysicAddress(h264Dec->memops,
            (void*)hCtx->frmBufInf.pCurPicturePtr->pVPicture->pData3)&0x7ffffffffLL;
        tmp = (u32)(tmpaddr & 0xffffffff);
        SetRegValue(vetop_reg_list->_40_cur_maf_addr, tmp);
    }

    if(hCtx->bFstField==0 || hCtx->nPicStructure == PICT_FRAME)
    {
        hCtx->frmBufInf.nRef2MafBufAddr = hCtx->frmBufInf.nRef1MafBufAddr;
        tmpaddr = (size_addr)CdcMemGetPhysicAddress(h264Dec->memops,
            (void*)hCtx->frmBufInf.pCurPicturePtr->pVPicture->pData3)&0x7ffffffffLL;;
        hCtx->frmBufInf.nRef1MafBufAddr = (u32)(tmpaddr & 0xffffffff);
        tmpaddr = (size_addr)CdcMemGetPhysicAddress(h264Dec->memops,
            (void*)hCtx->frmBufInf.pCurPicturePtr->pVPicture->pData0) & 0x7ffffffffLL;
        hCtx->frmBufInf.nRef1YBufAddr   = (u32)(tmpaddr & 0xffffffff);
        tmpaddr = (size_addr)CdcMemGetPhysicAddress(h264Dec->memops,
            (void*)hCtx->frmBufInf.pCurPicturePtr->pVPicture->pData1) & 0x7ffffffffLL;
        hCtx->frmBufInf.nRef1CBufAddr   = (u32)(tmpaddr & 0xffffffff);
    }
    return;
 }
#endif

void H264ConfigTopLeveLRegisters(H264Dec* h264Dec, H264Context* hCtx)
{
    size_addr tmpaddr = 0;

    if(hCtx->bUseDramBufFlag == 1)
    {

        vetop_reglist_t* vetop_reg_list
            = (vetop_reglist_t*)CdcVeGetGroupRegAddr(h264Dec->pConfig->veOpsS,
                                                     h264Dec->pConfig->pVeOpsSelf,
                                                     REG_GROUP_VETOP);

        vetop_reg_list->_50_deblk_intrapred_buf_ctrl.deblk_buf_ctrl = 1;
        vetop_reg_list->_50_deblk_intrapred_buf_ctrl.intrapred_buf_ctrl = 1;

        #if 0
        tmpaddr = (size_addr)CdcMemGetPhysicAddress(h264Dec->memops,
            (void*)hCtx->pDeblkDramBuf);
        #else
        tmpaddr = (size_addr)hCtx->phyDeblkDramBuf;
        #endif
        vetop_reg_list->_54_deblk_dram_buf.addr  = (u32)(tmpaddr);
        #if 0
        tmpaddr = (size_addr)CdcMemGetPhysicAddress(h264Dec->memops,
            (void*)hCtx->pIntraPredDramBuf);
        #else
        tmpaddr = (size_addr)hCtx->phyIntraPredDramBuf;
        #endif
        vetop_reg_list->_58_intrapred_dram_buf.addr  = (u32)(tmpaddr);
    }
}


void H264ConfigVbvRegisters(H264DecCtx * h264DecCtx, H264Context* hCtx, u32 sliceDataLen)
{
    u32 nBitCounts = 0;
    H264Dec* h264Dec = NULL;
    u32 i = 0;
    volatile u32 dwVal;
    int64_t offset = 0;
    int64_t bufBitSize = 0;

    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;
#if 1
    if(h264DecCtx->bSoftDecHeader == 1)
    {
        nBitCounts = H264GetBitCountSw((void*)h264DecCtx);

        if(h264Dec->nEmulateNum > 0)
        {
           for(i=0; i<=h264Dec->nEmulateNum; i++)
           {
               if(h264Dec->emulateIndex[i] >= nBitCounts/8)
               {
                   break;
               }
           }
       }
       nBitCounts += i*8;

       sliceDataLen -= nBitCounts;

       offset = (hCtx->vbvInfo.pReadPtr-hCtx->vbvInfo.pVbvBuf)*8;

       offset += nBitCounts;

       bufBitSize =  (hCtx->vbvInfo.pVbvBufEnd+1-hCtx->vbvInfo.pVbvBuf)*8;
       if(offset > bufBitSize)
       {
            offset -= bufBitSize;
       }

       vdec_readuint32(h264Dec->nRegisterBaseAddr+REG_FUNC_CTRL);
       dwVal &= ~STARTCODE_DETECT_E;
       dwVal &= ~EPTB_DETECTION_BY_PASS;
       vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_FUNC_CTRL, dwVal);

       vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_VLD_OFFSET,offset);

       vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_VLD_BIT_LENGTH, sliceDataLen);

       vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_VLD_END_ADDR,
               hCtx->vbvInfo.nVbvBufEndPhyAddr);
       vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_VLD_BITSTREAM_ADDR,
               (hCtx->vbvInfo.bVbvDataCtrlFlag<<28)|hCtx->vbvInfo.nVbvBufPhyAddr);
       vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_TRIGGER_TYPE, INIT_SWDEC);

    }
#else
    if(h264DecCtx->bSoftDecHeader == 1)
    {
        nBitCounts = H264GetBitCountSw((void*)h264DecCtx);
        //nBitCounts = (nBitCounts+7) &~ 7;

        logd("*****nBitCounts=%d\n", nBitCounts);

        if(hCtx->bIsAvc)
            H264ConfigureAvcRegister(h264DecCtx, hCtx, 1, sliceDataLen);
        else
            H264ConfigureEptbDetect(h264DecCtx, hCtx, sliceDataLen, 1);
        i = 0;
    #if 0
        if(h264Dec->nEmulateNum > 0)
        {
           logd("here1:nBitCounts=%d, h264Dec->nEmulateNum=%d\n", nBitCounts, h264Dec->nEmulateNum);

            for(i=0; i<=h264Dec->nEmulateNum; i++)
            {
                if(h264Dec->emulateIndex[i] >= nBitCounts/8)
                {
                    break;
                }
            }
        }
    #endif

        nBitCounts += i*8;
        logd("here2:nBitCounts=%d\n", nBitCounts);

        for(i=0; i<nBitCounts/8; i++)
        {
             logd("*************start showBits32=%x\n", H264ShowBits(h264DecCtx,32));
             H264GetBits(h264DecCtx, 8);
        }
        H264GetBits(h264DecCtx, nBitCounts-8*(nBitCounts/8));
        //H264GetBits(h264DecCtx, 8);

    }
#endif
}


void H264ConfigureSliceRegister(H264DecCtx *h264DecCtx, H264Context* hCtx,
                                         u8 decStreamIndex, u32 sliceDataLen)
{
    s32 i = 0;
    s32 j = 0;
    s32 maxFrmNum = 0;

    volatile u32* pdwTemp;
    volatile u32 dwVal;
    size_addr tmpaddr = 0;
    u32 nYBufferOffset = 0;
    u32 nCBufferOffset = 0;
    H264Dec* h264Dec = NULL;
    H264PicInfo* pCurPicturePtr = NULL;

    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;

    //logd("showBits=%x\n", H264ShowBits(h264DecCtx, 32));

    H264ConfigTopLeveLRegisters(h264Dec, hCtx);

    //congigure 0x00 register
    pdwTemp = (u32*)(&avc_sps_reg00);
    *pdwTemp = 0;
    avc_sps_reg00.pic_height_mb_minus1 = hCtx->pCurSps->nMbHeight-1;
    avc_sps_reg00.pic_width_mb_minus1 = hCtx->pCurSps->nMbWidth-1;
    avc_sps_reg00.direct_8x8_inference_flag = hCtx->pCurSps->bDirect8x8InferenceFlag;
    avc_sps_reg00.mb_adaptive_frame_field_flag = hCtx->pCurSps->bMbAff;
    //mb_adaptive_frame_field_flag
    avc_sps_reg00.frame_mbs_only_flag = hCtx->pCurSps->bFrameMbsOnlyFlag;
    avc_sps_reg00.chroma_format_idc = 1;
    //000:monochroma 001:4;2;0 010:4:2:2 011:4:4:4 others:reserved
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SPS,*pdwTemp);
    PRINT_REG("0x00: %x\n", *pdwTemp);

    //congigure 0x04 register
    pdwTemp = (u32*)(&avc_pps_reg04);
    *pdwTemp = 0;
    avc_pps_reg04.transform_8x8_mode_flag = hCtx->pCurPps->bTransform8x8Mode;
    avc_pps_reg04.constrained_intra_pred_flag = hCtx->pCurPps->bConstrainedIntraPred;
    avc_pps_reg04.weighted_bipred_idc = hCtx->pCurPps->nWeightedBIpredIdc;
    avc_pps_reg04.weighted_pred_idc = hCtx->pCurPps->bWeightedPred;
    avc_pps_reg04.num_ref_idx_l1_active_minus1_pic = hCtx->pCurPps->nRefCount[1]-1;
    avc_pps_reg04.num_ref_idx_l0_active_minus1_pic = hCtx->pCurPps->nRefCount[0]-1;
    avc_pps_reg04.entropy_coding_mode_flag = hCtx->pCurPps->bCabac;
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_PPS,*pdwTemp);
    PRINT_REG("0x04: %x\n", *pdwTemp);

    //congigure 0x08 register
    pdwTemp = (u32*)(&shs_reg08);
    *pdwTemp = 0;
    shs_reg08.bCabac_init_idc = hCtx->nCabacInitIdc;
    shs_reg08.direct_spatial_mv_pred_flag = hCtx->bDirectSpatialMvPred;
    shs_reg08.bottom_field_flag = (hCtx->nPicStructure==PICT_BOTTOM_FIELD);
    shs_reg08.field_pic_flag = (hCtx->nPicStructure!=PICT_FRAME);
    shs_reg08.first_slice_in_pic = (hCtx->nCurSliceNum==1)? 1: 0;
    shs_reg08.slice_type = hCtx->nLastSliceType;
    shs_reg08.nal_ref_flag =  hCtx->nNalRefIdc;
    shs_reg08.first_mb_y = hCtx->nMbY;
    shs_reg08.first_mb_x = hCtx->nMbX;
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SHS,*pdwTemp);
    PRINT_REG("0x08: %x\n", *pdwTemp);

    //congigure 0x0c register
    pdwTemp = (u32*)(&shs2_reg0c);
    *pdwTemp = 0;
    shs2_reg0c.slice_beta_offset_div2 = hCtx->nSliceBetaoffset;
    shs2_reg0c.slice_alpha_c0_offset_div2 = hCtx->nSliceAlphaC0offset;
    shs2_reg0c.disable_deblocking_filter_idc = hCtx->bDisableDeblockingFilter;
    shs2_reg0c.num_ref_idx_active_override_flag = hCtx->bNumRefIdxActiveOverrideFlag;
    if(hCtx->nSliceType==H264_B_TYPE)
    {
        shs2_reg0c.num_ref_idx_l1_active_minus1_slice  = hCtx->nRefCount[1]-1;
    }
    shs2_reg0c.num_ref_idx_l0_active_minus1_slice  =  hCtx->nRefCount[0]-1;
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SHS2,*pdwTemp);
    PRINT_REG("0x0c: %x\n", *pdwTemp);

    //configure 0x1c register
    pdwTemp = (u32*)(&shs_qp_reg1c);
    *pdwTemp = 0;
    shs_qp_reg1c.slice_qpy      = hCtx->nQscale;
    shs_qp_reg1c.chroma_qp_index_offset = hCtx->pCurPps->nChromaQpIndexOffset[0];
    shs_qp_reg1c.second_chroma_qp_index_offset =  hCtx->pCurPps->nChromaQpIndexOffset[1];
    shs_qp_reg1c.scaling_matix_flat_flag = 1;

    //logd("hCtx->pCurSps->nProfileIdc=%d,
    //hCtx->pCurSps->bScalingMatrixPresent=%d,
    //hCtx->pCurPps->bScalingMatrixPresent=%d\n",
    //hCtx->pCurSps->nProfileIdc,
    //hCtx->pCurSps->bScalingMatrixPresent, hCtx->pCurPps->bScalingMatrixPresent);

    if(hCtx->pCurSps->nProfileIdc==100 ||
        hCtx->pCurSps->nProfileIdc==118 ||
        hCtx->pCurSps->nProfileIdc==128)
    {
        if(hCtx->pCurSps->bScalingMatrixPresent==1 ||
            hCtx->pCurPps->bScalingMatrixPresent==1)
        {
            H264ConfigureQuantMatrixRegister(h264DecCtx, hCtx);
            shs_qp_reg1c.scaling_matix_flat_flag = 0;
        }
    }
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SHS_QP,*pdwTemp);
    PRINT_REG("0x1c: %x\n", *pdwTemp);

    // configure 0x50 register
    pdwTemp = (u32*)(&mb_field_intra_info_addr_reg50);
    *pdwTemp = 0;
    #if 0
    tmpaddr = (size_addr)CdcMemGetPhysicAddress(h264Dec->memops,
        (void*)hCtx->pMbFieldIntraBuf);
    #else
    tmpaddr = (size_addr)hCtx->phyMbFieldIntraBuf;
    #endif
    mb_field_intra_info_addr_reg50.mb_field_intra_info_addr = (u32)(tmpaddr);
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_MB_FIELD_INTRA_INFO_ADDR,*pdwTemp);
    PRINT_REG("0x50: %x\n", *pdwTemp);

    // configure 0x54 register
    pdwTemp = (u32*)(&mb_neighbor_info_addr_reg54);
    *pdwTemp = 0;

    tmpaddr = hCtx->uMbNeighBorInfo16KAlignBufPhy;
    mb_neighbor_info_addr_reg54.mb_neighbor_info_addr = (u32)(tmpaddr);
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_MB_NEIGHBOR_INFO_ADDR,*pdwTemp);
    PRINT_REG("0x54: %x\n", *pdwTemp);

    if(hCtx->nCurSliceNum > 1)
    {
        goto start_decode_slice;
    }

    //configure 0x2c register
    pdwTemp = (u32*)(&cur_mb_num_reg2c);
    *pdwTemp = 0;
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_CUR_MB_NUM,*pdwTemp);
    PRINT_REG("0x2c: %x\n", *pdwTemp);

    // configure 0x60 register
    pdwTemp = (u32*)(&mb_addr_reg60);
    *pdwTemp = 0;
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_MB_ADDR,*pdwTemp);
    PRINT_REG("0x60: %x\n", *pdwTemp);

    pCurPicturePtr = hCtx->frmBufInf.pCurPicturePtr;
    //0x4c
    pdwTemp = (u32*)(&shs_recon_frmbuf_index_reg4c);
    *pdwTemp = 0;
    shs_recon_frmbuf_index_reg4c.cur_reconstruct_frame_buf_index =
        pCurPicturePtr->nDecodeBufIndex;
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SHS_RECONSTRUCT_FRM_BUF_INDEX,*pdwTemp);
    PRINT_REG("0x4c: %x\n", *pdwTemp);

    maxFrmNum = hCtx->frmBufInf.nMaxValidFrmBufNum;
    if(maxFrmNum >= 18)
    {
    //    logd("maxFrmNum =%d\n", maxFrmNum);
        maxFrmNum = 18;
    }

    nYBufferOffset = 0;
    nCBufferOffset = 0;
    if(decStreamIndex == 1)
    {
        pCurPicturePtr = h264Dec->pMajorRefFrame;
        if(pCurPicturePtr != NULL)
        {
            //logd("major frame: H264ConfigureReconMvcolBufRegister,
            //pVpicture=%p\n", pCurPicturePtr->pVPicture);
            H264ConfigureReconMvcolBufRegister(h264Dec,pCurPicturePtr,
                    nYBufferOffset, nCBufferOffset);
        }
    }
    nYBufferOffset = FbmGetBufferOffset(hCtx->pFbm, 1);
    nCBufferOffset = FbmGetBufferOffset(hCtx->pFbm, 0);
    for(i=0+hCtx->nBufIndexOffset; i<maxFrmNum+hCtx->nBufIndexOffset; i++)
    {
        //logv("H264ConfigureReconMvcolBufRegister:i=%d\n", i);
        pCurPicturePtr = &(hCtx->frmBufInf.picture[i]);
        if(pCurPicturePtr->pVPicture != NULL)
        {
            //logd("configure decode frame: H264ConfigureReconMvcolBufRegister\n");
            H264ConfigureReconMvcolBufRegister(h264Dec,pCurPicturePtr,
                    nYBufferOffset, nCBufferOffset);
        }
    }

    pCurPicturePtr = hCtx->frmBufInf.pCurPicturePtr;
    H264ConfigureFrameInfoRegister(h264Dec,pCurPicturePtr);

    H264ConfigureRefListRegister(h264Dec, hCtx);

    if(h264DecCtx->vconfig.bScaleDownEn==1 ||
        h264DecCtx->vconfig.bRotationEn==1 ||
        hCtx->bYv12OutFlag==1 || hCtx->bNv21OutFlag==1)
    {
        H264ConfigureScaleRotateRegister(h264DecCtx, h264Dec, hCtx);
    }
    #if 0
    if(hCtx->bUseMafFlag == 1)
    {
        if(hCtx->bDecodeKeyFrameOnly == 0)
        {
            H264ConfigMafRegisters(h264Dec, hCtx);
        }
        else
        {
            hCtx->frmBufInf.pCurPicturePtr->pVPicture->bMafValid = 0;
            hCtx->frmBufInf.pCurPicturePtr->pVPicture->nMafFlagStride = 0;
            hCtx->frmBufInf.pCurPicturePtr->pVPicture->bPreFrmValid = 0;
            hCtx->frmBufInf.pCurPicturePtr->pVPicture->pMafData = NULL;
        }
    }
    #endif

start_decode_slice:

    if(h264DecCtx->bSoftDecHeader == 1)
    {
        H264ConfigVbvRegisters(h264DecCtx, hCtx, sliceDataLen);
    }
    H264EnableIntr(h264DecCtx);

#if H264_DEBUG_PRINTF_REGISTER
//    if(decStreamIndex==0 && hCtx->nCurFrmNum==0)
    {
        H264PrintRegister(h264DecCtx);
    }
#endif
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_TRIGGER_TYPE, DECODE_SLICE);
}
/***********************************************************************/
/***********************************************************************/

void H264ConfigureAvcRegister( H264DecCtx* h264DecCtx,
                            H264Context* hCtx,
                            u8 eptbDetectEnable, u32 nBitsLen)
{
    volatile u32 dwVal;
    H264Dec* h264Dec = NULL;

    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;

    vdec_readuint32(h264Dec->nRegisterBaseAddr+REG_FUNC_CTRL);
    dwVal &= ~STARTCODE_DETECT_E;
    dwVal |= EPTB_DETECTION_BY_PASS;

    if(eptbDetectEnable ==1)
    {
        dwVal &= ~EPTB_DETECTION_BY_PASS;
    }
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_FUNC_CTRL, dwVal);

    H264ConfigureBitstreamRegister(h264DecCtx, hCtx, nBitsLen);
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_TRIGGER_TYPE, INIT_SWDEC);
}

void H264SyncByte(H264DecCtx *h264DecCtx)
{
    CEDARC_UNUSE(H264SyncByte);
    H264Dec* h264Dec = NULL;
    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;

    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_TRIGGER_TYPE, SYNC_BYTE);
}

u32 H264GetBits(void* param, u32 len)
{
    volatile u32 dwVal;
    H264Dec* h264Dec = NULL;
    u32 value = 0;
    u32 nWaitTime = 0;
    #define CHECK_BUSY_TIME_OUT    (1000000)
    H264DecCtx* h264DecCtx = (H264DecCtx*)param;

    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;

    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_TRIGGER_TYPE, (len<<8)|GET_BITS);

    while(1)
    {
        vdec_readuint32(h264Dec->nRegisterBaseAddr+REG_FUNC_STATUS);
        if(dwVal & VLD_BUSY)
        {
            if((dwVal&0x4) ||(nWaitTime > CHECK_BUSY_TIME_OUT))
            {
                value = 0;
                break;
            }
            nWaitTime++;

        }
        else
        {
            vdec_readuint32(h264Dec->nRegisterBaseAddr+REG_BASIC_BITS_RETURN_DATA);
            value = dwVal;
            break;
        }
    }
    return value;
}

u32 H264ShowBits(void *param, u32 len)
{
    CEDARC_UNUSE(H264ShowBits);
    volatile u32 dwVal;
    H264Dec* h264Dec = NULL;
    u32 value = 0;
    u32 nWaitTime = 0;
    #define CHECK_BUSY_TIME_OUT    (1000000)

    H264DecCtx* h264DecCtx = (H264DecCtx*)param;

    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;

    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_TRIGGER_TYPE, (len<<8)|SHOW_BITS);

    while(1)
    {
        vdec_readuint32(h264Dec->nRegisterBaseAddr+REG_FUNC_STATUS);
        if(dwVal & VLD_BUSY)
        {
            if((dwVal&0x4)||(nWaitTime>CHECK_BUSY_TIME_OUT))
            {
                value = 0;
                break;
            }
            nWaitTime++;
        }
        else
        {
            vdec_readuint32(h264Dec->nRegisterBaseAddr+REG_BASIC_BITS_RETURN_DATA);
            value = dwVal;
            break;
        }
    }
    return value;
}

u32 H264GetUeGolomb(void* param)
{
    volatile u32 dwVal;
    H264Dec* h264Dec = NULL;
    u32 value = 0;
    u32 nWaitTime = 0;
    #define CHECK_BUSY_TIME_OUT    (1000000)

    H264DecCtx* h264DecCtx = (H264DecCtx*)param;

    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;

    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_TRIGGER_TYPE, GET_VLCUE);

    while(1)
    {
        vdec_readuint32(h264Dec->nRegisterBaseAddr+REG_FUNC_STATUS);
        if(dwVal & VLD_BUSY)
        {
            if((dwVal&0x4)||(nWaitTime>CHECK_BUSY_TIME_OUT))
            {
                value = 0;
                break;
            }
            nWaitTime++;
        }
        else
        {
            vdec_readuint32(h264Dec->nRegisterBaseAddr+REG_BASIC_BITS_RETURN_DATA);
            value = dwVal;
            break;
        }
    }
    return value;
}

s32 H264GetSeGolomb(void* param)
{
    volatile u32 dwVal;
    H264Dec* h264Dec = NULL;
    u32 value = 0;
    u32 nWaitTime = 0;
    #define CHECK_BUSY_TIME_OUT    (1000000)

    H264DecCtx* h264DecCtx = (H264DecCtx*)param;

    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;

    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_TRIGGER_TYPE, GET_VLCSE);

    while(1)
    {
        vdec_readuint32(h264Dec->nRegisterBaseAddr+REG_FUNC_STATUS);
        if(dwVal & VLD_BUSY)
        {
            if((dwVal&0x4)||(nWaitTime>CHECK_BUSY_TIME_OUT))
            {
                value = 0;
                break;
            }
            nWaitTime++;
        }
        else
        {
            vdec_readuint32(h264Dec->nRegisterBaseAddr+REG_BASIC_BITS_RETURN_DATA);
            value = dwVal;
            break;
        }
    }
    return value;
}

//#else
#define AV_RB32(x)  ((((const u8*)(x))[0] << 24) | \
                (((const u8*)(x))[1] << 16) | \
                (((const u8*)(x))[2] <<  8) | \
                ((const u8*)(x))[3])

#    define NEG_SSR32(a,s) (((s32)(a))>>(32-(s)))
#    define NEG_USR32(a,s) (((u32)(a))>>(32-(s)))

#   define MIN_CACHE_BITS 25

#   define OPEN_READER(name, gb)\
        s32 name##_index= (gb)->index;\
s32 name##_cache= 0;\

#   define CLOSE_READER(name, gb)\
        (gb)->index= name##_index;\

#   define UPDATE_CACHE(name, gb)\
        name##_cache= AV_RB32( ((const u8 *)(gb)->buffer)+(name##_index>>3) ) \
        << (name##_index&0x07);\

#   define SKIP_CACHE(name, gb, num)\
        name##_cache <<= (num);

#   define SHOW_UBITS(name, gb, num)\
        NEG_USR32(name##_cache, num)

#   define SHOW_SBITS(name, gb, num)\
        NEG_SSR32(name##_cache, num)

#   define SKIP_COUNTER(name, gb, num)\
        name##_index += (num);\

#   define SKIP_BITS(name, gb, num)\
{\
        SKIP_CACHE(name, gb, num)\
        SKIP_COUNTER(name, gb, num)\
}\

#   define LAST_SKIP_BITS(name, gb, num) SKIP_COUNTER(name, gb, num)
#   define LAST_SKIP_CACHE(name, gb, num) ;

#   define GET_CACHE(name, gb)\
        ((u32)name##_cache)

const u8 h264_golomb_vlc_len[512]={
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
        3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
        3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
        3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
        3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};

u8 h264_ue_golomb_vlc_code[512]={
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,
        7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9,10,10,10,10,
        11,11,11,11,12,12,12,12,13,13,13,13,14,14,14,14,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

u8 h264_log2_tab[256]={
        0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
        5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};

const s8 h264_se_golomb_vlc_code[512]={
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        8, -8,  9, -9, 10,-10, 11,-11, 12,-12, 13,-13, 14,-14, 15,-15,
        4,  4,  4,  4, -4, -4, -4, -4,  5,  5,  5,  5, -5, -5, -5, -5,
        6,  6,  6,  6, -6, -6, -6, -6,  7,  7,  7,  7, -7, -7, -7, -7,
        2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
        -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3,
        1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
        1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
        1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
        1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
};

#define min(x, y)   ((x) <= (y) ? (x) : (y));

static s32 H264av_log2(u32 v)
{
    s32 n = 0;
    if (v & 0xffff0000)
    {
        v >>= 16;
        n += 16;
    }
    if (v & 0xff00)
    {
        v >>= 8;
        n += 8;
    }
    n += h264_log2_tab[v];

    return n;
}

static u32 H264GetBits1(GetBitContext *s)
{
    s32 index= s->index;
    u8 result = s->buffer[index>>3] ;
    result<<= (index&0x07);
    result>>= 8 - 1;
    index++;
    s->index= index;
    return result;
}


u32 H264GetBitsSw(void* param, u32 n)
{
    register s32 tmp;
    GetBitContext *gb = NULL;

    H264DecCtx* h264DecCtx = (H264DecCtx*)param;

    gb = &h264DecCtx->sBitContext;

    if(n == 1)
    {
        tmp = H264GetBits1(gb);
    }
    else
    {
        OPEN_READER(re, gb)
        UPDATE_CACHE(re, gb)
        tmp = SHOW_UBITS(re, gb, n);
        LAST_SKIP_BITS(re, gb, n)
        CLOSE_READER(re, gb)
    }
    return tmp;
}


u32 H264GetUeGolombSw(void* param)
{
    u32 buf;
    s32 log;

    GetBitContext *gb = NULL;
    H264DecCtx* h264DecCtx = (H264DecCtx*)param;

    gb = &h264DecCtx->sBitContext;

    OPEN_READER(re, gb);
    UPDATE_CACHE(re, gb);
    buf=GET_CACHE(re, gb);

    if(buf >= (1<<27))
    {
        buf >>= 32 - 9;
        LAST_SKIP_BITS(re, gb, h264_golomb_vlc_len[buf]);
        CLOSE_READER(re, gb);
        return h264_ue_golomb_vlc_code[buf];
    }
    else
    {
        log= 2*H264av_log2(buf) - 31;
        buf>>= log;
        buf--;
        LAST_SKIP_BITS(re, gb, 32 - log);
        CLOSE_READER(re, gb);
        return buf;
    }
}


u32 H264ShowBitsSw(void *param, u32 n)
{
    //unsigned int buf;
    GetBitContext *s = NULL;

    H264DecCtx* h264DecCtx = (H264DecCtx*)param;
    s = &h264DecCtx->sBitContext;

    register int tmp;
    OPEN_READER(re, s);
    UPDATE_CACHE(re, s);
    tmp = SHOW_UBITS(re, s, n);
    return tmp;
}


void H264InitGetBitsSw(void* param, u8 *buffer, u32 bit_size)
{
    GetBitContext *s = NULL;
    H264DecCtx* h264DecCtx = (H264DecCtx*)param;

    s = &h264DecCtx->sBitContext;

    s32 buffer_size = (bit_size+7)>>3;
    s->buffer = (u8*)buffer;
    s->size_in_bits = (s32)bit_size;
    s->buffer_end = (u8*)(buffer + buffer_size);
    s->index=0;
}

s32 H264GetSeGolombSw(void* param)

{
    unsigned int buf;
    GetBitContext *gb = NULL;
    H264DecCtx* h264DecCtx = (H264DecCtx*)param;

    gb = &h264DecCtx->sBitContext;

    OPEN_READER(re, gb);
    UPDATE_CACHE(re, gb);
    buf = GET_CACHE(re, gb);

    if (buf >= (1 << 27))
    {
        buf >>= 32 - 9;
        LAST_SKIP_BITS(re, gb, h264_golomb_vlc_len[buf]);
        CLOSE_READER(re, gb);

        return h264_se_golomb_vlc_code[buf];
    }
    else
    {
        int log = H264av_log2(buf), sign;
        LAST_SKIP_BITS(re, gb, 31 - log);
        UPDATE_CACHE(re, gb);
        buf = GET_CACHE(re, gb);

        buf >>= log;

        LAST_SKIP_BITS(re, gb, 32 - log);
        CLOSE_READER(re, gb);

        sign = -(buf & 1);
        buf  = ((buf >> 1) ^ sign) - sign;

        return buf;
    }
}

u32 H264GetBitCountSw(void* param)
{
    GetBitContext *gb = NULL;
    H264DecCtx* h264DecCtx = (H264DecCtx*)param;
    gb = &h264DecCtx->sBitContext;

    //logd("******gb->index=%x, gb->size_in_bits=%x\n", gb->index, gb->size_in_bits);
    return gb->index;
}


//#endif

u32 H264GetDecodeMbNum(H264DecCtx* h264DecCtx)
{
    H264Dec* h264Dec = NULL;
    volatile u32 dwVal;

    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;
    vdec_readuint32(h264Dec->nRegisterBaseAddr+REG_CUR_MB_NUM);
    return dwVal;
}

u32 H264GetFunctionStatus(H264DecCtx* h264DecCtx)
{
    CEDARC_UNUSE(H264GetFunctionStatus);
    H264Dec* h264Dec = NULL;
    volatile u32 dwVal;

    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;
    vdec_readuint32(h264Dec->nRegisterBaseAddr+REG_FUNC_STATUS);
    return dwVal;
}

u32 H264VeIsr(H264DecCtx* h264DecCtx)
{
    H264Dec* h264Dec = NULL;
    volatile u32 dwVal;
    u32 decode_result = 0;

    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;
    decode_result  = 0;
    vdec_readuint32(h264Dec->nRegisterBaseAddr+REG_FUNC_STATUS);

    if((dwVal&0x02) > 0)
    {
        decode_result |= H264_IR_ERR;
       // logw("ve dec error.\n");
    }
    if((dwVal&0x01) > 0)
    {
        decode_result |= H264_IR_FINISH;
    }
    if((dwVal&0x04) > 0)
    {
        decode_result |= H264_IR_DATA_REQ;
    }

    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_FUNC_STATUS,dwVal);

    //vdec_readuint32(h264Dec->nRegisterBaseAddr+REG_ERROR_CASE);
    //logd("***************0xb8 error=%x\n", dwVal);
    return decode_result;
}

void H264CongigureWeightTableRegisters(H264DecCtx* h264DecCtx, H264Context* hCtx)
{
    u32 weight = 0;
    s32 i = 0;
    u32 j = 0;
    H264Dec* h264Dec = NULL;
    u32 lumaLog2WeightDenom = 0;
    u32 chromaLog2WeightDenom = 0;
       volatile u32* pdwTemp;

    h264Dec = (H264Dec*)h264DecCtx->pH264Dec;

    if((hCtx->pCurPps->nWeightedBIpredIdc==2)&& (hCtx->nSliceType==H264_B_TYPE))
    {
        lumaLog2WeightDenom = 5;
        chromaLog2WeightDenom = 5;
    }
    else
    {
        lumaLog2WeightDenom = h264DecCtx->GetUeGolomb((void*)h264DecCtx);
        chromaLog2WeightDenom = h264DecCtx->GetUeGolomb((void*)h264DecCtx);
    }

    pdwTemp = (u32*)(&shs_wp_reg10);
    *pdwTemp = 0;
    shs_wp_reg10.luma_log2_weight_denom = lumaLog2WeightDenom;
    shs_wp_reg10.chroma_log2_weight_denom = chromaLog2WeightDenom;
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SHS_WP,*pdwTemp);

    //logv("0x10:%x\n", *pdwTemp);

    pdwTemp = (u32*)(&sram_port_rw_offset_rege0);
    *pdwTemp = 0;
    vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_OFFSET,*pdwTemp);
    PRINT_REG("32:0xe0:%x\n", *pdwTemp);

    for(j=0; j<2; j++)
    {
        weight = 1<<lumaLog2WeightDenom;
        for(i=0; i<32; i++)
        {
            vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_DATA,weight);
        }
        PRINT_REG("32:0xe4:%x\n", weight);
        weight = 1<<chromaLog2WeightDenom;
        for(i=0; i<64; i++)
        {
            vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_DATA,weight);
        }
        PRINT_REG("64:0xe4:%x\n", weight);
    }

    if((hCtx->pCurPps->bWeightedPred&& hCtx->nSliceType==H264_P_TYPE) ||
            (hCtx->pCurPps->nWeightedBIpredIdc==1 && hCtx->nSliceType==H264_B_TYPE))
    {
        for(i=0; i<hCtx->nRefCount[0]; i++)
        {
            if(h264DecCtx->GetBits((void*)h264DecCtx,1))
            {
                weight = h264DecCtx->GetSeGolomb((void*)h264DecCtx)&0xffff;
                weight |= (h264DecCtx->GetSeGolomb((void*)h264DecCtx)&0xffff)<<16;
                vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_OFFSET,i<<2);
                vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_DATA,weight);
                PRINT_REG("0xe0:%x\n", i<<2);
                PRINT_REG("0xe4:%x\n", weight);
            }

            if(h264DecCtx->GetBits((void*)h264DecCtx,1))
            {
                vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_OFFSET,32*4+(i<<3));
                PRINT_REG("0xe0:%x\n", 32*4+(i<<3));

                weight = h264DecCtx->GetSeGolomb((void*)h264DecCtx)&0xffff;
                weight |= (h264DecCtx->GetSeGolomb((void*)h264DecCtx)&0xffff)<<16;
                vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_DATA,weight);
                PRINT_REG("0xe4:%x\n", weight);
                PRINT_REG("0xe4:%x\n", weight);
                weight = h264DecCtx->GetSeGolomb((void*)h264DecCtx)&0xffff;
                weight |= (h264DecCtx->GetSeGolomb((void*)h264DecCtx)&0xffff)<<16;
                vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_DATA,weight);
                PRINT_REG("0xe4:%x\n", weight);
            }
        }
    }

    if(hCtx->nSliceType==H264_B_TYPE && hCtx->pCurPps->nWeightedBIpredIdc==1)
    {
        for(i=0; i<hCtx->nRefCount[1]; i++)
        {
            if(h264DecCtx->GetBits((void*)h264DecCtx,1))
            {
                weight = h264DecCtx->GetSeGolomb((void*)h264DecCtx)&0xffff;
                weight |= (h264DecCtx->GetSeGolomb((void*)h264DecCtx)&0xffff)<<16;
                vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_OFFSET,
                    32*3*4+(i<<2));
                vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_DATA,weight);
                PRINT_REG("0xe0:%x\n", 32*3*4+(i<<2));
                PRINT_REG("0xe4:%x\n", weight);
            }
            if(h264DecCtx->GetBits((void*)h264DecCtx,1))
            {
                vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_OFFSET,
                    32*3*4+32*4+(i<<3));
                PRINT_REG("0xe0:%x\n",32*3*4+32*4+(i<<3));

                weight = h264DecCtx->GetSeGolomb((void*)h264DecCtx)&0xffff;
                weight |= (h264DecCtx->GetSeGolomb((void*)h264DecCtx)&0xffff)<<16;
                vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_DATA,weight);
                PRINT_REG("0xe4:%x\n", weight);

                weight = h264DecCtx->GetSeGolomb((void*)h264DecCtx)&0xffff;
                weight |= (h264DecCtx->GetSeGolomb((void*)h264DecCtx)&0xffff)<<16;
                vdec_writeuint32(h264Dec->nRegisterBaseAddr+REG_SRAM_PORT_RW_DATA,weight);
                PRINT_REG("0xe4:%x\n", weight);
            }
        }
    }
    return;
}

