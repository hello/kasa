/*
 * Filename : ov9718_pri.h
 *
 * History:
 *    2012/03/23 - [Long Zhao] Create
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

#ifndef __OV9718_PRI_H__
#define __OV9718_PRI_H__

#define OV9718_STANDBY    		0x0100
#define OV9718_IMG_ORI			0x0101
#define OV9718_SWRESET    		0x0103

#define OV9718_GRP_ACCESS  		0x3208

#define OV9718_GAIN_MSB    		0x350A
#define OV9718_GAIN_LSB    		0x350B

#define OV9718_VMAX_MSB    		0x0340
#define OV9718_VMAX_LSB    		0x0341
#define OV9718_HMAX_MSB   		0x0342
#define OV9718_HMAX_LSB    		0x0343

#define OV9718_X_START			0x0345
#define OV9718_Y_START			0x0347

#define OV9718_EXPO0_HSB   		0x3500
#define OV9718_EXPO0_MSB   		0x3501
#define OV9718_EXPO0_LSB    		0x3502

#define OV9718_MIPI_CTRL00		0x4800

#define OV9718_MIPI_GATE		(1<<5)

#define OV9718_V_FLIP				(1<<1)
#define OV9718_H_MIRROR			(1<<0)
#define OV9718_MIRROR_MASK		(OV9718_H_MIRROR + OV9718_V_FLIP)

#endif /* __OV9718_PRI_H__ */

