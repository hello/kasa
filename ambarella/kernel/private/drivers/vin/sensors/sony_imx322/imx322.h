/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx322/imx322_pri.h
 *
 * History:
 *    2014/08/15 - [Long Zhao] Create
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


#ifndef __IMX322_PRI_H__
#define __IMX322_PRI_H__
		/* for SPI */
#ifdef CONFIG_SENSOR_IMX322_SPI
#define IMX322_STANDBY		0x0200
#define IMX322_HVREVERSE		0x0201

#define IMX322_REG_GAIN		0x021E

#define IMX322_XMSTA			0x022C

#define IMX322_HMAX_LSB		0x0203
#define IMX322_HMAX_MSB		0x0204
#define IMX322_VMAX_LSB		0x0205
#define IMX322_VMAX_MSB		0x0206

#define IMX322_SHS1_LSB		0x0208
#define IMX322_SHS1_MSB		0x0209

#define IMX322_STREAMING		0x00
#define IMX322_H_MIRROR		(1<<1)
#define IMX322_V_FLIP			(1<<0)
#else	/* for I2C */
#define IMX322_STANDBY		0x0100
#define IMX322_HVREVERSE		0x0101

#define IMX322_REG_GAIN		0x301E

#define IMX322_XMSTA			0x302C

#define IMX322_VMAX_MSB		0x0340
#define IMX322_VMAX_LSB		0x0341
#define IMX322_HMAX_MSB		0x0342
#define IMX322_HMAX_LSB		0x0343

#define IMX322_SHS1_MSB		0x0202
#define IMX322_SHS1_LSB		0x0203

#define IMX322_STREAMING		0x01
#define IMX322_H_MIRROR		(1<<0)
#define IMX322_V_FLIP			(1<<1)
#endif

#endif /* __IMX322_PRI_H__ */

