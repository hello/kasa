/**
 * bld/cmd_tftp.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
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

