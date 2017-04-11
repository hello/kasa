/**********************************************************************
 * boards/s2lmironman/devices.h
 * History:
 * 2014/09/17 - [Bin Wang] Created this file
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


#ifndef __DEVICES_H__
#define __DEVICES_H__

#define SUPPORT_DC_IRIS         1
#define SUPPORT_IR_LED          0
#define SUPPORT_IR_CUT          0

/* DC IRIS */
#define PWM_CHANNEL_DC_IRIS     0

/* IR LED */

#define PWM_CHANNEL_IR_LED      -1

#define GPIO_ID_LED_POWER       -1
#define GPIO_ID_LED_IR_ENABLE   -1
#define GPIO_ID_LED_NETWORK     -1
#define GPIO_ID_LED_SD_CARD     -1

#define IR_LED_ADC_CHANNEL      -1

/* IR CUT */
#define GPIO_ID_IR_CUT_CTRL     -1

typedef enum {
	GPIO_UNEXPORT = 0,
	GPIO_EXPORT = 1
} gpio_ex;

typedef enum {
	GPIO_IN = 0,
	GPIO_OUT = 1
} gpio_direction;

typedef enum {
	GPIO_LOW = 0,
	GPIO_HIGH = 1
} gpio_state;

#endif
