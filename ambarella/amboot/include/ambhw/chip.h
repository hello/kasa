/**
 * ambhw/chip.h
 *
 * History:
 *	2007/11/29 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AMBHW__CHIP_H__
#define __AMBHW__CHIP_H__

/* ==========================================================================*/
#define A5S		(5100)
#define A7L		(7500)
#define A8		(8000)
#define S2		(9000)
#define S2E		(9100)
#define S2L		(12000)
#define S3		(11000)
#define S3L		(13000)

#define CHIP_ID(x)	((x / 1000))
#define CHIP_MAJOR(x)	((x / 100) % 10)
#define CHIP_MINOR(x)	((x / 10) % 10)

#if defined(CONFIG_ARCH_A5S)
#define CHIP_REV	A5S
#elif defined(CONFIG_ARCH_A7L)
#define CHIP_REV	A7L
#elif defined(CONFIG_ARCH_S2)
#define CHIP_REV	S2
#elif defined(CONFIG_ARCH_S2E)
#define CHIP_REV	S2E
#elif defined(CONFIG_ARCH_A8)
#define CHIP_REV	A8
#elif defined(CONFIG_ARCH_S2L)
#define CHIP_REV	S2L
#elif defined(CONFIG_ARCH_S3)
#define CHIP_REV	S3
#elif defined(CONFIG_ARCH_S3L)
#define CHIP_REV	S3L
#else
#error "Undefined CHIP_REV"
#endif

/* ==========================================================================*/
#if (CHIP_REV == A8)
#define REF_CLK_FREQ			27000000
#else
#define REF_CLK_FREQ			24000000
#endif

/* ==========================================================================*/
#if (CHIP_REV == A5S) || (CHIP_REV == A7L)
#define	CHIP_BROKEN_UNALIGNED_ACCESS	1
#endif

#if (CHIP_REV == S3)
#define	CHIP_FIX_2NDCORTEX_BOOT	1
#endif

/* ==========================================================================*/

#endif

