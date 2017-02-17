/**
 * bld/nand_bbt.c
 *
 * Bad block table support for NAND chips.
 *
 * History:
 *    2011/09/21 - [Kerson Chen] created file
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


#include <bldfunc.h>
#include <ambhw/nand.h>
#include <fio/ftl_const.h>

#if defined(CONFIG_NAND_USE_FLASH_BBT)
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

	rval = nand_read_pages(td->block, 0, 1, page_data, NULL, 1);
	if (rval < 0) {
		nand_mark_bad_block(td->block);
		putstr("read failed. <block ");
		putdec(td->block);
		putstr(", page 0 ");
		putstr(">\r\n");
		return -1;
	}

	offs = add_marker_len(td);
	for (i = 0; i < (flnand.blocks_per_bank >> 2); i++)
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

	/* Reset version information */
	td->version = 0;
	td->block = -1;

	/* Search direction backward */
	startblock = flnand.blocks_per_bank - 1;

	/* Scan the maximum number of blocks */
	for (block = 0; block < td->maxblocks; block++) {
		actblock = startblock - block;
		if (nand_is_bad_block(actblock)) {
			startblock--;
			block--;
			continue;
		}

		/* Read first page */
		rval = nand_read_pages(actblock, 0, 1, page_data, NULL, 1);
		if (rval < 0) {
			nand_mark_bad_block(actblock);
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

	numblocks = flnand.blocks_per_bank;
	/* There was already a version of the table, reuse the page
	 * This applies for absolute placement too, as we have the
	 * page nr. in td->pages.
	 */
	if (td->block != -1) {
		bbt_block = td->block;
		goto write;
	}

	/* Search direction backward */
	startblock = flnand.blocks_per_bank - 1;

	for (i = 0; i < td->maxblocks; i++) {
		bbt_block = startblock - i;
		if (nand_is_bad_block(bbt_block)) {
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
	memset(page_data, 0xff, flnand.main_size + flnand.spare_size);

	/* bbt pattern */
	memcpy(&page_data[td->offs], td->pattern, td->len);

	/* bbt version */
	if (td->options & NAND_BBT_VERSION)
		page_data[td->veroffs] = td->version;
	/* bbt contents */
	memcpy(&page_data[add_marker_len(td)], bbt, NAND_BBT_SIZE);

	rval = nand_erase_block(bbt_block);
	if (rval < 0) {
		putstr("erase failed. <block ");
		putdec(bbt_block);
		putstr(">\r\n");
		goto outerr;
	}

	/* dual spare mode use ecc bit >1 case */
	if (flnand.ecc_bits > 1)
		rval = nand_prog_pages(bbt_block, 0, 1, page_data, page_data + flnand.main_size);
	else
		rval = nand_prog_pages(bbt_block, 0, 1, page_data, NULL);
	if (rval < 0) {
		putstr("program failed. <block ");
		putdec(bbt_block);
		putstr(", page 0 ");
		putstr(">\r\n");
		goto outerr;
	}

	/* Read it back for verification */
	if (flnand.ecc_bits > 1)
		rval = nand_read_pages(bbt_block, 0, 1, check_buf, check_buf + flnand.main_size, 1);
	else
		rval = nand_read_pages(bbt_block, 0, 1, check_buf, NULL, 1);
	if (rval < 0) {
		putstr("read failed. <block ");
		putdec(bbt_block);
		putstr(", page 0 ");
		putstr(">\r\n");
		goto outerr;
	}

	/* Compare memory content after read back */
	rval = memcmp(page_data, check_buf, flnand.main_size);
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
	nand_mark_bad_block(bbt_block);
	putstr("nand_bbt: Error while writing bad block table.\n");
	return rval;
}

/**
 * create_bbt - [GENERIC] Create a bad block table by scanning the device
 */
static int create_bbt(int verbose)
{
	int block, ret;

	if (verbose)
		putstr("Scanning device for bad blocks\r\n");

	if (flnand.blocks_per_bank > (NAND_BBT_SIZE << 2)) {
		putstr("bbt size is too small (");
		putdec(flnand.blocks_per_bank);
		putstr("/");
		putdec(NAND_BBT_SIZE << 2);
		putstr(")\r\n");
		return -1;
	}

	for (block = 0; block < flnand.blocks_per_bank; block++) {
		ret = nand_is_bad_block(block);
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
int nand_scan_bbt(int verbose)
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
int nand_update_bbt(u32 bblock, u32 gblock)
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
		if (nand_isbad_bbt(bblock))
			return 0;

		bbt[bblock >> 2] &= ~(0x01 << ((bblock << 1) % 8));
	} else {
		/* If it was good already, return success and do nothing. */
		if (!nand_isbad_bbt(gblock))
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

int nand_erase_bbt(void)
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
		rval = nand_erase_block(td->block);
		if (rval < 0) {
			nand_mark_bad_block(td->block);
			putstr("erase failed. <block ");
			putdec(td->block);
			putstr(">\r\n");
		}
	}

	if (md->block != -1) {
		rval = nand_erase_block(md->block);
		if (rval < 0) {
			nand_mark_bad_block(md->block);
			putstr("erase failed. <block ");
			putdec(md->block);
			putstr(">\r\n");
		}
	}

	putstr("\r\n");

	return 0;
}

/**
 * nand_isbad_bbt - [NAND Interface] Check if a block is bad
*/
int nand_isbad_bbt(u32 block)
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
 * nand_show_bbt - Output bad block table
*/
int nand_show_bbt()
{
	int i, rval = 0;

	for (i = 0; i < flnand.blocks_per_bank; i++) {
		rval = nand_isbad_bbt(i);
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
 * nand_has_bbt - Does nand have bad block table
*/
int nand_has_bbt()
{
	return bbt_is_created;
}
#endif
