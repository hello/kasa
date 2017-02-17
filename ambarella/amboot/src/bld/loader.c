/**
 * bld/loader.c
 *
 * History:
 *    2005/03/08 - [Charles Chiou] created file
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
#include <ambhw/spinor.h>
#include <ambhw/cortex.h>
#include <eth/network.h>
#include <sdmmc.h>

#if defined(SECURE_BOOT)
#include "secure/secure_boot.h"
#endif

/*===========================================================================*/
void bld_loader_display_part(const char *s, const flpart_t *part)
{
	if ((part->img_len == 0) || (part->img_len == 0xffffffff))
		return;

	if (part->magic != FLPART_MAGIC) {
		putstr(s);
		putstr(": partition appears damaged...\r\n");
		return;
	}

	putstr(s);
	putstr(": 0x");
	puthex(part->crc32);
	putstr(" ");
	putdec(part->ver_num >> 16);
	putstr(".");
	putdec(part->ver_num & 0xffff);
	putstr(" \t(");
	if ((part->ver_date >> 16) == 0)
		putstr("0000");
	else
		putdec(part->ver_date >> 16);
	putstr("/");
	putdec((part->ver_date >> 8) & 0xff);
	putstr("/");
	putdec(part->ver_date & 0xff);
	putstr(")\t0x");
	puthex(part->mem_addr);
	putstr(" 0x");
	puthex(part->flag);
	putstr(" (");
	putdec(part->img_len);
	putstr(")\r\n");
}

void bld_loader_display_ptb_content(const flpart_table_t *ptb)
{
	int i;

	for (i = 0; i < HAS_IMG_PARTS; i++) {
		bld_loader_display_part(get_part_str(i), &ptb->part[i]);
	}
	putstr("\r\n");
}

/*===========================================================================*/
#if defined(CONFIG_AMBOOT_ENABLE_NAND)
static int bld_loader_load_partition_nand(int part_id,
	u32 mem_addr, u32 img_len, u32 flag)
{
	int ret_val = -1;

	ret_val = nand_read_data((u8 *)mem_addr,
		(u8 *)(flnand.sblk[part_id] * flnand.block_size), img_len);
	if (ret_val == img_len) {
		ret_val = 0;
	} else {
		ret_val = -1;
	}

	return ret_val;
}
#endif

#if defined(CONFIG_AMBOOT_ENABLE_SD)
static int bld_loader_load_partition_sm(int part_id,
	u32 mem_addr, u32 img_len, u32 flag)
{
	u32 sectors = (img_len + sdmmc.sector_size - 1) / sdmmc.sector_size;

	return sdmmc_read_sector(sdmmc.ssec[part_id],
			sectors, (u8 *)mem_addr);
}
#endif

#if defined(CONFIG_AMBOOT_ENABLE_SPINOR)
static int bld_loader_load_partition_spinor(int part_id,
	uintptr_t mem_addr, u32 img_len, u32 flag)
{
	u32 address;
	int ret_val;

	address = flspinor.ssec[part_id] * flspinor.sector_size;

	ret_val = spinor_read_data(address, (void *)mem_addr, img_len);
	if (ret_val < 0)
		return ret_val;

	return 0;
}

#endif

#if defined(CONFIG_AMBOOT_ENABLE_SPINAND)
static int bld_loader_load_partition_spinand(int part_id,
	uintptr_t mem_addr, u32 img_len, u32 flag)
{
	int ret_val;

	ret_val = spinand_read_data((u8 *)mem_addr,
		(u8 *)(flspinand.sblk[part_id] * flspinand.block_size), img_len);

	if (ret_val < 0)
		return ret_val;

	return 0;
}

#endif

int bld_loader_load_partition(int part_id,
	const flpart_table_t *pptb, int verbose)
{
	int ret_val = -1;
	const char *ppart_name;
	const flpart_t *pptb_part;
	u32 mem_addr = 0x0;
	u32 img_len = 0x0;
	u32 boot_from;

	if ((part_id >= HAS_IMG_PARTS) || (part_id == PART_PTB)) {
		if (verbose) {
			putstr("illegal partition ID.\r\n");
		}
		goto bld_loader_load_partition_exit;
	}
	ppart_name = get_part_str(part_id);
	pptb_part = &pptb->part[part_id];

	if (pptb_part->magic != FLPART_MAGIC) {
		if (verbose) {
			putstr(ppart_name);
			putstr(" partition appears damaged... skipping\r\n");
		}
		goto bld_loader_load_partition_exit;
	}
	if ((pptb_part->mem_addr < DRAM_START_ADDR) ||
		(pptb_part->mem_addr > (DRAM_START_ADDR + DRAM_SIZE - 1))){
		if (verbose) {
			putstr(ppart_name);
			putstr(" wrong load address... skipping\r\n");
		}
		goto bld_loader_load_partition_exit;
	}
	if ((pptb_part->img_len == 0) || (pptb_part->img_len == 0xffffffff)) {
		if (verbose) {
			putstr(ppart_name);
			putstr(" image absent... skipping\r\n");
		}
		goto bld_loader_load_partition_exit;
	}
	if (pptb_part->flag & PART_NO_LOAD) {
		if (verbose) {
			putstr(ppart_name);
			putstr(" has no-load flag set... skipping\r\n");
		}
		goto bld_loader_load_partition_exit;
	}

	img_len = pptb_part->img_len;
	mem_addr = pptb_part->mem_addr;
	if (verbose) {
		putstr("loading ");
		putstr(ppart_name);
		putstr(" to 0x");
		puthex(mem_addr);
		putstr("\r\n");
	}

	boot_from = get_part_dev(part_id);
#if defined(CONFIG_AMBOOT_ENABLE_NAND)
	if ((ret_val < 0) && (boot_from & PART_DEV_NAND)) {
		ret_val = bld_loader_load_partition_nand(part_id,
			mem_addr, img_len, pptb_part->flag);
	}
#endif
#if defined(CONFIG_AMBOOT_ENABLE_SD)
	if ((ret_val < 0) && (boot_from & PART_DEV_EMMC)) {
		ret_val = bld_loader_load_partition_sm(part_id,
			mem_addr, img_len, pptb_part->flag);
	}
#endif
#if defined(CONFIG_AMBOOT_ENABLE_SPINOR)
	if ((ret_val < 0) && (boot_from & PART_DEV_SPINOR)) {
		ret_val = bld_loader_load_partition_spinor(part_id,
			mem_addr, img_len, pptb_part->flag);
	}
#endif

#if defined(CONFIG_AMBOOT_ENABLE_SPINAND)
	if ((ret_val < 0) && (boot_from & PART_DEV_SPINAND)) {
		ret_val = bld_loader_load_partition_spinand(part_id,
			mem_addr, img_len, pptb_part->flag);
	}
#endif

	if (ret_val < 0) {
		if (verbose) {
			putstr(ppart_name);
			putstr("[");
			puthex(boot_from);
			putstr("] - load_partition() failed!\r\n");
		}
		goto bld_loader_load_partition_exit;
	}

bld_loader_load_partition_exit:
	return ret_val;
}

/*===========================================================================*/
int bld_loader_boot_partition(int verbose, u32 *pjump_addr,
	u32 *pinitrd2_start, u32 *pinitrd2_size)
{
	int ret_val;
	flpart_table_t ptb;
	u32 os_start = 0;
	u32 os_len = 0;
	u32 rmd_start = 0;
	u32 rmd_size = 0;

	ret_val = flprog_get_part_table(&ptb);
	if (ret_val < 0)
		return ret_val;

	if (verbose) {
		bld_loader_display_ptb_content(&ptb);
	}

	ret_val = bld_loader_load_partition(PART_PRI, &ptb, verbose);
	if (ret_val < 0) {
		ret_val = bld_loader_load_partition(PART_SEC, &ptb, verbose);
		if (ret_val < 0)
			return ret_val;

		os_start = ptb.part[PART_SEC].mem_addr;
		os_len = ptb.part[PART_SEC].img_len;
	} else {
		os_start = ptb.part[PART_PRI].mem_addr;
		os_len = ptb.part[PART_PRI].img_len;
	}

#if defined(SECURE_BOOT)
	if (ptb.dev.need_generate_firmware_hw_signature) {
		ret_val = generate_firmware_hw_signature((unsigned char *) os_start, os_len);
		if (0 == ret_val) {
			putstr("[secure boot]: generate hw signature done\r\n");
		} else {
			putstr("[secure boot]: generate hw signature fail\r\n");
			return FLPROG_ERR_FIRM_HW_SIGN_FAIL;
		}

		ptb.dev.need_generate_firmware_hw_signature = 0;
		flprog_set_part_table(&ptb);
	} else {
		if (0 == verify_firmware_hw_signature((unsigned char *) os_start, os_len)) {
			putstr("[secure boot]: verify firmware OK\r\n");
		} else {
			putstr("[secure boot]: verify firmware fail\r\n");
			return FLPROG_ERR_FIRM_HW_SIGN_VERIFY_FAIL;
		}
	}
#endif

	ret_val = bld_loader_load_partition(PART_RMD, &ptb, verbose);
	if (ret_val == 0x0) {
		rmd_start = ptb.part[PART_RMD].mem_addr;
		rmd_size  = ptb.part[PART_RMD].img_len;
	}

	if (pjump_addr) {
		*pjump_addr = os_start;
	}
	if (pinitrd2_start) {
		*pinitrd2_start = rmd_start;
	}
	if (pinitrd2_size) {
		*pinitrd2_size = rmd_size;
	}

	return 0;
}

int boot(const char *cmdline, int verbose)
{
	u32 jump_addr = 0, rmd_start = 0, rmd_size = 0;
	u32 cortex_jump, cortex_atag;
	int ret_val;

	ret_val = bld_loader_boot_partition(verbose,
			&jump_addr, &rmd_start, &rmd_size);
	if (ret_val < 0) {
		goto boot_exit;
	}

	if (verbose) {
		putstr("Jumping to 0x");
		puthex(jump_addr);
		putstr(" ...\r\n");
	}
#if defined(CONFIG_AMBOOT_ENABLE_ETH)
	bld_net_down();
#endif

#if defined(AMBOOT_DEV_BOOT_CORTEX)
	cortex_jump = ARM11_TO_CORTEX((u32)cortex_processor_start);
#elif defined(AMBOOT_BOOT_SECONDARY_CORTEX)
	cortex_jump = (u32)secondary_cortex_jump;
#else
	cortex_jump = 0;
#endif

#if defined(CONFIG_AMBOOT_BD_FDT_SUPPORT)
	cortex_atag = fdt_update_tags((void *)jump_addr,
		cmdline, cortex_jump, rmd_start, rmd_size, verbose);
#elif defined(CONFIG_AMBOOT_BD_ATAG_SUPPORT)
	cortex_atag = setup_tags((void *)jump_addr,
		cmdline, cortex_jump, rmd_start, rmd_size, verbose);
#endif

#if defined(AMBOOT_BOOT_SECONDARY_CORTEX)
	bld_boot_secondary_cortex();
#endif

#if defined(AMBOOT_DEV_BOOT_CORTEX)
	*cortex_atag_data = cortex_atag;
	ret_val = bld_cortex_boot(verbose, jump_addr);
#else
	jump_to_kernel((void *)jump_addr, cortex_atag);
#endif

boot_exit:
	return ret_val;
}

/*===========================================================================*/
static int bld_loader_bios_partition(int verbose, u32 *pjump_addr)
{
	flpart_table_t ptb;
	int ret_val;

	ret_val = flprog_get_part_table(&ptb);
	if (ret_val < 0)
		return ret_val;

	if (verbose) {
		bld_loader_display_ptb_content(&ptb);
	}

	ret_val = bld_loader_load_partition(PART_PBA, &ptb, verbose);
	if (ret_val < 0)
		return ret_val;

	if (pjump_addr) {
		*pjump_addr = ptb.part[PART_PBA].mem_addr;
	}

	return 0;
}

int bios(const char *cmdline, int verbose)
{
	u32 jump_addr = 0;
	u32 cortex_jump;
	u32 cortex_atag;
	int ret_val;

	ret_val = bld_loader_bios_partition(verbose, &jump_addr);
	if (ret_val < 0) {
		goto bios_exit;
	}

	if (verbose) {
		putstr("Jumping to 0x");
		puthex(jump_addr);
		putstr(" ...\r\n");
	}
#if defined(CONFIG_AMBOOT_ENABLE_ETH)
	bld_net_down();
#endif

#if defined(AMBOOT_DEV_BOOT_CORTEX)
	cortex_jump = ARM11_TO_CORTEX((u32)cortex_processor_start);
#elif defined(AMBOOT_BOOT_SECONDARY_CORTEX)
	cortex_jump = (u32)secondary_cortex_jump;
#else
	cortex_jump = 0;
#endif

#if defined(CONFIG_AMBOOT_BD_FDT_SUPPORT)
	cortex_atag = fdt_update_tags((void *)jump_addr,
			cmdline, cortex_jump, 0, 0, verbose);
#elif defined(CONFIG_AMBOOT_BD_ATAG_SUPPORT)
	cortex_atag = setup_tags((void *)jump_addr,
			cmdline, cortex_jump, 0, 0, verbose);
#endif

#if defined(AMBOOT_DEV_BOOT_CORTEX)
	*cortex_atag_data = cortex_atag;
	ret_val = bld_cortex_boot(verbose, jump_addr);
#else
	jump_to_kernel((void *)jump_addr, cortex_atag);
#endif

bios_exit:
	return ret_val;
}
