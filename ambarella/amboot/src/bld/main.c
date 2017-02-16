/**
 * bld/main.c
 *
 * History:
 *    2005/01/27 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2007, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>
#include <ambhw/vic.h>
#include <ambhw/uart.h>
#include <ambhw/nand.h>
#include <ambhw/spinor.h>
#include <ambhw/usbdc.h>
#include <ambhw/gpio.h>
#include <sdmmc.h>
#include <bapi.h>
#include <eth/network.h>
#include <dsp/dsp.h>

#if defined(SECURE_BOOT)
#include "secure/cryptography_if.h"
#include "secure/secure_boot.h"
#endif

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
#if defined(SECURE_BOOT)
	int secure_boot_ret = 0;
	int ask_for_debug = 1;
	int force_reinitialize_for_debug = 0;
#endif

	rct_pll_init();
	enable_fio_dma();
	rct_reset_fio();
	fio_exit_random_mode();
	dma_channel_select();

	/* Initialize various peripherals used in AMBoot */
	if (amboot_bsp_early_init != NULL) {
		amboot_bsp_early_init();
	}
	vic_init();
	uart_init();
	putstr("\x1b[4l");	/* Set terminal to replacement mode */
	putstr("\r\n");		/* First, output a blank line to UART */

	mem_malloc_init();

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

#if defined(CONFIG_AMBOOT_BAPI_SUPPORT)
	bld_bapi_init(0);
#endif

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
		secure_boot_ret = secure_boot_init();
		if (0 > secure_boot_ret) {
			putstr("[secure boot check fail]: no cryptochip found, exit..\r\n");
			goto __amboot_console;
		} else if (1 == secure_boot_ret) {
			goto __force_reinitialize_secure_boot;
		} else {
			if (!ptb.dev.secure_boot_init) {
				putstr("[secure boot check fail]: cryptochip is initialized, but firmware is not initialized\r\n");
				if (ask_for_debug) {
					int c = 0x0;
					putstr("[debug]: do you want force re-initialize?\r\n");
					c = uart_get_onechar_blocked();
					if (('y' == c) || ('Y' == c)) {
						goto __force_reinitialize_secure_boot;
					}
				}
				putstr("fail 1, exit...\r\n");
				goto __amboot_console;
			} else {
				rsa_context_t rsa_content;

				//putstr("rsakey-n:\r\n");
				//putstr((const char*) ptb.dev.rsa_key_n);
				//putstr("\r\nrsakey-e:\r\n");
				//putstr((const char*) ptb.dev.rsa_key_e);
				//putstr("\r\n");

				secure_boot_ret = verify_and_fill_pubkey(&ptb.dev, &rsa_content);
				if (secure_boot_ret) {
					putstr("[secure boot check fail]: invalid rsa pubkey\r\n");
					if (ask_for_debug) {
						int c = 0x0;
						putstr("[debug]: do you want force re-initialize?\r\n");
						c = uart_get_onechar_blocked();
						if (('y' == c) || ('Y' == c)) {
							goto __force_reinitialize_secure_boot;
						}
					}
					putstr("fail 2, exit...\r\n");
					goto __amboot_console;
				} else {
					putstr("[secure boot check]: key is a valid rsa pubkey\r\n");
				}

				secure_boot_ret = verify_rsapubkey_hw_signature((unsigned char *) ptb.dev.rsa_key_n, 256 + 4 + 16);
				if (secure_boot_ret) {
					putstr("[secure boot check fail]: rsa pubkey is modified?\r\n");
					if (ask_for_debug) {
						int c = 0x0;
						putstr("[debug]: do you want force re-initialize?\r\n");
						c = uart_get_onechar_blocked();
						if (('y' == c) || ('Y' == c)) {
							goto __force_reinitialize_secure_boot;
						}
					}
					putstr("fail 3, exit...\r\n");
					goto __amboot_console;
				} else {
					putstr("[secure boot check]: verify rsa pubkey OK\r\n");
				}

				secure_boot_ret = verify_sn_signature(ptb.dev.sn_signature, &rsa_content);
				if (secure_boot_ret) {
					putstr("[secure boot check fail]: serial number signature check fail, hardware clone?\r\n");
					if (ask_for_debug) {
						int c = 0x0;
						putstr("[debug]: do you want force re-initialize?\r\n");
						c = uart_get_onechar_blocked();
						if (('y' == c) || ('Y' == c)) {
							goto __force_reinitialize_secure_boot;
						}
					}
					putstr("fail 4, exit...\r\n");
					goto __amboot_console;
				} else {
					putstr("[secure boot check]: verify serial number signature OK\r\n");
				}

			}
		}

		if (force_reinitialize_for_debug) {
			rsa_key_t rsa_key;

__force_reinitialize_secure_boot:

			putstr("[secure boot for initialization]: please enter rsa key\r\n");
			memset(&rsa_key, 0x0, sizeof(rsa_key));

			secure_boot_ret = uart_get_rsakey_1024(&rsa_key);
			if (secure_boot_ret) {
				putstr("[secure boot for initialization error]: read rsa key fail...\r\n");
				uart_print_rsakey_1024(&rsa_key);
				goto __amboot_console;
			} else {
				rsa_context_t rsa_content;

				uart_print_rsakey_1024(&rsa_key);

				secure_boot_ret = verify_and_fill_key(&rsa_key, &rsa_content);
				if (secure_boot_ret) {
					putstr("[secure boot for initialization error]: invalid rsa key\r\n");
					goto __amboot_console;
				}

				memcpy(ptb.dev.rsa_key_n, rsa_key.n, 256 + 4);
				memcpy(ptb.dev.rsa_key_e, rsa_key.e, 16);

				secure_boot_ret = generate_rsapubkey_hw_signature((unsigned char *) ptb.dev.rsa_key_n, 256 + 4 + 16);
				if (0 == secure_boot_ret) {
					putstr("[secure boot for initialization]: generate rsa public key hw signature done\r\n");
				} else {
					putstr("[secure boot for initialization error]: generate rsa public key hw signature fail\r\n");
					goto __amboot_console;
				}

				secure_boot_ret = generate_sn_signature(ptb.dev.sn_signature, &rsa_content);
				if (secure_boot_ret) {
					putstr("[secure boot for initialization error]: sign serial number fail?\r\n");
					goto __amboot_console;
				} else {
					putstr("[secure boot for initialization]: sign serial number done\r\n");
				}

				ptb.dev.secure_boot_init = 1;
				ptb.dev.need_generate_firmware_hw_signature = 1;
				flprog_set_part_table(&ptb);
			}
		}

#endif

		if (ret_val == 1) {
			bios(NULL, 0);
		} else if (ret_val == 2) {
			bios(ptb.dev.cmdline, 0);  /* Auto BIOS */
		}  else {
			ret_val = boot(NULL, 0);  /* Auto boot */
		}

#if defined(SECURE_BOOT)
		switch (ret_val) {

			case FLPROG_ERR_FIRM_HW_SIGN_FAIL:
			case FLPROG_ERR_FIRM_HW_SIGN_VERIFY_FAIL:
				putstr("[secure boot check fail]: firmware changed?\r\n");
				if (ask_for_debug) {
					int c = 0x0;
					putstr("[debug]: do you want force re-initialize?\r\n");
					c = uart_get_onechar_blocked();
					if (('y' == c) || ('Y' == c)) {
						goto __force_reinitialize_secure_boot;
					}
				}
				putstr("fail 6, exit...\r\n");
				goto __amboot_console;
				break;

			default:
				break;
		}
#endif

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

#if defined(SECURE_BOOT)
__amboot_console:
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

