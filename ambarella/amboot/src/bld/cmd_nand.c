/**
 * bld/cmd_nand.c
 *
 * History:
 *    2008/11/19 - [Chien-Yang Chen] created file
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
#include <ambhw/uart.h>
#include <fio/ftl_const.h>

/*===========================================================================*/
static int cmd_nand_show_bb(void)
{
	int ret_val;
	u32 *init_bad_tbl = (u32 *)bld_hugebuf_addr;
	u32 *late_bad_tbl = (u32 *)(bld_hugebuf_addr + 2048);
	u32 *other_bad_tbl = (u32 *)(bld_hugebuf_addr + 2048 * 2);
	u32 block, blocks, i;
	u32 init_bad = 0, late_bad = 0, other_bad = 0, total_bad = 0;

	blocks = flnand.blocks_per_bank * flnand.banks;

	for (block = 0; block < blocks; block++) {
		ret_val = nand_is_bad_block(block);
		if (ret_val != 0) {
			if (ret_val == NAND_INITIAL_BAD_BLOCK) {
				init_bad_tbl[init_bad++] = block;
			} else if (ret_val == NAND_LATE_DEVEL_BAD_BLOCK) {
				late_bad_tbl[late_bad++] = block;
			} else {
				other_bad_tbl[other_bad++] = block;
			}

			total_bad++;

			if (init_bad >= 2048 || late_bad >= 2048 ||
			    other_bad >= 2048)
				break;
		}
	}

	if (init_bad)
		putstr("----- Initial bad blocks -----");
	for (i = 0; i < init_bad; i++) {
		if (i % 5)
			putstr("\t");
		else
			putstr("\r\n");

		putdec(init_bad_tbl[i]);
	}

	if (late_bad)
		putstr("\r\n----- Late developed bad blocks -----");
	for (i = 0; i < late_bad; i++) {
		if (i % 5)
			putstr("\t");
		else
			putstr("\r\n");

		putdec(late_bad_tbl[i]);
	}

	if (other_bad)
		putstr("\r\n----- Other bad blocks -----");
	for (i = 0; i < other_bad; i++) {
		if (i % 5)
			putstr("\t");
		else
			putstr("\r\n");

		putdec(other_bad_tbl[i]);
	}

	if (total_bad) {
		putstr("\r\nTotal bad blocks: ");
		putdec(total_bad);
		putstr("\r\n");
	} else {
		putstr("\r\nNo bad blocks\r\n");
	}

	return 0;
}

/*===========================================================================*/
static int cmd_nand_verify_data(u8* origin, u8* data, u32 len)
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

/*===========================================================================*/
static int cmd_nand_rclm(int argc, char *argv[])
{
	int ret_val = -1;
	u32 block, blocks, page;
	u32 block_size, spa_size;
	u8 *wbuf, *rbuf, *swbuf, *srbuf;
	int i, bb_type, c = 0;

	blocks = flnand.blocks_per_bank * flnand.banks;
	block_size = flnand.pages_per_block * flnand.main_size;
	spa_size = flnand.pages_per_block * flnand.spare_size;

	wbuf = (u8 *)bld_hugebuf_addr;
	rbuf = (u8 *)(bld_hugebuf_addr +
		(flnand.pages_per_block * flnand.main_size));
	swbuf = (u8 *)(bld_hugebuf_addr +
		(flnand.pages_per_block * flnand.main_size * 2));
	srbuf = (u8 *)(bld_hugebuf_addr +
		(flnand.pages_per_block * flnand.main_size * 2) +
		(flnand.pages_per_block * flnand.spare_size));
	for (i = 0; i < block_size; i++) {
		wbuf[i] = rand() / 256;
	}
	for (i = 0; i < spa_size; i++) {
		swbuf[i] = rand() / 256;
	}

	if (strcmp(argv[0], "init") == 0) {
		bb_type = NAND_INITIAL_BAD_BLOCK;
	} else if (strcmp(argv[0], "late") == 0) {
		bb_type = NAND_LATE_DEVEL_BAD_BLOCK;
	} else if (strcmp(argv[0], "other") == 0) {
		bb_type = NAND_OTHER_BAD_BLOCK;
	} else if (strcmp(argv[0], "all") == 0) {
		bb_type = NAND_INITIAL_BAD_BLOCK |
			  NAND_LATE_DEVEL_BAD_BLOCK |
			  NAND_OTHER_BAD_BLOCK;
	} else {
		return ret_val;
	}

	putstr("\r\n Start to reclaim bad blocks...\r\n");

	for (block = 0; block < blocks; block++) {
		if (uart_poll()){
			c = uart_read();
			if (c == 0x20 || c == 0x0d || c == 0x1b) {
				break;
			}
		}

		ret_val = nand_is_bad_block(block);
		if ((ret_val & bb_type)== 0) {
			continue;
		}

		/* Start to reclaim bad block. */
		ret_val = nand_erase_block(block);
		if (ret_val < 0) {
			nand_mark_bad_block(block);
			putstr("nand_erase_block failed\r\n");
			goto done;
		}

		ret_val = nand_prog_pages(block, 0,
			flnand.pages_per_block, wbuf, NULL);
		if (ret_val < 0) {
			nand_mark_bad_block(block);
			putstr("nand_prog_pages failed\r\n");
			goto done;
		}

		ret_val = nand_read_pages(block, 0,
			flnand.pages_per_block, rbuf, NULL, 1);
		if (ret_val < 0) {
			nand_mark_bad_block(block);
			putstr("nand_read_pages failed\r\n");
			goto done;
		}

		ret_val = nand_erase_block(block);
		if (ret_val < 0) {
			nand_mark_bad_block(block);
			putstr("nand_erase_block failed\r\n");
			goto done;
		}

		for (page = 0; page < flnand.pages_per_block; page++) {
			ret_val = nand_prog_spare(block, page, 1, swbuf);
			if (ret_val < 0) {
				nand_mark_bad_block(block);
				putstr("nand_prog_spare failed\r\n");
				goto done;
			}

			ret_val = nand_read_spare(block, page, 1, srbuf);
			if (ret_val < 0) {
				nand_mark_bad_block(block);
				putstr("nand_read_spare failed\r\n");
				goto done;
			}

			swbuf += flnand.spare_size;
			srbuf += flnand.spare_size;
		}

		ret_val = nand_erase_block(block);
		if (ret_val < 0) {
			nand_mark_bad_block(block);
			putstr("nand_erase_block failed\r\n");
			goto done;
		}

		ret_val = cmd_nand_verify_data(wbuf, rbuf, block_size);
		if (ret_val != 0) {
			nand_mark_bad_block(block);
			putstr("nand verify data error count\r\n");
			putdec(ret_val);
			ret_val = -1;
			goto done;
		}

		swbuf -= spa_size;
		srbuf -= spa_size;

		ret_val = cmd_nand_verify_data(swbuf, srbuf, spa_size);
		if (ret_val != 0) {
			nand_mark_bad_block(block);
			putstr("nand verify data error count\r\n");
			putdec(ret_val);
			ret_val = -1;
			goto done;
		}

done:
		if (ret_val == 0) {
			putstr("block ");
			putdec(block);
			putstr(" is a good block\r\n");
#if defined(CONFIG_NAND_USE_FLASH_BBT)
			nand_update_bbt(0, block);
#endif
		} else {
			putstr("\r\nblock ");
			putdec(block);
			putstr(" is a bad block\r\n");
		}
	}

	putstr("done!\r\n");
	ret_val = 0;

	return ret_val;
}

/*===========================================================================*/
static int cmd_nand_verify(int argc, char *argv[])
{
	int ret_val = -1;
	u32 start_block, block, blocks, page;
	u32 block_size, spa_size;
	u32 enable_ecc;
	u32 bb_num = 0;
	u8 *wbuf, *rbuf, *swbuf, *srbuf;
	int i;

	if (argc == 0)
		return -1;

	putstr("running nand stress test ...\r\n");
	putstr("press any key to terminate!\r\n");

	block_size = flnand.pages_per_block * flnand.main_size;
	spa_size = flnand.pages_per_block * flnand.spare_size;

	wbuf = (u8 *)bld_hugebuf_addr;
	rbuf = (u8 *)(bld_hugebuf_addr +
		(flnand.pages_per_block * flnand.main_size));
	swbuf = (u8 *)(bld_hugebuf_addr +
		(flnand.pages_per_block * flnand.main_size * 2));
	srbuf = (u8 *)(bld_hugebuf_addr +
		(flnand.pages_per_block * flnand.main_size * 2) +
		(flnand.pages_per_block * flnand.spare_size));
	for (i = 0; i < block_size; i++) {
		wbuf[i] = rand() / 256;
	}
	for (i = 0; i < spa_size; i++) {
		swbuf[i] = rand() / 256;
	}

	if (strcmp(argv[0], "all") == 0) {
		start_block = 0;
		blocks = flnand.blocks_per_bank * flnand.banks;
	} else if (strcmp(argv[0], "free") == 0) {
		start_block = 0;
		for (i = 0; i < HAS_IMG_PARTS; i++)
			start_block += flnand.nblk[i];

		blocks = flnand.blocks_per_bank * flnand.banks - start_block;
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

		ret_val = nand_is_bad_block(block);
		if (ret_val & NAND_FW_BAD_BLOCK_FLAGS) {
			nand_output_bad_block(block, ret_val);
			bb_num ++;
			continue;
		}

		ret_val = nand_erase_block(block);
		if (ret_val < 0) {
			nand_mark_bad_block(block);
			putstr("\r\nnand_erase_block failed\r\n");
			bb_num ++;
			goto done;
		}

		ret_val = nand_prog_pages(block, 0,
			flnand.pages_per_block, wbuf, NULL);
		if (ret_val < 0) {
			nand_mark_bad_block(block);
			putstr("\r\nnand_prog_pages failed\r\n");
			bb_num ++;
			goto done;
		}

		ret_val = nand_read_pages(block, 0,
			flnand.pages_per_block, rbuf, NULL, enable_ecc);
		if (ret_val < 0) {
			nand_mark_bad_block(block);
			putstr("\r\nnand_read_pages failed\r\n");
			bb_num ++;
			goto done;
		}

		ret_val = nand_erase_block(block);
		if (ret_val < 0) {
			nand_mark_bad_block(block);
			putstr("\r\nnand_erase_block failed\r\n");
			bb_num ++;
			goto done;
		}

		for (page = 0; page < flnand.pages_per_block; page++) {
			ret_val = nand_prog_spare(block, page, 1, swbuf);
			if (ret_val < 0) {
				nand_mark_bad_block(block);
				putstr("\r\nnand_prog_spare failed\r\n");
				bb_num ++;
				goto done;
			}

			ret_val = nand_read_spare(block, page, 1, srbuf);
			if (ret_val < 0) {
				nand_mark_bad_block(block);
				putstr("\r\nnand_read_spare failed\r\n");
				bb_num ++;
				goto done;
			}

			swbuf += flnand.spare_size;
			srbuf += flnand.spare_size;
		}

		ret_val = nand_erase_block(block);
		if (ret_val < 0) {
			nand_mark_bad_block(block);
			putstr("\r\nnand_erase_block failed\r\n");
			bb_num ++;
			goto done;
		}

		ret_val = cmd_nand_verify_data(wbuf, rbuf, block_size);
		if (ret_val != 0) {
			nand_mark_bad_block(block);
			putstr("\r\nnand verify data error count ");
			putdec(ret_val);
			ret_val = -1;
			bb_num ++;
			goto done;
		}

		swbuf -= spa_size;
		srbuf -= spa_size;

		ret_val = cmd_nand_verify_data(swbuf, srbuf, spa_size);
		if (ret_val != 0) {
			nand_mark_bad_block(block);
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

static int cmd_nand_dump(int argc, char *argv[])
{
	u32 page_addr, pages, total_pages;
	u32 block, page;
	u32 i, j, enable_ecc;
	u8 *rbuf, *srbuf;
	int rval = 0;

	total_pages = flnand.pages_per_block * flnand.blocks_per_bank * flnand.banks;

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

	if (strcmp(argv[1], "no_ecc") == 0 || strcmp(argv[2], "no_ecc") == 0)
		enable_ecc = 0;
	else
		enable_ecc = 1;

	rbuf = (u8*)bld_hugebuf_addr;
	srbuf = (u8*)(bld_hugebuf_addr + flnand.main_size);

	for (i = 0; i < pages; i++) {
		block = (page_addr + i) / flnand.pages_per_block;
		page = (page_addr + i) % flnand.pages_per_block;

		if (nand_is_bad_block(block)) {
			putstr("block ");
			putdec(block);
			putstr(" is a bad block\r\n");
			continue;
		}

		rval = nand_read_pages(block, page, 1, rbuf, NULL, enable_ecc);
		if (rval == 1) {
			putstr("BCH code corrected (0x");
			puthex(block);
			putstr(")!\n\r");
		} else if (rval < 0) {
			nand_mark_bad_block(block);
			putstr("nand_read_pages failed\r\n");
			goto done;
		}

		putstr("PAGE[");
		putdec(page_addr + i);
		putstr("] main data:\r\n");
		for (j = 0; j < flnand.main_size; j++) {
			putstr(" ");
			putbyte(rbuf[j]);
			if ((j+1) % 32 == 0)
				putstr("\r\n");
		}
		putstr("\r\n");

		rval = nand_read_spare(block, page, 1, srbuf);
		if (rval < 0) {
			nand_mark_bad_block(block);
			putstr("nand_read_spare failed\r\n");
			goto done;
		}

		putstr("PAGE[");
		putdec(page_addr + i);
		putstr("] spare data:\r\n");
		for (j = 0; j < flnand.spare_size; j++) {
			putstr(" ");
			putbyte(srbuf[j]);
			if ((j+1) % 32 == 0)
				putstr("\r\n");
		}
		putstr("\r\n\r\n");
	}

done:
	putstr("done!\r\n");
	return rval;
}

static int cmd_nand_prog(int argc, char *argv[])
{
	u32 page_addr, pages, total_pages;
	u32 block, page, i;
	u32 enable_ecc, bitflip;
	u8 *wbuf;
	int rval = 0;

	total_pages = flnand.pages_per_block * flnand.blocks_per_bank * flnand.banks;

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

	if (strcmp(argv[1], "no_ecc") == 0) {
		enable_ecc = 0;
		if (argc > 2 && strtou32(argv[2], &bitflip) < 0) {
			rval = -1;
			goto done;
		}
	} else if (strcmp(argv[2], "no_ecc") == 0) {
		enable_ecc = 0;
		if (argc > 3 && strtou32(argv[3], &bitflip) < 0) {
			rval = -1;
			goto done;
		}
	} else {
		enable_ecc = 1;
		bitflip = 0;
	}

	wbuf = (u8*)bld_hugebuf_addr;
	memset(wbuf, 0xff, flnand.main_size * pages);
	if (enable_ecc == 0) {
		/* create bitflip manually on the first bytes data */
		for (i = 0; i < bitflip; i++)
			wbuf[0] &= ~(1 << i);
	}

	for (i = 0; i < pages; i++) {
		block = (page_addr + i) / flnand.pages_per_block;
		page = (page_addr + i) % flnand.pages_per_block;

		if (nand_is_bad_block(block)) {
			putstr("block ");
			putdec(block);
			putstr(" is a bad block\r\n");
			continue;
		}

		if (enable_ecc) {
			rval = nand_prog_pages(block, page, 1,
					wbuf + flnand.main_size * i, NULL);
			if (rval < 0) {
				putstr("block ");
				putdec(block);
				putstr(": nand_erase_block failed\r\n");
				goto done;
			}
		} else {
			rval = nand_prog_pages_noecc(block, page, 1,
					wbuf + flnand.main_size * i);
			if (rval < 0) {
				putstr("block ");
				putdec(block);
				putstr(": nand_erase_block failed\r\n");
				goto done;
			}
		}
	}

done:
	putstr("done!\r\n");
	return rval;
}

static int cmd_nand_speed(int argc, char *argv[])
{
	u32 start_block, blocks, size, time, i;
	u8 *buf;
	int rval = 0;

	if (argc == 0 || !strcmp(argv[0], "free")) {
		start_block = 0;
		for (i = 0; i < HAS_IMG_PARTS; i++)
			start_block += flnand.nblk[i];

		blocks = flnand.blocks_per_bank * flnand.banks - start_block;
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
		rval = nand_erase_block(i);
		if (rval < 0) {
			nand_mark_bad_block(i);
			putstr("nand_erase_block failed\r\n");
			continue;
		}
		size += flnand.block_size;
	}

	time = rct_timer_get_count();

	putstr(" done!\r\n");
	putstr("    size = ");
	putdec(size);
	putstr(" Bytes, time = ");
	putdec(time);
	putstr("ms, speed = ");
	putdec((u64)size * 1000/ time);
	putstr(" Byte/s\r\n");

	putstr("write...");
	size = 0;
	rct_timer_reset_count();
	for (i = start_block; i < start_block + blocks; i++) {
		if (nand_is_bad_block(i)) {
			putstr("block ");
			putdec(i);
			putstr(" is a bad block\r\n");
			continue;
		}

		rval = nand_prog_pages(i, 0, flnand.pages_per_block, buf, NULL);
		if (rval < 0) {
			nand_mark_bad_block(i);
			putstr("nand_prog_pages failed\r\n");
			break;
		}

		size += flnand.block_size;
	}

	time = rct_timer_get_count();

	putstr(" done!\r\n");
	putstr("    size = ");
	putdec(size);
	putstr(" Bytes, time = ");
	putdec(time);
	putstr("ms, speed = ");
	putdec((u64)size * 1000/ time);
	putstr(" Byte/s\r\n");

	putstr("read...");
	size = 0;
	rct_timer_reset_count();
	for (i = start_block; i < start_block + blocks; i++) {
		if (nand_is_bad_block(i)) {
			putstr("block ");
			putdec(i);
			putstr(" is a bad block\r\n");
			continue;
		}

		rval = nand_read_pages(i, 0,
			flnand.pages_per_block, buf, NULL, 1);
		if (rval < 0) {
			nand_mark_bad_block(i);
			putstr("nand_read_pages failed\r\n");
			break;
		}

		size += flnand.block_size;
	}

	time = rct_timer_get_count();

	putstr(" done!\r\n");
	putstr("    size = ");
	putdec(size);
	putstr(" Bytes, time = ");
	putdec(time);
	putstr("ms, speed = ");
	putdec((u64)size * 1000/ time);
	putstr(" Byte/s\r\n");

	for (i = start_block; i < start_block + blocks; i++) {
		rval = nand_erase_block(i);
		if (rval < 0) {
			nand_mark_bad_block(i);
			putstr("nand_erase_block failed\r\n");
			continue;
		}
	}

	return rval;
}

/*===========================================================================*/
static int cmd_nand(int argc, char *argv[])
{
	u32 start_block, block, blocks, total_blocks;
	int i, ret_val;

	total_blocks = flnand.blocks_per_bank * flnand.banks;

	if (strcmp(argv[1], "show") == 0) {
		if (strcmp(argv[2], "bb") == 0) {
			return cmd_nand_show_bb();
		} else if (strcmp(argv[2], "partition") == 0) {
			putstr("main size: ");
			putdec(flnand.main_size);
			putstr("\r\n");
			putstr("pages per block: ");
			putdec(flnand.pages_per_block);
			putstr("\r\n");
			for (i = 0; i < HAS_IMG_PARTS; i++) {
				putstr(get_part_str(i));
				putstr(" partition blocks: [");
				putdec(flnand.sblk[i]);
				putstr(" - ");
				putdec(flnand.sblk[i] + flnand.nblk[i]);
				putstr(")\r\n");
			}
			return 0;
#if defined(CONFIG_NAND_USE_FLASH_BBT)
		} else if (strcmp(argv[2], "bbt") == 0) {
			return nand_show_bbt();
#endif
		}
	} else if (strcmp(argv[1], "rclm") == 0) {
		return cmd_nand_rclm(1, &argv[2]);
	} else if (strcmp(argv[1], "verify") == 0) {
		return cmd_nand_verify(argc - 2, &argv[2]);
	} else if (strcmp(argv[1], "markbad") == 0) {
		if (strtou32(argv[2], &block) == 0)
			return nand_mark_bad_block(block);
		else
			return -1;
	} else if (strcmp(argv[1], "dump") == 0) {
		return cmd_nand_dump(argc - 2, &argv[2]);
	} else if (strcmp(argv[1], "prog") == 0) {
		return cmd_nand_prog(argc - 2, &argv[2]);
	} else if (strcmp(argv[1], "speed") == 0) {
		return cmd_nand_speed(argc - 2, &argv[2]);
#if defined(CONFIG_NAND_USE_FLASH_BBT)
	} else if ((strcmp(argv[1], "erase") == 0) && (strcmp(argv[2], "bbt") == 0)) {
		return nand_erase_bbt();
#endif
	}

	if (argc < 4) {
		return -1;
	}
	if (strcmp(argv[1], "erase") != 0) {
		return -1;
	}
	putstr("erase nand blocks including any bad blocks...\r\n");
	putstr("press enter to start!\r\n");
	putstr("press any key to terminate!\r\n");
	ret_val = uart_wait_escape(0xffffffff);
	if (ret_val == 0) {
		return -1;
	}

	strtou32(argv[2], &start_block);
	strtou32(argv[3], &blocks);
	for (i = 0, block = start_block; i < blocks; i++, block++) {
		if (uart_poll())
			break;
		if (block >= total_blocks)
			break;

		ret_val = nand_erase_block(block);
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

	return 0;
}

/*===========================================================================*/
static char help_nand[] =
#if defined(CONFIG_NAND_USE_FLASH_BBT)
	"nand show bb|bbt|partition\r\n"
	"nand erase bbt\r\n"
#else
	"nand show bb|partition\r\n"
#endif
	"nand erase BLOCK BLOCKS\r\n"
	"nand markbad BLOCK\r\n"
	"nand rclm init|late|other|all\r\n"
	"nand verify free|all|BLOCK [no_ecc]\r\n"
	"nand dump PAGE [PAGES] [no_ecc]\r\n"
	"nand prog PAGE [PAGES] [no_ecc] [BITFLIP]\r\n"
	"nand speed [free]|[BLOCK BLOCKS]\r\n";
__CMDLIST(cmd_nand, "nand", help_nand);

