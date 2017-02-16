/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx290/imx290_pri.h
 *
 * History:
 *    2015/03/23 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2015, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IMX290_PRI_H__
#define __IMX290_PRI_H__

#define USE_1080P_2X_30FPS

#define IMX290_STANDBY    		0x3000
#define IMX290_REGHOLD			0x3001
#define IMX290_XMSTA			0x3002
#define IMX290_SWRESET    		0x3003

#define IMX290_ADBIT0    			0x3005
#define IMX290_ADBIT1    			0x3129
#define IMX290_ADBIT2    			0x317C
#define IMX290_ADBIT3    			0x31EC
#define IMX290_WINMODE    		0x3007
#define IMX290_FRSEL			0x3009
#define IMX290_BLKLEVEL_LSB	0x300A
#define IMX290_BLKLEVEL_MSB	0x300B
#define IMX290_WDMODE			0x300C

#define IMX290_GAIN			0x3014

#define IMX290_VMAX_LSB			0x3018
#define IMX290_VMAX_MSB			0x3019
#define IMX290_VMAX_HSB			0x301A

#define IMX290_HMAX_LSB			0x301C
#define IMX290_HMAX_MSB			0x301D

#define IMX290_SHS1_LSB			0x3020
#define IMX290_SHS1_MSB			0x3021
#define IMX290_SHS1_HSB			0x3022
#define IMX290_SHS2_LSB			0x3024
#define IMX290_SHS2_MSB			0x3025
#define IMX290_SHS2_HSB			0x3026
#define IMX290_SHS3_LSB			0x3028
#define IMX290_SHS3_MSB			0x3029
#define IMX290_SHS3_HSB			0x302A

#define IMX290_RHS1_LSB			0x3030
#define IMX290_RHS1_MSB			0x3031
#define IMX290_RHS1_HSB			0x3032
#define IMX290_RHS2_LSB			0x3034
#define IMX290_RHS2_MSB			0x3035
#define IMX290_RHS2_HSB			0x3036

#define IMX290_PATTERN			0x3045
#define IMX290_ODBIT			0x3046

#define IMX290_INCKSEL1			0x305C
#define IMX290_INCKSEL2			0x305D
#define IMX290_INCKSEL3			0x305E
#define IMX290_INCKSEL4			0x305F
#define IMX290_INCKSEL5			0x315E
#define IMX290_INCKSEL6			0x3164

#define IMX290_XVSCNT_INT		0x3106

#define IMX290_HBLANK_LSB		0x31A0
#define IMX290_HBLANK_MSB		0x31A1

#define IMX290_NULL0SIZE		0x3415
#define IMX290_YOUTSIZE_LSB	0x3418
#define IMX290_YOUTSIZE_MSB	0x3419

#define IMX290_V_FLIP	(1<<0)
#define IMX290_H_MIRROR	(1<<1)

#define IMX290_HI_GAIN_MODE	(1<<4)

#define IMX290_1080P_BRL		(1109)
#define IMX290_720P_BRL		(735)
#define IMX290_1080P_H_PIXEL	(1945)
#define IMX290_1080P_HBLANK	(695)
#define IMX290_1080P_H_PERIOD	(2640)
#define IMX290_1080P_12B_H_PERIOD	(2200)

#ifdef USE_1080P_2X_30FPS
#define IMX290_1080P_2X_RHS1		(0x1B3)/* for 1080p30 2x, the max value of RHS1 is 2259 */
#else
#define IMX290_1080P_2X_RHS1		(11)/* for 1080p60 2x, the max value of RHS1 is 11 */
#endif

#define IMX290_1080P_2X_12B_RHS1	(11)

#define IMX290_1080P_3X_RHS1		(0x208)
#define IMX290_1080P_3X_RHS2		(0x25A)/* for 1080p30 3x, the max value of RHS2 is 1148 */

#endif /* __IMX290_PRI_H__ */

