/*
 * Filename : ar0230_pri.h
 *
 * History:
 *    2014/11/18 - [Long Zhao] Create
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

#ifndef __AR0230_PRI_H__
#define __AR0230_PRI_H__

#define AR0230_RESET_REGISTER				0x301A
#define AR0230_LINE_LENGTH_PCK			0x300C
#define AR0230_FRAME_LENGTH_LINES			0x300A

#define AR0230_CHIP_VERSION_REG			0x3000
#define AR0230_COARSE_INTEGRATION_TIME	0x3012

#define AR0230_VT_PIX_CLK_DIV				0x302A
#define AR0230_VT_SYS_CLK_DIV				0x302C
#define AR0230_PRE_PLL_CLK_DIV				0x302E
#define AR0230_PLL_MULTIPLIER				0x3030

#define AR0230_DGAIN						0x305E
#define AR0230_AGAIN						0x3060
#define AR0230_DCG_CTL						0x3100

/* AR0230 mirror mode */
#define AR0230_READ_MODE				0x3040
#define AR0230_H_MIRROR			(0x01 << 14)
#define AR0230_V_FLIP				(0x01 << 15)
#define AR0230_MIRROR_MASK			(AR0230_H_MIRROR + AR0230_V_FLIP)

#endif /* __AR0230_PRI_H__ */

