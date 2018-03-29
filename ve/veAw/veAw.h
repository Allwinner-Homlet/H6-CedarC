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
* File : cedarv_ve.h
* Description : Video engine driver API, Don't modify it in user space.
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*/
/* Notice: It's video engine driver API, Don't modify it in user space. */
#ifndef _VE_AW_H_
#define _VE_AW_H_

enum IOCTL_CMD {
    IOCTL_UNKOWN = 0x100,
    IOCTL_GET_ENV_INFO,
    IOCTL_WAIT_VE_DE,
    IOCTL_WAIT_VE_EN,
    IOCTL_RESET_VE,
    IOCTL_ENABLE_VE,
    IOCTL_DISABLE_VE,
    IOCTL_SET_VE_FREQ,

    IOCTL_CONFIG_AVS2 = 0x200,
    IOCTL_GETVALUE_AVS2 ,
    IOCTL_PAUSE_AVS2 ,
    IOCTL_START_AVS2 ,
    IOCTL_RESET_AVS2 ,
    IOCTL_ADJUST_AVS2,
    IOCTL_ENGINE_REQ,
    IOCTL_ENGINE_REL,
    IOCTL_ENGINE_CHECK_DELAY,
    IOCTL_GET_IC_VER,
    IOCTL_ADJUST_AVS2_ABS,
    IOCTL_FLUSH_CACHE,
    IOCTL_SET_REFCOUNT,
    IOCTL_FLUSH_CACHE_ALL,
    IOCTL_TEST_VERSION,

    IOCTL_GET_LOCK = 0x310,
    IOCTL_RELEASE_LOCK,

    IOCTL_SET_VOL = 0x400,

    IOCTL_WAIT_JPEG_DEC = 0x500,
    /*for get the ve ref_count for ipc to delete the semphore*/
    IOCTL_GET_REFCOUNT,

    /*for iommu*/
    IOCTL_GET_IOMMU_ADDR,
    IOCTL_FREE_IOMMU_ADDR,

    /*for debug*/
    IOCTL_SET_PROC_INFO,
    IOCTL_STOP_PROC_INFO,
    IOCTL_COPY_PROC_INFO,

    IOCTL_SET_DRAM_HIGH_CHANNAL = 0x600,
    IOCTL_READ_DDR_VALUE,
    IOCTL_WRITE_DDR_VALUE,
    IOCTL_CLEAR_DDR_VALUE,
};

#define VE_LOCK_VDEC        0x01
#define VE_LOCK_VENC        0x02
#define VE_LOCK_JDEC        0x04
#define VE_LOCK_00_REG      0x08
#define VE_LOCK_04_REG      0x10
#define VE_LOCK_ERR         0x80

//* do not modify the struct, must be the same with /lichee/cedar_ve
typedef struct cedarv_env_addr_macc{
    unsigned int phymem_start;
    int  phymem_total_size;
    unsigned long  address_macc;
}cedarv_env_addr_macc;

struct cedarv_env_infomation {
    int nVeRefCount;
    int nVeDriverFd;
    unsigned long  address_macc;

    pthread_mutex_t  VeMutex;
    pthread_mutex_t  VeRegisterMutex;

    pthread_mutex_t  decMutex;
    pthread_mutex_t  encMutex;
    pthread_mutex_t  jdecMutex;

    int              nNomalEncRefCount;
    int              nPerfEncRefCount;
    int              nClkCount;
};
#endif
