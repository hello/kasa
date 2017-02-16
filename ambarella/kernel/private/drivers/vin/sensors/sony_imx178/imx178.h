/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx178/imx178_pri.h
 *
 * History:
 *    2012/02/21 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IMX178_PRI_H__
#define __IMX178_PRI_H__

#ifdef CONFIG_SENSOR_IMX178_SPI
/************************ for SPI *****************************/
#define IMX178_STANDBY    		0x0200
#define IMX178_XMSTA			0x0208
#define IMX178_SWRESET    		0x0209

#define IMX178_WINMODE    		0x020F

#define IMX178_LPMODE    			0x021B

#define IMX178_GAIN_LSB    		0x021F
#define IMX178_GAIN_MSB    		0x0220

#define IMX178_VMAX_LSB    		0x022C
#define IMX178_VMAX_MSB    		0x022D
#define IMX178_VMAX_HSB   		0x022E
#define IMX178_HMAX_LSB    		0x022F
#define IMX178_HMAX_MSB   		0x0230

#define IMX178_SHS1_LSB    		0x0234
#define IMX178_SHS1_MSB   		0x0235
#define IMX178_SHS1_HSB   		0x0236
#else
/************************ for I2C *****************************/
#define IMX178_STANDBY    		0x3000
#define IMX178_XMSTA			0x3008
#define IMX178_SWRESET    		0x3009

#define IMX178_WINMODE    		0x300F

#define IMX178_LPMODE    			0x301B

#define IMX178_GAIN_LSB    		0x301F
#define IMX178_GAIN_MSB    		0x3020

#define IMX178_VMAX_LSB    		0x302C
#define IMX178_VMAX_MSB    		0x302D
#define IMX178_VMAX_HSB   		0x302E
#define IMX178_HMAX_LSB    		0x302F
#define IMX178_HMAX_MSB   		0x3030

#define IMX178_SHS1_LSB    		0x3034
#define IMX178_SHS1_MSB   		0x3035
#define IMX178_SHS1_HSB   		0x3036
#endif

/*************************************************************/
#define IMX178_V_FLIP		(1<<0)
#define IMX178_H_MIRROR	(1<<1)

#endif /* __IMX178_PRI_H__ */

