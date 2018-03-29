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
* File : h264_mvc.c
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#include "h264_dec.h"
#include "h264_hal.h"
#include "h264_func.h"


s32 H264DecodeNalHeaderExt(H264DecCtx* h264DecCtx, H264Context* hCtx)
{
    u8 bSvcExtensionFlag = 0;
    u32 nNextCode = 0;
    H264MvcContext* pMvcContext = NULL;

    nNextCode = h264DecCtx->GetBits((void*)h264DecCtx, 24);
    bSvcExtensionFlag = (nNextCode>>23)& 0x01;
    if(bSvcExtensionFlag == 1)
    {
        logd("nal_unit_header_svc_extension, specified in Annex G\n");
    }
    pMvcContext = (H264MvcContext*)hCtx->pMvcContext;

    pMvcContext->pH264MvcSliceExtParam = (H264MvcSliceExtParam*)pMvcContext->pMvcParamBuf;
    pMvcContext->pH264MvcSliceExtParam->bNonIdrFlag         = (nNextCode>>22) & 0x01;
    pMvcContext->pH264MvcSliceExtParam->nPriorityId         = (nNextCode>>16) & 0x3f;
    pMvcContext->pH264MvcSliceExtParam->nViewId                = (nNextCode>>6)  & 0x3ffff;
    pMvcContext->pH264MvcSliceExtParam->nTemporalId         = (nNextCode>>3)  & 0x7;
    pMvcContext->pH264MvcSliceExtParam->bAnchorPicFlag      = (nNextCode>>2)  & 0x1;
    pMvcContext->pH264MvcSliceExtParam->bInterViewFlag       = (nNextCode>>1)  & 0x1;
    hCtx->bIdrFrmFlag = 1 - pMvcContext->pH264MvcSliceExtParam->bNonIdrFlag;
    return VDECODE_RESULT_OK;
}

