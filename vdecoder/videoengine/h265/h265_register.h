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
* File : h265_register.h
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#ifndef H265_REGISTER_H
#define H265_REGISTER_H

#ifdef __cplusplus
extern "C" {
#endif
#include "h265_config.h"

u32    VECORE_TOP_ADDR;
u32    HEVC_REG_BASE;
//#define VECORE_TOP_ADDR             0x01c0e000
//#define HEVC_REG_BASE               0x01c0e500
#define TOP2HEVC_OFFSET             0x500

#define HEVC_SRAM_BASE_ADDR         0x0000
#define SDRAM_SIZE                   (128*1024*1024) // ?? sean  need confire 4096

#define HEVC_WEIGHT_PREDPARA_LEN    768        //32*3*2*4 bytes
#define HEVC_FRM_BUF_INFO_LEN        768        //32*6*4 bytes
#define HEVC_SCALING_MATRIX_LEN        224        //(4*6+16*2)*4 bytes
#define HEVC_REF_LIST_LEN            32        //8*4 bytes

//  SRAM block address offset. The BASE is HEVC_SRAM_BASE_ADDR
#define HEVC_WEIGHT_PRED_PARAM_ADDR    0x000
#define HEVC_FRM_BUF_INFO_ADDR        0x400
#define HEVC_SCALING_MATRIX_ADDR    0x800
#define HEVC_REF_LIST_ADDR            0xc00

///  VECORE register address offset. The BASE is VECORE_TOP_ADDR
#define VECORE_MODESEL_REG                    0x00
#define VECORE_RESET_REG                    0x04
#define VECORE_CYCLES_COUNTER_REG            0x08
#define VECORE_OVERTIMETH_REG                0x0c
#define VECORE_MMCREQ_WNUM_REG                0x10
#define VECORE_CACHEREG_WNUM_REG            0x14
//reserved                                    0x18
#define VECORE_STATUS_REG                    0x1c
#define VECORE_RDDATA_COUNTER_REG            0x20
#define VECORE_WRDATA_COUNTER_REG            0x24
#define VECORE_ANAGLYPH_CTRL_REG            0x28

#define VECORE_MAF_CTRL_REG                    0x30
#define VECORE_MAF_TH_CLIP_REG                0x34
#define VECORE_MAF_REF1_FRMBUF_YADDR_REG    0x38
#define VECORE_MAF_REF1_FRMBUF_CADDR_REG    0x3c
#define VECORE_CUR_FRM_MAF_ADDR_REG         0x40
#define VECORE_REF1_FRM_MAF_ADDR_REG        0x44
#define VECORE_REF2_FRM_MAF_ADDR_REG        0x48
#define VECORE_MAF_MAX_GROUP_DIFF_REG       0x4c
#define VECORE_ABOVE_BUF_CTRL                0x50
#define VECORE_DBLK_BUF_ADDR                0x54
#define VECORE_INTRAPRED_BUF_ADDR            0x58

#define VECORE_CMD_QUEUE_REG                0x5c
#define VECORE_SRC1BLK_ADDR_REG                0x60
#define VECORE_SRC2BLK_ADDR_REG                0x64
#define VECORE_TGTBLK_ADDR_REG                0x68
#define VECORE_SRCBLK_STRIDE_REG            0x6c
#define VECORE_TGTBLK_STRIDE_REG            0x70
#define VECORE_BLKSIZE_REG                    0x74
#define VECORE_ARGB_VALUE_REG                0x78
#define VECORE_ARGB_CTRL_REG                0x7c

#define VECORE_PRI_CHROMA_BUF_LEN_REG        0xc4
#define VECORE_PRI_FRMBUF_STRIDE_REG        0xc8
#define VECORE_SEC_FRMBUF_STRIDE_REG        0xcc
#define VECORE_ANAGLYPH_OUTR_REG            0xD0
#define VECORE_ANAGLYPH_OUTG_REG            0xD4
#define VECORE_ANAGLYPH_OUTB_REG            0xD8

#define VECORE_SRAM_OFFSET_REG              0xe0
#define VECORE_SRAM_DATA_REG                0xe4
#define VECORE_OUT_CHROMA_LEN_REG           0xE8
#define VECORE_PRIMARY_OUT_FORMAT_REG       0xEC

///  HEVC register address offset. The BASE is HEVC_REG_BASE
#define HEVC_NAL_HDR_REG                        0x00
#define HEVC_SPS_REG                            0x04
#define HEVC_PIC_SIZE_REG                       0x08
#define HEVC_PCM_CTRL_REG                       0x0C
#define HEVC_PPS_CTRL0_REG                      0x10
#define HEVC_PPS_CTRL1_REG                      0x14
#define HEVC_SCALING_LIST_CTRL0_REG             0x18
#define HEVC_SCALING_LIST_CTRL1_REG             0x1c
#define HEVC_SLICE_HDR_INFO0_REG                0x20
#define HEVC_SLICE_HDR_INFO1_REG                0x24
#define HEVC_SLICE_HDR_INFO2_REG                0x28
#define HEVC_CTB_ADDR_REG                       0x2C
#define HEVC_FUNC_CTRL_REG                      0x30
#define HEVC_TRIGGER_TYPE_REG                   0x34
#define HEVC_FUNCTION_STATUS_REG                0x38
#define HEVC_DECCTU_NUM_REG                     0x3C
#define HEVC_BITS_BASE_ADDR_REG                 0x40
#define HEVC_BITS_OFFSET_REG                    0x44
#define HEVC_BITS_LEN_REG                       0x48
#define HEVC_BITS_END_ADDR_REG                  0x4C
#define HEVC_SDRT_CTRL_REG                      0x50
#define HEVC_SDRT_YBUF_REG                      0x54
#define HEVC_SDRT_CBUF_REG                      0x58
#define HEVC_CUR_REC_FRMBUF_IDX_REG             0x5C
#define HEVC_NEIGHBOR_INFO_ADDR_REG             0x60
#define HEVC_ENTRY_POINT_OFFSET_ADDR_REG        0x64
#define HEVC_TILE_START_ADDR_REG                0x68
#define HEVC_TILE_END_ADDR_REG                  0x6C
#define HEVC_TQ_BYPASS_ADDR_REG                 0x70
#define HEVC_VLD_BYPASS_ADDR_REG                0x74
#define HEVC_SCALER_LIST_DC_VAL0_REG            0x78
#define HEVC_SCALER_LIST_DC_VAL1_REG            0x7C
#define HEVC_ENTRY_POINT_OFFSET_LOW_ADDR_REG    0x80
#define HEVC_LOWER_2BIT_ADDR_OF_FIRST_OUTPUT_REG    0x84
#define HEVC_LOWER_2BIT_ADDR_OF_second_OUTPUT_REG   0x88
#define HEVC_10BIT_CONFIGURE_REG                    0x8c
//0x78~0xB4    reserved
#define HEVC_ERROR_CASE_REG                     0xB8
//0xBC~0xD8    reserved
#define HEVC_BITS_RETDATA_REG                   0xDC
#define HEVC_SRAM_PORT_ADDR_REG                 0xE0
#define HEVC_SRAM_PORT_DATA_REG                 0xE4

#define HEVC_STCD_HW_BITOFFSET_REG              0xF0
#define HEVC_DEBUG_REG                          0xFC
//0xE8~0xFC    reserved

// Video Engine start type:
//       first 4 bits 0000 refer to HEVC
//       second 4 bits refer to different operations
#define HEVC_SHOW_BITS_START            0x01
#define    HEVC_GET_BITS_START                0x02
#define HEVC_FLUSH_BITS_START            0x03
#define HEVC_GET_VLCSE_START            0x04
#define HEVC_GET_VLCUE_START            0x05
#define HEVC_BYTE_ALIGN                    0x06
#define HEVC_INIT_SWDEC_START            0x07
#define HEVC_SLICE_DEC_START            0x08
#define    JUMP2NEXTSTARTCODE                0x0c
#define    TerminateSTARTCODEDET            0x0d

#define HEVC_BS_DMA_BUSY                1<<19
#define HEVC_MCRI_CACHE_BUSY            1<<16
#define HEVC_DBLK_BUSY                    1<<15
#define HEVC_IREC_BUSY                    1<<14
#define HEVC_INTRA_PRED_BUSY            1<<13
#define HEVC_MCRI_BUSY                    1<<12
#define HEVC_IQIt_BUSY                    1<<11
#define HEVC_MVP_BUSY                    1<<10
#define HEVC_IS_BUSY                    1<<9
#define HEVC_VLD_BUSY                    1<<8
#define HEVC_OVERTIME                    1<<3
#define HEVC_VLD_DATA_REQ                1<<2
#define HEVC_DEC_ERROR                    1<<1
#define HEVC_SLICE_DEC_FINISH            1<<0

///  SRAM typedef
typedef struct SRAM_FRM_BUFFER_INFO
{
    u32  poc;
    u32  bottom_poc;
    u32  mvInfo;
    u32  bottom_mvInfo;
    u32  lumaBufAddr;
    u32  chromaBufAddr;

}SramFrameBufInfo;

typedef struct HEVC_LOWER_2BIT_ADDR_OF_FIRST_OUTPUT
{
    volatile unsigned int first_lower_2bit_offset_addr : 28; //0:27
    volatile unsigned int r0                           : 4;//28:31
}regHEVC_LOWER_2BIT_ADDR_OF_FIRST_OUTPUT;

typedef struct HEVC_LOWER_2BIT_ADDR_OF_second_OUTPUT
{
    volatile unsigned int second_lower_2bit_offset_addr : 28; //0:27
    volatile unsigned int r0                            : 4;//28:31
}regHEVC_LOWER_2BIT_ADDR_OF_SECOND_OUTPUT;

typedef struct HEVC_10BIT_CONFIGURE
{
    volatile unsigned int lower_2bit_data_stride_first_output  : 11;//0:10
    volatile unsigned int lower_2bit_data_stride_second_output : 11;//11:21
    volatile unsigned int lower_2bit_data_output_enable        : 1; //22
    volatile unsigned int second_out_format                    : 2;//23:24
    volatile unsigned int r0                                   : 7;//25:31
}regHEVC_10BIT_CONFIGURE_REG;

typedef struct REFERENCE_LIST
{
    volatile unsigned int idx            : 5; // 0:4
    volatile unsigned int r0             : 1; // 5
    volatile unsigned int top_or_bottom  : 1; // 6   0: top or frame   1: bottom
    volatile unsigned int lt_or_st       : 1; // 7   0: short term     1: long term
    volatile unsigned int r1             : 24; // 8:31
}ReferenceList;

///  HEVC register typedef
typedef struct HEVC_HAL_HDR
{
    volatile unsigned nal_unit_type                    :    6;    //bit 0~5
    volatile unsigned nuh_temporal_id_plus1            :    3;    //bit 6~8
    volatile unsigned r1                            :    23;    //bit 9~31
}regHEVC_NAL_HDR;

typedef struct HEVC_SPS
{
    volatile unsigned chroma_format_idc                :    2;    //bit 0~1
    volatile unsigned separate_color_plane_flag        :    1;    //bit 2
    volatile unsigned bit_depth_luma_minus_8        :    3;    //bit 3~5
    volatile unsigned bit_depth_chroma_minus_8        :    3;    //bit 6~8
    volatile unsigned log2_min_luma_CB_size_minus3    :    2;    //bit 9~10
    volatile unsigned log2_diff_max_min_luma_CB_size:    2;    //bit 11~12
    volatile unsigned log2_min_TB_size_minus2        :    2;    //bit 13~14
    volatile unsigned log2_diff_max_min_TB_size        :    2;    //bit 15~16
    volatile unsigned max_transform_depth_inter        :    3;    //bit 17~19
    volatile unsigned max_transform_depth_intra        :    3;    //bit 20~22
    volatile unsigned amp_enabled_flag                :    1;    //bit 23
    volatile unsigned sample_adaptive_offset_en        :    1;    //bit 24
    volatile unsigned sps_temporal_mvp_en_flag        :    1;    //bit 25
    volatile unsigned strong_intra_smoothing_en        :    1;    //bit 26
    volatile unsigned r0                            :    5;    //bit 27~31
}regHEVC_SPS;

typedef struct HEVC_PIC_SIZE
{
    volatile unsigned pic_width_in_luma_samples        :    13;    //bit 0~12
    volatile unsigned r0                            :    3;    //bit 13~15
    volatile unsigned pic_height_in_luma_samples    :    13;    //bit 16~28
    volatile unsigned r1                            :    3;    //bit 29~31
}regHEVC_PIC_SIZE;

typedef struct HEVC_PCM_CTRL
{
    volatile unsigned pcm_bit_depth_luma_minus1            :    4;    //bit 0~3
    volatile unsigned pcm_bit_depth_chroma_minus1        :    4;    //bit 4~7
    volatile unsigned log2_min_pcm_luma_CB_size_minus3    :    2;    //bit 8~9
    volatile unsigned log2_diff_max_min_pcm_luma_CB_size:    2;    //bit 10~11
    volatile unsigned r0                                :    2;    //bit 12~13
    volatile unsigned pcm_loop_filter_disable_flag        :    1;    //bit 14
    volatile unsigned pcm_enable_flag                    :    1;    //bit 15
    volatile unsigned r1                                :    16;    //bit 16~31
}regHEVC_PCM_CTRL;

typedef struct regHEVC_PPS_CTRL0_VERSION01
{
    volatile unsigned sign_data_hiding_flag            :    1;    //bit 00
    volatile unsigned constrained_intra_pred_flag    :    1;    //bit 01
    volatile unsigned transform_skip_en                :    1;    //bit 02
    volatile unsigned cu_qp_delta_en_flag            :    1;    //bit 03
    volatile unsigned diff_cu_qp_delta_depth        :    2;    //bit 04~05
    volatile unsigned r0                            :    2;    //bit 06~07
    volatile unsigned init_qp_minus26                :    6;    //bit 08~14
    volatile unsigned r1                            :    2;    //bit 15
    volatile unsigned pps_cb_qp_offset                :    6;    //bit 16~21
    volatile unsigned r2                            :    2;    //bit 22~23
    volatile unsigned pps_cr_qp_offset                :    6;    //bit 24~29
    volatile unsigned r3                            :    2;    //bit 30~21
}regHEVC_PPS_CTRL0_VERSION01;

typedef struct regHEVC_PPS_CTRL0_VERSION02
{
    volatile unsigned sign_data_hiding_flag            :    1;    //bit 00
    volatile unsigned constrained_intra_pred_flag    :    1;    //bit 01
    volatile unsigned transform_skip_en                :    1;    //bit 02
    volatile unsigned cu_qp_delta_en_flag            :    1;    //bit 03
    volatile unsigned diff_cu_qp_delta_depth        :    2;    //bit 04~05
    volatile unsigned r0                            :    2;    //bit 06~07
    volatile unsigned init_qp_minus26                :    7;    //bit 08~14
    volatile unsigned r1                            :    1;    //bit 15
    volatile unsigned pps_cb_qp_offset                :    6;    //bit 16~21
    volatile unsigned r2                            :    2;    //bit 22~23
    volatile unsigned pps_cr_qp_offset                :    6;    //bit 24~29
    volatile unsigned r3                            :    2;    //bit 30~21
}regHEVC_PPS_CTRL0_VERSION02;

typedef struct HEVC_PPS_CTRL1
{
    volatile unsigned weighted_pred_flag            :     1;    //bit 0
    volatile unsigned weighted_birped_flag            :     1;    //bit 1
    volatile unsigned transquant_bypass_en            :     1;    //bit 2
    volatile unsigned tiles_en_flag                    :     1;    //bit 3
    volatile unsigned entropy_coding_sync_en        :     1;    //bit 4
    volatile unsigned loop_filter_accross_tiles_en    :     1;    //bit 5
    volatile unsigned loop_filter_accross_slice_en    :     1;    //bit 6
    volatile unsigned r0                            :     1;    //bit 7
    volatile unsigned log2_parallel_merge_level_minus2    :     3; //bit 8~10
    volatile unsigned r1                            :     21; //bit 11~31
}regHEVC_PPS_CTRL1;

typedef struct HEVC_SCALING_LIST_CTRL0
{
    volatile unsigned intra4x4_Y                    :     3;    //bit 0~2
    volatile unsigned intra4x4_Cb                    :     3;    //bit 3~5
    volatile unsigned intra4x4_Cr                    :     3;    //bit 6~8
    volatile unsigned inter4x4_Y                    :     3;    //bit 9~11
    volatile unsigned inter4x4_Cb                    :     3;    //bit 12~14
    volatile unsigned inter4x4_Cr                    :     3;    //bit 15~17
    volatile unsigned intra8x8_Y                    :     3;    //bit 18~20
    volatile unsigned intra8x8_Cb                    :     3;    //bit 21~23
    volatile unsigned intra8x8_Cr                    :     3;    //bit 24~26
    volatile unsigned r0                            :     3; //bit 27~29
    volatile unsigned use_default_scaling_matrix    :     1; //bit 30
    volatile unsigned scaling_list_enabled_flag        :     1;    //bit 31
}regHEVC_SCALING_LIST_CTRL0;

typedef struct HEVC_SCALING_LIST_CTRL1
{
    volatile unsigned inter8x8_Y                    :     3;    //bit 0~2
    volatile unsigned inter8x8_Cb                    :     3;    //bit 3~5
    volatile unsigned inter8x8_Cr                    :     3;    //bit 6~8
    volatile unsigned intra16x16_Y                    :     3;    //bit 9~11
    volatile unsigned intra16x16_Cb                    :     3;    //bit 12~14
    volatile unsigned intra16x16_Cr                    :     3;    //bit 15~17
    volatile unsigned inter16x16_Y                    :     3;    //bit 18~20
    volatile unsigned inter16x16_Cb                    :     3;    //bit 21~23
    volatile unsigned inter16x16_Cr                    :     3;    //bit 24~26
    volatile unsigned intra32x32_Y                    :     1;    //bit 27
    volatile unsigned inter32x32_Y                    :     1;    //bit 28
    volatile unsigned r0                            :     3; //bit 29~31
}regHEVC_SCALING_LIST_CTRL1;

typedef struct HEVC_SLICE_HDR_INFO0
{
    volatile unsigned first_slice_segment_flag        :    1;    //bit 0
    volatile unsigned dependent_slice_segment_flag    :    1;    //bit 1
    volatile unsigned slice_type                    :    2;    //bit 2~3
    volatile unsigned color_plane_id                :    2;    //bit 4~5
    volatile unsigned slice_temporal_mvp_enable_flag:    1;    //bit 6
    volatile unsigned slice_sao_luma_flag            :    1;    //bit 7
    volatile unsigned slice_sao_chroma_flag            :    1;    //bit 8
    volatile unsigned mvd_l1_zero_flag                :    1;    //bit 9
    volatile unsigned cabac_init_flag                :    1;    //bit 10
    volatile unsigned collocated_from_l0_flag        :    1;    //bit 11
    volatile unsigned collocated_ref_idx            :    4;    //bit 12:15
    volatile unsigned num_ref_idx0_active_minus1    :    4;    //bit 16:19
    volatile unsigned num_ref_idx1_active_minus1    :    4;    //bit 20:23
    volatile unsigned five_minus_max_num_merg_cand    :    3;    //bit 24:26
    volatile unsigned r0                            :    1;    //bit 27
    volatile unsigned picture_type                    :    2;    //bit 28~29
    volatile unsigned r1                            :    2;    //bit 30~31
}regHEVC_SLICE_HDR_INFO0;

typedef struct HEVC_SLICE_HDR_INFO1_V01
{
    volatile unsigned slice_qp_delta                    :    6;    //bit 0~6
    volatile unsigned r0                                :    2;    //bit 7
    volatile unsigned slice_cb_qp_offset                :    5;    //bit 8~12
    volatile unsigned r1                                :    3;    //bit 13~15
    volatile unsigned slice_cr_qp_offset                :    5;    //bit 16~20
    volatile unsigned is_not_blowdelay_flag                :   1;    //bit 21
    volatile unsigned slice_loop_filter_accross_slcies_en:    1;    //bit 22
    volatile unsigned slice_disable_deblocking_flag        :    1;    //bit 23
    volatile unsigned slice_beta_offset_div2            :    4;    //bit 24~27
    volatile unsigned slice_tc_offset_div2                :    4;    //bit 28~31
}regHEVC_SLICE_HDR_INFO1_V01;

typedef struct HEVC_SLICE_HDR_INFO1_V02
{
    volatile unsigned slice_qp_delta                    :    7;    //bit 0~6
    volatile unsigned r0                                :    1;    //bit 7
    volatile unsigned slice_cb_qp_offset                :    5;    //bit 8~12
    volatile unsigned r1                                :    3;    //bit 13~15
    volatile unsigned slice_cr_qp_offset                :    5;    //bit 16~20
    volatile unsigned is_not_blowdelay_flag                :   1;    //bit 21
    volatile unsigned slice_loop_filter_accross_slcies_en:    1;    //bit 22
    volatile unsigned slice_disable_deblocking_flag        :    1;    //bit 23
    volatile unsigned slice_beta_offset_div2            :    4;    //bit 24~27
    volatile unsigned slice_tc_offset_div2                :    4;    //bit 28~31
}regHEVC_SLICE_HDR_INFO1_V02;

typedef struct HEVC_SLICE_HDR_INFO2
{
    volatile unsigned luma_log2_weight_denom        :    3;    //bit 0~2
    volatile unsigned r0                            :    1;    //bit 3
    volatile unsigned chroma_log2_weight_denom        :    3;    //bit 4~6
    volatile unsigned r1                            :    1;    //7
    volatile unsigned num_entry_point_offsets        :  14;    //bit 8~21
    volatile unsigned r2                            :  10;    //bit 22~31
}regHEVC_SLICE_HDR_INFO2;

typedef struct HEVC_CTB_ADDR_V01
{
    volatile unsigned CTB_x                            :    9;    //bit 0~8
    volatile unsigned r0                            :    7;    //bit 9~15
    volatile unsigned CTB_y                            :    9;    //bit 16~24
    volatile unsigned r1                            :    7;    //bit 25~31
}regHEVC_CTB_ADDR_V01;

typedef struct HEVC_CTB_ADDR_V02
{
    volatile unsigned CTB_x                            :    10;    //bit 0~9
    volatile unsigned r0                            :    6;    //bit 10~15
    volatile unsigned CTB_y                            :    10;    //bit 16~25
    volatile unsigned r1                            :    6;    //bit 26~31
}regHEVC_CTB_ADDR_V02;

typedef struct HEVC_FUNC_CTRL
{
    volatile unsigned finish_interrupt_en            :    1;    //bit 0
    volatile unsigned error_interrupt_en            :    1;    //bit 1
    volatile unsigned data_request_interrupt_en        :    1;    //bit 2
    volatile unsigned r0                            :    5;    //bit 3~7
    volatile unsigned not_wirte_recons_pic            :    1;    //bit 8
    volatile unsigned write_sc_rt_pic                :    1;    //bit 9
    volatile unsigned mcri_cache_en                    :    1;    //bit 10
    volatile unsigned vld_bypass_en                    :   1;  //bit 11
    volatile unsigned tq_bypass_en                    :   1;    //bit 12
    volatile unsigned r1                            :    11;    //bit 13~23
    volatile unsigned eptb_detection_by_pass        :    1;    //bit 24
    volatile unsigned startcode_detect_en            :    1;    //bit 25
    volatile unsigned r2                            :    5;    //bit 26~30
    volatile unsigned ddr_consistency_en            :    1;  //bit 31 , for 0x31010
}regHEVC_FUNC_CTRL;

typedef struct HEVC_TRIGGER_TYPE
{
    volatile unsigned trigger_type_pul                :    4;    //bit 0~3
    volatile unsigned stcd_type                        :    2;    //bit 4~5
    volatile unsigned r0                            :    2;    //bit 6~7
    volatile unsigned n_bits                        :    6;    //bit 8~13
    volatile unsigned r1                            :    18;    //bit 14~31
}regHEVC_TRIGGER_TYPE;

typedef struct HEVC_FUNCTION_STSTUS
{
    volatile unsigned slice_dec_finish                :    1;    //bit 0
    volatile unsigned slice_dec_error                :    1;    //bit 1
    volatile unsigned vld_data_req                    :    1;    //bit 2
    volatile unsigned overtime                        :    1;    //bit 3
    volatile unsigned r0                                :    4;    //bit 4~7
    volatile unsigned vld_busy                        :    1;    //bit 8
    volatile unsigned is_busy                        :    1;    //bit 9
    volatile unsigned mvp_busy                        :    1;    //bit 10
    volatile unsigned iqit_busy                        :    1;    //bit 11
    volatile unsigned mcri_busy                        :    1;    //bit 12
    volatile unsigned intra_pred_busy                :    1;    //bit 13
    volatile unsigned irec_busy                        :    1;    //bit 14
    volatile unsigned dblk_busy                        :    1;    //bit 15
    volatile unsigned more_data_flag                    :    1;    //bit 16
    volatile unsigned interm_busy                    :    1;    //bit 17
    volatile unsigned it_busy                        :    1;    //bit 18
    volatile unsigned bs_dma_busy                    :    1;    //bit 19
    volatile unsigned wb_busy                        :    1;    //bit 20
    volatile unsigned stcd_busy                        :    1;    //bit 21
    volatile unsigned startcode_type                :    2;    //bit 22~23
    volatile unsigned r1                            :    8;    //bit 24~31
}regHEVC_FUNCTION_STATUS;

typedef struct HEVC_DEC_CTB_NUM
{
    volatile unsigned num                            :    18;    //bit 0~17
    volatile unsigned r0                                :    14;    //bit 18~31
}regHEVC_DEC_CTB_NUM;

typedef struct HEVC_BITS_BASE_ADDR
{
    volatile unsigned bitstream_base_addr            :    28;    //bit 0~27
    volatile unsigned data_valid                        :    1;    //bit 28
    volatile unsigned data_last                        :    1;    //bit 29
    volatile unsigned data_first                        :    1;  //bit 30
    volatile unsigned r1                                :    1;    //bit 31
}regHEVC_BITS_BASE_ADDR;

typedef struct HEVC_BITS_OFFSET
{
    volatile unsigned bits_offset                :    30;    //bit 0~29
    volatile unsigned r0                            :    2;  //bit 30~31
}regHEVC_BITS_OFFSET;

typedef struct HEVC_BITS_LEN
{
    volatile unsigned bits_len                    :    30;    //bit 0~29
    volatile unsigned r0                        :    2;    //bit 30~31
}regHEVC_BITS_LEN;

typedef struct HEVC_BITS_END_ADDR
{
    volatile unsigned bitstream_end_addr            :    30;    //bit 2~31
    volatile unsigned bitstream_end_addr_hard_wired :   2;  //bit 0~1
}regHEVC_BITS_END_ADDR;

//////////////////////////
typedef struct HEVC_EXTRA_CTRL
{
    volatile unsigned rotate_angle                    :    3;
    //bit 0~2
    volatile unsigned r0                            :    5;
    //bit 3~7
    volatile unsigned scale_precision                :    4;
    //bit 8~11
    volatile unsigned field_scale_mod                :    1;
    //bit 12 0: both field; 1: only one field
    volatile unsigned bottom_field_sel                :    1;
    //bit 13 0: top; 1: bottom
    volatile unsigned r1                            :    18;
    //bit 14~31
}regHEVC_EXTRA_CTRL;

typedef struct HEVC_REC_FRM_BUF_IDX
{
    volatile unsigned frm_buf_idx                    :    5;    //bit 0~4
    volatile unsigned r0                            :    27;    //bit 05~31
}regHEVC_RecFrmBufIdx;

typedef struct HEVC_BUF_ADDR
{
    volatile unsigned addr                            :    32;    //bit 0~31
}regHEVC_BUF_ADDR;

typedef struct HEVC_ERROR_CASE
{
    volatile unsigned no_more_data_error            :    1;    //bit 0
    volatile unsigned mbh_error                        :    1;    //bit 1
    volatile unsigned ref_idx_error                    :    1;    //bit 2
    volatile unsigned block_error                    :    1;    //bit 3
    volatile unsigned r0                            :    28;    //bit 28~31
}regHEVC_ERROR_CASE;

typedef struct HEVC_8BIT_ADDR
{
    volatile unsigned entry_point_addr                :   8; //bit 0:7
    volatile unsigned r0                            :   8; //bit 8:15
    volatile unsigned sd_low8_chroma_addr            :   8; //bit 16:23
    volatile unsigned pri_low8_chroma_addr          :   8; //bit 24:31
}regHEVC_8BIT_ADDR;

//for vld by pass struct
typedef struct VLD_BYPASS_WORD0
{
    volatile unsigned int sao_merge_left_flag            :   1; //bit 0
    volatile unsigned int sao_merge_up_flag              :   1; //bit 1
    volatile unsigned int sao_type_idx_luma              :   2; //bit 3:2
    volatile unsigned int sao_type_idx_chroma            :   2; //bit 5:4
    volatile unsigned int sao_offset_abs_y1              :   3; //bit 8:6
    volatile unsigned int sao_offset_abs_y2              :   3; //bit 11:9
    volatile unsigned int sao_offset_abs_y3              :   3; //bit 14:12
    volatile unsigned int sao_offset_abs_y4              :   3; //bit 17:15
    volatile unsigned int sao_offset_abs_u1              :   3; //bit 20:18
    volatile unsigned int sao_offset_abs_u2              :   3; //bit 23:21
    volatile unsigned int sao_offset_abs_u3              :   3; //bit 26:24
    volatile unsigned int sao_offset_abs_u4              :   3; //bit 29:27
    volatile unsigned int r0                             :   1; //bit 30
    volatile unsigned int last_ctu_flag                     :   1; //bit 31
}regVLD_BYPASS_WORD0;

typedef struct VLD_BYPASS_WORD1
{
    volatile unsigned int sao_offset_abs_v1              :   3; //bit 2:0
    volatile unsigned int sao_offset_abs_v2              :   3; //bit 5:3
    volatile unsigned int sao_offset_abs_v3              :   3; //bit 8:6
    volatile unsigned int sao_offset_abs_v4                 :   3; //bit 11:9
    volatile unsigned int sao_band_position_y            :   5; //bit 16:12
    volatile unsigned int sao_band_position_u            :   5; //bit 21:17
    volatile unsigned int sao_band_position_v            :   5; //bit 26:22
    volatile unsigned int sao_offset_sign_y1             :   1; //bit 27
    volatile unsigned int sao_offset_sign_y2             :   1; //bit 28
    volatile unsigned int sao_offset_sign_y3             :   1; //bit 29
    volatile unsigned int sao_offset_sign_y4             :   1; //bit 30
    volatile unsigned int sao_offset_sign_u1             :   1; //bit 31
}regVLD_BYPASS_WORD1;

typedef struct VLD_BYPASS_WORD2
{
    volatile unsigned int sao_offset_sign_u2              :   1; //bit 0
    volatile unsigned int sao_offset_sign_u3              :   1; //bit 1
    volatile unsigned int sao_offset_sign_u4              :   1; //bit 2
    volatile unsigned int sao_offset_sign_v1               :   1; //bit 3
    volatile unsigned int sao_offset_sign_v2              :   1; //bit 4
    volatile unsigned int sao_offset_sign_v3              :   1; //bit 5
    volatile unsigned int sao_offset_sign_v4              :   1; //bit 6
    volatile unsigned int sao_eo_class_luma               :   2; //bit 8:7
    volatile unsigned int sao_eo_class_chroma             :   2; //bit 10:9
    volatile unsigned int split_cu_flag_0                 :   1; //bit 11
    volatile unsigned int split_cu_flag_1                 :   4; //bit 15:12
    volatile unsigned int split_cu_flag_2                 :  16; //bit 31:16
}regVLD_BYPASS_WORD2;

typedef struct VLD_BYPASS_WORD3
{
    volatile unsigned int pu0_merge_flag                     :   1;//bit 0
    volatile unsigned int pu1_merge_flag                    :   1;//bit 1
    volatile unsigned int pu2_merge_flag                    :   1;//bit 2
    volatile unsigned int pu3_merge_flag                    :   1;//bit 3
    volatile unsigned int pcm_flag                            :   1;//bit 4
    volatile unsigned int cu_transquant_bypass_flag          :   1;//bit 5
    volatile unsigned int cu_skip_flag                        :   1;//bit 6
    volatile unsigned int part_mode                            :   3;//bit 9:7
    volatile unsigned int pred_mode_flag                    :   1;//bit 10
    volatile unsigned int r0                                 :   5;//bit 15:11
}regVLD_BYPASS_WORD3;

typedef struct VLD_BYPASS_WORD4
{
    volatile unsigned int split_transform_flag0          :   1;//bit 0
    volatile unsigned int r0                             :   3;//bit 3:1
    volatile unsigned int split_transform_flag1          :   4;//bit 7:4
    volatile unsigned int r1                             :   8;//bit 15:8
    volatile unsigned int split_transform_flga2          :  16;//bit 31:16
}regVLD_BYPASS_WORD4;

typedef union VLD_BYPASS_CB1
{
    struct CB_MERGE
    {
        volatile unsigned int pu0_merge_idx              :   3;//bit2:0
        volatile unsigned int r0                         :  13;//bit 15:3
        volatile unsigned int pu1_merge_idx              :   3;//bit 18:16
        volatile unsigned int r1                         :  13;//bit 31:19
    }cb_merge;
    struct CB_INTER
    {
        volatile unsigned int pu0_sign_mvdl0_x           :   1;//bit 0
        volatile unsigned int pu0_sign_mvdl0_y           :   1;//bit 1
        volatile unsigned int pu0_sign_mvdl1_x           :   1;//bit 2
        volatile unsigned int pu0_sign_mvdl1_y           :   1;//bit 3
        volatile unsigned int pu0_ref_idx_l0             :   4;//bit 7:4
        volatile unsigned int pu0_mvp_l0_flag            :   1;//bit 8
        volatile unsigned int pu0_ref_idx_l1             :   4;//bit 12:9
        volatile unsigned int pu0_mvp_l1_flag            :   1;//bit 13
        volatile unsigned int pu0_inter_pred_idc         :   2;//bit 15:14
        volatile unsigned int pu1_sign_mvdl0_x           :   1;//bit 16
        volatile unsigned int pu1_sign_mvdl0_y           :   1;//bit 17
        volatile unsigned int pu1_sign_mvdl1_x           :   1;//bit 18
        volatile unsigned int pu1_sign_mvdl1_y           :   1;//bit 19
        volatile unsigned int pu1_ref_idx_l0             :   4;//bit 23:20
        volatile unsigned int pu1_mvp_l0_flag            :   1;//bit 24
        volatile unsigned int pu1_ref_idx_l1             :   4;//bit 28:25
        volatile unsigned int pu1_mvp_l1_flag            :   1;//bit 29
        volatile unsigned int pu1_inter_pred_idc         :   2;//bit 31:30
    }cb_inter;
    struct CB_INTRA
    {
        volatile unsigned int intrapredmode_y0           :   6;//bit 5:0
        volatile unsigned int intrapredmode_y1           :   6;//bit 11:6
        volatile unsigned int intrapredmode_y2           :   6;//bit 17:12
        volatile unsigned int intrapredmode_y3           :   6;//bit 23:18
        volatile unsigned int intrapredmode_c             :   6;//bit 29:24
        volatile unsigned int r0                         :   2;//bit 31:30
    }cb_intra;
}regVLD_BYPASS_CB1;

typedef union VLD_BYPASS_CB2
{
    struct CB_INTER1
    {
        volatile unsigned int pu0_abs_mvdl0_x            :  16;//bit 15:0
        volatile unsigned int pu0_abs_mvdl0_y            :  16;//bit 31:16
    }cb_inter;
}regVLD_BYPASS_CB2;

typedef union VLD_BYPASS_CB3
{
    struct CB_INTER2
    {
        volatile unsigned int pu0_abs_mvdl1_x            :  16;//bit 15:0
        volatile unsigned int pu0_abs_mvdl1_y            :  16;//bit 31:16
    }cb_inter;
}regVLD_BYPASS_CB3;

typedef struct VLD_BYPASS_CB4
{
    volatile unsigned int pu1_abs_mvdl0_x                :  16;//bit 15:0
    volatile unsigned int pu1_abs_mvdl0_y                :  16;//bit 31:16
}regVLD_BYPASS_CB4;

typedef struct VLD_BYPASS_CB5
{
    volatile unsigned int pu1_abs_mvdl1_x                :  16;//bit 15:0
    volatile unsigned int pu1_abs_mvdl1_y                :  16;//bit 31:16
}regVLD_BYPASS_CB5;

typedef union VLD_BYPASS_CB6
{
    struct CB_MERGE1
    {
        volatile unsigned int pu2_merge_idx              :   3;//bit2:0
        volatile unsigned int r0                         :  13;//bit 15:3
        volatile unsigned int pu3_merge_idx              :   3;//bit 18:16
        volatile unsigned int r1                         :  13;//bit 31:19
    }cb_merge;
    struct CB_INTER3
    {
        volatile unsigned int pu2_sign_mvdl0_x           :   1;//bit 0
        volatile unsigned int pu2_sign_mvdl0_y           :   1;//bit 1
        volatile unsigned int pu2_sign_mvdl1_x           :   1;//bit 2
        volatile unsigned int pu2_sign_mvdl1_y           :   1;//bit 3
        volatile unsigned int pu2_ref_idx_l0             :   4;//bit 7:4
        volatile unsigned int pu2_mvp_l0_flag            :   1;//bit 8
        volatile unsigned int pu2_ref_idx_l1             :   4;//bit 12:9
        volatile unsigned int pu2_mvp_l1_flag            :   1;//bit 13
        volatile unsigned int pu2_inter_pred_idc         :   2;//bit 15:14
        volatile unsigned int pu3_sign_mvdl0_x           :   1;//bit 16
        volatile unsigned int pu3_sign_mvdl0_y           :   1;//bit 17
        volatile unsigned int pu3_sign_mvdl1_x           :   1;//bit 18
        volatile unsigned int pu3_sign_mvdl1_y           :   1;//bit 19
        volatile unsigned int pu3_ref_idx_l0             :   4;//bit 23:20
        volatile unsigned int pu3_mvp_l0_flag            :   1;//bit 24
        volatile unsigned int pu3_ref_idx_l1             :   4;//bit 28:25
        volatile unsigned int pu3_mvp_l1_flag            :   1;//bit 29
        volatile unsigned int pu3_inter_pred_idc         :   2;//bit 31:30
    }cb_inter;
}regVLD_BYPASS_CB6;

typedef struct VLD_BYPASS_CB7
{
    volatile unsigned short pu2_abs_mvdl0_x;//bit 15:0
    volatile unsigned short pu2_abs_mvdl0_y;//bit 31:16
}regVLD_BYPASS_CB7;

typedef struct VLD_BYPASS_CB8
{
    volatile unsigned short pu2_abs_mvdl1_x;//bit 15:0
    volatile unsigned short pu2_abs_mvdl1_y;//bit 31:16
}regVLD_BYPASS_CB8;

typedef struct VLD_BYPASS_CB9
{
    volatile unsigned short pu3_abs_mvdl0_x;//bit 15:0
    volatile unsigned short pu3_abs_mvdl0_y;//bit 31:16
}regVLD_BYPASS_CB9;

typedef struct VLD_BYPASS_CB10
{
    volatile unsigned short pu3_abs_mvdl1_x;//bit 15:0
    volatile unsigned short pu3_abs_mvdl1_y;//bit 31:16
}regVLD_BYPASS_CB10;

static inline u32 HevcDecRu32Software( char *name)
{
    if(HEVC_SOFTWARE_REGISTER_SHOW)
        logd(" read regitster:\t%s ", name);
    return 0;
}

static inline void HevcDecWu32Software(s32 value, char *name)
{
    if(HEVC_SOFTWARE_REGISTER_SHOW)
        logd(" write regitster:\t%s, \t\t 0x%8.8x", name, value);
}

static inline void HevcDecWu8Software( s32 value, char *name)
{
    if(HEVC_SOFTWARE_REGISTER_SHOW)
        logd(" write u8 regitster:\t%s, \t\t 0x%8.8x", name, value);
}

#define SYS_WriteByte(uAddr, bVal) \
        do{*(volatile u8 *)(uAddr) = (bVal);}while(0)
#define SYS_WriteWord(uAddr, wVal) \
        do{*(volatile u16 *)(uAddr) = (wVal);}while(0)
#define SYS_WriteDWord(uAddr, dwVal) \
        do{*(volatile u32 *)(uAddr) = (dwVal);}while(0)

#define SYS_ReadByte(uAddr) \
        bVal = (*(volatile u8 *)(uAddr));

#define SYS_ReadWord(uAddr) \
        wVal = (*(volatile u16 *)(uAddr));

#define SYS_ReadDWord(uAddr) \
        (*(volatile u32 *)(uAddr));

#define  HevcZeroReg(reg)        (*((u32*)(&reg)) = 0)

#define SYS_ReadDWordDebug(Addr) SYS_ReadDWord(Addr)
#define SYS_WriteDWordDebug(Addr,Value,name)  \
    do{                                            \
    logd("val: %8.8x, reg: %s ", Value, name);  \
    SYS_WriteDWord(Addr,Value);                    \
    }while(0);

static inline u32 HevcDecRu32Debug(size_addr Addr, char *name)
{
    volatile u32 nTemp = SYS_ReadDWord(Addr);
    logd("read val = %x, reg: %s", nTemp, name);
    return nTemp;
}

#if HEVC_SOFTWARE_REGISTER
#define    HevcDecRu32(Addr,name)            HevcDecRu32Software(Addr,name)
#define    HevcDecWu32(Addr,Value,name)    HevcDecWu32Software(Addr,Value,name)
#define    HevcDecWu8(Addr,Value,name)        HevcDecWu8Software(Addr,Value,name)
#elif HEVC_PRINT_REGISTER
#define    HevcDecRu32(Addr,name)            HevcDecRu32Debug(Addr,name)
#define    HevcDecWu32(Addr,Value,name)    SYS_WriteDWordDebug(Addr,Value,name)
#define    HevcDecWu8(Addr,Value,name)        SYS_WriteByte(Addr,Value)
#else
#define    HevcDecRu32(Addr,name)            SYS_ReadDWord(Addr)
#define    HevcDecWu32(Addr,Value,name)    SYS_WriteDWord(Addr,Value)
#define    HevcDecWu8(Addr,Value,name)        SYS_WriteByte(Addr,Value)
#endif

#define HevcSramWriteAddr(BaseAddr,addr,name)    \
HevcDecWu32((HEVC_SRAM_PORT_ADDR_REG + BaseAddr),addr,name)
#define HevcSramWriteData(BaseAddr,data,name)   \
HevcDecWu32((HEVC_SRAM_PORT_DATA_REG + BaseAddr),data,name)

#define  REG_SET_VALUE(reg, value)        (*((u32*)(&reg)) = (value))
#if 1 // A80 = 0 1673 = 1

#define  Write_U32(uAddr,dwVal)           {  \
                     *(volatile u32 *)(uAddr) = *((volatile u32*)(&dwVal));  \
                     logd("   uAddr = %08x   value = %08x", uAddr, dwVal); }

#define  Read_U32(uAddr)                  *(volatile u32 *)(uAddr)
#else

#define  Write_U32(uAddr,dwVal)           {  \
                        LOGD("   uAddr = %08x   value = %08x", uAddr, dwVal); }

#define  Read_U32(uAddr)                  0

#endif

#if 0
#define  Write_U16(uAddr,wVal)            *(volatile u16 *)(uAddr) = (wVal)
#define  Read_U16(uAddr)                  *(volatile u16 *)(uAddr)

#define  Reg_Read_U32(base, offset)              Read_U32((base)+(offset))
#define  Reg_Write_U32(base, offset, value)      Write_U32((base)+(offset), value)

#define  Reg_Read_U16(base, offset)              Read_U16((base)+(offset))
#define  Reg_Write_U16(base, offset, value)      Write_U16((base)+(offset), value)
#endif

#define  HEVC_REG_WRITE_U32(hevc_offset, value)  \
Write_U32(((hevc_offset) + HEVC_REG_BASE),   value)
#define  HEVC_REG_READ_U32(hevc_offset)           \
Read_U32((hevc_offset) + HEVC_REG_BASE)
#define  TOP_REG_WRITE_U32(top_offset, value)     \
Write_U32(((top_offset ) + VECORE_TOP_ADDR), value)
#define  TOP_REG_READ_U32(top_offset)            \
Read_U32((top_offset ) + VECORE_TOP_ADDR)

#define  SRAM_WRITE_ADDR_U32(addr)                \
Write_U32((HEVC_SRAM_PORT_ADDR_REG + HEVC_REG_BASE), addr)
#define  SRAM_WRITE_DATA_U32(data)                \
Write_U32((HEVC_SRAM_PORT_DATA_REG + HEVC_REG_BASE), data)

#define  PRINT_REG(reg)    LOGD("(reg_value)= %x ", *((volatile u32*)(&(reg))))
///////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif /* H265_REGISTER_H */
