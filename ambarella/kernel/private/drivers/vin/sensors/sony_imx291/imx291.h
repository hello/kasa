/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx291/imx291_pri.h
 *
 * History:
 *    2014/12/05 - [Long Zhao] Create
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


#ifndef __IMX291_PRI_H__
#define __IMX291_PRI_H__

#define IMX291_STANDBY    		0x3000
#define IMX291_REGHOLD			0x3001
#define IMX291_XMSTA			0x3002
#define IMX291_SWRESET    		0x3003

#define IMX291_ADBIT    			0x3005
#define IMX291_WINMODE    		0x3007
#define IMX291_FRSEL			0x3009
#define IMX291_BLKLEVEL_LSB	0x300A
#define IMX291_BLKLEVEL_MSB	0x300B

#define IMX291_GAIN		    		0x3014

#define IMX291_VMAX_LSB    		0x3018
#define IMX291_VMAX_MSB    		0x3019
#define IMX291_VMAX_HSB   		0x301A

#define IMX291_HMAX_LSB    		0x301C
#define IMX291_HMAX_MSB   		0x301D

#define IMX291_SHS1_LSB    		0x3020
#define IMX291_SHS1_MSB   		0x3021
#define IMX291_SHS1_HSB   		0x3022

#define IMX291_ODBIT			0x3046

#define IMX291_INCKSEL1			0x305C
#define IMX291_INCKSEL2			0x305D
#define IMX291_INCKSEL3			0x305E
#define IMX291_INCKSEL4			0x305F
#define IMX291_INCKSEL5			0x315E
#define IMX291_INCKSEL6			0x3164

#define IMX291_V_FLIP	(1<<0)
#define IMX291_H_MIRROR	(1<<1)

#define IMX291_HI_GAIN_MODE	(1<<4)

#endif /* __IMX291_PRI_H__ */

