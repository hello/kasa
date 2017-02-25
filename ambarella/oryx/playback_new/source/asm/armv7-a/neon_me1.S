/*******************************************************************************
 * neon_me1.S
 *
 * History:
 *    2016/06/08 - [Zhi He] create file
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
 ******************************************************************************/

.fpu neon
.text

#define D_DST           r0
#define D_SRC_Y     r1
#define D_W    r2
#define D_H    r3
#define D_DST_PITCH      r4
#define D_SRC_PITCH       r5

#define D_SRC_Y_2       r6
#define D_SRC_Y_3       r7
#define D_SRC_Y_4       r8

#define D_COUNT       r9

    .align 2
    .global asm_neon_generate_me1_pitch
    .type   asm_neon_generate_me1_pitch, %function
asm_neon_generate_me1_pitch:
    push        {r4-r8,r9-r11,lr}
@@    vpush       {q0-q15}

    /* load arguments */
    ldmia       r0, {D_DST, D_SRC_Y, D_W, D_H, D_DST_PITCH, D_SRC_PITCH}

    add D_SRC_Y_2, D_SRC_Y, D_SRC_PITCH
    add D_SRC_Y_3, D_SRC_Y_2, D_SRC_PITCH
    add D_SRC_Y_4, D_SRC_Y_3, D_SRC_PITCH

    mov D_SRC_PITCH, D_SRC_PITCH, LSL #2
    sub D_SRC_PITCH, D_SRC_PITCH, D_W
    mov D_W, D_W, LSR #2
    mov D_H, D_H, LSR #2

    sub D_DST_PITCH, D_DST_PITCH, D_W

1:
    mov D_COUNT, D_W, LSR #3

2:
    vld1.64 {d0, d1},   [D_SRC_Y]
    vld1.64 {d2, d3},   [D_SRC_Y_2]
    vld1.64 {d4, d5},   [D_SRC_Y_3]
    vld1.64 {d6, d7},   [D_SRC_Y_4]

    add D_SRC_Y, D_SRC_Y, # 16
    add D_SRC_Y_2, D_SRC_Y_2, # 16
    add D_SRC_Y_3, D_SRC_Y_3, # 16
    add D_SRC_Y_4, D_SRC_Y_4, # 16

    vld1.64 {d16, d17},   [D_SRC_Y]
    vld1.64 {d18, d19},   [D_SRC_Y_2]
    vld1.64 {d20, d21},   [D_SRC_Y_3]
    vld1.64 {d22, d23},   [D_SRC_Y_4]

    vpaddl.u8 q4, q0
    vpaddl.u8 q5, q1
    vpaddl.u8 q6, q2
    vpaddl.u8 q7, q3

    vpaddl.u8 q12, q8
    vpaddl.u8 q13, q9
    vpaddl.u8 q14, q10
    vpaddl.u8 q15, q11

    vadd.u16 q4, q4, q5
    vadd.u16 q5, q6, q7
    vadd.u16 q6, q4, q5

    vadd.u16 q12, q12, q13
    vadd.u16 q13, q14, q15
    vadd.u16 q14, q12, q13

    vpaddl.u16 q4, q6
    vshr.u32 q4, #2
    vqmovn.u32 d14, q4

    vpaddl.u16 q12, q14
    vshr.u32 q12, #2
    vqmovn.u32 d15, q12

    vqmovn.u16 d12, q7

    vst1.64 {d12}, [D_DST]

    add D_SRC_Y, D_SRC_Y, # 16
    add D_SRC_Y_2, D_SRC_Y_2, # 16
    add D_SRC_Y_3, D_SRC_Y_3, # 16
    add D_SRC_Y_4, D_SRC_Y_4, # 16
    add D_DST, D_DST, #8

    subs D_COUNT, D_COUNT, #1
    beq 7f
    b 2b

7:
    subs D_H, D_H, #1
    beq 8f

    add D_SRC_Y, D_SRC_Y, D_SRC_PITCH
    add D_SRC_Y_2, D_SRC_Y_2, D_SRC_PITCH
    add D_SRC_Y_3, D_SRC_Y_3, D_SRC_PITCH
    add D_SRC_Y_4, D_SRC_Y_4, D_SRC_PITCH
    add D_DST, D_DST, D_DST_PITCH
    b 1b

8:
@@    vpop  {q0-q15}
    pop   {r4-r8,r9-r11,pc}


