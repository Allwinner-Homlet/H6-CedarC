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
* File : memoryAdapter.h
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#ifndef MEMORY_ADAPTER_H
#define MEMORY_ADAPTER_H

#ifdef __cplusplus
extern "C" {
#endif


//* get current ddr frequency, if it is too slow, we will cut some spec off.
int MemAdapterGetDramFreq();

struct ScMemOpsS* MemAdapterGetOpsS();
struct ScMemOpsS* SecureMemAdapterGetOpsS();

#ifdef __cplusplus
}
#endif

#endif /* MEMORY_ADAPTER_H */
