/**
 * bld/cmd_boot.c
 *
 * History:
 *    2005/08/18 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>

/*===========================================================================*/
static int cmd_boot(int argc, char *argv[])
{
	char cmdline[MAX_CMDLINE_LEN];

	if (argc == 1) {
		boot(NULL, 1);
	} else {
		strfromargv(cmdline, sizeof(cmdline), argc - 1, &argv[1]);
		boot(cmdline, 1);
	}

	return 0;
}

/*===========================================================================*/
static char help_boot[] =
	"boot <param>\r\n"
	"Load images from flash to memory and boot\r\n";
__CMDLIST(cmd_boot, "boot", help_boot);

