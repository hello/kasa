/*
 * Filename : ov9750_pri.h
 *
 * History:
 *    2015/05/08 - [Hao Zeng] Create
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

#ifndef __OV9750_PRI_H__
#define __OV9750_PRI_H__

#define OV9750_STANDBY			0x0100
#define OV9750_SWRESET			0x0103

#define OV9750_GRP_ACCESS		0x3208

#define OV9750_GAIN_MSB			0x3508
#define OV9750_GAIN_LSB			0x3509

#define OV9750_HMAX_MSB			0x380C
#define OV9750_HMAX_LSB			0x380D
#define OV9750_VMAX_MSB			0x380E
#define OV9750_VMAX_LSB			0x380F

#define OV9750_EXPO0_HSB			0x3500
#define OV9750_EXPO0_MSB			0x3501
#define OV9750_EXPO0_LSB			0x3502

#define OV9750_GAIN_DCG			0x37C7

#define OV9750_V_FORMAT			0x3820
#define OV9750_H_FORMAT			0x3821

#define OV9750_BLC_CTRL10		0x4010

#define OV9750_R_GAIN_MSB		0x5032
#define OV9750_R_GAIN_LSB		0x5033
#define OV9750_G_GAIN_MSB		0x5034
#define OV9750_G_GAIN_LSB		0x5035
#define OV9750_B_GAIN_MSB		0x5036
#define OV9750_B_GAIN_LSB		0x5037

#define OV9750_V_FLIP				(3<<1)
#define OV9750_H_MIRROR			(3<<1)

#endif /* __OV9750_PRI_H__ */

