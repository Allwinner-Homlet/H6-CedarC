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
* File : h265_drv.h
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#ifndef H265_DRV_H
#define H265_DRV_H

#include "h265_config.h"
#include <semaphore.h>
#include "h265_var.h"

#ifdef __cplusplus
extern "C" {
#endif
// interface to decode_interface


typedef struct H265_DEC_CTX
{
    DecoderInterface    interface;
    VideoEngine*        pVideoEngine;
    VConfig             vconfig;   //* decode library configuration
    VideoStreamInfo     videoStreamInfo;   //* video stream information;

    HevcContex            *pHevcDec;
    VideoFbmInfo*       pFbmInfo;
}H265DecCtx;

#ifdef __cplusplus
}
#endif

#endif
