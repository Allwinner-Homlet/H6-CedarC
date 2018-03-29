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
* File : sbm.h
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#ifndef SBM_H
#define SBM_H
#include <pthread.h>

#include "sbmInterface.h"
#include "log.h"
#include "CdcMessageQueue.h"

#ifdef __cplusplus
extern "C" {
#endif

//SbmInterface* GetSbmInterfaceStream();

SbmInterface* GetSbmInterface(int nType);

#define SBM_FRAME_FIFO_SIZE (2048)  //* store 2048 frames of bitstream data at maximum.
#define MAX_INVALID_STREAM_DATA_SIZE (1*1024*1024) //* 1 MB
#define MAX_NALU_NUM_IN_FRAME (1024)
#define TMP_BUFFER_SIZE (1*1024*1024)

enum SBM_THREAD_CMD
{
    SBM_THREAD_CMD_START = 0,
    SBM_THREAD_CMD_READ  = 1,
    SBM_THREAD_CMD_QUIT  = 2,
    SBM_THREAD_CMD_RESET = 3
};

typedef struct StreamBufferManagerFrame SbmFrame;

struct StreamBufferManagerFrame
{
    SbmInterface    sbmInterface;

    pthread_mutex_t mutex;
    char*           pStreamBuffer;    //* start buffer address
    char*           pStreamBufferEnd;
    int             nStreamBufferSize; //* buffer total size
    char*           pWriteAddr;
    int             nValidDataSize;
    StreamFrameFifo frameFifo;

    CdcMessageQueue* mq;
    pthread_t        mThreadId;
    FramePicFifo     mFramePicFifo;
    sem_t            streamDataSem;
    sem_t            emptyFramePicSem;

    sem_t            resetSem;
    int              bStreamWithStartCode;
    DetectFramePicInfo mDetectInfo;
    SbmConfig mConfig;
    int       nEosFlag;

    int nDebugNum;

    char* pTmpBuffer;     //* for secure video
    int   nTmpBufferSize;

    char* pDetectBufStart;
    char* pDetectBufEnd;

    SbmFrameBufferNode*   pSbmFrameBufferValidQueue;
    SbmFrameBufferNode*   pSbmFrameBufferEmptyQueue;
    SbmFrameBufferNode*   pSbmFrameBufferLastNode;
    SbmFrameBufferNode*   pSbmFrameBuffer;
    pthread_mutex_t       pSbmFrameBufferMutex;

    s32 (*checkBitStreamType)(SbmFrame* pSbm);
    void (*detectOneFramePic)(SbmFrame* pSbm);
};


#ifdef __cplusplus
}
#endif
#endif

