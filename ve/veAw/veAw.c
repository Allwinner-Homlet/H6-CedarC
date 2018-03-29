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
* File : ve.c
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#include "log.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <sys/mman.h>
#include <pthread.h>

#include "veAw.h"
#include "vdecoder.h"
#include "vencoder.h"
#include "../include/veInterface.h"
#include "../include/veAwTopRegDef.h"
#include "CdcUtil.h"

#include <sys/ioctl.h>
#include <assert.h>

#define VE_MODE_SELECT 0x00
#define VE_RESET       0x04
#define JPG_VE_RESET   0x2c

typedef struct VeRegisterGroupMap{
               int  eGroupId;
              char  sGroupName[20];
         size_addr  nGroupRegAddrOffset;
}VeRegisterGroupMap;

static const VeRegisterGroupMap mVeRegisterGroupMap[] = {
{REG_GROUP_VETOP,        "VE/Top Level",   0},
{REG_GROUP_H264_DECODER, "DEC/H264",       0x200},
{REG_GROUP_H265_DECODER, "DEC/H265",       0x500},
};

typedef struct VeContext
{
    uint64_t nIcVeVersion;
    int nJpegPlusDecoderFlag;
    VeConfig mVeConfig;
    int nDdrMode;
    uint32_t nPhyOffset;
    int need_check_idle;             //check idle when interrupt coming
    int need_check_idle_disable_mode;//check idle when disable codec mode. using in ic before aw1708
    int ve_list_num;
    int bDynamicEnableDramHightChannalFlag;

#if defined(CONFIG_VE_IPC_ENABLE)
    uint32_t lock_vdec;
    uint32_t lock_venc;
    uint32_t lock_jdec;
    uint32_t lock_00_reg;
    uint32_t lock_04_reg;
#else
    pthread_mutex_t *locks[3];

    pthread_mutex_t *lock_vdec;
    pthread_mutex_t *lock_venc;
    pthread_mutex_t *lock_jdec;
#endif
}VeContext;

struct VeLockPolicyS
{
    uint64_t id;
    int32_t sole_venc;
    int32_t sole_jdec;
    uint32_t default_freq;
};

static struct VeLockPolicyS policy_list[] =
{
 /* {chip-id,                      sole_venc,   sole_jdec,  default_freq}     */
    {0x3101000012010,   0,         0,   552},         //(H6)
};

static struct cedarv_env_infomation gVeEnvironmentInfo = {
    .nVeRefCount     = 0,
    .nVeDriverFd     = -1,
    .address_macc    = 0,
    .VeMutex         = PTHREAD_MUTEX_INITIALIZER,
    .VeRegisterMutex = PTHREAD_MUTEX_INITIALIZER,
    .decMutex        = PTHREAD_MUTEX_INITIALIZER,
    .encMutex        = PTHREAD_MUTEX_INITIALIZER,
    .jdecMutex       = PTHREAD_MUTEX_INITIALIZER,
    .nNomalEncRefCount = 0,
    .nPerfEncRefCount  = 0,
    .nClkCount  = 0
};

void VeSetDramType(void* p);
void* VeGetGroupRegAddr(void* p, int nGroupId);
void VeDecoderWidthMode(void *p, int nWidth);
static void enableDecoder(VeContext* pVeContext);
static void disableDecoder();

int VeSetSpeed(void* p, unsigned int nSpeedMHz);

/* VLPL: VeLockPolicyList */
static int initVELocks(VeContext* pVeContext)
{
    CC_LOG_ASSERT(pVeContext->nIcVeVersion, "You should know ic version!");

    int i = 0;
    int VLPL_size = sizeof(policy_list)/sizeof(struct VeLockPolicyS);

    for (; i < VLPL_size; i++)
    {
        if (pVeContext->nIcVeVersion == policy_list[i].id)
        {
            pVeContext->ve_list_num = i;
            break;
        }
    }

    CC_LOG_ASSERT(i < VLPL_size, "IC not support. '0x%llx'",
                  (long long unsigned int) pVeContext->nIcVeVersion);

#if defined(CONFIG_VE_IPC_ENABLE)
    pVeContext->lock_vdec = VE_LOCK_VDEC;

    if (policy_list[i].sole_venc)
    {
        pVeContext->lock_venc = VE_LOCK_VENC;
    }
    else
    {
        pVeContext->lock_venc = VE_LOCK_VDEC;
    }

    if (policy_list[i].sole_jdec)
    {
        pVeContext->lock_jdec = VE_LOCK_JDEC;
    }
    else
    {
        pVeContext->lock_jdec = VE_LOCK_ERR;
    }

    //init top reg lock for sole_venc or sole_jdec
    if(policy_list[i].sole_venc || policy_list[i].sole_jdec)
    {
        pVeContext->lock_00_reg = VE_LOCK_00_REG;
        pVeContext->lock_04_reg = VE_LOCK_04_REG;
    }

#else
    pVeContext->locks[0] = &gVeEnvironmentInfo.decMutex;
    pVeContext->locks[1] = &gVeEnvironmentInfo.encMutex;
    pVeContext->locks[2] = &gVeEnvironmentInfo.jdecMutex;

    pVeContext->lock_vdec = pVeContext->locks[0];

    if (policy_list[i].sole_venc)
    {
        pVeContext->lock_venc = pVeContext->locks[1];
    }
    else
    {
        pVeContext->lock_venc = pVeContext->lock_vdec;
    }

    if (policy_list[i].sole_jdec)
    {
        pVeContext->lock_jdec = pVeContext->locks[2];
    }
    else
    {
        pVeContext->lock_jdec = NULL;
    }

#endif

    return 0;
}

static unsigned int readSyncIdle(int mode)
{
    volatile vetop_reg_reset_t* pVeReset;
    volatile unsigned int dwVal;

    pVeReset = (vetop_reg_reset_t*)(gVeEnvironmentInfo.address_macc + VE_RESET);

    switch(mode)
    {
        case VE_NORMAL_MODE:
        {
            dwVal = pVeReset->ve_sync_idle;
            break;
        }
        case VE_DEC_MODE:
        {
            dwVal = pVeReset->ve_sync_idle;
            break;
        }
        default:
        {
            dwVal = pVeReset->ve_sync_idle;
            break;
        }
    }

    return dwVal;
}

static void veInquireIdle(int mode)
{
    unsigned int dwVal;
    int i = 0;

    dwVal = readSyncIdle(mode);
    while(!dwVal)
    {
        i++;

        if(i > 0x400000)
        {
            loge("wait ve mem sync idle bit too long, reset anyway after 10 ms.");
            usleep(10*1000);
            break;
        }

        dwVal = readSyncIdle(mode);
    }
}

static int lockReg00(void* p)
{
    int ret = 0;

#if defined(CONFIG_VE_IPC_ENABLE)
    VeContext* pVeContext;
    pVeContext  = (VeContext*)p;
    if(policy_list[pVeContext->ve_list_num].sole_jdec
    || policy_list[pVeContext->ve_list_num].sole_venc)
    {
        ret = ioctl(gVeEnvironmentInfo.nVeDriverFd,
                        IOCTL_GET_LOCK,
                        (unsigned long)pVeContext->lock_00_reg);
    }
    else
        pthread_mutex_lock(&gVeEnvironmentInfo.VeRegisterMutex);
#else
    pthread_mutex_lock(&gVeEnvironmentInfo.VeRegisterMutex);
#endif

    return ret;
}

static int unLockReg00(void* p)
{
    int ret = 0;

#if defined(CONFIG_VE_IPC_ENABLE)
    VeContext* pVeContext;
    pVeContext  = (VeContext*)p;
    if(policy_list[pVeContext->ve_list_num].sole_jdec
    || policy_list[pVeContext->ve_list_num].sole_venc)
    {
        //for sole_venc or sole_jdec
        ret = ioctl(gVeEnvironmentInfo.nVeDriverFd,
                        IOCTL_RELEASE_LOCK,
                        (unsigned long)pVeContext->lock_00_reg);
    }
    else
        pthread_mutex_unlock(&gVeEnvironmentInfo.VeRegisterMutex);
#else
    pthread_mutex_unlock(&gVeEnvironmentInfo.VeRegisterMutex);
#endif

    return ret;
}

static int lockReg04(void* p)
{
    int ret = 0;

#if defined(CONFIG_VE_IPC_ENABLE)
    VeContext* pVeContext;
    pVeContext  = (VeContext*)p;
    if(policy_list[pVeContext->ve_list_num].sole_jdec
    || policy_list[pVeContext->ve_list_num].sole_venc)
    {
        //for sole_venc or sole_jdec
        ret = ioctl(gVeEnvironmentInfo.nVeDriverFd,
                        IOCTL_GET_LOCK,
                        (unsigned long)pVeContext->lock_04_reg);
    }
    else
        pthread_mutex_lock(&gVeEnvironmentInfo.VeRegisterMutex);
#else
        pthread_mutex_lock(&gVeEnvironmentInfo.VeRegisterMutex);
#endif

    return ret;
}

static int unLockReg04(void* p)
{
    int ret = 0;

#if defined(CONFIG_VE_IPC_ENABLE)
    VeContext* pVeContext;
    pVeContext  = (VeContext*)p;
    if(policy_list[pVeContext->ve_list_num].sole_jdec
    || policy_list[pVeContext->ve_list_num].sole_venc)
    {
        //for sole_venc or sole_jdec
        ret = ioctl(gVeEnvironmentInfo.nVeDriverFd,
                        IOCTL_RELEASE_LOCK,
                        (unsigned long)pVeContext->lock_04_reg);
    }
    else
        pthread_mutex_unlock(&gVeEnvironmentInfo.VeRegisterMutex);

#else
        pthread_mutex_unlock(&gVeEnvironmentInfo.VeRegisterMutex);
#endif

    return ret;
}

static int lockDecoder(void* p)
{
    VeContext* pVeContext;
    pVeContext  = (VeContext*)p;

    int ret;

#if defined(CONFIG_VE_IPC_ENABLE)
    ret = ioctl(gVeEnvironmentInfo.nVeDriverFd, IOCTL_GET_LOCK,
                (unsigned long)pVeContext->lock_vdec);
#else
    ret = pthread_mutex_lock(pVeContext->lock_vdec);
#endif

    return ret;
}

/* Ve Dec Unlock */
static int unLockDecoder(VeContext* pVeContext)
{
    int ret;

#if defined(CONFIG_VE_IPC_ENABLE)
    ret = ioctl(gVeEnvironmentInfo.nVeDriverFd, IOCTL_RELEASE_LOCK,
                (unsigned long)pVeContext->lock_vdec);

#else
    ret = pthread_mutex_unlock(pVeContext->lock_vdec);
#endif
    return ret;
}


static void setTopRegisters(void* p)
{
    VeContext* pVeContext;
    pVeContext  = (VeContext*)p;
    int ret;

    vetop_reglist_t* vetop_reg_list;

    vetop_reg_list = (vetop_reglist_t*)VeGetGroupRegAddr(p,REG_GROUP_VETOP);

#if CONFIG_USE_TIMEOUT_CONTROL

    //* set maximum cycle count for decoding one frame.
    vetop_reg_list->_0c_overtime.overtime_value = CONFIG_TIMEOUT_VALUE;

    #if RV_CONFIG_USE_INTRRUPT
    //* enable timeout interrupt.
    vetop_reg_list->_1c_status.timeout_enable = 1;
    #endif

#else   //* #if CONFIG_USE_TIMEOUT_CONTROL

    //* disable timeout interrupt.
    vetop_reg_list->_1c_status.timeout_enable = 0;

#endif   //* #if CONFIG_USE_TIMEOUT_CONTROL

    vetop_reg_list->_c4_pri_chroma_buf_len.pri_chroma_buf_len = 0;
    vetop_reg_list->_c8_pri_buf_line_stride.pri_luma_line_stride = 0;
    vetop_reg_list->_c8_pri_buf_line_stride.pri_chroma_line_stride = 0;

    vetop_reg_list->_cc_sec_buf_line_stride.sec_luma_line_stride = 0;
    vetop_reg_list->_cc_sec_buf_line_stride.sec_chroma_line_stride = 0;

    vetop_reg_list->_e8_chroma_buf_len0.chroma_align_mode = 0;
    vetop_reg_list->_e8_chroma_buf_len0.luma_align_mode  = 0;
    vetop_reg_list->_e8_chroma_buf_len0.output_data_format = 0;
    vetop_reg_list->_e8_chroma_buf_len0.sdrt_chroma_buf_len = 0;

    vetop_reg_list->_ec_pri_output_format.primary_output_format = 0;
    vetop_reg_list->_ec_pri_output_format.second_special_output_format=0;

    //VeDecoderWidthMode(p->videoStreamInfo.nWidth);
    volatile vetop_reg_mode_sel_t* pVeModeSelect;

    ret = lockReg00((void *)pVeContext);
    if(ret < 0)
        loge("setTopRegisters: get top reg 0x00 lock error, please check\n");

    pVeModeSelect = (vetop_reg_mode_sel_t*)VeGetGroupRegAddr(p,REG_GROUP_VETOP);

    if(pVeContext->mVeConfig.nWidth >= 4096)
    {
        pVeModeSelect->pic_width_more_2048 = 1;
        pVeModeSelect->pic_width_is_4096 = 1;
    }
    else if(pVeContext->mVeConfig.nWidth >= 2048)
    {
        pVeModeSelect->pic_width_more_2048 = 1;
        pVeModeSelect->pic_width_is_4096 = 0;
    }
    else
    {
        pVeModeSelect->pic_width_more_2048 = 0;
        pVeModeSelect->pic_width_is_4096 = 0;
    }
    ret = unLockReg00((void *)pVeContext);
    if(ret < 0)
        loge("setTopRegisters: release top reg 0x00 lock error, please check\n");
}


static void resetDecAndEnc(void* p)
{
    VeContext* pVeContext;
    pVeContext  = (VeContext*)p;

    ioctl(gVeEnvironmentInfo.nVeDriverFd, IOCTL_RESET_VE, 0);
    VeSetDramType(p);
}

static void resetDecoder(void* p)
{
    VeContext* pVeContext;
    pVeContext  = (VeContext*)p;
    {
        //* disable decoder
        disableDecoder(p);

        //* reset decoder
        if(pVeContext->mVeConfig.nResetVeMode == RESET_VE_NORMAL)
        {
            if(policy_list[pVeContext->ve_list_num].sole_venc || \
               policy_list[pVeContext->ve_list_num].sole_jdec )
            {
                volatile vetop_reg_reset_t* pVeReset;
                int ret = lockReg04((void *)pVeContext);
                if(ret < 0)
                    loge("resetDecoder: get top reg 0x04 lock error, please check\n");

                pVeReset = (vetop_reg_reset_t*)(gVeEnvironmentInfo.address_macc + VE_RESET);
                pVeReset->decoder_reset = 1;
                pVeReset->decoder_reset = 0;

                ret = unLockReg04((void *)pVeContext);
                if(ret < 0)
                    loge("resetDecoder: release top reg 0x04 lock error, please check\n");
            }
            else
                resetDecAndEnc(p);
        }

        //* enable decoder
        enableDecoder(pVeContext);

        //* set top registers
        setTopRegisters(p);
    }
}

static int waitInterruptDecoder(VeContext* pVeContext)
{
    int ret;

    ret = ioctl(gVeEnvironmentInfo.nVeDriverFd, IOCTL_WAIT_VE_DE, 1);
    if(ret <= 0)
    {
        logw("wait ve interrupt timeout. ret = %d",ret);
        return -1;  //* wait ve interrupt fail.
    }
    else
    {
        if(pVeContext->need_check_idle)
            veInquireIdle(VE_NORMAL_MODE);
        return 0;
    }
}

static void enableDecoder(VeContext* pVeContext)
{
    volatile vetop_reg_mode_sel_t* pVeModeSelect;
    int ret;

    ret = lockReg00((void *)pVeContext);
    if(ret < 0)
        loge("enableDecoder: get top reg 0x00 lock error, please check\n");

    pVeModeSelect = (vetop_reg_mode_sel_t*)(gVeEnvironmentInfo.address_macc + VE_MODE_SELECT);

    switch(pVeContext->mVeConfig.nFormat)
    {
        case VIDEO_CODEC_FORMAT_H264:
            pVeModeSelect->mode = 1;
            break;
        case VIDEO_CODEC_FORMAT_H265:
            pVeModeSelect->mode = 4;
            break;
        default:
            pVeModeSelect->mode = 0;
            break;
    }

    //* afbc just support h265 now
    if(pVeContext->mVeConfig.nEnableAfbcFlag == 1
       && pVeContext->nIcVeVersion == 0x3101000012010
       && pVeContext->mVeConfig.nFormat == VIDEO_CODEC_FORMAT_H265)
    {
        pVeModeSelect->reserved0_compress_mode = 1;
        pVeModeSelect->compress_en = 1;

        pVeModeSelect->reserved1_bodybuf_1024_aligned =1;
        pVeModeSelect->rgb_def_color_en = 1;
        pVeModeSelect->min_val_wrap_en = 1;
    }
    ret = unLockReg00((void *)pVeContext);
    if(ret < 0)
        loge("enableDecoder: release top reg 0x00 lock error, please check\n");
}

static void disableDecoder(VeContext* pVeContext)
{
    volatile vetop_reg_mode_sel_t* pVeModeSelect;
    int ret;

    ret = lockReg00((void *)pVeContext);
    if(ret < 0)
        loge("disableDecoder: get top reg 0x00 lock error, please check\n");

    if(pVeContext->need_check_idle_disable_mode)
    {
        veInquireIdle(VE_NORMAL_MODE);
    }

    pVeModeSelect = (vetop_reg_mode_sel_t*)(gVeEnvironmentInfo.address_macc + VE_MODE_SELECT);
    pVeModeSelect->mode = 7;

    ret = unLockReg00((void *)pVeContext);
    if(ret < 0)
        loge("disableDecoder: release top reg 0x00 lock error, please check\n");
}

void getIcVersion(VeContext* pVeContext)
{
    volatile unsigned int value;
    unsigned int nReReadCount = 0;

READ_IC_VERSION:
    value = *((unsigned int*)((char *)gVeEnvironmentInfo.address_macc + 0xf0));
    if(value == 0)
    {
         volatile unsigned int value_enc_ip;
         volatile unsigned int value_dec_ip;
         unsigned long long dec_enc_ip;

         value_dec_ip = *((unsigned int*)((char *)gVeEnvironmentInfo.address_macc + 0xe0));
         logv("the get dec_ip_version:%08x\n", value_dec_ip);
         if(value_dec_ip == 0)
         {
             logw("can not get the ve version ,both 0xf0 and 0xe0 is 0, try to read again\n");
             pVeContext->nIcVeVersion = 0;

             //we should read the ic version again when other process's codec is reseting the ve
             nReReadCount++;
             if(nReReadCount < 4)
             {
                usleep(1*1000);
                goto READ_IC_VERSION;
             }
             else
                return ;
         }

         value_enc_ip = *((unsigned int*)((char *)gVeEnvironmentInfo.address_macc + 0xe4));
         logv("the get enc_ip_version:%08x\n", value_enc_ip);
         if(value_enc_ip == 0)
         {
             logw("can not get the ve version ,both 0xf0 and 0xe4 is 0, try to read again\n");
             pVeContext->nIcVeVersion = 0;

             //we should read the ic version again when other process's codec is reseting the ve
             nReReadCount++;
             if(nReReadCount < 4)
             {
                usleep(1*1000);
                goto READ_IC_VERSION;
             }
             else
                return ;
         }

         dec_enc_ip = ((unsigned long long)value_dec_ip << 32) | ((unsigned long long)value_enc_ip);
         logv("the get enc_dec_ip_version:%llx, gVeEnvironmentInfo.address_macc:%lx\n", \
         dec_enc_ip,gVeEnvironmentInfo.address_macc);

         pVeContext->nIcVeVersion = ((uint64_t)value_dec_ip << 32) | ((uint64_t)value_enc_ip);
    }
    else
    {
         pVeContext->nIcVeVersion = (uint64_t)(value>>16);
    }

    return ;
}

#if CEDARC_DEBUG
void VeWriteValue(void* p, u32 value)
{
    CEDARC_UNUSE(p);
    ioctl(gVeEnvironmentInfo.nVeDriverFd, IOCTL_WRITE_DDR_VALUE, value);
}
u32 VeReadValue(void* p,u32 value)
{
    CEDARC_UNUSE(p);
    u32 ret;
    ret = ioctl(gVeEnvironmentInfo.nVeDriverFd, IOCTL_READ_DDR_VALUE, value);
    return ret;
}
void VeClearnValue(void* p)
{
    CEDARC_UNUSE(p);
    ioctl(gVeEnvironmentInfo.nVeDriverFd, IOCTL_CLEAR_DDR_VALUE, 0);
}
#endif

static int setIcVeFreq(VeContext* pVeContext)
{
    int ret = 0;
    VeContext* p = pVeContext;

    if(p->mVeConfig.nVeFreq != 0 && p->mVeConfig.nVeFreq <= 900)
    {
        //use the freq of user's config
        VeSetSpeed((void *)p, p->mVeConfig.nVeFreq);
    }
    else if(p->nIcVeVersion == 0x3101000012010
        && (p->mVeConfig.nFormat == VIDEO_CODEC_FORMAT_H265
        || p->mVeConfig.nFormat == VIDEO_CODEC_FORMAT_VP9))
    {
        //when 0x1719/0x1728 and the dec_format are h265/vp9 use default freq 648
        VeSetSpeed((void *)p, 648);
    }
    else
    {
        //use default freq
        VeSetSpeed((void *)p, policy_list[p->ve_list_num].default_freq);
    }

    return ret;
}

void* VeInitialize(VeConfig* pVeConfig)
{
    VeContext* p = NULL;
    pthread_mutex_lock(&gVeEnvironmentInfo.VeMutex);

    p = malloc(sizeof(VeContext));
    if(p == NULL)
    {
        loge("malloc for vecontext failed");
        return NULL;
    }
    memset(p, 0, sizeof(VeContext));
    memcpy(&p->mVeConfig, pVeConfig, sizeof(VeConfig));

    if(gVeEnvironmentInfo.nVeRefCount == 0)
    {
        //* open Ve driver.
        gVeEnvironmentInfo.nVeDriverFd = open("/dev/cedar_dev", O_RDWR);
        if(gVeEnvironmentInfo.nVeDriverFd < 0)
        {
            loge("open /dev/cedar_dev fail.");
            pthread_mutex_unlock(&gVeEnvironmentInfo.VeMutex);
            free(p);
            return NULL;
        }

        logv("**********gVeEnvironmentInfo.nVeDriverFd = %d", gVeEnvironmentInfo.nVeDriverFd);
        //* request ve.
        ioctl(gVeEnvironmentInfo.nVeDriverFd, IOCTL_ENGINE_REQ, 0);

        //* map registers.
        cedarv_env_addr_macc mCedarvAddrMacc;
        memset(&mCedarvAddrMacc, 0, sizeof(cedarv_env_addr_macc));

        ioctl(gVeEnvironmentInfo.nVeDriverFd, IOCTL_GET_ENV_INFO,
              (unsigned long)&mCedarvAddrMacc);
        gVeEnvironmentInfo.address_macc = (unsigned long)mmap(NULL,
                                           2048,
                                           PROT_READ | PROT_WRITE, MAP_SHARED,
                                           gVeEnvironmentInfo.nVeDriverFd,
                                           (unsigned long)mCedarvAddrMacc.address_macc);


    }

    getIcVersion(p);
    initVELocks(p);

    //set default ddr_mode
    if(p->nIcVeVersion == 0x3101000012010)
    {
        p->nDdrMode = DDRTYPE_DDR2_32BITS;
    }

    p->nPhyOffset = 0;
    p->need_check_idle = 0;

    //config ve freq
    setIcVeFreq(p);

    gVeEnvironmentInfo.nVeRefCount++;
    pthread_mutex_unlock(&gVeEnvironmentInfo.VeMutex);

    logd("ve init ok\n");
    return (void*)p;
}

void VeRelease(void* p )
{
    VeContext* pVeContext;
    pVeContext  = (VeContext*)p;

    pthread_mutex_lock(&gVeEnvironmentInfo.VeMutex);

    if(gVeEnvironmentInfo.nVeRefCount <= 0)
    {
        loge("invalid status, gVeEnvironmentInfo.nVeRefCount=%d at AdpaterRelease",
             gVeEnvironmentInfo.nVeRefCount);
        pthread_mutex_unlock(&gVeEnvironmentInfo.VeMutex);
        return;
    }

    gVeEnvironmentInfo.nVeRefCount--;

    if(gVeEnvironmentInfo.nVeRefCount == 0)
    {
        {
            if(gVeEnvironmentInfo.nVeDriverFd != -1)
            {
                ioctl(gVeEnvironmentInfo.nVeDriverFd, IOCTL_ENGINE_REL, 0);
                munmap((void *)gVeEnvironmentInfo.address_macc, 2048);
                close(gVeEnvironmentInfo.nVeDriverFd);
                gVeEnvironmentInfo.nVeDriverFd = -1;
                gVeEnvironmentInfo.nClkCount = 0;
            }
        }

    }

    pthread_mutex_unlock(&gVeEnvironmentInfo.VeMutex);

#if defined(CONFIG_VE_IPC_ENABLE)
    logi("not malloc locks\n");
#else
    int j = 0;
    for(j = 0; j < 3; j++)
    {
        if(pVeContext->locks[j] != NULL)
        {
            //free(pVeContext->locks[j]);
            pVeContext->locks[j] = NULL;
        }
    }
#endif

    if(pVeContext)
        free(pVeContext);

    logd("ve release ok\n");
    return;
}

/* Ve Dec Lock */
int VeLock(void* p)
{
    VeContext* pVeContext;
    pVeContext  = (VeContext*)p;
    int ret = -1;

    if(pVeContext->mVeConfig.nDecoderFlag)
    {
        ret = lockDecoder(p);
    }
    return ret;
}

int VeUnLock(void* p)
{
    VeContext* pVeContext;
    pVeContext  = (VeContext*)p;
    int ret = -1;

    if(pVeContext->mVeConfig.nDecoderFlag)
    {
        ret = unLockDecoder(pVeContext);
    }

    return ret;
}

void VeReset(void* p)
{
    VeContext* pVeContext;
    pVeContext  = (VeContext*)p;

   if(pVeContext->mVeConfig.nDecoderFlag)
   {
        resetDecoder(p);
   }
   return ;
}

int VeWaitInterrupt(void* p)
{
    VeContext* pVeContext;
    pVeContext  = (VeContext*)p;
    int ret = -1;

    if(pVeContext->mVeConfig.nDecoderFlag)
    {
        ret = waitInterruptDecoder(pVeContext);
    }

    return ret;
}


void* VeGetGroupRegAddr(void* p, int nGroupId)
{
    CEDARC_UNUSE(p);
    size_addr    nBaseAddr;
    size_addr    nGroupAddr;

    nBaseAddr = (size_addr)gVeEnvironmentInfo.address_macc;
    int i;
    int nGroupSize = sizeof(mVeRegisterGroupMap)/sizeof(VeRegisterGroupMap);
    for(i = 0; i < nGroupSize; i++)
    {
        if(nGroupId == mVeRegisterGroupMap[i].eGroupId)
            break;
    }
    if(i >= nGroupSize)
    {
        loge("match nGroupId and register addr failed, id = %d",nGroupId);
        return NULL;
    }
    nGroupAddr = nBaseAddr + mVeRegisterGroupMap[i].nGroupRegAddrOffset;
    CEDARC_UNUSE(mVeRegisterGroupMap[i].sGroupName);
    return (void*)nGroupAddr;
}

uint64_t VeGetIcVeVersion(void* p)
{
    VeContext* pVeContext;
    pVeContext  = (VeContext*)p;

    return pVeContext->nIcVeVersion;

}

int VeGetDramType(void* p)
{
    VeContext* pVeContext;
    pVeContext  = (VeContext*)p;

    return pVeContext->nDdrMode;
}

uint32_t VeGetPhyOffset(void* p)
{
    VeContext* pVeContext;
    pVeContext  = (VeContext*)p;

    return pVeContext->nPhyOffset;
}

void VeSetDramType(void* p)
{
    volatile vetop_reg_mode_sel_t* pVeModeSelect;
    pthread_mutex_lock(&gVeEnvironmentInfo.VeRegisterMutex);
    pVeModeSelect = (vetop_reg_mode_sel_t*)(gVeEnvironmentInfo.address_macc + VE_MODE_SELECT);
    switch (VeGetDramType(p))
    {
        case DDRTYPE_DDR1_16BITS:
            pVeModeSelect->ddr_mode = 0;
            break;

        case DDRTYPE_DDR1_32BITS:
        case DDRTYPE_DDR2_16BITS:
            pVeModeSelect->ddr_mode = 1;
            break;

        case DDRTYPE_DDR2_32BITS:
        case DDRTYPE_DDR3_16BITS:
            pVeModeSelect->ddr_mode = 2;
            break;

        case DDRTYPE_DDR3_32BITS:
        case DDRTYPE_DDR3_64BITS:
            pVeModeSelect->ddr_mode = 3;
            pVeModeSelect->rec_wr_mode = 1;
            break;

        default:
            break;
    }
    pthread_mutex_unlock(&gVeEnvironmentInfo.VeRegisterMutex);
}

void VeSetDdrMode(void* p, int ddr_mode)
{
    VeContext* pVeContext;
    pVeContext  = (VeContext*)p;

    pVeContext->nDdrMode = ddr_mode;
    if(pVeContext->nDdrMode < DDRTYPE_MIN || pVeContext->nDdrMode > DDRTYPE_MAX)
    {
        loge("the set ddr mode %d is unknow\n", pVeContext->nDdrMode);
        pVeContext->nDdrMode = DDRTYPE_DDR3_32BITS;
    }
}

//Please ensure the codec thread is without the codec lock before calling VeSetSpeed()
int VeSetSpeed(void* p, unsigned int nSpeedMHz)
{
    CEDARC_UNUSE(VeSetSpeed);
    VeContext* pVeContext = (VeContext *)p;
    int ret,i;

    i = pVeContext->ve_list_num;

    lockDecoder(p);

    ret = ioctl(gVeEnvironmentInfo.nVeDriverFd, IOCTL_SET_VE_FREQ, nSpeedMHz);

    unLockDecoder(p);
    logd("*** set ve freq to %d Mhz ***", nSpeedMHz);
    return ret;
}

void VeSetEnableAfbcFlag(void* p, int bEnableFlag)
{
    logd("**** VeSetEnableAfbcFlag: %d",bEnableFlag);
    VeContext* pVeContext;
    pVeContext  = (VeContext*)p;

    pVeContext->mVeConfig.nEnableAfbcFlag = bEnableFlag;
}

void VeSetAdjustDramSpeedFlag(void* p, int nAdjustDramSpeedFlag)
{
    logd("**** VeSetAdjustDramSpeedFlag: %d",nAdjustDramSpeedFlag);
    VeContext* pVeContext;
    pVeContext  = (VeContext*)p;

    pVeContext->bDynamicEnableDramHightChannalFlag = nAdjustDramSpeedFlag;
}


void VeEnableVe(void* p)
{
    VeContext* pVeContext;
    pVeContext  = (VeContext*)p;

    if(pVeContext->bDynamicEnableDramHightChannalFlag == 1)
    {
        ioctl(gVeEnvironmentInfo.nVeDriverFd, IOCTL_SET_DRAM_HIGH_CHANNAL, 1);
    }

   if(pVeContext->mVeConfig.nDecoderFlag)
   {
        enableDecoder(pVeContext);
   }
   return ;
}

void VeDisableVe(void* p)
{
    VeContext* pVeContext;
    pVeContext  = (VeContext*)p;

    if(pVeContext->bDynamicEnableDramHightChannalFlag == 1)
    {
        ioctl(gVeEnvironmentInfo.nVeDriverFd, IOCTL_SET_DRAM_HIGH_CHANNAL, 0);
    }

    if(pVeContext->mVeConfig.nDecoderFlag)
    {
        disableDecoder(pVeContext);
    }

    return ;
}

void VeInitEncoderPerformance(void* p, int nMode) //* 0: normal performance; 1. high performance
{
    CEDARC_UNUSE(nMode);
}

void VeUninitEncoderPerformance(void* p, int nMode) //* 0: normal performance; 1. high performance
{
    CEDARC_UNUSE(nMode);
}

int VeGetIommuAddr(void* p, struct user_iommu_param *iommu_buffer)
{
    CEDARC_UNUSE(p);
    int ret;

    ret = ioctl(gVeEnvironmentInfo.nVeDriverFd, IOCTL_GET_IOMMU_ADDR, iommu_buffer);
    return ret;
}

int VeFreeIommuAddr(void* p, struct user_iommu_param *iommu_buffer)
{
    CEDARC_UNUSE(p);
    int ret;

    ret = ioctl(gVeEnvironmentInfo.nVeDriverFd, IOCTL_FREE_IOMMU_ADDR, iommu_buffer);
    return ret;
}

int VeSetProcInfo(void* p, char *proc_info_buf, unsigned int buf_len, unsigned char cChannelNum)
{
    CEDARC_UNUSE(p);
    int ret;
    ve_proc_info_t ve_proc;

    ve_proc.proc_info_len = buf_len;
    ve_proc.channel_id = cChannelNum;

    logv("ve:proc_info_buf:%p, len:%d, num:%d\n",
                                                                    proc_info_buf,
                                                                    ve_proc.proc_info_len,
                                                                    ve_proc.channel_id);
    ret = ioctl(gVeEnvironmentInfo.nVeDriverFd, IOCTL_SET_PROC_INFO, &ve_proc);
    if(ret < 0)
    {
        loge("set ve proc info error.\n");
        return -1;
    }

    ret = ioctl(gVeEnvironmentInfo.nVeDriverFd, IOCTL_COPY_PROC_INFO, proc_info_buf);
    if(ret < 0)
    {
        loge("copy ve proc info error.\n");
        return -1;
    }

    return ret;
}

int VeStopProcInfo(void* p,unsigned char cChannelNum)
{
    CEDARC_UNUSE(p);
    int ret;

    ret = ioctl(gVeEnvironmentInfo.nVeDriverFd, IOCTL_STOP_PROC_INFO, cChannelNum);
    if(ret < 0)
    {
        loge("stop ve proc info error.\n");
        return -1;
    }

    return ret;
}

int VeGetChipId(void* p)
{
    CEDARC_UNUSE(p);
    return 0;
}
VeOpsS _veAwOpsS =
{
    .init                     = VeInitialize,
    .release                  = VeRelease,
    .lock                     = VeLock,
    .unlock                   = VeUnLock,
    .reset                    = VeReset,
    .waitInterrupt            = VeWaitInterrupt,

#if CEDARC_DEBUG
    .WriteValue               = VeWriteValue,
    .ReadValue                = VeReadValue,
    .CleanValue               = VeClearnValue,
#endif

    .getChipId                = VeGetChipId,
    .getIcVeVersion           = VeGetIcVeVersion,
    .getGroupRegAddr          = VeGetGroupRegAddr,
    .getDramType              = VeGetDramType,
    .getPhyOffset             = VeGetPhyOffset,

    .setDramType              = VeSetDramType,
    .setDdrMode               = VeSetDdrMode,
    .setSpeed                 = VeSetSpeed,
    .setEnableAfbcFlag        = VeSetEnableAfbcFlag,
    .setAdjustDramSpeedFlag   = VeSetAdjustDramSpeedFlag,

    .enableVe                 = VeEnableVe,
    .disableVe                = VeDisableVe,

    .initEncoderPerformance   = VeInitEncoderPerformance,
    .unInitEncoderPerformance = VeUninitEncoderPerformance,

    .getIommuAddr             = VeGetIommuAddr,
    .freeIommuAddr            = VeFreeIommuAddr,

    .setProcInfo              = VeSetProcInfo,
    .stopProcInfo             = VeStopProcInfo,
};

VeOpsS* getVeAwOpsS()
{
    return &_veAwOpsS;
}
