/**
 * system/src/comsvc/cache.c
 *
 * History:
 *    2005/05/26 - [Charles Chiou] created file
 *    2005/08/29 - [Chien-Yang Chien] assumed maintenance of this module
 *    2014/02/13 - [Anthony Ginger] Amboot V2
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <amboot.h>
#include <ambhw/cache.h>

/*===========================================================================*/
void flush_all_cache_region(void *addr, unsigned int size)
{
	u32 addr_start;

	K_ASSERT((((u32) addr) & ((0x1 << CLINE) - 1)) == 0);
	K_ASSERT((size & ((0x1 << CLINE) - 1)) == 0);

	addr_start = (u32) addr;
	size = size >> CLINE;

	while ((int) size > 0) {
		__asm__ __volatile__ ("mcr p15, 0, %0, c7, c5, 1" :
				      "=r " (addr_start));
		__asm__ __volatile__ ("mcr p15, 0, %0, c7, c6, 1" :
				      "=r " (addr_start));
		addr_start += (0x1 << CLINE);
		size--;
	}
}

void clean_flush_all_cache_region(void *addr, unsigned int size)
{
	u32 addr_start, addr_end;
	u32 misalign;

	addr_start = ((u32) addr) & ~((0x1 << CLINE) - 1);
	addr_end = ((u32) addr + size) & ~((0x1 << CLINE) - 1);

	misalign = ((u32) addr + size) & ((0x1 << CLINE) - 1);
	if (misalign)
		addr_end += (0x1 << CLINE);

	size = (addr_end - addr_start) >> CLINE;

	do {
		__asm__ __volatile__ ("mcr p15, 0, %0, c7, c14, 1" : :
				      "r" (addr_start));
		__asm__ __volatile__ ("mcr p15, 0, %0, c7, c5, 1" : :
				      "r" (addr_start));
		addr_start += (0x1 << CLINE);
		size--;
	} while ((int) size > 0);
}

void flush_i_cache_region(void *addr, unsigned int size)
{
	u32 addr_start;

	K_ASSERT((((u32) addr) & ((0x1 << CLINE) - 1)) == 0);
	K_ASSERT((size & ((0x1 << CLINE) - 1)) == 0);

	addr_start = (u32) addr;
	size = size >> CLINE;

	while ((int) size > 0) {
		__asm__ __volatile__ ("mcr p15, 0, %0, c7, c5, 1" : :
				      "r" (addr_start));
		addr_start += (0x1 << CLINE);
		size--;
	}
}

void flush_d_cache_region(void *addr, unsigned int size)
{
	u32 addr_start;

	K_ASSERT((((u32) addr) & ((0x1 << CLINE) - 1)) == 0);
	K_ASSERT((size & ((0x1 << CLINE) - 1)) == 0);

	addr_start = (u32) addr;
	size = size >> CLINE;

	while ((int) size > 0) {
		__asm__ __volatile__ ("mcr p15, 0, %0, c7, c6, 1" : :
				      "r" (addr_start));
		addr_start += (0x1 << CLINE);
		size--;
	}
}

void clean_d_cache_region(void *addr, unsigned int size)
{
	u32 addr_start, addr_end;
	u32 misalign;

	addr_start = ((u32) addr) & ~((0x1 << CLINE) - 1);
	addr_end = ((u32) addr + size) & ~((0x1 << CLINE) - 1);

	misalign = ((u32) addr + size) & ((0x1 << CLINE) - 1);
	if (misalign)
		addr_end += (0x1 << CLINE);

	size = (addr_end - addr_start) >> CLINE;

	do {
		__asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 1" : :
				      "r" (addr_start));
		addr_start += (0x1 << CLINE);
		size--;
	} while ((int) size > 0);
}

void clean_flush_d_cache_region(void *addr, unsigned int size)
{
	u32 addr_start, addr_end;
	u32 misalign;

	addr_start = ((u32) addr) & ~((0x1 << CLINE) - 1);
	addr_end = ((u32) addr + size) & ~((0x1 << CLINE) - 1);

	misalign = ((u32) addr + size) & ((0x1 << CLINE) - 1);
	if (misalign)
		addr_end += (0x1 << CLINE);

	size = (addr_end - addr_start) >> CLINE;

	do {
		__asm__ __volatile__ ("mcr p15, 0, %0, c7, c14, 1" : :
				      "r" (addr_start));
		addr_start += (0x1 << CLINE);
		size--;
	} while ((int) size > 0);
}

void drain_write_buffer(u32 addr)
{
	addr = addr & ~((0x1 << CLINE) - 1);

	__asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 4" : : "r" (addr));
}

void clean_d_cache(void *addr, unsigned int size)
{
	if (size >= (0x1 << CSIZE) * NCSIZE) {
		_clean_d_cache();
	} else {
		clean_d_cache_region(addr, size);
	}

#if defined(ENABLE_DRAIN_WRITE_BUF)
	drain_write_buffer((u32)addr);
#endif
}

void flush_d_cache(void *addr, unsigned int size)
{
	flush_d_cache_region(addr, size);
}

void clean_flush_d_cache(void *addr, unsigned int size)
{
	if (size >= (0x1 << CSIZE) * NCSIZE) {
		_clean_flush_d_cache();
	} else {
		clean_flush_d_cache_region(addr, size);
	}

#if defined(ENABLE_DRAIN_WRITE_BUF)
	drain_write_buffer((u32)addr);
#endif
}

#if defined(__ARM1136JS__)
int pli_cache_region(void *addr, unsigned int size)
{
	int rval = 0;

	size = size + (u32)addr;
	addr = (void *)(((u32)addr) & ~((0x1 << CLINE) - 1));
	if (size & ((0x1 << CLINE) - 1)) {
		size = ((size - (u32)addr) >> CLINE) + 1;
	} else {
		size = (size - (u32)addr) >> CLINE;
	}

	if (size > (0x1 << CSIZE)) {
		rval = -1;
		goto pli_cache_region_exit;
	}

	do {
		__asm__ __volatile__ (
			"mcr p15, 0, %0, c7, c13, 1" : : "r" ((u32)addr));
		addr = (void *)(((u32)addr) + (0x1 << CLINE));
		size--;
	} while (size > 0);

pli_cache_region_exit:
	return rval;
}
#endif

