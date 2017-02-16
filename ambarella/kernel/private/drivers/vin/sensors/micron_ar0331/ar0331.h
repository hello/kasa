/*
 * Filename : ar0331_pri.h
 *
 * History:
 *    2015/02/06 - [Hao Zeng] Create
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
#ifndef __AR0331_PRI_H__
#define __AR0331_PRI_H__

#define AR0331_RESET_REGISTER				0x301A
#define AR0331_LINE_LENGTH_PCK				0x300C
#define AR0331_FRAME_LENGTH_LINES			0x300A

#define AR0331_CHIP_VERSION_REG			0x3000
#define AR0331_COARSE_INTEGRATION_TIME		0x3012

#define AR0331_VT_PIX_CLK_DIV				0x302A
#define AR0331_VT_SYS_CLK_DIV				0x302C
#define AR0331_PRE_PLL_CLK_DIV				0x302E
#define AR0331_PLL_MULTIPLIER				0x3030

#define AR0331_DGAIN						0x305E
#define AR0331_AGAIN						0x3060
#define AR0331_DCG_CTL						0x3100

/* AR0331 mirror mode */
#define AR0331_READ_MODE				0x3040
#define AR0331_H_MIRROR			(0x01 << 14)
#define AR0331_V_FLIP				(0x01 << 15)
#define AR0331_MIRROR_MASK			(AR0331_H_MIRROR + AR0331_V_FLIP)

#endif /* __AR0331_PRI_H__ */

