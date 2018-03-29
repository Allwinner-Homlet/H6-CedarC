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
* File : vp9_google_engine.c
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#include "log.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <sys/mman.h>
#include <pthread.h>
#include <stdint.h>

#include "veVp9.h"

#include "../include/veInterface.h"

//#include "vdecoder.h"

//for ipc
#include <linux/sem.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>

#ifdef CONFIG_VE_IPC_ENABLE
   #define VE_IPC 1
#else
   #define VE_IPC 0
#endif
#define VE_IPC 0

#if VE_IPC
#define         SEM_NUM 1
static int      sem_id = 0;
static key_t    id = 0;

static int aw_set_semvalue();
static void aw_del_semvalue();
static int semaphore_p(unsigned short sem_index);
static int semaphore_v(unsigned short sem_index);
extern key_t ftok(const char *pathname, int proj_id);
#endif

void Vp9Reset(void* p);

static pthread_mutex_t  gVP9Mutex = PTHREAD_MUTEX_INITIALIZER;

static int              gVP9RefCount = 0;
int                     gVP9DriverFd = -1;
vp9_env_info_t          gVP9EnvironmentInfo;
vp9_param_t             gVP9Param;

static int              gNomalDecRefCount = 0;
static int              gPerfDecRefCount = 0;

typedef struct Vp9Context{
    VeConfig mVeConfig;
    pthread_mutex_t *locks[3];
}Vp9Context;

void* Vp9Initialize(VeConfig* pVeConfig)
{
    //unsigned short sem_index;
    //int sem_result = 0;

    pthread_mutex_lock(&gVP9Mutex);

    Vp9Context* p = NULL;
    p = malloc(sizeof(Vp9Context));
    if(p == NULL)
    {
        loge("malloc for vecontext failed");
        return NULL;
    }
    memset(p, 0, sizeof(Vp9Context));
    memcpy(&p->mVeConfig, pVeConfig, sizeof(VeConfig));

    if(gVP9RefCount == 0)
    {
        #if VE_IPC
        id = ftok("/data/device_VP9.info", 'b');

        // Create and check for existence
        sem_result = syscall(__NR_semget, id, SEM_NUM, 0666 | IPC_CREAT | IPC_EXCL);
        if(sem_result == -1)
        {
            //the sem of vp9 has been created, only need to get the sem_id for current process
            sem_id = syscall(__NR_semget, id, SEM_NUM, 0666 | IPC_CREAT);
            if(sem_id == -1)
            {
                fprintf(stderr, "google-vp9 semphore has already been created , \
                    but the process %d get sem_id error\n", getpid());
                exit(EXIT_FAILURE);
            }
            logd("get google-vp9 semphore ok: the process that call the google-vp9 is %d, \
                sem id is %d\n", getpid(),sem_id);
        }
        else
        {
            // here is the first creating the sem of ve, need to init sem
            if(!aw_set_semvalue())
            {
                fprintf(stderr, "Failed to initialize semaphore\n");
                exit(EXIT_FAILURE);
            }
            logd("init google-vp9 semphore ok: the process that first call the google-vp9 is %d,\
                sem id is %d\n", getpid(),sem_id);
        }

        sem_index = 0;
        if(!semaphore_p(sem_index))
            exit(EXIT_FAILURE);
        #endif

        //* open Ve driver.
        gVP9DriverFd = open("/dev/googlevp9_dev", O_RDWR);
        if(gVP9DriverFd < 0)
        {
            loge("open /dev/googlevp9_dev fail.");
            pthread_mutex_unlock(&gVP9Mutex);
            free(p);
            return NULL;
        }

        //* request ve.
        ioctl(gVP9DriverFd, VP9_IOCTL_ENGINE_REQ, 0);

        //* map registers.
        ioctl(gVP9DriverFd, VP9_IOCTL_GET_ENV_INFO, (unsigned long)&gVP9EnvironmentInfo);
        gVP9EnvironmentInfo.address_macc = (unsigned long)mmap(NULL,
                                   2048,
                                   PROT_READ | PROT_WRITE, MAP_SHARED,
                                   gVP9DriverFd,
                                   (unsigned long)gVP9EnvironmentInfo.address_macc);

        Vp9Reset(p);

        //gVP9Param.ic_version = VeGetIcVersion();
        //set default ddr_mode
        //gVP9Param.ddr_mode = DDRTYPE_DDR3_32BITS;
        //gVP9Param.need_check_idle = 0;
        gVP9Param.phy_offset = 0x40000000;

        logd("*** set vp9 freq to 576MHZ**");
        ioctl(gVP9DriverFd, VP9_IOCTL_SET_FREQ, 576);

        #if VE_IPC
        sem_index = 0;
        if(!semaphore_v(sem_index))
            exit(EXIT_FAILURE);
        #endif
    }

    gVP9RefCount++;
    pthread_mutex_unlock(&gVP9Mutex);

    return p;
}

void Vp9Release(void* p)
{
    //unsigned short sem_index;
    //long ve_ref_count;
    Vp9Context* pVp9Context = (Vp9Context*)p;
    //int j = 0;

    pthread_mutex_lock(&gVP9Mutex);

    if(gVP9RefCount <= 0)
    {
        loge("invalid status, gVP9RefCount=%d at AdpaterRelease", gVP9RefCount);
        pthread_mutex_unlock(&gVP9Mutex);
        return;
    }

    gVP9RefCount--;

    if(gVP9RefCount == 0)
    {
        #if VE_IPC
        sem_index = 0;
        if(!semaphore_p(sem_index))
            exit(EXIT_FAILURE);
        #endif

        {
            if(gVP9DriverFd != -1)
            {
                ioctl(gVP9DriverFd, VP9_IOCTL_ENGINE_REL, 0);
                munmap((void *)gVP9EnvironmentInfo.address_macc, 2048);
                close(gVP9DriverFd);
                gVP9DriverFd = -1;
            }
        }

        #if VE_IPC
        sem_index = 0;
        if(!semaphore_v(sem_index))
            exit(EXIT_FAILURE);
        #endif
    }

    pthread_mutex_unlock(&gVP9Mutex);

#if defined(CONFIG_VE_IPC_ENABLE)
        logi("not malloc locks\n");
#else
        int j;
        for(j = 0; j < 3; j++)
        {
            if(pVp9Context->locks[j] != NULL)
            {
                free(pVp9Context->locks[j]);
                pVp9Context->locks[j] = NULL;
            }
        }
#endif

#if defined(CONFIG_VE_IPC_ENABLE)
        logi("not malloc locks\n");
#else
        for(j = 0; j < 3; j++)
        {
            if(pVp9Context->locks[j] != NULL)
            {
                free(pVp9Context->locks[j]);
                pVp9Context->locks[j] = NULL;
            }
        }
#endif

    free(pVp9Context);

    return;
}

int Vp9Lock(void* p)
{
    CEDARC_UNUSE(p);
#if VE_IPC
    if(!semaphore_p(0))
        exit(EXIT_FAILURE);
#endif

    return pthread_mutex_lock(&gVP9Mutex);
}

int Vp9UnLock(void* p)
{
    CEDARC_UNUSE(p);
#if VE_IPC
    if(!semaphore_v(0))
        exit(EXIT_FAILURE);
#endif

    return pthread_mutex_unlock(&gVP9Mutex);
}

void Vp9Reset(void* p)
{
    CEDARC_UNUSE(p);
    ioctl(gVP9DriverFd, VP9_IOCTL_RESET, 0);

    //VeSetDramType();
}

int Vp9WaitInterrupt(void* p)
{
    CEDARC_UNUSE(p);
    int ret;

    ret = ioctl(gVP9DriverFd, VP9_IOCTL_WAIT_INTERRUPT, 1);
    if(ret <= 0)
    {
        logw("wait vp9 interrupt timeout. ret = %d",ret);
        return -1;  //* wait ve interrupt fail.
    }
    else
    {
        //if(gVP9Param.need_check_idle)
            //veInquireIdle();
        return 0;
    }
}

int Vp9GetChipId(void* p)
{
    CEDARC_UNUSE(p);
    return 0;
}

void* Vp9GetGroupRegAddr(void* p, int nGroupId)
{
    CEDARC_UNUSE(p);
    CEDARC_UNUSE(nGroupId);
    //logd("***** VP9GetRegisterBaseAddress, addr = %p",(void*)gVP9EnvironmentInfo.address_macc);
    return (void*)gVP9EnvironmentInfo.address_macc;
}

uint64_t Vp9GetIcVeVersion(void* p)
{
    CEDARC_UNUSE(p);
    return 1;
}

int Vp9GetDramType(void* p)
{
    CEDARC_UNUSE(p);
    return 0;
}

uint32_t Vp9GetPhyOffset(void* p)
{
    CEDARC_UNUSE(p);
    //return gVP9Param.phy_offset;
    return 0;
}

void Vp9SetDramType(void* p)
{
    CEDARC_UNUSE(p);
    return ;
}

void Vp9SetDdrMode(void* p, int ddr_mode)
{
    CEDARC_UNUSE(p);
    CEDARC_UNUSE(ddr_mode);
    return ;
}

int Vp9SetSpeed(void* p, unsigned int nSpeedMHz)
{
    CEDARC_UNUSE(p);
    CEDARC_UNUSE(nSpeedMHz);
    return 0;
}

void Vp9SetEnableAfbcFlag(void* p, int bEnableFlag)
{
    CEDARC_UNUSE(p);
    CEDARC_UNUSE(bEnableFlag);
}

void Vp9SetAdjustDramSpeedFlag(void* p, int nAdjustDramSpeedFlag)
{
    CEDARC_UNUSE(p);
    CEDARC_UNUSE(nAdjustDramSpeedFlag);
}

void Vp9EnableVe(void* p)
{
    CEDARC_UNUSE(p);
    return ;
}

void Vp9DisableVe(void* p)
{
    CEDARC_UNUSE(p);
    return ;
}

void Vp9InitDecoderPerformance(void* p, int nMode) //* 0: normal performance; 1. high performance
{
    CEDARC_UNUSE(p);
    if(nMode == 1)
    {
        if(gPerfDecRefCount == 0)
        {
            ioctl(gVP9DriverFd, VP9_IOCTL_SET_HIGH_PERF_MSG, 1);//notify dram begin high perfmance
        }
        gPerfDecRefCount++;
    }
}

void Vp9UninitDecoderPerformance(void* p, int nMode)
{
    CEDARC_UNUSE(p);
    if(nMode == 1)
    {
        gPerfDecRefCount--;
        if(gPerfDecRefCount == 0)
        {
            ioctl(gVP9DriverFd, VP9_IOCTL_SET_HIGH_PERF_MSG, 0);//notify dram end high perfmance
        }
    }
}

int Vp9GetIommuAddr(void* p, struct user_iommu_param *iommu_buffer)
{
    CEDARC_UNUSE(p);
    CEDARC_UNUSE(iommu_buffer);
    int ret;

    ret = ioctl(gVP9DriverFd, IOCTL_GET_IOMMU_ADDR, iommu_buffer);
    return ret;
}

int Vp9FreeIommuAddr(void* p, struct user_iommu_param *iommu_buffer)
{
    CEDARC_UNUSE(p);
    CEDARC_UNUSE(iommu_buffer);
    int ret;

    ret = ioctl(gVP9DriverFd, IOCTL_FREE_IOMMU_ADDR, iommu_buffer);
    return ret;
}

#if VE_IPC
static int aw_set_semvalue()
{
    union semun sem_union;
    unsigned short sem_init_value[2] = {1,1};

    sem_union.array = sem_init_value;
    if(syscall(__NR_semctl, sem_id, 0, SETALL, sem_union)== -1)
        return 0;
    return 1;
}

static void aw_del_semvalue()
{
    union semun sem_union;

    if(syscall(__NR_semctl, sem_id, 0, IPC_RMID, sem_union) == -1)
        fprintf(stderr, "Failed to delete semaphore\n");
}

static int semaphore_p(unsigned short sem_index)
{
    struct sembuf sem_b;
    sem_b.sem_num = sem_index;
    sem_b.sem_op = -1;//P()
    sem_b.sem_flg = SEM_UNDO;
    if(syscall(__NR_semop, sem_id, &sem_b, 1) == -1)
    {
        fprintf(stderr, "semaphore_p failed\n");
        return 0;
    }
    return 1;
}

static int semaphore_v(unsigned short sem_index)
{
    struct sembuf sem_b;
    sem_b.sem_num = sem_index;
    sem_b.sem_op = 1;//V()
    sem_b.sem_flg = SEM_UNDO;
    if(syscall(__NR_semop, sem_id, &sem_b, 1) == -1)
    {
        fprintf(stderr, "semaphore_v failed\n");
        return 0;
    }
    return 1;
}
#endif

VeOpsS _veVp9OpsS =
{
    .init                     = Vp9Initialize,
    .release                  = Vp9Release,
    .lock                     = Vp9Lock,
    .unlock                   = Vp9UnLock,
    .reset                    = Vp9Reset,
    .waitInterrupt            = Vp9WaitInterrupt,

    .getChipId                = Vp9GetChipId,
    .getIcVeVersion           = Vp9GetIcVeVersion,
    .getGroupRegAddr          = Vp9GetGroupRegAddr,
    .getDramType              = Vp9GetDramType,
    .getPhyOffset             = Vp9GetPhyOffset,

    .setDramType              = Vp9SetDramType,
    .setDdrMode               = Vp9SetDdrMode,
    .setSpeed                 = Vp9SetSpeed,
    .setEnableAfbcFlag        = Vp9SetEnableAfbcFlag,
    .setAdjustDramSpeedFlag   = Vp9SetAdjustDramSpeedFlag,

    .enableVe                 = Vp9EnableVe,
    .disableVe                = Vp9DisableVe,

    .initEncoderPerformance   = Vp9InitDecoderPerformance,
    .unInitEncoderPerformance = Vp9UninitDecoderPerformance,

    .getIommuAddr             = Vp9GetIommuAddr,
    .freeIommuAddr            = Vp9FreeIommuAddr
};

VeOpsS* getVeVp9OpsS()
{
    logd("***********3333 getVeVp9OpsS************");
    return &_veVp9OpsS;
}

