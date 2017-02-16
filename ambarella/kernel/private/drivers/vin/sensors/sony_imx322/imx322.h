/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx322/imx322_pri.h
 *
 * History:
 *    2014/08/15 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IMX322_PRI_H__
#define __IMX322_PRI_H__
		/* for SPI */
#ifdef CONFIG_SENSOR_IMX322_SPI
#define IMX322_STANDBY		0x0200
#define IMX322_VREVERSE		0x0201

#define IMX322_REG_GAIN		0x021E

#define IMX322_XMSTA			0x022C

#define IMX322_HMAX_LSB		0x0203
#define IMX322_HMAX_MSB		0x0204
#define IMX322_VMAX_LSB		0x0205
#define IMX322_VMAX_MSB		0x0206

#define IMX322_SHS1_LSB		0x0208
#define IMX322_SHS1_MSB		0x0209

#define IMX322_STREAMING		0x00
#define IMX322_V_FLIP			(1<<0)
#else	/* for I2C */
#define IMX322_STANDBY		0x0100
#define IMX322_VREVERSE		0x0101

#define IMX322_REG_GAIN		0x301E

#define IMX322_XMSTA			0x302C

#define IMX322_VMAX_MSB		0x0340
#define IMX322_VMAX_LSB		0x0341
#define IMX322_HMAX_MSB		0x0342
#define IMX322_HMAX_LSB		0x0343

#define IMX322_SHS1_MSB		0x0202
#define IMX322_SHS1_LSB		0x0203

#define IMX322_STREAMING		0x01
#define IMX322_V_FLIP			(1<<1)
#endif

#endif /* __IMX322_PRI_H__ */

