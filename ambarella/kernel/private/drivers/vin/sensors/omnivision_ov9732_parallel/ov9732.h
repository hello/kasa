/*
 * Filename : ov9732.h
 *
 * History:
 *    2015/07/28 - [Hao Zeng] Create
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

#ifndef __OV9732_PRI_H__
#define __OV9732_PRI_H__

#define OV9732_STANDBY			0x0100
#define OV9732_SWRESET			0x0103

#define OV9732_CHIP_ID_H			0x300A
#define OV9732_CHIP_ID_L			0x300B
#define OV9732_CHIP_REV			0x302A

#define OV9732_GRP_ACCESS		0x3208

#define OV9732_L_EXPO_HSB		0x3500
#define OV9732_L_EXPO_MSB		0x3501
#define OV9732_L_EXPO_LSB		0x3502

#define OV9732_GAIN_MSB			0x350A
#define OV9732_GAIN_LSB			0x350B

#define OV9732_HTS_MSB			0x380C
#define OV9732_HTS_LSB			0x380D
#define OV9732_VTS_MSB			0x380E
#define OV9732_VTS_LSB			0x380F

#define OV9732_FORMAT0			0x3820

#define OV9732_R_GAIN_MSB		0x5180
#define OV9732_R_GAIN_LSB		0x5181
#define OV9732_G_GAIN_MSB		0x5182
#define OV9732_G_GAIN_LSB		0x5183
#define OV9732_B_GAIN_MSB		0x5184
#define OV9732_B_GAIN_LSB		0x5185

#define OV9732_V_FLIP				(1<<2)
#define OV9732_H_MIRROR			(1<<3)
#define OV9732_MIRROR_MASK		(OV9732_H_MIRROR + OV9732_V_FLIP)

#endif /* __OV9732_PRI_H__ */

