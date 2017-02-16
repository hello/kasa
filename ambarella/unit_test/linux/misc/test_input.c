/*
 * test_input.c
 *
 * History:
 *	2009/10/19 - [Jian Tang] created file
 *	2014/09/25 - [Wu Dongge] modify for multiple event
 *
 * Copyright (C) 2007-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#ifdef __UCLIBC__
#include <bits/getopt.h>
#else
#include <getopt.h>
#endif
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <linux/input.h>
#include <pthread.h>
#include <assert.h>

#include "basetypes.h"
#include <signal.h>


struct keypadctrl {
	pthread_t thread;
	int running;
	int *pfd;
	unsigned int count;
};

struct keypadctrl G_keypadctrl;

 static int key_cmd_process(struct input_event *cmd)
{

	if (cmd->type != EV_KEY)
		return 0;

	if (cmd->value)
		printf(" == Key [%d] is pressed ==\n", cmd->code);
	else
		printf(" == Key [%d] is release ==\n", cmd->code);

	return 0;
}

static void *keypadctrl_thread(void *args)
{
	struct keypadctrl *keypad = (struct keypadctrl *) args;
	struct input_event event;
	fd_set rfds;
	int rval = -1;
	int i = 0;
	char str[64] = {0};

	keypad->running = 1;
	keypad->count = 0;
	keypad->pfd = NULL;
	printf("keypadctrl_thread started!\n");

	do {
		sprintf(str, "/dev/input/event%d", keypad->count);
		keypad->count++;
	} while(access(str, F_OK) == 0);

	keypad->count--;
	if (keypad->count == 0) {
		printf("No any event device!\n");
		goto done;
	}

	printf("Totally %d event\n", keypad->count);
	keypad->pfd = (int *)malloc(keypad->count * sizeof(int));
	if (!keypad->pfd) {
		printf("Malloc failed!\n");
		goto done;
	}

	for (i = 0; i < keypad->count; i++) {
		sprintf(str, "/dev/input/event%d", i);
		keypad->pfd[i] = open(str, O_NONBLOCK);
		if (keypad->pfd[i] < 0) {
			printf("open %s error", str);
			goto done;
		}
	}

	while (keypad->running) {
		struct timeval timeout = {0, 1};
		FD_ZERO(&rfds);
		for (i = 0; i < keypad->count; i++) {
			FD_SET(keypad->pfd[i], &rfds);
		}

		rval = select(FD_SETSIZE, &rfds, NULL, NULL, &timeout);
		if (rval < 0) {
			perror("select error!");
			if (errno == EINTR) {
			continue;
		}
		  goto done;
		} else if (rval == 0) {
		  continue;
		}

		for (i = 0; i < keypad->count; i++) {
			if (FD_ISSET(keypad->pfd[i], &rfds)) {
				rval = read(keypad->pfd[i], &event, sizeof(event));
				if (rval < 0) {
					printf("read keypad->fd[%d] error",i);
					continue;
				}

				key_cmd_process(&event);
				}
			}
		}
done:

	keypad->running = 0;
	for (i = 0; i < keypad->count; i++) {
		close(keypad->pfd[i]);
	}

	keypad->count = 0;
	if (keypad->pfd){
		free(keypad->pfd);
		keypad->pfd = NULL;
	}
	printf("keypadctrl_thread stopped!\n");

	return NULL;
}

int keypadctrl_init(void)
{
	struct keypadctrl *keypad = (struct keypadctrl *) &G_keypadctrl;

	memset(keypad, 0x0, sizeof(*keypad));

	return 0;
}

void keypadctrl_cleanup(void)
{
	struct keypadctrl *keypad = (struct keypadctrl *) &G_keypadctrl;

	if (keypad->running == 0) {
		return;
	}

	keypad->running = 0;
	pthread_join(keypad->thread, NULL);

	memset(keypad, 0x0, sizeof(*keypad));
}

int keypadctrl_start(void)
{
	struct keypadctrl *keypad = (struct keypadctrl *) &G_keypadctrl;
	int rval = 0;

	if (keypad->running)
		return 1;

	rval = pthread_create(&keypad->thread, NULL,
			      keypadctrl_thread, keypad);
	if (rval < 0)
		perror("pthread_create keypadctrl_thread failed\n");

	return 0;
}

static void sigstop(int signum)
{
	keypadctrl_cleanup();
	exit(1);
}

int main(int argc, char ** argv)
{
	char c;

	signal(SIGINT, sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);

	keypadctrl_init();
	keypadctrl_start();

	while (1) {
		scanf("%c", &c);
		if (c == 'q') {
			break;
		}
	}

	keypadctrl_cleanup();

	return 0;
}

