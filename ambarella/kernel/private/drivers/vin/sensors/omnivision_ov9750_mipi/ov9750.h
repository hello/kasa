/*
 * Filename : ov9750_pri.h
 *
 * History:
 *    2014/12/04 - [Hao Zeng] Create
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 * This file is produced by perl.
 */
#ifndef __OV9750_PRI_H__
#define __OV9750_PRI_H__

#define OV9750_STANDBY			0x0100
#define OV9750_SWRESET			0x0103

#define OV9750_GRP_ACCESS		0x3208

#define OV9750_GAIN_MSB			0x3508
#define OV9750_GAIN_LSB			0x3509

#define OV9750_HMAX_MSB			0x380C
#define OV9750_HMAX_LSB			0x380D
#define OV9750_VMAX_MSB			0x380E
#define OV9750_VMAX_LSB			0x380F

#define OV9750_EXPO0_HSB			0x3500
#define OV9750_EXPO0_MSB			0x3501
#define OV9750_EXPO0_LSB			0x3502

#define OV9750_GAIN_DCG			0x37C7

#define OV9750_V_FORMAT			0x3820
#define OV9750_H_FORMAT			0x3821

#define OV9750_BLC_CTRL10		0x4010

#define OV9750_MIPI_CTRL00		0x4800

#define OV9750_R_GAIN_MSB		0x5032
#define OV9750_R_GAIN_LSB		0x5033
#define OV9750_G_GAIN_MSB		0x5034
#define OV9750_G_GAIN_LSB		0x5035
#define OV9750_B_GAIN_MSB		0x5036
#define OV9750_B_GAIN_LSB		0x5037

#define OV9750_MIPI_GATE			(1<<5)

#define OV9750_V_FLIP				(3<<1)
#define OV9750_H_MIRROR			(3<<1)

#endif /* __OV9750_PRI_H__ */

