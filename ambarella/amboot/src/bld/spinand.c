/**
 * src/bld/spinand.c
 *
 * SPI NOR-Flash controller functions with spi nand chips.
 *
 * History:
 *    2015/10/26 - [Ken He] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
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

#include <bldfunc.h>
#include <fio/firmfl.h>
#include <flash/spinor/spinor_flash.h>
#include <ambhw/dma.h>
#include <ambhw/cache.h>
#include <ambhw/rct.h>
#include <ambhw/spinor.h>
#include <fio/ftl_const.h>

struct spinor_ctrl {
	u32 len;
	u8 *buf;
	u8 lane;
	u8 is_dtr : 1;
	u8 is_read : 1; /* following are only avaiable for data */
	u8 is_io : 1;
	u8 is_dma : 1;
};

#define PAGE_CACHE_SIZE		(2048 + 64)
#define SPARE_SIZE_PAGE		128
#define PAGES_PER_BLCOK		64
#define NAND_SPARE_128_SHF	7	/* spare size 128B */
#define SPINAND_SPARE_USE_SIZE	64

static u8 spinand_cache_buf[PAGE_CACHE_SIZE] __attribute__ ((aligned(32)));

#if defined(CONFIG_SPINAND_USE_FLASH_BBT)
/*
 * The bad block table is located in the last good blocks of the device, and
 * is marked in the OOB area with an ident pattern.
 *
 * The table is mirrored, so it can be updated eventually. Both tables has a
 * version number which indicates which of both tables is more up to date.
 *
 * The bad block table uses 2 bits per block
 * 11b:		block is good
 * 00b:		block is factory marked bad
 * 01b, 10b:	block is marked bad due to wear
 */

#define MAIN_SPARE_SIZE		(2048 + 128)
#define NAND_BBT_SIZE		(4096 >> 2)

static int bbt_is_created = 0;

static u8 bbt[NAND_BBT_SIZE]
__attribute__ ((aligned(32), section(".bss.noinit")));
static u8 page_data[MAIN_SPARE_SIZE]
__attribute__ ((aligned(32), section(".bss.noinit")));
static u8 check_buf[MAIN_SPARE_SIZE]
__attribute__ ((aligned(32), section(".bss.noinit")));

static u8 bbt_pattern[] = { 'B', 'b', 't', '0' };
static u8 mirror_pattern[] = { '1', 't', 'b', 'B' };

static nand_bbt_descr_t bbt_main_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_VERSION | NAND_BBT_NO_OOB,
	.offs = 0,
	.len = 4,
	.veroffs = 4,
	.maxblocks = 4,
	.pattern = bbt_pattern,
	.block = -1,
};

static nand_bbt_descr_t bbt_mirror_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_VERSION | NAND_BBT_NO_OOB,
	.offs = 0,
	.len = 4,
	.veroffs = 4,
	.maxblocks = 4,
	.pattern = mirror_pattern,
	.block = -1,
};

static u32 add_marker_len(nand_bbt_descr_t *td)
{
	u32 len;
	if (!(td->options & NAND_BBT_NO_OOB))
		return 0;

	len = td->len;
	if (td->options & NAND_BBT_VERSION)
		len++;
	return len;
}

/**
 * read_bbt - [GENERIC] Read the bad block table starting from page
 *
 * Read the bad block table starting from page.
 */
static int read_bbt(nand_bbt_descr_t *td)
{
	int rval, i;
	u32 offs;
	u32 total_blks;
	total_blks = flspinand.chip_size / flspinand.block_size;

	rval = spinand_read_pages(td->block, 0, 1, page_data, 1);
	if (rval < 0) {
		spinand_mark_bad_block(td->block);
		putstr("read failed. <block ");
		putdec(td->block);
		putstr(", page 0 ");
		putstr(">\r\n");
		return -1;
	}

	offs = add_marker_len(td);
	for (i = 0; i < (total_blks >> 2); i++)
		bbt[i] &= page_data[offs + i];

	return 0;
}
/**
 * check_pattern_no_oob - [GENERIC] check if a pattern is in the buffer
 *
 * Check for a pattern at the given place. Used to search bad block
 * tables and good / bad block identifiers.
*/
static int check_pattern_no_oob(u8 *buf_data, nand_bbt_descr_t *td)
{
	int ret;
	ret = memcmp(buf_data, td->pattern, td->len);
	if (!ret)
		return ret;

	return -1;
}

/**
 * search_bbt - [GENERIC] scan the device for a specific bad block table
 *
 * Read the bad block table by searching for a given ident pattern.
 * Search is preformed either from the beginning up or from the end of
 * the device downwards. The search starts always at the start of a
 * block.
 *
 * The bbt ident pattern resides in the oob area of the first page
 * in a block.
 */
static int search_bbt(nand_bbt_descr_t *td, int verbose)
{
	int actblock, startblock, block, rval;

	u32 total_blks;
	total_blks = flspinand.chip_size / flspinand.block_size;

	/* Reset version information */
	td->version = 0;
	td->block = -1;

	/* Search direction backward */
	startblock = total_blks - 1;

	/* Scan the maximum number of blocks */
	for (block = 0; block < td->maxblocks; block++) {
		actblock = startblock - block;
		if (spinand_is_bad_block(actblock)) {
			startblock--;
			block--;
			continue;
		}

		/* Read first page */
		rval = spinand_read_pages(actblock, 0, 1, page_data, 1);
		if (rval < 0) {
			spinand_mark_bad_block(actblock);
			putstr("read failed. <block ");
			putdec(actblock);
			putstr(", page 1");
			putstr(">\r\n");
			break;
		}

		if (!check_pattern_no_oob(page_data, td)) {
			td->block = actblock;
			if (td->options & NAND_BBT_VERSION) {
				td->version = page_data[td->veroffs];
			}
			break;
		}

	}

	/* Check, if we found a bbt for each requested chip */
	if (td->block == -1) {
		if (verbose)
			putstr("Bad block table not found!\r\n");
		return 0;
	} else {
		if (verbose) {
			putstr("Bad block table found at block ");
			putdec(td->block);
			putstr(", page 0 ");
			putstr(", version 0x");
			puthex(td->version);
			putstr("\r\n");
		}
		return 1;
	}
}

/**
 * write_bbt - [GENERIC] (Re)write the bad block table
 *
 * (Re)write the bad block table
*/
static int write_bbt(nand_bbt_descr_t *td, nand_bbt_descr_t *md, int verbose)
{
	int i, rval;
	int numblocks, startblock, bbt_block;

	numblocks = flspinand.chip_size / flspinand.block_size;
	/* There was already a version of the table, reuse the page
	 * This applies for absolute placement too, as we have the
	 * page nr. in td->pages.
	 */
	if (td->block != -1) {
		bbt_block = td->block;
		goto write;
	}

	/* Search direction backward */
	startblock = numblocks - 1;

	for (i = 0; i < td->maxblocks; i++) {
		bbt_block = startblock - i;
		if (spinand_is_bad_block(bbt_block)) {
			startblock--;
			i--;
			continue;
		}
		/* Check, if the block is used by the mirror table */
		if (!md || md->block != bbt_block)
			goto write;
	}
	putstr("No space left to write bad block table\n");
	return -1;

write:
	/* Preset the buffer with 0xff */
	memset(page_data, 0xff, flspinand.main_size + flspinand.spare_size);

	/* bbt pattern */
	memcpy(&page_data[td->offs], td->pattern, td->len);

	/* bbt version */
	if (td->options & NAND_BBT_VERSION)
		page_data[td->veroffs] = td->version;
	/* bbt contents */
	memcpy(&page_data[add_marker_len(td)], bbt, NAND_BBT_SIZE);

	rval = spinand_erase_block(bbt_block);
	if (rval < 0) {
		putstr("erase failed. <block ");
		putdec(bbt_block);
		putstr(">\r\n");
		goto outerr;
	}

	rval = spinand_prog_pages(bbt_block, 0, 1, page_data);
	if (rval < 0) {
		putstr("program failed. <block ");
		putdec(bbt_block);
		putstr(", page 0 ");
		putstr(">\r\n");
		goto outerr;
	}

	/* Read it back for verification */
	rval = spinand_read_pages(bbt_block, 0, 1, check_buf, 1);
	if (rval < 0) {
		putstr("read failed. <block ");
		putdec(bbt_block);
		putstr(", page 0 ");
		putstr(">\r\n");
		goto outerr;
	}

	/* Compare memory content after read back */
	rval = memcmp(page_data, check_buf, flspinand.main_size);
	if (rval != 0) {
		putstr("write_bbt check failed. <block ");
		putdec(bbt_block);
		putstr(", page 0 ");
		putstr(">\r\n");
		rval = -1;
		goto outerr;
	}

	if (verbose) {
		putstr("Bad block table written to block ");
		putdec(bbt_block);
		putstr(", version 0x");
		puthex(td->version);
		putstr("\r\n");
	}

	/* Mark it as used */
	td->block = bbt_block;

	return 0;

outerr:
	spinand_mark_bad_block(bbt_block);
	putstr("nand_bbt: Error while writing bad block table.\n");
	return rval;
}

/**
 * create_bbt - [GENERIC] Create a bad block table by scanning the device
 */
static int create_bbt(int verbose)
{
	int block, ret;
	u32 total_blks;
	total_blks = flspinand.chip_size / flspinand.block_size;

	if (verbose)
		putstr("Scanning device for bad blocks\r\n");

	if (total_blks > (NAND_BBT_SIZE << 2)) {
		putstr("bbt size is too small (");
		putdec(total_blks);
		putstr("/");
		putdec(NAND_BBT_SIZE << 2);
		putstr(")\r\n");
		return -1;
	}

	for (block = 0; block < total_blks; block++) {
		ret = spinand_is_bad_block(block);
		if (ret) {
			if (verbose) {
				putstr("Bad block found at ");
				putdec(block);
				putstr("\r\n");
			}
			if (ret == NAND_LATE_DEVEL_BAD_BLOCK)
				bbt[block >> 2] &= ~(0x01 << ((block << 1) % 8));
			else
				bbt[block >> 2] &= ~(0x03 << ((block << 1) % 8));
		}
	}

	return 0;
}

/**
 * check_create - [GENERIC] create and write bbt(s) if necessary
 *
 * The function checks the results of the previous call to read_bbt
 * and creates / updates the bbt(s) if necessary
 * Creation is necessary if no bbt was found for the chip/device
 * Update is necessary if one of the tables is missing or the
 * version nr. of one table is less than the other
*/
static int check_create(nand_bbt_descr_t *td, nand_bbt_descr_t *md, int verbose)
{
	int writeops = 0, rval;
	nand_bbt_descr_t *rd = NULL, *rd2 = NULL;

	/* Mirrored table avilable ? */
	if (md) {
		if (td->block == -1 && md->block == -1) {
			writeops = 0x03;
			goto create;
		}

		if (td->block == -1) {
			rd = md;
			td->version = md->version;
			writeops = 1;
			goto writecheck;
		}

		if (md->block == -1) {
			rd = td;
			md->version = td->version;
			writeops = 2;
			goto writecheck;
		}

		if (td->version == md->version) {
			rd = td;
			if (!(td->options & NAND_BBT_VERSION))
				rd2 = md;
			goto writecheck;
		}

		if (td->version > md->version) {
			rd = td;
			md->version = td->version;
			writeops = 2;
		} else {
			rd = md;
			td->version = md->version;
			writeops = 1;
		}

		goto writecheck;

	} else {
		if (td->block == -1) {
			writeops = 0x01;
			goto create;
		}
		rd = td;
		goto writecheck;
	}

create:
	/* Create the table in memory by scanning the chip(s) */
	rval = create_bbt(verbose);
	if (rval < 0)
		return rval;

	td->version = 1;
	if (md)
		md->version = 1;

writecheck:
	/* read back first ? */
	if (rd)
		read_bbt(rd);
	/* If don't use version, read both. */
	if (rd2)
		read_bbt(rd2);

	/* Write the bad block table to the device ? */
	if (writeops & 0x01) {
		rval = write_bbt(td, md, verbose);
		if (rval < 0)
			return rval;
	}

	/* Write the mirror bad block table to the device ? */
	if ((writeops & 0x02) && md) {
		rval = write_bbt(md, td, verbose);
		if (rval < 0)
			return rval;
	}

	return 0;
}

/**
 * nand_scan_bbt - [NAND Interface] scan, find, read and maybe create bad block table(s)
 *
 * The function checks, if a bad block table(s) is/are already
 * available. If not it scans the device for manufacturer
 * marked good / bad blocks and writes the bad block table(s) to
 * the selected place.
*/
int spinand_scan_bbt(int verbose)
{
	int rval = 0;
	nand_bbt_descr_t *td;
	nand_bbt_descr_t *md;

	td = &bbt_main_descr;
	md = &bbt_mirror_descr;

	/* Avoid to be called more times */
	if (bbt_is_created)
		return 0;

	/* Preset bbt contents with 0xff */
	memset(&bbt, 0xff, NAND_BBT_SIZE);

	/* Search the primary bad block table */
	search_bbt(td, verbose);

	/* Search the mirror bad block table */
	search_bbt(md, verbose);

	rval = check_create(td, md, verbose);
	if (rval < 0)
		return rval;

	bbt_is_created = 1;

	return 0;
}

/**
 * nand_update_bbt - [NAND Interface] update bad block table(s)
 * @bblock:	the newly bad block
 * @gblock:	the newly good block save by diag_nand_rclm_bb
 *
 * The function updates the bad block table(s)
*/
int spinand_update_bbt(u32 bblock, u32 gblock)
{
	int rval = 0, writeops = 0;
	nand_bbt_descr_t *td;
	nand_bbt_descr_t *md;

	td = &bbt_main_descr;
	md = &bbt_mirror_descr;

	if (!bbt_is_created)
		return -1;

	if (bblock) {
		/* If it was bad already, return success and do nothing. */
		if (spinand_isbad_bbt(bblock))
			return 0;

		bbt[bblock >> 2] &= ~(0x01 << ((bblock << 1) % 8));
	} else {
		/* If it was good already, return success and do nothing. */
		if (!spinand_isbad_bbt(gblock))
			return 0;

		bbt[gblock >> 2] |= 0x03 << ((gblock << 1) % 8);
	}

	writeops = 0x1;
	td->version++;
	if (md) {
		md->version++;
		writeops |= 0x2;
	}

	/* Write the bad block table to the device ? */
	if (writeops & 0x01) {
		rval = write_bbt(td, md, 0);
		if (rval < 0)
			goto out;
	}

	/* Write the mirror bad block table to the device ? */
	if ((writeops & 0x02) && md) {
		rval = write_bbt(md, td, 0);
	}
out:
	return rval;
}

int spinand_erase_bbt(void)
{
	int rval;
	nand_bbt_descr_t *td;
	nand_bbt_descr_t *md;

	td = &bbt_main_descr;
	md = &bbt_mirror_descr;

	putstr("erase bad block table...\r\n");

	bbt_is_created = 0;

	search_bbt(td, 1);
	search_bbt(md, 1);

	if (td->block != -1) {
		rval = spinand_erase_block(td->block);
		if (rval < 0) {
			spinand_mark_bad_block(td->block);
			putstr("erase failed. <block ");
			putdec(td->block);
			putstr(">\r\n");
		}
	}

	if (md->block != -1) {
		rval = spinand_erase_block(md->block);
		if (rval < 0) {
			spinand_mark_bad_block(md->block);
			putstr("erase failed. <block ");
			putdec(md->block);
			putstr(">\r\n");
		}
	}

	putstr("\r\n");

	return 0;
}

int spinand_show_bbt()
{
	int i, rval = 0;
	u32 total_blks;
	total_blks = flspinand.chip_size / flspinand.block_size;

	for (i = 0; i < total_blks; i++) {
		rval = spinand_isbad_bbt(i);
		if (rval < 0)
			return -1;
		else if (rval != 0) {
			putstr("Bad block found at ");
			putdec(i);
			putstr("\r\n");
		}
	}

	return 0;
}

/**
 * nand_isbad_bbt - [NAND Interface] Check if a block is bad
*/
int spinand_isbad_bbt(u32 block)
{
	int rval = -1;
	u8 bb;

	if (!bbt_is_created)
		return rval;

	bb = (bbt[block >> 2] >> ((block << 1) % 8)) & 0x03;

	/* good block */
	if (bb == 0x03)
		rval = NAND_GOOD_BLOCK;
	else if (bb == 0x02 || bb == 0x01)
		rval = NAND_LATE_DEVEL_BAD_BLOCK;
	else if (bb == 0x00)
		rval = NAND_INITIAL_BAD_BLOCK;

	return rval;
}

/**
 * nand_has_bbt - Does nand have bad block table
*/
int spinand_has_bbt()
{
	return bbt_is_created;
}
#endif

static void spinor_setup_dma_devmem(struct spinor_ctrl *data)
{
	u32 reg;

	clean_flush_d_cache(data->buf, data->len);

	reg = DMA_CHAN_STA_REG(NOR_SPI_RX_DMA_CHAN);
	writel(reg, 0x0);

	reg = DMA_CHAN_SRC_REG(NOR_SPI_RX_DMA_CHAN);
	writel(reg, SPINOR_RXDATA_REG);

	reg = DMA_CHAN_DST_REG(NOR_SPI_RX_DMA_CHAN);
	writel(reg, (u32)data->buf);

	reg = DMA_CHAN_CTR_REG(NOR_SPI_RX_DMA_CHAN);
	writel(reg, DMA_CHANX_CTR_EN | DMA_CHANX_CTR_WM |
		    DMA_CHANX_CTR_NI | DMA_CHANX_CTR_BLK_32B |
		    DMA_CHANX_CTR_TS_4B | (data->len & (~0x1f)));

	writel(SPINOR_DMACTRL_REG, SPINOR_DMACTRL_RXEN);
}

static void spinor_setup_dma_memdev(struct spinor_ctrl *data)
{
	u32 reg;

	clean_flush_d_cache(data->buf, data->len);

	reg = DMA_CHAN_STA_REG(NOR_SPI_TX_DMA_CHAN);
	writel(reg, 0x0);

	reg = DMA_CHAN_SRC_REG(NOR_SPI_TX_DMA_CHAN);
	writel(reg, (u32)data->buf);

	reg = DMA_CHAN_DST_REG(NOR_SPI_TX_DMA_CHAN);
	writel(reg, SPINOR_TXDATA_REG);

	reg = DMA_CHAN_CTR_REG(NOR_SPI_TX_DMA_CHAN);
	writel(reg, DMA_CHANX_CTR_EN | DMA_CHANX_CTR_RM |
		    DMA_CHANX_CTR_NI | DMA_CHANX_CTR_BLK_32B |
		    DMA_CHANX_CTR_TS_4B | data->len);

	writel(SPINOR_DMACTRL_REG, SPINOR_DMACTRL_TXEN);
}

static int spinor_send_cmd(struct spinor_ctrl *cmd, struct spinor_ctrl *addr,
		struct spinor_ctrl *data, struct spinor_ctrl *dummy)
{
	u32 reg_length = 0, reg_ctrl = 0;
	u32 val, i;

	/* setup basic info */
	if (cmd != NULL) {
		reg_length |= SPINOR_LENGTH_CMD(cmd->len);

		reg_ctrl |= cmd->is_dtr ? SPINOR_CTRL_CMDDTR : 0;
		switch(cmd->lane) {
		case 8:
			reg_ctrl |= SPINOR_CTRL_CMD8LANE;
			break;
		case 4:
			reg_ctrl |= SPINOR_CTRL_CMD4LANE;
			break;
		case 2:
			reg_ctrl |= SPINOR_CTRL_CMD2LANE;
			break;
		case 1:
		default:
			reg_ctrl |= SPINOR_CTRL_CMD1LANE;
			break;
		}
	} else {
		return -1;
	}

	if (addr != NULL) {
		reg_length |= SPINOR_LENGTH_ADDR(addr->len);

		reg_ctrl |= addr->is_dtr ? SPINOR_CTRL_ADDRDTR : 0;
		switch(addr->lane) {
		case 4:
			reg_ctrl |= SPINOR_CTRL_ADDR4LANE;
			break;
		case 2:
			reg_ctrl |= SPINOR_CTRL_ADDR2LANE;
			break;
		case 1:
		default:
			reg_ctrl |= SPINOR_CTRL_ADDR1LANE;
			break;
		}
	}

	if (data != NULL) {
		if (data->len > SPINOR_MAX_DATA_LENGTH) {
			putstr("spinor: data length is too large.\r\n");
			return -1;
		}

		if (data->is_dma && ((u32)data->buf & 0x7)) {
			putstr("spinor: data buf is not 8 Bytes align.\r\n");
			return -1;
		}

		reg_length |= SPINOR_LENGTH_DATA(data->len);

		reg_ctrl |= data->is_dtr ? SPINOR_CTRL_DATADTR : 0;
		switch(data->lane) {
		case 8:
			reg_ctrl |= SPINOR_CTRL_DATA8LANE;
			break;
		case 4:
			reg_ctrl |= SPINOR_CTRL_DATA4LANE;
			break;
		case 2:
			reg_ctrl |= SPINOR_CTRL_DATA2LANE;
			break;
		case 1:
		default:
			reg_ctrl |= SPINOR_CTRL_DATA1LANE;
			break;
		}

		if (data->is_read)
			reg_ctrl |= SPINOR_CTRL_RDEN;
		else
			reg_ctrl |= SPINOR_CTRL_WREN;

		if (!data->is_io)
			reg_ctrl |= SPINOR_CTRL_RXLANE_TXRX;

		if (data->is_dma && data->len < 32)
			data->is_dma = 0;
	}

	if (dummy != NULL) {
		reg_length |= SPINOR_LENGTH_DUMMY(dummy->len);
		reg_ctrl |= dummy->is_dtr ? SPINOR_CTRL_DUMMYDTR : 0;
	}

	writel(SPINOR_LENGTH_REG, reg_length);
	writel(SPINOR_CTRL_REG, reg_ctrl);

	/* setup cmd id */
	val = 0;
	for (i = 0; i < cmd->len; i++)
		val |= (cmd->buf[i] << (i << 3));
	writel(SPINOR_CMD_REG, val);

	/* setup address */
	val = 0;
	for (i = 0; addr && i < addr->len; i++) {
		if (i >= 4)
			break;
		val |= (addr->buf[i] << (i << 3));
	}
	writel(SPINOR_ADDRLO_REG, val);

	val = 0;
	for (; addr && i < addr->len; i++)
		val |= (addr->buf[i] << ((i - 4) << 3));
	writel(SPINOR_ADDRHI_REG, val);

	/* setup dma if data phase is existed and dma is required.
	 * Note: for READ, dma will just transfer the length multiple of
	 * 32Bytes, the residual data in FIFO need to be read manually. */
	if (data != NULL && data->is_dma) {
		if (data->is_read)
			spinor_setup_dma_devmem(data);
		else
			spinor_setup_dma_memdev(data);
	} else if (data != NULL && !data->is_read) {
		if (data->len >= 0x7f) {
			putstr("spinor: tx length exceeds fifo size.\r\n");
			return -1;
		}
		for (i = 0; i < data->len; i++) {
			writeb(SPINOR_TXDATA_REG, data->buf[i]);
		}
	}

	/* clear all pending interrupts */
	writel(SPINOR_CLRINTR_REG, SPINOR_INTR_ALL);

	/* start tx/rx transaction */
	writel(SPINOR_START_REG, 0x1);

	/* wait for spi done */
	while((readl(SPINOR_RAWINTR_REG) & SPINOR_INTR_DATALENREACH) == 0x0);

	if (data != NULL && data->is_dma) {
		u32 chan, n;

		if (data->is_read)
			chan = NOR_SPI_RX_DMA_CHAN;
		else
			chan = NOR_SPI_TX_DMA_CHAN;
		/* wait for dma done */
		while(!(readl(DMA_REG(DMA_INT_OFFSET)) & DMA_INT_CHAN(chan)));
		writel(DMA_REG(DMA_INT_OFFSET), 0x0);	/* clear */

		if (data->is_read) {
			clean_flush_d_cache(data->buf, data->len);
			/* read the residual data in FIFO manually */
			n = data->len & (~0x1f);
			for (i = 0; i < (data->len & 0x1f); i++)
				data->buf[n + i] = readb(SPINOR_RXDATA_REG);
		}
		/* disable spi-nor dma */
		writel(SPINOR_DMACTRL_REG, 0x0);
	} else if (data != NULL && data->is_read) {
		for (i = 0; i < data->len; i++) {
			data->buf[i] = readb(SPINOR_RXDATA_REG);
		}
	}

	return 0;
}

int spinor_send_alone_cmd(u8 cmd_id)
{
	struct spinor_ctrl cmd;
	int rval;

	cmd.buf = &cmd_id;
	cmd.len = 1;
	cmd.lane = 1;
	cmd.is_dtr = 0;

	rval = spinor_send_cmd(&cmd, NULL, NULL, NULL);

	return rval;
}

int spinor_read_reg(u8 cmd_id, void *buf, u8 len)
{
	struct spinor_ctrl cmd;
	struct spinor_ctrl data;
	int rval;

	cmd.buf = &cmd_id;
	cmd.len = 1;
	cmd.lane = 1;
	cmd.is_dtr = 0;

	data.buf = buf;
	data.len = len;
	data.lane = 2;
	data.is_read = 1;
	data.is_io = 0;
	data.is_dtr = 0;
	data.is_dma = 0;

	rval = spinor_send_cmd(&cmd, NULL, &data, NULL);

	return rval;
}

/* spinor_write_reg is also called when issuing operation cmd, e.g., SE */
int spinor_write_reg(u8 cmd_id, void *buf, u8 len)
{
	struct spinor_ctrl cmd;
	struct spinor_ctrl addr;
	int rval;

	cmd.buf = &cmd_id;
	cmd.len = 1;
	cmd.lane = 1;
	cmd.is_dtr = 0;

	addr.buf = buf;
	addr.len = len;
	addr.lane = 1;
	addr.is_dtr = 0;

	rval = spinor_send_cmd(&cmd, &addr, NULL, NULL);

	return rval;
}

static int spinand_set_feature(u8 feature_addr, void *buf)
{
	struct spinor_ctrl cmd;
	struct spinor_ctrl addr;
	struct spinor_ctrl data;
	struct spinor_ctrl dummy;
	u8 cmd_id;
	int rval;

	cmd_id = SPINAND_CMD_SET_REG;
	cmd.buf = &cmd_id;
	cmd.len = 1;
	cmd.lane = 1;
	cmd.is_dtr = 0;

	addr.buf = (u8 *)&feature_addr;
	addr.len = 1;
	addr.lane = 1;
	addr.is_dtr = 0;

	data.buf = buf;
	data.len = 1;
	data.lane = 1;
	data.is_dtr = 0;
	data.is_read = 0;
	data.is_io = 0;
	data.is_dma = 0;

	dummy.len = 0;
	dummy.is_dtr = 0;

	rval = spinor_send_cmd(&cmd, &addr, &data, &dummy);
	if (rval < 0)
		return rval;

	return 0;
}

static int spinand_get_feature(u8 feature_addr, void *buf)
{
	struct spinor_ctrl cmd;
	struct spinor_ctrl addr;
	struct spinor_ctrl data;
	struct spinor_ctrl dummy;
	u8 cmd_id;
	int rval;

	cmd_id = SPINAND_CMD_GET_REG;
	cmd.buf = &cmd_id;
	cmd.len = 1;
	cmd.lane = 1;
	cmd.is_dtr = 0;

	addr.buf = (u8 *)&feature_addr;
	addr.len = 1;
	addr.lane = 1;
	addr.is_dtr = 0;

	data.buf = buf;
	data.len = 1;
	data.lane = 2;
	data.is_dtr = 0;
	data.is_read = 1;
	data.is_io = 0;
	data.is_dma = 0;

	dummy.len = 0;
	dummy.is_dtr = 0;

	rval = spinor_send_cmd(&cmd, &addr, &data, &dummy);
	if (rval < 0)
		return rval;

	return 0;
}

#if 0
static int spinand_get_status(void *value)
{
       return spinand_get_feature(REG_STATUS, value);
}

static int spinand_get_pro(void *value)
{
       return spinand_get_feature(REG_PROTECTION, value);
}

static int spinand_get_ecc_en(void *value)
{
       return spinand_get_feature(REG_FEATURE, value);
}
#endif

static int spinand_disable_ecc(void)
{
	u8 status;
	int ret;

	status = 0;
	spinand_get_feature(REG_FEATURE, &status);
	status &= (~SPI_NAND_ECC_EN);
	ret = spinand_set_feature(REG_FEATURE, &status);
	return ret;
}

static int spinand_enable_ecc(void)
{
	u8 status;
	int ret;

	status = 0;
	spinand_get_feature(REG_FEATURE, &status);
	status |= SPI_NAND_ECC_EN;
	ret = spinand_set_feature(REG_FEATURE, &status);
	return ret;
}

int spinand_wait_till_ready()
{
	int rval = 0;
	u8 status;
	do {
			rval = spinand_get_feature(REG_STATUS, &status);
			if (rval < 0)
				return rval;
		} while (status & 0x1);
	return 0;
}

int spinand_erase_block(u32 block_id)
{
	u32 address;
	u8 status;
	int ret = 0;

	spinor_send_alone_cmd(SPINAND_CMD_WREN);

	address = block_id * flspinand.pages_per_block;
	spinor_write_reg(SPINAND_CMD_ERASE_BLK, &address, 3);

	/* Wait until the operation completes or a timeout occurs. */
	ret = spinand_wait_till_ready();
	if (ret) {
		if (ret < 0) {
			putstr("Wait execution complete failed!\n");
			return ret;
		}
	}

	/* Check status register for program fail bit */
	ret = spinand_get_feature(REG_STATUS, &status);
	if (ret < 0) {
		putstr("Error when reading status register.\n");
		return ret;
	}

	if (status & STATUS_E_FAIL) {
		putstr("Erase failed on block 0x");
		puthex(block_id);
		putstr("\r\n");
		   return -1;
	}
	return 0;
}

static int spinand_read_page_to_cache(u32 page_id)
{
	struct spinor_ctrl cmd;
	struct spinor_ctrl addr;
	int rval = 0;
	u8 cmd_id;

	cmd_id = SPINAND_CMD_READ;
	cmd.buf = &cmd_id;
	cmd.len = 1;
	cmd.lane = 1;
	cmd.is_dtr = 0;

	addr.buf = (u8 *)&page_id;
	addr.len = 3;
	addr.lane = 1;
	addr.is_dtr = 0;

	rval = spinor_send_cmd(&cmd, &addr, NULL, NULL);
	if (rval < 0) {
		putstr("send read command error \r\n");
	}

	return rval;
}

static int spinand_read_from_cache(u32 byte_id, void *rbuf, int len)
{
	struct spinor_ctrl cmd;
	struct spinor_ctrl addr;
	struct spinor_ctrl data;
	struct spinor_ctrl dummy;
	u8 cmd_id, is_dma;
	//u32 addr_id;

	cmd_id = SPINAND_CMD_RFC;
	cmd.buf = &cmd_id;
	cmd.len = 1;
	cmd.lane = 1;
	cmd.is_dtr = 0;

	//addr_id = 0;
	//addr_id = byte_id << 8;
	addr.buf = (u8 *)&byte_id;
	//addr.buf = (u8 *)&addr_id;
	addr.len = 3;//sizeof(byte_id);
	addr.lane = 1;
	addr.is_dtr = 0;

	if ((u32)rbuf & 0x7) {
		is_dma = 0;
	} else {
		is_dma = 1;
	}

	data.buf = rbuf;
	data.len = len;
	data.lane = 2;
	data.is_dtr = 0;
	data.is_read = 1;
	data.is_io = 0;
	data.is_dma = is_dma;

	dummy.len = 0;
	dummy.is_dtr = 0;

	return spinor_send_cmd(&cmd, &addr, &data, &dummy);
}

int spinand_read_page(u32 page, u32 offset, void *buf, int len, u32 enable_ecc)
{
	int ret;
	u8 status;

	/* Issue read cache command */
	ret = spinand_read_page_to_cache(page);
	if (ret < 0) {
		putstr("Error when reading page to cache.\r\n");
		return ret;
	}

	/* Wait until the operation completes or a timeout occurs. */
	ret = spinand_wait_till_ready();
	if (ret) {
		if (ret < 0) {
			putstr("Wait execution complete failed!\n");
			return ret;
		}
	}

	ret = spinand_read_from_cache(offset, buf, len);
	if (ret < 0) {
		putstr("Error when reading page from cache.\r\n");
		return ret;
	}

	status = 0;
	/* Check status register for ecc bits */
	ret = spinand_get_feature(REG_STATUS, &status);
	if (ret < 0) {
		putstr("Error when reading status register.\n");
		return ret;
	}

	if (enable_ecc) {
		if ((status & STATUS_ECC) == STATUS_ECC) {
			putstr("ecc error on status value 0x");
			puthex(status);
			putstr("\r\n");
			putstr("ecc error on page 0x");
			puthex(page);
			putstr("\r\n");
			putstr("ecc error on offset 0x");
			puthex(offset);
			putstr("\r\n");
			puthex(len);
			putstr("\r\n");
			return -1;
		}
	}

	return 0;
}

int spinand_read_pages(u32 block, u32 page, u32 pages,
		void *rbuf, u32 enable_ecc)
{
	u32 i, page_start;
	u32 len;
	u8 *buf;
	int ret = 0;

	buf = rbuf;
	//len = flspinand.main_size + flspinand.spare_size;
	len = flspinand.main_size;
	page_start = block * flspinand.pages_per_block + page;

	if (enable_ecc == 0)
		spinand_disable_ecc();

	for (i = page_start; i < (page_start + pages); i++) {
		ret = spinand_read_page(i, 0, buf, len, enable_ecc);
		if (ret < 0) {
			if (enable_ecc == 0)
				spinand_enable_ecc();
			return ret;
		}
		buf += len;
	}

	if (enable_ecc == 0)
		spinand_enable_ecc();
	return ret;
}
static int spinand_program_data_to_cache(u16 byte_id, void *buf, int len)
{
	struct spinor_ctrl cmd;
	struct spinor_ctrl addr;
	struct spinor_ctrl data;
	u8 cmd_id, is_dma;

	cmd_id = SPINAND_CMD_PRG_LOAD;
	cmd.buf = &cmd_id;
	cmd.len = 1;
	cmd.lane = 1;
	cmd.is_dtr = 0;

	addr.buf = (u8 *)&byte_id;
	addr.len = 2;//sizeof(byte_id);
	addr.lane = 1;
	addr.is_dtr = 0;

	if ((u32)buf & 0x7) {
		is_dma = 0;
	} else {
		is_dma = 1;
	}

	data.buf = buf;
	data.len = len;
	data.lane = 1;
	data.is_dtr = 0;
	data.is_read = 0;
	data.is_io = 0;
	data.is_dma = is_dma;

	return spinor_send_cmd(&cmd, &addr, &data, NULL);

}

static int spinand_program_execute(void *buf)
{
	struct spinor_ctrl cmd;
	struct spinor_ctrl addr;
	u8 cmd_id;
	int rval;

	cmd_id = SPINAND_CMD_PRG_EXC;
	cmd.buf = &cmd_id;
	cmd.len = 1;
	cmd.lane = 1;
	cmd.is_dtr = 0;

	addr.buf = buf;
	addr.len = 3;
	addr.lane = 1;
	addr.is_dtr = 0;

	rval = spinor_send_cmd(&cmd, &addr, NULL, NULL);

	return rval;
}

int spinand_program_page(u32 page, u16 offset, void *buf, int len)
{
	int ret;
	u8 status;
	u32 page_id;

	/* Issue program cache command */
	ret = spinand_program_data_to_cache(offset, buf, len);
	if (ret < 0) {
		putstr("Error when programming cache.\n");
		return ret;
	}

	spinor_send_alone_cmd(SPINOR_CMD_WREN);

	page_id = page;
	/* Issue program execute command */
	ret = spinand_program_execute(&page_id);
	if (ret < 0) {
		putstr("Error when programming NAND cells.\n");
		return ret;
	}

	/* Wait until the operation completes or a timeout occurs. */
	ret = spinand_wait_till_ready();
	if (ret) {
		if (ret < 0) {
			putstr("Wait execution complete failed!\n");
			return ret;
		}
	}

	/* Check status register for program fail bit */
	ret = spinand_get_feature(REG_STATUS, &status);
	if (ret < 0) {
		putstr("Error when reading status register.\n");
		return ret;
	}

	if (status & STATUS_P_FAIL) {
		putstr("Program failed on page 0x");
		puthex(page);
		putstr("\r\n");
		   return -1;
	}
	return 0;
}

int spinand_prog_pages(u32 block, u32 page, u32 pages, void *wbuf)
{
	u32 i, page_start;
	u32 len;
	int ret = 0;
	u8 *buf;

	page_start = block * flspinand.pages_per_block + page;
	len = flspinand.main_size;
	buf = wbuf;

	for (i = page_start; i < (page_start + pages); i++) {
		ret = spinand_program_page(i, 0, buf, len);
		if (ret < 0) {
			putstr("program page failed at 0x");
			puthex(i);
			putstr("\r\n");
			return ret;
		}
		buf += len;
	}

	return ret;
}

static int spinand_read(u32 block, u32 page, u32 pages, u8 *buf)
{
	int rval = 0;
	u32 first_blk_pages, blocks, last_blk_pages;
	u32 bad_blks = 0;

#if defined(DEBUG)
	putstr("spinand_read( ");
	putdec(block);
	putstr(", ");
	putdec(page);
	putstr(", ");
	putdec(pages);
	putstr(", 0x");
	puthex((u32)buf);
	putstr(" )\r\n");
#endif

	first_blk_pages = flspinand.pages_per_block - page;
	if (pages > first_blk_pages) {
		pages -= first_blk_pages;
		blocks = pages / flspinand.pages_per_block;
		last_blk_pages = pages % flspinand.pages_per_block;
	} else {
		first_blk_pages = pages;
		blocks = 0;
		last_blk_pages = 0;
	}

	if (first_blk_pages) {
		while (spinand_is_bad_block(block)) {
			/* if bad block, find next */
			block++;
			bad_blks++;
		}
		rval = spinand_read_pages(block, page, first_blk_pages, buf, 1);
		if (rval < 0)
			return -1;
		block++;
		buf += first_blk_pages * flspinand.main_size;
	}

	while (blocks > 0) {
		while (spinand_is_bad_block(block)) {
			/* if bad block, find next */
			block++;
			bad_blks++;
		}
		rval = spinand_read_pages(block, 0, flspinand.pages_per_block, buf, 1);
		if (rval < 0)
			return -1;
		block++;
		blocks--;
		buf += flspinand.block_size;
	}

	if (last_blk_pages) {
		while (spinand_is_bad_block(block)) {
			/* if bad block, find next */
			block++;
			bad_blks++;
		}
		rval = spinand_read_pages(block, 0, last_blk_pages, buf, 1);
		if (rval < 0)
			return -1;
	}

	return bad_blks;
}

static void spinand_get_offset_adr(u32 *block, u32 *page, u32 pages, u32 bad_blks)
{
	u32 blocks;

	blocks = pages / flspinand.pages_per_block;
	pages  = pages % flspinand.pages_per_block;

	*block =  *block + blocks;
	*page += pages;

	if (*page >= flspinand.pages_per_block) {
		*page -= flspinand.pages_per_block;
		*block += 1;
	}

	*block += bad_blks;
}

int spinand_read_data(u8 *dst, u8 *src, int len)
{
	u32 block, page, pages, pos;
	u32 first_ppage_size, last_ppage_size;
	int val, rval = -1;

#if defined(DEBUG)
	putstr("spinand_read_data( 0x");
	puthex((u32)dst);
	putstr(", 0x");
	puthex((u32)src);
	putstr(", ");
	putdec(len);
	putstr(" )\r\n");
#endif

	/* translate address to block, page, address */
	val = (int) src;
	block = val / flspinand.block_size;
	val  -= block * flspinand.block_size;
	page  = val / flspinand.main_size;
	pos   = val % flspinand.main_size;
	pages = len / flspinand.main_size;

	if (pos == 0)
		first_ppage_size = 0;
	else
		first_ppage_size = flspinand.main_size - pos;

	if (len >= first_ppage_size) {
		pages = (len - first_ppage_size) / flspinand.main_size;

		last_ppage_size = (len - first_ppage_size) % flspinand.main_size;
	} else {
		first_ppage_size = len;
		pages = 0;
		last_ppage_size = 0;
	}

	if (len !=
	    (first_ppage_size + pages * flspinand.main_size + last_ppage_size)) {
		return -1;
	}

	len = 0;
	if (first_ppage_size) {
		rval = spinand_read(block, page, 1, spinand_cache_buf);
		if (rval < 0)
			return len;

		memcpy(dst, (void *) (spinand_cache_buf + pos), first_ppage_size);
		dst += first_ppage_size;
		len += first_ppage_size;
		spinand_get_offset_adr(&block, &page, 1, rval);
	}

	if (pages > 0) {
		rval = spinand_read(block, page, pages, dst);
		if (rval < 0)
			return len;

		dst += pages * flspinand.main_size;
		len += pages * flspinand.main_size;
		spinand_get_offset_adr(&block, &page, pages, rval);
	}

	if (last_ppage_size > 0) {
		rval = spinand_read(block, page, 1, spinand_cache_buf);
		if (rval < 0)
			return len;

		memcpy(dst, (void *) spinand_cache_buf, last_ppage_size);
		len += last_ppage_size;
	}

	return len;
}

int spinand_init(void)
{
	int part_size[HAS_IMG_PARTS];
	u32 divider, val, i;
	u32 sblk, nblk;
	u32 total_blocks;
	u8 status = 0;

	rct_set_ssi3_pll();

	/* make sure spinor occupy dma channel 0 */
	val = readl(AHB_SCRATCHPAD_REG(0x0c));
	val &= ~(1 << 21);
	writel(AHB_SCRATCHPAD_REG(0x0c), val);

	divider = get_ssi3_freq_hz() / SPINOR_SPI_CLOCK;

	/* global configuration for spinor controller. Note: if the
	 * data received is invalid, we need to tune rxsampldly.*/
	val = SPINOR_CFG_FLOWCTRL_EN |
	      SPINOR_CFG_HOLDPIN(3) |
	      SPINOR_CFG_CHIPSEL(SPINOR_FLASH_CHIP_SEL) |
	      SPINOR_CFG_CLKDIV(divider) |
	      SPINOR_CFG_RXSAMPDLY(SPINOR_RX_SAMPDLY);
	writel(SPINOR_CFG_REG, val);

	/* mask all interrupts */
	writel(SPINOR_INTRMASK_REG, SPINOR_INTR_ALL);

	/* reset tx/rx fifo */
	writel(SPINOR_TXFIFORST_REG, 0x1);
	writel(SPINOR_RXFIFORST_REG, 0x1);
	/* SPINOR_RXFIFOLV_REG may have invalid value, so we read rx fifo
	 * manually to clear tx fifo. This is a hardware bug. */
	while (readl(SPINOR_RXFIFOLV_REG) != 0) {
		val = readl(SPINOR_RXDATA_REG);
	}

	writel(SPINOR_TXFIFOTHLV_REG, 31);
	writel(SPINOR_RXFIFOTHLV_REG, 31);

	flspinand.main_size = SPINAND_MAIN_SIZE;
	flspinand.spare_size = SPINAND_SPARE_SIZE;
	flspinand.pages_per_block = SPINAND_PAGES_PER_BLOCK;
	flspinand.block_size = flspinand.main_size * flspinand.pages_per_block;
	flspinand.chip_size = SPINAND_CHIP_SIZE;

	sblk = nblk = 0;
	get_part_size(part_size);

	for (i = 0; i < HAS_IMG_PARTS; i++) {
		if ((get_part_dev(i) & PART_DEV_SPINAND) != PART_DEV_SPINAND) {
			continue;
		}

		sblk += nblk;
		nblk = part_size[i] / flspinand.block_size;
		if ((part_size[i] % flspinand.block_size) != 0x0)
			nblk++;
		flspinand.sblk[i] = (nblk == 0) ? 0 : sblk;
		flspinand.nblk[i] = nblk;
	}
	for (; i < PART_MAX; i++) {
		flspinand.sblk[i] = 0;
		flspinand.nblk[i] = 0;
	}
	total_blocks = (flspinand.chip_size / flspinand.block_size);
	nblk = (total_blocks > sblk) ? (total_blocks - sblk) : 0;
	//Raw part include BBT, take care!
	flspinand.sblk[PART_RAW] = (nblk == 0) ? 0 : sblk;
	flspinand.nblk[PART_RAW] = nblk;
	if (flspinand.sblk[PART_RAW] < 2) {
		putstr("No Space for BBT!\n\r");
	}

	spinand_flash_reset();

	val = 0;
	spinor_read_reg(SPINOR_CMD_READID, &val, sizeof(val));

	putstr("spi device flash ID is 0x");
	puthex(val);
	putstr("\r\n");

	status = 0;
	//spinand_get_pro(&val);
	spinand_get_feature(REG_PROTECTION, &status);
	putstr("spi device flash pro value is 0x");
	puthex(status);
	putstr("\r\n");

	status = 0;
	spinand_set_feature(REG_PROTECTION, &status);
	/* unlock all blocks */
	status = 0;
	spinand_get_feature(REG_PROTECTION, &status);
	putstr("spi device flash pro value is 0x");
	puthex(status);
	putstr("\r\n");

	status = 0;
	//spinand_get_status(&val);
	spinand_get_feature(REG_STATUS, &status);
	putstr("spi device flash status value is 0x");
	puthex(status);
	putstr("\r\n");

	status = 0;
	//spinand_get_ecc_en(&val);
	spinand_get_feature(REG_FEATURE, &status);
	putstr("spi device flash ecc value is 0x");
	puthex(status);
	putstr("\r\n");

	status = 0;
	//spinand_get_status(&val);
	spinand_get_feature(REG_STATUS, &status);
	putstr("spi device flash status value is 0x");
	puthex(status);
	putstr("\r\n");

	return 0;
}

/* spinand read spare*/
int spinand_read_spare(u32 block, u32 page, u32 pages, void *buf)
{
	int i = 0;
	int ret;
	u32 page_start = 0;

	page_start = block * flspinand.pages_per_block + page;
	ret = spinand_disable_ecc();
	if (ret < 0) {
		putstr("disable ecc error.\r\n");
		spinand_enable_ecc();
		return ret;
	}
	for (i = page_start; i < (page_start + pages); i++)
	{
		ret = spinand_read_page(i, flspinand.main_size, buf, flspinand.spare_size, 0);
		if (ret < 0)
			return ret;
		buf += flspinand.spare_size;
	}
	ret = spinand_enable_ecc();
	return ret;
}

/**
 * Program spare area to NAND flash.
 * Only for mark bad block, disable ECC.
 * only one byte for 0x800H pos
 */
int spinand_prog_spare(u32 block, u32 page, u32 pages, void *buf, u32 len)
{
	int i = 0;
	int ret;
	u32 page_start = 0;

	page_start = block * flspinand.pages_per_block + page;
	ret = spinand_disable_ecc();
	if (ret < 0) {
		putstr("disable ecc 2 error.\r\n");
		spinand_enable_ecc();
		return ret;
	}
	i = page_start;
	//for (i = page_start; i < (page_start + pages); i++)
	{
		ret = spinand_program_page(i, flspinand.main_size, buf, len);
		if (ret < 0)
			return ret;
		buf += len;
	}
	ret = spinand_enable_ecc();
	return ret;
}

/**
 * Check for bad block.
 */
int spinand_is_bad_block(u32 block)
{
	int ret_val = -1;
	u8 sbuf[1024], *sbuf_ptr;
	u8 bi;

	/* make sure 32 bytes aligned */
	sbuf_ptr = (u8 *)(((u32)sbuf + 31) & (~31));

#if defined(CONFIG_SPINAND_USE_FLASH_BBT)
	if(spinand_has_bbt())
		return spinand_isbad_bbt(block);
#endif

	ret_val = spinand_read_spare(block, 0, 1, sbuf_ptr);
	if (ret_val < 0) {
		putstr("check bad block failed >> "
				"read spare data error.\r\n");
		/* Treat as factory bad block */
		return NAND_INITIAL_BAD_BLOCK;
	}

	bi = *sbuf_ptr;

	if (bi != 0xff)
		return NAND_INITIAL_BAD_BLOCK;
	else
		return NAND_GOOD_BLOCK;
}

void spinand_output_bad_block(u32 block, int bb_type)
{
	if (bb_type & NAND_INITIAL_BAD_BLOCK) {
		putstr("initial bad block. <block ");
	} else if (bb_type & NAND_LATE_DEVEL_BAD_BLOCK) {
		putstr("late developed bad block. <block ");
	} else {
		putstr("other bad block. <block ");
	}
	putdec(block);
	putstr(">\r\n");
	putstr("Try next block...\r\n");
}

/**
 * Mark a bad block.
 */
int spinand_mark_bad_block(u32 block)
{
	int ret_val = -1;
	u8 sbuf[256], *sbuf_ptr;
	u8 bi;

	/* make sure 32 bytes aligned */
	sbuf_ptr = (u8 *)(((u32)sbuf + 31) & (~31));

	putstr("try to mark bad block failed >> "
				"verify failed at block ");
			putdec(block);
			putstr("\r\n");
#if defined(CONFIG_SPINAND_USE_FLASH_BBT)
	spinand_update_bbt(block, 0);
#endif

	memset(sbuf_ptr, 0xff, flspinand.spare_size);
	*sbuf_ptr = AMB_BAD_BLOCK_MARKER;

	ret_val = spinand_prog_spare(block, 0, 1, sbuf_ptr, 16);
	if (ret_val < 0) {
			putstr("mark bad block failed >> "
				"write spare data error.\r\n");
			return ret_val;
		}

		ret_val = spinand_read_spare(block, 0, 1, sbuf_ptr);
		if (ret_val < 0) {
			putstr("mark bad block failed >> "
				"read spare data error.\r\n");
			return ret_val;
		}

		bi = *sbuf_ptr;

		if (bi == 0xff) {
			putstr("mark bad block failed >> "
				"verify failed at block ");
			putdec(block);
			putstr("\r\n");
			return -1;
		}
	return 0;
}
