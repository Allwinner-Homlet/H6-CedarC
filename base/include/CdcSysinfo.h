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
* File : CdcSysinfo.h
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#ifndef CDX_SYS_INFO_H
#define CDX_SYS_INFO_H


#ifdef __cplusplus
extern "C"
{
#endif

//* get current ddr frequency, if it is too slow, we will cut some spec off.
int CdcGetDramFreq();


enum CHIPID
{
    SI_CHIP_UNKNOWN = 0,
    SI_CHIP_H3s = 1,
    SI_CHIP_H3 = 2,
    SI_CHIP_H2 = 3,
    SI_CHIP_H2PLUS = 4,
};

int CdcGetSysinfoChipId(void);


#ifdef __cplusplus
}
#endif

#endif

