/**
 * bld/cortex_bst.S
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
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


#include <macro.S>

/*===========================================================================*/
#define WRONG_START				0xdeadbeaf
#define START_COUNTER				0xffffffff

/*===========================================================================*/
	.macro	alock, ptr, tmp0, tmp1, tmp2
	mov		\tmp0, #0x01
	adr		\tmp2, \ptr
lock_again:
	ldrex		\tmp1, [\tmp2]
	cmp		\tmp1, #0x00
	strexeq		\tmp1, \tmp0, [\tmp2]
	cmpeq		\tmp1, #0x00
	bne		lock_again
	.endm

	.macro	aunlock, ptr, tmp0
	mov		\tmp0, #0x00
	str		\tmp0, \ptr
	.endm

/*===========================================================================*/
.text

.globl	cortex_bst_entry
.globl	cortex_bst_head
.globl	cortex_bst_end

.globl	cortex_processor_start
.globl	cortex_processor_status
.globl	cortex_machine_id
.globl	cortex_atag_data
.globl	cortex_processor0_r0

.align 8
cortex_bst_entry:
	invalidate_all_tlb			r3
	invalidate_all_cache			r3, r4, r5, r6, r7

	alock					intdis_locker, r0, r1, r2
	ldr					r0, intdis_status
	cmp					r0, #0x00

	/* Init interrupt Distributor, use Reset values */
	mrceq					p15, 4, r0, c15, c0, 0
	addeq					r0, r0, #0x1000

	/* enable the gic (icddcr) */
	ldreq					r1, [r0]
	orreq					r1, r1, #0x01
	streq					r1, [r0]

	/* (icdiser0) */
	ldreq					r1, icdiser0_mask
	streq					r1, [r0, #0x100]

	aunlock					intdis_locker, r0

	ldr					r4, wrong_start

	/* Read processor id */
	mrc					p15, 0, r3, c0, c0, 5
	and					r3, r3, #0x03

	/* if (id ==0) */
	cmp					r3, #0
	bne					cpux_start

cpu0_start:
	ldr					r0, cortex_processor_status
	add					r0, r0, #1
	str					r0, cortex_processor_status

	ldr					r0, cortex_processor0_r0
	ldr					r1, cortex_machine_id
	ldr					r2, cortex_atag_data
	ldr					r3, cortex_processor_start
	cmp					r3, r4
	beq					cpu0_start
	bx					r3

cpux_start:
	mrc					p15, 4, r0, c15, c0, 0
	ldr					r1, [r0, #0x100]
	orr					r1, r1, #0x01
	str					r1, [r0, #0x100]

	mov					r1, #0xF0
	str					r1, [r0, #0x0104]

cpux_wait:
	wfi
	adr					r1, cortex_processor_status
	ldr					r0, [r1, r3, LSL #2]
	add					r0, r0, #1
	str					r0, [r1, r3, LSL #2]

	adr					r1, cortex_processor_start
	ldr					r0, [r1, r3, LSL #2]
	cmp					r0, r4
	beq					cpux_wait
	str					r4, [r1, r3, LSL #2]
	bx					r0

.align 5
cortex_bst_head:
cortex_processor_start:
.rept 4
			.word	WRONG_START
.endr
cortex_processor_status:
.rept 4
			.word	START_COUNTER
.endr
icdiser0_mask:		.word	0x0000ffff	/* enable SGIs 15-0 */
intdis_locker:		.word	0x00000000
intdis_status:		.word	0x00000000
wrong_start:		.word	WRONG_START
.align 5
cortex_machine_id:	.word	0x000004c7
cortex_atag_data:	.word	0x00000000
cortex_processor0_r0:	.word	0x00000000
head_offset:		.word	(. - cortex_processor_start)

.p2alignl 8, 0xffaa5500
cortex_bst_end:

