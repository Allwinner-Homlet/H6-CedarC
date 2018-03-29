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
* File : typedef.h
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/


#ifndef TYPEDEF_H
#define TYPEDEF_H
#include <stdint.h>

    typedef unsigned char u8;
    typedef unsigned short u16;
    typedef unsigned int u32;
#ifdef COMPILER_ARMCC
    typedef unsigned __int64 u64;
#else
    typedef unsigned long long u64;
#endif
    typedef signed char s8;
    typedef signed short s16;
    typedef signed int s32;
#ifdef COMPILER_ARMCC
    typedef signed __int64 s64;
#else
    typedef signed long long s64;
#endif
    typedef uintptr_t size_addr;

#endif

