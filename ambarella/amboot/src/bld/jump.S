/**
 * bld/jump.S
 *
 * History:
 *    2006/12/29 - [Charles Chiou] created file
 *    2014/02/13 - [Anthony Ginger] Amboot V2
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>

.text

/*
 * Jump to the kernel
 * r0 - jump address
 * r1 - dtb address
 */
.globl	jump_to_kernel
jump_to_kernel:
	mov	r4, r0
	mov	r5, r1
	ldr	r6, =AMBARELLA_LINUX_MACHINE_ID

	bl	_clean_flush_all_cache
	bl	_disable_icache
	bl	_disable_dcache
	bl	disable_interrupts

	mrc	p15, 0, r0, c1, c0, 0	@ Read control register
	orr	r0, r0, #0x1000		@ Turn on bit 12
	bic	r0, r0, #0x1		@ Turn off bit 1
	bic	r0, r0, #0x2000		@ Turn off bit 13
	mcr	p15, 0, r0, c1, c0, 0	@ Write control register

	mrs	r1, cpsr	@ read-modify-write to get to SVC mode
	bic	r1, r1, #0x1f	@ clear current mode
	orr	r1, r1, #0x13	@ set supervisor mode
	msr	cpsr, r1	@ now we should be in supervisor mode

	mov	r0, #0x0
	mov	r1, r6
	mov	r2, r5
	mov	pc, r4

	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop

