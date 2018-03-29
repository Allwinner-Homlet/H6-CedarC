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
* File : veAdapter.c
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#include "veInterface.h"
#include "veAdapter.h"
#include "veAw/veAwEntry.h"
#include "veVp9/veVp9Entry.h"
#include "log.h"

VeOpsS* GetVeOpsS(int type)
{

    if(type == VE_OPS_TYPE_NORMAL)
    {
        return getVeAwOpsS();
    }
    else if(type == VE_OPS_TYPE_VP9)
    {
        return getVeVp9OpsS();
    }
    else
    {
        loge("getVeOpsS: not suppurt type = %d",type);
        return NULL;
    }
}

