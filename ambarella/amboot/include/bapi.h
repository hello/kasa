/**
 * bapi.h
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

#ifndef __BAPI_H__
#define __BAPI_H__

/*===========================================================================*/
#define DEFAULT_BAPI_AOSS_PART			PART_SWP
#define DEFAULT_BAPI_OFFSET			(0x00002000)
#define DEFAULT_BAPI_BACKUP_OFFSET		(0x00001000)
#define DEFAULT_BAPI_FB_OFFSET			(0x00010000)
#define DEFAULT_BAPI_NAND_OFFSET		(8)
#define DEFAULT_BAPI_SM_OFFSET			(8)

#define DEFAULT_BAPI_TAG_MAGIC			(0x19450107)
#define DEFAULT_BAPI_MAGIC			(0x19790110)
#define DEFAULT_BAPI_VERSION			(0x00000001)
#define DEFAULT_BAPI_SIZE			(4096)
#define DEFAULT_BAPI_AOSS_SIZE			(1024)

#define DEFAULT_BAPI_AOSS_MAGIC			(0x19531110)

#define DEFAULT_BAPI_REBOOT_MAGIC		(0x4a32e9b0)
#define AMBARELLA_BAPI_CMD_REBOOT_NORMAL	(0xdeadbeaf)
#define AMBARELLA_BAPI_CMD_REBOOT_RECOVERY	(0x5555aaaa)
#define AMBARELLA_BAPI_CMD_REBOOT_FASTBOOT	(0x555aaaa5)
#define AMBARELLA_BAPI_CMD_REBOOT_SELFREFERESH	(0x55aaaa55)
#define AMBARELLA_BAPI_CMD_REBOOT_HIBERNATE	(0x5aaaa555)

#define AMBARELLA_BAPI_REBOOT_HIBERNATE		(0x1 << 0)
#define AMBARELLA_BAPI_REBOOT_SELFREFERESH	(0x1 << 1)

/* ==========================================================================*/

/* ==========================================================================*/
#ifndef __ASM__
/* ==========================================================================*/

enum ambarella_bapi_cmd_e {
	AMBARELLA_BAPI_CMD_INIT			= 0x0000,

	AMBARELLA_BAPI_CMD_AOSS_INIT		= 0x1000,
	AMBARELLA_BAPI_CMD_AOSS_COPY_PAGE	= 0x1001,
	AMBARELLA_BAPI_CMD_AOSS_SAVE		= 0x1002,

	AMBARELLA_BAPI_CMD_SET_REBOOT_INFO	= 0x2000,
	AMBARELLA_BAPI_CMD_CHECK_REBOOT		= 0x2001,

	AMBARELLA_BAPI_CMD_UPDATE_FB_INFO	= 0x3000,
};

struct ambarella_bapi_aoss_page_info_s {
	u32					src;
	u32					dst;
	u32					size;
};

struct ambarella_bapi_aoss_s {
	u32					fn_pri[256 - 4];
	u32					magic;
	u32					total_pages;
	u32					copy_pages;
	u32					page_info;
};

struct ambarella_bapi_reboot_info_s {
	u32					magic;
	u32					mode;
	u32					flag;
	u32					rev;
};

struct ambarella_bapi_fb_info_s {
	int					xres;
	int					yres;
	int					xvirtual;
	int					yvirtual;
	int					format;
	u32					fb_start;
	u32					fb_length;
	u32					bits_per_pixel;
};

struct ambarella_bapi_s {
	u32					magic;
	u32					version;
	int					size;
	u32					crc;
	u32					mode;
	u32					block_dev;
	u32					block_start;
	u32					block_num;
	u32					rev0[64 - 8];
	struct ambarella_bapi_reboot_info_s	reboot_info;
	u32					fb_start;
	u32					fb_length;
	struct ambarella_bapi_fb_info_s		fb0_info;
	struct ambarella_bapi_fb_info_s		fb1_info;
	u32					rev1[64 - 4 - 8 - 8 - 2];
	u32					debug[128];
	u32					rev2[1024 - 128 - 128 - 256];
	struct ambarella_bapi_aoss_s		aoss_info;
};

struct ambarella_bapi_tag_s {
	u32					magic;
	u32					pbapi_info;
};

/* ==========================================================================*/
extern int aoss_init(u32, u32, u32, u32);
extern void aoss_outcoming(u32, u32, u32, u32);

extern int bld_bapi_init(int verbose);

extern int bld_bapi_reboot_set(int verbose, u32 mode);
extern int bld_bapi_reboot_get(int verbose, u32 *mode);

extern int bld_bapi_set_fb_info(int verbose, u32 fb_id, int xres, int yres,
	int xvirtual, int yvirtual, int format, u32 bits_per_pixel);
extern int bld_bapi_get_splash_info(u32 fb_id,
	struct ambarella_bapi_fb_info_s *pfb_info);

/* ==========================================================================*/
#endif
/* ==========================================================================*/

#endif

