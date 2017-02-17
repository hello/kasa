/**
 * bld/cmd_show.c
 *
 * History:
 *    2005/08/18 - [Charles Chiou] created file
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

/*===========================================================================*/
static char __to_4bit_char(u8 v)
{
	if (v <= 9)
		return '0' + v;
	else if (v < 15)
		return 'a' + v - 10;
	return 'f';
}

static void cmd_show_mac(const char *msg, u8 *mac)
{
	putstr(msg);
	putchar(__to_4bit_char(mac[0] >>  4));
	putchar(__to_4bit_char(mac[0] & 0xf));
	putchar(':');
	putchar(__to_4bit_char(mac[1] >>  4));
	putchar(__to_4bit_char(mac[1] & 0xf));
	putchar(':');
	putchar(__to_4bit_char(mac[2] >>  4));
	putchar(__to_4bit_char(mac[2] & 0xf));
	putchar(':');
	putchar(__to_4bit_char(mac[3] >>  4));
	putchar(__to_4bit_char(mac[3] & 0xf));
	putchar(':');
	putchar(__to_4bit_char(mac[4] >>  4));
	putchar(__to_4bit_char(mac[4] & 0xf));
	putchar(':');
	putchar(__to_4bit_char(mac[5] >>  4));
	putchar(__to_4bit_char(mac[5] & 0xf));
	putstr("\r\n");
}

static void cmd_show_ip(const char *msg, u32 ip)
{
	putstr(msg);
	putdec(ip & 0xff);
	putchar('.');
	putdec((ip >> 8) & 0xff);
	putchar('.');
	putdec((ip >> 16) & 0xff);
	putchar('.');
	putdec((ip >> 24) & 0xff);
	putstr("\r\n");
}

static int cmd_show(int argc, char *argv[])
{
	int ret_val = 0;

	if (argc < 2) {
		putstr("requires an argument!\r\n");
		return -1;
	}

	if (strcmp(argv[1], "ptb") == 0) {
		flpart_table_t ptb;
		int i;

		ret_val = flprog_get_part_table(&ptb);
		if (ret_val < 0)
			return ret_val;

		bld_loader_display_ptb_content(&ptb);
		putstr("S/N: ");
		for (i = 0; i < sizeof(ptb.dev.sn); i++) {
			if (ptb.dev.sn[i] == '\0')
				break;
			putchar(ptb.dev.sn[i]);
		}
		putstr("\r\nusbdl_mode: ");
		putdec(ptb.dev.usbdl_mode);
		putstr("\r\nauto_boot: ");
		putdec(ptb.dev.auto_boot);
		putstr("\r\n");
	} else if (strcmp(argv[1], "meta") == 0) {
		flprog_show_meta();
	} else if (strcmp(argv[1], "poc") == 0) {
		u32 boot_from;

		boot_from = rct_boot_from();
		if (argc >= 3) {
			strtou32(argv[2], &boot_from);
		}
		rct_show_boot_from(boot_from);
	} else if (strcmp(argv[1], "netboot") == 0) {
		flpart_table_t ptb;
		int i;

		/* Read the partition table */
		ret_val = flprog_get_part_table(&ptb);
		if (ret_val < 0)
			return ret_val;

		cmd_show_mac("eth0_mac: ", ptb.dev.eth[0].mac);
		cmd_show_ip("eth0_ip: ", ptb.dev.eth[0].ip);
		cmd_show_ip("eth0_mask: ", ptb.dev.eth[0].mask);
		cmd_show_ip("eth0_gw: ", ptb.dev.eth[0].gw);
		cmd_show_mac("eth1_mac: ", ptb.dev.eth[1].mac);
		cmd_show_ip("eth1_ip: ", ptb.dev.eth[1].ip);
		cmd_show_ip("eth1_mask: ", ptb.dev.eth[1].mask);
		cmd_show_ip("eth1_gw: ", ptb.dev.eth[1].gw);
		putstr("auto_dl: ");
		putdec(ptb.dev.auto_dl);
		putstr("\r\n");

		/* tftpd */
		cmd_show_ip("tftpd: ", ptb.dev.tftpd);

		/* pri_addr */
		putstr("pri_addr: 0x");
		puthex(ptb.dev.pri_addr);
		putstr("\r\n");

		/* pri_file */
		putstr("pri_file: ");
		for (i = 0; i < sizeof(ptb.dev.pri_file); i++) {
			if (ptb.dev.pri_file[i] == '\0')
				break;
			putchar(ptb.dev.pri_file[i]);
		}
		putstr("\r\n");

		/* pri_comp */
		putstr("pri_comp: ");
		putdec(ptb.dev.pri_comp);
		putstr("\r\n");

		/* rmd_addr */
		putstr("rmd_addr: 0x");
		puthex(ptb.dev.rmd_addr);
		putstr("\r\n");

		/* rmd_file */
		putstr("rmd_file: ");
		for (i = 0; i < sizeof(ptb.dev.rmd_file); i++) {
			if (ptb.dev.rmd_file[i] == '\0')
				break;
			putchar(ptb.dev.rmd_file[i]);
		}
		putstr("\r\n");

		/* rmd_comp */
		putstr("rmd_comp: ");
		putdec(ptb.dev.rmd_comp);
		putstr("\r\n");

		/* dsp_addr */
		putstr("dsp_addr: 0x");
		puthex(ptb.dev.dsp_addr);
		putstr("\r\n");

		/* dsp_file */
		putstr("dsp_file: ");
		for (i = 0; i < sizeof(ptb.dev.dsp_file); i++) {
			if (ptb.dev.dsp_file[i] == '\0')
				break;
			putchar(ptb.dev.dsp_file[i]);
		}
		putstr("\r\n");

		/* dsp_comp */
		putstr("dsp_comp: ");
		putdec(ptb.dev.dsp_comp);
		putstr("\r\n");

	} else if (strcmp(argv[1], "wifi") == 0) {
		flpart_table_t ptb;

		/* Read the partition table */
		ret_val = flprog_get_part_table(&ptb);
		if (ret_val < 0)
			return ret_val;

		cmd_show_mac("wifi0_mac: ", ptb.dev.wifi[0].mac);
		cmd_show_ip("wifi0_ip: ", ptb.dev.wifi[0].ip);
		cmd_show_ip("wifi0_mask: ", ptb.dev.wifi[0].mask);
		cmd_show_ip("wifi0_gw: ", ptb.dev.wifi[0].gw);
		cmd_show_mac("wifi1_mac: ", ptb.dev.wifi[1].mac);
		cmd_show_ip("wifi1_ip: ", ptb.dev.wifi[1].ip);
		cmd_show_ip("wifi1_mask: ", ptb.dev.wifi[1].mask);
		cmd_show_ip("wifi1_gw: ", ptb.dev.wifi[1].gw);

	} else if (strcmp(argv[1], "usb_eth") == 0) {
		flpart_table_t ptb;

		ret_val = flprog_get_part_table(&ptb);
		if (ret_val < 0)
			return ret_val;

		cmd_show_mac("usb_eth0_mac: ", ptb.dev.usb_eth[0].mac);
		cmd_show_ip("usb_eth0_ip: ", ptb.dev.usb_eth[0].ip);
		cmd_show_ip("usb_eth0_mask: ", ptb.dev.usb_eth[0].mask);
		cmd_show_ip("usb_eth0_gw: ", ptb.dev.usb_eth[0].gw);
		cmd_show_mac("usb_eth1_mac: ", ptb.dev.usb_eth[1].mac);
		cmd_show_ip("usb_eth1_ip: ", ptb.dev.usb_eth[1].ip);
		cmd_show_ip("usb_eth1_mask: ", ptb.dev.usb_eth[1].mask);
		cmd_show_ip("usb_eth1_gw: ", ptb.dev.usb_eth[1].gw);

	} else {
		ret_val = -2;
	}

	return ret_val;
}

/*===========================================================================*/
static char help_show[] =
	"\r\n"
	"show ptb       - flash partition table\r\n"
	"show poc       - power on config\r\n"
	"show netboot   - netboot parameters\r\n"
	"show wifi      - show wifi infomations\r\n"
	"show usb_eth   - show usb ethernet infomations\r\n"
	"Display various system properties\r\n";
__CMDLIST(cmd_show, "show", help_show);

