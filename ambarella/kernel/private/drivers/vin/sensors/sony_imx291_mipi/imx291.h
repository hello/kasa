/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx291_mipi/imx291.h
 *
 * History:
 *    2016/08/23 - [Hao Zeng] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
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

#ifndef __IMX291_H__
#define __IMX291_H__

#define IMX291_STANDBY			0x3000
#define IMX291_REGHOLD			0x3001
#define IMX291_XMSTA				0x3002
#define IMX291_SWRESET			0x3003

#define IMX291_ADBIT			0x3005
#define IMX291_WINMODE			0x3007
#define IMX291_FRSEL			0x3009
#define IMX291_BLKLEVEL_LSB	0x300A
#define IMX291_BLKLEVEL_MSB	0x300B

#define IMX291_GAIN			0x3014

#define IMX291_VMAX_LSB			0x3018
#define IMX291_VMAX_MSB			0x3019
#define IMX291_VMAX_HSB			0x301A

#define IMX291_HMAX_LSB			0x301C
#define IMX291_HMAX_MSB			0x301D

#define IMX291_SHS1_LSB			0x3020
#define IMX291_SHS1_MSB			0x3021
#define IMX291_SHS1_HSB			0x3022

#define IMX291_ODBIT			0x3046

#define IMX291_INCKSEL1			0x305C
#define IMX291_INCKSEL2			0x305D
#define IMX291_INCKSEL3			0x305E
#define IMX291_INCKSEL4			0x305F
#define IMX291_INCKSEL5			0x315E
#define IMX291_INCKSEL6			0x3164

#define IMX291_REPETITION		0x3405
#define IMX291_PHY_LANE_NUM		0x3407
#define IMX291_OPB_SIZE_V		0x3414
#define IMX291_Y_OUT_SIZE_LSB	0x3418
#define IMX291_Y_OUT_SIZE_MSB	0x3419
#define IMX291_THSEXIT_LSB		0x342C
#define IMX291_THSEXIT_MSB		0x342D
#define IMX291_TCLKPRE_LSB		0x3430
#define IMX291_TCLKPRE_MSB		0x3431
#define IMX291_CSI_DT_FMT_LSB	0x3441
#define IMX291_CSI_DT_FMT_MSB	0x3442
#define IMX291_CSI_LANE_MODE		0x3443
#define IMX291_EXTCK_FREQ_LSB	0x3444
#define IMX291_EXTCK_FREQ_MSB	0x3445
#define IMX291_TCLKPOST_LSB		0x3446
#define IMX291_TCLKPOST_MSB		0x3447
#define IMX291_THSZERO_LSB		0x3448
#define IMX291_THSZERO_MSB		0x3449
#define IMX291_THSPREPARE_LSB	0x344A
#define IMX291_THSPREPARE_MSB	0x344B
#define IMX291_TCLKTRAIL_LSB		0x344C
#define IMX291_TCLKTRAIL_MSB		0x344D
#define IMX291_THSTRAIL_LSB		0x344E
#define IMX291_THSTRAIL_MSB		0x344F
#define IMX291_TCLKZERO_LSB		0x3450
#define IMX291_TCLKZERO_MSB		0x3451
#define IMX291_TCLKPREPARE_LSB	0x3452
#define IMX291_TCLKPREPARE_MSB	0x3453
#define IMX291_TLPX_LSB			0x3454
#define IMX291_TLPX_MSB			0x3455
#define IMX291_X_OUT_SIZE_LSB	0x3472
#define IMX291_X_OUT_SIZE_MSB	0x3473

#define IMX291_V_FLIP	(1<<0)
#define IMX291_H_MIRROR	(1<<1)

#define IMX291_HI_GAIN_MODE	(1<<4)

#endif /* __IMX291_H__ */

