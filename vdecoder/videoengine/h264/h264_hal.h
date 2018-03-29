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

#ifndef H264_V2_HAL_H
#define H264_V2_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

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
    dwVal = (*(volatile u32 *)(uAddr));

#define    vdec_readuint32(offset)            SYS_ReadDWord(offset)
#define    vdec_writeuint32(offset,value)    SYS_WriteDWord(offset,value)
#define    vdec_writeuint8(offset,value)    SYS_WriteByte(offset,value)

#define  MACC_VE_VERSION                                0xF0

#define STARTCODE_DETECT_E          (1<<25)
#define EPTB_DETECTION_BY_PASS      (1<<24)
#define HISTOGRAM_OUTPUT_E          (1<<13)
#define MCRI_CACHE_E                (1<<10)
#define WRITE_SCALE_ROTATED_PIC     (1<<9)
#define NOT_WRITE_RECONS_PIC        (1<<8)
#define VLD_DATA_REQ_IE             (1<<2)
#define DEC_ERROR_IE                (1<<1)
#define SLICE_DEC_END_IE            (1)

#define N_BITS(n)                   (n<<8)
#define SHOW_BITS                   1
#define GET_BITS                    2
#define FLUSH_BITS                  3
#define GET_VLCSE                   4
#define GET_VLCUE                   5
#define SYNC_BYTE                   6
#define INIT_SWDEC                  7
#define DECODE_SLICE                8
#define AVS_PIC                     9
#define START_CODE_DETECT           12
#define START_CODE_TERMINATE        13

#define START_CODE_DETECTED     (1<<31)
#define START_CODE_TYPE         (7<<28)
#define STCD_BUSY               (1<<27)
#define SYNC_IDLE               (1<<24)
#define BS_DMA_Busy             (1<<22)
#define MORE_DATA_FLAG          (1<<16)
#define DBLK_BUSY               (1<<15)
#define IREC_BUSY               (1<<14)
#define INTRA_PRED_BUSY         (1<<13)
#define MCRI_BUSY               (1<<12)
#define IQIT_BUSY               (1<<11)
#define MVP_BUSY                (1<<10)
#define IS_BUSY                    (1<<9)
#define VLD_BUSY                (1<<8)
#define OVER_TIME_I             (1<<3)
#define VLD_DATA_REQ_I          (1<<2)
#define ERROR_I                 (1<<1)
#define SLICE_END_I             (1)

#define REG_SPS                             0x00
#define REG_PPS                             0x04
#define REG_SHS                             0x08
#define REG_SHS2                            0x0c
#define REG_SHS_WP                          0x10
#define REG_SHS_QP                          0x1c
#define REG_FUNC_CTRL                       0x20
#define REG_TRIGGER_TYPE                    0x24
#define REG_FUNC_STATUS                     0x28
#define REG_CUR_MB_NUM                      0x2c
#define REG_VLD_BITSTREAM_ADDR              0x30
#define REG_VLD_OFFSET                      0x34
#define REG_VLD_BIT_LENGTH                  0x38
#define REG_VLD_END_ADDR                    0x3c
#define REG_SD_ROTATE_CTRL                  0x40
#define REG_SD_ROTATE_LUMA_BUF_ADDR         0x44
#define REG_SD_ROTATE_CHROMA_BUF_ADDR       0x48
#define REG_SHS_RECONSTRUCT_FRM_BUF_INDEX   0x4c
#define REG_MB_FIELD_INTRA_INFO_ADDR        0x50
#define REG_MB_NEIGHBOR_INFO_ADDR           0x54
#define REG_MB_ADDR                         0x60
#define REG_MB_NEIGHBOR1                    0x64
#define REG_MB_NEIGHBOR2                    0x68
#define REG_MB_NEIGHBOR3                    0x6c
#define REG_MB_NEIGHBOR4                    0x70
#define REG_MB_NEIGHBOR5                    0x74
#define REG_MB_NEIGHBOR6                    0x78
#define REG_MB_NEIGHBOR7                    0x7c
#define REG_MB_MVBS_INFO                    0x80
#define REG_MB_CUR_INFO                     0x84
#define REG_MB_CBP_CUR                      0x8c
#define REG_MB_QP_CUR                       0x90
#define REG_MB_DISTSCALE_CUR1               0x94
#define REG_MB_DISTSCALE_CUR2               0x98
#define REG_CUR_MB_MV_L0                    0x9c
#define REG_MB_MV_L1                        0xa0
#define REG_CUE_MB_INTRA_PRED_MODE1         0xa4
#define REG_CUR_MB_INTRA_PRED_MODE2         0xa8
#define REG_ERROR_CASE                      0xb8
#define REG_SLICE_QUEUE_CMD_ADDR            0xc0
#define REG_SLICE_QUEUE_LENGTH              0xc4
#define REG_SLICE_QUEUE_START               0xc8
#define REG_SLICE_QUEUE_STATUS              0xcc
#define REG_BASIC_BITS_RETURN_DATA          0xdc
#define REG_STCD_OFFSET                     0xf0
#define REG_SRAM_PORT_RW_OFFSET             0xe0
#define REG_SRAM_PORT_RW_DATA               0xe4

//0x00 AVC_SPS
typedef struct AVC_SPS
{
    volatile unsigned pic_height_mb_minus1          : 8;    //lsb    PIC_HEIGHT_IN_MAP_UNITS_MINUS1
    volatile unsigned pic_width_mb_minus1           : 8;    // |     PIC_WIDTH_IN_MAP_UNITS_MINUS1
    volatile unsigned direct_8x8_inference_flag     : 1;    // |    DIRECT_8X8_INFERENCE_FLAG
    volatile unsigned mb_adaptive_frame_field_flag  : 1;    // |    MB_ADAPTIVE_FRAME_FIELD_FLAG
    volatile unsigned frame_mbs_only_flag           : 1;    // |    FRAME_MBS_ONLY_FLAG
    volatile unsigned chroma_format_idc             : 3;    // |    CHROMA_FORMAT_IDC
    volatile unsigned r2                            : 10;    //msb  Reserved
}reg_avc_pSps;

// 0x04 AVC_PPS
typedef struct AVC_PPS
{
    volatile unsigned transform_8x8_mode_flag           : 1;
    //lsb    TRANSFROM_8X8_MODE_FLAG
    volatile unsigned constrained_intra_pred_flag       : 1;
    // |    CONSTRAINED_INTRA_PRED_FLAG
    volatile unsigned weighted_bipred_idc               : 2;
    // |     WEIGHTED_BIPRED_IDC
    volatile unsigned weighted_pred_idc                 : 1;
    // |    WEIGHTED_PRED_FLAG
    volatile unsigned num_ref_idx_l1_active_minus1_pic  : 5;
    // |    NUM_REF_IDX_L0_ACTIVE_MINUS1_PIC
    volatile unsigned num_ref_idx_l0_active_minus1_pic  : 5;
    // |    NUM_REF_IDX_L1_ACTIVE_MINUS1_PIC
    volatile unsigned entropy_coding_mode_flag          : 1;
    // |    ENTROPY_CODING_MODE_FLAG
    volatile unsigned r0                                : 16;
    //msb  Reserved
}reg_avc_pPps;

//0x08 AVC_SHS
typedef struct AVC_SHS
{
    volatile unsigned bCabac_init_idc               : 2;
    //lsb    CABAC_INIT_IDC
    volatile unsigned direct_spatial_mv_pred_flag   : 1;
    // |    DIRECT_SPATIAL_MV_PRED_FLAG
    volatile unsigned bottom_field_flag             : 1;
    // |     BOTTOM_FIELD_FLAG
    volatile unsigned field_pic_flag                : 1;
    // |    FIELD_PIC_FLAG
    volatile unsigned first_slice_in_pic            : 1;
    // |    FIRST_SLICE_IN_PIC
    volatile unsigned r0                            : 2;
    // |    Reserved
    volatile unsigned slice_type                    : 4;
    // |    NAL_REF_FLAG
    volatile unsigned nal_ref_flag                  : 1;
    // |    NAL_REF_FLAG
    volatile unsigned r1                            : 3;
    // |    Reserved
    volatile unsigned first_mb_y                    : 8;
    // |    FIRST_MB_Y
    volatile unsigned first_mb_x                    : 8;
    //msb    FIRST_MB_x
}reg_shs;

//0x0c AVC_SHS2
typedef struct AVC_SHS2
{
    volatile unsigned slice_beta_offset_div2                : 4;
    //lsb    SLICE_BETA_OFFSET_DIV2
    volatile unsigned slice_alpha_c0_offset_div2            : 4;
    // |    SLICE_ALPHA_C0_OFFSET_DIV2
    volatile unsigned disable_deblocking_filter_idc         : 2;
    // |     DISABLE_DEBLOCKING_FILTER_IDC
    volatile unsigned r0                                    : 2;
    // |    Reserved
    volatile unsigned num_ref_idx_active_override_flag      : 1;
    // |    NUM_REF_IDX_ACTIVE_OVERRIDE_FLAG
    volatile unsigned r1                                    : 3;
    // |    Reserved
    volatile unsigned num_ref_idx_l1_active_minus1_slice    : 5;
    // |    NUM_REF_IDX_L1_ACTIVE_MINUS1_SLICE
    volatile unsigned r2                                    : 3;
    // |    Reserved
    volatile unsigned num_ref_idx_l0_active_minus1_slice    : 5;
    // |    NUM_REF_IDX_L0_ACTIVE_MINUS1_SLICE
    volatile unsigned r3                                    : 3;
    //msb  Reserved
}reg_shs2;

// 0x10 AVC_SHS_WP
typedef struct AVC_SHS_WP
{
    volatile unsigned luma_log2_weight_denom        : 3;
    //lsb    LUMA_LOG2_WEIGHT_DENOM
    volatile unsigned r0                            : 1;
    // |    Reserved
    volatile unsigned chroma_log2_weight_denom      : 3;
    // |    LUMA_LOG2_WEIGHT_DENOM
    volatile unsigned r1                            : 25;
    //msb    Reserved
}reg_shs_wp;

//0x1c AVC_SHS_QP
typedef struct AVC_SHS_QP
{
    volatile unsigned slice_qpy                     : 6;
    //lsb    SLICE_QPY
    volatile unsigned r0                            : 2;
    // |    Reserved
    volatile unsigned chroma_qp_index_offset        : 6;
    // |    CHROMA_QP_INDEX_OFFSET
    volatile unsigned r1                            : 2;
    // |    Reserved
    volatile unsigned second_chroma_qp_index_offset : 6;
    // |    SECOND_CHROMA_QP_INDEX_OFFSET
    volatile unsigned r2                            : 2;
    // |    Reserved
    volatile unsigned scaling_matix_flat_flag       : 1;
    // |    SCALING_MATRIX_FLAT_FLAG
    volatile unsigned r3                            : 7;
    //msb  Reserved
}reg_shs_qp;

//0x20 function_ctrl
typedef struct AVC_FUNC_CTRL                    // 0x20
{
    volatile  unsigned slice_decode_finish_interrupt_enable     :    1;    //bit 0
    volatile  unsigned decode_error_intrupt_enable              :    1;    //bit 1
    volatile  unsigned vld_data_request_interrupt_enable        :    1;    //bit 2
    volatile  unsigned r0                                       :    5;    //bit 3~7
    volatile  unsigned not_write_recons_pic_flag                :    1;    //bit 8
    volatile  unsigned write_scale_rotated_pic_flag             :    1;    //bit 9
    volatile  unsigned mcri_cache_enable                        :    1;    //bit 10
    volatile  unsigned r1                                       :    13;    //bit 11~23
    volatile  unsigned eptb_detection_by_pass                   :    1;    //bit 24
    volatile  unsigned startcode_detect_enable                  :    1;    //bit 25
    volatile  unsigned AVS_Demulate_Enable                      :    1;    //bit 26
    volatile  unsigned r2                                       :    1;    //bit 27
    volatile  unsigned isAVS                                    :    1;  //bit 28
    volatile  unsigned isVP8                                    :    1;  //bit 29
    volatile  unsigned r3                                       :    2;    //bit 30~31
}reg_function_ctrl;

//0x24 trigger_type
typedef struct AVC_TRIGGER_TYPE
{
    volatile  unsigned trigger_type_pul               :  4;  // bit0~3
    volatile  unsigned stcd_type                      :  2;  // bit4~5
    volatile  unsigned r0                             :  2;  // bit6~7
    volatile  unsigned n_bits                         :  6;  // bit8~13
    volatile  unsigned r1                             :  2;  // bit14~15
    volatile  unsigned bin_lens                       :  3;  // bit16~18
    volatile  unsigned r2                             :  5;  // bit19~23
    volatile  unsigned probability                    :  8;  // bit24~31
}reg_trigger_type;

//0x28 function_status
typedef struct AVC_FUNC_STATUS
{
    volatile  unsigned slice_decode_finish_interrupt : 1;     // bit0
    volatile  unsigned decode_error_interrupt        : 1;     // bit1
    volatile  unsigned vld_data_req_interrupt        : 1;     // bit2
    volatile  unsigned over_time_interrupt           : 1;     // bit3
    volatile  unsigned r0                            : 4;     // bit4~7
    volatile  unsigned vld_busy                      : 1;     // bit8
    volatile  unsigned is_busy                       : 1;     // bit9
    volatile  unsigned mvp_busy                      : 1;     // bit10
    volatile  unsigned iq_it_bust                    : 1;     // bit11
    volatile  unsigned mcri_busy                     : 1;     // bit12
    volatile  unsigned intra_pred_busy               : 1;     // bit13
    volatile  unsigned irec_busy                     : 1;     // bit14
    volatile  unsigned dblk_busy                     : 1;     // bit15
    volatile  unsigned more_data_flag                : 1;     // bit16
    volatile  unsigned vp8_upprob_busy               : 1;     // bit17
    volatile  unsigned vp8_busy                      : 1;     // bit18
    volatile  unsigned r1                            : 1;     // bit19
    volatile  unsigned intram_busy                   : 1;     // bit20
    volatile  unsigned it_busy                       : 1;     // bit21
    volatile  unsigned bs_dma_busy                   : 1;     // bit22
    volatile  unsigned wb_busy                       : 1;     // bit23
    volatile  unsigned avs_busy                      : 1;     // bit24
    volatile  unsigned avs_idct_busy                 : 1;     // bit25
    volatile  unsigned r2                            : 1;     // bit26
    volatile  unsigned stcd_busy                     : 1;     // bit27
    volatile  unsigned startcode_type                : 3;     // bit28~30
    volatile  unsigned startcode_detected            : 1;     // bit31
}reg_function_status;

//0x2c current_mb_num
typedef struct AVC_CURT_MB_NUM
{
    volatile  unsigned cur_mb_number                : 16; // bit0~15
    volatile  unsigned r0                           : 16; // bit16~31
}reg_cur_mb_num;

//0x30 vld_bitstream_addr
typedef struct AVC_VBV_ADDR
{
    volatile  unsigned vbv_base_addr_high4          : 4;  // bit0~3
    volatile  unsigned vbv_base_addr_low24          : 24; // bit4~27
    volatile  unsigned slice_data_valid             : 1;  // bit28
    volatile  unsigned last_slice_data              : 1;  // bit29
    volatile  unsigned first_slice_data             : 1;  // bit30
    volatile  unsigned r0                           : 1;  // bit31
}reg_vld_bitstrem_addr;

//0x34 vld_offset
typedef struct AVC_VBV_DATA_OFFSET
{
    volatile  unsigned  vld_bit_offset          : 30;  // bit0~29
    volatile  unsigned  r0                      : 2;   // bit30~31
}reg_vld_offset;

//0x38 vld_bit_length
typedef struct AVC_VBV_DATA_LEN
{
    volatile  unsigned vld_bit_length              : 29;   // bit0~28
    volatile  unsigned r0                          : 3;    // bit29~31
}reg_vld_bit_length;

//0x3c vld_end_addr
typedef struct AVC_VBV_END_ADDR
{
    volatile  unsigned vbv_byte_end_addr          : 32;
}reg_vld_end_addr;

//0x40 sd_rotate_ctrl
typedef struct AVC_ROTATE_CTRL
{
    volatile  unsigned rot_angle                  : 3; //bit0~2
    volatile  unsigned r0                         : 5; //bit3~7
    volatile  unsigned scale_precision            : 4; //bit8~11
    volatile  unsigned field_scale_mode           : 1; //bit12
    volatile  unsigned bottom_field_sel           : 1; //bit13
    volatile  unsigned r1                         : 18;//bit14~31
}reg_sd_rotate_ctrl;

//0x44 sd_rotate_luma_buf_addr
typedef struct AVC_SD_ROTATE_BUF_ADDR
{
    volatile  unsigned sd_rotate_luma_buf_addr         : 32; // bit0~31
}reg_sd_rotate_buf_addr;

//0x48 sd_rotate_chroma_buf_addr
typedef struct AVC_SD_ROTATE_CHROM_BUF_ADDR
{
    volatile  unsigned sd_rotate_chroma_buf_addr       : 32; // bit0~31
}reg_sd_rotate_chroma_buf_addr;

//0x4c shs_reconstruct_frmbuf_index
typedef struct AVC_SHS_BUF_ADDR
{
    volatile  unsigned cur_reconstruct_frame_buf_index : 5; // bit0~4
    volatile  unsigned r0                              : 27; //bit5~31
}reg_shs_reconstruct_frmbuf_index;

//0x50 mb_filed_intra_info_addr
typedef struct AVC_MB_FIELD_INFO_ADDR
{
    volatile  unsigned mb_field_intra_info_addr        : 32;// bit8~31
}reg_mb_field_intra_info_addr;

//0x54 mb_neighbor_info_addr
typedef struct AVC_MB_NEIGHBOR_INFO_ADDR
{
    volatile  unsigned mb_neighbor_info_addr           : 32;// bit8~31
}reg_mb_neighbor_info_addr;

//0x60 mb_addr
typedef struct AVC_MB_ADDR
{
    volatile  unsigned mb_y                          : 7; //bit0~6
    volatile  unsigned r0                            : 1; //bit7
    volatile  unsigned mb_x                          : 7; //bit8~14
    volatile  unsigned r1                            : 17; //bit15~31
}reg_mb_addr;

//0x64 mb_neighbor1
typedef struct AVC_MB_NEIGHBOUR1
{
    volatile  unsigned r0                                       : 1; //bit0
    volatile  unsigned trans_size_8X8_flag_cur_minus1           : 1; //bit1
    volatile  unsigned trans_size_8X8_flag_b                    : 1; //bit2
    volatile  unsigned trans_size_8X8_flag_b1                   : 1; //bit3
    volatile  unsigned trans_size_8X8_flag_A                    : 1; //bit4
    volatile  unsigned trans_size_8X8_flag_A1                   : 1; //bit5
    volatile  unsigned sp_mb_type_cur_minus1                    : 2; //bit6_7
    volatile  unsigned sp_mb_type_d                             : 2; //bit8~9
    volatile  unsigned sp_mb_type_d1                            : 2; //bit10~11
    volatile  unsigned sp_mb_type_c                             : 2; //bit12~13
    volatile  unsigned sp_mb_type_c1                            : 2; //bit14~15
    volatile  unsigned sp_mb_type_b                             : 2; //bit16~17
    volatile  unsigned sp_mb_type_b1                            : 2; //bit18~19
    volatile  unsigned sp_mb_type_a                             : 2; //bit20~21
    volatile  unsigned sp_mb_type_a1                            : 2; //bit22~23
    volatile  unsigned mb_field_flag_d                          : 1; //bit24
    volatile  unsigned mb_field_flag_c                          : 1; //bit25
    volatile  unsigned mb_field_flag_b                          : 1; //bit26
    volatile  unsigned mb_field_flag_a                          : 1; //bit27
    volatile  unsigned mb_avalible_d                            : 1; //bit28
    volatile  unsigned mb_avalible_c                            : 1; //bit29
    volatile  unsigned mb_avalible_b                            : 1; //bit30
    volatile  unsigned mb_avalible_a                            : 1; //bit31
}reg_mb_neighbor1;

//0x68 mb_neighbor2
typedef struct AVC_MB_NEIGHBOUR2
{
    volatile  unsigned  cbf_a                                   : 4; //bit0~bit3
    volatile  unsigned  cbf_a1                                  : 4; //bit4~bit7
    volatile  unsigned  cbf_b                                   : 4; //bit8~bit11
    volatile  unsigned  cbf_b1                                  : 4; //bit12~bit15
    volatile  unsigned  cbf_cur_minus1                          : 4; //bit16~bit19
    volatile  unsigned  cbp_a                                   : 2; //bit20~bit21
    volatile  unsigned  cbp_a1                                  : 2; //bit22~bit23
    volatile  unsigned  cbp_b                                   : 2; //bit24~bit25
    volatile  unsigned  cbp_b1                                  : 2; //bit26~bit27
    volatile  unsigned  vbp_cur_minus1                          : 2; //bit28~bit29
    volatile  unsigned  r0                                      : 2; //bit30~bit31
}reg_mb_neighbor2;

//ox6c mb_neighbor3
typedef struct AVC_MB_NEIGHBOUR3
{
    volatile  unsigned qpy_a                                   : 6; //bit0~bit5
    volatile  unsigned r0                                      : 2; //bit6~bit7
    volatile  unsigned qpcb_a                                  : 6; //bit8~bit13
    volatile  unsigned r1                                      : 2; //bit14~bit15
    volatile  unsigned qpcr_a                                  : 6; //bit16~bit21
    volatile  unsigned r2                                      : 10; //bit22~bit31
}reg_mb_neighbor3;

//0x70 mb_neighbor4
typedef struct AVC_MB_NEIGHBOUR4
{
    volatile  unsigned qpy_a1                                  : 6; //bit0~bit5
    volatile  unsigned r0                                      : 2; //bit6~bit7
    volatile  unsigned qpcb_a1                                 : 6; //bit8~bit13
    volatile  unsigned r1                                      : 2; //bit14~bit15
    volatile  unsigned qpcr_a1                                 : 6; //bit16~bit21
    volatile  unsigned r2                                      : 10; //bit22~bit31
}reg_mb_neighbor4;

//0x74 mb_neighbor5
typedef struct AVC_MB_NEIGHBOUR5
{
    volatile  unsigned qpy_b                                   : 6; //bit0~bit5
    volatile  unsigned r0                                      : 2; //bit6~bit7
    volatile  unsigned qpcb_b                                  : 6; //bit8~bit13
    volatile  unsigned r1                                      : 2; //bit14~bit15
    volatile  unsigned qpcr_b                                  : 6; //bit16~bit21
    volatile  unsigned r2                                      : 10; //bit22~bit31
}reg_mb_neighbor5;

//0x78 mb_neighbor6
typedef struct AVC_MB_NEIGHBOUR6
{
    volatile  unsigned qpy_b1                                  : 6; //bit0~bit5
    volatile  unsigned r0                                      : 2; //bit6~bit7
    volatile  unsigned qpcb_b                                  : 6; //bit8~bit13
    volatile  unsigned r1                                      : 2; //bit14~bit15
    volatile  unsigned qpcr_b1                                 : 6; //bit16~bit21
    volatile  unsigned r2                                      : 10; //bit22~bit31
}reg_mb_neighbor6;

//0x7c mb_neightbor7
typedef struct AVC_MB_NEIGHBOUR7
{
    volatile  unsigned qpy_minus1                              : 6; //bit0~bit5
    volatile  unsigned r0                                      : 2; //bit6~bit7
    volatile  unsigned qpcb_minus1                             : 6; //bit8~bit13
    volatile  unsigned r1                                      : 2; //bit14~bit15
    volatile  unsigned qp_cr_cur_minus1                        : 6; //bit16~bit21
    volatile  unsigned r2                                      : 10;//bit22~bit31
}reg_mb_neighbor7;

//0x80 mb_mvbs_info
typedef struct AVC_MB_MVBS_INFO
{
    volatile  unsigned mvp2dblk_edge_info                      : 32; //bit0~bit31
}reg_mb_mvbs_info;

//0x84 mb_cur_info
typedef struct AVC_MB_CUR_INFO
{
    volatile unsigned sub_mb_type0                              : 4; //bit0~bit3;
    volatile unsigned sub_mb_type1                              : 4; //bit4~bit7
    volatile unsigned sub_mb_type2                              : 4; //bit8~bit11
    volatile unsigned sub_mb_type3                              : 4; //bit12~bit15
    volatile unsigned inter_mb_type                             : 6; //bit16~bit21
    volatile unsigned mb_direct_8x8_flag                        : 1; //bit22
    volatile unsigned mb_direct_8x8_flag1                       : 1; //bit23
    volatile unsigned mb_direct_8x8_flag2                       : 1; //bit24
    volatile unsigned mb_direct_8x8_flag3                       : 1; //bit25
    volatile unsigned mb_direct_flag                            : 1; //bit26
    volatile unsigned mb_skip_flag                              : 1; //bit27
    volatile unsigned r0                                        : 1; //bit28
    volatile unsigned transform_size_8x8_flag                   : 1; //bit29
    volatile unsigned mb_intra_flag                             : 1; //bit30
    volatile unsigned mb_field_flag                             : 1; //bit31
}reg_mb_inter_cur_info;

typedef struct AVC_MB_INTRA_CUR_INFO
{
    volatile unsigned intra_chroma_pred_mode                   : 2; //bit0~bit1
    volatile unsigned r0                                       : 2; //bit2~bit3
    volatile unsigned intra_16x16_pred_mode                    : 2; //bit4~bit5
    volatile unsigned r1                                       : 10;//bit6~bit15
    volatile unsigned intra_mb_type                            : 2; //bit16~bit17
    volatile unsigned r2                                       : 11; //bit18~bit28
    volatile unsigned transform_size_8x8_flag                  : 1; //bit29
    volatile unsigned mb_intra_flag                            : 1; //bit30
    volatile unsigned mb_field_flag                            : 1; //bit31
}reg_mb_intra_cur_info;

//0x88 reserved
//0x8c mb_cbp_cur
typedef struct AVC_MB_CBP
{
    volatile unsigned r0                                       : 24; //bit0~bit23
    volatile unsigned cbp_cur                                  : 6;  //bit24~bit29
    volatile unsigned r1                                       : 2;  //bit30~bit31
}reg_mb_cbp_cur;
//0x90 mb_qp_cur
typedef struct AVC_MB_QP
{
    volatile unsigned qpy_cur                                  : 6; //bit0~bit5
    volatile unsigned r0                                       : 2; //bit6~bit7
    volatile unsigned qpcb_cur                                 : 6; //bit8~bit13
    volatile unsigned r1                                       : 2; //bit14~bit15
    volatile unsigned qpcr_cur                                 : 6; //bit16~bit21
    volatile unsigned r2                                       : 10; //bit22~bit31
}reg_mb_qp_cur;

//0x94 mb_distscale_cur1
typedef struct AVC_MB_DISTSCALE_CUR1
{
    volatile unsigned distscale_factor0                        : 11; //bit0~bit10
    volatile unsigned r0                                       : 4; //bit11~bit14
    volatile unsigned distscale_factor0_valid_flag             : 1; //bit15
    volatile unsigned distscale_factor1                        : 11;//bit16~bit26
    volatile unsigned r1                                       : 4; //bit27~bit30
    volatile unsigned distscale_factor1_valid_flag             : 1; //bit31
}reg_mb_distscale_cur1;

//0x98 mb_distscale_cur2
typedef struct AVC_MB_DISTSCALE_CUR2
{
    volatile unsigned distscale_factor2                        : 11; //bit0~bit10
    volatile unsigned r0                                       : 4; //bit11~bit14
    volatile unsigned distscale_factor2_valid_flag             : 1; //bit15
    volatile unsigned distscale_factor3                        : 11;//bit16~bit26
    volatile unsigned r1                                       : 4; //bit27~bit30
    volatile unsigned distscale_factor3_valid_flag             : 1; //bit31
}reg_mb_distscale_cur2;
//0x9c cur_mb_mv_L0
typedef struct AVC_MB_MV_L0
{
    volatile unsigned mv_x_l0                                  : 14; //bit0~bit13
    volatile unsigned mv_y_l0                                  : 12; //bit14~bit25
    volatile unsigned ref_idx_l0                               : 6; //bit26~bit31
}reg_cur_mv_mv_l0;

//0xa0 cur_mb_mv_L1
typedef struct AVC_MB_MV_L1
{
    volatile unsigned mv_x_l1                                  : 14; //bit0~bit13
    volatile unsigned mv_y_l1                                  : 12; //bit14~bit25
    volatile unsigned ref_idx_l1                               : 6; //bit26~bit31
}reg_cur_mv_mv_l1;

//0xa4 cur_mb_intra_pred_mode1
typedef struct AVC_MB_INTRA_PRED_MODE1
{
    volatile unsigned block_8x8_4x4_0_mode                     : 4;// bit0~bit3
    volatile unsigned block_8x8_4x4_1_mode                     : 4; //bit4~bit7
    volatile unsigned block_8x8_4x4_2_mode                     : 4; //bit8~bit11
    volatile unsigned block_8x8_4x4_3_mode                     : 4; //bit12~bit15
    volatile unsigned block_4x4_4_mode                         : 4; //bit16~bit19
    volatile unsigned block_4x4_5_mode                         : 4; //bit20~bit23
    volatile unsigned block_4x4_6_mode                         : 4; //bit24~bit27
    volatile unsigned block_4x4_7_mode                         : 4; //bit28~bit31
}reg_cur_mb_intra_pred_mode1;

//0xa8 cur_mb_intra_pred_mode2
typedef struct AVC_MB_INTRA_PRED_MODE2
{
    volatile unsigned block_4x4_8_mode                         : 4; //bit0~bit3
    volatile unsigned block_4x4_9_mode                         : 4; //bit4~bit7
    volatile unsigned block_4x4_10_mode                        : 4; //bit8~bit11
    volatile unsigned block_4x4_11_mode                        : 4; //bit12~bit15
    volatile unsigned block_4x4_12_mode                        : 4; //bit16~bit19
    volatile unsigned block_4x4_13_mode                        : 4; //bit20~bit23
    volatile unsigned block_4x4_14_mode                        : 4; //bit24~bit27
    volatile unsigned block_4x4_15_mode                        : 4; //bit28~bit31
}reg_cur_mb_intra_pred_mode2;

//0xb8 error_case
typedef struct AVC_ERROR_CASE
{
    volatile unsigned no_more_data_error                        : 1; //bit0
    volatile unsigned mbh_error                                 : 1; //bit1
    volatile unsigned ref_idx_error                             : 1; //bit2
    volatile unsigned block_error                               : 1; //bit3
    volatile unsigned r0                                        : 28;
}reg_error_case;

//0xc0 slice_queue_cmd_addr
typedef struct AVC_SLICE_QUEUE_CMD_ADDR
{
    volatile unsigned slice_queue_cmd_addr_28_31               : 4; //bit0~bit3
    volatile unsigned r0                                       : 4; //bit4~bit7
    volatile unsigned slice_queue_cmd_addr_8_27                : 20;//bit8~bit27
    volatile unsigned r1                                       : 2; //bit28~bit29
    volatile unsigned slice_queue_decode_interrupt_enable      : 1; //bit30
    volatile unsigned slice_queue_decode_enable                : 1; //bit31
}reg_slice_queue_cmd_addr;

//0xc4 slice_queue_length
typedef struct AVC_SLICE_QUEUE_LENGTH
{
    volatile unsigned slice_queue_decode_length                : 16; //bit0~bit15
    volatile unsigned slice_queue_decode_offset                : 16; //bit16~bit31
}reg_slice_queue_length;

//0xc8 slice_queue_start
typedef struct AVC_SLICE_QUEUE_START
{
    volatile unsigned one_slice_queue_decode_start              : 1; //bit0
    volatile unsigned r0                                        : 31;//bit1~bit31
}reg_slice_queue_start;

//0xcc slice_queue_status
typedef struct AVC_SLICE_QUEUE_STATUS
{
    volatile unsigned slice_queue_decode_interrupt              : 1; //bit0
    volatile unsigned r0                                        : 23;//bit1~bit23
    volatile unsigned slice_queue_cmd_err                       : 1; //bit24
    volatile unsigned r1                                        : 6; //bit25~bit30
    volatile unsigned slice_queue_busy                          : 1; //bit31
}reg_slice_queue_status;

//0xdc basic_bits_return_data
typedef struct AVC_BASIC_BITS_RETURN_DATA
{
    volatile unsigned basic_bits_data                           : 32; //bit0~bit31
}reg_basic_bits_return_data;

//0xe0 stream_port_rw_offset
typedef struct AVC_STREAM_PORT_RW_OFFSET
{
    volatile unsigned sram_addr                                 : 32; //bit2~bit11
}reg_sram_port_rw_offset;

//0xe4 stream_port_rw_data
typedef struct AVC_STREAM_PORT_RW_DATA
{
    volatile unsigned sram_data                                  : 32;//bit0~bit31
}reg_sram_port_rw_data;

typedef struct AVC_FRAME_STRUCT_REF_INFO
{
    volatile unsigned top_ref_type      :2;
    volatile unsigned r0                :2;
    volatile unsigned bot_ref_type      :2;
    volatile unsigned r1                :2;
    volatile unsigned frm_struct        :2;
    volatile unsigned r2                :22;
}reg_frame_struct_ref_info;

reg_avc_pSps                     avc_sps_reg00;
reg_avc_pPps                     avc_pps_reg04;
reg_shs                          shs_reg08;
reg_shs2                         shs2_reg0c;
reg_shs_wp                       shs_wp_reg10;
reg_shs_qp                       shs_qp_reg1c;
reg_function_ctrl                func_ctrl_reg20;
reg_trigger_type                 trigger_type_reg24;
reg_function_status              func_status_reg28;
reg_cur_mb_num                   cur_mb_num_reg2c;
reg_vld_bitstrem_addr            vld_bitstream_addr_reg30;
reg_vld_offset                   vld_offset_reg34;
reg_vld_bit_length               vld_bit_length_reg38;
reg_vld_end_addr                 vld_end_addr_reg3c;
reg_sd_rotate_ctrl               sd_rotate_ctrl_reg40;
reg_sd_rotate_buf_addr           sd_rotate_buf_addr_reg44;
reg_sd_rotate_chroma_buf_addr    sd_rotate_chroma_buf_addr_reg48;
reg_shs_reconstruct_frmbuf_index shs_recon_frmbuf_index_reg4c;
reg_mb_field_intra_info_addr     mb_field_intra_info_addr_reg50;
reg_mb_neighbor_info_addr        mb_neighbor_info_addr_reg54;
reg_mb_addr                      mb_addr_reg60;
reg_mb_neighbor1                 mb_neighbor1_reg64;
reg_mb_neighbor2                 mb_neighbor2_reg68;
reg_mb_neighbor3                 mb_neighbor3_reg6c;
reg_mb_neighbor4                 mb_neighbor4_reg70;
reg_mb_neighbor5                 mb_neighbor5_reg74;
reg_mb_neighbor6                 mb_neighbor6_reg78;
reg_mb_neighbor7                 mb_neighbor7_reg7c;
reg_mb_mvbs_info                 mb_mvbs_info_reg80;
reg_mb_inter_cur_info            mb_inter_cur_info_reg84;
reg_mb_intra_cur_info            mb_intra_cur_info_reg84;
reg_mb_cbp_cur                   mb_cbp_cur_reg8c;
reg_mb_qp_cur                    mb_qp_cur_reg90;
reg_mb_distscale_cur1            mb_distscale_cur1_reg94;
reg_mb_distscale_cur2            mb_distscale_cur2_reg98;
reg_cur_mv_mv_l0                 cur_mv_mv_l0_reg9c;
reg_cur_mv_mv_l1                 cur_mv_mv_l1_rega0;
reg_cur_mb_intra_pred_mode1      cur_mb_intra_pred_mode1_rega4;
reg_cur_mb_intra_pred_mode2      cur_mb_intra_pred_mode2_rega8;
reg_error_case                   error_case_regb8;
reg_slice_queue_cmd_addr         slice_queue_cmd_addr_regc0;
reg_slice_queue_length           slice_queue_length_regc4;
reg_slice_queue_start            slice_queue_start_regc8;
reg_slice_queue_status           slice_queue_status_regcc;
reg_basic_bits_return_data       basic_bits_return_data_regdc;
reg_sram_port_rw_offset          sram_port_rw_offset_rege0;
reg_sram_port_rw_data            sram_port_rw_data_rege4;
reg_frame_struct_ref_info        frame_struct_ref_info_rege4;

void H264ConfigureAvcRegister( H264DecCtx* h264DecCtx,
                            H264Context* hCtx,
                            u8 eptbDetectEnable, u32 nBitsLen);

u32 H264GetBits(void* param, u32 len);
u32 H264GetBitsSw(void* param, u32 len);
u32 H264ShowBits(void *param, u32 len);
u32 H264ShowBitsSw(void *param, u32 len);
u32 H264GetUeGolomb(void* param);
u32 H264GetUeGolombSw(void* param);
s32 H264GetSeGolomb(void* param);
s32 H264GetSeGolombSw(void* param);
void H264InitGetBitsSw(void* param, u8 *buffer, u32 bit_size);


#ifdef __cplusplus
}
#endif

#endif

