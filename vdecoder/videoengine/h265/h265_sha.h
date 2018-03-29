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
* File : h265_sha.h
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2017/04/13
*   Comment :
*
*
*/

#include   <stdio.h>
#include   <string.h>

typedef   struct {

    unsigned   int  state[5];

    unsigned   int   count[2];

    unsigned   char   buffer[64];

}   SHA1_CTX;


void   SHA1Final(unsigned   char   digest[20],   SHA1_CTX*   context)   ;
void   SHA1Update(SHA1_CTX*   context,   unsigned   char*   data,   unsigned   int   len) ;
void   SHA1Init(SHA1_CTX*   context) ;
void   SHA1Transform(unsigned int   state[5],   unsigned   char   buffer[64])  ;
