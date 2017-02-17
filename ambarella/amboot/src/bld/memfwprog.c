/**
 * bld/memfwprog.c
 *
 * History:
 *    2005/02/27 - [Charles Chiou] created file
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
#include <ambhw/vic.h>
#include <ambhw/gpio.h>
#include <ambhw/uart.h>
#include <ambhw/nand.h>
#include <ambhw/spinor.h>
#include <libfdt.h>
#include <sdmmc.h>

/* ==========================================================================*/
static const char *MEMFWPROG_LOGO =
	"\r\n" \
	"------------------------------------------------------\r\n" \
	"In-memory Firmware Flash Programming Utility (by C.C.)\r\n" \
	"Ambarella(R) Copyright (C) 2004-2008\r\n"		     \
	"------------------------------------------------------\r\n";

extern u32 firmware_start;
extern u32 begin_bst_image;
extern u32 end_bst_image;
extern u32 begin_bld_image;
extern u32 end_bld_image;
extern u32 begin_splash_image;
extern u32 end_splash_image;
extern u32 begin_pba_image;
extern u32 end_pba_image;
extern u32 begin_kernel_image;
extern u32 end_kernel_image;
extern u32 begin_secondary_image;
extern u32 end_secondary_image;
extern u32 begin_backup_image;
extern u32 end_backup_image;
extern u32 begin_ramdisk_image;
extern u32 end_ramdisk_image;
extern u32 begin_romfs_image;
extern u32 end_romfs_image;
extern u32 begin_dsp_image;
extern u32 end_dsp_image;
extern u32 begin_lnx_image;
extern u32 end_lnx_image;
extern u32 begin_swp_image;
extern u32 end_swp_image;
extern u32 begin_add_image;
extern u32 end_add_image;
extern u32 begin_adc_image;
extern u32 end_adc_image;
#if defined(CONFIG_AMBOOT_BD_FDT_SUPPORT)
extern u8 dt_blob_start[];
extern u8 dt_blob_abs_end[];
#endif

/* ==========================================================================*/
extern void hook_pre_memfwprog(fwprog_cmd_t *) __attribute__ ((weak));
extern void hook_post_memfwprog(fwprog_cmd_t *) __attribute__ ((weak));

extern unsigned int __memfwprog_result;
extern unsigned int __memfwprog_command;

/* ==========================================================================*/
static u32 get_bst_id(void)
{
	u32 bst_id = 0;

#if defined(BST_IMG_ID)
	return BST_IMG_ID;
#endif

#if defined(BST_SEL0_GPIO)
	gpio_config_sw_in(BST_SEL0_GPIO);
	bst_id = gpio_get(BST_SEL0_GPIO);
#endif

#if defined(BST_SEL1_GPIO)
	gpio_config_sw_in(BST_SEL1_GPIO);
	bst_id |= gpio_get(BST_SEL1_GPIO) << 1;
#endif

#if defined(BST_SEL0_GPIO) || defined(BST_SEL1_GPIO)
	putstr("BST ID = ");
	putdec(bst_id);
	putstr("\r\n");
#endif

	return bst_id;
}

static int select_bst_fw(u8 *img_in, int len_in, u8 **img_out, int *len_out)
{
	int bst_id, fw_size;

	bst_id = get_bst_id();
	fw_size = AMBOOT_BST_FIXED_SIZE + sizeof(partimg_header_t);

	/* Sanity check */
	if (len_in % fw_size != 0 || (bst_id + 1) * fw_size > len_in ) {
		putstr("invalid bst_id or firmware length: ");
		putdec(bst_id);
		putstr(", ");
		putdec(len_in);
		putstr("(");
		putdec(fw_size);
		putstr(")\r\n");
		return FLPROG_ERR_LENGTH;
	}

	*img_out = img_in + fw_size * bst_id;
	*len_out = fw_size;

	return 0;
}

int main(void)
{
	int ret_val = 0;
	u32 boot_from;
	u32 part_dev;
	u8 code;
	u8 *image;
	int part_len[HAS_IMG_PARTS];
	int begin_image[HAS_IMG_PARTS];
	u8 buffer[512];
	int i;
	flpart_table_t ptb;
	fwprog_result_t *result = (fwprog_result_t *)&__memfwprog_result;

	malloc_init();

	enable_fio_dma();
	rct_reset_fio();
	fio_exit_random_mode();

#if defined(CONFIG_AMBOOT_ENABLE_GPIO)
	gpio_init();
#endif

	/* Initialize the UART */
	uart_init();
	putstr("\x1b[4l");
	putstr("\r\n");

	putstr(MEMFWPROG_LOGO);

#if defined(CONFIG_BOOT_MEDIA_EMMC)
	boot_from = RCT_BOOT_FROM_EMMC;
#elif defined(CONFIG_BOOT_MEDIA_SPINOR)
	boot_from = RCT_BOOT_FROM_SPINOR;
#elif defined(CONFIG_BOOT_MEDIA_SPINAND)
	boot_from = RCT_BOOT_FROM_SPINOR;
#else
	boot_from = RCT_BOOT_FROM_NAND;
#endif
	rct_show_boot_from(boot_from);
	rct_show_pll();

	part_dev = set_part_dev(boot_from);
#if defined(CONFIG_AMBOOT_ENABLE_SD)
	if (part_dev & PART_DEV_EMMC) {
		sdmmc_init_mmc(0, SDMMC_MODE_AUTO, -1, 1);
	}
#endif
#if defined(CONFIG_AMBOOT_ENABLE_NAND)
	if (part_dev & PART_DEV_NAND) {
		nand_init();
		nand_reset();
#if defined(CONFIG_NAND_USE_FLASH_BBT)
		nand_scan_bbt(0);
#endif
	}
#endif
#if defined(CONFIG_AMBOOT_ENABLE_SPINOR)
	if (part_dev & PART_DEV_SPINOR) {
		spinor_init();
	}
#endif
#if defined(CONFIG_AMBOOT_ENABLE_SPINAND)
	if (part_dev & PART_DEV_SPINAND) {
		spinand_init();
#if defined(CONFIG_SPINAND_USE_FLASH_BBT)
		spinand_scan_bbt(0);
#endif
	}
#endif
	if (hook_pre_memfwprog != 0x0) {
		fwprog_cmd_t *fwprog_cmd;
		fwprog_cmd = (fwprog_cmd_t *) &__memfwprog_command;
		putstr("\r\nhook_pre_memfwprog\r\n");
		hook_pre_memfwprog(fwprog_cmd);
	}

	memzero(buffer, sizeof(buffer));
	memzero(result, sizeof(*result));
	result->magic = FWPROG_RESULT_MAGIC;

	/* Calculate the firmware payload offsets of images */
	begin_image[PART_BST] = begin_bst_image;
	begin_image[PART_PTB] = 0;
	begin_image[PART_BLD] = begin_bld_image;
	begin_image[PART_SPL] = begin_splash_image;
	begin_image[PART_PBA] = begin_pba_image;
	begin_image[PART_PRI] = begin_kernel_image;
	begin_image[PART_SEC] = begin_secondary_image;
	begin_image[PART_BAK] = begin_backup_image;
	begin_image[PART_RMD] = begin_ramdisk_image;
	begin_image[PART_ROM] = begin_romfs_image;
	begin_image[PART_DSP] = begin_dsp_image;
	begin_image[PART_LNX] = begin_lnx_image;
	begin_image[PART_SWP] = begin_swp_image;
	begin_image[PART_ADD] = begin_add_image;
	begin_image[PART_ADC] = begin_adc_image;

	part_len[PART_BST] = end_bst_image - begin_bst_image;
	part_len[PART_PTB] = 0;
	part_len[PART_BLD] = end_bld_image - begin_bld_image;
	part_len[PART_SPL] = end_splash_image - begin_splash_image;
	part_len[PART_PBA] = end_pba_image - begin_pba_image;
	part_len[PART_PRI] = end_kernel_image - begin_kernel_image;
	part_len[PART_SEC] = end_secondary_image - begin_secondary_image;
	part_len[PART_BAK] = end_backup_image - begin_backup_image;
	part_len[PART_RMD] = end_ramdisk_image - begin_ramdisk_image;
	part_len[PART_ROM] = end_romfs_image - begin_romfs_image;
	part_len[PART_DSP] = end_dsp_image - begin_dsp_image;
	part_len[PART_LNX] = end_lnx_image - begin_lnx_image;
	part_len[PART_SWP] = end_swp_image - begin_swp_image;
	part_len[PART_ADD] = end_add_image - begin_add_image;
	part_len[PART_ADC] = end_adc_image - begin_adc_image;

	for (i = PART_BST; i < HAS_IMG_PARTS; i++) {
		if (part_len[i] > 0) {
			putstr("\r\n");
			putstr(get_part_str(i));
			putstr(" code found in firmware!\r\n");

			image = (u8 *)(firmware_start + begin_image[i]);

			if (i == PART_BST) {
				ret_val = select_bst_fw(image, part_len[i],
							&image, &part_len[i]);
				if (ret_val < 0)
					goto select_bst_err;
			}

			output_header((partimg_header_t *)image, part_len[i]);
			ret_val = flprog_write_partition(i, image, part_len[i]);
select_bst_err:
			code = ret_val < 0 ? -ret_val : ret_val;
			result->flag[i] = FWPROG_RESULT_MAKE(code, part_len[i]);
			if (ret_val == FLPROG_ERR_PROG_IMG) {
				result->bad_blk_info |= (0x1 << i);
			}
			output_failure(ret_val);
		}
	}

#if defined(CONFIG_AMBOOT_BD_FDT_SUPPORT)
	if ((int)(dt_blob_abs_end - dt_blob_start) > 0 && part_len[PART_BLD] > 0) {
		void *fdt = dt_blob_start;

		if (fdt && fdt_check_header(fdt) == 0) {
			putstr("\r\nDTB found in firmware!\r\n");
			fdt_update_cmdline(fdt, CONFIG_AMBOOT_BD_CMDLINE);
			ret_val = flprog_set_dtb(fdt,
				(dt_blob_abs_end - dt_blob_start), 1);
			output_failure(ret_val);
		} else {
			putstr("\r\nInvalid DTB!\r\n");
		}
	}
#endif

	putstr("\r\n------ Report ------\r\n");
	for (i = 0; i < HAS_IMG_PARTS; i++) {
		if (i == PART_PTB)
			continue;
		output_report(get_part_str(i), result->flag[i]);
	}

	flprog_get_part_table(&ptb);
	if (ptb.dev.magic != FLPART_MAGIC) {
		putstr("sanitized ptb.dev\r\n");
		flprog_set_part_table(&ptb);
	}

	if (hook_post_memfwprog != 0x0) {
		fwprog_cmd_t *fwprog_cmd;
		fwprog_cmd = (fwprog_cmd_t *) &__memfwprog_command;
		putstr("\r\nhook_post_memfwprog\r\n");
		hook_post_memfwprog(fwprog_cmd);
	}

	putstr("\r\n\t- Program Terminated -\r\n");

	if (((fwprog_cmd_t*)&__memfwprog_command)->cmd[7] == MEMFWPROG_CMD_VERIFY) {
		void (*bld_start)() = ((void(*)())AMBOOT_BLD_RAM_START);
		putstr("\r\n\t- Re-enter USB Download Mode to Verify -\r\n");
		bld_start();
	}

	return 0;
}

