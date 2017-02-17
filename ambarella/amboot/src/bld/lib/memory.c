/**
 * lib/memory.c
 *
 * Author: Jorney Tu <qtu@ambarella.com>
 * History: 2015/04/29 - created
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


#include <bldfunc.h>
#include <vsprintf.h>

#if 0
#define pr_debug(fmt, args...)  printf(fmt, ##args)
#else
#define pr_debug(fmt, args...)   do {} while(0)
#endif

#define MALLOC_BLOCK_SIZE          0x200 /* available values: 512B, 1KB, 2KB, 4KB */

struct malloc_info {
	u32 start_blk;
	u32 nr_blk;
};

static struct malloc_info *malloc_table;
static u32 malloc_table_num;

static u32 *malloc_bitmap = NULL;

static inline void set_bit(u32 *addr, int nr)
{
	BUG_ON(nr > 31 || nr < 0);
	*addr |= (1 << nr);
}

static inline void clear_bit(u32 *addr, int nr)
{
	BUG_ON(nr > 31 || nr < 0);
	*addr &= ~(1 << nr);
}

static inline int bit_isset(u32 *addr, int nr)
{
	BUG_ON(nr > 31 || nr < 0);
	return !!((*addr) & (1 << nr));
}

/* check if the bits located in the range [start_bit, start_bit + nr_bit)
 * in bitmap are all cleared. If yes, return the original start_bit, orelse
 * return the bit position after the bit that is set */
static int bit_isclear_range(u32 start_bit, int nr_bit)
{
	int _start_bit = start_bit;

	while (nr_bit > 0) {
		if (bit_isset(&malloc_bitmap[_start_bit / 32], _start_bit % 32))
			return _start_bit + 1;

		_start_bit++;
		nr_bit--;
	}

	return start_bit;
}

void *malloc(int size)
{
	u32 i, start_blk, _start_blk, blks;

	if (size <= 0) {
		printf("zero-sized request: %d!\n", size);
		return NULL;
	}

	/* NOTE: if you need memory buffer with small size, you'd better
	 * use stack instead of malloc */
	if (size < MALLOC_BLOCK_SIZE / 2) {
		printf("requested size is too small: %d!\n", size);
		return NULL;
	}


	blks = (size + MALLOC_BLOCK_SIZE - 1) / MALLOC_BLOCK_SIZE;

	pr_debug("request size: %d, malloc blks: %d\n", size, blks);

	/* search continous free memory to malloc */
	start_blk = 0;
	while (start_blk < MALLOC_BLOCK_SIZE * 8) {
		_start_blk = bit_isclear_range(start_blk, blks);
		if (_start_blk == start_blk)
			break;

		start_blk = _start_blk;
	}

	if (start_blk >= MALLOC_BLOCK_SIZE * 8) {
		pr_debug("No free memory to malloc\n");
		return NULL;
	}

	/* mark the bitmap for corresponding memory block */
	for (i = 0; i < blks; i++) {
		_start_blk = start_blk + i;
		set_bit(&malloc_bitmap[_start_blk / 32], _start_blk % 32);
	}

	for(i = 0; i < malloc_table_num; i++){
		if (malloc_table[i].start_blk == 0) {
			malloc_table[i].start_blk = start_blk;
			malloc_table[i].nr_blk = blks;
			break;

		}
	}

	if (i >= malloc_table_num) {
		pr_debug("No table space for malloc!\n");
		return NULL;
	}

	pr_debug("start_blk: %d, blks: %d\n", start_blk, blks);

	return (void *)((uintptr_t)malloc_bitmap + start_blk * MALLOC_BLOCK_SIZE);
}

void free(void *ptr)
{
	u32 i, start_blk, _start_blk, nr_blk;

	pr_debug("0x%x free ..\n", ptr);

	if (ptr == NULL)
		return;

	start_blk = ((uintptr_t)ptr - (uintptr_t)malloc_bitmap) / MALLOC_BLOCK_SIZE;
	nr_blk = 0;

	for (i = 0; i < malloc_table_num; i++) {
		if(malloc_table[i].start_blk == start_blk) {
			nr_blk = malloc_table[i].nr_blk;
			malloc_table[i].start_blk = 0;
			malloc_table[i].nr_blk = 0;
		}
	}

	BUG_ON(nr_blk == 0);

	for (i = 0; i < nr_blk; i++) {
		_start_blk = start_blk + i;

		BUG_ON(!bit_isset(&malloc_bitmap[_start_blk / 32], _start_blk % 32));

		clear_bit(&malloc_bitmap[_start_blk / 32], _start_blk % 32);
	}
}

/*
 * Initialize memory pool for malloc heap.
 * NOTE:
 *   1. the memory pool start address must be 1024 aligned.
 *   2. It's forbidden to output any message by UART in malloc_init.
 */
int malloc_init(void)
{
	int i, total_blks;

	malloc_bitmap = (u32 *)(uintptr_t)bld_buf_addr;
	malloc_table = (struct malloc_info *)((uintptr_t)bld_buf_addr + MALLOC_BLOCK_SIZE);
	malloc_table_num = MALLOC_BLOCK_SIZE / sizeof(struct malloc_info);

	total_blks = (bld_buf_end  - bld_buf_addr) / MALLOC_BLOCK_SIZE;

	memset(malloc_table ,0 , MALLOC_BLOCK_SIZE);

	/* now clear the bitmap for existed memory, and set the bitmap for
	 * non-existed memory, note that the first block is reserved for bitmap,
	 * and the second block is reserved for malloc_table. */
	memset(malloc_bitmap, 0xff, MALLOC_BLOCK_SIZE);
	for (i = 2; i < total_blks; i++)
		clear_bit(&malloc_bitmap[i / 32], i % 32);

	return 0;
}

