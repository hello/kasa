/*
 * test_flash_fs.c
 *
 * History:
 *	2009/12/17 - [Qiao Wang] created file
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>

#define LOOP_EXT	1024
#define LOOP_INN	4
#define BUFFER_SIZE	4096
typedef unsigned int	u32;
typedef unsigned short	u16;
typedef unsigned char 	u8;
typedef signed int	s32;
typedef signed short	s16;
typedef signed char	s8;

char filename[256]= "/tmp/ubi_v/ubi_tmp_file";
void usage(void)
{
	printf("\n\tThis program is used to test flash fs write and read performance, use"\
		" test_file [filename] the set the file for test!\n");
}

int main(int argc,char *argv[])
{
	int i, j;
	int fd;
	char buf[BUFFER_SIZE];
	float spd;
	struct timeval start, end;
	u32 time_interval_us, filesize = 0, bytes;

	if(argc == 1){
		printf("defualt to read file /tmp/ubi_v/ubi_tmp_file\n");
	} else if (argc == 2) {
		strcpy(filename, argv[1]);
	} else {
		usage();
		return 0;
	}

/************** test read performance ******************/
	fd = open(filename, O_RDONLY);
	if(fd == -1) {
		printf("Can't open the file for read test %s\n", filename);
                exit(1);
	}

	gettimeofday(&start, NULL);
	printf("start time\t[%ds:%dus]\n", (u32)start.tv_sec, (u32)start.tv_usec);

	while((bytes = read(fd, buf, BUFFER_SIZE)) > 0 ) {
		filesize += bytes;
	}

	close(fd);

	gettimeofday(&end, NULL);
	printf("end time\t[%ds:%dus]\n", (u32)end.tv_sec,(u32) end.tv_usec);

	time_interval_us = (end.tv_sec - start.tv_sec) * 1000000 +
		end.tv_usec - start.tv_usec;

	if (time_interval_us == 0) {
		printf(" time_interval_us is 0, you need read more date for the test\n");
	} else {
		printf("spend [%dus] to read %d MB data\n", time_interval_us, filesize / (1024 * 1024));

		spd = filesize;
		spd = spd / (1024 * 1024);
		spd = (spd * 1000 * 1000 ) / time_interval_us;

		printf("flash fs read speed is %.6f MB/s\n", spd);
	}

/************** test write performance ******************/
	fd = creat(filename, 0);
	if(fd == -1) {
		printf("Can't create or write file!\n");
                exit(1);
	}

	gettimeofday(&start, NULL);
	printf("start time\t[%ds:%dus]\n", (u32)start.tv_sec, (u32)start.tv_usec);

	for(j = 0; j < LOOP_EXT; j++){
		for(i = 0; i < LOOP_INN; i++){
			write(fd, buf, BUFFER_SIZE);
		}
	}

	fsync(fd);
	close(fd);

	gettimeofday(&end, NULL);
	printf("end time\t[%ds:%dus]\n", (u32)end.tv_sec,(u32) end.tv_usec);

	time_interval_us = (end.tv_sec - start.tv_sec) * 1000000 +
		end.tv_usec - start.tv_usec;

	filesize = BUFFER_SIZE * LOOP_EXT * LOOP_INN / (1024 * 1024); // Mega Bytes
	printf("spend [%dus] to write %d MB data\n", time_interval_us, filesize);

	if (time_interval_us == 0) {
		printf(" time_interval_us is 0, you need write more data for the test\n");
		return -1;
	}

	spd = filesize;
	spd = (spd * 1000 * 1000 ) / time_interval_us;

	printf("flash fs write speed is %.6f MB/s\n", spd);

	return 0;
}


