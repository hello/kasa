/**
 * bld/cmd_spinand.c
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
#include <ambhw/uart.h>
#include <fio/ftl_const.h>
#include <ambhw/spinor.h>


static int cmd_spinand_dump(int argc, char *argv[])
{

	u32 block, page, i, j, total_pages, page_addr, pages;
	u8 *rbuf;
	int rval = 0;

	total_pages = flspinand.chip_size/flspinand.main_size;
	if (strtou32(argv[0], &page_addr) < 0) {
		putstr("invalid page address!\r\n");
		rval = -1;
		goto done;
	}

	if (argc == 1 || strtou32(argv[1], &pages) < 0)
		pages = 1;

	if (page_addr + pages > total_pages) {
		putstr("page_addr = 0x");
		puthex(page_addr);
		putstr(", pages = 0x");
		puthex(pages);
		putstr(", Overflow!\r\n");
		rval = -1;
		goto done;
	}

	rbuf = (u8*)bld_hugebuf_addr;

	for (i = 0; i < pages; i++) {
		block = (page_addr + i) / flspinand.pages_per_block;
		page = (page_addr + i) % flspinand.pages_per_block;

		if (spinand_is_bad_block(block)) {
			putstr("block ");
			putdec(block);
			putstr(" is a bad block\r\n");
			continue;
		}

		rval = spinand_read_pages(block, page, 1, rbuf, 1);
		if (rval < 0) {
			putstr("nand_read_pages failed\r\n");
			goto done;
		}

		putstr("PAGE[");
		putdec(page_addr + i);
		putstr("] main data:\r\n");
		for (j = 0; j < flspinand.main_size; j++) {
			putstr(" ");
			putbyte(rbuf[j]);
			if ((j+1) % 32 == 0)
				putstr("\r\n");
		}
		putstr("\r\n");
	}

done:
	putstr("done!\r\n");
	return rval;
}

static int cmd_spinand_verify_data(u8 *origin, u8 *data, u32 len)
{
	int err_cnt = 0;
	u32 i;

	for (i = 0; i < len; i++) {
		if (origin[i] != data[i]){
			err_cnt++;
		}
	}

	return err_cnt;
}

static int cmd_spinand_erase(int argc, char *argv[])
{
	u32 start_block, block, blocks, total_blocks;;
	int ret_val = -1;
	int i = 0;

	if (argc < 2 )
		return -1;

	total_blocks = flspinand.chip_size/flspinand.block_size;
	strtou32(argv[0], &start_block);
	strtou32(argv[1], &blocks);
	for (i = 0, block = start_block; i < blocks; i++, block++) {
		if (uart_poll())
			break;
		if (block >= total_blocks)
			break;

		ret_val = spinand_erase_block(block);
		putchar('.');
		if ((i & 0xf) == 0xf) {
			putchar(' ');
			putdec(i);
			putchar('/');
			putdec(blocks);
			putstr(" (");
			putdec(i * 100 / blocks);
			putstr("%)\t\t\r");
		}
		if (ret_val < 0) {
			putstr("\r\nfailed at block ");
			putdec(block);
			putstr("\r\n");
		}
	}

	putstr("\r\ndone!\r\n");

	return ret_val;
}

static int cmd_spinand_verify(int argc, char *argv[])
{
	int ret_val = -1;
	u32 start_block, block, blocks;
	u32 block_size;
	u32 enable_ecc;
	u32 bb_num = 0;
	u8 *wbuf, *rbuf;
	int i;

	if (argc == 0)
		return -1;

	putstr("running spinand stress test ...\r\n");
	putstr("press any key to terminate!\r\n");

	block_size = flspinand.block_size;

	wbuf = (u8 *)bld_hugebuf_addr;
	rbuf = (u8 *)(bld_hugebuf_addr + flspinand.block_size);

	for (i = 0; i < block_size; i++) {
		wbuf[i] = rand() / 256;
	}

	if (strcmp(argv[0], "all") == 0) {
		start_block = 0;
		blocks = flspinand.chip_size / flspinand.block_size;
	} else if (strcmp(argv[0], "free") == 0) {
		start_block = 0;
		for (i = 0; i < HAS_IMG_PARTS; i++)
			start_block += flspinand.nblk[i];

		blocks = flspinand.chip_size / flspinand.block_size - start_block;
	} else if (strtou32(argv[0], &start_block) == 0) {
		blocks = 1;
	} else {
		return -1;
	}

	putstr("\r\nstart_block = ");
	putdec(start_block);
	putstr(", blocks = ");
	putdec(blocks);
	putstr("\r\n\r\n");

	if ((argc >= 2) && (strcmp(argv[1], "no_ecc") == 0))
		enable_ecc = 0;
	else
		enable_ecc = 1;

	for (i = 0, block = start_block; i < blocks; i++, block++) {
		if (uart_poll())
			break;

		ret_val = spinand_is_bad_block(block);
		if (ret_val & NAND_FW_BAD_BLOCK_FLAGS) {
			spinand_output_bad_block(block, ret_val);
			bb_num ++;
			continue;
		}

		ret_val = spinand_erase_block(block);
		if (ret_val < 0) {
			spinand_mark_bad_block(block);
			putstr("\r\nspinand_erase_block failed\r\n");
			bb_num ++;
			goto done;
		}

		ret_val = spinand_prog_pages(block, 0,
			flspinand.pages_per_block, wbuf);
		if (ret_val < 0) {
			spinand_mark_bad_block(block);
			putstr("\r\nspinand_prog_pages failed\r\n");
			bb_num ++;
			goto done;
		}

		ret_val = spinand_read_pages(block, 0,
			flspinand.pages_per_block, rbuf, enable_ecc);
		if (ret_val < 0) {
			spinand_mark_bad_block(block);
			putstr("\r\nspinand_read_pages failed\r\n");
			bb_num ++;
			goto done;
		}

		ret_val = spinand_erase_block(block);
		if (ret_val < 0) {
			spinand_mark_bad_block(block);
			putstr("\r\nnand_erase_block failed\r\n");
			bb_num ++;
			goto done;
		}

		ret_val = cmd_spinand_verify_data(wbuf, rbuf, block_size);
		if (ret_val != 0) {
			spinand_mark_bad_block(block);
			putstr("\r\nnand verify data error count ");
			putdec(ret_val);
			ret_val = -1;
			bb_num ++;
			goto done;
		}

		putchar('.');

		if ((i & 0xf) == 0xf) {
			putchar(' ');
			putdec(i);
			putchar('/');
			putdec(blocks);
			putstr(" (");
			putdec(i * 100 / blocks);
			putstr("%)\t\t\r");
		}

done:
		if (ret_val < 0) {
			putstr("\r\nfailed at block ");
			putdec(block);
			putstr("\r\n");
		}
	}

	putstr("\r\ntotal bad blocks: ");
	putdec(bb_num);
	putstr("\r\n");

	putstr("\r\ndone!\r\n");
	ret_val = 0;

	return ret_val;
}

static int cmd_spinand_speed(int argc, char *argv[])
{
	u32 start_block, blocks, size, time, i;
	u8 *buf;
	int rval = 0;

	if (argc == 0 || !strcmp(argv[0], "free")) {
		start_block = 0;
		for (i = 0; i < HAS_IMG_PARTS; i++)
			start_block += flspinand.nblk[i];

		blocks = flspinand.chip_size / flspinand.block_size- start_block;
	} else {
		if (strtou32(argv[0], &start_block) < 0)
			return -1;
		if (strtou32(argv[1], &blocks) < 0)
			return -1;
	}

	if (start_block == 0 || blocks == 0) {
		putstr("invalid block!\r\n");
		return -1;
	}

	putstr("test nand speed, start_block = ");
	putdec(start_block);
	putstr(", blocks = ");
	putdec(blocks);
	putstr("\r\n");

	buf = (u8 *)bld_hugebuf_addr;

	putstr("erase...");
	size = 0;
	rct_timer_reset_count();
	for (i = start_block; i < start_block + blocks; i++) {
		rval = spinand_erase_block(i);
		if (rval < 0) {
			spinand_mark_bad_block(i);
			putstr("nand_erase_block failed\r\n");
			continue;
		}
		size += flspinand.block_size;
	}

	time = rct_timer_get_count();

	putstr(" done!\r\n");
	putstr("    size = ");
	putdec(size);
	putstr(" Bytes, time = ");
	putdec(time);
	putstr("ms, speed = ");
	putdec(size / time);
	putstr(" KByte/s\r\n");

	putstr("write...");
	size = 0;
	rct_timer_reset_count();
	for (i = start_block; i < start_block + blocks; i++) {
		if (spinand_is_bad_block(i)) {
			putstr("block ");
			putdec(i);
			putstr(" is a bad block\r\n");
			continue;
		}

		rval = spinand_prog_pages(i, 0, flspinand.pages_per_block, buf);
		if (rval < 0) {
			spinand_mark_bad_block(i);
			putstr("spinand_prog_pages failed\r\n");
			break;
		}

		size += flspinand.block_size;
	}

	time = rct_timer_get_count();

	putstr(" done!\r\n");
	putstr("    size = ");
	putdec(size);
	putstr(" Bytes, time = ");
	putdec(time);
	putstr("ms, speed = ");
	putdec(size / time);
	putstr(" KByte/s\r\n");

	putstr("read...");
	size = 0;
	rct_timer_reset_count();
	for (i = start_block; i < start_block + blocks; i++) {
		if (spinand_is_bad_block(i)) {
			putstr("block ");
			putdec(i);
			putstr(" is a bad block\r\n");
			continue;
		}

		rval = spinand_read_pages(i, 0,
			flspinand.pages_per_block, buf, 1);
		if (rval < 0) {
			spinand_mark_bad_block(i);
			putstr("nand_read_pages failed\r\n");
			break;
		}

		size += flspinand.block_size;
	}

	time = rct_timer_get_count();

	putstr(" done!\r\n");
	putstr("    size = ");
	putdec(size);
	putstr(" Bytes, time = ");
	putdec(time);
	putstr("ms, speed = ");
	putdec(size / time);
	putstr(" KByte/s\r\n");

	for (i = start_block; i < start_block + blocks; i++) {
		rval = spinand_erase_block(i);
		if (rval < 0) {
			spinand_mark_bad_block(i);
			putstr("nand_erase_block failed\r\n");
			continue;
		}
	}

	return rval;
}

/*===========================================================================*/
static int cmd_spinand(int argc, char *argv[])
{
	int i;

	if (!strcmp(argv[1], "show")) {
		if (strcmp(argv[2], "partition") == 0) {
		putstr("chip size: ");
		putdec(flspinand.chip_size);
		putstr(", block size: ");
		putdec(flspinand.block_size);
		putstr(", page size: ");
		putdec(flspinand.main_size);
		putstr("\r\n");
		putstr("page per chip: ");
		putdec(flspinand.pages_per_block);
		putstr("\r\n");
		for (i = 0; i < HAS_IMG_PARTS; i++) {
			putstr(get_part_str(i));
			putstr(" partition block: ");
			putdec(flspinand.sblk[i]);
			putstr(" - ");
			putdec(flspinand.sblk[i] + flspinand.nblk[i]);
			putstr("\r\n");
		}
		return 0;
#if defined(CONFIG_SPINAND_USE_FLASH_BBT)
		} else if (strcmp(argv[2], "bbt") == 0) {
			return spinand_show_bbt();
#endif
		}
	} else if (!strcmp(argv[1], "dump")) {
		return cmd_spinand_dump(argc - 2, &argv[2]);
	} else if (!strcmp(argv[1], "verify")) {
		return cmd_spinand_verify(argc - 2, &argv[2]);
	} else if (!strcmp(argv[1], "speed")) {
		return cmd_spinand_speed(argc - 2, &argv[2]);
#if defined(CONFIG_SPINAND_USE_FLASH_BBT)
	} else if ((strcmp(argv[1], "erase") == 0) && (strcmp(argv[2], "bbt") == 0)) {
		return spinand_erase_bbt();
#endif
	} else if ((strcmp(argv[1], "erase") == 0) && (argc == 4)) {
		return cmd_spinand_erase(argc - 2, &argv[2]);
	} else {
		return -2;
	}
	return 0;
}

/*===========================================================================*/
static char help_spinand[] =
	"spinand show partition|bbt\r\n"
	"spinand erase bbt\r\n"
	"spinand dump PAGE [PAGES]\r\n"
	"spinand verify free|all|BLOCK \r\n"
	"spinand erase BLOCK [BLOCKS]\r\n"
	"spinand speed [free]|[BLOCK BLOCKS]\r\n";

__CMDLIST(cmd_spinand, "spinand", help_spinand);

