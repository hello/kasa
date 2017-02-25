/*******************************************************************************
 * neon_yuyv_to_nv12.S
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

#define D_Y_DST     r0
#define D_UV_DST    r1
#define D_SRC       r2
#define D_W     r3
#define D_H     r4
#define D_DST_Y_PITCH       r5
#define D_DST_UV_PITCH      r6
#define D_SRC_PITCH     r7

#define D_Y_DST_1       r8
#define D_SRC_1     r9

#define D_COUNT       r10

    .align 2
    .global asm_neon_copy_yuyv_to_nv12
    .type   asm_neon_copy_yuyv_to_nv12, %function
asm_neon_copy_yuyv_to_nv12:
    push        {r4-r8,r9-r11,lr}
@@    vpush       {q0-q15}

    /* load arguments */
    ldmia       r0, {D_Y_DST, D_UV_DST, D_SRC, D_W, D_H, D_DST_Y_PITCH, D_DST_UV_PITCH, D_SRC_PITCH}

    add D_Y_DST_1, D_Y_DST, D_DST_Y_PITCH
    add D_SRC_1, D_SRC, D_SRC_PITCH

    mov D_DST_Y_PITCH, D_DST_Y_PITCH, LSL #1
    sub D_DST_Y_PITCH, D_DST_Y_PITCH, D_W
    sub D_DST_UV_PITCH, D_DST_UV_PITCH, D_W

    mov D_SRC_PITCH, D_SRC_PITCH, LSL #1
    sub D_SRC_PITCH, D_SRC_PITCH, D_W, LSL #1

    mov D_W, D_W, LSR #4
    mov D_H, D_H, LSR #1

1:
    mov D_COUNT, D_W

2:
    vld1.64 {d0, d1},   [D_SRC]
    vld1.64 {d2, d3},   [D_SRC_1]

    add D_SRC, D_SRC, # 16
    add D_SRC_1, D_SRC_1, # 16

    vld1.64 {d4, d5},   [D_SRC]
    vld1.64 {d6, d7},   [D_SRC_1]

    veor q7, q7, q7
    veor q8, q8, q8

    vtrn.8 q0, q1 @ yyyy in q0, uuvv in q1
    vtrn.8 q2, q3 @ yyyy in q2, uuvv in q3

    vpaddl.u8 q4, q1 @ u+u, v+v
    vpaddl.u8 q5, q3 @ u+u, v+v

    vtrn.8 q0, q7
    vtrn.8 q2, q8

    vshr.u16 q4, #1 @ avg u v
    vshr.u16 q5, #1 @ avg u v

    vqmovn.u16 d18, q7
    vqmovn.u16 d19, q8

    vqmovn.u16 d20, q0
    vqmovn.u16 d21, q2

    vqmovn.u16 d12, q4
    vqmovn.u16 d13, q5

    vst1.64 {d20, d21}, [D_Y_DST]
    vst1.64 {d18, d19}, [D_Y_DST_1]
    vst1.64 {d12, d13}, [D_UV_DST]

    add D_SRC, D_SRC, # 16
    add D_SRC_1, D_SRC_1, # 16

    add D_Y_DST, D_Y_DST, # 16
    add D_Y_DST_1, D_Y_DST_1, # 16
    add D_UV_DST, D_UV_DST, # 16

    subs D_COUNT, D_COUNT, #1
    beq 7f
    b 2b

7:
    subs D_H, D_H, #1
    beq 8f

    add D_SRC, D_SRC, D_SRC_PITCH
    add D_SRC_1, D_SRC_1, D_SRC_PITCH
    add D_Y_DST, D_Y_DST, D_DST_Y_PITCH
    add D_Y_DST_1, D_Y_DST_1, D_DST_Y_PITCH
    add D_UV_DST, D_UV_DST, D_DST_UV_PITCH

    b 1b

8:
@@    vpop  {q0-q15}
    pop   {r4-r8,r9-r11,pc}


