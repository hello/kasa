/**
 * ambhw/cache.h
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
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

