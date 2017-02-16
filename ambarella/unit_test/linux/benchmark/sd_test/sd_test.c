/*
 * sd_test.c
 *
 * History:
 *	2011/8/29 - [Tao Wu] created file
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <memory.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/vfs.h>

#ifdef __UCLIBC__
#include <bits/getopt.h>
#else
#include <getopt.h>
#endif
#define NO_ARG		0
#define HAS_ARG		1

static char path[128] = "/tmp/mmcblk1p1/";	// "/tmp/mmcblk1p1"  by default
static char dev[128] = "/dev/mmcblk1p1";	// "/dev/mmcblk1p1"  by default

static int file_new = 0;						// writing file now
static int file_old = 0;						// writed file last time

static int file_max = 0;						//write max file number on sdcard
static char filename[32] = "0";				// write file name

static unsigned long file_size = 0;			// one file size, unit: Byte

static unsigned long speed_max = 0;			// maximal speed
static unsigned long speed_min = 0;			// minimum speed

static unsigned long speed_total = 0;			// record every speed for calculate average speed
static unsigned long print_times = 0;			// record times totally

static int init_speed_result = 1;			// whether is initial speed
static int write_data = 0;					// flag to control whether write date
static int isSync = 0;						// don't synchronize buffer to sdcard by default

static unsigned long write_size_k = 0;		// filesize = "write_size_k" x 1024 x "write_times"
static unsigned long write_size_b = 0;		// filesize = "write_size_b" x "write_times"
static unsigned long write_times = 1;

static struct itimerval value;
static time_t interval_sec = 1;					// 1sec. by default

static unsigned long speed_den = 1;				// as denominator for calculate write speed

static unsigned long stat_size_old = 0;	// for calculate every write speed,
static unsigned long stat_size_new = 0;	// not sure just writing one file at on interval, so we need these variables

static struct option long_options[] = {
		{"Help.", 						NO_ARG, 	NULL, 	'h'},
		{"The path to write.", 				HAS_ARG, 	NULL, 	'p'},
		{"Write BYTES KBytes at a time.", 	HAS_ARG, 	NULL, 	'k'},
		{"Write BYTES Bytes at a time.", 		HAS_ARG, 	NULL, 	'b'},
		{"Write TIMES times on one file.", 	HAS_ARG, 	NULL, 	't'},
		{"Sync filesystem buffer.", 			NO_ARG, 	NULL, 	'y'},
		{"Print speed every TIMES.",			HAS_ARG, 	NULL, 	's'},
		{"mkfs.vfat device.",				HAS_ARG, 	NULL, 	'm'},
		{0, 0, 0, 0},
};

struct hint_s{
	const char *arg;
	const char *str;
};

static const char *short_options = "hp:k:b:t:yds:m:";

static const struct hint_s hint[] = {
		{"","Print command parm"},
		{"","The data path"},
		{"","Units: KByte"},
		{"","Units: Byte"},
		{"","Number of times"},
		{"","Sync after had written one file"},
		{"","Units: seconds"},
		{"","Format sdcard device in FAT32 "},
};

void usage(void)
{
	int i;
	printf("\nTest sdcard quality by show every wirte speed\n");
	printf("******************************************\n");
	printf("Example: sd_test -k 1024 -t 128 -s 1 \n\t Write [1024x128 KB=128MB] size file and print speed every [1] sec.\n");
	printf("******************************************\n");
	for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
		if (isalpha(long_options[i].val))
			printf("-%c ", long_options[i].val);
		else
			printf("   ");
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0)
			printf(" [%s]", hint[i].arg);
		printf("\t%s\n", hint[i].str);
	}
}

int format_sdcard(void)
{
	char cmd[128];

       sprintf(cmd, "umount %s", dev);

	if(system(cmd) < 0 )
		perror("umount ");
	sleep(1);
	sprintf(cmd, "mkfs.vfat %s", dev);

	if(system(cmd) < 0)
		perror("mkfs.vfat");
	sleep(1);
	sprintf(cmd, "mount %s %s", dev, path);

	if(system (cmd) < 0 )
		perror("mount");

	return 0;
}

int init_opt(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;

	while ( (ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1){
		switch(ch){
			case 'p':
				strcpy(path, optarg);
				break;
			case 'k':
				write_size_k = atol(optarg);
				printf("write_size: %ld KByte\n", write_size_k);
				write_data = 1;
				break;
			case 'b':
				write_size_b = atol(optarg);
				printf("write_size: %ld Byte\n", write_size_b);
				write_data = 1;
				break;
			case 't':
				write_times = atol(optarg);
				printf("write_times: %ld\n", write_times);
				break;
			case 'y':
				isSync = 1;
				printf("sync file system\n");
				break;
			case 's':
				interval_sec = atol(optarg);
				printf("interval seconds: %ld sec.\n", interval_sec);
				break;
			case 'h':
				usage();
				exit(0);
				break;
			case 'm':
				strcpy(dev, optarg);
				if ( format_sdcard() < 0)
					printf("Format %s failed\n", dev);
				else
					printf("Format %s success\n", dev);
				break;
			default:
				printf("unknown option found %c or option requires an argument \n", ch);
				return -1;
				break;
		}
	}
	return 0;
}

unsigned long path_free_space(void)
{
	struct statfs fs;
	if( statfs(path, &fs) < 0){
		perror("statfs");
		return 0;
	}
	return fs.f_bfree;
}

int get_block_size(char *pathname)
{
	struct statfs fs;
	if (statfs(pathname, &fs) < 0){
		perror("statfs");
		return 0;
	}
	return fs.f_bsize ;
}

void print_speed(int signo)
{
	struct stat ffs;
	long speed = 0;
	long total_wsize = 0;
	if (signo == SIGALRM){
		if( stat(filename, &ffs) < 0){
			perror("stat");
		}
		stat_size_new = ffs.st_size;

		if (file_new < file_old ) {
			total_wsize = stat_size_new - stat_size_old + (file_max + file_new - file_old ) * file_size;
			// original formula: total_wsize = stat_size_new + file_new * file_size +(file_max - 1 - file_old) * file_size + file_size - stat_size_old;
		}else{
			total_wsize = stat_size_new - stat_size_old + (file_new - file_old) * file_size;
		}

		speed = total_wsize / speed_den ; 		// Units: KB/s
		printf("filename = %d ~ %d, write speed = %ld KB/s\n",file_old, file_new, speed);

		if(init_speed_result){
			speed_min = speed;
			speed_max = speed;
			init_speed_result = 0;
		}else{
			if (speed < speed_min )
				speed_min = speed;
			if (speed > speed_max)
				speed_max = speed;
		}
		speed_total += speed;
		print_times ++;

		stat_size_old = stat_size_new;
		file_old = file_new;
	}
}

static void print_result()
{
	printf("Max write speed is: %ld KB/s\n", speed_max);
	printf("Min write speed is: %ld KB/s\n", speed_min);
	printf("Print times is: %ld\n", print_times);
	printf("Average write speed is: %ld KB/s\n", speed_total  / print_times);
	exit(0);
}

int init_timer(void)
{
	signal(SIGALRM, print_speed);
	value.it_value.tv_sec = interval_sec;
	value.it_value.tv_usec = 0;
	value.it_interval = value.it_value;
	return 0;
}

void enable_timer(int enable)
{
	struct itimerval ovalue;
	if (enable){
		value.it_value.tv_sec = interval_sec;
	}else{
		value.it_value.tv_sec = 0;
	}
	setitimer(ITIMER_REAL, &value, &ovalue);
}

int write_loop(void)
{
	int i = 0, count = 0, w_size = 0;
	int fd = -1, fister = 1;
	char *buf;

	unsigned long file_block = 0;
	if (write_size_k > 0 ){
		w_size= write_size_k * 1024;
	}else if (write_size_b > 0 ){
		w_size= write_size_b;
	}else{
		printf("Please type -k or -b to set buffer size\n");
		return -1;
	}
	buf = (char *)malloc(w_size);

	file_size = w_size * write_times;
	file_block = file_size / get_block_size(path) ;
	printf("write path: %s\n",path);
	printf("file_size = %ld Byte\n", file_size );
	printf("file_block = %ld \n", file_block ); 	 // on Linux system 1block = 4KByte
	if (chdir(path) < 0){
		perror("chdir");
		if (buf)
			free(buf);
		return -1;
	}
	enable_timer(1);
	while(1)
	{
		if ( path_free_space() <  file_block){
			if (fister){
				file_max = count;
				fister = 0;
				printf("max file = %d\n", file_max);
			}
			if (count == file_max ){
				count = 0;
				printf("sdcard has been full\n");
				fprintf(stderr, "rotat write ...\n");
			}
		}
		file_new = count ++;
		sprintf(filename, "%d", file_new);
		if( (fd = open(filename, O_CREAT | O_TRUNC | O_RDWR |O_SYNC, 0666)) < 0) {
			perror("open");
			return -1;
		}
		for (i = 0; i < write_times; i++){
			if(write( fd, buf, w_size) < 0){
				perror("write");
				break;
			}
		}
		if ( isSync ){
			if (fsync (fd) < 0){
				perror("fsync");
				return -1;
			}
		}
		close(fd);
	}
	free(buf);
	return 0;
}

int main(int argc, char **argv)
{
	signal(SIGINT, print_result);
	signal(SIGQUIT,print_result);
	signal(SIGTERM,print_result);

	if (argc < 2){
		usage();
		return -1;
	}

	if (init_opt(argc, argv) < 0)
		return -1;

	if (init_timer() < 0 )
		return -1;

	speed_den = 1024 * interval_sec;

	if (write_data){
		write_loop();
		print_result();
	}

	return 0;
}

