/**
 * system/src/comsvc/cache.c
 *
 * History:
 *    2005/05/26 - [Charles Chiou] created file
 *    2005/08/29 - [Chien-Yang Chien] assumed maintenance of this module
 *    2014/02/13 - [Anthony Ginger] Amboot V2
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

