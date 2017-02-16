/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx123/imx123_pri.h
 *
 * History:
 *    2014/08/05 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IMX123_PRI_H__
#define __IMX123_PRI_H__

#define IMX123_STANDBY    		0x3000
#define IMX123_REGHOLD			0x3001
#define IMX123_XMSTA			0x3002
#define IMX123_SWRESET    		0x3003

#define IMX123_WINMODE    		0x3007

#define IMX123_GAIN_LSB    		0x3014
#define IMX123_GAIN_MSB    		0x3015

#define IMX123_VMAX_LSB    		0x3018
#define IMX123_VMAX_MSB    		0x3019
#define IMX123_VMAX_HSB   		0x301A
#define IMX123_HMAX_LSB    		0x301B
#define IMX123_HMAX_MSB   		0x301C

#define IMX123_SHS1_LSB    		0x301E
#define IMX123_SHS1_MSB   		0x301F
#define IMX123_SHS1_HSB   		0x3020

#define IMX123_SHS2_LSB    		0x3021
#define IMX123_SHS2_MSB   		0x3022
#define IMX123_SHS2_HSB   		0x3023

#define IMX123_SHS3_LSB    		0x3024
#define IMX123_SHS3_MSB   		0x3025
#define IMX123_SHS3_HSB   		0x3026

#define IMX123_RHS1_LSB    		0x302E
#define IMX123_RHS1_MSB   		0x302F
#define IMX123_RHS1_HSB   		0x3030

#define IMX123_RHS2_LSB    		0x3031
#define IMX123_RHS2_MSB   		0x3032
#define IMX123_RHS2_HSB   		0x3033

#define IMX123_DCKRST			0x3044

#define IMX123_GAIN2_LSB    		0x30F2
#define IMX123_GAIN2_MSB    		0x30F3

#define IMX123_V_FLIP	(1<<0)
#define IMX123_H_MIRROR	(1<<1)

#define IMX123_DCKRST_BIT	(1<<6)

#define IMX123_QXGA_BRL		(1564)
#define IMX123_1080P_BRL		(1108)
#define IMX123_QXGA_H_PIXEL	(2048)
#define IMX123_1080P_H_PIXEL	(1936)
#define IMX123_H_PERIOD		(2250)
#define IMX123_QXGA_HBLANK	(192)

#define IMX123_QXGA_2X_RHS1		(0x142)

#define IMX123_QXGA_3X_RHS1		(0x26C)
#define IMX123_QXGA_3X_RHS2		(0x2C8)

#endif /* __IMX123_PRI_H__ */

