/*
 * Filename : ov4689_pri.h
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
#ifndef __OV4689_PRI_H__
#define __OV4689_PRI_H__

#define OV4689_STANDBY			0x0100
#define OV4689_IMG_ORI			0x0101
#define OV4689_SWRESET			0x0103

#define OV4689_GRP_ACCESS		0x3208

#define OV4689_HTS_MSB			0x380C
#define OV4689_HTS_LSB			0x380D
#define OV4689_VTS_MSB			0x380E
#define OV4689_VTS_LSB			0x380F

#define OV4689_X_START			0x0345
#define OV4689_Y_START			0x0347

#define OV4689_L_EXPO_HSB	0x3500
#define OV4689_L_EXPO_MSB	0x3501
#define OV4689_L_EXPO_LSB	0x3502

#define OV4689_L_GAIN_HSB	0x3507
#define OV4689_L_GAIN_MSB	0x3508
#define OV4689_L_GAIN_LSB	0x3509

#define OV4689_M_EXPO_HSB		0x350A
#define OV4689_M_EXPO_MSB		0x350B
#define OV4689_M_EXPO_LSB		0x350C

#define OV4689_M_GAIN_HSB		0x350D
#define OV4689_M_GAIN_MSB		0x350E
#define OV4689_M_GAIN_LSB		0x350F

#define OV4689_S_EXPO_HSB		0x3510
#define OV4689_S_EXPO_MSB		0x3511
#define OV4689_S_EXPO_LSB		0x3512

#define OV4689_S_GAIN_HSB		0x3513
#define OV4689_S_GAIN_MSB		0x3514
#define OV4689_S_GAIN_LSB		0x3515

#define OV4689_M_MAX_EXPO_MSB	0x3760
#define OV4689_M_MAX_EXPO_LSB		0x3761

#define OV4689_S_MAX_EXPO_MSB	0x3762
#define OV4689_S_MAX_EXPO_LSB		0x3763

#define OV4689_L_WB_R_GAIN_MSB	0x500C
#define OV4689_L_WB_R_GAIN_LSB	0x500D

#define OV4689_L_WB_G_GAIN_MSB	0x500E
#define OV4689_L_WB_G_GAIN_LSB	0x500F

#define OV4689_L_WB_B_GAIN_MSB	0x5010
#define OV4689_L_WB_B_GAIN_LSB	0x5011

#define OV4689_M_WB_R_GAIN_MSB	0x5012
#define OV4689_M_WB_R_GAIN_LSB	0x5013

#define OV4689_M_WB_G_GAIN_MSB	0x5014
#define OV4689_M_WB_G_GAIN_LSB	0x5015

#define OV4689_M_WB_B_GAIN_MSB	0x5016
#define OV4689_M_WB_B_GAIN_LSB	0x5017

#define OV4689_S_WB_R_GAIN_MSB	0x5018
#define OV4689_S_WB_R_GAIN_LSB	0x5019

#define OV4689_S_WB_G_GAIN_MSB	0x501A
#define OV4689_S_WB_G_GAIN_LSB	0x501B

#define OV4689_S_WB_B_GAIN_MSB	0x501C
#define OV4689_S_WB_B_GAIN_LSB	0x501D

#define OV4689_MIPI_CTRL00		0x4800

#define OV4689_MIPI_GATE		(1<<5)


#define OV4689_V_FORMAT		0x3820
#define OV4689_H_FORMAT		0x3821

#define OV4689_V_FLIP			(3<<1)
#define OV4689_H_MIRROR		(3<<1)

#define OV4689_TPM_TRIGGER		0x4D12
#define OV4689_TPM_READ		0x4D13
#define OV4689_TPM_OFFSET		64

#endif /* __OV4689_PRI_H__ */

