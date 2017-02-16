/**
 * bld/bapi.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>
#include <bapi.h>

/* ==========================================================================*/
static struct ambarella_bapi_s	*bapi_info = NULL;

/* ==========================================================================*/
static void bld_bapi_info()
{
	if (bapi_info) {
		putstr("bapi_info: 0x");
		puthex((u32)bapi_info);
		putstr("\r\nmagic: 0x");
		puthex(bapi_info->magic);
		putstr("\r\nversion: 0x");
		puthex(bapi_info->version);
		putstr("\r\nsize: 0x");
		puthex(bapi_info->size);
		putstr("\r\ncrc: 0x");
		puthex(bapi_info->crc);
		putstr("\r\nmode: 0x");
		puthex(bapi_info->mode);
		putstr("\r\nblock_dev: 0x");
		puthex(bapi_info->block_dev);
		putstr("\r\nblock_start: 0x");
		puthex(bapi_info->block_start);
		putstr("\r\nblock_num: 0x");
		puthex(bapi_info->block_num);
		putstr("\r\n\r\n");

		putstr("reboot_info: 0x");
		puthex((u32)&bapi_info->reboot_info);
		putstr("\r\nmagic: 0x");
		puthex(bapi_info->reboot_info.magic);
		putstr("\r\nmode: 0x");
		puthex(bapi_info->reboot_info.mode);
		putstr("\r\nflag: 0x");
		puthex(bapi_info->reboot_info.flag);
		putstr("\r\n\r\n");

		putstr("fb_start: 0x");
		puthex(bapi_info->fb_start);
		putstr("\r\nfb_length: 0x");
		puthex(bapi_info->fb_length);
		putstr("\r\n\r\n");

		putstr("aoss_info: 0x");
		puthex((u32)&bapi_info->aoss_info);
		putstr("\r\nmagic: 0x");
		puthex(bapi_info->aoss_info.magic);
		putstr("\r\ntotal_pages: 0x");
		puthex(bapi_info->aoss_info.total_pages);
		putstr("\r\ncopy_pages: 0x");
		puthex(bapi_info->aoss_info.copy_pages);
		putstr("\r\npage_info: 0x");
		puthex(bapi_info->aoss_info.page_info);
		putstr("\r\n");
	}
}

static int bld_bapi_aoss_check(int verbose)
{
	int ret_val = 0;

	if (bapi_info == NULL) {
		if (verbose) {
			putstr("bapi_info NULL.\r\n");
		}
		ret_val = -1;
	}

	return ret_val;
}

/* ==========================================================================*/
int bld_bapi_init(int verbose)
{
	int ret_val = 0;
	unsigned char *pbld_bapi_buf;
	unsigned char *pppm_end;
	struct ambarella_bapi_aoss_s *aoss_info = NULL;
	u32 aoss_size;

	pbld_bapi_buf = (unsigned char *)(DRAM_START_ADDR + 0x000F0000);
	pppm_end = (unsigned char *)(KERNEL_RAM_START & (~SIZE_1MB_MASK));
	if (pppm_end < &pbld_bapi_buf[DEFAULT_BAPI_FB_OFFSET]) {
		ret_val = -1;
		goto bld_bapi_init_exit;
	}

	bapi_info = (struct ambarella_bapi_s *)(
		&pbld_bapi_buf[DEFAULT_BAPI_OFFSET]);
	bapi_info->magic = DEFAULT_BAPI_MAGIC;
	bapi_info->version = DEFAULT_BAPI_VERSION;
	bapi_info->size = DEFAULT_BAPI_FB_OFFSET;

	//bapi_info->reboot_info.flag = AMBARELLA_BAPI_REBOOT_HIBERNATE;
	bapi_info->reboot_info.flag = 0;
	if (amboot_bsp_self_refresh_enter) {
		bapi_info->reboot_info.flag |=
			AMBARELLA_BAPI_REBOOT_SELFREFERESH;
	}

	bapi_info->fb_start = (u32)(&pbld_bapi_buf[DEFAULT_BAPI_FB_OFFSET]);
	bapi_info->fb_length = (u32)(pppm_end -
		&pbld_bapi_buf[DEFAULT_BAPI_FB_OFFSET]);
	memzero(&bapi_info->fb0_info, sizeof(struct ambarella_bapi_fb_info_s));
	memzero(&bapi_info->fb1_info, sizeof(struct ambarella_bapi_fb_info_s));
#if defined(CONFIG_AMBOOT_BAPI_ZERO_FB)
	memzero((void *)bapi_info->fb_start, bapi_info->fb_length);
#endif

	aoss_info = &bapi_info->aoss_info;
	aoss_size = (bapi_info->size - DEFAULT_BAPI_SIZE);
	memzero(aoss_info, (aoss_size + DEFAULT_BAPI_AOSS_SIZE));
	aoss_info->page_info = ARM11_TO_CORTEX((u32)aoss_info +
		DEFAULT_BAPI_AOSS_SIZE);
	aoss_info->total_pages = aoss_size /
		sizeof(struct ambarella_bapi_aoss_page_info_s);
	if (verbose) {
		bld_bapi_info();
	}

bld_bapi_init_exit:
	return ret_val;
}


/* ==========================================================================*/
int bld_bapi_reboot_set(int verbose, u32 mode)
{
	int ret_val = 0;

	ret_val = bld_bapi_aoss_check(verbose);
	if (ret_val) {
		goto bld_bapi_reboot_set_exit;
	}

	bapi_info->reboot_info.magic = DEFAULT_BAPI_REBOOT_MAGIC;
	bapi_info->reboot_info.mode = mode;

	if (verbose) {
		putstr("mode: ");
		puthex(mode);
		putstr("\r\n");
	}

bld_bapi_reboot_set_exit:
	return ret_val;
}

int bld_bapi_reboot_get(int verbose, u32 *mode)
{
	int ret_val = 0;

	ret_val = bld_bapi_aoss_check(verbose);
	if (ret_val) {
		goto bld_bapi_reboot_get_exit;
	}

	if (bapi_info->reboot_info.magic == DEFAULT_BAPI_REBOOT_MAGIC) {
		*mode = bapi_info->reboot_info.mode;
	} else {
		*mode = AMBARELLA_BAPI_CMD_REBOOT_NORMAL;
		ret_val = -1;
	}

	if (verbose) {
		putstr("mode: 0x");
		puthex(*mode);
		putstr("\r\n");
		putstr("mode add: 0x");
		puthex((u32)&bapi_info->reboot_info.mode);
		putstr(" val: 0x");
		puthex(bapi_info->reboot_info.mode);
		putstr("\r\n");
	}

bld_bapi_reboot_get_exit:
	return ret_val;
}

/* ==========================================================================*/
int bld_bapi_set_fb_info(int verbose, u32 fb_id, int xres, int yres,
	int xvirtual, int yvirtual, int format, u32 bits_per_pixel)
{
	int ret_val = -1;
	u32 length;
	u32 max_length;

	ret_val = bld_bapi_aoss_check(verbose);
	if (ret_val) {
		goto bld_bapi_set_fb_info_exit;
	}

	length = xvirtual * yvirtual * bits_per_pixel / 8;
	if (fb_id == 0) {
		max_length = (length + SIZE_1MB_MASK) & (~SIZE_1MB_MASK);
		if (bapi_info->fb_length >= max_length) {
			bapi_info->fb0_info.xres = xres;
			bapi_info->fb0_info.yres = yres;
			bapi_info->fb0_info.xvirtual = xvirtual;
			bapi_info->fb0_info.yvirtual = yvirtual;
			bapi_info->fb0_info.format = format;
			bapi_info->fb0_info.bits_per_pixel = bits_per_pixel;
			bapi_info->fb0_info.fb_start =
				ARM11_TO_CORTEX(bapi_info->fb_start);
			bapi_info->fb0_info.fb_length = max_length;
			ret_val = 0;
		}
	} else if (fb_id == 1) {
		max_length = length;
		max_length += bapi_info->fb0_info.fb_length;
		if (bapi_info->fb_length >= max_length) {
			bapi_info->fb1_info.fb_start =
				bapi_info->fb0_info.fb_start +
				bapi_info->fb0_info.fb_length;
			max_length = bapi_info->fb_length -
				bapi_info->fb0_info.fb_length;
			bapi_info->fb1_info.xres = xres;
			bapi_info->fb1_info.yres = yres;
			bapi_info->fb1_info.xvirtual = xvirtual;
			bapi_info->fb1_info.yvirtual = yvirtual;
			bapi_info->fb1_info.format = format;
			bapi_info->fb1_info.bits_per_pixel = bits_per_pixel;
			bapi_info->fb1_info.fb_length = max_length;
			ret_val = 0;
		}
	}

bld_bapi_set_fb_info_exit:
	return ret_val;
}

int bld_bapi_get_splash_info(u32 fb_id,
	struct ambarella_bapi_fb_info_s *pfb_info)
{
	int ret_val;

	ret_val = bld_bapi_aoss_check(0);
	if (ret_val) {
		goto bld_bapi_get_splash_info_exit;
	}

	ret_val = -1;
	if (pfb_info) {
		if (fb_id == 0) {
			*pfb_info = bapi_info->fb0_info;
			pfb_info->fb_start =
				CORTEX_TO_ARM11(pfb_info->fb_start);
			ret_val = 0;
		} else if (fb_id == 1) {
			*pfb_info = bapi_info->fb1_info;
			pfb_info->fb_start =
				CORTEX_TO_ARM11(pfb_info->fb_start);
			ret_val = 0;
		}
	}

bld_bapi_get_splash_info_exit:
	return ret_val;
}

