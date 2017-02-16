/*
 * test_ide.c
 *
 * History:
 *	2011/06/21 - [Cao Rongrong] created file
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* test_uhc_stg is used to test USB host controller which is
 * interfacing with a USB drive.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#define BUFFER_SIZE	(5 * 1024 * 1024)
#define	TEST_SIZE	(20 * BUFFER_SIZE)

int main(int argc,char *argv[])
{
	char filename[128], buf[BUFFER_SIZE];
	int fd, n, cnt = 1, size;
	float spd;
	time_t tm_begin, tm_end, tm_diff;

	if (argc > 1)
		strcpy(filename, argv[1]);
	else
		strcpy(filename, "/tmp/sda1/Ducks.Take.Off.1080p.QHD.CRF25.x264-CtrlHD.mkv");

	fd = open(filename, O_RDONLY);
	if(fd < 0) {
		perror("open failed!\n");
                exit(-1);
	}

	printf("start to read ...\n");

	do {
		size = 0;
		time(&tm_begin);
		do {
			n = read(fd, buf, BUFFER_SIZE);
			if (n < 0) {
				perror("read failed");
				exit(-1);
			}
			if (n == 0) {
				printf("read again (%d) ...\n", cnt++);
				lseek(fd, 0, SEEK_SET);
				break;
			}
			size += n;
		} while (size < TEST_SIZE);
		time(&tm_end);
		tm_diff = tm_end - tm_begin;
		if (tm_diff <= 0)
			printf("time elapsed: %ds, speed is NULL\n", (int)tm_diff);
		else {
			spd = TEST_SIZE / tm_diff;
			printf("time elapsed: %ds, speed is %.2f MB/s 11\n", (int)tm_diff, spd / 1000000);
		}

		if(system ("echo 3 > /proc/sys/vm/drop_caches") < 0 )
			perror("drop_caches");
	} while(1);

	close(fd);

	return 0;
}


