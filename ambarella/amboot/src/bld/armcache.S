/**
 * bld/armcache.S
 *
 * History:
 *    2005/08/15 - [Charles Chiou] created file by consolidating the
 *			old files icache.c and dcache.c
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

