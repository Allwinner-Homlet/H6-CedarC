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
* File : h264_mmco.c
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#include "h264_dec.h"
#include "h264_func.h"

#define SWAP(type,a,b) do{type SWAP_tmp= b; b= a; a= SWAP_tmp;}while(0)

/********************************************************************/
/**
 * Executes the nReference picture marking (memory management control operations).
 */
/*******************************************************************/

/**
 * Extract structure information about the picture described by pic_num in
 * the current decoding context (frame or field). Note that pic_num is
 * picture number without wrapping (so, 0<=pic_num<max_pic_num).
 * @param pic_num picture number for which to extract structure information
 * @param structure one of PICT_XXX describing structure of picture
 *                      with pic_num
 * @return frame number (short term) or long term index of picture
 *         described by pic_num
 */
s32 H264PicNumExtract(H264Context* hCtx, s32 picNum, s32 *structure)
{
    *structure = hCtx->nPicStructure;
    if(hCtx->nPicStructure != PICT_FRAME)
    {
        if (!(picNum & 1))
            /* opposite field */
            *structure ^= PICT_FRAME;
        picNum >>= 1;
    }
    return picNum;
}

//********************************************************************************//
/**********************************************************************************
 * Mark a picture as no longer needed for nReference. The refmask
 * argument allows unreferencing of individual fields or the whole frame.
 * If the picture becomes entirely unnReferenced, but is being held for
 * display purposes, it is marked as such.
 * @param refmask mask of fields to unnReference; the mask is bitwise
 *                anded with the ref erence marking of pic
 * @return non-zero if pic becomes entirely unnReferenced (except possibly
 *         for display purposes) zero if one of the fields remains in
 *         nReference
 *********************************************************************************/
//********************************************************************************//
static inline s32 H264UnnReferencePic(H264Context* hCtx, H264PicInfo* pic, s32 refmask)
{
    s32 i = 0;

    if(pic->nReference &= refmask)
    {
        if((pic->nReference==0||pic->nReference==4)&&(pic->pVPicture!=NULL))
         {
            if(pic->bHasDispedFlag==1 || (pic->pScaleDownVPicture!=NULL))
            {
                FbmReturnBuffer(hCtx->pFbm, pic->pVPicture, 0);
                if(pic->nDispBufIndex < MAX_PICTURE_COUNT)
                {
                    hCtx->frmBufInf.picture[pic->nDispBufIndex].pVPicture = NULL;
                    if((hCtx->pVbv->bUseNewVeMemoryProgram==1)&&ENABLE_MV_BUF_OPTIMIZATION_PROGRAM)
                    {
                        hCtx->pH264MvBufInf[pic->nH264MvBufIndex].pH264MvBufManager->nCalculateNum =
                                   INIT_MV_BUF_CALCULATE_NUM;
                        FIFOEnqueue((FiFoQueueInst**)&(hCtx->pH264MvBufEmptyQueue),
                                 (FiFoQueueInst*)&(hCtx->pH264MvBufInf[pic->nH264MvBufIndex]));
                    }
                }
                pic->pVPicture = NULL;
                pic->bHasDispedFlag = 0;
                pic->nReference = 0;
                pic->bCodedFrame = 0;
            }
         }
        return 0;
    }
    else
    {
        if(pic == hCtx->frmBufInf.pDelayedOutPutPic)
        {
            pic->nReference = DELAYED_PIC_REF;
        }
        else
        {
            for(i = 0; hCtx->frmBufInf.pDelayedPic[i]; i++)
            {
                if((i<MAX_PICTURE_COUNT)&&(pic== hCtx->frmBufInf.pDelayedPic[i]))
                {
                    pic->nReference = DELAYED_PIC_REF;
                    break;
                }
            }
        }

        if((pic->nReference==0||pic->nReference==4)&&(pic->pVPicture!=NULL))
        {
            if(pic->bHasDispedFlag==1 || (pic->pScaleDownVPicture!=NULL))
            {
                FbmReturnBuffer(hCtx->pFbm, pic->pVPicture, 0);
                if(pic->nDispBufIndex < MAX_PICTURE_COUNT)
                {
                    hCtx->frmBufInf.picture[pic->nDispBufIndex].pVPicture = NULL;
                    if((hCtx->pVbv->bUseNewVeMemoryProgram == 1)&&  \
						ENABLE_MV_BUF_OPTIMIZATION_PROGRAM)
                    {
                        hCtx->pH264MvBufInf[pic->nH264MvBufIndex].pH264MvBufManager->nCalculateNum =
                                   INIT_MV_BUF_CALCULATE_NUM;
                        FIFOEnqueue((FiFoQueueInst**)&(hCtx->pH264MvBufEmptyQueue),
                                 (FiFoQueueInst*)&(hCtx->pH264MvBufInf[pic->nH264MvBufIndex]));
                    }
                }
                pic->pVPicture = NULL;
                pic->bHasDispedFlag = 0;
                pic->nReference = 0;
            }
        }
        return 1;
    }
}

void H264ReferenceRefresh(H264Context* hCtx)
{
    s32 i = 0;

    for(i=0; i<hCtx->nLongRefCount; i++)
    {
        if((i<MAX_PICTURE_COUNT) && (hCtx->frmBufInf.pLongRef[i]!= NULL))
        {
            H264UnnReferencePic(hCtx, hCtx->frmBufInf.pLongRef[i], 0);
            hCtx->frmBufInf.pLongRef[i] = NULL;
        }
    }

    hCtx->nLongRefCount = 0;

    for(i=0; i<hCtx->nShortRefCount; i++)
    {
        if(i<MAX_PICTURE_COUNT)
        {
            H264UnnReferencePic(hCtx, hCtx->frmBufInf.pShortRef[i], 0);
            hCtx->frmBufInf.pShortRef[i] = NULL;
        }
    }
    hCtx->nShortRefCount = 0;
    if((hCtx->pVbv!= NULL) &&(hCtx->pVbv->bUseNewVeMemoryProgram==1)&&     \
      ENABLE_MV_BUF_OPTIMIZATION_PROGRAM)
    {
         H264MvBufInf*  pH264MvBufNode = NULL;
         while(1)
         {
            pH264MvBufNode = (H264MvBufInf*)FIFODequeue((FiFoQueueInst**)&    \
               (hCtx->pH264MvBufEmptyQueue));
            if(pH264MvBufNode == NULL)
            {
                break;
            }
            usleep(1);
        }
        for(i=0; i<hCtx->nH264MvBufNum; i++)
        {
            hCtx->pH264MvBufInf[i].pH264MvBufManager->nCalculateNum =
                                   INIT_MV_BUF_CALCULATE_NUM;
            FIFOEnqueue((FiFoQueueInst**)&(hCtx->pH264MvBufEmptyQueue),
                         (FiFoQueueInst*)&hCtx->pH264MvBufInf[i]);
        }
  }
}

static s32 H264SplitFieldCopy(H264PicInfo *dest, H264PicInfo *src, s32 parity, s32 idAdd)
{
    s32 match = !!(src->nReference & parity);

    if(match)
    {
        //COPY_PICTURE(dest, src);
        memcpy(dest, src, sizeof(H264PicInfo));
        if(parity != PICT_FRAME)
        {
            if(parity == PICT_BOTTOM_FIELD)
            {
                dest->nPicStructure = PICT_BOTTOM_FIELD;
            }
            else
            {
                dest->nPicStructure = PICT_TOP_FIELD;
            }
            dest->nReference = parity;
            dest->nPicId *= 2;
            dest->nPicId += idAdd;
        }
    }

    return match;
}

static s32 H264BuildDefList(H264PicInfo *def, H264PicInfo **in, s32 nLen, s32 bIsLong, s32 sel)
{
    s32 i[2] = {0};
    s32 index = 0;

    while(i[0]<nLen || i[1]<nLen)
    {
        while(i[0]<nLen && !(in[i[0]] && (in[i[0]]->nReference & sel)))
            i[0]++;
        while(i[1]<nLen && !(in[i[1]] && (in[i[1]]->nReference & (sel ^ 3))))
            i[1]++;
        if(i[0] < nLen)
        {
            in[i[0]]->nPicId = bIsLong? i[0] : in[i[0]]->nFrmNum;
            H264SplitFieldCopy(&def[index++], in[i[0]++], sel, 1);
        }
        if (i[1] < nLen)
        {
            in[i[1]]->nPicId = bIsLong ? i[1] : in[i[1]]->nFrmNum;
            H264SplitFieldCopy(&def[index++], in[i[1]++], sel ^ 3, 0);
        }
    }

    return index;
}

static s32 H264AddSorted(H264PicInfo **sorted, H264PicInfo **src, s32 len, s32 limit, s32 dir)
{
    #define INTMAX (1<<30)
    #define INTMIN (-1<<30)

    s32 i, bestPoc;
    s32 outI = 0;

    for(;;)
    {
        bestPoc = dir ? INTMIN : INTMAX;
        for(i = 0; i < len; i++)
        {
            const int poc = src[i]->nPoc;
            if (((poc > limit) ^ dir) && ((poc < bestPoc) ^ dir))
            {
                bestPoc   = poc;
                sorted[outI] = src[i];
            }
        }
        if (bestPoc == (dir ? INTMIN : INTMAX))
        {
            break;
        }
        limit = sorted[outI++]->nPoc - dir;
    }
    return outI;
}

s32 H264FillDefaultRefList(H264Context* hCtx, H264PicInfo* pMajorRefFrame, u8 nDecStreamIndex)
{
    s32 i = 0;
    s32 nLen = 0;
    s32 nCurPoc = 0;
    s32 nList = 0;
    //s32 ret = 0;
    s32 nLens[2];
    H264PicInfo *refPictureBuf[1];
    u32 index = 0;

    if(hCtx->nSliceType == H264_B_TYPE)
    {
        if(hCtx->nPicStructure != PICT_FRAME)
        {
            index = hCtx->nPicStructure==PICT_BOTTOM_FIELD;
            nCurPoc = hCtx->frmBufInf.pCurPicturePtr->nFieldPoc[index];
        }
        else
        {
            nCurPoc = hCtx->frmBufInf.pCurPicturePtr->nPoc;
        }

        for(nList = 0; nList < 2; nList++)
        {
            H264PicInfo *sorted[32];
            nLen  = H264AddSorted(sorted,hCtx->frmBufInf.pShortRef,
                hCtx->nShortRefCount, nCurPoc, 1 ^ nList);

            nLen += H264AddSorted(sorted+ nLen,
                hCtx->frmBufInf.pShortRef,hCtx->nShortRefCount, nCurPoc, 0 ^ nList);

            nLen  = H264BuildDefList(hCtx->frmBufInf.defaultRefList[nList],
                sorted, nLen, 0, hCtx->nPicStructure);
            nLen += H264BuildDefList(hCtx->frmBufInf.defaultRefList[nList]+nLen,
                hCtx->frmBufInf.pLongRef, 16, 1, hCtx->nPicStructure);

            if(nDecStreamIndex == 1 && (pMajorRefFrame->bLongRef==0))
            {
                refPictureBuf[0] = pMajorRefFrame;
                nLen += H264BuildDefList(hCtx->frmBufInf.defaultRefList[nList]+nLen,
                    refPictureBuf, 1, 0, hCtx->nPicStructure);
            }

            if (nLen < hCtx->nRefCount[nList])
            {
                memset(&hCtx->frmBufInf.defaultRefList[nList][nLen],
                    0, sizeof(H264PicInfo)*(hCtx->nRefCount[nList]-nLen));
            }
            nLens[nList] = nLen;
        }

        if(nLens[0] == nLens[1] && nLens[1] > 1)
        {
            for (i = 0; hCtx->frmBufInf.defaultRefList[0][i].nDispBufIndex==
                        hCtx->frmBufInf.defaultRefList[1][i].nDispBufIndex && i < nLens[0]; i++);
            if (i == nLens[0])
            {
                SWAP(H264PicInfo, hCtx->frmBufInf.defaultRefList[1][0],
                    hCtx->frmBufInf.defaultRefList[1][1]);
            }
        }
    }
    else
    {
        nLen  = H264BuildDefList(hCtx->frmBufInf.defaultRefList[0],
            hCtx->frmBufInf.pShortRef, hCtx->nShortRefCount, 0, hCtx->nPicStructure);
        nLen += H264BuildDefList(hCtx->frmBufInf.defaultRefList[0]+nLen,
            hCtx->frmBufInf.pLongRef,16, 1, hCtx->nPicStructure);

        if(nDecStreamIndex == 1 && (pMajorRefFrame->bLongRef==0))
        {
            refPictureBuf[0] = pMajorRefFrame;
            nLen += H264BuildDefList(hCtx->frmBufInf.defaultRefList[nList]+nLen,
                refPictureBuf, 1, 0, hCtx->nPicStructure);
        }
        if ((nLen < hCtx->nRefCount[0])&&(nLen<MAX_PICTURE_COUNT))
        {
            memset(&hCtx->frmBufInf.defaultRefList[0][nLen], 0,
                sizeof(H264PicInfo) * (hCtx->nRefCount[0]-nLen));
        }
    }
    return 0;
}

void H264DirectRefListInit(H264Context* hCtx)
{
    H264PicInfo* cur  = NULL;
    s32 list = 0;
    s32 j = 0;

    cur = hCtx->frmBufInf.pCurPicturePtr;

    if(cur->nPictType == H264_I_TYPE)
    {
        cur->nRefCount[0] = 0;
    }
    if(cur->nPictType != H264_B_TYPE)
    {
        cur->nRefCount[1] = 0;
    }
    for(list=0; list<2; list++)
    {
        cur->nRefCount[list] = hCtx->nRefCount[list];
        for(j=0; j<hCtx->nRefCount[list]; j++)
        {
            if(j >= 16)
            {
                continue;
            }
            cur->nRefPoc[list][j] = hCtx->frmBufInf.refList[list][j].nPoc;
        }
    }
}

s32 H264DecodeRefPicMarking(H264DecCtx* h264DecCtx, H264Context* hCtx)
{
    s32 i = 0;
    u32 bLongArg = 0;
    h264_mmco_opcode opCode;

    //if(hCtx->nalUnitType == NAL_IDR_SLICE)
    hCtx->nMmcoIndex = 0;

    if(hCtx->bIdrFrmFlag == 1)
    {
        h264DecCtx->GetBits((void*)h264DecCtx, 1);
        hCtx->mmco[0].bLongArg = h264DecCtx->GetBits((void*)h264DecCtx, 1)-1;
        // current_long_term_index
        if(hCtx->mmco[0].bLongArg == -1)
        {
            hCtx->mmco[0].bLongArg = 0;
        }
        else
        {
            hCtx->mmco[0].opCode = MMCO_LONG;
            hCtx->nMmcoIndex = 1;
        }
    }
    else
    {
        if(h264DecCtx->GetBits((void*)h264DecCtx, 1))
        {
            for(i=0; i<MAX_MMCO_COUNT; i++)
            {
                opCode = h264DecCtx->GetUeGolomb((void*)h264DecCtx);
                hCtx->mmco[i].opCode = opCode;
                if(opCode==MMCO_SHORT_TO_UNUSED || opCode==MMCO_SHORT_TO_LONG)
                {
                    hCtx->mmco[i].nShortPicNum  =
                        (hCtx->nCurPicNum-h264DecCtx->GetUeGolomb((void*)h264DecCtx)-1) &
                        (hCtx->nMaxPicNum-1);
                }
                if(opCode==MMCO_SHORT_TO_LONG || opCode==MMCO_LONG_TO_UNUSED ||
                    opCode==MMCO_LONG ||opCode==MMCO_SET_MAX_LONG)
                {
                    bLongArg = h264DecCtx->GetUeGolomb((void*)h264DecCtx);
                    if(bLongArg>32 ||(bLongArg>=16&&
                        !(opCode==MMCO_LONG_TO_UNUSED&&hCtx->nPicStructure!=PICT_FRAME)))
                    {
                        return VRESULT_ERR_FAIL;
                    }
                    hCtx->mmco[i].bLongArg = bLongArg;
                }
                if(opCode == MMCO_END)
                {
                    break;
                }
            }
            hCtx->nMmcoIndex = i;
        }
        else
        {
            hCtx->nMmcoIndex = 0;
            if((hCtx->nShortRefCount&&
                hCtx->nLongRefCount+hCtx->nShortRefCount==hCtx->pCurSps->nRefFrmCount)&&
                !(hCtx->nPicStructure!=PICT_FRAME &&
                !hCtx->bFstField&&hCtx->frmBufInf.pCurPicturePtr->nReference))
            {
                hCtx->mmco[0].opCode = MMCO_SHORT_TO_UNUSED;
                if((hCtx->nShortRefCount-1) >= MAX_PICTURE_COUNT)
                {
                    abort();
                }
                hCtx->mmco[0].nShortPicNum =
                    hCtx->frmBufInf.pShortRef[hCtx->nShortRefCount-1]->nFrmNum;
                hCtx->nMmcoIndex = 1;

                if(hCtx->nPicStructure != PICT_FRAME)
                {
                    hCtx->mmco[0].nShortPicNum *= 2;
                    hCtx->mmco[1].opCode = MMCO_SHORT_TO_UNUSED;
                    hCtx->mmco[1].nShortPicNum = hCtx->mmco[0].nShortPicNum + 1;
                    hCtx->nMmcoIndex = 2;
                }
            }
        }
    }
    return 0;
}

s32 H264ReorderInterviewPic(H264DecCtx* h264DecCtx,
                        H264Context* hCtx,
                        s32 listIdx,
                        s32 refIdxLx)
{
    s32 cIdx = 0;
    s32 nIdx = 0;
    H264Dec* pH264Dec = NULL;
    H264PicInfo* pPicLX = NULL;
    pH264Dec = h264DecCtx->pH264Dec;

    if(pH264Dec->pMajorRefFrame!=NULL && pH264Dec->pMajorRefFrame->bLongRef!=1)
    {
        pPicLX = pH264Dec->pMajorRefFrame;
    }
    if(pPicLX != NULL)
    {
        for(cIdx = hCtx->nRefCount[listIdx]; cIdx>refIdxLx; cIdx--)
        {
            if((cIdx>=48) || (listIdx>=2))
            {
                continue;
            }
            hCtx->frmBufInf.refList[listIdx][cIdx] =
                hCtx->frmBufInf.refList[listIdx][cIdx-1];
        }
        hCtx->frmBufInf.refList[listIdx][refIdxLx++] = *pPicLX;
        nIdx = refIdxLx;

        for(cIdx=refIdxLx; cIdx<=hCtx->nRefCount[listIdx]; cIdx++)
        {
            if((cIdx>=48) || (listIdx>=2))
            {
                continue;
            }
            if((hCtx->frmBufInf.refList[listIdx][cIdx].bLongRef==1)||
                (hCtx->frmBufInf.refList[listIdx][cIdx].nFrmNum!=(s32)hCtx->nCurPicNum))
            {
                hCtx->frmBufInf.refList[listIdx][nIdx++] =
                    hCtx->frmBufInf.refList[listIdx][cIdx];
            }
        }
    }
    return 0;
}

s32 H264DecodeRefPicListReordering(H264DecCtx* h264DecCtx, H264Context* hCtx)
{
    s32 i = 0;
    s32 frameNum = 0;
    s32 list = 0;
    s32 index = 0;
    s32 pPicStructure = 0;
    s32 pred = 0;
    s32 longIdx = 0;
    u32 nPicId = 0;
    u32 reorderingOfPicNumsIdc = 0;
    u32 absDiffPicNum = 0;
    H264PicInfo* ref = NULL;
    u8 updateRefListFlag = 0;

    if(hCtx->nSliceType==H264_I_TYPE || hCtx->nSliceType==H264_SI_TYPE)
    {
        return 0; //FIXME move before func
    }

    for(list=0; list<(s32)hCtx->nListCount; list++)
    {
        if(list >= 2)
        {
            continue;
        }
        memcpy(hCtx->frmBufInf.refList[list],
            hCtx->frmBufInf.defaultRefList[list],
            sizeof(H264PicInfo)*hCtx->nRefCount[list]);
        updateRefListFlag = h264DecCtx->GetBits((void*)h264DecCtx, 1);

        if(updateRefListFlag == 1)
        {
            pred = hCtx->nCurPicNum;

            for(index=0; ; index++)
            {
                reorderingOfPicNumsIdc = h264DecCtx->GetUeGolomb((void*)h264DecCtx);

                if(reorderingOfPicNumsIdc==3)
                {
                    break;
                }
                if(index >= hCtx->nRefCount[list])
                {
                    return VRESULT_ERR_FAIL;
                }

                if(reorderingOfPicNumsIdc < 3)
                {
                    if(reorderingOfPicNumsIdc < 2)
                    {
                        absDiffPicNum = h264DecCtx->GetUeGolomb((void*)h264DecCtx) + 1;
                        if(absDiffPicNum > hCtx->nMaxPicNum)
                        {
                            return VRESULT_ERR_FAIL;
                        }
                        if(reorderingOfPicNumsIdc == 0)
                        {
                            pred -= absDiffPicNum;
                        }
                        else
                        {
                            pred += absDiffPicNum;
                        }
                        pred &= hCtx->nMaxPicNum - 1;
                        frameNum = H264PicNumExtract(hCtx, pred, &pPicStructure);

                        for(i= hCtx->nShortRefCount-1; i>=0; i--)
                        {
                            if(i >= MAX_PICTURE_COUNT)
                            {
                                abort();
                            }
                            ref = hCtx->frmBufInf.pShortRef[i];
                            if(ref->pVPicture!= NULL &&
                                ref->nFrmNum == frameNum &&
                                (ref->nReference & pPicStructure) &&
                                ref->bLongRef == 0)
                                // ignore non existing pictures by testing data[0] pointe
                            {
                                break;
                            }
                        }
                        if(i>=0)
                        {
                            ref->nPicId = pred;
                        }
                    }
                    else
                    {
                        nPicId = h264DecCtx->GetUeGolomb((void*)h264DecCtx); //long_term_pic_idx
                        longIdx = H264PicNumExtract(hCtx, nPicId, &pPicStructure);
                        if(longIdx >= MAX_PICTURE_COUNT)
                        {
                            return VRESULT_ERR_FAIL;
                        }
                        ref = hCtx->frmBufInf.pLongRef[longIdx];

                        if(ref && (ref->nReference & pPicStructure))
                        {
                            ref->nPicId = nPicId;
                            i = 0;
                        }
                        else
                        {
                            i = -1;
                        }
                    }

                    if(i < 0)
                    {
                        if((list>=2) || (index>=48))
                        {
                            abort();
                        }
                        memset(&hCtx->frmBufInf.refList[list][index],
                            0, sizeof(H264PicInfo)); //FIXME
                        return VRESULT_ERR_FAIL;
                    }
                    else
                    {
                        for(i=index; i+1<hCtx->nRefCount[list]; i++)
                        {
                            if(i>=48)
                            {
                                abort();
                            }
                            if(ref->bLongRef == hCtx->frmBufInf.refList[list][i].bLongRef &&
                                ref->nPicId == hCtx->frmBufInf.refList[list][i].nPicId)
                            {
                                break;
                            }
                        }
                        for(; i > index; i--)
                        {
                            if(i>=48)
                            {
                                abort();
                            }
                            hCtx->frmBufInf.refList[list][i]=
                                hCtx->frmBufInf.refList[list][i-1];
                        }
                        if(index>=48)
                        {
                            abort();
                        }
                        hCtx->frmBufInf.refList[list][index]= *ref;
                        if(hCtx->nPicStructure != PICT_FRAME)
                        {
                            hCtx->frmBufInf.refList[list][index].nReference =
                                pPicStructure;
                            hCtx->frmBufInf.refList[list][index].nPicStructure =
                                pPicStructure;
                        }
                    }
                }
                else if(reorderingOfPicNumsIdc==4|| reorderingOfPicNumsIdc==5)
                {
                    h264DecCtx->GetUeGolomb((void*)h264DecCtx);
                    H264ReorderInterviewPic(h264DecCtx, hCtx, list, index);
                }
            }
        }
    }

    H264DirectRefListInit(hCtx);
    return 0;
}

//******************************************************************//
//************************************* ****************************//

/**
 * Find a Picture in the short term nReference list by frame number.
 * @param frame_num frame number to search for
 * @param idx the index into h->short_ref where returned picture is found
 *            undefined if no picture found.
 * @return pointer to the found picture, or NULL if no pic with the provided
 *                 frame number is found
 */
static H264PicInfo* H264FindShort(H264Context* hCtx, s32 frameNum, s32 *idx)
{
    s32 i = 0;
    H264PicInfo *pic = NULL;

    for(i=0; i<hCtx->nShortRefCount; i++)
    {
        if(i>=MAX_PICTURE_COUNT)
        {
            continue;
        }
        pic = hCtx->frmBufInf.pShortRef[i];
        if(pic->nFrmNum == frameNum)
        {
            *idx = i;
            return pic;
        }
    }
    return NULL;
}

/**
 * Remove a picture from the short term nReference list by its index in
 * that list.  This does no checking on the provided index; it is assumed
 * to be valid. Other list entries are shifted down.
 * @param i index into h->short_ref of picture to remove.
 */
static void H264RemoveShort_at_index(H264Context* hCtx, s32 i)
{
    if(i >= MAX_PICTURE_COUNT)
    {
        abort();
    }
    hCtx->frmBufInf.pShortRef[i]= NULL;
    if (--hCtx->nShortRefCount)
    {
        if((i+1) >= MAX_PICTURE_COUNT)
        {
            abort();
        }
        memmove(&hCtx->frmBufInf.pShortRef[i],
            &hCtx->frmBufInf.pShortRef[i+1],
            (hCtx->nShortRefCount-i)*sizeof(H264PicInfo*));
    }
}

/**
 *
 * @return the removed picture or NULL if an error occurs
 */
static H264PicInfo * H264RemoveShort(H264Context* hCtx, s32 frameNum)
{
    H264PicInfo *pic = NULL;
    s32 i = 0;

    pic = H264FindShort(hCtx, frameNum, &i);
    if(pic)
    {
        H264RemoveShort_at_index(hCtx, i);
    }
    return pic;
}

/**
 * Remove a picture from the long term nReference list by its index in
 * that list.  This does no checking on the provided index; it is assumed
 * to be valid. The removed entry is set to NULL. Other entries are unaffected.
 * @param i index into h->long_ref of picture to remove.
 */
static void H264RemoveLong_at_index(H264Context* hCtx, s32 i)
{
    if(i >= MAX_PICTURE_COUNT)
    {
        abort();
    }
    hCtx->frmBufInf.pLongRef[i]= NULL;
    hCtx->nLongRefCount--;
}

/**
 *
 * @return the removed picture or NULL if an error occurs
 */
static H264PicInfo* H264RemoveLong(H264Context* hCtx, s32 i)
{
    H264PicInfo *pic = NULL;
    if(i >= MAX_PICTURE_COUNT)
    {
        abort();
    }
    pic = hCtx->frmBufInf.pLongRef[i];
    if(pic)
    {
        H264RemoveLong_at_index(hCtx, i);
    }
    return pic;
}

s32 H264ExecuteRefPicMarking(H264Context* hCtx, H264MmcoInfo* mmco, s32 mmcoCount)
{
    s32 i = 0;
    s32 j = 0;
    s32 structure = 0;
    s32 frameNum = 0;
    s32 unRefPic = 0;
    s32 currentRefAssigned=0;
    H264PicInfo *pic = NULL;

    for(i=0; i<mmcoCount; i++)
    {
        if(i>= MAX_MMCO_COUNT)
        {
            continue;
        }
        switch(mmco[i].opCode)
        {
            case MMCO_SHORT_TO_UNUSED:
            {
                frameNum = H264PicNumExtract(hCtx, mmco[i].nShortPicNum, &structure);
                pic = H264FindShort(hCtx, frameNum, &j);
                if(pic)
                {
                    if(H264UnnReferencePic(hCtx, pic, structure ^ PICT_FRAME))
                    {
                        H264RemoveShort_at_index(hCtx, j);
                    }
                }
                break;
            }
            case MMCO_SHORT_TO_LONG:
            {
                if(mmco[i].bLongArg >= MAX_PICTURE_COUNT)
                {
                    abort();
                }
                if((hCtx->frmBufInf.pLongRef[mmco[i].bLongArg]!=NULL)&&
                    (hCtx->nPicStructure!=PICT_FRAME) &&
                    mmco[i].bLongArg < hCtx->nLongRefCount &&
                     hCtx->frmBufInf.pLongRef[mmco[i].bLongArg]->nFrmNum==
                     (mmco[i].nShortPicNum>>1))
                {
                    /* do nothing, we've already moved this field pair. */
                }
                else
                {
                    frameNum = mmco[i].nShortPicNum>>(hCtx->nPicStructure!=PICT_FRAME);

                    pic = H264RemoveLong(hCtx, mmco[i].bLongArg);
                    if(pic)
                    {
                        H264UnnReferencePic(hCtx, pic, 0);
                    }

                    hCtx->frmBufInf.pLongRef[mmco[i].bLongArg]= H264RemoveShort(hCtx, frameNum);
                    if(hCtx->frmBufInf.pLongRef[mmco[i].bLongArg])
                    {
                        hCtx->frmBufInf.pLongRef[mmco[i].bLongArg]->bLongRef =1;
                        hCtx->nLongRefCount++;
                    }
                }
                break;
            }
            case MMCO_LONG_TO_UNUSED:
            {
                j = H264PicNumExtract(hCtx, mmco[i].bLongArg, &structure);
                if(j >= MAX_PICTURE_COUNT)
                {
                    abort();
                }
                pic = hCtx->frmBufInf.pLongRef[j];
                if(pic)
                {
                    if(H264UnnReferencePic(hCtx, pic, structure ^ PICT_FRAME))
                    {
                        H264RemoveLong_at_index(hCtx, j);
                    }
                }
                break;
            }
            case MMCO_LONG:
            {
                if(mmco[i].bLongArg >= MAX_PICTURE_COUNT)
                {
                    abort();
                }
                unRefPic = 1;
                if((hCtx->nPicStructure!=PICT_FRAME) && !hCtx->bFstField)
                {
                    if(hCtx->frmBufInf.pLongRef[mmco[i].bLongArg] == hCtx->frmBufInf.pCurPicturePtr)
                    {
                            /* Just mark second field as nReferenced */
                        unRefPic = 0;
                    }
                    else if(hCtx->frmBufInf.pCurPicturePtr->nReference)
                    {
                    /* First field in pair is in short term list or
                     * at a different long term index.
                     * This is not allowed; see 7.4.3, notes 2 and 3.
                     * Report the problem and keep the pair where it is,
                     * and mark this field valid.
                     */
                        unRefPic = 0;
                    }
                }

                if(unRefPic)
                {
                    pic = H264RemoveLong(hCtx, mmco[i].bLongArg);
                    if(pic)
                    {
                        H264UnnReferencePic(hCtx, pic, 0);
                    }
                    if(mmco[i].bLongArg >= MAX_PICTURE_COUNT)
                    {
                       abort();
                    }
                    hCtx->frmBufInf.pLongRef[mmco[i].bLongArg]= hCtx->frmBufInf.pCurPicturePtr;
                    hCtx->frmBufInf.pLongRef[mmco[i].bLongArg]->bLongRef = 1;
                    hCtx->nLongRefCount++;
                }

                hCtx->frmBufInf.pCurPicturePtr->nReference |= hCtx->nPicStructure;
                currentRefAssigned=1;
                break;
            }
            case MMCO_SET_MAX_LONG:
            {
                // just remove the long term which index is greater than new max
                for(j=mmco[i].bLongArg; j<16; j++)
                {
                    pic = H264RemoveLong(hCtx, j);
                    if(pic)
                    {
                        H264UnnReferencePic(hCtx, pic, 0);
                    }
                }
                break;
            }
            case MMCO_RESET:
            {
                while(hCtx->nShortRefCount)
                {
                    pic = H264RemoveShort(hCtx, hCtx->frmBufInf.pShortRef[0]->nFrmNum);
                    if(pic)
                    {
                        H264UnnReferencePic(hCtx, pic, 0);
                    }
                }
                for(j = 0; j < 16; j++)
                {
                    pic = H264RemoveLong(hCtx, j);
                    if(pic)
                    {
                        H264UnnReferencePic(hCtx, pic, 0);
                    }
                }
                break;
            }
            default: break;
        }
    }

    if(!currentRefAssigned &&(hCtx->nPicStructure!=PICT_FRAME) &&
            !hCtx->bFstField && hCtx->frmBufInf.pCurPicturePtr->nReference)
    {

        /* Second field of complementary field pair; the first field of
         * which is already nReferenced. If short nReferenced, it
         * should be first entry in short_ref. If not, it must exist
         * in long_ref; trying to put it on the short list here is an
         * error in the encoded bit stream (ref: 7.4.3, NOTE 2 and 3).
         */
        if(hCtx->nShortRefCount && hCtx->frmBufInf.pShortRef[0] == hCtx->frmBufInf.pCurPicturePtr)
        {
            /* Just mark the second field valid */
            hCtx->frmBufInf.pCurPicturePtr->nReference = PICT_FRAME;
        }
        currentRefAssigned = 1;
    }

    if(!currentRefAssigned)
    {
        pic = H264RemoveShort(hCtx, hCtx->frmBufInf.pCurPicturePtr->nFrmNum);
        if(pic)
        {
            H264UnnReferencePic(hCtx, pic, 0);
        }

        if(hCtx->nShortRefCount)
        {
            memmove(&hCtx->frmBufInf.pShortRef[1],
                &hCtx->frmBufInf.pShortRef[0],
                hCtx->nShortRefCount*sizeof(H264PicInfo*));
        }

        hCtx->frmBufInf.pShortRef[0]= hCtx->frmBufInf.pCurPicturePtr;
        hCtx->frmBufInf.pShortRef[0]->bLongRef=0;
        hCtx->nShortRefCount++;
        hCtx->frmBufInf.pCurPicturePtr->nReference |= hCtx->nPicStructure;
    }

    if(hCtx->nLongRefCount + hCtx->nShortRefCount > hCtx->pCurSps->nRefFrmCount)
    {

        /* We have too many nReference frames, probably due to corrupted
         * stream. Need to discard one frame. Prevents overrun of the
         * short_ref and long_ref buffers.
         */

        if((hCtx->nLongRefCount) && (hCtx->nShortRefCount==0))
        {
            for(i = 0; i < 16; ++i)
            {
                if(hCtx->frmBufInf.pLongRef[i])
                {
                    break;
                }
            }
            pic = hCtx->frmBufInf.pLongRef[i];
            H264RemoveLong_at_index(hCtx, i);
        }
        else
        {
            pic = hCtx->frmBufInf.pShortRef[hCtx->nShortRefCount-1];
            H264RemoveShort_at_index(hCtx, hCtx->nShortRefCount-1);
        }
        if(pic)
        {
            H264UnnReferencePic(hCtx, pic, 0);
        }
    }

#if 0
    H264PrintShortTerm(hCtx);
    H264PrintLongTerm(hCtx);
#endif
    return 0;
}
