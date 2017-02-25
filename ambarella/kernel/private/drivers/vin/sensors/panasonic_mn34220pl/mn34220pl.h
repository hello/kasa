/*
 * Filename : mn34220pl_pri.h
 *
 * History:
 *    2013/06/08 - [Long Zhao] Create
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

#ifndef __MN34220PL_PRI_H__
#define __MN34220PL_PRI_H__

#define GAIN_160_STEPS (1)

#define MN34220PL_RESET		0x3001

#define MN34220PL_VCYCLE_H	0x0340
#define MN34220PL_VCYCLE_L	0x0341
#define MN34220PL_HCYCLE_H	0x0342
#define MN34220PL_HCYCLE_L	0x0343

#define MN34220PL_AGAIN_H	0x0204
#define MN34220PL_AGAIN_L	0x0205
#define MN34220PL_DGAIN_H	0x3108
#define MN34220PL_DGAIN_L	0x3109

#define MN34220PL_SHTPOS_H	0x0221
#define MN34220PL_SHTPOS_M	0x0202
#define MN34220PL_SHTPOS_L	0x0203

#define MN34220PL_SHTPOS_WDR1_H	0x312A
#define MN34220PL_SHTPOS_WDR1_L	0x312B
#define MN34220PL_SHTPOS_WDR2_H	0x312C
#define MN34220PL_SHTPOS_WDR2_L	0x312D
#define MN34220PL_SHTPOS_WDR3_H	0x312E
#define MN34220PL_SHTPOS_WDR3_L	0x312F

#define MN34220PL_MODE	0x0101
#define MN34220PL_V_FLIP (1<<1)

#endif /* __MN34220PL_PRI_H__ */

