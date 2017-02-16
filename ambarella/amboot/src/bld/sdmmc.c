/**
 * system/src/bld/sdmmc.c
 *
 * History:
 *    2008/05/23 - [Dragon Chiang] created file
 *    2015/04/20 - [Cao Rongrong] rewrite the codes
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>
#include <ambhw/cache.h>
#include <ambhw/gpio.h>
#include <ambhw/fio.h>
#include <ambhw/sd.h>
#include <sdmmc.h>

/*===========================================================================*/
#define sd_putstr(s) do {if (sdmmc.verbose) putstr(s);} while(0)
#define sd_puthex(h) do {if (sdmmc.verbose) puthex(h);} while(0)
#define sd_putdec(d) do {if (sdmmc.verbose) putdec(d);} while(0)

/*===========================================================================*/
static u8 sdmmc_safe_buf[1024] __attribute__ ((aligned(1024), section(".bss.noinit")));
sdmmc_t sdmmc;

int sdmmc_show_card_info(void)
{
	const char *curr_mode;
	int i;

	putstr("cid: ");
	for (i = 0; i < 4; i++)
		puthex(sdmmc.raw_cid[i]);
	putstr("\r\n");

	putstr("csd: ");
	for (i = 0; i < 4; i++)
		puthex(sdmmc.raw_csd[i]);
	putstr("\r\n");

	putstr("scr: ");
	for (i = 0; i < 2; i++)
		puthex(sdmmc.raw_scr[i]);
	putstr("\r\n");

	putstr("\r\n");
	putstr("spec_vers: ");
	putdec(sdmmc.spec_vers);
	putstr("\r\n");

	putstr("ocr: ");
	puthex(sdmmc.ocr);
	putstr("\r\n");

	putstr("ccc: ");
	puthex(sdmmc.ccc);
	putstr("\r\n");

	putstr("rca: ");
	puthex(sdmmc.rca);
	putstr("\r\n");

	putstr("address mode: ");
	if (sdmmc.sector_mode)
		putstr("SECTOR");
	else
		putstr("BYTE");
	putstr("\r\n");

	if (sdmmc.type == SDMMC_TYPE_SD) {
		const char *sdr_mode[] = {"SDR12", "SDR25", "SDR50", "SDR104", "DDR50"};
		const char *hs_mode[] = {"DS", "HS"};
		putstr("supported mode: ");
		if (sdmmc.ocr & (0x1 << S18A_SHIFT_BIT)) {
			for (i = 0; i < 5; i++) {
				if (sdmmc.supported_mode & (0x1 << i)) {
					putstr(sdr_mode[i]);
					putstr(" ");
				}
			}
		} else {
			for (i = 0; i < 2; i++) {
				if (sdmmc.supported_mode & (0x1 << i)) {
					putstr(hs_mode[i]);
					putstr(" ");
				}
			}
		}
		putstr("\r\n");

		putstr("supported width: ");
		for (i = 0; i < 4; i++) {
			if (sdmmc.bus_width & (0x1 << i)) {
				putdec(0x1 << i);
				putstr("bit ");
			}
		}
		putstr("\r\n");
	}

	putstr("current mode: ");
	switch(sdmmc.current_mode) {
	case SDMMC_MODE_DS:
		curr_mode = "DS";
		break;
	case SDMMC_MODE_HS:
		curr_mode = "HS";
		break;
	case SDMMC_MODE_HS200:
		curr_mode = "HS200";
		break;
	case SDMMC_MODE_SDR12:
		curr_mode = "SDR12";
		break;
	case SDMMC_MODE_SDR25:
		curr_mode = "SDR25";
		break;
	case SDMMC_MODE_SDR50:
		curr_mode = "SDR50";
		break;
	case SDMMC_MODE_SDR104:
		curr_mode = "SDR104";
		break;
	case SDMMC_MODE_DDR50:
		curr_mode = "DDR50";
		break;
	default:
		curr_mode = "UNKNOWN";
		break;
	}

	putstr(curr_mode);
	putstr("\r\n");

	putstr("clock: ");
	putdec(sdmmc.clk_hz / 1000 / 1000);
	putstr("MHz\r\n");

	putstr("\r\n");

	return 0;
}

/*===========================================================================*/
static int sdmmc_set_pll(int slot, int clock)
{
	if (slot == 0) {
		rct_set_sd_pll(clock);
		clock = get_sd_freq_hz();
	} else if (slot == 1) {
		rct_set_sdio_pll(clock);
		clock = get_sdio_freq_hz();
	} else if (slot == 2) {
		rct_set_sdxc_pll(clock);
		clock = get_sdxc_freq_hz();
	} else {
		putstr("Slot not supported!\r\n");
		return -1;
	}

	sdmmc.clk_hz = clock;

	/* wait for pll stable, cannot be removed. */
	rct_timer_dly_ms(3);

	return 0;
}

/**
 * Reset the CMD line.
 */
static void sdmmc_reset_cmd_line(int slot)
{
	u32 sd_base = SD_BASE(slot);

	/* Reset the CMD line */
	writeb(sd_base + SD_RESET_OFFSET, SD_RESET_CMD);

	/* wait command line ready */
	while ((readl(sd_base + SD_STA_OFFSET)) & SD_STA_CMD_INHIBIT_CMD);
}

/**
 * Reset the DATA line.
 */
static void sdmmc_reset_data_line(int slot)
{
	u32 sd_base = SD_BASE(slot);

	/* Reset the DATA line */
	writeb(sd_base + SD_RESET_OFFSET, SD_RESET_DAT);

	/* wait data line ready */
	while ((readl(sd_base + SD_STA_OFFSET)) & SD_STA_CMD_INHIBIT_DAT);
}

/**
 * Reset the SD controller.
 */
static void sdmmc_reset_all(u32 slot)
{
	u32 sd_base = SD_BASE(slot);

	/* Reset the controller and wait for the register to clear */
	writeb(sd_base + SD_RESET_OFFSET, SD_RESET_ALL);

	/* Wait for reset to complete (busy wait!) */
	while (readb(sd_base + SD_RESET_OFFSET) != 0x0);

	/* Set SD Power (3.3V) */
	writeb((sd_base + SD_PWR_OFFSET), (SD_PWR_3_3V | SD_PWR_ON));

	/* Set data timeout */
	writeb((sd_base + SD_TMO_OFFSET), 0xe);

	/* Setup clock */
	writew((sd_base + SD_CLK_OFFSET),
	       (SD_CLK_DIV_256 | SD_CLK_ICLK_EN | SD_CLK_EN));

	/* Wait until clock becomes stable */
	while ((readw(sd_base + SD_CLK_OFFSET) & SD_CLK_ICLK_STABLE) !=
	       SD_CLK_ICLK_STABLE);

	/* Enable interrupts on events we want to know */
	writew((sd_base + SD_NISEN_OFFSET), 0x1cf);
	writew((sd_base + SD_NIXEN_OFFSET), 0x1cf);

	/* Enable error interrupt status and signal */
	writew((sd_base + SD_EISEN_OFFSET), 0x3ff);
	writew((sd_base + SD_EIXEN_OFFSET), 0x3ff);
}

/*
 * Clean interrupts status.
 */
static void sdmmc_clean_interrupt(int slot)
{
	u32 sd_base = SD_BASE(slot);

	writew((sd_base + SD_NIS_OFFSET), readw(sd_base + SD_NIS_OFFSET));
	writew((sd_base + SD_EIS_OFFSET), readw(sd_base + SD_EIS_OFFSET));
}

/**
 * Get sd protocol response.
 */
static void sdmmc_get_resp(int slot, u32 *resp)
{
	u32 sd_base = SD_BASE(slot);

	/* The Arasan HC has unusual response register contents (compared) */
	/* to other host controllers, we need to read and convert them into */
	/* one supported by the protocol stack */
	resp[0] = ((readl(sd_base + SD_RSP3_OFFSET) << 8) |
		   (readl(sd_base + SD_RSP2_OFFSET)) >> 24);
	resp[1] = ((readl(sd_base + SD_RSP2_OFFSET) << 8) |
		   (readl(sd_base + SD_RSP1_OFFSET) >> 24));
	resp[2] = ((readl(sd_base + SD_RSP1_OFFSET) << 8) |
		   (readl(sd_base + SD_RSP0_OFFSET) >> 24));
	resp[3] = (readl(sd_base + SD_RSP0_OFFSET) << 8);
}

/**
 * Check if sdmmc is in slot.
 *
 * @returns - 0 if sdmmc is absent; 1 if sdmmc is present
 */
static u32 sdmmc_card_in_slot(int slot)
{
	return !!(readl(SD_BASE(slot) + SD_STA_OFFSET) & SD_STA_CARD_INSERTED);
}

/* Wait for command to complete, return -1 if timeout or error happened */
static int sdmmc_wait_command_done(int slot)
{
	u32 sd_base = SD_BASE(slot);
	u32 timeout, nis, eis;
	int rval = 0;

	rct_timer_reset_count();
	while(1) {
		timeout = rct_timer_get_count();
		nis = readw(sd_base + SD_NIS_OFFSET);
		eis = readw(sd_base + SD_EIS_OFFSET);

		if ((nis & SD_NIS_CMD_DONE) && !(nis & SD_NIS_ERROR))
			break;

		if ((nis & SD_NIS_ERROR) || (eis != 0x0) || (timeout >= SD_CMD_TIMEOUT)) {
			sdmmc_reset_cmd_line(slot);
			sdmmc_reset_data_line(slot);
			sd_putstr("CMD error: cmd=0x");
			sd_puthex(readw(SD_BASE(slot) + SD_CMD_OFFSET));
			sd_putstr(", eis=0x");
			sd_puthex(eis);
			sd_putstr(", nis=0x");
			sd_puthex(nis);
			sd_putstr(timeout >= SD_CMD_TIMEOUT ? ", [timeout]" : "");
			sd_putstr("\r\n");
			rval = -1;
			break;
		}
	}

	/* clear interrupts */
	sdmmc_clean_interrupt(slot);

	return rval;
}

/* Wait for data transfer to complete, return -1 if timeout or error happened */
static int sdmmc_wait_data_done(int slot)
{
	u32 sd_base = SD_BASE(slot);
	u32 timeout, nis, eis;
	int rval = 0;

	rct_timer_reset_count();
	while (1) {
		timeout = rct_timer_get_count();
		nis = readw(sd_base + SD_NIS_OFFSET);
		eis = readw(sd_base + SD_EIS_OFFSET);

		if ((nis & SD_NIS_XFR_DONE) && !(nis & SD_NIS_ERROR))
			break;

		if ((nis & SD_NIS_ERROR) || (eis != 0x0) || (timeout >= SD_TRAN_TIMEOUT)) {
			sdmmc_reset_cmd_line(slot);
			sdmmc_reset_data_line(slot);
			sd_putstr("DATA error: cmd=0x");
			sd_puthex(readw(SD_BASE(slot) + SD_CMD_OFFSET));
			sd_putstr(", eis=0x");
			sd_puthex(eis);
			sd_putstr(", nis=0x");
			sd_puthex(nis);
			sd_putstr(timeout >= SD_TRAN_TIMEOUT ? "[timeout]" : "");
			sd_putstr("\r\n");
			rval = -1;
			break;
		}
	}

	/* clear interrupts */
	sdmmc_clean_interrupt(slot);

	return rval;
}

/*
 * SD/MMC command
 */
int sdmmc_command(int command, int argument)
{
	int slot = sdmmc.slot;
	u32 sd_base = SD_BASE(slot);

	/* wait CMD line ready */
	while ((readl(sd_base + SD_STA_OFFSET) & SD_STA_CMD_INHIBIT_CMD));

	/* argument */
	writel((sd_base + SD_ARG_OFFSET), argument);
	writew(sd_base + SD_XFR_OFFSET, 0x0);

	/* command */
	writew((sd_base + SD_CMD_OFFSET), command);

	return sdmmc_wait_command_done(slot);
}

static int sdmmc_command_with_data(int command, int argument, void *buf, int size)
{
	int slot = sdmmc.slot;
	u32 sd_base = SD_BASE(slot);
	int rval;

	BUG_ON(size > 1024);

	/* wait CMD line ready */
	while ((readl(sd_base + SD_STA_OFFSET) & SD_STA_CMD_INHIBIT_CMD));

	/* wait DAT line ready */
	while ((readl(sd_base + SD_STA_OFFSET) & SD_STA_CMD_INHIBIT_DAT));

	writel((sd_base + SD_DMA_ADDR_OFFSET), (u32)sdmmc_safe_buf);
	writel((sd_base + SD_ARG_OFFSET), argument);
	writew((sd_base + SD_BLK_SZ_OFFSET), (SD_BLK_SZ_512KB | size));
	writew((sd_base + SD_BLK_CNT_OFFSET), 0x0);
	writew((sd_base + SD_XFR_OFFSET), (SD_XFR_CTH_SEL | SD_XFR_DMA_EN));
	writew((sd_base + SD_CMD_OFFSET), command);

	rval = sdmmc_wait_command_done(slot);
	if (rval < 0)
		return rval;

	rval = sdmmc_wait_data_done(slot);
	if (rval < 0)
		return rval;

	flush_d_cache((void *)sdmmc_safe_buf, 1024);
	if (buf != NULL)
		memcpy(buf, sdmmc_safe_buf, size);

	return 0;
}

static u32 sdmmc_send_status_cmd(u32 wait_busy, u32 wait_data_ready)
{
	u32 sd_base = SD_BASE(sdmmc.slot);
	u32 resp0 = 0x0;
	int rval;

	do {
		rval = sdmmc_command(cmd_13, sdmmc.rca << 16);
		if (rval < 0)
			goto sdmmc_send_status_exit;

		resp0 = readl(sd_base + SD_RSP0_OFFSET);
	} while ((wait_data_ready && !(resp0 & (1 << 8))) ||
		(wait_busy && (((resp0 & 0x00001E00) >> 9) == 7)));

sdmmc_send_status_exit:
	return resp0;
}

static void sdmmc_fio_select(int slot)
{
#if (FIO_INDEPENDENT_SD == 0)
	if (slot == 1)
		setbitsl(FIO_CTR_REG, FIO_CTR_XD);
	else
		clrbitsl(FIO_CTR_REG, FIO_CTR_XD);

	/* enable SD controller */
	writel(FIO_DMACTR_REG,
	       ((readl(FIO_DMACTR_REG) & 0xcfffffff) | FIO_DMACTR_SD));
#endif
}

static int mmc_init_partition()
{
#if defined(CONFIG_BOOT_MEDIA_EMMC)
	int i, ssec = 0, nsec;
	int part_size[HAS_IMG_PARTS];

	get_part_size(part_size);

	for (i = 0; i < HAS_IMG_PARTS; i++) {
		if ((get_part_dev(i) & PART_DEV_EMMC) != PART_DEV_EMMC)
			continue;

		nsec = (part_size[i] + sdmmc.sector_size - 1) / sdmmc.sector_size;
		sdmmc.ssec[i] = ssec;
		sdmmc.nsec[i] = nsec;
		ssec += nsec;
	}

	for (; i < PART_MAX; i++) {
		sdmmc.ssec[i] = 0;
		sdmmc.nsec[i] = 0;
	}

	if (ssec * sdmmc.sector_size >= sdmmc.capacity) {
		putstr("partition length in total exceeds eMMC size\r\n");
		return -1;
	}
#endif
	return 0;
}

static int sd_switch_to_1v8(int slot)
{
	u32 sd_base = SD_BASE(slot);
	int rval;

	/* CMD11: invoke voltage switch sequence */
	rval = sdmmc_command(cmd_11, 0x0);
	if (rval < 0)
		return rval;

	/* disable clock */
	clrbitsw(sd_base + SD_CLK_OFFSET, SD_CLK_EN);
	/* for safe, wait for 1ms to make sure clk is NOT output. */
	rct_timer_dly_ms(1);

	/* Set SD Power (1.8V) */
	writeb(sd_base + SD_PWR_OFFSET, SD_PWR_1_8V | SD_PWR_ON);
	amboot_bsp_sd_slot_init(slot, SDMMC_VOLTAGE_1V8);
	/*sd spec needs 5ms at least, but some cards need more than 10ms
	 * so we wait for 20ms here for safe. */
	rct_timer_dly_ms(20);

	/* enable clock */
	setbitsw(sd_base + SD_CLK_OFFSET, SD_CLK_EN);
	/*according to sd spec, we wait for 1ms here */
	rct_timer_dly_ms(1);

	/*wait switch voltage is completed*/
	rct_timer_reset_count();
	while(1) {
		if(readl(sd_base + SD_STA_OFFSET) & 0x01f00000)
			break;

		if (rct_timer_get_count() > 500) {
			putstr("failed to switch voltage to 1.8V\r\n");
			return -1;
		}
	}

	return 0;
}

static int sd_switch_mode(int slot, int mode, int clock)
{
	u32 sd_base = SD_BASE(slot);
	u8 sw_func[64];
	u32 arg;
	int rval;

	/* CMD6: query Switch Functions */
	rval = sdmmc_command_with_data(scmd_6, 0, sw_func, 64);
	if (rval < 0)
		goto switch_done;

	sdmmc.supported_mode = sw_func[13];
	if (sdmmc.supported_mode == 0) {
		rval = -1;
		putstr("Unknown mode!!!\r\n");
		goto switch_done;
	}

	if (mode == SDMMC_MODE_AUTO) {
		if (sdmmc.ocr & (0x1 << S18A_SHIFT_BIT)) {
			if (sdmmc.supported_mode & 0x8)
				mode = SDMMC_MODE_SDR104;
			else if (sdmmc.supported_mode & 0x10)
				mode = SDMMC_MODE_DDR50;
			else if (sdmmc.supported_mode & 0x4)
				mode = SDMMC_MODE_SDR50;
			else if (sdmmc.supported_mode & 0x2)
				mode = SDMMC_MODE_SDR25;
			else if (sdmmc.supported_mode & 0x1)
				mode = SDMMC_MODE_SDR12;
		} else {
			if (sdmmc.supported_mode & 0x2)
				mode = SDMMC_MODE_HS;
			else
				mode = SDMMC_MODE_DS;
		}
	} else {
		if ((mode == SDMMC_MODE_SDR104 && !(sdmmc.supported_mode & 0x8)) ||
		    (mode == SDMMC_MODE_DDR50 && !(sdmmc.supported_mode & 0x10)) ||
		    (mode == SDMMC_MODE_SDR50 && !(sdmmc.supported_mode & 0x4)) ||
		    (mode == SDMMC_MODE_SDR25 && !(sdmmc.supported_mode & 0x2)) ||
		    (mode == SDMMC_MODE_SDR12 && !(sdmmc.supported_mode & 0x1)) ||
		    (mode == SDMMC_MODE_HS && !(sdmmc.supported_mode & 0x2)) ||
		    (mode == SDMMC_MODE_DS && !(sdmmc.supported_mode & 0x1))) {
		    putstr("Non-supported mode by the card!!!\r\n");
		    rval = -1;
		    goto switch_done;
		}
	}

	switch(mode) {
	case SDMMC_MODE_SDR12:
		arg = 0x80fffff0;
		break;
	case SDMMC_MODE_SDR25:
		arg = 0x80fffff1;
		break;
	case SDMMC_MODE_SDR50:
		arg = 0x80fffff2;
		break;
	case SDMMC_MODE_SDR104:
		arg = 0x80fffff3;
		break;
	case SDMMC_MODE_DDR50:
		arg = 0x80fffff4;
		break;
	case SDMMC_MODE_HS:
		arg = 0x80fffff1;
		break;
	case SDMMC_MODE_DS:
	default:
		arg = 0x80fffff0;
		break;
	}

	sdmmc.current_mode = mode;

	/* CMD6: Set Switch function */
	rval = sdmmc_command_with_data(scmd_6, arg, NULL, 64);
	if (rval < 0)
		goto switch_done;

	/* ACMD6: set 4bit bus width */
	rval = sdmmc_command(cmd_55, sdmmc.rca << 16);
	if (rval < 0)
		goto switch_done;
	rval = sdmmc_command(acmd_6, 0x2);
	if (rval < 0)
		goto switch_done;

	if (mode == SDMMC_MODE_DDR50) {
		writel(sd_base + SD_XC_CTR_OFFSET, SD_XC_CTR_DDR_EN);
		setbitsw(sd_base + SD_HOST2_OFFSET, 0x4);
		writeb((sd_base + SD_HOST_OFFSET), SD_HOST_4BIT | SD_HOST_HIGH_SPEED);
	} else {
		writel(sd_base + SD_XC_CTR_OFFSET, 0x0);
		clrbitsw(sd_base + SD_HOST2_OFFSET, 0x4);
		writeb((sd_base + SD_HOST_OFFSET), SD_HOST_4BIT);
	}

switch_done:
	return rval;
}

static int sdmmc_init_pre(int slot, int verbose)
{
	int rval;

	if (!sd_slot_is_valid(slot)) {
		putstrdec("No sucn slot: ", slot);
		return -1;
	}

	memset(&sdmmc, 0, sizeof(sdmmc));

	sdmmc.slot = slot;
	sdmmc.verbose = verbose;
	sdmmc.sector_size = SDMMC_SEC_SIZE;
	/* no_1v8 is determined by board,
	 * so it may be changed in amboot_bsp_sd_slot_init */
	sdmmc.no_1v8 = 0;

	/*power on slot for card*/
	if(amboot_bsp_sd_slot_init != NULL)
		amboot_bsp_sd_slot_init(slot, SDMMC_VOLTAGE_3V3);

	/* reset PHY to default value */
	if(amboot_bsp_sd_phy_init != NULL)
		amboot_bsp_sd_phy_init(slot, SDMMC_MODE_AUTO);

	sdmmc_set_pll(slot, 24000000);

	/* configure FIO */
	sdmmc_fio_select(slot);

	sdmmc_reset_all(slot);

	/* make sure card present */
	rval = sdmmc_card_in_slot(slot);
	if (rval == 0) {
		putstr("No sdmmc present.\r\n");
		return -1;
	}

	/* CMD0 - GO_IDLE_STATE */
	rval = sdmmc_command(cmd_0, 0x0);
	if (rval < 0)
		return -1;

	/* Make sure the data line ready for Emmc boot */
	sdmmc_reset_data_line(slot);

	return 0;
}

static int sdmmc_init_post(int slot, int clock)
{
	u32 sd_base = SD_BASE(slot);
	int rval;

	writew((sd_base + SD_BLK_SZ_OFFSET), (SD_BLK_SZ_512KB | 0x200));

	/* CMD16: Set block length */
	if (sdmmc.sector_mode == 0) {
		rval = sdmmc_command(cmd_16, SDMMC_SEC_SIZE);
		if (rval < 0)
			return rval;
	}

	/* Disable clock */
	clrbitsw(sd_base + SD_CLK_OFFSET, SD_CLK_ICLK_EN | SD_CLK_EN);

	sdmmc_set_pll(slot, clock);

	writew(sd_base + SD_CLK_OFFSET, (SD_CLK_DIV_1 | SD_CLK_ICLK_EN | SD_CLK_EN));

	/* Wait until clock becomes stable */
	while (!(readw(sd_base + SD_CLK_OFFSET) & SD_CLK_ICLK_STABLE));

	amboot_bsp_sd_phy_init(slot, sdmmc.current_mode);

	return 0;
}

int sdmmc_init_mmc(int slot, int mode, int clock, int verbose)
{
	u32 sd_base = SD_BASE(slot);
	u32 poll_count, resp[4];
	int rval;

	rval = sdmmc_init_pre(slot, verbose);
	if (rval <0)
		goto emmc_done;

	/* CMD0 - GO_IDLE_STATE */
	rval = sdmmc_command(cmd_0, 0x0);
	if (rval < 0)
		goto emmc_done;

	sdmmc.ocr = 0x40ff0000;
	/* Send CMD1 with working voltage and address mode until the
	 * memory becomes ready. */
	for (poll_count = 0; poll_count < 1000; poll_count++) {
		/* CMD1 */
		rval = sdmmc_command(cmd_1, sdmmc.ocr);
		if (rval < 0)
			goto emmc_done;

		/* ocr = UNSTUFF_BITS(cmd.resp, 8, 32) */
		resp[0] = readl(sd_base + SD_RSP0_OFFSET);
		/* 0: busy, 1: ready */
		if (resp[0] & SDMMC_CARD_BUSY)
			break;
	}

	if (poll_count >= 1000) {
		putstr("give up polling cmd1 for eMMC\r\n");
		rval = -1;
		goto emmc_done;
	}

	/* check if using sector address mode */
	if ((resp[0] & 0x60000000) == 0x40000000)
		sdmmc.sector_mode = 1;
	else
		sdmmc.sector_mode = 0;

	/* CMD2: All Send CID. */
	rval = sdmmc_command(cmd_2, 0x0);
	if (rval < 0)
		goto emmc_done;
	sdmmc_get_resp(slot, resp);
	memcpy(sdmmc.raw_cid, resp, sizeof(sdmmc.raw_cid));

	/* CMD3: Set RCA. PS: CMD3 of eMMC is different from CMD3 of SD. */
	sdmmc.rca = 0x01;
	rval = sdmmc_command(cmd_3, (sdmmc.rca << 16));
	if (rval < 0)
		goto emmc_done;

	/* CMD9: Get the CSD */
	rval = sdmmc_command(cmd_9, sdmmc.rca << 16);
	if (rval < 0)
		goto emmc_done;

	sdmmc_get_resp(slot, resp);
	memcpy(sdmmc.raw_csd, resp, sizeof(sdmmc.raw_csd));

	sdmmc.spec_vers =	UNSTUFF_BITS(resp, 122,  4);
	if (sdmmc.spec_vers < 4) {
		putstr("The eMMC card is too out-of-dated!\r\n");
		return 0;
	}

	sdmmc.ccc = UNSTUFF_BITS(resp, 84, 12);

	/* Set bus width to 1 bit */
	writeb((sd_base + SD_HOST_OFFSET), 0x0);

	/* CMD13: Get status */
	sdmmc_send_status_cmd(1, 0);

	/* CMD7: Select sdmmc */
	rval = sdmmc_command(cmd_7, sdmmc.rca << 16);
	if (rval < 0)
		goto emmc_done;

	clock = (clock == -1) ? 48000000 : clock;

	if (clock > 52000000) {
		sdmmc_command(cmd_6, 0x03b90201);
		if (rval < 0)
			goto emmc_done;
		sdmmc.current_mode = SDMMC_MODE_HS200;
	} else if (clock > 26000000) {
		sdmmc_command(cmd_6, 0x03b90101);
		if (rval < 0)
			goto emmc_done;
		sdmmc.current_mode = SDMMC_MODE_HS;
	} else {
		sdmmc.current_mode = SDMMC_MODE_DS;
	}

#if defined (ENABLE_MMC_8BIT)
	rval = sdmmc_command(cmd_6, 0x03b70200);
	if (rval == 0)
		writeb((sd_base + SD_HOST_OFFSET), SD_HOST_8BIT);
	else
		writeb((sd_base + SD_HOST_OFFSET), 0x0);
#else
	rval = sdmmc_command(cmd_6, 0x03b70100);
	if (rval == 0)
		writeb((sd_base + SD_HOST_OFFSET), SD_HOST_4BIT);
	else
		writeb((sd_base + SD_HOST_OFFSET), 0x0);
#endif

	rval = sdmmc_init_post(slot, clock);
	if (rval < 0)
		goto emmc_done;

	rval = sdmmc_command_with_data(mcmd_8, 0x0, sdmmc_safe_buf, 512);
	if (rval < 0)
		goto emmc_done;

	sdmmc.capacity = *(u32 *)(sdmmc_safe_buf + 212); /* sector count */
	sdmmc.capacity <<= 9;

	sdmmc.type = SDMMC_TYPE_MMC;

	rval = mmc_init_partition();
emmc_done:
	if (rval < 0)
		putstr("sdmmc init mmc fail!!!\r\n");

	return rval;
}

int sdmmc_init_sd(int slot, int mode, int clock, int verbose)
{
	u32 sd_base = SD_BASE(slot);
	u32 poll_count, resp[4];
	u32 csd_struct, scr[2], ccs;
	int rval;

	rval = sdmmc_init_pre(slot, verbose);
	if (rval <0)
		goto sd_done;

	/* CMD8 (SD Ver2.0 support only) */
	rval = sdmmc_command(cmd_8, 0x01aa);
	if (rval < 0) {
		/* Ver2.00 or later SD memory card(voltage mismatch),
		 * or Ver1.X SD memory card,
		 * or not SD memory card */

		/* Reset again if card does not support SD ver2.00
		 * This is not mentioned by SD 2.00 spec!*/
		rval = sdmmc_command(cmd_0, 0x0);
		if (rval != 0)
			goto sd_done;

		ccs = 0x0;
	} else {
		ccs = 0x1;
	}

	/* rca = 0 since card is in IDLE state */
	rval = sdmmc_command(cmd_55, 0x0);
	if (rval < 0)
		goto sd_done;

	/* ACMD41 to query OCR */
	rval = sdmmc_command(acmd_41, 0x0);
	if (rval < 0)
		goto sd_done;

	sdmmc.ocr = readl(sd_base + SD_RSP0_OFFSET);
	sdmmc.ocr &= SDMMC_HOST_AVAIL_OCR;
	sdmmc.ocr |= ccs << HCS_SHIFT_BIT;
	if ((mode != SDMMC_MODE_HS) && (mode != SDMMC_MODE_DS))
		sdmmc.ocr |= (!!!sdmmc.no_1v8) << S18R_SHIFT_BIT;

	/* ACMD41 - Send working voltage until the sd card becomes ready */
	for (poll_count = 0; poll_count < 1000; poll_count++) {
		/* rca = 0 since card is in IDLE state */
		rval = sdmmc_command(cmd_55, 0x0);
		if (rval < 0)
			goto sd_done;

		rval = sdmmc_command(acmd_41, sdmmc.ocr);
		if (rval < 0)
			goto sd_done;

		resp[0] = readl(sd_base + SD_RSP0_OFFSET);
		/* 0: busy, 1: ready */
		if (resp[0] & SDMMC_CARD_BUSY)
			break;
	}

	if (poll_count >= 1000) {
		putstr("give up polling acmd41 for SD card\r\n");
		rval = -1;
		goto sd_done;
	}

	if ((resp[0] & (0x1 << CCS_SHIFT_BIT)) == 0)
		sdmmc.ocr &= ~(0x1 << CCS_SHIFT_BIT);

	if ((resp[0] & (0x1 << S18A_SHIFT_BIT)) == 0) {
		sdmmc.ocr &= ~(0x1 << S18A_SHIFT_BIT);
	} else {
		/* switch the voltage to 1.8v*/
		rval = sd_switch_to_1v8(slot);
		if (rval < 0)
			goto sd_done;
	}

	/* CMD2: All Send CID. */
	rval = sdmmc_command(cmd_2, 0x0);
	if (rval < 0)
		goto sd_done;
	sdmmc_get_resp(slot, resp);
	memcpy(sdmmc.raw_cid, resp, sizeof(sdmmc.raw_cid));

	/* CMD3: query RCA */
	rval = sdmmc_command(cmd_3, 0x0);
	if (rval < 0)
		goto sd_done;
	sdmmc.rca = readl(sd_base + SD_RSP0_OFFSET) >> 16;

	/* CMD3: send CSD */
	rval = sdmmc_command(cmd_9, (sdmmc.rca << 16));
	if (rval < 0)
		goto sd_done;

	sdmmc_get_resp(slot, resp);
	memcpy(sdmmc.raw_csd, resp, sizeof(sdmmc.raw_csd));

	csd_struct = UNSTUFF_BITS(resp, 126, 2);
	switch(csd_struct) {
	case 0:
		/* Standard Capacity */
		sdmmc.capacity = (u64)(1 + UNSTUFF_BITS(resp, 62, 12)); /* C_SIZE */
		sdmmc.capacity <<= (UNSTUFF_BITS(resp, 47, 3) + 2); /* C_SIZE_MULT */
		sdmmc.capacity *= (u64)(1 << UNSTUFF_BITS(resp, 80, 4)); /* READ_BL_LEN */
		sdmmc.sector_mode = 0;
		break;
	case 1:
		/* High Capacity or Extended Capacity */
		sdmmc.capacity = (u64)(UNSTUFF_BITS(resp, 48, 22) + 1) * 512 * 1024;
		sdmmc.sector_mode = 1;
		break;
	default:
		putstrdec("Unknown CSD structure version: ", csd_struct);
		rval = -1;
		goto sd_done;
	}

	sdmmc.ccc = UNSTUFF_BITS(resp, 84, 12);

	/* CMD7: Select card */
	rval = sdmmc_command(cmd_7, sdmmc.rca << 16);
	if (rval < 0)
		goto sd_done;

	/* ACMD51: Read SCR */
	rval = sdmmc_command(cmd_55, sdmmc.rca << 16);
	if (rval < 0)
		goto sd_done;
	rval = sdmmc_command_with_data(acmd_51, 0, scr, 8);
	if (rval < 0)
		goto sd_done;

	resp[3] = sdmmc.raw_scr[1] = be32_to_cpu(scr[1]);
	resp[2] = sdmmc.raw_scr[0] = be32_to_cpu(scr[0]);

	if (UNSTUFF_BITS(resp, 56, 4) < 2) { /* SD_SPEC */
		sdmmc.spec_vers = UNSTUFF_BITS(resp, 56, 4);
	} else {
		if (UNSTUFF_BITS(resp, 47, 1) == 0) /* SD_SPEC3 */
			sdmmc.spec_vers = 2;
		else
			sdmmc.spec_vers = 3;
	}

	if (sdmmc.spec_vers < 2 || !(sdmmc.ccc & (1<<10))) {
		putstr("The SD card is too out-of-dated!\r\n");
		return 0;
	}

	sdmmc.bus_width = UNSTUFF_BITS(resp, 48, 4);
	if ((sdmmc.bus_width & 0x4) == 0) {
		putstr("The SD card doesn't support 4bit bus width!\r\n");
		return 0;
	}

	/* CMD13: Get SD card status, not used, just follow the sd spec. */
	rval = sdmmc_command(cmd_55, (sdmmc.rca << 16));
	if (rval < 0)
		goto sd_done;
	rval = sdmmc_command_with_data(acmd_13, 0, NULL, 64);
	if (rval < 0)
		goto sd_done;

	/* Switch to proper mode */
	rval = sd_switch_mode(slot, mode, clock);
	if (rval < 0)
		goto sd_done;

	switch(sdmmc.current_mode) {
	case SDMMC_MODE_SDR12:
		clock = (clock == -1) ? 25000000 : clock;
		break;
	case SDMMC_MODE_SDR25:
		clock = (clock == -1) ? 50000000 : clock;
		break;
	case SDMMC_MODE_SDR50:
		clock = (clock == -1) ? 100000000 : clock;
		break;
	case SDMMC_MODE_SDR104:
		clock = (clock == -1) ? 208000000 : clock;
		break;
	case SDMMC_MODE_DDR50:
		clock = (clock == -1) ? 50000000 : clock;
		break;
	case SDMMC_MODE_HS:
		clock = (clock == -1) ? 50000000 : clock;
		break;
	case SDMMC_MODE_DS:
	default:
		clock = (clock == -1) ? 25000000 : clock;
		break;
	}

	rval = sdmmc_init_post(slot, clock);
	if (rval < 0)
		goto sd_done;

	sdmmc.type = SDMMC_TYPE_SD;
sd_done:
	if (rval < 0)
		putstr("sdmmc init sd fail!!!\r\n");

	return rval;
}


/*****************************************************************************/
/*
 * Read single/multi sector from SD/MMC.
 */
static int _sdmmc_read_sector_DMA(int sector, int sectors, u8 *target)
{
	u32 slot = sdmmc.slot;
	u32 sd_base = SD_BASE(slot);
	u32 timout = 0;
	int rval = 0;

	u32 start_512kb = (u32)target & 0xfff80000;
	u32 end_512kb = ((u32)target + (sectors << 9) - 1) & 0xfff80000;

	if (start_512kb != end_512kb) {
		putstr("WARNING: crosses 512KB DMA boundary!\r\n");
		return -1;
	}

	clean_flush_d_cache((void *) target, (sectors << 9));

	/* wait CMD line ready */
	while ((readl(sd_base + SD_STA_OFFSET) & SD_STA_CMD_INHIBIT_CMD));

	rct_timer_reset_count();
	/* wait DAT line ready */
	while ((readl(sd_base + SD_STA_OFFSET) & SD_STA_CMD_INHIBIT_DAT)){
		timout = rct_timer_get_count();
		if (timout >= 1000) {
			putstr("wait DAT line timeout\r\n");
			rval = -1;
			goto done;
		}
	}

	writel((sd_base + SD_DMA_ADDR_OFFSET), (u32)target);

	if (sdmmc.sector_mode)			/* argument */
		writel((sd_base + SD_ARG_OFFSET), sector);
	else
		writel((sd_base + SD_ARG_OFFSET), (sector << 9));

	if (sectors == 1) {
		/* single sector *********************************************/
		writew((sd_base + SD_BLK_CNT_OFFSET), 0x0);

		writew((sd_base + SD_XFR_OFFSET),
		       (SD_XFR_CTH_SEL | SD_XFR_DMA_EN));

		writew((sd_base + SD_CMD_OFFSET), cmd_17);
	} else {
		/* multi sector **********************************************/
		writew((sd_base + SD_BLK_CNT_OFFSET), sectors);

		writew((sd_base + SD_XFR_OFFSET), (SD_XFR_MUL_SEL	|
						   SD_XFR_CTH_SEL	|
						   SD_XFR_AC12_EN	|
						   SD_XFR_BLKCNT_EN	|
						   SD_XFR_DMA_EN));

		writew((sd_base + SD_CMD_OFFSET), cmd_18);
	}

	rval = sdmmc_wait_command_done(slot);
	if (rval < 0)
		goto done;

	rval = sdmmc_wait_data_done(slot);
	if (rval < 0)
		goto done;

	sdmmc_send_status_cmd(1, 1);

done:
	return rval;
}

/**
 * Handle read DMA mode 512Kb alignment.
 */
static int sdmmc_read_sector_DMA(int sector, int sectors, u8 *target)
{
	int rval = 0;
	u32 s_bound;	/* Start address boundary */
	u32 e_bound;	/* End address boundary */
	u32 s_block, e_block, blocks;
	u8 *buf_ptr = target;

	int addr;

	if (sectors == 1)
		goto single_block_read;
	else
		goto multi_block_read;

single_block_read:
	/* Single-block read */

	s_bound = ((u32) buf_ptr) & 0xfff80000;
	e_bound = ((u32) buf_ptr) + (sectors << 9) - 1;
	e_bound &= 0xfff80000;

	if (s_bound != e_bound) {
		u8 *tmp = sdmmc_safe_buf;
		rval = _sdmmc_read_sector_DMA(sector, 1, tmp);
		memcpy(buf_ptr, tmp, 512);
	} else {
		rval = _sdmmc_read_sector_DMA(sector, 1, buf_ptr);
	}

	return rval;

multi_block_read:
	/* Multi-block read */
	blocks = sectors;
	s_block = 0;
	e_block = 0;
	while (s_block < blocks) {
		/* Check if the first block crosses DMA buffer */
		/* boundary */
		s_bound = ((u32) target) + (s_block << 9);
		s_bound &= 0xfff80000;

		e_bound = ((u32) target) + ((s_block + 1) << 9) - 1;
		e_bound &= 0xfff80000;

		if (s_bound != e_bound) {
			u8 *tmp = sdmmc_safe_buf;

			addr = (sector + s_block);

			/* Read single block */
			rval = _sdmmc_read_sector_DMA(addr, 1, tmp);
			if (rval < 0)
				return rval;
			memcpy((u32 *)((u32)target + (s_block << 9)), tmp, 512);
			s_block++;

			if (s_block >= sectors)
				break;
		}

		/* Try with maximum data within same boundary */
		s_bound = ((u32) target) + (s_block << 9);
		s_bound &= 0xfff80000;
		e_block = s_block;
		do {
			e_block++;
			e_bound = ((u32) target) + ((e_block + 1) << 9) - 1;
			e_bound &= 0xfff80000;
		} while (e_block < blocks && s_bound == e_bound);
		e_block--;

		addr = (sector + s_block);

		/* Read multiple blocks */
		rval = _sdmmc_read_sector_DMA(addr, e_block - s_block + 1,
					(u8 *)((u32)target + (s_block << 9)));
		if (rval < 0)
			return rval;

		s_block = e_block + 1;
	}

	return rval;
}

int sdmmc_read_sector(int sector, int sectors, u8 *target)
{
	int rval = -1;

	sdmmc_fio_select(sdmmc.slot);

	while (sectors > SDMMC_SEC_CNT) {

		rval = sdmmc_read_sector_DMA(sector, SDMMC_SEC_CNT, target);
		if (rval < 0)
			return rval;

		sector += SDMMC_SEC_CNT;
		sectors -= SDMMC_SEC_CNT;
		target += ((SDMMC_SEC_SIZE * SDMMC_SEC_CNT) >> 2);
	}

	rval = sdmmc_read_sector_DMA(sector, sectors, target);

	return rval;
}

/*****************************************************************************/
/**
 * ACMD23 Set pre-erased write block count.
 */
static int sdmmc_set_wr_blk_erase_cnt(u32 blocks)
{
	int rval = 0;
#if (SD_SUPPORT_ACMD23)
	if (sdmmc.type == SDMMC_TYPE_SD) {
		/* ACMD23 : CMD55 */
		rval = sdmmc_command(cmd_55, sdmmc.rca << 16);

		/* ACMD23 */
		rval = sdmmc_command(acmd_23, blocks);
	}
#endif
	return rval;
}

/**
 * Write single/multi sector to SD/MMC.
 */
static int _sdmmc_write_sector_DMA(int sector, int sectors, u8 *image)
{
	u32 slot = sdmmc.slot;
	u32 sd_base = SD_BASE(slot);
	u32 cur_tim = 0, s_tck, e_tck;
	int rval = 0;

	u32 start_512kb = (u32)image & 0xfff80000;
	u32 end_512kb = ((u32)image + (sectors << 9) - 1) & 0xfff80000;

	if (start_512kb != end_512kb) {
		putstr("WARNING: crosses 512KB DMA boundary!\r\n");
		return -1;
	}

	clean_d_cache((void *)image, (sectors << 9));

	/* wait command line ready */
	while ((readl(sd_base + SD_STA_OFFSET) & SD_STA_CMD_INHIBIT_CMD) ==
	       SD_STA_CMD_INHIBIT_CMD);

	/* enable timer */
	rct_timer_reset_count();
	rct_timer_dly_ms(5);
	s_tck = rct_timer_get_tick();

	/* wait data line ready */
	while ((readl(sd_base + SD_STA_OFFSET) & SD_STA_CMD_INHIBIT_DAT) ==
	       SD_STA_CMD_INHIBIT_DAT){
		e_tck = rct_timer_get_tick();
		cur_tim = rct_timer_tick2ms(s_tck, e_tck);
		if (cur_tim >= 100) {
			sdmmc_reset_cmd_line(sd_base);
			sdmmc_reset_data_line(sd_base);
			putstr("write timeout\r\n");
			return -1;
		}

	}

	writel((sd_base + SD_DMA_ADDR_OFFSET), (u32)image);

	if (sdmmc.sector_mode)			/* argument */
		writel((sd_base + SD_ARG_OFFSET), sector);
	else
		writel((sd_base + SD_ARG_OFFSET), (sector << 9));

	if (sectors == 1) {
		/* single sector *********************************************/
		writew((sd_base + SD_BLK_CNT_OFFSET), 0x0);

		writew((sd_base + SD_XFR_OFFSET), (SD_XFR_DMA_EN));

		writew((sd_base + SD_CMD_OFFSET), cmd_24);
	} else {
		/* multi sector **********************************************/
		writew((sd_base + SD_BLK_CNT_OFFSET), sectors);

		writew((sd_base + SD_XFR_OFFSET), (SD_XFR_MUL_SEL	|
						   SD_XFR_AC12_EN	|
						   SD_XFR_BLKCNT_EN	|
						   SD_XFR_DMA_EN));

		writew((sd_base + SD_CMD_OFFSET), cmd_25);
	}

	rval = sdmmc_wait_command_done(slot);
	if (rval < 0)
		goto done;

	rval = sdmmc_wait_data_done(slot);
	if (rval < 0)
		goto done;

	sdmmc_send_status_cmd(1, 1);

done:
	return rval;
}

/**
 * Handle write DMA mode 512Kb alignment.
 */
static int sdmmc_write_sector_DMA(int sector, int sectors, u8 *image)
{
	int rval = 0;
	u32 s_bound;	/* Start address boundary */
	u32 e_bound;	/* End address boundary */
	u32 s_block, e_block, blocks;
	u8 *buf_ptr = image;
	int addr;

	if (sectors == 1)
		goto single_block_write;
	else
		goto multi_block_write;

single_block_write:
	/* Single-block write */

	s_bound = ((u32) image) & 0xfff80000;
	e_bound = ((u32) image) + (sectors << 9) - 1;
	e_bound &= 0xfff80000;

	rval = sdmmc_set_wr_blk_erase_cnt(1);
	if (rval < 0)
		return rval;

	if (s_bound != e_bound) {
		u8 *tmp = sdmmc_safe_buf;
		memcpy(tmp, buf_ptr, 512);
		rval = _sdmmc_write_sector_DMA(sector, 1, tmp);
	} else {
		rval = _sdmmc_write_sector_DMA(sector, 1, buf_ptr);
	}
	return rval;

multi_block_write:
	/* Multi-block write */
	blocks = sectors;
	s_block = 0;
	e_block = 0;
	while (s_block < blocks) {
		/* Check if the first block crosses DMA buffer */
		/* boundary */
		s_bound = ((u32) image) + (s_block << 9);
		s_bound &= 0xfff80000;

		e_bound = ((u32) image) + ((s_block + 1) << 9) - 1;
		e_bound &= 0xfff80000;

		if (s_bound != e_bound) {
			u8 *tmp = sdmmc_safe_buf;
			memcpy(tmp, (u32 *)((u32)image + (s_block << 9)), 512);

			addr = (sector + s_block);

			/* Set pre-erased write block count */
			rval = sdmmc_set_wr_blk_erase_cnt(1);
			if (rval < 0)
				return rval;

			/* Write single block */
			rval = _sdmmc_write_sector_DMA(addr, 1, tmp);
			if (rval < 0)
				return rval;
			s_block++;

			if (s_block >= sectors)
				break;
		}

		/* Try with maximum data within same boundary */
		s_bound = ((u32) image) + (s_block << 9);
		s_bound &= 0xfff80000;
		e_block = s_block;
		do {
			e_block++;
			e_bound = ((u32) image) + ((e_block + 1) << 9) - 1;
			e_bound &= 0xfff80000;
		} while (e_block < blocks && s_bound == e_bound);
		e_block--;

		addr = (sector + s_block);

		/* Set pre-erased write block count */
		rval = sdmmc_set_wr_blk_erase_cnt(e_block - s_block + 1);
		if (rval < 0)
			return rval;

		/* Write multiple blocks */
		rval = _sdmmc_write_sector_DMA(addr, e_block - s_block + 1,
					(u8 *)((u32)image + (s_block << 9)));
		if (rval < 0)
			return rval;

		s_block = e_block + 1;
	}

	return rval;
}

int sdmmc_write_sector(int sector, int sectors, u8 *image)
{
	int rval = -1;

	sdmmc_fio_select(sdmmc.slot);

	while (sectors > SDMMC_SEC_CNT) {

		rval = sdmmc_write_sector_DMA(sector, SDMMC_SEC_CNT, image);
		if (rval < 0)
			return rval;

		sector += SDMMC_SEC_CNT;
		sectors -= SDMMC_SEC_CNT;
		image += ((SDMMC_SEC_SIZE * SDMMC_SEC_CNT) >> 2);
	}

	rval = sdmmc_write_sector_DMA(sector, sectors, image);

	return rval;
}

int __sdmmc_erase_sector(int sector, int sectors)
{
	int cmd1, cmd2, rval;

	if (sector + sectors > sdmmc.capacity) {
		putstr("sdmmc_erase_sector() wrong argument");
		return -1;
	}

	sectors += sector;
	if (sdmmc.sector_mode == 0) {
		sector <<= 9;
		sectors <<= 9;
	}

	cmd1 = (sdmmc.type == SDMMC_TYPE_MMC) ? cmd_35 : cmd_32;
	cmd2 = (sdmmc.type == SDMMC_TYPE_MMC) ? cmd_36 : cmd_33;

	sdmmc_fio_select(sdmmc.slot);

	/* CMD32/35:ERASE_WR_BLK_START */
	rval = sdmmc_command(cmd1, sector);
	if (rval < 0)
		goto done;

	/* CMD33/36:ERASE_WR_BLK_END */
	rval = sdmmc_command(cmd2, sectors);
	if (rval < 0)
		goto done;

	/* CMD38:ERASE */
	rval = sdmmc_command(cmd_38, 0x0);
	if (rval < 0)
		goto done;

	sdmmc_send_status_cmd(1, 1);
done:
	return rval;

}

int sdmmc_erase_sector(int sector, int sectors)
{
	int rval = 0;

#ifndef USE_SDMMC_ERASE_WRITE_SECTOR
	rval = __sdmmc_erase_sector(sector, sectors);
#else
	int i = sector;
#ifndef USE_SDMMC_HUGE_BUFFER
	u32 max_wsec = sizeof(sdmmc_safe_buf) / SDMMC_SEC_SIZE;
	u8 *tmp_buf = sdmmc_safe_buf;
#else
	u32 max_wsec = SDMMC_SEC_CNT;
	u8 *tmp_buf = bld_hugebuf_addr;
#endif
	u32 n = sectors / max_wsec;
	u32 remind = sectors % max_wsec;

	memzero((void *)tmp_buf, max_wsec << 9);

	for (; n > 0; n--) {
		rval = sdmmc_write_sector(i, max_wsec, tmp_buf);
		if (rval < 0)
			return rval;
		i += max_wsec;
	}
	if (remind != 0)
		rval = sdmmc_write_sector(i, remind, tmp_buf);
#endif

	return rval;
}

u32 sdmmc_get_total_sectors(void)
{
	return sdmmc.capacity >> 9;
}

/*===========================================================================*/
#if defined(CONFIG_BOOT_MEDIA_EMMC)
void amboot_rct_reset_chip_sdmmc()
{
	sdmmc_command(0, 0xf0f0f0f0);
}
#endif

/*===========================================================================*/
int sdmmc_set_emmc_boot_info(void)
{
	int ret_val = -1;
	u32 emmc_poc;
	u32 emmc_boot_bus_width;
	u32 emmc_rst_n;
	u32 emmc_partition_config;

	/* do dummy read to make sure DAT line is ok. */
	ret_val = sdmmc_read_sector(0, 1, sdmmc_safe_buf);
	if (ret_val < 0) {
		goto sdmmc_set_emmc_boot_info_exit;
	}

	putstr("Set eMMC BOOT_BUS_WIDTH:");
	/* BOOT_BUS_WIDTH [177] */
	emmc_boot_bus_width = (177 << 16);
	emmc_poc = rct_get_emmc_poc();
	if (emmc_poc & RCT_BOOT_EMMC_8BIT) {
		emmc_boot_bus_width |= (0x02 << 8);
		putstr(" 8Bit");
	} else if (emmc_poc & RCT_BOOT_EMMC_4BIT) {
		emmc_boot_bus_width |= (0x01 << 8);
		putstr(" 4Bit");
	} else {
		putstr(" 1Bit");
	}
	if (emmc_poc & RCT_BOOT_EMMC_DDR) {
		emmc_boot_bus_width |= (0x10 << 8);
		putstr(" DDR");
	} else if (emmc_poc & RCT_BOOT_EMMC_HS) {
		emmc_boot_bus_width |= (0x08 << 8);
		putstr(" HS");
	} else {
		putstr(" SDR");
	}
	/* retain boot bus width and boot mode after boot operation */
	emmc_boot_bus_width |= (0x04 << 8);
	putstr("\r\n");
	emmc_boot_bus_width |= ((ACCESS_WRITE_BYTE & 0x3) << 24);
	ret_val = sdmmc_command(cmd_6, emmc_boot_bus_width);
	if (ret_val < 0) {
		goto sdmmc_set_emmc_boot_info_exit;
	}

	putstr("Set eMMC RST_n_FUNCTION:");
	/* RST_n_FUNCTION [162] */
	emmc_rst_n = (162 << 16);
#if defined(CONFIG_EMMC_BOOT_RST_DISABLE)
	emmc_rst_n |= (0x02 << 8) ;
	putstr(" Disable");
#else
	emmc_rst_n |= (0x01 << 8) ;
	putstr(" Enable");
#endif
	putstr("\r\n");
	emmc_rst_n |= ((ACCESS_WRITE_BYTE & 0x3) << 24);
	ret_val = sdmmc_command(cmd_6, emmc_rst_n);
	if (ret_val < 0) {
		goto sdmmc_set_emmc_boot_info_exit;
	}

	putstr("Set eMMC PARTITION_CONFIG:");
	/* PARTITION_CONFIG [179] */
	emmc_partition_config = (179 << 16);
#if defined(CONFIG_EMMC_BOOT_PARTITION_USER)
	emmc_partition_config |= (0x38 << 8);
	putstr(" User");
#elif defined(CONFIG_EMMC_BOOT_PARTITION_BP2)
	emmc_partition_config |= (0x12 << 8);
	putstr(" BP2");
#else
	emmc_partition_config |= (0x09 << 8) ;
	putstr(" BP1");
#endif
	putstr("\r\n");
	emmc_partition_config |= ((ACCESS_WRITE_BYTE & 0x3) << 24);
	ret_val = sdmmc_command(cmd_6, emmc_partition_config);
	if (ret_val < 0) {
		goto sdmmc_set_emmc_boot_info_exit;
	}

sdmmc_set_emmc_boot_info_exit:
	return ret_val;
}

int sdmmc_set_emmc_normal(void)
{
	int ret_val = -1;
	u32 emmc_partition_config;

	/* PARTITION_CONFIG [179] */
	emmc_partition_config = (179 << 16);
#if defined(CONFIG_EMMC_BOOT_PARTITION_USER)
	emmc_partition_config |= (0x38 << 8);
#elif defined(CONFIG_EMMC_BOOT_PARTITION_BP2)
	emmc_partition_config |= (0x10 << 8);
#else
	emmc_partition_config |= (0x08 << 8) ;
#endif
	emmc_partition_config |= ((ACCESS_WRITE_BYTE & 0x3) << 24);
	ret_val = sdmmc_command(cmd_6, emmc_partition_config);

	return ret_val;
}

