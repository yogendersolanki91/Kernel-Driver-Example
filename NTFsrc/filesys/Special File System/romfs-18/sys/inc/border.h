/*
    This is a romfs file system driver for Windows NT/2000/XP.
    Copyright (C) 1999, 2000, 2001, 2002 Bo Brantén.
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _BORDER_
#define _BORDER_

#if defined(_X86_) || defined(_IA64_) || defined(_AMD64_)
#define __LITTLE_ENDIAN TRUE
#endif

#if !defined(__LITTLE_ENDIAN) && !defined(__BIG_ENDIAN)
#error Define __LITTLE_ENDIAN or __BIG_ENDIAN
#endif

#ifdef __LITTLE_ENDIAN
#define le16_to_cpu(x) (x)
#define le32_to_cpu(x) (x)
#define le64_to_cpu(x) (x)
#define be16_to_cpu(x) RtlUshortByteSwap(x)
#define be32_to_cpu(x) RtlUlongByteSwap(x)
#define be64_to_cpu(x) RtlUlonglongByteSwap(x)
#endif // __LITTLE_ENDIAN

#ifdef __BIG_ENDIAN
#define le16_to_cpu(x) RtlUshortByteSwap(x)
#define le32_to_cpu(x) RtlUlongByteSwap(x)
#define le64_to_cpu(x) RtlUlonglongByteSwap(x)
#define be16_to_cpu(x) (x)
#define be32_to_cpu(x) (x)
#define be64_to_cpu(x) (x)
#endif // __BIG_ENDIAN

#define cpu_to_le16(x) le16_to_cpu(x)
#define cpu_to_le32(x) le32_to_cpu(x)
#define cpu_to_le64(x) le64_to_cpu(x)
#define cpu_to_be16(x) be16_to_cpu(x)
#define cpu_to_be32(x) be32_to_cpu(x)
#define cpu_to_be64(x) be64_to_cpu(x)

//
// The RtlXxxByteSwap functions is missing in the Windows NT 4.0 DDK
//
#if (VER_PRODUCTBUILD < 2195)

#define RtlUshortByteSwap(x)    ___swab16(x)
#define RtlUlongByteSwap(x)     ___swab32(x)
#define RtlUlonglongByteSwap(x) ___swab64(x)

//
// Types used by Linux
//
#include "ltypes.h"

//
// The following is a subset of linux/include/linux/byteorder/swab.h from
// version 2.2.14
//

#define ___swab16(x) \
    ((__u16)( \
        (((__u16)(x) & (__u16)0x00ffU) << 8) | \
        (((__u16)(x) & (__u16)0xff00U) >> 8) ))

#define ___swab32(x) \
    ((__u32)( \
        (((__u32)(x) & (__u32)0x000000ffUL) << 24) | \
        (((__u32)(x) & (__u32)0x0000ff00UL) <<  8) | \
        (((__u32)(x) & (__u32)0x00ff0000UL) >>  8) | \
        (((__u32)(x) & (__u32)0xff000000UL) >> 24) ))

#define ___swab64(x) \
    ((__u64)( \
        (__u64)(((__u64)(x) & (__u64)0x00000000000000ffULL) << 56) | \
        (__u64)(((__u64)(x) & (__u64)0x000000000000ff00ULL) << 40) | \
        (__u64)(((__u64)(x) & (__u64)0x0000000000ff0000ULL) << 24) | \
        (__u64)(((__u64)(x) & (__u64)0x00000000ff000000ULL) <<  8) | \
        (__u64)(((__u64)(x) & (__u64)0x000000ff00000000ULL) >>  8) | \
        (__u64)(((__u64)(x) & (__u64)0x0000ff0000000000ULL) >> 24) | \
        (__u64)(((__u64)(x) & (__u64)0x00ff000000000000ULL) >> 40) | \
        (__u64)(((__u64)(x) & (__u64)0xff00000000000000ULL) >> 56) ))

//
// End of subset of linux/include/linux/byteorder/swab.h
//

#endif // (VER_PRODUCTBUILD < 2195)

#endif
