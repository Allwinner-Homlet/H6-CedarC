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
* File : cedarv_api.h
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#ifndef __VP9DEV_API_H__
#define __VP9DEV_API_H__

typedef struct VP9_PARAM
{
    unsigned int ic_version;
    int             ddr_mode;
    uint32_t      phy_offset;
    int             need_check_idle;
}vp9_param_t;

enum IOCTL_CMD {
    VP9_IOCTL_UNKOWN = 0x100,
    VP9_IOCTL_GET_ENV_INFO,
    VP9_IOCTL_WAIT_INTERRUPT,
    VP9_IOCTL_RESET,
    VP9_IOCTL_SET_FREQ,
    VP9_IOCTL_SET_HIGH_PERF_MSG,    //1:begin high perfmance, 0:end high perfmance

    VP9_IOCTL_ENGINE_REQ,
    VP9_IOCTL_ENGINE_REL,

    /*for iommu*/
    IOCTL_GET_IOMMU_ADDR,
    IOCTL_FREE_IOMMU_ADDR,
};

typedef struct VP9_ENV_INFOMATION{
    unsigned int phymem_start;
    int  phymem_total_size;
    unsigned long  address_macc;
}vp9_env_info_t;

enum CEDARX_CACHE_OP {
    CEDARX_DCACHE_FLUSH = 0,
    CEDARX_DCACHE_CLEAN_FLUSH,
    CEDARX_DCACHE_FLUSH_ALL,
};

typedef struct cedarv_cache_range_{
    long start;
    long end;
}cedarv_cache_range;

struct cedarv_regop {
    unsigned int addr;
    unsigned int value;
};

#endif
