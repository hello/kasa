/**
 * @file bld/partition.c
 *
 * Firmware partition information.
 *
 * History:
 *    2009/10/06 - [Evan Chen] created file
 *    2014/02/13 - [Anthony Ginger] Amboot V2
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <amboot.h>
#include <fio/partition.h>

/*===========================================================================*/
static const char *g_part_str[HAS_IMG_PARTS + 1] = {
	"bst", "bld", "ptb", "spl", "pba",
	"pri", "sec", "bak", "rmd", "rom",
	"dsp", "lnx", "swp", "add", "adc",
	"raw"};

static u32 g_part_dev[HAS_IMG_PARTS + 1] = {
	PART_BST_DEV,
	PART_BLD_DEV,
	PART_PTB_DEV,
	PART_SPL_DEV,
	PART_PBA_DEV,
	PART_PRI_DEV,
	PART_SEC_DEV,
	PART_BAK_DEV,
	PART_RMD_DEV,
	PART_ROM_DEV,
	PART_DSP_DEV,
	PART_LNX_DEV,
	PART_SWP_DEV,
	PART_ADD_DEV,
	PART_ADC_DEV,
	PART_RAW_DEV,
};

/*===========================================================================*/
void get_part_size(int *part_size)
{
	part_size[PART_BST] = AMBOOT_BST_SIZE;
	part_size[PART_PTB] = AMBOOT_PTB_SIZE;
	part_size[PART_BLD] = AMBOOT_BLD_SIZE;
	part_size[PART_SPL] = AMBOOT_SPL_SIZE;
	part_size[PART_PBA] = AMBOOT_PBA_SIZE;
	part_size[PART_PRI] = AMBOOT_PRI_SIZE;
	part_size[PART_SEC] = AMBOOT_SEC_SIZE;
	part_size[PART_BAK] = AMBOOT_BAK_SIZE;
	part_size[PART_RMD] = AMBOOT_RMD_SIZE;
	part_size[PART_ROM] = AMBOOT_ROM_SIZE;
	part_size[PART_DSP] = AMBOOT_DSP_SIZE;
	part_size[PART_LNX] = AMBOOT_LNX_SIZE;
	part_size[PART_SWP] = AMBOOT_SWP_SIZE;
	part_size[PART_ADD] = AMBOOT_ADD_SIZE;
	part_size[PART_ADC] = AMBOOT_ADC_SIZE;
}

const char *get_part_str(int part_id)
{
	K_ASSERT(part_id <= HAS_IMG_PARTS);
	return g_part_str[part_id];
}

u32 set_part_dev(u32 boot_from)
{
	u32 i;
	u32 dev;

	switch (boot_from) {
	case RCT_BOOT_FROM_NAND:
		dev = PART_DEV_NAND;
		break;
	case RCT_BOOT_FROM_EMMC:
		dev = PART_DEV_EMMC;
		break;
	case RCT_BOOT_FROM_SPINOR:
		dev = PART_DEV_SPINOR;
		break;
	case RCT_BOOT_FROM_BYPASS:
	case RCT_BOOT_FROM_USB:
	case RCT_BOOT_FROM_HIF:
	default:
#if defined(CONFIG_BOOT_MEDIA_EMMC)
		dev = PART_DEV_EMMC;
#elif defined(CONFIG_BOOT_MEDIA_SPINOR)
		dev = PART_DEV_SPINOR;
#else
		dev = PART_DEV_NAND;
#endif
		break;
	}

	for (i = 0; i <= HAS_IMG_PARTS; i++) {
		if (g_part_dev[i] == PART_DEV_AUTO) {
			g_part_dev[i] = dev;
		}
		// PTB need on all dev
		g_part_dev[PART_PTB] |= g_part_dev[i];
	}

	return g_part_dev[PART_PTB];
}

u32 get_part_dev(u32 part_id)
{
	K_ASSERT(part_id <= HAS_IMG_PARTS);
	return g_part_dev[part_id];
}

