/*
 * Filename : ambds.h
 *
 * History:
 *    2015/01/30 - [Long Zhao] Create
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
#ifndef __AMBDS_PRI_H__
#define __AMBDS_PRI_H__

#define AMBDS_STANDBY    		0x3000
#define AMBDS_XMSTA			0x3008
#define AMBDS_SWRESET    		0x3009

#define AMBDS_WINMODE    		0x300F

#define AMBDS_GAIN_LSB    		0x301F
#define AMBDS_GAIN_MSB    		0x3020

#define AMBDS_VMAX_LSB    		0x302C
#define AMBDS_VMAX_MSB    		0x302D
#define AMBDS_VMAX_HSB   		0x302E
#define AMBDS_HMAX_LSB    		0x302F
#define AMBDS_HMAX_MSB   		0x3030

#define AMBDS_SHS1_LSB    		0x3034
#define AMBDS_SHS1_MSB   		0x3035
#define AMBDS_SHS1_HSB   		0x3036

#endif /* __AMBDS_PRI_H__ */

