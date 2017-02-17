/**
 * bld/cmd_tftp.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
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
#include <bldfunc.h>
#include <eth/network.h>

/*===========================================================================*/
static int cmd_tftp(int argc, char *argv[])
{
	int ret_val = 0;
	char cmdline[MAX_CMDLINE_LEN];

	if (argc < 2) {
		return -1;
	}

	if (strcmp(argv[1], "boot") == 0) {
		if (argc > 2) {
			strfromargv(cmdline, sizeof(cmdline),
				(argc - 2), &argv[2]);
			bld_netboot(cmdline, 1);
		} else {
			bld_netboot(NULL, 1);
		}
	} else if (strcmp(argv[1], "program") == 0) {
		const char *pfile_name = "bld_release.bin";
		u32 addr = MEMFWPROG_RAM_START;
		u32 exec = 1;

		if (argc >= 5) {
			pfile_name = (const char *)argv[2];
			strtou32(argv[3], &addr);
			strtou32(argv[4], &exec);
		} else if (argc >= 4) {
			pfile_name = (const char *)argv[2];
			strtou32(argv[3], &addr);
		} else if (argc >= 3) {
			pfile_name = (const char *)argv[2];
		}
		bld_netprogram(pfile_name, addr, exec, 1);
	} else {
		ret_val = -2;
	}

	return ret_val;
}

static char help_tftp[] =
	"tftp boot [cmdline]\r\n"
	"tftp program [file_name] [addr] [exec]\r\n"
	"Load images from TFTP server\r\n";
__CMDLIST(cmd_tftp, "tftp", help_tftp);

