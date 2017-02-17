/**
 * bld/firmfl.c
 *
 * Embedded Flash and Firmware Programming Utilities
 *
 * History:
 *    2005/01/31 - [Charles Chiou] created file
 *    2014/02/13 - [Anthony Ginger] Amboot V2
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


#include <amboot.h>
#include <ambhw/nand.h>
#include <ambhw/spinor.h>
#include <bldfunc.h>
#include <libfdt.h>
#include <flash/flash.h>
#include <flash/spinor/spinor_flash.h>
#include <fio/ftl_const.h>
#include <sdmmc.h>

/*===========================================================================*/
#define FWPROG_BUF_SIZE		0x10000 /* should be multiple of 2048 */

/*===========================================================================*/
static const char *FLPROG_ERR_STR[] = {
	"program ok",
	"wrong magic number",
	"invalid length",
	"invalid CRC32 code",
	"invalid version number",
	"invalid version date",
	"program image failed",
	"get ptb failed",
	"set ptb failed",
	"chunk header err",
};

/*===========================================================================*/
static u32 bld_ptb_buf_filled = 0;
static u8 bld_ptb_buf[AMBOOT_PTB_BUF_SIZE]
	__attribute__ ((aligned(32), section(".bss.noinit")));
#if defined(CONFIG_AMBOOT_BD_FDT_SUPPORT)
static void *fdt_root = PTB_DTB(bld_ptb_buf);
#endif

/*===========================================================================*/
#if defined(CONFIG_AMBOOT_ENABLE_NAND) || defined(CONFIG_AMBOOT_ENABLE_SD) || \
	defined(CONFIG_AMBOOT_ENABLE_SPINOR) || defined(CONFIG_AMBOOT_ENABLE_SPINAND)
static u8 check_buf[FWPROG_BUF_SIZE]
	__attribute__ ((aligned(32), section(".bss.noinit")));
#endif

#if defined(CONFIG_AMBOOT_ENABLE_NAND)
struct nand_oobfree oobfree_512[] = {
	{0, 5}, {11, 5}, {0, 0}};
struct nand_oobfree oobfree_2048[] = {
	{1, 7}, {13, 3}, {17, 7}, {29, 3},
	{33, 7}, {45, 3}, {49, 7}, {61, 3}, {0, 0}};

flnand_t flnand;
#endif



#if defined(CONFIG_AMBOOT_ENABLE_SPINOR)
flspinor_t flspinor;
#endif

#if defined(CONFIG_AMBOOT_ENABLE_SPINAND)
flspinand_t flspinand;
#endif

/*===========================================================================*/
static int flprog_output_progress(int percentage, void *arg)
{
	putstr("progress: ");
	putdec(percentage);
	putstr("%\r");

	return 0;
}

/*===========================================================================*/
static int flprog_ptb_check_header(u8 *pptb_buf)
{
	ptb_header_t *header;
	u32 raw_crc32, offs;

	header = PTB_HEADER(pptb_buf);

	if (header->magic != PTB_HEADER_MAGIC)
		return -1;

	/* crc32 calculates all of the data in bld_ptb_buf, only except
	 * for header->magic and header->crc32 itself */
	offs = sizeof(u32) * 2;
	raw_crc32 = crc32(pptb_buf + offs, AMBOOT_PTB_BUF_SIZE - offs);
	if (header->crc32 != raw_crc32) {
		putstr("required CRC32 = 0x");
		puthex(header->crc32);
		putstr(", calculated CRC32 = 0x");
		puthex(raw_crc32);
		putstr("\r\n");
		return -1;
	}

	return 0;
}

static int flprog_ptb_fix_header(u8 *pptb_buf)
{
	ptb_header_t *header;
	u32 offs;

	header = PTB_HEADER(pptb_buf);
	header->magic = PTB_HEADER_MAGIC;
	header->version = PTB_HEADER_VERSION;
	/* crc32 calculates all of the data in bld_ptb_buf, only except
	 * for header->magic and header->crc32 itself */
	offs = sizeof(u32) * 2;
	header->crc32 = crc32(pptb_buf + offs, AMBOOT_PTB_BUF_SIZE - offs);

	return 0;
}


#if defined(CONFIG_AMBOOT_ENABLE_NAND)
static int flprog_nand_prog_block_loop(u8 *raw_image, unsigned int raw_size,
	u32 sblk, u32 nblk, int (*output_progress)(int, void *), void *arg)
{
	int ret_val = 0;
	int firm_ok = 0;
	u32 block;
	u32 page;
	u32 percentage;
	unsigned int offset;
	unsigned int pre_offset;

	offset = 0;
	for (block = sblk; block < (sblk + nblk); block++) {
		ret_val = nand_is_bad_block(block);
		if (ret_val & NAND_ALL_BAD_BLOCK_FLAGS) {
			nand_output_bad_block(block, ret_val);
			ret_val = 0;
			continue;
		}

		ret_val = nand_erase_block(block);
		if (ret_val < 0) {
			putstr("erase failed. <block ");
			putdec(block);
			putstr(">\r\n");
			putstr("Try next block...\r\n");

			/* Marked and skipped bad block */
			ret_val = nand_mark_bad_block(block);
			if (ret_val < 0) {
				return FLPROG_ERR_PROG_IMG;
			} else {
				continue;
			}
		}

#if defined(CONFIG_NAND_ERASE_UNUSED_BLOCK)
		/* erase the unused block after program ok */
		if (firm_ok == 1)
			continue;
#endif

		pre_offset = offset;
		/* Program each page */
		for (page = 0; page < flnand.pages_per_block; page++) {
			/* Program a page */
			ret_val = nand_prog_pages(block, page, 1,
				(raw_image + offset), NULL);
			if (ret_val < 0) {
				putstr("program failed. <block ");
				putdec(block);
				putstr(", page ");
				putdec(page);
				putstr(">\r\n");
				break;
			}

			/* Read it back for verification */
			ret_val = nand_read_pages(block, page, 1,
				check_buf, NULL, 1);
			if (ret_val < 0) {
				putstr("read failed. <block ");
				putdec(block);
				putstr(", page ");
				putdec(page);
				putstr(">\r\n");
				break;
			}

			/* Compare memory content after read back */
			ret_val = memcmp(raw_image + offset,
				check_buf, flnand.main_size);
			if (ret_val != 0) {
				putstr("check failed. <block ");
				putdec(block);
				putstr(", page ");
				putdec(page);
				putstr(">\r\n");
				ret_val = -1;
				break;
			}

			offset += flnand.main_size;
			if (offset >= raw_size) {
				firm_ok = 1;
#if !defined(CONFIG_NAND_ERASE_UNUSED_BLOCK)
				block = (sblk + nblk + 1);  /* force out! */
#endif
				break;
			}
		}

		if (ret_val < 0) {
			offset = pre_offset;
			ret_val = nand_mark_bad_block(block);
			if (ret_val < 0) {
				break;
			} else {
				ret_val = nand_is_bad_block(block);
				nand_output_bad_block(block, ret_val);
				ret_val = 0;
				continue;
			}
		} else {
			if (output_progress) {
				if (offset >= raw_size)
					percentage = 100;
				else
					percentage = offset / (raw_size / 100);

				output_progress(percentage, NULL);
			}
		}
	}

	if ((ret_val < 0) || (firm_ok == 0)) {
		ret_val = FLPROG_ERR_PROG_IMG;
	}

	return ret_val;
}

static void flprog_ptb_nand_fix_meta(flpart_meta_t *pmeta)
{
	int i;

	for (i = 0; i <= HAS_IMG_PARTS; i++) {
		memzero(pmeta->part_info[i].name, PART_NAME_LEN);
		memcpy(pmeta->part_info[i].name, get_part_str(i),
			strlen(get_part_str(i)));
		pmeta->part_info[i].dev = get_part_dev(i);
		pmeta->part_info[i].sblk = flnand.sblk[i];
		pmeta->part_info[i].nblk = flnand.nblk[i];
	}
	for (; i < PART_MAX; i++) {
		memzero(pmeta->part_info[i].name, PART_NAME_LEN);
		pmeta->part_info[i].dev = PART_DEV_AUTO;
		pmeta->part_info[i].sblk = 0;
		pmeta->part_info[i].nblk = 0;
	}

	pmeta->magic = PTB_META_MAGIC;
}

static int flprog_ptb_nand_read(u8 *pptb_buf)
{
	int ret_val = -1;
	u32 blk, pages;
#if !defined(CONFIG_AMBOOT_SEARCH_PTB)
	u32 sblk, nblk;

	sblk = flnand.sblk[PART_PTB];
	nblk = flnand.nblk[PART_PTB];
	/* skip bad block */
	for (blk = sblk; blk < (sblk + nblk); blk++) {
		if (!nand_is_bad_block(blk))
			break;
	}
	if (blk >= (sblk + nblk)) {
		goto flprog_ptb_nand_read_exit;
	}

	pages = (AMBOOT_PTB_BUF_SIZE + flnand.main_size - 1) / flnand.main_size;
	ret_val = nand_read_pages(blk, 0, pages, pptb_buf, NULL, 1);
	if (ret_val < 0)
		goto flprog_ptb_nand_read_exit;

	ret_val = flprog_ptb_check_header(pptb_buf);
	if (ret_val < 0)
		goto flprog_ptb_nand_read_exit;
#else
	/* search PTB starting from the 4th block:
	 * 1st blk must be BST, and 2nd/3rd blk must be BLD */
	blk = 3;
	while (blk++ < 1000 && ret_val < 0) {
		if (nand_is_bad_block(blk))
			continue;

		pages = (AMBOOT_PTB_BUF_SIZE + flnand.main_size - 1) / flnand.main_size;
		ret_val = nand_read_pages(blk, 0, pages, pptb_buf, NULL, 1);
		if (ret_val < 0)
			goto flprog_ptb_nand_read_exit;

		ret_val = flprog_ptb_check_header(pptb_buf);
	}
#endif
	flprog_ptb_nand_fix_meta(PTB_META(pptb_buf));

flprog_ptb_nand_read_exit:
	return ret_val;
}

static int flprog_ptb_nand_write(u8 *pptb_buf)
{
	int ret_val;

	flprog_ptb_nand_fix_meta(PTB_META(pptb_buf));

	flprog_ptb_fix_header(pptb_buf);

	ret_val = flprog_nand_prog_block_loop(pptb_buf, AMBOOT_PTB_BUF_SIZE,
		flnand.sblk[PART_PTB], flnand.nblk[PART_PTB], NULL, NULL);

	return ret_val;
}
#endif

#if defined(CONFIG_AMBOOT_ENABLE_SD)
static int flprog_sm_prog_sector_loop(u8 *raw_image, unsigned int raw_size,
	u32 ssec, u32 nsec, int (*output_progress)(int, void *), void *arg)
{
	int ret_val = 0;
	u32 remain;
	u32 sector = 0;
	u32 sectors;
	u32 offset = 0;
	u32 percentage = 0;

	BUG_ON(FWPROG_BUF_SIZE % sdmmc.sector_size);

	/* Program image into the flash */
	for (sector = ssec; sector < (ssec + nsec); sector += sectors) {
		remain = raw_size - offset;
		if (remain >= FWPROG_BUF_SIZE)
			sectors = FWPROG_BUF_SIZE / sdmmc.sector_size;
		else
			sectors = (remain + sdmmc.sector_size - 1) / sdmmc.sector_size;

		/* Program the sector */
		ret_val = sdmmc_write_sector(sector, sectors, raw_image + offset);
		if (ret_val < 0) {
			putstr("writing sector ");
			putdec(sector);
			putstr(", sectors ");
			putdec(sectors);
			putstr(" ... failed in program\r\n");
		}

		/* Read it back for verification */
		ret_val = sdmmc_read_sector(sector, sectors, check_buf);
		if (ret_val < 0) {
			putstr("writing sector ");
			putdec(sector);
			putstr(", sectors ");
			putdec(sectors);
			putstr(" ... failed in read back\r\n");
		}

		/* Compare memory content after read back */
		ret_val = memcmp(raw_image + offset, check_buf, sectors * sdmmc.sector_size);
		if (ret_val != 0) {
			putstr("writing sector ");
			putdec(sector);
			putstr(", sectors ");
			putdec(sectors);
			putstr(" ... failed in verify\r\n");
			ret_val = -1;
		}

		offset += sectors * sdmmc.sector_size;

		/* display progress */
		if (output_progress) {
			if (offset >= raw_size)
				percentage = 100;
			else
				percentage = offset / (raw_size / 100);

			output_progress(percentage, NULL);
		}

		if (offset >= raw_size) {
			break;
		}
	}

	return ret_val;
}

static void flprog_ptb_sm_fix_meta(flpart_meta_t *pmeta)
{
	int i;

	for (i = 0; i <= HAS_IMG_PARTS; i++) {
		memzero(pmeta->part_info[i].name, PART_NAME_LEN);
		memcpy(pmeta->part_info[i].name, get_part_str(i),
			strlen(get_part_str(i)));
		pmeta->part_info[i].dev = get_part_dev(i);
		pmeta->part_info[i].sblk = sdmmc.ssec[i];
		pmeta->part_info[i].nblk = sdmmc.nsec[i];
	}
	for (; i < PART_MAX; i++) {
		memzero(pmeta->part_info[i].name, PART_NAME_LEN);
		pmeta->part_info[i].dev = PART_DEV_AUTO;
		pmeta->part_info[i].sblk = 0;
		pmeta->part_info[i].nblk = 0;
	}

	pmeta->magic = PTB_META_MAGIC;
}

static int flprog_ptb_sm_read(u8 *pptb_buf)
{
	int ret_val;

	BUG_ON(AMBOOT_PTB_BUF_SIZE % sdmmc.sector_size);

	ret_val = sdmmc_read_sector(sdmmc.ssec[PART_PTB],
			AMBOOT_PTB_BUF_SIZE / sdmmc.sector_size, pptb_buf);
	if (ret_val < 0)
		return ret_val;

	ret_val = flprog_ptb_check_header(pptb_buf);
	if (ret_val < 0)
		return ret_val;

	flprog_ptb_sm_fix_meta(PTB_META(pptb_buf));

	return 0;
}

static int flprog_ptb_sm_write(u8 *pptb_buf)
{
	int ret_val;

	flprog_ptb_sm_fix_meta(PTB_META(pptb_buf));

	flprog_ptb_fix_header(pptb_buf);

	ret_val = flprog_sm_prog_sector_loop(pptb_buf, AMBOOT_PTB_BUF_SIZE,
		sdmmc.ssec[PART_PTB], sdmmc.nsec[PART_PTB], NULL, NULL);

	return ret_val;
}
#endif

#if defined(CONFIG_AMBOOT_ENABLE_SPINOR)
static int flprog_spinor_prog_sector_loop(u8 *raw_image, u32 raw_size,
	u32 ssec, u32 nsec, int (*output_progress)(int, void *), void *arg)
{
	u32 sector, address, len, offset, percentage;
	int residual, rval = 0;

	offset = 0;
	residual = raw_size;

	for (sector = ssec; sector < ssec + nsec; sector++) {
		rval = spinor_erase_sector(sector);
		if (rval < 0) {
			putstr("erase failed. <sector ");
			putdec(sector);
			putstr(">\r\n");
			break;
		}

		address = sector * flspinor.sector_size;
		len = min(residual, flspinor.sector_size);

		rval = spinor_write_data(address, raw_image + offset, len);
		if (rval < 0) {
			putstr("program failed. <sector ");
			putdec(sector);
			putstr(">\r\n");
			break;
		}

		offset += len;
		residual -= len;

		if (offset >= raw_size)
			break;

		if (output_progress) {
			if (offset >= raw_size) {
				percentage = 100;
			} else {
				percentage = offset / (raw_size / 100);
				output_progress(percentage, NULL);
			}
		}
	}

	return rval < 0 ? FLPROG_ERR_PROG_IMG : 0;
}

static int flprog_spinor_prog_bst(u8 *raw_image, u32 raw_size,
	u32 ssec, u32 nsec, int (*output_progress)(int, void *), void *arg)
{
	int address, rval;

	/* bst is always programmed in the first sector */
	if (ssec != 0)
		return -1;

	if (AMBOOT_BST_FIXED_SIZE > flspinor.sector_size) {
		putstr("flspinor sector size is too small\r\n");
		return -1;
	}

	rval = spinor_erase_sector(ssec);
	if (rval < 0) {
		putstr("erase failed. <sector 0>\r\n");
		return rval;
	}

	rval = spinor_write_boot_header();
	if (rval < 0)
		return rval;

	address = 0 + sizeof(struct spinor_boot_header);
	rval = spinor_write_data(address, raw_image, raw_size);
	if (rval < 0) {
		putstr("program failed. <sector 0>\r\n");
		return rval;
	}

	if (output_progress)
		output_progress(100, NULL);

	return 0;
}

static void flprog_ptb_spinor_fix_meta(flpart_meta_t *pmeta)
{
	int i;

	for (i = 0; i <= HAS_IMG_PARTS; i++) {
		memzero(pmeta->part_info[i].name, PART_NAME_LEN);
		memcpy(pmeta->part_info[i].name, get_part_str(i),
			strlen(get_part_str(i)));
		pmeta->part_info[i].dev = get_part_dev(i);
		pmeta->part_info[i].sblk = flspinor.ssec[i];
		pmeta->part_info[i].nblk = flspinor.nsec[i];
	}
	for (; i < PART_MAX; i++) {
		memzero(pmeta->part_info[i].name, PART_NAME_LEN);
		pmeta->part_info[i].dev = PART_DEV_AUTO;
		pmeta->part_info[i].sblk = 0;
		pmeta->part_info[i].nblk = 0;
	}

	pmeta->magic = PTB_META_MAGIC;
}

static int flprog_ptb_spinor_write(u8 *pptb_buf)
{
	int ret_val = -1;

	flprog_ptb_spinor_fix_meta(PTB_META(pptb_buf));

	flprog_ptb_fix_header(pptb_buf);

	ret_val = flprog_spinor_prog_sector_loop(pptb_buf, AMBOOT_PTB_BUF_SIZE,
		flspinor.ssec[PART_PTB], flspinor.nsec[PART_PTB], NULL, NULL);

	return ret_val;
}

static int flprog_ptb_spinor_read(u8 *pptb_buf)
{
	u32 address;
	int ret_val;

	address = flspinor.ssec[PART_PTB] * flspinor.sector_size;

	ret_val = spinor_read_data(address, pptb_buf, AMBOOT_PTB_BUF_SIZE);
	if (ret_val < 0)
		return ret_val;

	ret_val = flprog_ptb_check_header(pptb_buf);
	if (ret_val < 0)
		return ret_val;

	flprog_ptb_spinor_fix_meta(PTB_META(pptb_buf));

	return 0;
}
#endif

#if defined(CONFIG_AMBOOT_ENABLE_SPINAND)
static int flprog_spinand_prog_block_loop(u8 *raw_image, u32 raw_size,
	u32 sblk, u32 nblk, int (*output_progress)(int, void *), void *arg)
{
	int ret_val = 0;
	int firm_ok = 0;
	u32 block;
	u32 page;
	u32 percentage;
	unsigned int offset;
	unsigned int pre_offset;
	offset = 0;
	for (block = sblk; block < (sblk + nblk); block++) {
		ret_val = spinand_is_bad_block(block);
		if (ret_val & NAND_ALL_BAD_BLOCK_FLAGS) {
			spinand_output_bad_block(block, ret_val);
			ret_val = 0;
			continue;
		}

		ret_val = spinand_erase_block(block);
		if (ret_val < 0) {
			putstr("erase failed. <block ");
			putdec(block);
			putstr(">\r\n");
			putstr("Try next block...\r\n");

			/* Marked and skipped bad block */
			ret_val = spinand_mark_bad_block(block);
			if (ret_val < 0) {
				return FLPROG_ERR_PROG_IMG;
			} else {
				continue;
			}
		}

		/* erase the unused block after program ok */
		if (firm_ok == 1)
			continue;

		pre_offset = offset;
		/* Program each page */
		for (page = 0; page < flspinand.pages_per_block; page++) {
			/* Program a page */
			ret_val = spinand_prog_pages(block, page, 1,
				(raw_image + offset));
			if (ret_val < 0) {
				putstr("program failed. <block ");
				putdec(block);
				putstr(", page ");
				putdec(page);
				putstr(">\r\n");
				break;
			}
			/* Read it back for verification */
			ret_val = spinand_read_pages(block, page, 1,
				check_buf, 1);
			if (ret_val < 0) {
				putstr("read failed. <block ");
				putdec(block);
				putstr(", page ");
				putdec(page);
				putstr(">\r\n");
				break;
			}

			/* Compare memory content after read back */
			ret_val = memcmp(raw_image + offset,
				check_buf, flspinand.main_size);
			if (ret_val != 0) {
				putstr("check failed. <block ");
				putdec(block);
				putstr(", page ");
				putdec(page);
				putstr(">\r\n");
				ret_val = -1;
				break;
			}
			offset += flspinand.main_size;
			if (offset >= raw_size) {
				firm_ok = 1;
				break;
			}
		}
		if (ret_val < 0) {
			offset = pre_offset;
			ret_val = spinand_mark_bad_block(block);
			if (ret_val < 0) {
				break;
			} else {
				ret_val = spinand_is_bad_block(block);
				spinand_output_bad_block(block, ret_val);
				ret_val = 0;
				continue;
			}
		} else {
			if (output_progress) {
				if (offset >= raw_size)
					percentage = 100;
				else
					percentage = offset / (raw_size / 100);

				output_progress(percentage, NULL);
			}
		}
	}

	if ((ret_val < 0) || (firm_ok == 0)) {
		ret_val = FLPROG_ERR_PROG_IMG;
	}

	return ret_val;
}
static u32 spinand_bst_crc = 0;
static int flprog_spinand_prog_bst(u8 *raw_image, u32 raw_size,
	u32 sblk, u32 nblk, int (*output_progress)(int, void *), void *arg)
{
	int address;
	u32 bst_size;

	struct spinor_boot_header header;
	int rval;

	memzero(&header, sizeof(struct spinor_boot_header));

	/* SPINOR_LENGTH_REG */
	header.data_len = AMBOOT_BST_FIXED_SIZE;
	header.clk_divider = 20;
	header.dummy_len = 0;
	header.addr_len = 3;
	header.cmd_len = 1;
	/* SPINOR_CTRL_REG */
	header.read_en = 1;
	header.write_en = 0;
	header.rsvd0 = 0;
	header.rxlane = 1;
	header.data_lane = 0x1;
	header.addr_lane = 0x0;
	header.cmd_lane = 0x0;
	header.rsvd1 = 0;
	header.data_dtr = 0;
	header.dummy_dtr = 0;
	header.addr_dtr = 0;
	header.cmd_dtr = 0;
	/* SPINOR_CFG_REG */
	header.rxsampdly = 1;
	header.rsvd2 = 0;
	header.chip_sel = (~(1 << (SPINOR_FLASH_CHIP_SEL))) & 0xff;
	header.hold_timing = 0;
	header.rsvd3 = 0;
	header.hold_pin = 3;
	header.flow_ctrl = 1;
	/* SPINOR_CMD_REG */
	header.cmd = SPINOR_CMD_READ;
	/* SPINOR_ADDRHI_REG */
	header.addr_hi = 0x0;
	/* SPINOR_ADDRLO_REG */
	header.addr_lo = 0x0 + sizeof(struct spinor_boot_header);

	/* bst is always programmed in the first block/page */
	if (sblk != 0)
		return -1;

	if (AMBOOT_BST_FIXED_SIZE > flspinand.main_size) {
		putstr("BST size is too big for spinand page size\r\n");
		return -1;
	}

	rval = spinand_erase_block(sblk);
	if (rval < 0) {
		putstr("erase failed. <sector 0>\r\n");
		return rval;
	}

#if 0
	spinand_bst_crc = crc32(raw_image, AMBOOT_PTB_BUF_SIZE);
	putstr("spi nand bst image crc32 value is 0x");
	puthex(spinand_bst_crc);
	putstr("\r\n");
#endif
	memcpy(check_buf, &header, sizeof(struct spinor_boot_header));

	address = 0 + sizeof(struct spinor_boot_header);
	bst_size = max((AMBOOT_BST_FIXED_SIZE - sizeof(struct spinor_boot_header)),raw_size);
	memcpy(check_buf + address, raw_image, bst_size);
	rval = spinand_program_page(0, 0, check_buf, AMBOOT_BST_FIXED_SIZE);
	if (rval < 0) {
		putstr("program failed. <block 0>\r\n");
		return rval;
	}

	if (output_progress)
		output_progress(100, NULL);

	return 0;
}

static void flprog_ptb_spinand_fix_meta(flpart_meta_t *pmeta)
{
	int i;

	for (i = 0; i <= HAS_IMG_PARTS; i++) {
		memzero(pmeta->part_info[i].name, PART_NAME_LEN);
		memcpy(pmeta->part_info[i].name, get_part_str(i),
			strlen(get_part_str(i)));
		pmeta->part_info[i].dev = get_part_dev(i);
		pmeta->part_info[i].sblk = flspinand.sblk[i];
		pmeta->part_info[i].nblk = flspinand.nblk[i];
	}
	for (; i < PART_MAX; i++) {
		memzero(pmeta->part_info[i].name, PART_NAME_LEN);
		pmeta->part_info[i].dev = PART_DEV_AUTO;
		pmeta->part_info[i].sblk = 0;
		pmeta->part_info[i].nblk = 0;
	}

	pmeta->magic = PTB_META_MAGIC;
}

static int flprog_ptb_spinand_write(u8 *pptb_buf)
{
	int ret_val = -1;

	flprog_ptb_spinand_fix_meta(PTB_META(pptb_buf));

	flprog_ptb_fix_header(pptb_buf);

	ret_val = flprog_spinand_prog_block_loop(pptb_buf, AMBOOT_PTB_BUF_SIZE,
		flspinand.sblk[PART_PTB], flspinand.nblk[PART_PTB], NULL, NULL);

	return ret_val;
}

static int flprog_ptb_spinand_read(u8 *pptb_buf)
{
	int ret_val;
	u32 blk, pages, sblk, nblk;

	sblk = flspinand.sblk[PART_PTB];
	nblk = flspinand.nblk[PART_PTB];
	/* skip bad block */
	for (blk = sblk; blk < (sblk + nblk); blk++) {
		if (!spinand_is_bad_block(blk))
			break;
	}
	if (blk >= (sblk + nblk)) {
		return -1;
	}
	pages = (AMBOOT_PTB_BUF_SIZE + flspinand.main_size - 1) / flspinand.main_size;

	ret_val = spinand_read_pages(blk, 0, pages, pptb_buf, 1);
	if (ret_val < 0)
		return ret_val;

	ret_val = flprog_ptb_check_header(pptb_buf);
	if (ret_val < 0)
		return ret_val;

	flprog_ptb_spinand_fix_meta(PTB_META(pptb_buf));

	return 0;
}
#endif

static int flprog_ptb_read_data(void)
{
	int ret_val = -1;
	u32 part_dev;
	flpart_meta_t *pmeta;

	memzero(bld_ptb_buf, AMBOOT_PTB_BUF_SIZE);

	part_dev = get_part_dev(PART_PTB);

#if defined(CONFIG_AMBOOT_ENABLE_NAND)
	if ((ret_val < 0) && (part_dev & PART_DEV_NAND)) {
		ret_val = flprog_ptb_nand_read(bld_ptb_buf);
	}
#endif
#if defined(CONFIG_AMBOOT_ENABLE_SD)
	if ((ret_val < 0) && (part_dev & PART_DEV_EMMC)) {
		ret_val = flprog_ptb_sm_read(bld_ptb_buf);
	}
#endif
#if defined(CONFIG_AMBOOT_ENABLE_SPINOR)
	if ((ret_val < 0) && (part_dev & PART_DEV_SPINOR)) {
		ret_val = flprog_ptb_spinor_read(bld_ptb_buf);
	}
#endif
#if defined(CONFIG_AMBOOT_ENABLE_SPINAND)
	if ((ret_val < 0) && (part_dev & PART_DEV_SPINAND)) {
		ret_val = flprog_ptb_spinand_read(bld_ptb_buf);
	}
#endif
	if (ret_val < 0)
		memzero(bld_ptb_buf, AMBOOT_PTB_BUF_SIZE);
	else {
		pmeta = PTB_META(bld_ptb_buf);
		pmeta->magic = PTB_META_MAGIC;
	}

	bld_ptb_buf_filled = 1;

	return ret_val;
}

static int flprog_ptb_write_data(void)
{
	int ret_val = -1;
	u32 part_dev;

	part_dev = get_part_dev(PART_PTB);
#if defined(CONFIG_AMBOOT_ENABLE_NAND)
	if (part_dev & PART_DEV_NAND) {
		ret_val = flprog_ptb_nand_write(bld_ptb_buf);
	}
#endif
#if defined(CONFIG_AMBOOT_ENABLE_SD)
	if (part_dev & PART_DEV_EMMC) {
		ret_val = flprog_ptb_sm_write(bld_ptb_buf);
	}
#endif
#if defined(CONFIG_AMBOOT_ENABLE_SPINOR)
	if (part_dev & PART_DEV_SPINOR) {
		ret_val = flprog_ptb_spinor_write(bld_ptb_buf);
	}
#endif
#if defined(CONFIG_AMBOOT_ENABLE_SPINAND)
	if (part_dev & PART_DEV_SPINAND) {
		ret_val = flprog_ptb_spinand_write(bld_ptb_buf);
	}
#endif
	if (ret_val < 0) {
		putstr("Set ptb failed\r\n");
		ret_val = FLPROG_ERR_PTB_SET;
	}

	return ret_val;
}

static void flprog_ptb_default_param(fldev_t *dev)
{
	dev->magic = FLPART_MAGIC;

	dev->usbdl_mode = 0;
	dev->splash_id = 0;

#if defined(AMBOOT_DEV_AUTO_BOOT)
	dev->auto_boot = 1;
#endif
#if defined(AMBOOT_DEFAULT_SN)
	strncpy(dev->sn, AMBOOT_DEFAULT_SN, (sizeof(dev->sn) - 1));
#endif
}

/*===========================================================================*/
int flprog_get_part_table(flpart_table_t *pptb)
{
	int ret_val = 0;

	if (!bld_ptb_buf_filled) {
		ret_val = flprog_ptb_read_data();
	}

	if (ret_val >= 0) {
		memcpy(pptb, PTB_TABLE(bld_ptb_buf), sizeof(flpart_table_t));
	} else {
		putstr("Get ptb failed\r\n");
		memzero(pptb, sizeof(flpart_table_t));
		ret_val = FLPROG_ERR_PTB_GET;
	}

	return ret_val;
}

int flprog_set_part_table(flpart_table_t *pptb)
{
	int i, ret_val = 0;

	if (!bld_ptb_buf_filled) {
		putstr("Read PTB before write\r\n");
		ret_val = flprog_ptb_read_data();
		if (ret_val < 0) {
			return ret_val;
		}
	}

	if (pptb->dev.magic != FLPART_MAGIC) {
		memzero(&pptb->dev, sizeof(pptb->dev));
		flprog_ptb_default_param(&pptb->dev);
	}

	for (i = 0; i < HAS_IMG_PARTS; i++) {
		if (pptb->part[i].magic != FLPART_MAGIC) {
			memzero(&pptb->part[i], sizeof(flpart_t));
			pptb->part[i].magic = FLPART_MAGIC;
		}
	}

	memcpy(PTB_TABLE(bld_ptb_buf), pptb, sizeof(flpart_table_t));

	return flprog_ptb_write_data();
}

/*===========================================================================*/
#if defined(CONFIG_AMBOOT_BD_FDT_SUPPORT)
int flprog_get_dtb(u8 *dtb)
{
	int ret_val = 0;

	if (!bld_ptb_buf_filled) {
		ret_val = flprog_ptb_read_data();
		if (ret_val < 0) {
			goto flprog_get_dtb_exit;
		}
	}

	if (fdt_check_header(fdt_root) == 0)
		memcpy(dtb, fdt_root, fdt_totalsize(fdt_root));
	else
		ret_val = FLPROG_ERR_MAGIC;

flprog_get_dtb_exit:
	return ret_val;
}

int flprog_set_dtb(u8 *dtb, u32 len, u32 write)
{
	int ret_val = 0;

	if (!dtb || fdt_check_header(dtb) < 0 || fdt_totalsize(dtb) != len) {
		ret_val = FLPROG_ERR_PROG_IMG;
		goto flprog_set_dtb_exit;
	}

	if (PTB_HEADER_SIZE + PTB_TABLE_SIZE + PTB_META_SIZE + len >
			AMBOOT_PTB_BUF_SIZE) {
		putstr("DTB ERROR: out of buffer, 0x");
		puthex(len);
		putstr("\r\n");
		ret_val = FLPROG_ERR_LENGTH;
		goto flprog_set_dtb_exit;
	}

	if (!bld_ptb_buf_filled) {
		ret_val = flprog_ptb_read_data();
		if (ret_val < 0) {
			goto flprog_set_dtb_exit;
		}
	}

	memcpy(fdt_root, dtb, len);
	if (write) {
		ret_val = flprog_ptb_write_data();
	}

flprog_set_dtb_exit:
	return ret_val;
}
#endif

/*===========================================================================*/
static int flprog_validate_image(u8 *image, unsigned int len)
{
	partimg_header_t *header;
	u32 raw_crc32;

	K_ASSERT(image != NULL);
	K_ASSERT(len > sizeof(partimg_header_t));

	header = (partimg_header_t *)image;
	if (header->magic != PARTHD_MAGIC) {
		putstr("wrong magic!\r\n");
		return FLPROG_ERR_MAGIC;
	}
	if (header->ver_num == 0x0) {
		putstr("invalid version!\r\n");
		return FLPROG_ERR_VER_NUM;
	}
	if (header->ver_date == 0x0) {
		putstr("invalid date!\r\n");
		return FLPROG_ERR_VER_DATE;
	}

	putstr("verifying image crc ... ");
	raw_crc32 = crc32((u8 *)(image + sizeof(partimg_header_t)),
		header->img_len);
	if (raw_crc32 != header->crc32) {
		putstr("0x");
		puthex(raw_crc32);
		putstr(" != 0x");
		puthex(header->crc32);
		putstr(" failed!\r\n");
		return FLPROG_ERR_CRC;
	} else {
		putstr("done\r\n");
	}

	return FLPROG_OK;
}

#if defined(CONFIG_AMBOOT_ENABLE_NAND)
static int flprog_write_partition_nand(int pid, u8 *image, unsigned int len)
{
	int ret_val;
	partimg_header_t *header;
	u8 *raw_image;
	u32 sblk = 0;
	u32 nblk = 0;

	putstr(" into NAND\r\n");

	header = (partimg_header_t *)image;
	raw_image = (image + sizeof(partimg_header_t));
	sblk = flnand.sblk[pid];
	nblk = flnand.nblk[pid];

	ret_val = flprog_nand_prog_block_loop(raw_image,
				header->img_len, sblk, nblk,
				flprog_output_progress, NULL);

	return ret_val;
}
#endif

#if defined(CONFIG_AMBOOT_ENABLE_SD)
static int flprog_write_partition_sd(int pid, u8 *image, unsigned int len)
{
	int ret_val = 0;
	partimg_header_t *header;
	u8 *raw_image;

	putstr(" into SD\r\n");

	header = (partimg_header_t *)image;
	raw_image = (image + sizeof(partimg_header_t));

	if (pid == PART_BST) {
		ret_val = sdmmc_set_emmc_boot_info();
		if (ret_val < 0) {
			goto flprog_write_partition_sd_exit;
		}
	}

	ret_val = flprog_sm_prog_sector_loop(raw_image,
			header->img_len, sdmmc.ssec[pid], sdmmc.nsec[pid],
			flprog_output_progress, NULL);
	if (ret_val < 0) {
		goto flprog_write_partition_sd_exit;
	}
	if (pid == PART_BST) {
		ret_val = sdmmc_set_emmc_normal();
	}

flprog_write_partition_sd_exit:
	return ret_val;
}
#endif
#if defined(CONFIG_AMBOOT_ENABLE_SPINOR)
static int flprog_write_partition_spinor(int pid, u8 *image, unsigned int len)
{
	partimg_header_t *header;
	u8 *raw_image;
	u32 raw_size, ssec, nsec;
	int ret_val;

	putstr(" into SPINOR\r\n");

	header = (partimg_header_t *)image;
	raw_image = (image + sizeof(partimg_header_t));
	raw_size = header->img_len;

	ssec = flspinor.ssec[pid];
	nsec = flspinor.nsec[pid];

	if (pid == PART_BST) {
		ret_val = flprog_spinor_prog_bst(raw_image, raw_size,
				ssec, nsec, flprog_output_progress, NULL);
	} else {
		ret_val = flprog_spinor_prog_sector_loop(raw_image, raw_size,
				ssec, nsec, flprog_output_progress, NULL);
	}

	return ret_val;
}
#endif
#if defined(CONFIG_AMBOOT_ENABLE_SPINAND)
static int flprog_write_partition_spinand(int pid, u8 *image, unsigned int len)
{
	partimg_header_t *header;
	u8 *raw_image;
	u32 raw_size = 0;
	u32 sblk = 0;
	u32 nblk = 0;
	int ret_val;

	putstr(" into SPINAND\r\n");

	header = (partimg_header_t *)image;
	raw_image = (image + sizeof(partimg_header_t));
	raw_size = header->img_len;

	sblk = flspinand.sblk[pid];
	nblk = flspinand.nblk[pid];

	if (pid == PART_BST) {
		ret_val = flprog_spinand_prog_bst(raw_image, raw_size,
				sblk, nblk, flprog_output_progress, NULL);
	} else {
		ret_val = flprog_spinand_prog_block_loop(raw_image, raw_size,
				sblk, nblk, flprog_output_progress, NULL);
	}

	return ret_val;
}
#endif

int flprog_write_partition(int pid, u8 *image, unsigned int len)
{
	int ret_val = 0;
	partimg_header_t *header;
	int part_size[HAS_IMG_PARTS];
	u32 part_dev;

	header = (partimg_header_t *)image;
	if (pid >= HAS_IMG_PARTS || pid == PART_PTB) {
		ret_val = -1;
		goto flprog_write_partition_exit;
	}

	/* First, validate the image */
	ret_val = flprog_validate_image(image, len);
	if (ret_val < 0) {
		goto flprog_write_partition_exit;
	}

	if (pid == PART_BST) {
		/* Special check for BST which must fit in 2KB for FIO FIFO */
		if (header->img_len > AMBOOT_BST_FIXED_SIZE) {
			putstr("bst length is bigger than ");
			putdec(AMBOOT_BST_FIXED_SIZE);
			putstr("\r\n");
			ret_val = FLPROG_ERR_LENGTH;
			goto flprog_write_partition_exit;
		}
	}
	get_part_size(part_size);
	if (header->img_len > part_size[pid]) {
		putstr(get_part_str(pid));
		putstr(" length is bigger than partition\r\n");
		ret_val = FLPROG_ERR_LENGTH;
		goto flprog_write_partition_exit;
	}

	putstr("program ");
	putstr(get_part_str(pid));
	part_dev = get_part_dev(pid);
#if defined(CONFIG_AMBOOT_ENABLE_NAND)
	if (part_dev & PART_DEV_NAND) {
		ret_val = flprog_write_partition_nand(pid, image, len);
	}
#endif
#if defined(CONFIG_AMBOOT_ENABLE_SD)
	if (part_dev & PART_DEV_EMMC) {
		ret_val = flprog_write_partition_sd(pid, image, len);
	}
#endif
#if defined(CONFIG_AMBOOT_ENABLE_SPINOR)
	if (part_dev & PART_DEV_SPINOR) {
		ret_val = flprog_write_partition_spinor(pid, image, len);
	}
#endif
#if defined(CONFIG_AMBOOT_ENABLE_SPINAND)
	if (part_dev & PART_DEV_SPINAND) {
		ret_val = flprog_write_partition_spinand(pid, image, len);
		if (pid == PART_BST)
			header->crc32 = spinand_bst_crc;
	}
#endif
	/* Update the PTB's entry */
	if (ret_val == 0) {
		flpart_table_t ptb;
		flpart_t part;
		int i, sanitize = 0;

		part.crc32 = header->crc32;
		part.ver_num = header->ver_num;
		part.ver_date = header->ver_date;
		part.img_len = header->img_len;
		part.mem_addr = header->mem_addr;
		part.flag = header->flag;
		part.magic = FLPART_MAGIC;

		ret_val = flprog_get_part_table(&ptb);
		if (ret_val < 0) {
			sanitize = 1;
		} else {
			for (i = 0; i < HAS_IMG_PARTS; i++) {
				if (ptb.part[i].magic != FLPART_MAGIC) {
					sanitize = 1;
					break;
				}
			}
		}

		if (sanitize) {
			putstr("PTB on flash is outdated, sanitizing ...\r\n");
			memset(&ptb, 0x0, sizeof(ptb));
			for (i = 0; i < HAS_IMG_PARTS; i++) {
				ptb.part[i].magic = FLPART_MAGIC;
			}
			flprog_ptb_default_param(&ptb.dev);
		}
		memcpy(&ptb.part[pid], &part, sizeof(flpart_t));

		ret_val = flprog_set_part_table(&ptb);
		if (ret_val < 0) {
			putstr("unable to update PTB!\r\n");
		}
	}

flprog_write_partition_exit:
	return ret_val;
}

/*===========================================================================*/
#if defined(CONFIG_AMBOOT_ENABLE_NAND)
static int flprog_erase_partition_nand(int pid)
{
	int ret_val = 0;
	u32 sblk = 0;
	u32 nblk = 0;
	u32 block;

	if (pid == HAS_IMG_PARTS) {
		sblk = 0;
		nblk = flnand.blocks_per_bank * flnand.banks;
	} else {
		sblk = flnand.sblk[pid];
		nblk = flnand.nblk[pid];
	}

	for (block = sblk; block < (sblk + nblk); block++) {
		ret_val = nand_is_bad_block(block);
		if (ret_val & NAND_FW_BAD_BLOCK_FLAGS) {
			nand_output_bad_block(block, ret_val);
			continue;
		}

		ret_val = nand_erase_block(block);
		if (ret_val < 0) {
			nand_mark_bad_block(block);
			putstr(" failed! <block ");
			putdec(block);
			putstr(">\r\n");
		}
	}

	return ret_val;
}
#endif

#if defined(CONFIG_AMBOOT_ENABLE_SD)
static int flprog_erase_partition_sd(int pid)
{
	int ret_val = 0;
	int i;
	u32 smssec = 0, smnsec = 0;
	u32 p, wsecs;

	if (pid == HAS_IMG_PARTS) {
		smssec = 0;
		for (i = 0; i < HAS_IMG_PARTS; i++)
			smnsec += sdmmc.nsec[i];
	} else {
		smssec = sdmmc.ssec[pid];
		smnsec = sdmmc.nsec[pid];
	}

	for (i = 0; i < smnsec; i += SDMMC_SEC_CNT) {
		if (smnsec > (SDMMC_SEC_CNT + i)) {
			wsecs = SDMMC_SEC_CNT;
		} else {
			wsecs = (smnsec - i);
		}
		ret_val = sdmmc_erase_sector(smssec + i, wsecs);
		if (ret_val < 0) {
			putstr("failed!\r\n");
		}

		if ((i + wsecs) >= smnsec) {
			p = 100;
		} else {
			p = (i + wsecs) * 100 / smnsec;
		}
		flprog_output_progress(p, NULL);
	}

	return ret_val;
}
#endif

#if defined(CONFIG_AMBOOT_ENABLE_SPINOR)
static int flprog_erase_partition_spinor(int pid)
{
	u32 ssec, nsec, sec, percentage;
	int ret_val = 0;

	if (pid == HAS_IMG_PARTS) {
		ssec = 0;
		nsec = flspinor.sectors_per_chip;
	} else {
		ssec = flspinor.ssec[pid];
		nsec = flspinor.nsec[pid];
	}

	for (sec = ssec; sec < (ssec + nsec); sec++) {
		ret_val = spinor_erase_sector(sec);
		if (ret_val < 0) {
			putstr(" failed! <sec ");
			putdec(sec);
			putstr(">\r\n");
		} else {
			percentage = (sec - ssec) * 100 / nsec;
			flprog_output_progress(percentage, NULL);
		}
	}

	putstr("\r\n");

	return ret_val;
}
#endif

#if defined(CONFIG_AMBOOT_ENABLE_SPINAND)
static int flprog_erase_partition_spinand(int pid)
{
	int ret_val = 0;
	u32 sblk = 0;
	u32 nblk = 0;
	u32 block;

	if (pid == HAS_IMG_PARTS) {
		sblk = 0;
		nblk = flspinand.chip_size / flspinand.block_size;
	} else {
		sblk = flspinand.sblk[pid];
		nblk = flspinand.nblk[pid];
	}

	for (block = sblk; block < (sblk + nblk); block++) {
		ret_val = spinand_is_bad_block(block);
		if (ret_val & NAND_FW_BAD_BLOCK_FLAGS) {
			spinand_output_bad_block(block, ret_val);
			continue;
		}

		ret_val = spinand_erase_block(block);
		if (ret_val < 0) {
			spinand_mark_bad_block(block);
			putstr(" failed! <block ");
			putdec(block);
			putstr(">\r\n");
		}
	}

	return ret_val;
}
#endif

int flprog_erase_partition(int pid)
{
	int ret_val = -1;
	u32 part_dev;

	putstr("erase ");
	if (pid < HAS_IMG_PARTS) {
		part_dev = get_part_dev(pid);
		putstr(get_part_str(pid));
	} else if (pid == HAS_IMG_PARTS) {
		part_dev = get_part_dev(PART_PTB);
		putstr("all");
	} else {
		putstr("!");
		goto flprog_erase_partition_exit;
	}
	putstr(" ... \r\n");

#if defined(CONFIG_AMBOOT_ENABLE_NAND)
	if (part_dev & PART_DEV_NAND) {
		ret_val = flprog_erase_partition_nand(pid);
	}
#endif
#if defined(CONFIG_AMBOOT_ENABLE_SD)
	if (part_dev & PART_DEV_EMMC) {
		ret_val = flprog_erase_partition_sd(pid);
	}
#endif
#if defined(CONFIG_AMBOOT_ENABLE_SPINOR)
	if (part_dev & PART_DEV_SPINOR) {
		ret_val = flprog_erase_partition_spinor(pid);
	}
#endif
#if defined(CONFIG_AMBOOT_ENABLE_SPINAND)
	if (part_dev & PART_DEV_SPINAND) {
		ret_val = flprog_erase_partition_spinand(pid);
	}
#endif
	if ((ret_val == 0) && (pid != HAS_IMG_PARTS) && (pid != PART_PTB)) {
		flpart_table_t ptb;
		flpart_t part;

		part.crc32 = 0xffffffff;
		part.ver_num = 0xffffffff;
		part.ver_date = 0xffffffff;
		part.img_len = 0xffffffff;
		part.mem_addr = 0xffffffff;
		part.flag = 0xffffffff;
		part.magic = 0xffffffff;
		ret_val = flprog_get_part_table(&ptb);
		if (ret_val < 0) {
			putstr("Can't get ptb\r\n");
		}

		memcpy(&ptb.part[pid], &part, sizeof(flpart_t));
		ret_val = flprog_set_part_table(&ptb);
		if (ret_val < 0) {
			putstr("unable to update PTB!\r\n");
		}

		putstr("done\r\n");
	}

flprog_erase_partition_exit:
	return ret_val;
}

/*===========================================================================*/
void output_header(partimg_header_t *header, u32 len)
{
	putstr("\tcrc32:\t\t0x");
	puthex(header->crc32);
	putstr("\r\n");
	putstr("\tver_num:\t");
	putdec(header->ver_num >> 16);
	putstr(".");
	putdec(header->ver_num & 0xffff);
	putstr("\r\n");
	putstr("\tver_date:\t");
	putdec(header->ver_date >> 16);
	putstr("/");
	putdec((header->ver_date >> 8) & 0xff);
	putstr("/");
	putdec(header->ver_date & 0xff);
	putstr("\r\n");
	putstr("\timg_len:\t");
	putdec(header->img_len);
	putstr("\r\n");
	putstr("\tmem_addr:\t0x");
	puthex(header->mem_addr);
	putstr("\r\n");
	putstr("\tflag:\t\t0x");
	puthex(header->flag);
	putstr("\r\n");
	putstr("\tmagic:\t\t0x");
	puthex(header->magic);
	putstr("\r\n");
}

void output_failure(int errcode)
{
	if (errcode == 0) {
		putstr("\r\n");
		putstr(FLPROG_ERR_STR[errcode]);
		putstr("\r\n");
	} else if (errcode < 0) {
		putstr("\r\nfailed: (");
		putstr(FLPROG_ERR_STR[-errcode]);
		putstr(")!\r\n");
	} else {
		putstr("\r\nfailed: (");
		putstr(FLPROG_ERR_STR[errcode]);
		putstr(")!\r\n");
	}
}

void output_report(const char *name, u32 flag)
{
	if ((flag & FWPROG_RESULT_FLAG_LEN_MASK) == 0) {
		return;
	}

	putstr(name);
	putstr(":\t");

	if ((flag & FWPROG_RESULT_FLAG_CODE_MASK) == 0) {
		putstr("success\r\n");
	} else {
		putstr("*** FAILED! ***\r\n");
	}
}

static void flprog_show_meta_detail(flpart_meta_t *pmeta)
{
	int i;

	if (pmeta->magic != PTB_META_MAGIC) {
		putstr("meta appears damaged...\r\n");
		return;
	}

	putstr("\r\n");
	for (i = 0; i < PART_MAX; i++) {
		putstr(pmeta->part_info[i].name);
		putstr("\t sblk: ");
		putdec(pmeta->part_info[i].sblk);
		putstr("\t nblk: ");
		putdec(pmeta->part_info[i].nblk);
		putstr("\t dev:");
		if (pmeta->part_info[i].dev & PART_DEV_NAND) {
			putstr(" NAND");
		}
		if (pmeta->part_info[i].dev & PART_DEV_EMMC) {
			putstr(" EMMC");
		}
		if (pmeta->part_info[i].dev & PART_DEV_SPINOR) {
			putstr(" SPI NOR");
		}
		if (pmeta->part_info[i].dev & PART_DEV_SPINAND) {
			putstr(" SPI NAND");
		}
		putstr("\r\n");
	}
	putstr("\r\n");
}

void flprog_show_meta(void)
{
	u32 part_dev;
	flpart_meta_t meta;

	memcpy(&meta, PTB_META(bld_ptb_buf), sizeof(flpart_meta_t));
	//putstr("Current meta:\r\n");
	//flprog_show_meta_detail(&meta);

	part_dev = get_part_dev(PART_PTB);
#if defined(CONFIG_AMBOOT_ENABLE_NAND)
	if (part_dev & PART_DEV_NAND) {
		putstr("NAND meta:\r\n");
		flprog_ptb_nand_fix_meta(&meta);
		flprog_show_meta_detail(&meta);
		putstr("\r\n");
	}
#endif
#if defined(CONFIG_AMBOOT_ENABLE_SD)
	if (part_dev & PART_DEV_EMMC) {
		putstr("SD meta:\r\n");
		flprog_ptb_sm_fix_meta(&meta);
		flprog_show_meta_detail(&meta);
		putstr("\r\n");
	}
#endif
#if defined(CONFIG_AMBOOT_ENABLE_SPINOR)
	if (part_dev & PART_DEV_SPINOR) {
		putstr("SPINOR meta:\r\n");
		flprog_ptb_spinor_fix_meta(&meta);
		flprog_show_meta_detail(&meta);
		putstr("\r\n");
	}
#endif
#if defined(CONFIG_AMBOOT_ENABLE_SPINAND)
	if (part_dev & PART_DEV_SPINAND) {
		putstr("SPINAND meta:\r\n");
		flprog_ptb_spinand_fix_meta(&meta);
		flprog_show_meta_detail(&meta);
		putstr("\r\n");
	}
#endif
	putstr("\r\n");
}

