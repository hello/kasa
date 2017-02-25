/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx136/imx136_pri.h
 *
 * History:
 *    2012/02/21 - [Long Zhao] Create
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


#ifndef __IMX136_PRI_H__
#define __IMX136_PRI_H__


#define IMX136_STANDBY   			0x0200
#define IMX136_REGHOLD   			0x0201
#define IMX136_XMSTA    			0x0202
#define IMX136_SWRESET   			0x0203

#define IMX136_ADBIT      		0x0205
#define IMX136_MODE       		0x0206
#define IMX136_WINMODE    		0x0207

#define IMX136_FRSEL      		0x0209
#define IMX136_BLKLEVEL_LSB		0x020A
#define IMX136_BLKLEVEL_MSB		0x020B

#define IMX136_GAIN_LSB    		0x0214
#define IMX136_GAIN_MSB    		0x0215

#define IMX136_VMAX_LSB    		0x0218
#define IMX136_VMAX_MSB    		0x0219
#define IMX136_VMAX_HSB   		0x021A
#define IMX136_HMAX_LSB    		0x021B
#define IMX136_HMAX_MSB   		0x021C

#define IMX136_SHS1_LSB    		0x0220
#define IMX136_SHS1_MSB   		0x0221
#define IMX136_SHS1_HSB   		0x0222

#define IMX136_WINWV_OB  			0x0236

#define IMX136_WINPV_LSB 			0x0238
#define IMX136_WINPV_MSB 			0x0239
#define IMX136_WINWV_LSB 			0x023A
#define IMX136_WINWV_MSB 			0x023B
#define IMX136_WINPH_LSB 			0x023C
#define IMX136_WINPH_MSB 			0x023D
#define IMX136_WINWH_LSB 			0x023E
#define IMX136_WINWH_MSB 			0x023F

#define IMX136_OUTCTRL   			0x0244

#define IMX136_XVSLNG   			0x0246
#define IMX136_XHSLNG   			0x0247

#define IMX136_XVHSOUT_SEL		0x0249

#define IMX136_INCKSEL0   		0x025B
#define IMX136_INCKSEL1   		0x025C
#define IMX136_INCKSEL2   		0x025D
#define IMX136_INCKSEL3   		0x025E
#define IMX136_INCKSEL4   		0x025F

#define IMX136_V_FLIP				(1<<0)
#define IMX136_H_MIRROR			(1<<1)

#endif /* __IMX136_PRI_H__ */

