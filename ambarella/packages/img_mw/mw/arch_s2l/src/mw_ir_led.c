/*
 * mw_ir_led.c
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

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <basetypes.h>

#include "iav_common.h"
#include "iav_ioctl.h"
#include "iav_vin_ioctl.h"
#include "iav_vout_ioctl.h"
#include "ambas_imgproc_arch.h"

#include "img_adv_struct_arch.h"
#include "img_api_adv_arch.h"
#include "ambas_imgproc_ioctl_arch.h"

#include "mw_aaa_params.h"
#include "mw_api.h"
#include "mw_pri_struct.h"
#include "mw_image_priv.h"

#include "mw_ir_led.h"
#include "img_dev.h"


static mw_ir_led_mode G_ir_led_mode = MW_IR_LED_MODE_OFF;
static gpio_state G_ir_led_enable_state = GPIO_LOW;
static ir_cut_state G_ir_cut_ctrl_state = IR_CUT_DAY;
static s32 G_ir_led_brightness = -1;
static day_night_mode G_day_night_mode = DISABLE_DAY_NIGHT_MODE;
/* we use global variables to remember the HW state and we do not r/w HW
 * unless the HW state change.*/

mw_ir_led_control_param G_ir_led_control_param = {
	.open_threshold = LOW_LIGHT_THRESHOLD,
	.close_threshold = HIGH_LIGHT_THRESHOLD,
	.open_delay = DELAY,
	.close_delay = DELAY, //500ms interval,3 seconds delay
	.threshold_1X = TRESHOLD_1X,
	.threshold_2X = TRESHOLD_2X,
	.threshold_3X = TRESHOLD_3X,
};

u32 ir_led_get_param(mw_ir_led_control_param * pParam)
{
	pParam->open_threshold = G_ir_led_control_param.open_threshold;
	pParam->close_threshold = G_ir_led_control_param.close_threshold;
	pParam->open_delay = G_ir_led_control_param.open_delay;
	pParam->close_delay = G_ir_led_control_param.close_delay;
	pParam->threshold_1X = G_ir_led_control_param.threshold_1X;
	pParam->threshold_2X = G_ir_led_control_param.threshold_2X;
	pParam->threshold_3X = G_ir_led_control_param.threshold_3X;
	return 0;
}

s32 ir_led_set_param(mw_ir_led_control_param * pParam)
{
	if (G_ir_led_mode != MW_IR_LED_MODE_OFF) {
		MW_ERROR("Cannot set control parameters when IR led is on!\n");
		return -1;
	}
	G_ir_led_control_param.open_threshold = pParam->open_threshold;
	G_ir_led_control_param.close_threshold = pParam->close_threshold;
	G_ir_led_control_param.open_delay = pParam->open_delay;
	G_ir_led_control_param.close_delay = pParam->close_delay;
	G_ir_led_control_param.threshold_1X = pParam->threshold_1X;
	G_ir_led_control_param.threshold_2X = pParam->threshold_2X;
	G_ir_led_control_param.threshold_3X = pParam->threshold_3X;
	return 0;
}

s32 set_led_brightness(s32 brightness)
{
	if (brightness > IR_LED_PWM_DUTY_MAX || brightness < 0) {
		MW_ERROR("IR LED brightness ranges from 99 to 0!\n");
		G_ir_led_brightness = 0;
		return -1;
	}
	G_ir_led_brightness = brightness;
	return 0;
}

s32 ir_led_control(mw_ir_led_mode mode)
{
	static u32 i, i1, i2, i3, i4;
	static s32 valid_lsv = 0;
	static u32 light_sensor_value = 0;
	static s32 ir_led_brightness = 0;
	static s32 first_time = 0;
	if (first_time == 0) {
		G_ir_led_enable_state = ir_led_get_state();
		if (G_ir_led_enable_state != GPIO_LOW) {
			ir_led_set_state(GPIO_LOW);
			G_ir_led_enable_state = GPIO_LOW;
		}
		if (ir_cut_is_supported()) {
			G_ir_cut_ctrl_state = ir_cut_get_state();
			if (G_ir_cut_ctrl_state != IR_CUT_DAY) {
				ir_cut_set_state(IR_CUT_DAY);;
				G_ir_cut_ctrl_state = IR_CUT_DAY;
			}
		}
		G_ir_led_brightness = ir_led_get_brightness();
		if (G_ir_led_brightness != BRIGHTNESS_LEVEL_0) {
			ir_led_set_brightness(BRIGHTNESS_LEVEL_0);
			G_ir_led_brightness = BRIGHTNESS_LEVEL_0;
		}
		first_time = 1;
	}
	G_ir_led_mode = mode;
	switch (mode) {
		case MW_IR_LED_MODE_OFF:
			ir_led_set_brightness(BRIGHTNESS_LEVEL_0);
			if (G_ir_led_enable_state != GPIO_LOW) {
				ir_led_set_state(GPIO_LOW);
				G_ir_led_enable_state = GPIO_LOW;
			}
			if (ir_cut_is_supported()) {
				if (G_ir_cut_ctrl_state != IR_CUT_DAY) {
					ir_cut_set_state(IR_CUT_DAY);;
					G_ir_cut_ctrl_state = IR_CUT_DAY;
				}
			}
			if (G_day_night_mode != DISABLE_DAY_NIGHT_MODE) {
				mw_enable_day_night_mode(DISABLE_DAY_NIGHT_MODE);
				G_day_night_mode = DISABLE_DAY_NIGHT_MODE;
			}
			break;
		case MW_IR_LED_MODE_ON:
			G_ir_led_brightness = BRIGHTNESS_LEVEL_3X;
			if (G_ir_led_enable_state != GPIO_HIGH) {
				ir_led_set_state(GPIO_HIGH);
				G_ir_led_enable_state = GPIO_HIGH;
				if (G_ir_led_brightness != ir_led_get_brightness()) {
					ir_led_set_brightness(G_ir_led_brightness);
				}
			} else {
				if (G_ir_led_brightness != ir_led_get_brightness()) {
					ir_led_set_brightness(G_ir_led_brightness);
				}
			}
			if (ir_cut_is_supported()) {
				if (G_ir_cut_ctrl_state != IR_CUT_NIGHT) {
					ir_cut_set_state(IR_CUT_NIGHT);;
					G_ir_cut_ctrl_state = IR_CUT_NIGHT;
				}
			}
			if (G_day_night_mode != ENABLE_DAY_NIGHT_MODE) {
				mw_enable_day_night_mode(ENABLE_DAY_NIGHT_MODE);
				G_day_night_mode = ENABLE_DAY_NIGHT_MODE;
			}
			break;
		case MW_IR_LED_MODE_AUTO:
			if (ir_led_get_adc_value(&light_sensor_value) < 0) {
				MW_ERROR("Get light sensor value failed!\n");
				return -1;
			}
			MW_DEBUG("light_sensor_value %d\n", light_sensor_value);
			if (!G_ir_led_enable_state) {
				if (light_sensor_value <=
					G_ir_led_control_param.open_threshold) {
					i++;
					usleep(DELAY_MS);
				} else {
					i = 0;
				}
				if (i == G_ir_led_control_param.open_delay) {
					i = 0;
					if (light_sensor_value >
						G_ir_led_control_param.threshold_1X) {
						ir_led_brightness = BRIGHTNESS_LEVEL_1X;
					} else if (light_sensor_value >
						G_ir_led_control_param.threshold_2X) {
						ir_led_brightness = BRIGHTNESS_LEVEL_2X;
					} else if (light_sensor_value >
						G_ir_led_control_param.threshold_3X) {
						ir_led_brightness = BRIGHTNESS_LEVEL_3X;
					} else {
						ir_led_brightness = BRIGHTNESS_LEVEL_4X;
					}
					if (G_ir_led_brightness != ir_led_brightness) {
						ir_led_set_brightness(ir_led_brightness);
						G_ir_led_brightness = ir_led_brightness;
					}
					if (G_day_night_mode != ENABLE_DAY_NIGHT_MODE) {
						mw_enable_day_night_mode(ENABLE_DAY_NIGHT_MODE);
						G_day_night_mode = ENABLE_DAY_NIGHT_MODE;
					}
					if (G_ir_led_enable_state != GPIO_HIGH) {
						ir_led_set_state(GPIO_HIGH);
						G_ir_led_enable_state = GPIO_HIGH;
					}
					if (ir_cut_is_supported()) {
						if (G_ir_cut_ctrl_state != IR_CUT_NIGHT) {
							ir_cut_set_state(IR_CUT_NIGHT);;
							G_ir_cut_ctrl_state = IR_CUT_NIGHT;
						}
					}
				}
			} else {
				if (G_day_night_mode != ENABLE_DAY_NIGHT_MODE) {
					mw_enable_day_night_mode(ENABLE_DAY_NIGHT_MODE);
					G_day_night_mode = ENABLE_DAY_NIGHT_MODE;
				}
				if (ir_cut_is_supported()) {
					if (G_ir_cut_ctrl_state != IR_CUT_NIGHT) {
						ir_cut_set_state(IR_CUT_NIGHT);;
						G_ir_cut_ctrl_state = IR_CUT_NIGHT;
					}
				}
				if ((abs(light_sensor_value -
					G_ir_led_control_param.threshold_1X) < DELTA_LSV
					|| abs(light_sensor_value -
					G_ir_led_control_param.threshold_2X) < DELTA_LSV
					|| abs(light_sensor_value -
					G_ir_led_control_param.threshold_3X) < DELTA_LSV)
					&& abs(valid_lsv - light_sensor_value) < 2 * DELTA_LSV) {
					//light sensor value is invalid, do nothing
				} else {
					valid_lsv = light_sensor_value;
				}
				MW_DEBUG("Valid light_sensor_value %d\n", valid_lsv);
				//IR led brightness adjusts according to ambient light,
				//4 brightness level totally.
				if (valid_lsv > G_ir_led_control_param.threshold_1X) {
					i1++;
					i2 = 0;
					i3 = 0;
					i4 = 0;
					usleep(DELAY_MS);
				} else if (valid_lsv > G_ir_led_control_param.threshold_2X) {
					i2++;
					i1 = 0;
					i3 = 0;
					i4 = 0;
					usleep(DELAY_MS);
				} else if (valid_lsv > G_ir_led_control_param.threshold_3X) {
					i3++;
					i1 = 0;
					i2 = 0;
					i4 = 0;
					usleep(DELAY_MS);
				} else {
					i4++;
					i1 = 0;
					i2 = 0;
					i3 = 0;
					usleep(DELAY_MS);
				}
				if (i1 == DELAY) {
					ir_led_brightness = BRIGHTNESS_LEVEL_1X;
					i1 = 0;
				}
				if (i2 == DELAY) {
					ir_led_brightness = BRIGHTNESS_LEVEL_2X;
					i2 = 0;
				}
				if (i3 == DELAY) {
					ir_led_brightness = BRIGHTNESS_LEVEL_3X;
					i3 = 0;
				}
				if (i4 == DELAY) {
					ir_led_brightness = BRIGHTNESS_LEVEL_4X;
					i4 = 0;
				}
				MW_DEBUG("ir_led_brightness %d\n", ir_led_brightness);
				if (G_ir_led_brightness != ir_led_brightness) {
					ir_led_set_brightness(ir_led_brightness);
					G_ir_led_brightness = ir_led_brightness;
				}
				if (light_sensor_value >=
					G_ir_led_control_param.close_threshold) {
					i++;
					usleep(DELAY_MS);
				} else {
					i = 0;
				}
				if (i == G_ir_led_control_param.close_delay) {
					i = 0;
					ir_led_set_brightness(BRIGHTNESS_LEVEL_0);
					if (G_ir_led_enable_state != GPIO_LOW) {
						ir_led_set_state(GPIO_LOW);
						G_ir_led_enable_state = GPIO_LOW;
					}
					if (ir_cut_is_supported()) {
						if (G_ir_cut_ctrl_state != IR_CUT_DAY) {
							ir_cut_set_state(IR_CUT_DAY);;
							G_ir_cut_ctrl_state = IR_CUT_DAY;
						}
					}
					if (G_day_night_mode != DISABLE_DAY_NIGHT_MODE) {
						mw_enable_day_night_mode(DISABLE_DAY_NIGHT_MODE);
						G_day_night_mode = DISABLE_DAY_NIGHT_MODE;
					}
				}
			}
			break;
		default:
			if (G_ir_led_enable_state != GPIO_LOW) {
				ir_led_set_state(GPIO_LOW);
				G_ir_led_enable_state = GPIO_LOW;
			}
			if (ir_cut_is_supported()) {
				if (G_ir_cut_ctrl_state != IR_CUT_DAY) {
					ir_cut_set_state(IR_CUT_DAY);;
					G_ir_cut_ctrl_state = IR_CUT_DAY;
				}
			}
			if (G_day_night_mode != DISABLE_DAY_NIGHT_MODE) {
				mw_enable_day_night_mode(DISABLE_DAY_NIGHT_MODE);
				G_day_night_mode = DISABLE_DAY_NIGHT_MODE;
			}
			break;
	}
	return 0;
}
