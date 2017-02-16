/**
 * bld/armcache.S
 *
 * History:
 *    2005/08/15 - [Charles Chiou] created file by consolidating the
 *			old files icache.c and dcache.c
 *    2014/02/13 - [Anthony Ginger] Amboot V2
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

.globl	_enable_icache
_enable_icache:
	mrc	p15, 0, r0, c1, c0, 0
	orr	r0, r0, #0x1000
	mcr	p15, 0, r0, c1, c0, 0
	mov	pc, lr
	nop
	nop

.globl	_disable_icache
_disable_icache:
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #0x1000
	mcr	p15, 0, r0, c1, c0, 0
	mov	pc, lr
	nop
	nop

.globl	_flush_i_cache
_flush_i_cache:
	mov	r0, #0x0
	mcr	p15, 0, r0, c7, c5, 0
	mov	pc, lr
	nop
	nop

.globl	_enable_dcache
_enable_dcache:
	mrc	p15, 0, r0, c1, c0, 0
	orr	r0, r0, #0xc
	mcr	p15, 0, r0, c1, c0, 0
	mov	pc, lr
	nop
	nop

.globl	_drain_write_buffer
_drain_write_buffer:
	mov	r0, #0x0
	mcr	p15, 0, r0, c7, c10, 4
	mov	pc, lr

#if defined(__ARM1136JS__)
.globl	_disable_dcache
_disable_dcache:
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #0xc
	mcr	p15, 0, r0, c1, c0, 0
	mov	pc, lr
	nop
	nop

.globl	_flush_d_cache
_flush_d_cache:
	mov	r0, #0x0
	mcr	p15, 0, r0, c7, c6, 0
	mov	pc, lr
	nop
	nop

.globl	_clean_d_cache
_clean_d_cache:
	mov	r0, #0x0
	mcr	p15, 0, r0, c7, c10, 0
	mov	pc, lr

.globl	_clean_flush_d_cache
_clean_flush_d_cache:
	mov	r0, #0x0
	mcr	p15, 0, r0, c7, c14, 0
	mov	pc, lr

.globl	_clean_flush_all_cache
_clean_flush_all_cache:
	mov	r0, #0x0
	mcr	p15, 0, r0, c7, c14, 0
        mcr	p15, 0, r0, c7, c5, 0
        mov	pc, lr

#elif defined(__CORTEXA9__)

#include <macro.S>

.globl	_disable_dcache
_disable_dcache:
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #0x4
	mcr	p15, 0, r0, c1, c0, 0
	mov	pc, lr
	nop
	nop

.globl	_flush_d_cache
_flush_d_cache:
	stmfd	sp!, {r0-r4, lr}
	invalidate_all_cache r0, r1, r2, r3, r4
	ldmfd	sp!, {r0-r4, lr}
	mov	pc, lr


.globl	_clean_d_cache
_clean_d_cache:
	stmfd	sp!, {r0-r4, lr}
	clean_all_cache r0, r1, r2, r3, r4
	ldmfd	sp!, {r0-r4, lr}
	mov	pc, lr

.globl	_clean_flush_d_cache
_clean_flush_d_cache:
	stmfd	sp!, {r0-r4, lr}
	invcln_all_cache r0, r1, r2, r3, r4
	ldmfd	sp!, {r0-r4, lr}
	mov	pc, lr

.globl	_clean_flush_all_cache
_clean_flush_all_cache:
	stmfd	sp!, {r0-r4, lr}
	invcln_all_cache r0, r1, r2, r3, r4
	mov	r0, #0x0
	mcr	p15, 0, r0, c7, c5, 6		/* Invalidate entire branch prediction array */
	mcr	p15, 0, r0, c7, c5, 0		/* Invalidate entire icache */
	ldmfd	sp!, {r0-r4, lr}
	mov	pc, lr
	nop
	nop

#endif

