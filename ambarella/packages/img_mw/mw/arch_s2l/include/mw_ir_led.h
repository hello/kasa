/*
 * mw_ir_led.h
 *
 * History:
 * 2014/09/17 - [Bin Wang] Created this file
 *
 * Copyright (C) 2015 Ambarella, Inc.
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

#ifndef MW_IR_LED_H_
#define MW_IR_LED_H_

#define LOW_LIGHT_THRESHOLD    445//3Lux
#define HIGH_LIGHT_THRESHOLD   840//10Lux
#define DELTA_LSV              8
#define DELAY                  3
#define DELAY_MS               500000

#define IR_LED_PWM_DUTY_MAX    99

typedef enum {
	ENABLE_DAY_NIGHT_MODE = 1,
	DISABLE_DAY_NIGHT_MODE = 0
} day_night_mode;

typedef enum {
	IR_CUT_NIGHT = 0,
	IR_CUT_DAY = 1
} ir_cut_state;

typedef enum {
	BRIGHTNESS_LEVEL_0  = 0, //closed
	BRIGHTNESS_LEVEL_1X = 30,//1.0V
	BRIGHTNESS_LEVEL_2X = 33,//1.1V
	BRIGHTNESS_LEVEL_3X = 37,//1.20V
	BRIGHTNESS_LEVEL_4X = 43//1.34V
} brightness_level;

typedef enum {
	TRESHOLD_1X = 388,//2Lux
	TRESHOLD_2X = 335,//1Lux
	TRESHOLD_3X = 260 //Under 0Lux
} threshold;

s32 ir_led_control(mw_ir_led_mode mode);
u32 ir_led_get_param(mw_ir_led_control_param * pParam);
s32 ir_led_set_param(mw_ir_led_control_param * pParam);
s32 set_led_brightness(s32 brightness);

#endif /* MW_IR_LED_H_ */
