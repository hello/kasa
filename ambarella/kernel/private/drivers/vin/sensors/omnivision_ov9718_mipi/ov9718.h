/*
 * Filename : ov9718_pri.h
 *
 * History:
 *    2012/03/23 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 * This file is produced by perl.
 */
#ifndef __OV9718_PRI_H__
#define __OV9718_PRI_H__

#define OV9718_STANDBY    		0x0100
#define OV9718_IMG_ORI			0x0101
#define OV9718_SWRESET    		0x0103

#define OV9718_GRP_ACCESS  		0x3208

#define OV9718_GAIN_MSB    		0x350A
#define OV9718_GAIN_LSB    		0x350B

#define OV9718_VMAX_MSB    		0x0340
#define OV9718_VMAX_LSB    		0x0341
#define OV9718_HMAX_MSB   		0x0342
#define OV9718_HMAX_LSB    		0x0343

#define OV9718_X_START			0x0345
#define OV9718_Y_START			0x0347

#define OV9718_EXPO0_HSB   		0x3500
#define OV9718_EXPO0_MSB   		0x3501
#define OV9718_EXPO0_LSB    		0x3502

#define OV9718_MIPI_CTRL00		0x4800

#define OV9718_MIPI_GATE		(1<<5)

#define OV9718_V_FLIP				(1<<1)
#define OV9718_H_MIRROR			(1<<0)
#define OV9718_MIRROR_MASK		(OV9718_H_MIRROR + OV9718_V_FLIP)

#endif /* __OV9718_PRI_H__ */

