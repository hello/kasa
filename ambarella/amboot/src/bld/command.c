/**
 * bld/command.c
 *
 * History:
 *    2005/07/03 - [Charles Chiou] created file
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

extern u32 __cmdlist_start;
extern u32 __cmdlist_end;

cmdlist_t *commands;

void commands_init(void)
{
	cmdlist_t *last_cmd;
	cmdlist_t *cmd, *next_cmd;

	commands = (cmdlist_t *) &__cmdlist_start;
	last_cmd = (cmdlist_t *) &__cmdlist_end;

	/* Make a list */
	cmd = next_cmd = commands;
	next_cmd++;

	while (next_cmd < last_cmd) {
		cmd->next = next_cmd;
		cmd++;
		next_cmd++;
	}
}

#define STATE_WHITESPACE (0)
#define STATE_WORD (1)

static void parse_args(char *cmdline, int *argc, char **argv)
{
	char *c;
	int state = STATE_WHITESPACE;
	int i;

	*argc = 0;

	if (strlen(cmdline) == 0)
		return;

	c = cmdline;
	while (*c != '\0') {
		if (*c == '\t')
			*c = ' ';
		c++;
	}
	
	c = cmdline;
	i = 0;

	while (*c != '\0') {
		if (state == STATE_WHITESPACE) {
			if (*c != ' ') {
				argv[i] = c;
				i++;
				state = STATE_WORD;
			}
		} else {
			if (*c == ' ') {
				*c = '\0';
				state = STATE_WHITESPACE;
			}
		}

		c++;
	}
	
	*argc = i;
}

static int get_num_command_matches(char *cmdline)
{
	cmdlist_t *cmd;
	int len;
	int num_matches = 0;

	len = strlen(cmdline);

	for (cmd = commands; cmd != NULL; cmd = cmd->next) {
		if (cmd->magic != COMMAND_MAGIC) {
			return -1;
		}

		if (strncmp(cmd->name, cmdline, len) == 0) 
			num_matches++;
	}

	return num_matches;
}

int parse_command(char *cmdline)
{
	int ret_val;
	cmdlist_t *cmd;
	int argc, num_commands, len;
	char buf[MAX_CMD_ARGS][MAX_ARG_LEN];
	char *argv[MAX_CMD_ARGS];

	for (argc = 0; argc < MAX_CMD_ARGS; argc++)
		argv[argc] = &buf[argc][0];

	parse_args(cmdline, &argc, argv);

	if (argc == 0) 
		return 0;

	num_commands = get_num_command_matches(argv[0]);

	if (num_commands < 0)
		return num_commands;

	if (num_commands != 1) {
		putstr("'");
		putstr(argv[0]);
		putstr("' is not a recognized command! ");
		putstr("Type 'help' for help...\r\n");
		return -1;
	}

	len = strlen(argv[0]);

	for (cmd = commands; cmd != NULL; cmd = cmd->next) {
		if (cmd->magic != COMMAND_MAGIC) {
			return -2;
		}

		if (strncmp(cmd->name, argv[0], len) == 0) {
			ret_val = cmd->fn(argc, argv);
			if (ret_val) {
				putstr("Help for '");
				putstr(argv[0]);
				putstr("':\r\n");
				putstr(cmd->help);
			}
		}
	}

	return -3;
}

