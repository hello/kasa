/**
 * @file bld/partition.c
 *
 * Firmware partition information.
 *
 * History:
 *    2009/10/06 - [Evan Chen] created file
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
#if defined(CONFIG_BOOT_MEDIA_SPINAND)
		dev = PART_DEV_SPINAND;
#else
		dev = PART_DEV_SPINOR;
#endif
		break;
	case RCT_BOOT_FROM_BYPASS:
	case RCT_BOOT_FROM_USB:
	case RCT_BOOT_FROM_HIF:
	default:
#if defined(CONFIG_BOOT_MEDIA_EMMC)
		dev = PART_DEV_EMMC;
#elif defined(CONFIG_BOOT_MEDIA_SPINOR)
		dev = PART_DEV_SPINOR;
#elif defined(CONFIG_BOOT_MEDIA_SPINAND)
		dev = PART_DEV_SPINAND;
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

