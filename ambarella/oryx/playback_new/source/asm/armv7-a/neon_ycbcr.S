/*******************************************************************************
 * neon_ycbcr.S
 *
 * History:
 *    2015/09/15 - [Zhi He] create file
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

#define D_SRC_Y     r0
#define D_SRC_CB    r1
#define D_SRC_CR    r2
#define D_DES           r3
#define D_SRC_STRIDE_Y      r4
#define D_SRC_STRIDE_CBCR       r5
#define D_DES_STRIDE        r6
#define D_WIDTH         r7
#define D_HEIGHT        r8
#define D_SRC_Y_1       r9
#define D_DES_1	    r10
#define D_COUNT	    r11
#define D_DRAM_STRIDE	    r12

    .align 2
    .global asm_neon_yuv420p_to_vyu565
    .type   asm_neon_yuv420p_to_vyu565, %function
asm_neon_yuv420p_to_vyu565:
    push        {r4-r8,r9-r11,lr}
@@    vpush       {q4-q13}

    /* load arguments */
    ldmia       r0, {D_SRC_Y, D_SRC_CB, D_SRC_CR, D_DES, D_SRC_STRIDE_Y, D_SRC_STRIDE_CBCR, D_DES_STRIDE, D_WIDTH, D_HEIGHT}

    add D_SRC_Y_1, D_SRC_Y, D_SRC_STRIDE_Y
    add D_DES_1, D_DES, D_DES_STRIDE, LSL #1

    mov D_SRC_STRIDE_Y, D_SRC_STRIDE_Y, LSL #1
    mov D_DES_STRIDE, D_DES_STRIDE, LSL #2
    sub D_SRC_STRIDE_CBCR, D_SRC_STRIDE_CBCR, D_WIDTH, LSR #1
    sub D_SRC_STRIDE_Y, D_SRC_STRIDE_Y, D_WIDTH
    sub D_DES_STRIDE, D_DES_STRIDE, D_WIDTH, LSL #1

    mov D_HEIGHT, D_HEIGHT, LSR #1
    add D_HEIGHT, D_HEIGHT, # 1

1:
    subs D_HEIGHT, D_HEIGHT, #1
    beq 8f

    mov D_COUNT, D_WIDTH, LSR #4

2:

    vld1.64 {d0, d1},   [D_SRC_Y]               @load src_y0, 1 x 16
    vld1.64 {d2, d3},   [D_SRC_Y_1]           @load src_y1, 1 x 16
    vld1.64 {d4},   [D_SRC_CB]                    @load src_cb, 1 x 8
    vld1.64 {d5},   [D_SRC_CR]                     @load src_cr, 1 x 8

    vmov.i8 q8, #2
    veor.64 q3, q5, q5
    vmov.i8 q9, #4
    veor.64 q4, q5, q5

    vqadd.u8 q0, q0, q8
    vqadd.u8 q2, q2, q9

    vtrn.8  q0, q3
    vtrn.8  q1, q4

    vshr.u16 q0, #2                                                                         @ y
    vshr.u16 q1, #2
    vshr.u16 q3, #2
    vshr.u16 q4, #2

    vshl.u16 q0, #5
    vshl.u16 q1, #5
    vshl.u16 q3, #5
    vshl.u16 q4, #5

    vmovl.u8    q5, d4
    vmovl.u8    q6, d5

    vshr.u16 q6, #3                                                                           @ cb cr
    vshr.u16 q5, #3
    vshl.u16 q6, #11

    vorr q5, q5, q6
    vorr q0, q0, q5
    vorr q1, q1, q5
    vorr q3, q3, q5
    vorr q4, q4, q5

    vmovl.u16    q6, d0
    vmovl.u16    q7, d1
    vmovl.u16    q8, d6
    vmovl.u16    q9, d7

    vshl.u32 q8, #16
    vshl.u32 q9, #16

    vorr q6, q6, q8
    vorr q7, q7, q9

    vmovl.u16    q10, d2
    vmovl.u16    q11, d3
    vmovl.u16    q12, d8
    vmovl.u16    q13, d9

    vshl.u32 q12, #16
    vshl.u32 q13, #16

    vorr q10, q10, q12
    vorr q11, q11, q13

    vst1.64 {d12, d13, d14, d15}, [D_DES]
    vst1.64 {d20, d21, d22, d23}, [D_DES_1]

    add D_SRC_Y, D_SRC_Y, #16
    add D_SRC_Y_1, D_SRC_Y_1, #16
    add D_SRC_CB, D_SRC_CB, #8
    add D_SRC_CR, D_SRC_CR, #8
    add D_DES, D_DES, #32
    add D_DES_1, D_DES_1, #32

    subs D_COUNT, D_COUNT, #1
    beq 7f
    b 2b
7:
    add D_SRC_Y, D_SRC_Y, D_SRC_STRIDE_Y
    add D_SRC_Y_1, D_SRC_Y_1, D_SRC_STRIDE_Y
    add D_SRC_CB, D_SRC_CB, D_SRC_STRIDE_CBCR
    add D_SRC_CR, D_SRC_CR, D_SRC_STRIDE_CBCR
    add D_DES, D_DES, D_DES_STRIDE
    add D_DES_1, D_DES_1, D_DES_STRIDE
    b 1b

8:
@@    vpop  {q4-q13}
    pop   {r4-r8,r9-r11,pc}


    .align 2
    .global asm_neon_yuv420p_to_avyu8888
    .type   asm_neon_yuv420p_to_avyu8888, %function
asm_neon_yuv420p_to_avyu8888:
    push        {r4-r8,r9-r11,lr}
@@    vpush       {q4-q13}

    /* load arguments */
    ldmia       r0, {D_SRC_Y, D_SRC_CB, D_SRC_CR, D_DES, D_SRC_STRIDE_Y, D_SRC_STRIDE_CBCR, D_DES_STRIDE, D_WIDTH, D_HEIGHT}

    add D_SRC_Y_1, D_SRC_Y, D_SRC_STRIDE_Y
    add D_DES_1, D_DES, D_DES_STRIDE, LSL #2

    mov D_SRC_STRIDE_Y, D_SRC_STRIDE_Y, LSL #1
    mov D_DES_STRIDE, D_DES_STRIDE, LSL #3
    sub D_SRC_STRIDE_CBCR, D_SRC_STRIDE_CBCR, D_WIDTH, LSR #1
    sub D_SRC_STRIDE_Y, D_SRC_STRIDE_Y, D_WIDTH
    sub D_DES_STRIDE, D_DES_STRIDE, D_WIDTH, LSL #2

    mov D_HEIGHT, D_HEIGHT, LSR #1
    add D_HEIGHT, D_HEIGHT, # 1

1:
    subs D_HEIGHT, D_HEIGHT, #1
    beq 8f

    mov D_COUNT, D_WIDTH, LSR #4

2:

    vld1.64 {d0, d1},   [D_SRC_Y]               @load src_y0, 1 x 16
    vld1.64 {d2, d3},   [D_SRC_Y_1]           @load src_y1, 1 x 16
    vld1.64 {d4},   [D_SRC_CB]                    @load src_cb, 1 x 8
    vld1.64 {d5},   [D_SRC_CR]                     @load src_cr, 1 x 8

    veor.64 q3, q5, q5
    veor.64 q4, q5, q5

    vtrn.8  q3, q0
    vtrn.8  q4, q1

    vmovl.u16    q5, d6                                @y
    vmovl.u16    q7, d7
    vmovl.u16    q6, d0
    vmovl.u16    q8, d1

    vmovl.u16    q11, d8                              @y1
    vmovl.u16    q13, d9
    vmovl.u16    q12, d2
    vmovl.u16    q14, d3

    vmovl.u8    q9, d4                                 @cb cr
    vmovl.u8    q10, d5

    vmovl.u16    q0, d20
    vmovl.u16    q1, d21
    vmovl.u16    q2, d18
    vmovl.u16    q3, d19

    vshl.u32       q0, #16
    vshl.u32       q1, #16

    vmov.i32      q9, #0xff000000
    vorr q0, q0, q2
    vorr q1, q1, q3
    vorr q0, q0, q9
    vorr q1, q1, q9

    vorr q5, q5, q0
    vorr q6, q6, q0
    vorr q7, q7, q1
    vorr q8, q8, q1

    vorr q11, q11, q0
    vorr q12, q12, q0
    vorr q13, q13, q1
    vorr q14, q14, q1

    vtrn.32 q5, q6
    vtrn.32 q7, q8

    vtrn.32 q11, q12
    vtrn.32 q13, q14

    vmov d0, d12
    vmov d1, d16
    vmov d2, d24
    vmov d3, d28

    vmov d12, d11
    vmov d16, d15
    vmov d24, d23
    vmov d28, d27

    vmov d11, d0
    vmov d15, d1
    vmov d23, d2
    vmov d27, d3

    vst1.64 {d10, d11, d12, d13}, [D_DES]
    add D_DES, D_DES, #32
    vst1.64 {d22, d23, d24, d25}, [D_DES_1]
    add D_DES_1, D_DES_1, #32
    vst1.64 {d14, d15, d16, d17}, [D_DES]
    vst1.64 {d26, d27, d28, d29}, [D_DES_1]

    add D_SRC_Y, D_SRC_Y, #16
    add D_SRC_Y_1, D_SRC_Y_1, #16
    add D_SRC_CB, D_SRC_CB, #8
    add D_SRC_CR, D_SRC_CR, #8
    add D_DES, D_DES, #32
    add D_DES_1, D_DES_1, #32

    subs D_COUNT, D_COUNT, #1
    beq 7f
    b 2b
7:
    add D_SRC_Y, D_SRC_Y, D_SRC_STRIDE_Y
    add D_SRC_Y_1, D_SRC_Y_1, D_SRC_STRIDE_Y
    add D_SRC_CB, D_SRC_CB, D_SRC_STRIDE_CBCR
    add D_SRC_CR, D_SRC_CR, D_SRC_STRIDE_CBCR
    add D_DES, D_DES, D_DES_STRIDE
    add D_DES_1, D_DES_1, D_DES_STRIDE
    b 1b

8:
@@    vpop  {q4-q13}
    pop   {r4-r8,r9-r11,pc}


    .align 2
    .global asm_neon_yuv420p_to_ayuv8888
    .type   asm_neon_yuv420p_to_ayuv8888, %function
asm_neon_yuv420p_to_ayuv8888:
    push        {r4-r8,r9-r12,lr}
@@    vpush       {q4-q13}

    /* load arguments */
    ldmia       r0, {D_SRC_Y, D_SRC_CB, D_SRC_CR, D_DES, D_SRC_STRIDE_Y, D_SRC_STRIDE_CBCR, D_DES_STRIDE, D_WIDTH, D_HEIGHT}

    add D_SRC_Y_1, D_SRC_Y, D_SRC_STRIDE_Y
    add D_DES_1, D_DES, D_DES_STRIDE, LSL #2

    mov D_SRC_STRIDE_Y, D_SRC_STRIDE_Y, LSL #1
    mov D_DES_STRIDE, D_DES_STRIDE, LSL #3
    sub D_SRC_STRIDE_CBCR, D_SRC_STRIDE_CBCR, D_WIDTH, LSR #1
    sub D_SRC_STRIDE_Y, D_SRC_STRIDE_Y, D_WIDTH
    sub D_DES_STRIDE, D_DES_STRIDE, D_WIDTH, LSL #2

    pld [D_SRC_Y]
    pld [D_SRC_Y_1]
    pld [D_SRC_CB]
    pld [D_SRC_CR]

    mov D_DRAM_STRIDE, #16

    mov D_HEIGHT, D_HEIGHT, LSR #1
    add D_HEIGHT, D_HEIGHT, # 1

1:
    subs D_HEIGHT, D_HEIGHT, #1
    beq 8f

    mov D_COUNT, D_WIDTH, LSR #4

2:

    vld1.64 {d0, d1},   [D_SRC_Y]               @load src_y0, 1 x 16
    vld1.64 {d2, d3},   [D_SRC_Y_1]           @load src_y1, 1 x 16
    vld1.64 {d4},   [D_SRC_CB]                    @load src_cb, 1 x 8
    vld1.64 {d5},   [D_SRC_CR]                     @load src_cr, 1 x 8

    add D_SRC_Y, D_SRC_Y, #16
    add D_SRC_Y_1, D_SRC_Y_1, #16
    add D_SRC_CB, D_SRC_CB, #8
    add D_SRC_CR, D_SRC_CR, #8

    pld [D_SRC_Y]
    pld [D_SRC_Y_1]
    pld [D_SRC_CB]
    pld [D_SRC_CR]

    veor.64 q3, q5, q5
    veor.64 q4, q5, q5

    vtrn.8  q3, q0
    vtrn.8  q4, q1

    vmovl.u16    q5, d6                                @y
    vmovl.u16    q7, d7
    vmovl.u16    q6, d0
    vmovl.u16    q8, d1

    vshl.u32       q5, #8
    vshl.u32       q7, #8
    vshl.u32       q6, #8
    vshl.u32       q8, #8

    vmovl.u16    q11, d8                              @y1
    vmovl.u16    q13, d9
    vmovl.u16    q12, d2
    vmovl.u16    q14, d3

    vshl.u32       q11, #8
    vshl.u32       q13, #8
    vshl.u32       q12, #8
    vshl.u32       q14, #8

    vmovl.u8    q9, d4                                 @cb cr
    vmovl.u8    q10, d5

    vmovl.u16    q0, d20
    vmovl.u16    q1, d21
    vmovl.u16    q2, d18
    vmovl.u16    q3, d19

    vshl.u32       q2, #8
    vshl.u32       q3, #8

    vmov.i32      q9, #0xff000000
    vorr q0, q0, q2
    vorr q1, q1, q3
    vorr q0, q0, q9
    vorr q1, q1, q9

    vorr q5, q5, q0
    vorr q6, q6, q0
    vorr q7, q7, q1
    vorr q8, q8, q1

    vorr q11, q11, q0
    vorr q12, q12, q0
    vorr q13, q13, q1
    vorr q14, q14, q1

    vst2.32 {d10, d12}, [D_DES], D_DRAM_STRIDE
    vst2.32 {d11, d13}, [D_DES], D_DRAM_STRIDE
    vst2.32 {d14, d16}, [D_DES], D_DRAM_STRIDE
    vst2.32 {d15, d17}, [D_DES], D_DRAM_STRIDE

    vst2.32 {d22, d24}, [D_DES_1], D_DRAM_STRIDE
    vst2.32 {d23, d25}, [D_DES_1], D_DRAM_STRIDE
    vst2.32 {d26, d28}, [D_DES_1], D_DRAM_STRIDE
    vst2.32 {d27, d29}, [D_DES_1], D_DRAM_STRIDE

    subs D_COUNT, D_COUNT, #1
    beq 7f
    b 2b
7:
    add D_SRC_Y, D_SRC_Y, D_SRC_STRIDE_Y
    add D_SRC_Y_1, D_SRC_Y_1, D_SRC_STRIDE_Y
    add D_SRC_CB, D_SRC_CB, D_SRC_STRIDE_CBCR
    add D_SRC_CR, D_SRC_CR, D_SRC_STRIDE_CBCR
    pld [D_SRC_Y]
    pld [D_SRC_Y_1]
    pld [D_SRC_CB]
    pld [D_SRC_CR]
    add D_DES, D_DES, D_DES_STRIDE
    add D_DES_1, D_DES_1, D_DES_STRIDE
    b 1b

8:
@@    vpop  {q4-q13}
    pop   {r4-r8,r9-r12,pc}


    .align 2
    .global asm_neon_yuv420p_to_ayuv8888_ori
    .type   asm_neon_yuv420p_to_ayuv8888_ori, %function
asm_neon_yuv420p_to_ayuv8888_ori:
    push        {r4-r8,r9-r11,lr}
@@    vpush       {q4-q13}

    /* load arguments */
    ldmia       r0, {D_SRC_Y, D_SRC_CB, D_SRC_CR, D_DES, D_SRC_STRIDE_Y, D_SRC_STRIDE_CBCR, D_DES_STRIDE, D_WIDTH, D_HEIGHT}

    add D_SRC_Y_1, D_SRC_Y, D_SRC_STRIDE_Y
    add D_DES_1, D_DES, D_DES_STRIDE, LSL #2

    mov D_SRC_STRIDE_Y, D_SRC_STRIDE_Y, LSL #1
    mov D_DES_STRIDE, D_DES_STRIDE, LSL #3
    sub D_SRC_STRIDE_CBCR, D_SRC_STRIDE_CBCR, D_WIDTH, LSR #1
    sub D_SRC_STRIDE_Y, D_SRC_STRIDE_Y, D_WIDTH
    sub D_DES_STRIDE, D_DES_STRIDE, D_WIDTH, LSL #2

    mov D_HEIGHT, D_HEIGHT, LSR #1
    add D_HEIGHT, D_HEIGHT, # 1

1:
    subs D_HEIGHT, D_HEIGHT, #1
    beq 8f

    mov D_COUNT, D_WIDTH, LSR #4

2:

    vld1.64 {d0, d1},   [D_SRC_Y]               @load src_y0, 1 x 16
    vld1.64 {d2, d3},   [D_SRC_Y_1]           @load src_y1, 1 x 16
    vld1.64 {d4},   [D_SRC_CB]                    @load src_cb, 1 x 8
    vld1.64 {d5},   [D_SRC_CR]                     @load src_cr, 1 x 8

    veor.64 q3, q5, q5
    veor.64 q4, q5, q5

    vtrn.8  q3, q0
    vtrn.8  q4, q1

    vmovl.u16    q5, d6                                @y
    vmovl.u16    q7, d7
    vmovl.u16    q6, d0
    vmovl.u16    q8, d1

    vshl.u32       q5, #8
    vshl.u32       q7, #8
    vshl.u32       q6, #8
    vshl.u32       q8, #8

    vmovl.u16    q11, d8                              @y1
    vmovl.u16    q13, d9
    vmovl.u16    q12, d2
    vmovl.u16    q14, d3

    vshl.u32       q11, #8
    vshl.u32       q13, #8
    vshl.u32       q12, #8
    vshl.u32       q14, #8

    vmovl.u8    q9, d4                                 @cb cr
    vmovl.u8    q10, d5

    vmovl.u16    q0, d20
    vmovl.u16    q1, d21
    vmovl.u16    q2, d18
    vmovl.u16    q3, d19

    vshl.u32       q2, #8
    vshl.u32       q3, #8

    vmov.i32      q9, #0xff000000
    vorr q0, q0, q2
    vorr q1, q1, q3
    vorr q0, q0, q9
    vorr q1, q1, q9

    vorr q5, q5, q0
    vorr q6, q6, q0
    vorr q7, q7, q1
    vorr q8, q8, q1

    vorr q11, q11, q0
    vorr q12, q12, q0
    vorr q13, q13, q1
    vorr q14, q14, q1

    vtrn.32 q5, q6
    vtrn.32 q7, q8

    vtrn.32 q11, q12
    vtrn.32 q13, q14

    vmov d0, d12
    vmov d1, d16
    vmov d2, d24
    vmov d3, d28

    vmov d12, d11
    vmov d16, d15
    vmov d24, d23
    vmov d28, d27

    vmov d11, d0
    vmov d15, d1
    vmov d23, d2
    vmov d27, d3

    vst1.64 {d10, d11, d12, d13}, [D_DES]
    add D_DES, D_DES, #32
    vst1.64 {d22, d23, d24, d25}, [D_DES_1]
    add D_DES_1, D_DES_1, #32
    vst1.64 {d14, d15, d16, d17}, [D_DES]
    vst1.64 {d26, d27, d28, d29}, [D_DES_1]

    add D_SRC_Y, D_SRC_Y, #16
    add D_SRC_Y_1, D_SRC_Y_1, #16
    add D_SRC_CB, D_SRC_CB, #8
    add D_SRC_CR, D_SRC_CR, #8
    add D_DES, D_DES, #32
    add D_DES_1, D_DES_1, #32

    subs D_COUNT, D_COUNT, #1
    beq 7f
    b 2b
7:
    add D_SRC_Y, D_SRC_Y, D_SRC_STRIDE_Y
    add D_SRC_Y_1, D_SRC_Y_1, D_SRC_STRIDE_Y
    add D_SRC_CB, D_SRC_CB, D_SRC_STRIDE_CBCR
    add D_SRC_CR, D_SRC_CR, D_SRC_STRIDE_CBCR
    add D_DES, D_DES, D_DES_STRIDE
    add D_DES_1, D_DES_1, D_DES_STRIDE
    b 1b

8:
@@    vpop  {q4-q13}
    pop   {r4-r8,r9-r11,pc}

