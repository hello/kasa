/*
 * Filename : ov5658_pri.h
 *
 * History:
 *    2014/05/28 - [Long Zhao] Create
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

#ifndef __OV5658_PRI_H__
#define __OV5658_PRI_H__

#define OV5658_STANDBY					0x0100
#define OV5658_SWRESET 					0x0103
#define OV5658_CHIP_ID_H				0x300A
#define OV5658_CHIP_ID_L				0x300B
#define OV5658_GRP_ACCESS				0x3208
#define OV5658_LONG_EXPO_H			0x3500
#define OV5658_LONG_EXPO_M			0x3501
#define OV5658_LONG_EXPO_L				0x3502
#define OV5658_AGC_ADJ_H				0x350A
#define OV5658_AGC_ADJ_L				0x350B
#define OV5658_HMAX_MSB				0x380C
#define OV5658_HMAX_LSB				0x380D
#define OV5658_VMAX_MSB				0x380E
#define OV5658_VMAX_LSB				0x380F
#define OV5658_MIPI_CTRL00				0x4800

#define OV5658_MIPI_GATE				(1<<5)

#define OV5658_V_FORMAT		0x3820
#define OV5658_H_FORMAT		0x3821
#define OV5658_V_FLIP			(0x03<<1)
#define OV5658_H_MIRROR		(0x03<<1)

#endif /* __OV5658_PRI_H__ */

