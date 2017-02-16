/**
 * @file system/include/flash/spi_nor/parts.h
 *
 * History:
 *    2013/10/15 - [Johnson Diao] created file
 *
 * Copyright (C) 2013-2017, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __FLASH_SPI_NOR_PARTS_H__
#define __FLASH_SPI_NOR_PARTS_H__

/* chip select id, should be overwritten in bsp.h if necessary */
#define SPINOR_FLASH_CHIP_SEL		0

/* COMMON SPI NOR FLASH COMMAND */
#define SPINOR_CMD_WRSR			0x01
#define SPINOR_CMD_PP			0x02
#define SPINOR_CMD_READ			0x03
#define SPINOR_CMD_WRDI			0x04
#define SPINOR_CMD_RDSR			0x05
#define SPINOR_CMD_WREN			0x06
#define SPINOR_CMD_READID		0x9f
#define SPINOR_CMD_DOR			0x3b
#define SPINOR_CMD_DIOR			0xbb
#define SPINOR_CMD_DIOR_4B		0xbc
#define SPINOR_CMD_DDRDIOR		0xbd
#define SPINOR_CMD_SE			0xd8
#define SPINOR_CMD_SE_4K		0x20
#define SPINOR_CMD_CE			0xc7


#define CFI_MFR_MACRONIX	0x000000C2
#define CFI_MFR_MICRON		0x00000020
#define CFI_MFR_WINBOND		0x000000EF

/* The boot image header is a128-byte block and only first 24 bytes are used.
 * Each 4 bytes is corresponding to the layout (register definition) of
 * registers 0x0~0x14, respectively except for the data length field, clock
 * divder field, and those controlled by boot options. Please refer to document
 * for detailed information. */

struct spinor_boot_header {
	/* SPINOR_LENGTH_REG */
	u32 data_len : 16;
	u32 clk_divider : 6;
	u32 dummy_len : 5;
	u32 addr_len : 3;
	u32 cmd_len : 2;
	/* SPINOR_CTRL_REG */
	u32 read_en : 1;
	u32 write_en : 1;
	u32 rsvd0 : 7;
	u32 rxlane : 1;
	u32 data_lane : 2;
	u32 addr_lane : 2;
	u32 cmd_lane : 2;
	u32 rsvd1 : 12;
	u32 data_dtr : 1;
	u32 dummy_dtr : 1;
	u32 addr_dtr : 1;
	u32 cmd_dtr : 1;
	/* SPINOR_CFG_REG */
	u32 rxsampdly : 5;
	u32 rsvd2 : 13;
	u32 chip_sel : 8;
	u32 hold_timing : 1;
	u32 rsvd3 : 1;
	u32 hold_pin : 3;
	u32 flow_ctrl : 1;
	/* SPINOR_CMD_REG */
	u32 cmd;
	/* SPINOR_ADDRHI_REG */
	u32 addr_hi;
	/* SPINOR_ADDRLO_REG */
	u32 addr_lo;
	u32 rsvd[26];
} __attribute__((packed));


/******************************************************************************/
/* SPI NOR                                                                        */
/******************************************************************************/
#if defined(CONFIG_SPI_NOR_N25Q256A)
#include <flash/spinor/n25q256a.h>
#elif defined(CONFIG_SPI_NOR_FL01GS)
#include <flash/spinor/fl01gs.h>
#elif defined(CONFIG_SPI_NOR_MX25L25645G)
#include <flash/spinor/mx25l25645g.h>
#elif defined(CONFIG_SPI_NOR_W25Q64FV)
#include <flash/spinor/w25q64fv.h>
#elif defined(CONFIG_SPI_NOR_W25Q128FV)
#include <flash/spinor/w25q128fv.h>
#elif defined(CONFIG_SPI_NOR_GD25Q512)
#include <flash/spinor/gd25q512.h>
#endif

#endif
