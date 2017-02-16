/**
 * bld/cmd_ping.c
 *
 * History:
 *    2006/10/16 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2006, Ambarella, Inc.
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
static int cmd_ping(int argc, char *argv[])
{
	int rval = 0;
	bld_ip_t ip;

	if (argc != 2) {
		return -1;
	}

	if (str_to_ipaddr(argv[1], &ip) < 0) {
		putstr("IP addr error!\r\n");
		return 0;
	}

	rval = bld_net_wait_ready(DEFAULT_NET_WAIT_TMO);
	if (rval < 0) {
		putstr("link down ...\r\n");
		return 0;
	}

	rval = bld_ping(ip, DEFAULT_NET_WAIT_TMO);
	if (rval >= 0) {
		putstr("alive!\r\n");
	} else {
		putstr("no echo reply! ...\r\n");
	}

	return 0;
}

/*===========================================================================*/
static char help_ping[] =
	"ping [addr]\r\n"
	"PING a host\r\n";
__CMDLIST(cmd_ping, "ping", help_ping);

