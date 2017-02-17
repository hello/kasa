/**
 * bld/cmd_usbdl.c
 *
 * History:
 *    2005/09/27 - [Arthur Yang] created file
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
#include <ambhw/usbdc.h>

/*===========================================================================*/
static int cmd_usbdl(int argc, char *argv[])
{
	u32 addr = MEMFWPROG_RAM_START;
	int len;
	int flag = 0;
	int exec = 0;

	if ((argc == 2) || (argc == 3)) {
		if (strcmp(argv[1], "fwprog") == 0) {
			exec = 1;
			flag |= USB_FLAG_FW_PROG;
		} else if (strcmp(argv[1], "ext") == 0) {
			putstr("Using external osc for USB\r\n");
			return 0;
		} else if (strcmp(argv[1], "kernel") == 0) {
			exec = 1;
			flag |= USB_FLAG_KERNEL;
		} else if (strcmp(argv[1], "upload") == 0) {
			flag |= USB_FLAG_UPLOAD;
		} else if (strcmp(argv[1], "test") == 0) {
			/* Check argv[2] */
			if (argc == 2)
				flag |= USB_FLAG_TEST_DOWNLOAD;
			else {
				if (strcmp(argv[2], "download") == 0)
					flag |= USB_FLAG_TEST_DOWNLOAD;
				else if (strcmp(argv[2], "pll") == 0)
					flag |= USB_FLAG_TEST_PLL;
				else {
					putstr ("Invalid test\r\n");
					return -1;
				}
			}
		} else {
			if ((strtou32(argv[1], &addr) < 0) ||
			    ((addr < DRAM_START_ADDR) ||
			    (addr > DRAM_START_ADDR + DRAM_SIZE - 1))) {
				putstr ("Invalid hex address\r\n");
				return -2;
			}
			if ((argc == 3) && ((strcmp(argv[2], "exec") == 0) ||
					     (strcmp(argv[2], "1") == 0)))
				exec = 1;
			flag |= USB_FLAG_MEMORY;
		}
	} else if (argc != 1) {
		putstr("Invalid parameter\r\n");
		return -3;
	}

	putstr("Start transferring using USB...\r\n");

	init_usb_pll();
	len = usb_download((void *)addr, exec, flag);
	if (len < 0) {
		putstr("\r\ntransfer failed!\r\n");
		return len;
	}

	putstr("\r\nSuccessfully transferred ");
	putdec(len);
	putstr(" bytes.\r\n");

	return 0;
}

/*===========================================================================*/
static char help_usbdl[] = "The command execute actions over USB\r\n"
	"usbdl "
		"(Perform actions controlled by Host)\r\n"
	"usbdl ADDRESS exec "
		"(Download image to target address and execute it)\r\n"
	"usbdl fwprog "
		"(Download firware and program it into flash)\r\n"
	"usbdl ext "
		"(Turn on USB external clock)\r\n"
	"usbdl kernel "
		"(Download kernel files and execute it)\r\n"
	"usbdl upload "
		"(Upload data to Host by host's request)\r\n"
	"usbdl test [download | pll] "
		"(Test download or dll power-on/off)\r\n";
__CMDLIST(cmd_usbdl, "usbdl", help_usbdl);

