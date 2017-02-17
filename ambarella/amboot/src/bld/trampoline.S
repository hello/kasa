/**
 * bld/trampoline.S
 *
 * History:
 *    2005/01/27 - [Charles Chiou] created file
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
#include <macro.S>
#include <ambhw/vic.h>

@
@ IRQ stack frame.
@
#define S_FRAME_SIZE	72

#define S_OLD_R0	68
#define S_PSR		64
#define S_PC		60
#define S_LR		56
#define S_SP		52

#define S_IP		48
#define S_FP		44
#define S_R10		40
#define S_R9		36
#define S_R8		32
#define S_R7		28
#define S_R6		24
#define S_R5		20
#define S_R4		16
#define S_R3		12
#define S_R2		8
#define S_R1		4
#define S_R0		0

.text

/* Start of the BLD code */
.globl bld_start
bld_start:
	b	bld_reset
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

/* set by AmbaUsb for usb hardware boot mode, to specify the real boot media */
.global	ambausb_boot_from
ambausb_boot_from:
	.word	0x00000000

.align 5
bld_reset:
#if defined(__CORTEXA9__)
	invalidate_all_tlb	r3
	invalidate_all_cache	r3, r4, r5, r6, r7
#endif
	mrc	p15, 0, r0, c0, c0, 0	/* Read ID value */
	mrc	p15, 0, r1, c0, c0, 1	/* Read cache type */
	mrc	p15, 0, r2, c0, c0, 2	/* Read TCM status */
#if defined(AMBOOT_DEV_BOOT_CORTEX)
	ldr	r3, =0xFF0FFFF0
	and	r0, r0, r3
	ldr	r3, =0x410FC090
	cmp	r0, r3
	ldrne	r0, =bld_normal_start
	ldreq	r0, =cortex_bst_entry
	addeq	r0, r0, #0x40000000
#elif defined(AMBOOT_BOOT_SECONDARY_CORTEX)
	mrc	p15, 0, r3, c0, c0, 5	/* Read processor id */
	and	r3, r3, #0x03
	cmp	r3, #0			/* if (id ==0) */
	beq	bld_normal_start

secondary_cortex_wait:
	wfi
	ldr	r4, =0xdeadbeaf
	adr	r1, secondary_cortex_jump
	ldr	r0, [r1, r3, LSL #2]
	cmp	r0, r4
	beq	secondary_cortex_wait
	str	r4, [r1, r3, LSL #2]
	bx	r0
	/* never run here */
	b	.
#else
	ldr	r0, =bld_normal_start
#endif
	bx	r0
	b	bld_start
	bkpt
	nop
	nop
	nop

.align 5
bld_normal_start:
	/* Turn on instrucion cache and disable MMU */
	mrc	p15, 0, r0, c1, c0, 0	/* Read control register */
	orr	r0, r0, #0x1000		@ Turn on bit 12 - I-cache
	bic	r0, r0, #0x2000		@ Turn off bit 13 - HV
	bic	r0, r0, #0x1		@ Turn off bit 0 - MMU
#if defined(CHIP_BROKEN_UNALIGNED_ACCESS)
	orr	r0, r0, #0x2		@ Turn on bit 1 - Alignment fault
	bic	r0, r0, #0x400000	@ Turn off bit 22 - Unainged support
#else
	bic	r0, r0, #0x2		@ Turn off bit 1 - Alignment fault
	orr	r0, r0, #0x400000	@ Turn on bit 22 - Unainged support
#endif
	orr	r0, r0, #0x800000	@ Turn on bit 23 - XP to disable subpage
	mcr	p15, 0, r0, c1, c0, 0	/* Write control register */

	/* Give client access to domain 0 */
	mov	r0, #0x1
	mcr	p15, 0, r0, c3, c0, 0

#if defined(__ARM1136JS__)
#if (PHY_BUS_MAP_TYPE == 1)
	/* -------------------------------------------------- */
	/* Redirect peripheral port 0xe0000000 - 0xefffffff   */
	/* -------------------------------------------------- */
	mov	r0, #AHB_PHYS_BASE
	orr	r0, r0, #0x00000013
	mcr	p15, 0, r0, c15, c2, 4
#else
	/* -------------------------------------------------- */
	/* Redirect peripheral port 0x60000000 - 0x7fffffff   */
	/* -------------------------------------------------- */
	mov	r0, #AHB_PHYS_BASE
	orr	r0, r0, #0x00000014
	mcr	p15, 0, r0, c15, c2, 4
#endif
#endif

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

	/* If main ever returns we just call it again */
	b	bld_start
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
	sub	sp, sp, #S_FRAME_SIZE
	stmia	sp, {r0 - r12}		@ Calling r0-r12
					@ !!!! R8 NEEDS to be saved !!!!
					@ a reserved stack spot would be good.
	add	r8, sp, #S_PC
	stmdb	r8, {sp, lr}^		@ Calling SP, LR
	str	lr, [r8, #0]		@ Save calling PC
	mrs	r6, spsr
	str	r6, [r8, #4]		@ Save CPSR
	str	r0, [r8, #8]		@ Save OLD_R0
	mov	r0, sp

	mov	r0, #AHB_BASE
#if defined(VIC_2NDGEN_BITASSIGNMENT)
	orr	r0, r0, #VIC3_OFFSET
	ldr	r5, [r0, #0x0]		@ read irq status register
#if defined(CONFIG_AMBOOT_ENABLE_ETH)
	@ test for Ethernet interrupt
	tst	r5, #(1 << (ETH_IRQ - VIC3_INT_VEC(0)))
	bne	irq_eth			@ Ethernet irq handler
#endif
#if defined(CONFIG_AMBOOT_ENABLE_USB)
	@ test for USB interrupt
	tst	r5, #(1 << (USBC_IRQ - VIC3_INT_VEC(0)))
	bne	irq_usb			@ usb irq handler
#endif
#else
	orr	r0, r0, #VIC_OFFSET
	ldr	r5, [r0, #0x0]		@ read irq status register
#if defined(CONFIG_AMBOOT_ENABLE_ETH)
	tst	r5, #(1 << ETH_IRQ)	@ test for Ethernet interrupt
	bne	irq_eth			@ Ethernet irq handler
#endif
#if defined(CONFIG_AMBOOT_ENABLE_USB)
	tst	r5, #(1 << USBC_IRQ)	@ test for USB interrupt
	bne	irq_usb			@ usb irq handler
#endif
#endif

.globl irq_done
irq_done:
	ldmia	sp, {r0 - lr}^		@ Calling r0 - lr
	mov	r0, r0
	ldr	lr, [sp, #S_PC]		@ Get PC
	add	sp, sp, #S_FRAME_SIZE
	subs	pc, lr, #4		@ return & move spsr_svc into cpsr

fiq:
	bkpt

bss_start:	.word	__bss_start
bss_end:	.word	__bss_end
und_stack_end:	.word	__und_stack_end
abt_stack_end:	.word	__abt_stack_end
irq_stack_end:	.word	__irq_stack_end
fiq_stack_end:	.word	__fiq_stack_end
svc_stack_end:	.word	__svc_stack_end
sys_stack_end:	.word	__sys_stack_end

#if defined(AMBOOT_BOOT_SECONDARY_CORTEX)
.globl secondary_cortex_jump
secondary_cortex_jump:
.rept 4
			.word	0xdeadbeaf
.endr
#endif

