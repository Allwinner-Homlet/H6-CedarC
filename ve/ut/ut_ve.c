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
* File : ut_ve.c
* Description : unit test for ve.c
* History :
*   Author  : BZ <bzchen@allwinnertech.com>
*   Date    : 2016/09/26
*   Comment : Create.
*/

#include <stdio.h>
#include <ve.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>

#define UNUSED(param) (void)param

#define UT_ASSERT(e)                                       \
            do {                                                        \
                if (!(e))                                               \
                {                                                       \
                    printf("check (%s) failed!\n", #e);           \
                    assert(0);                                          \
                }                                                       \
            } while (0)

int TC_VeLock(void)
{
    UNUSED(TC_VeLock);

    int ret = 0;

    ret = VeInitialize();

    UT_ASSERT(ret == 0);

    ret = VeLock();

    if (ret == 0)
    {
        printf(" Get Lock. \n");
        int i;
        for (i = 0; i < 10; i++)
        {
            printf("sleep 2sec... \n");
            usleep(2000000);
        }
        VeUnLock();
    }
    else
    {
        printf("get lock failure, ret:'%d' \n", ret);
    }

    VeRelease();
    return ret;
}

void *__MutilPhreadTest(void *param)
{
    (void)param;
    static int count = 0;
    int id = count++;
    int ret = 0;

    printf("<%d> Test Thread start... \n", id);

    ret = VeInitialize();

    UT_ASSERT(ret == 0);

    while (1)
    {
        printf("<%d> Getting Lock... \n", id);
        ret = VeLock();
        if (ret == 0)
        {
            printf("<%d> Got it, usleep sometime. \n", id);
        }
        usleep(20000);

        printf("<%d> release lock. \n", id);
        VeUnLock();
    }
    VeRelease();

    return NULL;
}

int main()
{
    pthread_t p;

    pthread_create(&p, NULL, __MutilPhreadTest, NULL);
    pthread_create(&p, NULL, __MutilPhreadTest, NULL);
    pthread_create(&p, NULL, __MutilPhreadTest, NULL);

    while (1)
    {
        usleep(10000000);
    }
    return 0;
}

