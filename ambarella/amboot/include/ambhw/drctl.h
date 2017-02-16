/*
 * ambhw/drctl.h
 *
 * History:
 *	2007/01/27 - [Charles Chiou] created file
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AMBHW__DRCTRL_H__
#define __AMBHW__DRCTRL_H__

/* ==========================================================================*/
#if (CHIP_REV == A5S) || (CHIP_REV == S2)
#define DRAM_PHYS_BASE			0x60000000
#elif (CHIP_REV == A7L)
#define DRAM_PHYS_BASE			0xFFFE0000
#else
#define DRAM_PHYS_BASE			0xDFFE0000
#endif

#if (CHIP_REV == A5S) || (CHIP_REV == S2)
#define DRAM_DRAM_OFFSET 		0x4000
#define DRAM_DDRC_OFFSET  		DRAM_DRAM_OFFSET
#else
#define DRAM_DRAM_OFFSET 		0x0000
#define DRAM_DDRC_OFFSET  		0x0800
#endif

#define DDRC_REG(x)			(DRAM_PHYS_BASE + DRAM_DDRC_OFFSET + (x))
#define DRAM_REG(x)			(DRAM_PHYS_BASE + DRAM_DRAM_OFFSET + (x))

/* ==========================================================================*/
#define DDRC_CTL_OFFSET			(0x00)
#define DDRC_CFG_OFFSET			(0x04)
#define DDRC_TIMING1_OFFSET		(0x08)
#define DDRC_TIMING2_OFFSET		(0x0C)
#define DDRC_TIMING3_OFFSET		(0x10)
#define DDRC_INIT_CTL_OFFSET		(0x14)
#define DDRC_MODE_OFFSET		(0x18)
#define DDRC_SELF_REF_OFFSET		(0x1C)
#define DDRC_DQS_SYNC_OFFSET		(0x20)
#define DDRC_PAD_ZCTL_OFFSET		(0x24)
#define DDRC_ZQ_CALIB_OFFSET		(0x28)
#define DDRC_RSVD_SPACE_OFFSET		(0x2C)
#define DDRC_BYTE_MAP_OFFSET		(0x30)
#define DDRC_DDR3PLUS_BNKGRP_OFFSET	(0x34)
#define DDRC_POWER_DOWN_OFFSET		(0x38)
#define DDRC_DLL_CALIB_OFFSET		(0x3C)
#define DDRC_DEBUG1_OFFSET		(0x40)

/* ==========================================================================*/
#endif /* __AMBHW__DRCTRL_H__ */

