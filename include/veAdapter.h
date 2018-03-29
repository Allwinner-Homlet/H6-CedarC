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
* File : veAdapter.h
* Description :
* History :
*   Author  : wangxiwang <wangxiwang@allwinnertech.com>
*   Date    : 2017/03/15
*   Comment :
*
*
*/


#ifndef VE_ADAPTER_H
#define VE_ADAPTER_H
#include "veInterface.h"

#ifdef __cplusplus
extern "C"
{
#endif

VeOpsS* GetVeOpsS(int type);

#ifdef __cplusplus
}
#endif

#endif
