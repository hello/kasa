/**
 * bld/run_once.S
 *
 * History:
 *    2005/02/27 - [Charles Chiou] created file
 *
 * Setup the run-time environment and jump to function 'main()' once; when
 * it returns, the execute a break-point instruction. This is mainly useful
 * for loading a progream from JTAG into memory and executing that program
 * once and have the ICE debugger stop upon code execution completion.
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <amboot.h>
#include <macro.S>

.text

.globl run_once
run_once:
	ldr	pc, =reset
	ldr	pc, =undefined_instruction
	ldr	pc, =software_interrupt
	ldr	pc, =prefetch_abort
	ldr	pc, =data_abort
	ldr	pc, =not_used
	ldr	pc, =irq
	ldr	pc, =fiq
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop

memfwprog_result_address:	.word	__memfwprog_result
	nop
	nop
	nop
memfwprog_command_address:	.word	__memfwprog_command
	nop
	nop
	nop

reset:
#if defined(__CORTEXA9__)
	invalidate_all_tlb	r3
	invalidate_all_cache	r3, r4, r5, r6, r7
#endif

	/* First read in some misc registers */
	mrc	p15, 0, r0, c0, c0, 0	/* Read ID value */
	mrc	p15, 0, r1, c0, c0, 1	/* Read cache type */
	mrc	p15, 0, r2, c0, c0, 2	/* Read TCM status */

	/* Turn on instrucion cache and disable MMU */
	mrc	p15, 0, r0, c1, c0, 0	/* Read control register */
	orr	r0, r0, #0x1000		/* Turn on bit 12 */
	bic	r0, r0, #0x1		/* Turn off bit 1 */
	mcr	p15, 0, r0, c1, c0, 0	/* Write control register */

	/* Give manager access permissions to all domains */
	mov	r0, #0x1
	mcr	p15, 0, r0, c3, c0, 0	/* Write domain access control reg */

	/* Disabale IRQ/FIQ interrupts */
	mrs	r0, cpsr
	orr	r0, r0, #0xc0
	msr	cpsr_c, r0

	/* Clear regs. */
	mov	r0, #0
	mov	r1, #0
	mov	r2, #0
	mov	r3, #0
	mov	r4, #0
	mov	r5, #0
	mov	r6, #0
	mov	r7, #0
	mov	r8, #0
	mov	r9, #0
	mov	r10, #0
	mov	r11, #0
	mov	r12, #0

	/* undef mode */
	mrs	r1, cpsr
	bic	r1, r1, #0x1f	@ clear current mode
	orr	r1, r1, #0x1b	@ set UNDEF mode
	msr	cpsr, r1	@ now we should be in UNDEF mode
	msr	spsr, r1	@ spsr_irq
	ldr	sp, und_stack_end	@ r13_und
	mov	lr, #0x0		@ r14_und

	/* abort mode */
	mrs	r1, cpsr
	bic	r1, r1, #0x1f	@ clear current mode
	orr	r1, r1, #0x17	@ set ABORT mode
	msr	cpsr, r1	@ now we should be in ABORT mode
	msr	spsr, r1	@ spsr_irq
	ldr	sp, abt_stack_end	@ r13_abort
	mov	lr, #0x0		@ r14_abort

	/* irq mode */
	mrs	r1, cpsr	@ read-modify-write to get to IRQ mode
	bic	r1, r1, #0x1f	@ clear current mode
	orr	r1, r1, #0x12	@ set IRQ mode
	msr	cpsr, r1	@ now we should be in IRQ mode
	msr	spsr, r1	@ spsr_irq
	ldr	sp, irq_stack_end	@ r13_irq
	mov	lr, #0x0		@ r14_irq

	/* fiq mode */
	mrs	r1, cpsr	@ read-modify-write to get to FIQ mode
	bic	r1, r1, #0x1f	@ clear current mode
	orr	r1, r1, #0x11	@ set FIQ mode
	msr	cpsr, r1	@ now we should be in FIQ mode
	msr	spsr, r1	@ spsr_fiq
	mov	r8,  #0x0
	mov	r9,  #0x0
	mov	r10, #0x0
	mov	r11, #0x0
	mov	r12, #0x0
	ldr	sp, fiq_stack_end	@ r13_fiq
	mov	lr, #0x0		@ r14_fiq

	/* svc mode */
	mrs	r1, cpsr	@ read-modify-write to get to supervisor mode
	bic	r1, r1, #0x1f	@ clear current mode
	orr	r1, r1, #0x13	@ set supervisor mode
	msr	cpsr, r1	@ now we should be in supervisor mode
	msr	spsr, r1	@ spsr_svc
	mov	r8,  #0x0
	mov	r9,  #0x0
	mov	r10, #0x0
	mov	r11, #0x0
	mov	r12, #0x0
	ldr	sp, svc_stack_end	@ r13_svc
	mov	lr, #0x0		@ r14_svc

	/* system mode */
	mrs	r1, cpsr	@ read-modify-write to get to system mode
	bic	r1, r1, #0x1f	@ clear current mode
	orr	r1, r1, #0x1f	@ set system mode
	msr	cpsr, r1	@ now we should be in system mode
	msr	spsr, r1	@ spsr_sys
	mov	r8,  #0x0
	mov	r9,  #0x0
	mov	r10, #0x0
	mov	r11, #0x0
	mov	r12, #0x0
	ldr	sp, sys_stack_end	@ r13
	mov	lr, #0x0		@ r14

	/* Initialize GPIO */
#if defined(CONFIG_AMBOOT_ENABLE_GPIO)
	bl	gpio_init
#endif
	/* Set up the page tables and enable MMU */
	bl	mmu_init
	bl	_enable_dcache

	/* Zero out the BSS section */
	/* r1 = start address */
	/* r2 = end address */
	ldr	r1, bss_start
	ldr	r2, bss_end

	mov	r0, #0
1:
	stmia	r1!, {r0}
	cmp	r1, r2
	blo	1b

	/* Jump to C code */
	bl	main

done:
	/* We are here when the process has finished! */
	bkpt

undefined_instruction:
	bkpt

software_interrupt:
	bkpt

prefetch_abort:
	bkpt

data_abort:
	bkpt

not_used:
	bkpt

irq:
	bkpt

fiq:
	bkpt

.globl firmware_start
firmware_start:	.word	__BEGIN_FIRMWARE_IMAGE__

bss_start:	.word	__bss_start
bss_end:	.word	__bss_end
und_stack_end:	.word	__und_stack_end
abt_stack_end:	.word	__abt_stack_end
irq_stack_end:	.word	__irq_stack_end
fiq_stack_end:	.word	__fiq_stack_end
svc_stack_end:	.word	__svc_stack_end
sys_stack_end:	.word	__sys_stack_end

