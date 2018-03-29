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
* File : h265_common.h
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#ifndef H265_COMMON_H
#define H265_COMMON_H

union unaligned_32 { u32 l; } __attribute__((packed)) ;

#define HEVC_RN(s, p) (((const union unaligned_##s *) (p))->l)
#define HEVC_WN(s, p, v) ((((union unaligned_##s *) (p))->l) = (v))

#define HEVC_RN32(p) HEVC_RN(32, p)
#define HEVC_WN32(p, v) HEVC_WN(32, p, v)

#define HEVC_BSWAP16C(x) (((x) << 8 & 0xff00)  | ((x) >> 8 & 0x00ff))
#define HEVC_BSWAP32C(x) (HEVC_BSWAP16C(x) << 16 | HEVC_BSWAP16C((x) >> 16))
#define HEVC_BSWAP64C(x) (HEVC_BSWAP32C(x) << 32 | HEVC_BSWAP32C((x) >> 32))
#define HEVC_BSWAPC(s, x) HEVC_BSWAP##s##C(x)

#define HEVC_RB(s, p)    hevc_bswap##s(HEVC_RN##s(p))
#define HEVC_WB(s, p, v) HEVC_WN##s(p, hevc_bswap##s(v))
#define HEVC_RL(s, p)    HEVC_RN##s(p)
#define HEVC_WL(s, p, v) HEVC_WN##s(p, v)
#define HEVC_RB32(p)    HEVC_RB(32, p)

#define HEVC_RB8(x)     (((const u8*)(x))[0])
#define HEVC_WB8(p, d)  do { ((u8*)(p))[0] = (d); } while(0)

#define HEVC_RL8(x)     HEVC_RB8(x)
#define HEVC_WL8(p, d)  HEVC_WB8(p, d)
#define HEVC_WL32(p, v) HEVC_WL(32, p, v)

#define MIN_CACHE_BITS 25
#define OPEN_READER(name, gb)                   \
    unsigned int name##_index = (gb)->index;    \
    unsigned int  name##_cache = 0;    \
    unsigned int  name##_size_plus8 =  \
                (gb)->size_in_bits_plus8; \
    CEDARC_UNUSE(name##_cache);    \
    CEDARC_UNUSE(name##_size_plus8)

#define CLOSE_READER(name, gb) (gb)->index = name##_index

#define UPDATE_CACHE(name, gb) name##_cache = \
        HEVC_RB32((gb)->buffer + (name##_index >> 3)) << (name##_index & 7)

#define GET_CACHE(name, gb) ((u32)name##_cache)

#define HEVC_SKIP_COUNTER(name, gb, num) \
    name##_index = HEVCMIN(name##_size_plus8, name##_index + (num))

#define HEVC_NEG_SSR32(a,s) ((( s32)(a))>>(32-(s)))
#define HEVC_NEG_USR32(a,s) (((u32)(a))>>(32-(s)))

#define LAST_SKIP_BITS(name, gb, num) HEVC_SKIP_COUNTER(name, gb, num)
#define SHOW_UBITS(name, gb, num) HEVC_NEG_USR32(name##_cache, num)
#define SHOW_SBITS(name, gb, num) HEVC_NEG_SSR32(name##_cache, num)

#define hevc_log2       hevc_log2_c
#define HEVCMIN(a,b) ((a) > (b) ? (b) : (a))

#endif //H265_COMMON_H

