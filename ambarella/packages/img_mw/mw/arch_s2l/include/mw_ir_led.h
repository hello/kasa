/**********************************************************************
 * mw_ir_led.h
 *
 * History:
 * 2014/09/17 - [Bin Wang] Created this file
 *
 * Copyright (C) 2014-2018, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *********************************************************************/

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
