/**
 * @file system/include/flash/spi_nor/parts.h
 *
 * History:
 *    2013/10/15 - [Johnson Diao] created file
 *
 *
 * Copyright (c) 2015 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
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

/* ---COMMON SPI NAND FLASH COMMAND--- */
#define SPINAND_CMD_WRDI		0x04
#define SPINAND_CMD_WREN		0x06

/* Read to cache */
#define SPINAND_CMD_READ		0x13
/* Read from cache to memory */
#define SPINAND_CMD_RFC			0x03
/* Fast read from cache to memory */
#define SPINAND_CMD_FRFC		0x0B
#define SPINAND_CMD_ERASE_BLK	0xD8
/* Program to cache */
#define SPINAND_CMD_PRG_LOAD	0x02

/* Program to SPI NAND */
#define SPINAND_CMD_PRG_EXC		0x10
/* SPI NAND get features */
#define SPINAND_CMD_GET_REG		0x0F
/* SPI NAND set features */
#define SPINAND_CMD_SET_REG		0x1F
#define SPINAND_CMD_READID		0x9F
#define SPINAND_CMD_RESET		0xFF
/* ---END of SPI NAND CMD--- */

/* SPI NAND REG */
#define REG_PROTECTION			0xA0
#define REG_FEATURE				0xB0
#define REG_STATUS				0xC0
#define REG_FEATURE_2			0xD0

/* STATUS */
#define STATUS_OIP_MASK			0x01
#define STATUS_READY			(0 << 0)
#define STATUS_BUSY				(1 << 0)
#define STATUS_WEL_MASK			0x02
#define STATUS_WEL				(1 << 1)
#define STATUS_E_FAIL_MASK		0x04
#define STATUS_E_FAIL			(1 << 2)
#define STATUS_P_FAIL_MASK		0x08
#define STATUS_P_FAIL			(1 << 3)
#define STATUS_ECC				0x70

#define SPI_NAND_ECC_EN		0x10

#define CFI_MFR_MACRONIX	0x000000C2
#define CFI_MFR_MICRON		0x00000020
#define CFI_MFR_WINBOND		0x000000EF
#define CFI_MFR_GD			0x000000C8

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
#elif defined(CONFIG_SPI_NAND_GD5F2GQ4UC)
#include <flash/spinand/gd5f2gq4uc.h>
#elif defined(CONFIG_SPI_NAND_GD5F1GQ4UC)
#include <flash/spinand/gd5f1gq4uc.h>
#endif

#endif
