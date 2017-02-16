/*
 * Filename : ar0141_pri.h
 *
 * History:
 *    2014/11/18 - [Long Zhao] Create
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
#ifndef __AR0141_PRI_H__
#define __AR0141_PRI_H__

#define AR0141_RESET_REGISTER				0x301A
#define AR0141_LINE_LENGTH_PCK			0x300C
#define AR0141_FRAME_LENGTH_LINES			0x300A

#define AR0141_CHIP_VERSION_REG			0x3000
#define AR0141_COARSE_INTEGRATION_TIME	0x3012

#define AR0141_VT_PIX_CLK_DIV				0x302A
#define AR0141_VT_SYS_CLK_DIV				0x302C
#define AR0141_PRE_PLL_CLK_DIV				0x302E
#define AR0141_PLL_MULTIPLIER				0x3030

#define AR0141_DGAIN						0x305E
#define AR0141_AGAIN						0x3060
#define AR0141_DCG_CTL						0x3100

/* AR0141 mirror mode */
#define AR0141_READ_MODE				0x3040
#define AR0141_H_MIRROR			(0x01 << 14)
#define AR0141_V_FLIP				(0x01 << 15)
#define AR0141_MIRROR_MASK			(AR0141_H_MIRROR + AR0141_V_FLIP)

#endif /* __AR0141_PRI_H__ */

