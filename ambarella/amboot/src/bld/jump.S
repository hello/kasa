/**
 * bld/jump.S
 *
 * History:
 *    2006/12/29 - [Charles Chiou] created file
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

