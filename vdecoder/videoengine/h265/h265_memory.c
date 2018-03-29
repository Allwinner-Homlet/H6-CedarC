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
* File : h265_memory.c
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#include "h265_memory.h"

#if HEVC_ENABLE_MEMORY_LEAK_DEBUG

static MemoryBlockInfo gMemBlockInfo[DEBUG_MEM_BLOCK_INFO_NUM] = {{0}};
static u32 gMemBlockNum = 0;
static u32 gMemCallocOrder = 0;
static u32 gMemPallocOrder = 0;
pthread_rwlock_t gMemDebugRwlock;

void  HevcFreeDebug(void *arg, const char *function, u32 line)
{
    void **ptr;
    s32 i, index;
    u32 pointer;
    ptr = (void **)arg;
    if(arg == NULL || (*ptr == NULL))
        return ;
    pointer = (u32)(*ptr);
    index = -1;
    pthread_rwlock_wrlock(&gMemDebugRwlock);
//    logd("-------FREE ptr: %d\t\tline: %d      function: %s", (u32)pointer, line, function);

    for(i = 0; i < DEBUG_MEM_BLOCK_INFO_NUM; i++)
    {
        if(gMemBlockInfo[i].ptr == pointer)
        {
            index = i;
#if 0
            logd("FREE: ptr: %p, \tsize: %d, \t\tline: %d, \t\tnum: %d,\t\t order: %d",
                    gMemBlockInfo[i].ptr, gMemBlockInfo[i].size, gMemBlockInfo[i].line,
                    gMemBlockNum, gMemBlockInfo[i].order);
#endif
            if(gMemBlockInfo[i].bIsPalloc == 1)
            {
                logd("free error. palloc block: \
                    ptr: 0x%x\t size: %d, func: %s, line: %d , order: %d, file: %s",
                    (u32)gMemBlockInfo[i].ptr, gMemBlockInfo[i].size, gMemBlockInfo[i].function,
                    gMemBlockInfo[i].line, gMemBlockInfo[i].order,
                    strrchr(gMemBlockInfo[i].filename, '/')+1);
            }
            memset(&gMemBlockInfo[i], 0, sizeof(MemoryBlockInfo));
            gMemBlockNum -= 1;
            break;
        }
    }

    pthread_rwlock_unlock(&gMemDebugRwlock);
    free(*ptr);
    *ptr = NULL;
}

void* HevcMallocDebug(u32 size, const char *filename, const char *function, u32 line)
{
    void* ptr;
    s32 i, index;
    index = 0;
    pthread_rwlock_wrlock(&gMemDebugRwlock);
    ptr = malloc(size);
    gMemCallocOrder += 1;
    gMemBlockNum += 1;
    for(i = 0; i < DEBUG_MEM_BLOCK_INFO_NUM; i++)
    {
        if(gMemBlockInfo[i].flag == 0)
        {
            index = i;
            break;
        }
    }
    if(i < DEBUG_MEM_BLOCK_INFO_NUM)
    {
        gMemBlockInfo[index].flag = 1;
        gMemBlockInfo[index].ptr = (u32)ptr;
        gMemBlockInfo[index].size = size;
        gMemBlockInfo[index].line = line;
        gMemBlockInfo[index].bIsPalloc = 0;
        gMemBlockInfo[index].filename = filename;
        gMemBlockInfo[index].function = function;
        gMemBlockInfo[index].order = gMemCallocOrder;
    }
    else
    {
        logd("HEVC_MEM_LEAK_DEBUG: MEM_BLOCK_INFO_NUM is too small !!!");
    }
    pthread_rwlock_unlock(&gMemDebugRwlock);
//    logd("MALLOC: ptr: %d     line: %d    size: %d,\t\torder: %d ",
//    (u32)ptr, line, size, gMemCallocOrder);
    return ptr;
}

void* HevcCallocDebug(u32 count, u32 size, const char *filename,
    const char *function, u32 line)
{
    void* ptr;
    s32 i, index;
    index = 0;
    pthread_rwlock_wrlock(&gMemDebugRwlock);
    ptr = calloc(count, size);
    gMemCallocOrder += 1;
    gMemBlockNum += 1;
    for(i = 0; i < DEBUG_MEM_BLOCK_INFO_NUM; i++)
    {
        if(gMemBlockInfo[i].flag == 0)
        {
            index = i;
            break;
        }
    }
    if(i < DEBUG_MEM_BLOCK_INFO_NUM)
    {
        gMemBlockInfo[index].flag = 1;
        gMemBlockInfo[index].ptr = (u32)ptr;
        gMemBlockInfo[index].size = size * count;
        gMemBlockInfo[index].line = line;
        gMemBlockInfo[index].bIsPalloc = 0;
        gMemBlockInfo[index].filename = filename;
        gMemBlockInfo[index].function = function;
        gMemBlockInfo[index].order = gMemCallocOrder;
    }
    else
    {
        logd("HEVC_MEM_LEAK_DEBUG: MEM_BLOCK_INFO_NUM is too small !!!");
    }
    pthread_rwlock_unlock(&gMemDebugRwlock);
//    logd("CALLOC:     ptr: %d     line: %d\t\tsize: %d\t\t order: %d",
//    (u32)ptr, line, size * count, gMemCallocOrder);
    return ptr;
}

void HevcAdapterFreeDebug(struct ScMemOpsS *_memops, void *arg,
                                    void *veOpsS, void *pVeOpsSelf,const char *function, u32 line)
{
    void **ptr;
    s32 i, index;
    u32 pointer;
    ptr = (void **)arg;
    if(arg == NULL || (*ptr == NULL))
        return ;
    pointer = (u32)(*ptr);
    index = -1;
    pthread_rwlock_wrlock(&gMemDebugRwlock);
//    logd("-------FREE ptr: %d\t\tline: %d  function: %s",
//     (u32)pointer, line, function);

    for(i = 0; i < DEBUG_MEM_BLOCK_INFO_NUM; i++)
    {
        if(gMemBlockInfo[i].ptr == pointer)
        {
            index = i;
#if 0
            logd("FREE: ptr: %p, \tsize: %d, \t\tline: %d, \t\tnum: %d,\t\t order: %d",
                    gMemBlockInfo[i].ptr, gMemBlockInfo[i].size, gMemBlockInfo[i].line,
                    gMemBlockNum, gMemBlockInfo[i].order);
#endif
            if(gMemBlockInfo[i].bIsPalloc == 0)
            {
                logd("palloc free. normal block: \
                    ptr: 0x%x\t size: %d, func: %s, line: %d , order: %d, file: %s",
                    (u32)gMemBlockInfo[i].ptr, gMemBlockInfo[i].size, gMemBlockInfo[i].function,
                    gMemBlockInfo[i].line, gMemBlockInfo[i].order,
                    strrchr(gMemBlockInfo[i].filename, '/')+1);
            }
            memset(&gMemBlockInfo[i], 0, sizeof(MemoryBlockInfo));
            gMemBlockNum -= 1;
            break;
        }
    }

    pthread_rwlock_unlock(&gMemDebugRwlock);
    CdcMemPfree(_memops, *ptr, veOpsS, pVeOpsSelf);
    *ptr = NULL;
}

void *HevcAdapterPallocDebug(struct ScMemOpsS *_memops, u32 size,void *veOpsS, void *pVeOpsSelf,
                             const char *filename, const char *function, u32 line)
{
    void* ptr;
    s32 i, index;
    index = 0;
    pthread_rwlock_wrlock(&gMemDebugRwlock);
    ptr = CdcMemPalloc(_memops, size, veOpsS, pVeOpsSelf);
    if(ptr == NULL)
    {
        pthread_rwlock_unlock(&gMemDebugRwlock);
        return NULL;
    }
    gMemCallocOrder += 1;
    gMemPallocOrder += 1;
    gMemBlockNum += 1;
    for(i = 0; i < DEBUG_MEM_BLOCK_INFO_NUM; i++)
    {
        if(gMemBlockInfo[i].flag == 0)
        {
            index = i;
            break;
        }
    }
    if(i < DEBUG_MEM_BLOCK_INFO_NUM)
    {
        gMemBlockInfo[index].flag = 1;
        gMemBlockInfo[index].ptr = (u32)ptr;
        gMemBlockInfo[index].size = size;
        gMemBlockInfo[index].line = line;
        gMemBlockInfo[index].bIsPalloc = 1;
        gMemBlockInfo[index].filename = filename;
        gMemBlockInfo[index].function = function;
        gMemBlockInfo[index].order = gMemCallocOrder;
    }
    else
    {
        logd("HEVC_MEM_LEAK_DEBUG: MEM_BLOCK_INFO_NUM is too small !!!");
    }
    pthread_rwlock_unlock(&gMemDebugRwlock);
//    logd("MALLOC: ptr: %d     line: %d    size: %d,\t\torder: %d ",
//    (u32)ptr, line, size, gMemCallocOrder);
    return ptr;
}

void HevcMeroryLeakDebugInfo(void)
{
    s32 num = 0, nPallocNum = 0;
    u32 i;
    pthread_rwlock_wrlock(&gMemDebugRwlock);
    num = gMemBlockNum;

    logd("--------------------- Memory debug information start ---------------------- ");
    logd(" UNFREE memory block number: %d ", gMemBlockNum);
    logd(" total memory block number:  %d ", gMemCallocOrder);
    logd(" calloc/malloc block number: %d ", gMemCallocOrder - gMemPallocOrder);
    logd(" palloc block number:        %d ", gMemPallocOrder);
//    logd("Memory Info: unfree memory block = %d,
//          total calloc/malloc number = %d, total palloc block: %d",
//            gMemBlockNum, gMemCallocOrder - gMemPallocOrder, gMemPallocOrder);
    logd("----------------------- Memory debug information end ---------------------- ");
//    if(num != 0)
    {
        for(i = 0; i < DEBUG_MEM_BLOCK_INFO_NUM; i++)
        {
            if(gMemBlockInfo[i].flag == 1)
            {
                logd("MEMORY LEAK unfree block: \
                    ptr: 0x%x\t size: %d, func: %s, line: %d , order: %d, file: %s",
                        (u32)gMemBlockInfo[i].ptr, gMemBlockInfo[i].size, gMemBlockInfo[i].function,
                        gMemBlockInfo[i].line, gMemBlockInfo[i].order,
                        strrchr(gMemBlockInfo[i].filename, '/')+1);
            }
        }
    }
    pthread_rwlock_unlock(&gMemDebugRwlock);
}

void HevcMeroryDebugInit(void)
{
    pthread_rwlock_init(&gMemDebugRwlock, NULL);
}

void HevcMeroryDebugClose(void)
{
    pthread_rwlock_destroy(&gMemDebugRwlock);
}

#else /* #if HEVC_ENABLE_MEMORY_LEAK_DEBUG */

void  HevcFreeNormal(void *arg)
{
    void **ptr;
    if(arg == NULL)
        return ;
    ptr = (void **)arg;
    free(*ptr);
    *ptr = NULL;
}

void* HevcMallocNormal(u32 size)
{
    void* ptr = malloc(size);
    return ptr;
}

void* HevcCallocNormal(u32 count, u32 size)
{
    void* ptr = calloc(count, size);
    return ptr;
}

void HevcAdapterFreeNormal(struct ScMemOpsS *_memops, void *arg, void *veOpsS, void *pVeOpsSelf)
{
    void **ptr;
    if(arg == NULL)
        return ;
    ptr = (void **)arg;
    CdcMemPfree(_memops, *ptr, veOpsS, pVeOpsSelf);
    *ptr = NULL;
}

void *HevcAdapterPallocNormal(struct ScMemOpsS *_memops, u32 size, void *veOpsS, void *pVeOpsSelf)
{
    void* ptr = CdcMemPalloc(_memops, size, veOpsS, pVeOpsSelf);
    return ptr;
}
#endif /* #if HEVC_ENABLE_MEMORY_LEAK_DEBUG */
