/**
 * bld/main.c
 *
 * History:
 *    2005/01/27 - [Charles Chiou] created file
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
#include <ambhw/uart.h>
#include <ambhw/nand.h>
#include <ambhw/spinor.h>
#include <ambhw/usbdc.h>
#include <ambhw/gpio.h>
#include <sdmmc.h>
#include <eth/network.h>
#include <dsp/dsp.h>

/* ==========================================================================*/
const char *AMBOOT_LOGO =						\
	"\r\n"								\
	"             ___  ___  _________                _   \r\n"	\
	"            / _ \\ |  \\/  || ___ \\              | |  \r\n"	\
	"           / /_\\ \\| .  . || |_/ /  ___    ___  | |_ \r\n"	\
	"           |  _  || |\\/| || ___ \\ / _ \\  / _ \\ | __|\r\n"	\
	"           | | | || |  | || |_/ /| (_) || (_) || |_ \r\n"	\
	"           \\_| |_/\\_|  |_/\\____/  \\___/  \\___/  \\__|\r\n" \
	"----------------------------------------------------------\r\n" \
	"Amboot(R) Ambarella(R) Copyright (C) 2004-2014\r\n";

/* ==========================================================================*/
int main(void)
{
	int ret_val = 0;
	u32 boot_from;
	u32 part_dev;
	int escape = 0;
#if defined(AMBOOT_DEV_NORMAL_MODE)
	flpart_table_t ptb;
#endif
	int h;
#if defined(CONFIG_AMBOOT_COMMAND_SUPPORT)
	int l;
	char cmd[MAX_CMDLINE_LEN];
#endif

	malloc_init();

	rct_pll_init();
	enable_fio_dma();
	rct_reset_fio();
	fio_exit_random_mode();
	dma_channel_select();

#if defined(CONFIG_AMBOOT_ENABLE_GPIO)
	gpio_init();
#else
	gpio_mini_init();
#endif
	/* Initialize various peripherals used in AMBoot */
	if (amboot_bsp_early_init != NULL) {
		amboot_bsp_early_init();
	}
	vic_init();
	uart_init();
	putstr("\x1b[4l");	/* Set terminal to replacement mode */
	putstr("\r\n");		/* First, output a blank line to UART */

	/* Initial boot device */
	boot_from = ambausb_boot_from[0] ? ambausb_boot_from[0] : rct_boot_from();
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
#if defined(AMBOOT_DEV_USBDL_MODE)
	if (usb_check_connected()) {
		usb_boot(USB_MODE_DEFAULT);
	} else {
		usb_disconnect();
	}
#elif defined(AMBOOT_DEV_NORMAL_MODE)
	flprog_get_part_table(&ptb);
	if (ptb.dev.magic != FLPART_MAGIC) {
		putstr("sanitized ptb.dev\r\n");
		flprog_set_part_table(&ptb);
	}

	if (amboot_bsp_hw_init != NULL) {
		amboot_bsp_hw_init();
	}

#if defined(AMBOOT_BOOT_DSP)
	dsp_init();
	audio_init();
#endif

	if ((ptb.dev.usbdl_mode == 0) && (amboot_bsp_check_usbmode != NULL)) {
		ptb.dev.usbdl_mode = amboot_bsp_check_usbmode();
	}

	/* Check for a 'enter' key on UART and halt auto_boot, usbdl_mode */
	if (ptb.dev.auto_boot || ptb.dev.usbdl_mode) {
#if defined(AMBOOT_DEV_FAST_BOOT)
		/* You may need to try more times to enter Amboot */
		escape = uart_wait_escape(1);
#else
		escape = uart_wait_escape(50);
#endif
	}

#if defined(CONFIG_AMBOOT_ENABLE_USB)
	/* If automatic USB download mode is enabled, */
	/* then enter special USB download mode */
	if (escape == 0 && ptb.dev.usbdl_mode) {
		usb_boot(ptb.dev.usbdl_mode);
	} else {
		usb_disconnect();
	}
#endif


	/* If automatic boot is enabled, attempt to boot from flash */
	if (escape == 0 && ptb.dev.auto_boot) {
		/* Call out to BSP supplied entry point (if exists) */
		if (amboot_bsp_entry != NULL) {
			ret_val = amboot_bsp_entry(&ptb);
		}

#if defined(AMBOOT_THAW_HIBERNATION)
		thaw_hibernation();
#endif

#if defined(SECURE_BOOT)
		secure_boot_main(ret_val, &ptb);
#endif

		if (ret_val == 1) {
			bios(NULL, 0);
		} else if (ret_val == 2) {
			bios(ptb.dev.cmdline, 0);  /* Auto BIOS */
		}  else {
			ret_val = boot(NULL, 0);  /* Auto boot */
		}


	}

#if defined(CONFIG_AMBOOT_ENABLE_ETH)
	/* Try booting from the network */
	ret_val = bld_net_init(0, &ptb);
	if ((ret_val == 0) && (escape == 0) && ptb.dev.auto_dl) {
		putstr("auto-boot from network\r\n");
		bld_netboot(NULL, 0);
	}
#endif

#endif


	putstr(AMBOOT_LOGO);
	rct_show_boot_from(rct_boot_from());
	rct_show_pll();

#if defined(CONFIG_AMBOOT_COMMAND_SUPPORT)
	commands_init();
#endif
	for (h = 0; ; h++) {
#if defined(CONFIG_AMBOOT_COMMAND_SUPPORT)
		putstr("amboot> ");
		l = uart_getcmd(cmd, sizeof(cmd), 0);
		if (l > 0) {
			parse_command(cmd);
		}
#endif
	}

	return 0;
}

