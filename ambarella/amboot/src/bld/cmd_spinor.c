/**
 * bld/cmd_spinor.c
 *
 * History:
 *    2014/05/14 - [Cao Rongrong] created file
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
#include <ambhw/spinor.h>

static int cmd_spinor_dump(int argc, char *argv[])
{
	u32 address, size, i;
	u8 *rbuf;

	if (strtou32(argv[0], &address) < 0) {
		putstr("invalid address!\r\n");
		return -1;
	}

	if (argc == 1 || strtou32(argv[1], &size) < 0)
		size = flspinor.page_size;

	if (address + size > flspinor.chip_size) {
		putstr("address = 0x");
		puthex(address);
		putstr(", size = 0x");
		puthex(size);
		putstr(", Overflow!\r\n");
		return -1;
	}

	rbuf = (u8*)bld_hugebuf_addr;

	if (spinor_read_data(address, rbuf, size) < 0)
		return -1;

	for (i = 0; i < size; i++) {
		putbyte(rbuf[i]);
		if ((i + 1) % 16 == 0)
			putstr("\r\n");
		else
			putstr(" ");
	}
	putstr("\r\n");

	return 0;
}

static int cmd_spinor_verify_data(u8 *origin, u8 *data, u32 len)
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

static int cmd_spinor_erase(int argc, char *argv[])
{
	u32 nsec;
	int rval = -1;

	if (argc == 0 || strtou32(argv[0], &nsec) < 0)
		nsec = 1;

	rval = spinor_erase_sector(nsec);
	if (rval < 0) {
		putstr("erase failed. <sector ");
		putdec(nsec);
		putstr(">\r\n");
	}
	return 0;
}

static int cmd_spinor_verify(int argc, char *argv[])
{
	u8 *wbuf, *rbuf;
	u32 ssec, nsec, i;
	int rval = -1;

	ssec = flspinor.ssec[PART_PTB] + flspinor.nsec[PART_PTB];

	if (argc == 0 || strtou32(argv[0], &nsec) < 0)
		nsec = 1;

	putstr("running spinor stress test ...\r\n");
	putstr("press any key to terminate!\r\n");

	putstr("\r\nssec = ");
	putdec(ssec);
	putstr(", nsec = ");
	putdec(nsec);
	putstr("\r\n");

	wbuf = (u8 *)bld_hugebuf_addr;
	rbuf = (u8 *)(bld_hugebuf_addr + flspinor.sector_size);
	for (i = 0; i < flspinor.sector_size; i++) {
		wbuf[i] = rand() / 256;
	}

	for (i = ssec; i < ssec + nsec; i++) {
		if (uart_poll())
			break;

		rval = spinor_erase_sector(i);
		if (rval < 0) {
			putstr("erase failed. <sector ");
			putdec(i);
			putstr(">\r\n");
			goto done;
		}

		if (uart_poll())
			break;

		rval = spinor_write_data(i * flspinor.sector_size,
					wbuf, flspinor.sector_size);
		if (rval < 0) {
			putstr("write failed. <sector ");
			putdec(i);
			putstr(">\r\n");
			goto done;
		}

		rval = spinor_read_data(i * flspinor.sector_size,
					rbuf, flspinor.sector_size);
		if (rval < 0) {
			putstr("read failed. <sector ");
			putdec(i);
			putstr(">\r\n");
			goto done;
		}

		if (uart_poll())
			break;

		rval = spinor_erase_sector(i);
		if (rval < 0) {
			putstr("erase failed, <sector ");
			putdec(i / flspinor.sector_size);
			putstr(">\r\n");
			goto done;
		}

		if (uart_poll())
			break;

		rval = cmd_spinor_verify_data(wbuf, rbuf, flspinor.sector_size);
		if (rval != 0) {
			putstr("\r\nspinor verify data error count ");
			putdec(rval);
			rval = -1;
			goto done;
		}

		if ((i & 0xf) == 0xf) {
			putchar(' ');
			putdec(i - ssec);
			putchar('/');
			putdec(nsec);
			putstr(" (");
			putdec((i - ssec) * 100 / nsec);
			putstr("%)\t\t\r");
		} else {
			putchar('.');
		}

done:
		if (rval < 0) {
			putstr("\r\nfailed at address ");
			putdec(i);
			putstr("\r\n");
		}
	}

	putstr("\r\ndone!\r\n");

	return 0;
}

static int cmd_spinor_speed(int argc, char *argv[])
{
	u8 *buf;
	u32 ssec, nsec, i, time;
	u64 size;
	int rval = -1;

	ssec = flspinor.ssec[PART_BLD] + flspinor.nsec[PART_BLD];

	if (argc == 0 || strtou32(argv[0], &nsec) < 0)
		nsec = 1;
	size = nsec * flspinor.sector_size;

	buf = (u8 *)bld_hugebuf_addr;

	putstr("test speed, nsec = ");
	putdec(nsec);
	putstr("\r\n");

	putstr("erase...");
	rct_timer_reset_count();
	for (i = ssec; i < ssec + nsec; i++) {
		rval = spinor_erase_sector(i);
		if (rval < 0) {
			putstr("erase failed. <sector ");
			putdec(i);
			putstr(">\r\n");
			break;
		}
	}

	time = rct_timer_get_count();

	putstr(" done!\r\n");
	putstr("    size = ");
	putdec(size);
	putstr(" Bytes, time = ");
	putdec(time);
	putstr("ms, speed = ");
	putdec(size * 1000/ time);
	putstr(" Byte/s\r\n");

	putstr("write...");
	rct_timer_reset_count();
	for (i = ssec; i < ssec + nsec; i++) {
		rval = spinor_write_data(i * flspinor.sector_size,
					buf, flspinor.sector_size);
		if (rval < 0) {
			putstr("write failed. <sector ");
			putdec(i);
			putstr(">\r\n");
			break;
		}
	}

	time = rct_timer_get_count();

	putstr(" done!\r\n");
	putstr("    size = ");
	putdec(size);
	putstr(" Bytes, time = ");
	putdec(time);
	putstr("ms, speed = ");
	putdec(size * 1000/ time);
	putstr(" Byte/s\r\n");

	putstr("read...");
	rct_timer_reset_count();
	for (i = ssec; i < ssec + nsec; i++) {
		rval = spinor_read_data(i * flspinor.sector_size,
					buf, flspinor.sector_size);
		if (rval < 0) {
			putstr("read failed. <sector ");
			putdec(i);
			putstr(">\r\n");
			break;
		}
	}

	time = rct_timer_get_count();

	putstr(" done!\r\n");
	putstr("    size = ");
	putdec(size);
	putstr(" Bytes, time = ");
	putdec(time);
	putstr("ms, speed = ");
	putdec(size * 1000/ time);
	putstr(" Byte/s\r\n");

	return 0;
}

/*===========================================================================*/
static int cmd_spinor(int argc, char *argv[])
{
	int i;

	if (!strcmp(argv[1], "show")) {
		putstr("chip size: ");
		putdec(flspinor.chip_size);
		putstr(", sector size: ");
		putdec(flspinor.sector_size);
		putstr(", page size: ");
		putdec(flspinor.page_size);
		putstr("\r\n");
		putstr("sectors per chip: ");
		putdec(flspinor.sectors_per_chip);
		putstr("\r\n");
		for (i = 0; i < HAS_IMG_PARTS; i++) {
			putstr(get_part_str(i));
			putstr(" partition sectors: ");
			putdec(flspinor.ssec[i]);
			putstr(" - ");
			putdec(flspinor.ssec[i] + flspinor.nsec[i]);
			putstr("\r\n");
		}
		return 0;
	} else if (!strcmp(argv[1], "dump")) {
		return cmd_spinor_dump(argc - 2, &argv[2]);
	} else if (!strcmp(argv[1], "erase")) {
		return cmd_spinor_erase(argc - 2, &argv[2]);
	} else if (!strcmp(argv[1], "erasechip")) {
		return spinor_erase_chip();
	} else if (!strcmp(argv[1], "verify")) {
		return cmd_spinor_verify(argc - 2, &argv[2]);
	} else if (!strcmp(argv[1], "speed")) {
		return cmd_spinor_speed(argc - 2, &argv[2]);
	} else {
		return -2;
	}

	return 0;
}

/*===========================================================================*/
static char help_spinor[] =
	"spinor show\r\n"
	"spinor dump ADDRESS [SIZE]\r\n"
	"spinor verify [NSEC]\r\n"
	"spinor erase [NSEC]\r\n"
	"spinor erasechip \r\n"
	"spinor speed [NSEC]\r\n";

__CMDLIST(cmd_spinor, "spinor", help_spinor);

