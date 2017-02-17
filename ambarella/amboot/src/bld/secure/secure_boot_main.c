/**
 * bld/secure/secure_boot_main.c
 *
 * History:
 *    2015/11/24 - [Zhi He] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
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
#include <ambhw/uart.h>
#include <ambhw/nand.h>
#include <ambhw/spinor.h>
#include <ambhw/usbdc.h>
#include <ambhw/gpio.h>

#include "cryptography_if.h"
#include "platform_misc.h"
#include "secure_boot.h"

/* ==========================================================================*/
static const char *AM_SECURE_BOOT_LOGO =	\
	"\r\n"
	"    _    __  __   ____                             ____              _\r\n"	\
	"   / \\  |  \\/  | / ___|  ___  ___ _   _ _ __ ___  | __ )  ___   ___ | |_\r\n"	\
	"  / _ \\ | |\\/| | \\___ \\ / _ \\/ __| | | | '__/ _ \\ |  _ \\ / _ \\ / _ \\| __|\r\n"	\
	" / ___ \\| |  | |  ___) |  __/ (__| |_| | | |  __/ | |_) | (_) | (_) | |_\r\n"	\
	"/_/   \\_\\_|  |_| |____/ \\___|\\___|\\__,_|_|  \\___| |____/ \\___/ \\___/ \\__|\r\n"	\
	"--------------------------------------------------------------------------\r\n" \
	"Amboot(R) (SecureBoot) Ambarella(R) Copyright (C) 2015\r\n";

/* ==========================================================================*/

int secure_boot_main(int ret_val, flpart_table_t *pptb)
{
	int h;
#if defined(CONFIG_AMBOOT_COMMAND_SUPPORT)
	int l;
	char cmd[MAX_CMDLINE_LEN];
#endif
	int secure_boot_ret = 0;
	int ask_for_debug = 1;
	int force_reinitialize_for_debug = 0;

	unsigned char i2c_index = 0;
	unsigned int i2c_freq = 100 * 1000;

#if (CHIP_REV == S3L)
	i2c_index = 2;
#else
	i2c_index = 0;
#endif

	putstr("i2c index:");
	putbyte(i2c_index);
	putstr("\r\n");
	secure_boot_ret = secure_boot_init(i2c_index, i2c_freq);
	if (0 > secure_boot_ret) {
		putstr("[secure boot check fail]: no cryptochip found, exit..\r\n");
		goto __am_secure_boot_console;
	} else if (1 == secure_boot_ret) {
		goto __force_reinitialize_secure_boot;
	} else {
		if (!pptb->dev.secure_boot_init) {
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
			goto __am_secure_boot_console;
		} else {
			rsa_context_t rsa_content;

			//putstr("rsakey-n:\r\n");
			//putstr((const char*) pptb->dev.rsa_key_n);
			//putstr("\r\nrsakey-e:\r\n");
			//putstr((const char*) pptb->dev.rsa_key_e);
			//putstr("\r\n");

			secure_boot_ret = verify_and_fill_pubkey(&pptb->dev, &rsa_content);
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
				goto __am_secure_boot_console;
			} else {
				putstr("[secure boot check]: key is a valid rsa pubkey\r\n");
			}

			secure_boot_ret = verify_rsapubkey_hw_signature((unsigned char *) pptb->dev.rsa_key_n, 256 + 4 + 16);
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
				goto __am_secure_boot_console;
			} else {
				putstr("[secure boot check]: verify rsa pubkey OK\r\n");
			}

			secure_boot_ret = verify_sn_signature(pptb->dev.sn_signature, &rsa_content);
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
				goto __am_secure_boot_console;
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
			goto __am_secure_boot_console;
		} else {
			rsa_context_t rsa_content;

			uart_print_rsakey_1024(&rsa_key);

			secure_boot_ret = verify_and_fill_key(&rsa_key, &rsa_content);
			if (secure_boot_ret) {
				putstr("[secure boot for initialization error]: invalid rsa key\r\n");
				goto __am_secure_boot_console;
			}

			memcpy(pptb->dev.rsa_key_n, rsa_key.n, 256 + 4);
			memcpy(pptb->dev.rsa_key_e, rsa_key.e, 16);

			secure_boot_ret = generate_rsapubkey_hw_signature((unsigned char *) pptb->dev.rsa_key_n, 256 + 4 + 16);
			if (0 == secure_boot_ret) {
				putstr("[secure boot for initialization]: generate rsa public key hw signature done\r\n");
			} else {
				putstr("[secure boot for initialization error]: generate rsa public key hw signature fail\r\n");
				goto __am_secure_boot_console;
			}

			secure_boot_ret = generate_sn_signature(pptb->dev.sn_signature, &rsa_content);
			if (secure_boot_ret) {
				putstr("[secure boot for initialization error]: sign serial number fail?\r\n");
				goto __am_secure_boot_console;
			} else {
				putstr("[secure boot for initialization]: sign serial number done\r\n");
			}

			pptb->dev.secure_boot_init = 1;
			pptb->dev.need_generate_firmware_hw_signature = 1;
			flprog_set_part_table(pptb);
		}
	}

	if (ret_val == 1) {
		bios(NULL, 0);
	} else if (ret_val == 2) {
		bios(pptb->dev.cmdline, 0);  /* Auto BIOS */
	}  else {
		ret_val = boot(NULL, 0);  /* Auto boot */
	}

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
			goto __am_secure_boot_console;
			break;

		default:
			break;
	}

__am_secure_boot_console:

	putstr(AM_SECURE_BOOT_LOGO);
	amboot_show_version();
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

