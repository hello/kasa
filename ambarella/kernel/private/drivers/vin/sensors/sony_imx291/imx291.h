/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx291/imx291_pri.h
 *
 * History:
 *    2014/12/05 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IMX291_PRI_H__
#define __IMX291_PRI_H__

#define IMX291_STANDBY    		0x3000
#define IMX291_REGHOLD			0x3001
#define IMX291_XMSTA			0x3002
#define IMX291_SWRESET    		0x3003

#define IMX291_ADBIT    			0x3005
#define IMX291_WINMODE    		0x3007
#define IMX291_FRSEL			0x3009
#define IMX291_BLKLEVEL_LSB	0x300A
#define IMX291_BLKLEVEL_MSB	0x300B

#define IMX291_GAIN		    		0x3014

#define IMX291_VMAX_LSB    		0x3018
#define IMX291_VMAX_MSB    		0x3019
#define IMX291_VMAX_HSB   		0x301A

#define IMX291_HMAX_LSB    		0x301C
#define IMX291_HMAX_MSB   		0x301D

#define IMX291_SHS1_LSB    		0x3020
#define IMX291_SHS1_MSB   		0x3021
#define IMX291_SHS1_HSB   		0x3022

#define IMX291_ODBIT			0x3046

#define IMX291_INCKSEL1			0x305C
#define IMX291_INCKSEL2			0x305D
#define IMX291_INCKSEL3			0x305E
#define IMX291_INCKSEL4			0x305F
#define IMX291_INCKSEL5			0x315E
#define IMX291_INCKSEL6			0x3164

#define IMX291_V_FLIP	(1<<0)
#define IMX291_H_MIRROR	(1<<1)

#define IMX291_HI_GAIN_MODE	(1<<4)

#endif /* __IMX291_PRI_H__ */

