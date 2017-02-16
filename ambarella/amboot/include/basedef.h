/**
 * basedef.h
 *
 * History:
 *    2004/10/27 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
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

