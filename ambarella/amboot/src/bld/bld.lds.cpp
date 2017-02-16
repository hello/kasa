/**
 * bld/bld.lds.cpp
 *
 * History:
 *    2005/01/27 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <amboot.h>

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(bld_start)
SECTIONS
{
	. = AMBOOT_BLD_RAM_START;

	.text : {
		trampoline*.o (.text)

		* (.text*)
		* (.rodata*)

		. = ALIGN(4);
		__cmdlist_start = .;
		* (.cmdlist)
		__cmdlist_end = .;
	}

	. = ALIGN(4);
	.data : {
		* (.data*)
	}

	. = ALIGN(4);
	.bss : {
		__bss_start = .;

		* (.bss)

		__bss_end = .;

		. = ALIGN(4);
		/* Stack for UND mode */
		__und_stack_start = .;
		. = __und_stack_start + 0x100;
		__und_stack_end = .;

		/* Stack for ABT mode */
		__abt_stack_start = .;
		. = __abt_stack_start + 0x100;
		__abt_stack_end = .;

		/* Stack for IRQ mode */
		__irq_stack_start = .;
		. = __irq_stack_start + 0x2000;
		__irq_stack_end = .;

		/* Stack for FIQ mode */
		__fiq_stack_start = .;
		. = __fiq_stack_start + 0x100;
		__fiq_stack_end = .;

		/* Stack for SVC mode */
		__svc_stack_start = .;
		. = __svc_stack_start + 0x100;
		__svc_stack_end = .;

		/* Stack for SYS mode */
		__sys_stack_start = .;
		. = __sys_stack_start + AMBOOT_BLD_STACK_SIZE;
		__sys_stack_end = .;

		/* Heap for BLD */
		__heap_start = .;
		. = __heap_start +  AMBOOT_BLD_HEAP_SIZE;
		__heap_end = .;
	}

	. = ALIGN(4);
	.bss.noinit : {
		* (.bss.noinit*)
	}

	. = ALIGN(0x4000);
	.pagetable (NOLOAD) : {
		/* MMU page table */
		__pagetable_main = .;
		. = __pagetable_main + (4096 * 4);	/* 16 KB of L1 */
		__pagetable_hv = .;
		. = __pagetable_hv + (256 * 4);		/* 1KB of L2 */
		__pagetable_cry = .;
		. = __pagetable_cry + (256 * 4);
		__pagetable_end = .;
	}

	. = ALIGN(0x1000);
	.bldbuf . (NOLOAD) : {
		bld_buf_addr = .;
		. = bld_buf_addr + (384 * SIZE_1KB);
		bld_buf_end = .;
	}

	. = ASSERT((. <= AMBOOT_BLD_RAM_START + AMBOOT_BLD_RAM_MAX_SIZE),
		"We limit BLD size to avoid across DSP region, Check Point!");

	. = (AMBOOT_BLD_RAM_START + SIZE_1MB);
	.splashbuf . (NOLOAD) : {
		splash_buf_addr = .;
		. = splash_buf_addr + (1 * SIZE_1MB);
		splash_buf_end = .;
	}

	/* In most case, it is DSP space and assume DRAM_SIZE >= 128MB */
	/* bld_hugebuf_addr will be used in:
	   1. amboot command
	   2. usd download
	   3. memfwprog
	   only */
	. = (DRAM_START_ADDR + DRAM_SIZE - (64 * SIZE_1MB));
	.hugebuf . (NOLOAD) : {
		. = ALIGN(0x100000);
		bld_hugebuf_addr = .;
		. = bld_hugebuf_addr + (48 * SIZE_1MB);
	}
}

