/**
 * include/macro.S
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


	.macro	invalidate_all_tlb, tmp0
	mov		\tmp0, #0x00
	mcr		p15, 0, \tmp0, c8, c7, 0		/* Invalidate entire unified TLB */
	mcr		p15, 0, \tmp0, c8, c6, 0		/* Invalidate entire data TLB */
	mcr		p15, 0, \tmp0, c8, c5, 0		/* Invalidate entire instruction TLB */
	.endm

	.macro	operate_all_cache, cr, tmp0, tmp1, tmp2, tmp3, tmp4
	mov		\tmp0, #0
	mcr		p15, 0, \tmp0, c7, c5, 6		/* Invalidate entire branch prediction array */
	mcr		p15, 0, \tmp0, c7, c5, 0		/* Invalidate entire icache */
	mcr		p15, 2, \tmp0, c0, c0, 0		/* cache size selection register, select dcache */
	mrc		p15, 1, \tmp0, c0, c0, 0		/* cache size id register */
	mov		\tmp0, \tmp0, asr #13
	movw		\tmp2, #0x0fff
	and		\tmp0, \tmp0, \tmp2
	cmp		\tmp0, #0x7f
	moveq		\tmp0, #0x1000
	beq		1f
	cmp		\tmp0, #0xff
	moveq		\tmp0, #0x2000
	movne		\tmp0, #0x4000
1:
	mov		\tmp1, #0
	mov		\tmp2, #0x40000000
	mov		\tmp3, #0x80000000
	mov		\tmp4, #0xc0000000
2:
	mcr		p15, 0, \tmp1, c7, \cr, 2		/* invalidate dcache by set / way */
	mcr		p15, 0, \tmp2, c7, \cr, 2		/* invalidate dcache by set / way */
	mcr		p15, 0, \tmp3, c7, \cr, 2		/* invalidate dcache by set / way */
	mcr		p15, 0, \tmp4, c7, \cr, 2		/* invalidate dcache by set / way */
	add		\tmp1, \tmp1, #0x20
	add		\tmp2, \tmp2, #0x20
	add		\tmp3, \tmp3, #0x20
	add		\tmp4, \tmp4, #0x20
	cmp		\tmp1, \tmp0
	bne		2b
	.endm

	.macro	invalidate_all_cache, tmp0, tmp1, tmp2, tmp3, tmp4
	operate_all_cache c6, \tmp0, \tmp1, \tmp2, \tmp3, \tmp4
	.endm

	.macro	clean_all_cache, tmp0, tmp1, tmp2, tmp3, tmp4
	operate_all_cache c10, \tmp0, \tmp1, \tmp2, \tmp3, \tmp4
	.endm

	.macro	invcln_all_cache, tmp0, tmp1, tmp2, tmp3, tmp4
	operate_all_cache c14, \tmp0, \tmp1, \tmp2, \tmp3, \tmp4
	.endm
