/*
 * Filename : mn34420pl.h
 *
 * History:
 *    2015/12/15 - [Hao Zeng] Create
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

#ifndef __MN34420PL_PRI_H__
#define __MN34420PL_PRI_H__

#define GAIN_160_STEPS (1)

#define MN34420PL_RESET_REG_H	0x3000
#define MN34420PL_RESET_REG_L	0x3001

#define MN34420PL_VLATCH_HOLD	0x3041

#define MN34420PL_AGAIN_H	0x3100
#define MN34420PL_AGAIN_L	0x3101
#define MN34420PL_DGAIN_H	0x3102
#define MN34420PL_DGAIN_L	0x3103

#define MN34420PL_AGAIN_WDR1	0x3108
#define MN34420PL_AGAIN_WDR2	0x3110
#define MN34420PL_AGAIN_WDR3	0x3116
#define MN34420PL_DGAIN_WDR1	0x310A
#define MN34420PL_DGAIN_WDR2	0x3112
#define MN34420PL_DGAIN_WDR3	0x3118

#define MN34420PL_SHTPOS_H	0x3104
#define MN34420PL_SHTPOS_L	0x3105
#define MN34420PL_LONG_EXP	0x3106

#define MN34420PL_SHTPOS_WDR1	0x310C
#define MN34420PL_SHTPOS_WDR2	0x3114
#define MN34420PL_SHTPOS_WDR3	0x311A

#define MN34420PL_VCYCLE_H	0x4302
#define MN34420PL_VCYCLE_L	0x4303
#define MN34420PL_HCYCLE_H	0x4304
#define MN34420PL_HCYCLE_L	0x4305

#define MN34420PL_M1_RDPOS_WDR1	0x431A
#define MN34420PL_M1_RDPOS_WDR2	0x431C
#define MN34420PL_M1_RDPOS_WDR3	0x431E

#define MN34420PL_MODE	0x4301
#define MN34420PL_V_FLIP (1<<7)

#endif /* __MN34420PL_PRI_H__ */

