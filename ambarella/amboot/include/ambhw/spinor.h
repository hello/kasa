/**
 * system/include/flash/spinor.h
 *
 * History:
 *    2014/05/05 - [Cao Rongrong] created file
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


#ifndef __SPINOR_H__
#define __SPINOR_H__

#include <ambhw/chip.h>

/* ==========================================================================*/

#if (CHIP_REV == S2E)
#define SPINOR_OFFSET			0xD000
#else
#define SPINOR_OFFSET			0x31000
#endif
#define SPINOR_BASE			(AHB_BASE + SPINOR_OFFSET)
#define SPINOR_REG(x)			(SPINOR_BASE + (x))

#define SPINOR_LENGTH_OFFSET		0x00
#define SPINOR_CTRL_OFFSET		0x04
#define SPINOR_CFG_OFFSET		0x08
#define SPINOR_CMD_OFFSET		0x0c
#define SPINOR_ADDRHI_OFFSET		0x10
#define SPINOR_ADDRLO_OFFSET		0x14
#define SPINOR_DMACTRL_OFFSET		0x18
#define SPINOR_TXFIFOTHLV_OFFSET	0x1c
#define SPINOR_RXFIFOTHLV_OFFSET	0x20
#define SPINOR_TXFIFOLV_OFFSET		0x24
#define SPINOR_RXFIFOLV_OFFSET		0x28
#define SPINOR_FIFOSTA_OFFSET		0x2c
#define SPINOR_INTRMASK_OFFSET		0x30
#define SPINOR_INTR_OFFSET		0x34
#define SPINOR_RAWINTR_OFFSET		0x38
#define SPINOR_CLRINTR_OFFSET		0x3c
#define SPINOR_TXFIFORST_OFFSET		0x40
#define SPINOR_RXFIFORST_OFFSET		0x44
#define SPINOR_START_OFFSET		0x50
#define SPINOR_TXDATA_OFFSET		0x100
#define SPINOR_RXDATA_OFFSET		0x200

#define SPINOR_LENGTH_REG		SPINOR_REG(0x00)
#define SPINOR_CTRL_REG			SPINOR_REG(0x04)
#define SPINOR_CFG_REG			SPINOR_REG(0x08)
#define SPINOR_CMD_REG			SPINOR_REG(0x0c)
#define SPINOR_ADDRHI_REG		SPINOR_REG(0x10)
#define SPINOR_ADDRLO_REG		SPINOR_REG(0x14)
#define SPINOR_DMACTRL_REG		SPINOR_REG(0x18)
#define SPINOR_TXFIFOTHLV_REG		SPINOR_REG(0x1c)
#define SPINOR_RXFIFOTHLV_REG		SPINOR_REG(0x20)
#define SPINOR_TXFIFOLV_REG		SPINOR_REG(0x24)
#define SPINOR_RXFIFOLV_REG		SPINOR_REG(0x28)
#define SPINOR_FIFOSTA_REG		SPINOR_REG(0x2c)
#define SPINOR_INTRMASK_REG		SPINOR_REG(0x30)
#define SPINOR_INTR_REG			SPINOR_REG(0x34)
#define SPINOR_RAWINTR_REG		SPINOR_REG(0x38)
#define SPINOR_CLRINTR_REG		SPINOR_REG(0x3c)
#define SPINOR_TXFIFORST_REG		SPINOR_REG(0x40)
#define SPINOR_RXFIFORST_REG		SPINOR_REG(0x44)
#define SPINOR_START_REG		SPINOR_REG(0x50)
#define SPINOR_TXDATA_REG		SPINOR_REG(0x100)
#define SPINOR_RXDATA_REG		SPINOR_REG(0x200)


/* SPINOR_LENGTH_REG */
#define SPINOR_LENGTH_CMD(x)		(((x) & 0x3) << 30)
#define SPINOR_LENGTH_ADDR(x)		(((x) & 0x7) << 27)
#define SPINOR_LENGTH_DUMMY(x)		(((x) & 0x1f) << 22)
#define SPINOR_LENGTH_DATA(x)		(((x) & 0x3fffff) << 0)

/* SPINOR_CTRL_REG */
#define SPINOR_CTRL_CMDDTR		0x80000000
#define SPINOR_CTRL_ADDRDTR		0x40000000
#define SPINOR_CTRL_DUMMYDTR		0x20000000
#define SPINOR_CTRL_DATADTR		0x10000000
#define SPINOR_CTRL_LSBFIRST		0x01000000
#define SPINOR_CTRL_CMD1LANE		0x00000000
#define SPINOR_CTRL_CMD2LANE		0x00004000
#define SPINOR_CTRL_CMD4LANE		0x00008000
#define SPINOR_CTRL_CMD8LANE		0x0000c000
#define SPINOR_CTRL_ADDR1LANE		0x00000000
#define SPINOR_CTRL_ADDR2LANE		0x00001000
#define SPINOR_CTRL_ADDR4LANE		0x00002000
#define SPINOR_CTRL_DATA1LANE		0x00000000
#define SPINOR_CTRL_DATA2LANE		0x00000400
#define SPINOR_CTRL_DATA4LANE		0x00000800
#define SPINOR_CTRL_DATA8LANE		0x00000c00
#define SPINOR_CTRL_RXLANE_TXRX		0x00000200
#define SPINOR_CTRL_RXLANE_TX		0x00000000
#define SPINOR_CTRL_WREN		0x00000002
#define SPINOR_CTRL_RDEN		0x00000001

/* SPINOR_CFG_REG */
#define SPINOR_CFG_FLOWCTRL_EN		0x80000000
#define SPINOR_CFG_HOLDPIN(x)		(((x) & 0x7) << 28)
#define SPINOR_CFG_CLKPOL_HI		0x08000000
#define SPINOR_CFG_HOLDSW_PHASE		0x04000000
#define SPINOR_CFG_CHIPSEL(x)		(((~(1 << (x))) & 0xff) << 18)
#define SPINOR_CFG_CHIPSEL_MASK		(0xff << 18)
#define SPINOR_CFG_CLKDIV(x)		(((x) & 0xff) << 10)
#define SPINOR_CFG_RXSAMPDLY(x)		((x) & 0x1f)

/* SPINOR_DMACTRL_REG */
#define SPINOR_DMACTRL_TXEN		0x00000002
#define SPINOR_DMACTRL_RXEN		0x00000001

/* SPINOR_FIFOSTA_REG */
#define SPINOR_FIFOSTA_RXFULL		0x00000010
#define SPINOR_FIFOSTA_RXNOTEMPTY	0x00000008
#define SPINOR_FIFOSTA_TXEMPTY		0x00000004
#define SPINOR_FIFOSTA_TXNOTFULL	0x00000002

/* SPINOR_INTRMASK_REG, SPINOR_INTR_REG, SPINOR_RAWINTR_REG, SPINOR_CLRINTR_REG */
#define SPINOR_INTR_TXUNDERFLOW		0x00000040
#define SPINOR_INTR_DATALENREACH	0x00000020
#define SPINOR_INTR_RXALMOSTFULL	0x00000010
#define SPINOR_INTR_RXOVERFLOW		0x00000008
#define SPINOR_INTR_RXUNDERFLOW		0x00000004
#define SPINOR_INTR_TXOVERFLOW		0x00000002
#define SPINOR_INTR_TXALMOSTEMPTY	0x00000001

#define SPINOR_INTR_ALL			SPINOR_INTR_TXUNDERFLOW | \
					SPINOR_INTR_DATALENREACH | \
					SPINOR_INTR_RXALMOSTFULL | \
					SPINOR_INTR_RXOVERFLOW | \
					SPINOR_INTR_RXUNDERFLOW | \
					SPINOR_INTR_TXOVERFLOW | \
					SPINOR_INTR_TXALMOSTEMPTY

#define SPINOR_MAX_DATA_LENGTH		0x3ffff0
#define SPINOR_OPERATION_TIMEOUT	5000

#ifndef __ASM__
/* ==========================================================================*/

#define MINIPIN_SPINOR_ALTFUNC		3
#define MINIPIN_SPINOR_PIN		{55, 56, 61, 62, 63, 64, 65, 66, \
					 67, 68, 69, 70, 71, 72}
#if defined(CONFIG_AMBOOT_ENABLE_SPINOR)
extern int spinor_init(void);
extern int spinor_send_alone_cmd(u8 cmd_id);
extern int spinor_read_reg(u8 cmd_id, void *buf, u8 len);
extern int spinor_write_reg(u8 cmd_id, void *buf, u8 len);
extern int spinor_erase_sector(u32 sector);
extern int (*spinor_read_data)(u32 address, void *buf, int len);
extern int spinor_write_data(u32 address, void *buf, int len);
extern int spinor_write_boot_header(void);


/* customised function implemented by each spi nor flash */
extern int spinor_flash_reset(void);
extern int spinor_flash_4b_mode(void);
extern int spinor_erase_chip(void);
#elif defined(CONFIG_AMBOOT_ENABLE_SPINAND)
extern int spinand_wait_till_ready();
extern int spinand_init(void);
extern int spinor_send_alone_cmd(u8 cmd_id);
extern int spinor_read_reg(u8 cmd_id, void *buf, u8 len);
extern int spinor_write_reg(u8 cmd_id, void *buf, u8 len);
extern int spinand_erase_block(u32 block_id);
/* customised function implemented by each spi nor flash */
extern int spinand_flash_reset(void);
extern int spinand_read_data(u8 *dst, u8 *src, int len);
extern int spinand_read_page(u32 page, u32 offset, void *buf, int len, u32 enable_ecc);
extern int spinand_program_page(u32 page, u16 offset, void *buf, int len);
extern int spinand_is_bad_block(u32 block);
extern void spinand_output_bad_block(u32 block, int bb_type);
extern int spinand_mark_bad_block(u32 block);
extern int spinand_prog_pages(u32 block, u32 page, u32 pages,
		void *wbuf);
extern int spinand_read_pages(u32 block, u32 page, u32 pages,
		void *rbuf, u32 enable_ecc);

#if defined(CONFIG_SPINAND_USE_FLASH_BBT)
extern int spinand_scan_bbt(int verbose);
extern int spinand_update_bbt(u32 bblock, u32 gblock);
extern int spinand_erase_bbt(void);
extern int spinand_isbad_bbt(u32 block);
extern int spinand_show_bbt(void);
extern int spinand_has_bbt(void);
#endif

/* ==========================================================================*/
#endif
/* ==========================================================================*/

#endif

#endif
