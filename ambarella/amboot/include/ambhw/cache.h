/**
 * ambhw/cache.h
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
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


#ifndef __AMBHW__CACHE_H__
#define __AMBHW__CACHE_H__

/*===========================================================================*/
#define ENABLE_DRAIN_WRITE_BUF

/*===========================================================================*/
#define CLINE	5			/* 32 byte cache line size */
#define NWAY	2			/* 4 way cache */
#define I7SET	5
#define I7WAY	30
#define NCSIZE	4			/* number of times of cache size */
#define CSIZE	14			/* 16 KB cache size */
#define SWAY	(CSIZE - NWAY)		/* size of way in bytes = 1 << SWAY */
#define NSET	(CSIZE - NWAY - CLINE)	/* cache lines per way = 1 << NSET */

/* ==========================================================================*/
#ifndef __ASM__

/* ==========================================================================*/

extern void setup_pagetbl(void);
extern void enable_mmu(void);
extern void disable_mmu(void);
extern void _enable_icache(void);
extern void _disable_icache(void);
extern void _enable_dcache(void);
extern void _disable_dcache(void);
extern void _flush_i_cache(void);
extern void _flush_d_cache(void);
extern void _clean_d_cache(void);
extern void _clean_flush_d_cache(void);
extern void _clean_flush_all_cache(void);
extern void _drain_write_buffer(void);
extern void flush_all_cache_region(void *addr, unsigned int size);
extern void clean_flush_all_cache_region(void *addr, unsigned int size);
extern void flush_i_cache_region(void *addr, unsigned int size);
extern void flush_d_cache_region(void *addr, unsigned int size);
extern void clean_d_cache_region(void *addr, unsigned int size);
extern void clean_flush_d_cache_region(void *addr, unsigned int size);
extern void drain_write_buffer(u32 addr);
extern void clean_d_cache(void *addr, unsigned int size);
extern void flush_d_cache(void *addr, unsigned int size);
extern void clean_flush_d_cache(void *addr, unsigned int size);
extern int pli_cache_region(void *addr, unsigned int size);

/* ==========================================================================*/

#endif
/* ==========================================================================*/

#endif

