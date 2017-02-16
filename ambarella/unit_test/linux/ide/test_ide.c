/*
 * test_ide.c
 *
 * History:
 *	2008/11/05 - [Cao Rongrong] created file
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#define LOOP_EXT	1000
#define LOOP_INN	25
#define SLEEP_TIME	10000

void usage(void)
{
	printf("\nThis program is used to test IDE disk.\n"
		"You should mount IDE disk to /tmp/ide first.\n"
		"mkdir /tmp/ide; mount -t vfat /dev/hda1 /tmp/ide\n");
}

int main(int argc,char *argv[])
{
	int i, j;
	FILE *fd;
	char *buf = malloc(4096);
	time_t tm_begin, tm_end;
	float spd;

	if(argc > 1){
		usage();
		return 1;
	}

	for(i = 0; i < 4096; i++){
		*(buf+i) = i;
	}

	fd = fopen("/tmp/ide/tmp_file", "w+");
	if(fd == NULL) {
		fprintf(stderr, "Can't create or write file!\n"
			"You may mount IDE disk to /tmp/ide first!\n"
			"mkdir /tmp/ide; mount -t vfat /dev/hda1 /tmp/ide\n");
                exit(1);
	}

	time(&tm_begin);
	fprintf(stderr, "%s", ctime(&tm_begin));

	for(j = 0; j < LOOP_EXT; j++){
		for(i = 0; i < LOOP_INN; i++){
			fwrite(buf, 4096, 1, fd);
		}
		usleep(SLEEP_TIME);
	}

	fsync(fileno(fd));

	time(&tm_end);
	fprintf(stderr, "%s", ctime(&tm_end));

	spd = (4096 * LOOP_EXT * LOOP_INN) /
		(tm_end - tm_begin - LOOP_EXT * SLEEP_TIME / 1000000);
	fprintf(stderr, "IDE transfer speed is %.2f MB/s\n", spd / 1000000);

	if (fd > 0)
		fclose(fd);
	if (buf)
		free(buf);
	return 0;
}


