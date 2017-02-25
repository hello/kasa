/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx123/imx123_pri.h
 *
 * History:
 *    2014/08/05 - [Long Zhao] Create
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


#ifndef __IMX123_PRI_H__
#define __IMX123_PRI_H__

#define IMX123_STANDBY    		0x3000
#define IMX123_REGHOLD			0x3001
#define IMX123_XMSTA			0x3002
#define IMX123_SWRESET    		0x3003

#define IMX123_WINMODE    		0x3007

#define IMX123_GAIN_LSB    		0x3014
#define IMX123_GAIN_MSB    		0x3015

#define IMX123_VMAX_LSB    		0x3018
#define IMX123_VMAX_MSB    		0x3019
#define IMX123_VMAX_HSB   		0x301A
#define IMX123_HMAX_LSB    		0x301B
#define IMX123_HMAX_MSB   		0x301C

#define IMX123_SHS1_LSB    		0x301E
#define IMX123_SHS1_MSB   		0x301F
#define IMX123_SHS1_HSB   		0x3020

#define IMX123_SHS2_LSB    		0x3021
#define IMX123_SHS2_MSB   		0x3022
#define IMX123_SHS2_HSB   		0x3023

#define IMX123_SHS3_LSB    		0x3024
#define IMX123_SHS3_MSB   		0x3025
#define IMX123_SHS3_HSB   		0x3026

#define IMX123_RHS1_LSB    		0x302E
#define IMX123_RHS1_MSB   		0x302F
#define IMX123_RHS1_HSB   		0x3030

#define IMX123_RHS2_LSB    		0x3031
#define IMX123_RHS2_MSB   		0x3032
#define IMX123_RHS2_HSB   		0x3033

#define IMX123_WINWV_OB			0x3036
#define IMX123_WINPV_LSB			0x3038
#define IMX123_WINPV_MSB			0x3039

#define IMX123_DCKRST			0x3044

#define IMX123_GAIN2_LSB    		0x30F2
#define IMX123_GAIN2_MSB    		0x30F3

#define IMX123_V_FLIP	(1<<0)
#define IMX123_H_MIRROR	(1<<1)

#define IMX123_DCKRST_BIT	(1<<6)

#define IMX123_QXGA_BRL		(1564)
#define IMX123_1080P_BRL		(1108)
#define IMX123_QXGA_H_PIXEL	(2048)
#define IMX123_1080P_H_PIXEL	(1936)
#define IMX123_H_PERIOD		(2256)
#define IMX123_QXGA_HBLANK	(192)

#define IMX123_QXGA_2X_RHS1		(0x142) /* max RHS1 is 0x4EA for 30fps(VMAX=2200) */

#define IMX123_QXGA_3X_RHS1		(0x26C)
#define IMX123_QXGA_3X_RHS2		(0x2C8)

#endif /* __IMX123_PRI_H__ */

