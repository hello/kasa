/*
 * Filename : ov5658_pri.h
 *
 * History:
 *    2014/05/28 - [Long Zhao] Create
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
#ifndef __OV5658_PRI_H__
#define __OV5658_PRI_H__

#define OV5658_STANDBY					0x0100
#define OV5658_SWRESET 					0x0103
#define OV5658_CHIP_ID_H				0x300A
#define OV5658_CHIP_ID_L				0x300B
#define OV5658_GRP_ACCESS				0x3208
#define OV5658_LONG_EXPO_H			0x3500
#define OV5658_LONG_EXPO_M			0x3501
#define OV5658_LONG_EXPO_L				0x3502
#define OV5658_AGC_ADJ_H				0x350A
#define OV5658_AGC_ADJ_L				0x350B
#define OV5658_HMAX_MSB				0x380C
#define OV5658_HMAX_LSB				0x380D
#define OV5658_VMAX_MSB				0x380E
#define OV5658_VMAX_LSB				0x380F
#define OV5658_MIPI_CTRL00				0x4800

#define OV5658_MIPI_GATE				(1<<5)

#define OV5658_V_FORMAT		0x3820
#define OV5658_H_FORMAT		0x3821
#define OV5658_V_FLIP			(0x03<<1)
#define OV5658_H_MIRROR		(0x03<<1)

#endif /* __OV5658_PRI_H__ */

