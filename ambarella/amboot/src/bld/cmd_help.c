/**
 * bld/cmd_help.c
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

extern cmdlist_t *commands;

/*===========================================================================*/
static int cmd_help(int argc, char *argv[])
{
	int i;
	cmdlist_t *cmd;

	/* Help on a command? */
	if (argc >= 2) {
		for (cmd = commands; cmd != NULL; cmd = cmd->next) {
			if (strcmp(cmd->name, argv[1]) == 0) {
				putstr("Help for '");
				putstr(argv[1]);
				putstr("':\r\n");
				putstr(cmd->help);
				return 0;
			}
		}

		return -1;
	}

	putstr("The following commands are supported:\r\n");

	for (cmd = commands, i = 0; cmd != NULL; cmd = cmd->next, i++) {
		putstr(cmd->name);
		putchar('\t');
		if ((i % 4) == 3)
			putstr("\r\n");
	}
	putstr("\r\n");

	putstr("Use 'help' to get help on a specific command\r\n");

	return 0;
}

/*===========================================================================*/
static char help_help[] =
	"help [command]\r\n"
	"Get help on [command], "
	"or a list of supported commands if a command is omitted.\r\n";
__CMDLIST(cmd_help, "help", help_help);

