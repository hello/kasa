/**
 * bld/mmu.S
 *
 * History:
 *    2005/03/10 - [Charles Chiou] created file
 *    2009/01/12 - [Charles Chiou] merged mmu.c & pagetable.c into mmu.S
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

.globl mmu_init
mmu_init:
	mov	r10, lr

	/* Set the interrupt vectors address to 0xffff0000 - 0xffff001c
	 * and disable subpage AP bits (it's safe for CortexA9 to set bit23) */
	mrc	p15, 0, r0, c1, c0, 0
	orr	r0, r0, #0x2000		/* Turn on bit 13 - HV */
	orr	r0, r0, #0x800000	/* Turn on bit 23 - XP */
	mcr	p15, 0, r0, c1, c0, 0

	/********************/
	/* Setup page table */
	/********************/

#if (PHY_BUS_MAP_TYPE == 1)
	ldr	r0, pagetable_main
	mov	r1, #0x000

	/* Map DRAM up to the range specified by the configuration */
	bl	mmu_dram_tbl

	/* Clear from dram end to 0xdff (~ 0xdfefffff) */
	ldr	r2, =0xdff
	bl	mmu_clear_tbl

	/* Map 0xdff - 0xf01 (0xdff00000 - 0xf00fffff) as peripheral address */
	ldr	r2, =0xf01
	bl	mmu_peripheral_tbl

	/* Clear 0xf01 - 0xfff (0xf0100000 - 0xffefffff) */
	ldr	r2, =0xfff
	bl	mmu_clear_tbl

#elif (PHY_BUS_MAP_TYPE == 2)
	ldr	r0, pagetable_main
	mov	r1, #0x000

	/* Map DRAM up to the range specified by the configuration */
	bl	mmu_dram_tbl

	/* Clear from dram end to 0x600 (~ 0x5fffffff) */
	mov	r2, #0x600
	bl	mmu_clear_tbl

	/* Map 0x600 - 0x800 (0x60000000 - 0x7fffffff) as peripheral address */
	mov	r2, #0x800
	bl	mmu_peripheral_tbl

	/* Clear 0x800 - 0xdff (0x80000000 - 0xdfefffff) */
	ldr	r2, =0xdff
	bl	mmu_clear_tbl

	/* Map 0xdff - 0xf01 (0xdff00000 - 0xf00fffff) as peripheral address */
	ldr	r2, =0xf01
	bl	mmu_peripheral_tbl

	/* Clear 0xf01 - 0xfff (0xf0100000 - 0xffefffff) */
	ldr	r2, =0xfff
	bl	mmu_clear_tbl

#else
	ldr	r0, pagetable_main
	mov	r1, #0x000

	/* Clear 0x000 - 0x600 (0x00000000 - 0x5fffffff) */
	mov	r2, #0x600
	bl	mmu_clear_tbl

	/* Map 0x600 - 0x800 (0x60000000 - 0x7fffffff) as peripheral address */
	mov	r2, #0x800
	bl	mmu_peripheral_tbl

	/* Clear 0x800 - 0xc00 (0x00000000 - 0xbfffffff) */
	mov	r2, #0xc00
	bl	mmu_clear_tbl

	/* Map DRAM up to the range specified by the configuration */
	bl	mmu_dram_tbl

	/* Clear from end of DRAM to 0xfff (0xffefffff) */
	ldr	r2, =0xfff
	cmp	r2, r1
	blhi	mmu_clear_tbl

#endif

	/* Setup 0xfff00000 to point to a corse page table */
	ldr	r3, pagetable_hv
	orr	r3, r3, #0x01
	str	r3, [r0]

	/* Now setup the 2nd-level pagetable for the 0xfff00000 section */
	ldr	r0, pagetable_hv

	/* Clear 0x00 - 0xef (0xfff00000 - 0xfffeffff) */
	mov	r1, #0x0
	mov	r2, #0xf0
	bl	mmu_clear_tbl

	/* Map 0xffff0000 to DRAM_START_ADDR */
	mov	r3, #DRAM_START_ADDR
	orr	r3, r3, #0x3e
	str	r3, [r0]
	add	r0, r0, #0x4
	add	r1, r1, #0x1

	/* Clear 0xf1 - 0xff */
	mov	r2, #0x100
	bl	mmu_clear_tbl

	/* Make sure MMU is in disabled state first */
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #0x1
	mcr	p15, 0, r0, c1, c0, 0

	/* Set the page table */
	ldr	r0, pagetable_main;
	mcr	p15, 0, r0, c2, c0, 0

	/* Flush all cache */
	bl	_flush_d_cache

	/* Flush all TLB */
	mcr	p15, 0, r0, c8, c6, 0

	/* Set domain 0 as client and access permission controlled by */
	/* descriptor. Other domains have no access permission */
	mov	r0, #0x1
	mcr	p15, 0, r0, c3, c0, 0

	/* Enable MMU */
	mrc	p15, 0, r0, c1, c0, 0
	orr	r0, r0, #0x1
	mcr	p15, 0, r0, c1, c0, 0

	mov	pc, r10

/**
 * Clear page table:
 * r0 - page table entry
 * r1 - start address
 * r2 - end address
 */
mmu_clear_tbl:
	teq	r1, r2
	moveq	pc, lr

	mov	r3, #0x0
1:
	str	r3, [r0]
	add	r0, r0, #0x4
	add	r1, r1, #0x1
	teq	r1, r2
	bne	1b
	mov	pc, lr

/**
 * Initialize page table for DRAM:
 * r0 - page table entry
 * r1 - virtual address start
 */
mmu_dram_tbl:
#if	(DRAM_SIZE == 0x02000000)
	add	r2, r1, #0x020	/* 256 Mbit (32 MB) */
#elif	(DRAM_SIZE == 0x04000000)
	add	r2, r1, #0x040	/* 512 Mbit (64 MB) */
#elif	(DRAM_SIZE == 0x08000000)
	add	r2, r1, #0x080	/* 1 Gbit (128 MB) */
#elif	(DRAM_SIZE == 0x10000000)
	add	r2, r1, #0x100	/* 2 Gbit (256 MB) */
#elif	(DRAM_SIZE == 0x20000000)
	add	r2, r1, #0x200	/* 4 Gbit (512 MB) */
#elif	(DRAM_SIZE == 0x40000000)
	add	r2, r1, #0x400	/* 8 Gbit (1024 MB) */
#elif	(DRAM_SIZE == 0x60000000)
	add	r2, r1, #0x600	/* 12 Gbit (1536 MB) */
#elif	(DRAM_SIZE == 0x80000000)
	add	r2, r1, #0x800	/* 16 Gbit (2048 MB) */
#else
	add	r2, r1, #0x020	/* 256 Mbit (32 MB) */
#endif
	ldr	r3, =0xfff	/* 0xfff00000 must be reserved for exception */
	cmp	r2, r3
	ldrhi	r2, =0xfff
2:
	mov	r3, r1, lsl #20		/* Address */
	orr	r3, r3, #0xc00		/* Access = R/W */
	orr	r3, r3, #0x0e
	str	r3, [r0]
	add	r0, r0, #0x4
	add	r1, r1, #0x1
	teq	r1, r2
	bne	2b
	mov	pc, lr

/**
 * Initialize page table for Peripheral:
 * r0 - page table entry
 * r1 - start address
 * r2 - end address
 */
mmu_peripheral_tbl:
3:
	mov	r3, r1, lsl #20		/* Address */
	orr	r3, r3, #0xc00		/* Access = R/W */
	orr	r3, r3, #0x02
	str	r3, [r0]
	add	r0, r0, #0x4
	add	r1, r1, #0x1
	teq	r1, r2
	bne	3b
	mov	pc, lr

pagetable_main:		.word	__pagetable_main
pagetable_hv:		.word	__pagetable_hv

