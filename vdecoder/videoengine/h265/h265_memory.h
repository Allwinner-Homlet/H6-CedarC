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
* File : h265_memory.h
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#ifndef H265_MEMORY_H
#define H265_MEMORY_H

#include "h265_config.h"

#if HEVC_ENABLE_MEMORY_LEAK_DEBUG
#define HevcMalloc(size)         HevcMallocDebug(size,__FILE__,__func__,__LINE__)
#define HevcCalloc(count,size) HevcCallocDebug(count,size,__FILE__,__func__,__LINE__)
#define HevcFree(ptr)        HevcFreeDebug(ptr,__func__,__LINE__)
#define HevcAdapterPalloc(_memops ,size, veOpsS, pVeOpsSelf)   \
        HevcAdapterPallocDebug(_memops ,size, veOpsS, pVeOpsSelf, __FILE__,__func__,__LINE__)
#define HevcAdapterFree(_memops , ptr, veOpsS, pVeOpsSelf)  \
        HevcAdapterFreeDebug(_memops , ptr, veOpsS, pVeOpsSelf,__func__,__LINE__)

#define DEBUG_MEM_BLOCK_INFO_NUM 10000
typedef struct Memory_Block_Info
{
    u32 ptr;
    u32 size;
    u32 line;
    u32 order;
    u32 bIsPalloc;
    const char *function;
    const char *filename;
    u32 flag;
}MemoryBlockInfo;

void  HevcFreeDebug(void *arg, const char *function, u32 line);
void* HevcMallocDebug(u32 size, const char *filename, const char *function, u32 line);
void* HevcCallocDebug(u32 count, u32 size, const char *filename,
                           const char *function, u32 line);
void  HevcAdapterFreeDebug(struct ScMemOpsS *_memops, void *arg, void *veOpsS,
                                  void *pVeOpsSelf,const char *function, u32 line);
void *HevcAdapterPallocDebug(struct ScMemOpsS *_memops, u32 size, void *veOpsS, void *pVeOpsSelf,
                             const char *filename, const char *function, u32 line);
void HevcMeroryLeakDebugInfo(void);
void HevcMeroryDebugInit(void);
void HevcMeroryDebugClose(void);
#else

#define HevcMalloc(size)         HevcMallocNormal(size)
#define HevcCalloc(count,size) HevcCallocNormal(count,size)
#define HevcFree(ptr)        HevcFreeNormal(ptr)
#define HevcAdapterPalloc(_memops ,size, veOpsS, pVeOpsSelf) \
        HevcAdapterPallocNormal(_memops, size, veOpsS, pVeOpsSelf)
#define HevcAdapterFree(_memops, ptr, veOpsS, pVeOpsSelf) \
        HevcAdapterFreeNormal(_memops, ptr, veOpsS, pVeOpsSelf)

void* HevcMallocNormal(u32 size);
void* HevcCallocNormal(u32 count, u32 size);
void  HevcFreeNormal(void *arg);
void  HevcAdapterFreeNormal(struct ScMemOpsS *_memops, void *arg, void *veOpsS, void *pVeOpsSelf);
void *HevcAdapterPallocNormal(struct ScMemOpsS *_memops, u32 size, void *veOpsS, void *pVeOpsSelf);

#endif /* HEVC_ENABLE_MEMORY_LEAK_DEBUG */

#endif /* H265_MEMORY_H */

