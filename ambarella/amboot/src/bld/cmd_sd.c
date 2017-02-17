/**
 * bld/cmd_sd.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
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
#include <ambhw/sd.h>
#include <sdmmc.h>

/*===========================================================================*/
#define DIAG_SD_BUF_START	bld_hugebuf_addr
#define SECTORS_PER_OP		128
#define SECTORS_PER_SHMOO	4096

/*===========================================================================*/
static int cmd_sd_init(int argc, char *argv[], int verbose)
{
	u32 slot, mode, clock;
	int ret_val;

	if (argc < 1)
		return -1;

	if (verbose) {
		putstr("running SD test ...\r\n");
		putstr("press any key to terminate!\r\n");
	}

	ret_val = strtou32(argv[0], &slot);
	if (ret_val < 0) {
		putstr("Invalid slot id!\r\n");
		return -1;
	}

	if (argc >= 2) {
		if (strcmp(argv[1], "ddr50") == 0)
			mode = SDMMC_MODE_DDR50;
		else if (strcmp(argv[1], "sdr104") == 0)
			mode = SDMMC_MODE_SDR104;
		else if (strcmp(argv[1], "sdr50") == 0)
			mode = SDMMC_MODE_SDR50;
		else if (strcmp(argv[1], "sdr25") == 0)
			mode = SDMMC_MODE_SDR25;
		else if (strcmp(argv[1], "sdr12") == 0)
			mode = SDMMC_MODE_SDR12;
		else if (strcmp(argv[1], "hs") == 0)
			mode = SDMMC_MODE_HS;
		else if (strcmp(argv[1], "ds") == 0)
			mode = SDMMC_MODE_DS;
		else
			mode = SDMMC_MODE_AUTO;
	} else {
		mode = SDMMC_MODE_AUTO;
	}

	if (argc >= 3) {
		ret_val = strtou32(argv[2], &clock);
		if (ret_val < 0) {
			putstr("Invalid clock\r\n");
			return -1;
		}
		clock *= 1000000;
	} else {
		clock = -1;
	}

	ret_val = sdmmc_init_sd(slot, mode, clock, verbose);
	if (ret_val < 0)
		ret_val = sdmmc_init_mmc(slot, mode, clock, verbose);

	if (verbose) {
		putstr("total_secs: ");
		putdec(sdmmc_get_total_sectors());
		putstr("\r\n");
	}

	return ret_val;
}

static int cmd_sd_show(int argc, char *argv[])
{
	int i, ret_val = -1;

	if (argc < 1)
		return -1;

	if (strcmp(argv[0], "partition") == 0) {
		putstr("total_sectors: ");
		putdec(sdmmc_get_total_sectors());
		putstr("\r\n");
		putstr("sector_size: ");
		putdec(sdmmc.sector_size);
		putstr("\r\n");
		for (i = 0; i < HAS_IMG_PARTS; i++) {
			putstr(get_part_str(i));
			putstr(" partition blocks: ");
			putdec(sdmmc.ssec[i]);
			putstr(" - ");
			putdec(sdmmc.ssec[i] + sdmmc.nsec[i]);
			putstr("\r\n");
		}

		ret_val = 0;
	} else if (strcmp(argv[0], "info") == 0) {
		ret_val = sdmmc_show_card_info();
		if (ret_val < 0)
			putstr("Please init card first\r\n");
	}

	return ret_val;
}

static int cmd_sd_read(int argc, char *argv[])
{
	int ret_val = 0;
	int i = 0;
	int total_secs;
	int sector, sectors;
	u8 *buf = (u8 *)DIAG_SD_BUF_START;

	ret_val = cmd_sd_init(argc, argv, 1);
	if (ret_val < 0)
		return ret_val;

	total_secs = sdmmc_get_total_sectors();
	for (sector = 0, i = 0; sector < total_secs;
		sector += SECTORS_PER_OP, i++) {

		if (uart_poll())
			break;

		if ((total_secs - sector) < SECTORS_PER_OP)
			sectors = total_secs - sector;
		else
			sectors = SECTORS_PER_OP;

		ret_val = sdmmc_read_sector(sector, sectors, buf);

		putchar('.');

		if ((i & 0xf) == 0xf) {
			putchar(' ');
			putdec(sector);
			putchar('/');
			putdec(total_secs);
			putstr(" (");
			putdec(sector * 100 / total_secs);
			putstr("%)\t\t\r");
		}

		if (ret_val < 0) {
			putstr("\r\nfailed at sector ");
			putdec(sector);
			putstr("\r\n");
			break;
		}

		if ((sector + SECTORS_PER_OP) >= total_secs)
			break;
	}

	putstr("\r\ndone!\r\n");

	return ret_val;
}

static int cmd_sd_write(int argc, char *argv[])
{
	int ret_val = 0;
	int i, j;
	int total_secs;
	int sector, sectors;
	u8 *buf = (u8 *)DIAG_SD_BUF_START;

	ret_val = cmd_sd_init(argc, argv, 1);
	if (ret_val < 0)
		return ret_val;

	for (i = 0; i < SECTORS_PER_OP * SDMMC_SEC_SIZE / 16; i++) {
		for (j = 0; j < 16; j++) {
			buf[(i * 16) + j] = i;
		}
	}

	total_secs = sdmmc_get_total_sectors();
	for (sector = 0, i = 0; sector < total_secs;
		sector += SECTORS_PER_OP, i++) {

		if (uart_poll())
			break;

		if ((total_secs - sector) < SECTORS_PER_OP)
			sectors = total_secs - sector;
		else
			sectors = SECTORS_PER_OP;

		ret_val = sdmmc_write_sector(sector, sectors, buf);

		putchar('.');

		if ((i & 0xf) == 0xf) {
			putchar(' ');
			putdec(sector);
			putchar('/');
			putdec(total_secs);
			putstr(" (");
			putdec(sector * 100 / total_secs);
			putstr("%)\t\t\r");
		}

		if (ret_val < 0) {
			putstr("\r\nfailed at sector ");
			putdec(sector);
			putstr("\r\n");
		}

		if ((sector + SECTORS_PER_OP) >= total_secs)
			break;
	}

	putstr("\r\ndone!\r\n");

	return ret_val;
}

static int cmd_sd_verify_data(u8* origin, u8* data, u32 len, u32 sector, u8 verbose)
{
	int i, rval = 0;

	for (i = 0; i < len; i++) {
		if (origin[i] != data[i]) {
			rval = -1;;
			if (verbose) {
				putstr("\r\nSector ");
				putdec(i / SDMMC_SEC_SIZE + sector);
				putstr(" byte ");
				putdec(i % SDMMC_SEC_SIZE);
				putstr(" failed data origin: 0x");
				puthex(origin[i]);
				putstr(" data: 0x");
				puthex(data[i]);
				putstr("\r\n");
			}
		}
	}

	return rval;
}

static int cmd_sd_verify(int argc, char *argv[], u8 shmoo, u8 verbose)
{
	u32 i, total_secs, sector, sectors;
	u8 *wbuf = (u8 *)DIAG_SD_BUF_START;
	u8 *rbuf = (u8 *)DIAG_SD_BUF_START + 0x100000;
	int ret_val = 0;

	if(!shmoo) {
		ret_val = cmd_sd_init(argc, argv, 1);
		if (ret_val < 0)
			return ret_val;

	}

	for (i = 0; i < SECTORS_PER_OP * SDMMC_SEC_SIZE; i++)
		wbuf[i] = rand() / SECTORS_PER_OP;

	if (shmoo)
		total_secs = SECTORS_PER_SHMOO;
	else
		total_secs = sdmmc_get_total_sectors();

	for (i = 0, sector = 0, sectors = 0; sector < total_secs;
		i++, sector += SECTORS_PER_OP) {

		if (uart_poll())
			break;

		sectors++;
		if (sectors > SECTORS_PER_OP)
			sectors = 1;

		ret_val = sdmmc_write_sector(sector, sectors, wbuf);
		if (ret_val != 0) {
			if (verbose) {
				putstr("Write sector fail ");
				putdec(ret_val);
				putstr(" @ ");
				putdec(sector);
				putstr(" : ");
				putdec(sectors);
				putstr("\r\n");
			}
			return -1;
		}

		ret_val = sdmmc_read_sector(sector, sectors, rbuf);
		if (ret_val != 0) {
			if (verbose) {
				putstr("Read sector fail ");
				putdec(ret_val);
				putstr(" @ ");
				putdec(sector);
				putstr(" : ");
				putdec(sectors);
				putstr("\r\n");
			}
			return -1;
		}

		ret_val = cmd_sd_verify_data(wbuf, rbuf,
				(sectors * SDMMC_SEC_SIZE), sector, verbose);
		if (ret_val < 0)
			return -1;

		if(verbose) {
			putchar('.');

			if ((i & 0xf) == 0xf) {
				putchar(' ');
				putdec(sector);
				putchar('/');
				putdec(total_secs);
				putstr(" (");
				putdec(sector * 100 / total_secs);
				putstr("%)\t\t\r");
			}
		}

		if ((sector + SECTORS_PER_OP) >= total_secs)
			break;
	}

	if (verbose)
		putstr("\r\ndone!\r\n");

	return ret_val;
}

static int cmd_sd_erase(int argc, char *argv[])
{
	u32 sector, sector_end, sector_num;
	u32 total_secs, sector_erase;
	int i, ret_val = 0;

	ret_val = cmd_sd_init(argc, argv, 1);
	if (ret_val < 0)
		return ret_val;

	total_secs = sdmmc_get_total_sectors();

	if (argc > 1) {
		ret_val = strtou32(argv[0], &sector);
		if (ret_val < 0 || sector >= total_secs) {
			putstr("Invalid sector address!\r\n");
			return -1;
		}
	} else {
		sector = 0;
	}

	if (argc > 2) {
		ret_val = strtou32(argv[0], &sector_num);
		if (ret_val < 0) {
			putstr("Invalid sector number!\r\n");
			return -1;
		}
	} else {
		sector_num = sdmmc_get_total_sectors();
	}

	if (sector + sector_num > total_secs)
		sector_num = total_secs - sector;

	sector_end = sector + sector_num;

	for (i = 0; sector < sector_end; i++, sector += sector_erase) {
		if (sector_end - sector > SECTORS_PER_OP)
			sector_erase = SECTORS_PER_OP;
		else
			sector_erase = sector_end - sector_num;

		ret_val = sdmmc_erase_sector(sector, sector_erase);

		putchar('.');

		if ((i & 0xf) == 0xf) {
			putchar(' ');
			putdec(i * SECTORS_PER_OP);
			putchar('/');
			putdec(sector_num);
			putstr(" (");
			putdec(i * SECTORS_PER_OP * 100 / sector_num);
			putstr("%)\t\t\r");
		}

		if (ret_val < 0) {
			putstr("\r\nfailed at sector ");
			putdec(sector);
			putstr("\r\n");
			break;
		}

		if (uart_poll())
			break;
	}

	putstr("\r\ndone!\r\n");

	return ret_val;
}

#if (SD_SOFT_PHY_SUPPORT == 1)

struct sd_timing_reg {
	u32 sd_phy0_reg;
	u32 sd_phy1_reg;
	u32 sd_ncr_reg;
} timing[32*3*3];

static int cmd_sd_shmoo(int argc, char *argv[])
{
	u32 phy0, phy1, ncr_reg, reg;
	u32 slot, i, num;
	const char *mode[] = {"ddr50", "sdr104", "sdr50", "sdr25", "sdr12", "hs", "ds"};
	int ret_val;

	ret_val = strtou32(argv[0], &slot);
	if (ret_val < 0) {
		putstr("Invalid slot id!\r\n");
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(mode); i++) {
		if (strcmp(argv[1], mode[i]) == 0)
			break;
	}

	if (i >= ARRAY_SIZE(mode)) {
		putstr("Unknown mode!\r\n");
		return -1;
	}

	putstr("\r\nRunning SD Shmoo for ");
	putstr(mode[i]);
	putstr(" ...\r\n");

	ret_val = cmd_sd_init(argc, argv, 0);
	if(ret_val < 0) {
		putstr("Shmoo: init sd card is fail\r\n");
		return 0;
	}

	memset(timing, 0, sizeof(timing));
	i = num = 0;

	/* [sd_clkout_bypass, sd_rx_clk_pol, sd_data_cmd_bypass, sbc_2, sbc_1]
	 * = 0000b to 1111b, [bit26, bit19, bit18, bit[2:1]] in SD_PHY_CTRL_0_REG */
	for(phy0 = 0; phy0 < 32; phy0++) {
		for (phy1 = 0x00000000; phy1 < 0x20202020; phy1 += 0x0f0f0f0f) {
			for (ncr_reg = 0x0000; ncr_reg < 0x3333; ncr_reg += 0x1111) {
				reg = 0x1;
				reg |= ((phy0 >> 0) & 0x3) << 1;   /* sbc */
				reg |= ((phy0 >> 2) & 0x1) << 18; /* sd_data_cmd_bypass */
				reg |= ((phy0 >> 3) & 0x1) << 19; /* sd_rx_clk_pol */
				reg |= ((phy0 >> 4) & 0x1) << 26; /* sd_clkout_bypass */

				writel(SD_PHY_CTRL_0_REG, reg | 0x02000000);
				rct_timer_dly_ms(2);
				writel(SD_PHY_CTRL_0_REG, reg);
				rct_timer_dly_ms(2);
				writel(SD_PHY_CTRL_1_REG, phy1);

				writel(SD_BASE(slot) + SD_LAT_CTRL_OFFSET, ncr_reg);

				/*begin to verify data*/
				ret_val = cmd_sd_verify(argc, argv, 1, 0);
				if(ret_val == 0) {
					timing[num].sd_phy0_reg =
						readl(SD_PHY_CTRL_0_REG);
					timing[num].sd_phy1_reg =
						readl(SD_PHY_CTRL_1_REG);
					timing[num].sd_ncr_reg =
						readl(SD_BASE(slot) + SD_LAT_CTRL_OFFSET);
					num++;
				}

				/*user abort*/
				if (uart_poll()) {
					putstr("\r\n!!! Abort by User !!!\r\n");
					goto __done;
				}

				putchar('.');

				if ((i++ & 0xf) == 0xf) {
					putchar(' ');
					putdec(i);
					putchar('/');
					putdec(ARRAY_SIZE(timing));
					putstr(" (");
					putdec(i * 100 / ARRAY_SIZE(timing));
					putstr("%) [");
					putdec(num);
					putstr("]\r");
				}
			}
		}
	}

	putstr("\r\nDone.\r\n\r\n");

__done:
	putstr("Shmoo Result: \r\n");

	for(i = 0; i < num; i++) {
		putstr("CTRL_0:0x");
		puthex(timing[i].sd_phy0_reg);
		putstr(", CTRL_1:0x");
		puthex(timing[i].sd_phy1_reg);
		putstr(", ncr_reg:");
		puthex(timing[i].sd_ncr_reg);
		putstr("\r\n");

	}

	putstrdec("\r\nThe num of sucessful: ", num);

	return 0;
}

#endif
/*===========================================================================*/
static int cmd_sd(int argc, char *argv[])
{
	int ret_val = -1;

	if (argc < 3)
		return ret_val;

	if (strcmp(argv[1], "init") == 0) {
		ret_val = cmd_sd_init(argc - 2, &argv[2], 1);
	} else if (strcmp(argv[1], "read") == 0) {
		ret_val = cmd_sd_read(argc - 2, &argv[2]);
	} else if (strcmp(argv[1], "write") == 0) {
		ret_val = cmd_sd_write(argc - 2, &argv[2]);
	} else if (strcmp(argv[1], "verify") == 0) {
		ret_val = cmd_sd_verify(argc - 2, &argv[2], 0, 1);
	} else if (strcmp(argv[1], "erase") == 0) {
		ret_val = cmd_sd_erase(argc - 2, &argv[2]);
#if (SD_SOFT_PHY_SUPPORT == 1)
	} else if (strcmp(argv[1], "shmoo") == 0) {
		ret_val = cmd_sd_shmoo(argc - 2, &argv[2]);
#endif
	} else if (strcmp(argv[1], "show") == 0) {
		ret_val = cmd_sd_show(argc - 2, &argv[2]);
	}

	return ret_val;
}

/*===========================================================================*/
static char help_sd[] =
	"\t[slot]: 0/1/2/..\r\n"
	"\t[mode]: ds/hs/sdr12/sdr25/sdr50/sdr104/ddr50\r\n"
	"\t[clock]: clock in MHz\r\n"
	"\tsd init slot [mode] [clock]\r\n"
	"\tsd read slot [mode] [clock]\r\n"
	"\tsd write slot [mode] [clock]\r\n"
	"\tsd verify slot [mode] [clock]\r\n"
	"\tsd erase slot [ssec] [nsec]\r\n"
#if (SD_SOFT_PHY_SUPPORT == 1)
	"\tsd shmoo slot [mode] [clock]\r\n"
#endif
	"\tsd show partition/info\r\n"
	"Test SD.\r\n";
__CMDLIST(cmd_sd, "sd", help_sd);

