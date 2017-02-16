/**********************************************************************
 * boards/s2lmironman/devices.h
 * History:
 * 2014/09/17 - [Bin Wang] Created this file
 *
 * Copyright (C) 2014-2018 Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *********************************************************************/

#ifndef __DEVICES_H__
#define __DEVICES_H__

#define SUPPORT_DC_IRIS         0
#define SUPPORT_IR_LED          1
#define SUPPORT_IR_CUT          1

/* DC IRIS */
#define PWM_CHANNEL_DC_IRIS     -1

/* IR LED */

#define PWM_CHANNEL_IR_LED      0
#define IR_LED_NO_GPIO_CONTROL

#define GPIO_ID_LED_POWER       -1
#define GPIO_ID_LED_IR_ENABLE   -1
#define GPIO_ID_LED_NETWORK     -1
#define GPIO_ID_LED_SD_CARD     -1

#define IR_LED_ADC_CHANNEL      2

/* IR CUT */
#define GPIO_ID_IR_CUT_CTRL     111

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
