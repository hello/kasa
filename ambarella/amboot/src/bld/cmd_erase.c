/**
 * bld/cmd_erase.c
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
#include <ambhw/nand.h>

/*===========================================================================*/
static int cmd_erase(int argc, char *argv[])
{
	u32 i;

	if (argc != 2) {
		putstr("partition needs to be specified!\r\n");
		return -1;
	}

	if (strcmp(argv[1], "os") == 0) {
		for (i = PART_PBA; i < HAS_IMG_PARTS; i++) {
			flprog_erase_partition(i);
		}
		return 0;
	} else if (strcmp(argv[1], "all") == 0) {
		return flprog_erase_partition(HAS_IMG_PARTS);
	}

	for (i = 0; i < HAS_IMG_PARTS; i++) {
		if (strcmp(argv[1], get_part_str(i)) == 0) {
			return flprog_erase_partition(i);
		}
	}
	putstr("parition name not recognized!\r\n");

	return -2;
}

/*===========================================================================*/
static char help_erase[] =
	"erase [bst|ptb|bld|spl|pba|pri|sec|bak|rmd|rom|dsp|lnx|raw|all|os]\r\n"
	"Where [os] means pba,pri,sec,bak,rmd,rom,dsp,lnx,swp,add,adc\r\n"
	"      [all] means full chip.\r\n"
	"Erase a parition as specified\r\n";
__CMDLIST(cmd_erase, "erase", help_erase);

