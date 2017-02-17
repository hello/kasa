/**
 * basedef.h
 *
 * History:
 *    2004/10/27 - [Charles Chiou] created file
 *
 *
 * Copyright (c) 2015 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


#ifndef __BASEDEF_H__
#define __BASEDEF_H__

#ifndef __ASM__

typedef unsigned char		u8;	/**< UNSIGNED 8-bit data type */
typedef unsigned short		u16;	/**< UNSIGNED 16-bit data type */
typedef unsigned int		u32;	/**< UNSIGNED 32-bit data type */
typedef unsigned long long	u64;	/**< UNSIGNED 64-bit data type */
typedef signed char		s8;	/**< SIGNED 8-bit data type */
typedef signed short		s16;	/**< SIGNED 16-bit data type */
typedef signed int		s32;	/**< SIGNED 32-bit data type */
typedef signed long long	s64;	/**< SIGNED 64-bit data type */
typedef unsigned long		uintptr_t;

#ifndef NULL
#define NULL ((void *) 0x0)
#endif

#if defined __cplusplus && defined(__GNUC__)
#undef NULL
#define NULL 0x0
#endif

#endif  /* !__ASM__ */

#ifndef xstr
#define xstr(s) str(s)
#endif

#ifndef str
#define str(s) #s
#endif

#define AMB_VER_NUM(major,minor) ((major << 16) | (minor))
#define AMB_VER_DATE(year,month,day) ((year << 16) | (month << 8) | day)

/* conversion between little-endian and big-endian */
#define uswap_16(x) \
	((((x) & 0xff00) >> 8) | \
	 (((x) & 0x00ff) << 8))
#define uswap_32(x) \
	((((x) & 0xff000000) >> 24) | \
	 (((x) & 0x00ff0000) >> 8) | \
	 (((x) & 0x0000ff00) << 8) | \
	 (((x) & 0x000000ff) << 24))
#define uswap_64(x) \
	((((x) & 0xff00000000000000ull) >> 56) | \
	 (((x) & 0x00ff000000000000ull) >> 40) | \
	 (((x) & 0x0000ff0000000000ull) >> 24) | \
	 (((x) & 0x000000ff00000000ull) >> 8) | \
	 (((x) & 0x00000000ff000000ull) << 8) | \
	 (((x) & 0x0000000000ff0000ull) << 24) | \
	 (((x) & 0x000000000000ff00ull) << 40) | \
	 (((x) & 0x00000000000000ffull) << 56))

#define cpu_to_be16(x)		uswap_16(x)
#define cpu_to_be32(x)		uswap_32(x)
#define cpu_to_be64(x)		uswap_64(x)
#define be16_to_cpu(x)		uswap_16(x)
#define be32_to_cpu(x)		uswap_32(x)
#define be64_to_cpu(x)		uswap_64(x)

#define isspace(c)	((c) == ' ' || ((c) >= '\t' && (c) <= '\r'))
#define isascii(c)	(((c) & ~0x7f) == 0)
#define isupper(c)	((c) >= 'A' && (c) <= 'Z')
#define islower(c)	((c) >= 'a' && (c) <= 'z')
#define isalpha(c)	(isupper(c) || islower(c))
#define isdigit_c(c)	((c) >= '0' && (c) <= '9')
#define isprint(c)	((c) >= 0x20 && (c) <= 0x7e)

#define min(x, y)		((x) < (y) ? (x) : (y))
#define max(x, y)		((x) > (y) ? (x) : (y))
#define CLIP(a, max, min)	((a) > (max)) ? (max) : (((a) < (min)) ? (min) : (a))

#define ARRAY_SIZE(x)		(sizeof(x) / sizeof((x)[0]))
#define AMBOOT_ALIGN(x, a)	(((x) + ((a) - 1)) & ~((a) - 1))

#include <hwio.h>
#include <debug.h>

#endif

