/**
 * ambhw/chip.h
 *
 * History:
 *	2007/11/29 - [Charles Chiou] created file
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

