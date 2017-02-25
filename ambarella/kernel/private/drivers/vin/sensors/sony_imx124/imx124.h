/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx124/imx124_pri.h
 *
 * History:
 *    2014/07/23 - [Long Zhao] Create
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


#ifndef __IMX124_PRI_H__
#define __IMX124_PRI_H__

#define IMX124_STANDBY    		0x3000
#define IMX124_XMSTA			0x3002
#define IMX124_SWRESET    		0x3003

#define IMX124_WINMODE    		0x3007
#define IMX124_FRSEL			0x3009

#define IMX124_GAIN_LSB    		0x3014
#define IMX124_GAIN_MSB    		0x3015

#define IMX124_VMAX_LSB    		0x3018
#define IMX124_VMAX_MSB    		0x3019
#define IMX124_VMAX_HSB   		0x301A
#define IMX124_HMAX_LSB    		0x301B
#define IMX124_HMAX_MSB   		0x301C

#define IMX124_SHS1_LSB    		0x301E
#define IMX124_SHS1_MSB   		0x301F
#define IMX124_SHS1_HSB   		0x3020

#define IMX124_DCKRST			0x3044

#define IMX124_INCKSEL1			0x3061
#define IMX124_INCKSEL2			0x3062
#define IMX124_INCKSEL3			0x316C
#define IMX124_INCKSEL4			0x316D
#define IMX124_INCKSEL5			0x3170
#define IMX124_INCKSEL6			0x3171

#define IMX124_V_FLIP	(1<<0)
#define IMX124_H_MIRROR	(1<<1)

#define IMX124_DCKRST_BIT	(1<<6)

#endif /* __IMX124_PRI_H__ */

