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
* File : CdcSysinfo.c
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
#include <sys/ioctl.h>

#include <CdcSysinfo.h>

#define NODE_DDR_FREQ    "/sys/class/devfreq/sunxi-ddrfreq/cur_freq"
static int readNodeValue(const char *node, char * strVal, size_t size)
{
    int ret = -1;
    int fd = open(node, O_RDONLY);
    if (fd >= 0)
    {
        read(fd, strVal, size);
        close(fd);
        ret = 0;
    }
    return ret;
}

//* get current ddr frequency, if it is too slow, we will cut some spec off.
int CdcGetDramFreq()
{
    CEDARC_UNUSE(CdcGetDramFreq);

    char freq_val[8] = {0};
    if (!readNodeValue(NODE_DDR_FREQ, freq_val, 8))
    {
        return atoi(freq_val);
    }

    // unknow freq
    return -1;
}


