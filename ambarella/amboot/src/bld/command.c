/**
 * bld/command.c
 *
 * History:
 *    2005/07/03 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
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

