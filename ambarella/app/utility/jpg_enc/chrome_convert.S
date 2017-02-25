/*
 * jpg_enc.c
 *
 * History:
 *	2014/11/17 - [Zhikan Yang] created file
 *
 *
 * Copyright (c) 2016 Ambarella, Inc.
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

.fpu neon
#ifdef __thumb2__
.syntax unified
.thumb
#endif
.text

/* ARM */
#define IN	r0
#define U_OUT	r1
#define V_OUT	r2
#define ROW	r3
#define COL	r4
#define PITCH	r5
#define CURRENT_ROW	r6
#define CURRENT_COL	r7
#define SPACING	r8

/* NEON */
#define U	D0
#define V	D1


	.align 2
	.global chrome_convert
	.type	chrome_convert, %function
chrome_convert:
	push		{r4-r8,r10-r11,lr}
	vpush		{q4-q7}

	/* load arguments */
	ldmia		r0,	{IN, U_OUT, V_OUT, ROW, COL, PITCH}

	mov CURRENT_ROW, #0
	sub SPACING, PITCH, COL

LOOP_ROW:
	cmp CURRENT_ROW, ROW
	beq	END
	mov			CURRENT_COL, #0

LOOP_COL:
	cmp CURRENT_COL, COL
	addeq IN, IN, SPACING
	addeq CURRENT_ROW, CURRENT_ROW, #1
	beq LOOP_ROW
	vld2.8		{U,V},	[IN]!
	vst1.8		{U},	[U_OUT]!
	vst1.8		{V},	[V_OUT]!

	add CURRENT_COL, CURRENT_COL, #16
	b LOOP_COL

END:
	/* exit if all rows have been processed */
	vpople	{q4-q7}
	pople	{r4-r8,r10-r11,pc}
