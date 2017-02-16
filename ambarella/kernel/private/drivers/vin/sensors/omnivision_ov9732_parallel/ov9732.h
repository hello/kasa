/*
 * Filename : ov9732.h
 *
 * History:
 *    2015/07/28 - [Hao Zeng] Create
 *
 * Copyright (C) 2004-2015, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 * This file is produced by perl.
 */
#ifndef __OV9732_PRI_H__
#define __OV9732_PRI_H__

#define OV9732_STANDBY			0x0100
#define OV9732_SWRESET			0x0103

#define OV9732_CHIP_ID_H			0x300A
#define OV9732_CHIP_ID_L			0x300B
#define OV9732_CHIP_REV			0x302A

#define OV9732_GRP_ACCESS		0x3208

#define OV9732_L_EXPO_HSB		0x3500
#define OV9732_L_EXPO_MSB		0x3501
#define OV9732_L_EXPO_LSB		0x3502

#define OV9732_GAIN_MSB			0x350A
#define OV9732_GAIN_LSB			0x350B

#define OV9732_HTS_MSB			0x380C
#define OV9732_HTS_LSB			0x380D
#define OV9732_VTS_MSB			0x380E
#define OV9732_VTS_LSB			0x380F

#define OV9732_FORMAT0			0x3820

#define OV9732_R_GAIN_MSB		0x5180
#define OV9732_R_GAIN_LSB		0x5181
#define OV9732_G_GAIN_MSB		0x5182
#define OV9732_G_GAIN_LSB		0x5183
#define OV9732_B_GAIN_MSB		0x5184
#define OV9732_B_GAIN_LSB		0x5185

#define OV9732_V_FLIP				(1<<2)
#define OV9732_H_MIRROR			(1<<3)
#define OV9732_MIRROR_MASK		(OV9732_H_MIRROR + OV9732_V_FLIP)

#endif /* __OV9732_PRI_H__ */

