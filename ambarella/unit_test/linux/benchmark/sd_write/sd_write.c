/*
 * sd_write.c
 *
 * History:
 *	2011/5/30 - [Tao Wu] created file
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
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
#include <pthread.h>

#ifdef __UCLIBC__
#include <bits/getopt.h>
#else
#include <getopt.h>
#endif

#define MAX_THREAD 128
#define MAX_SIZE 	256
#define FRE_30		33333	//frequency = 30
#define FRE_60		16667	//frequcncy = 60

struct create_thread
{
	char fName[MAX_SIZE];
	int threadNum;
	int time;
	int size;
	int flag_show;
	int flag_sync;
};

struct write_data
{
	char fileName[MAX_SIZE];
	int write_time;
	int write_size;
	int write_freq;
	int flag_show;
	int flag_sync;
	int fd;
	char *buff;
};

struct tim
{
	int sec;
	int usec;
};

pthread_cond_t cntl_cond;
pthread_mutex_t cntl_mutex;
static struct create_thread gRunThread[MAX_THREAD];
static struct write_data gData[MAX_SIZE];
static struct tim gTimer;
static int total_time = 0;
static int every_freq_time = 0;
static int gFreq = 1;

void write_sd ( const void *_pData )
{
	int i,j;
	int num = 0;
	unsigned int time_interval_us;
	struct timeval time_point1, time_point2;
	struct write_data *pData = (struct write_data*) _pData;
	pData->buff = (char *) malloc ( pData->write_size );

	pData->fd = open(pData->fileName, O_CREAT | O_TRUNC | O_RDWR, 0666);
	if( pData->fd < 0 ) {
		perror("open_file:");
		exit(1);
	}
	while (1){
		if ( pData->fd ){
			for (j = 0; j < gFreq; j ++){
				if ( pData->flag_show && (num == 0) ) {
					gettimeofday(&time_point1, NULL);
					//printf("gettimeofday1 : tv_sec =%d, tv_usec =%d \n",  time_point1.tv_sec, time_point1.tv_usec);
				}
				pthread_cond_wait(&cntl_cond, &cntl_mutex);
				for ( i = 0; i < pData->write_time; i++){
					if(write(pData->fd, pData->buff, pData->write_size) != pData->write_size){
						printf("write error!\n");
						exit(1);
					}
				}
				num++;
				if ( pData->flag_show && (num == gFreq) ) {
					gettimeofday(&time_point2, NULL);
					//printf("gettimeofday2 : tv_sec =%d, tv_usec =%d \n",  time_point2.tv_sec, time_point2.tv_usec);
					time_interval_us = (time_point2.tv_sec - time_point1.tv_sec) * 1000000 +
									time_point2.tv_usec - time_point1.tv_usec;
					if ( time_interval_us != 0)
						printf("[%s]Write speed=%.1f KByte/s\n", pData->fileName, ( (double) (pData->write_size * pData->write_time * gFreq) / (time_interval_us/1000.f)));
					num = 0;
				}
			}
			if ( pData->flag_sync ) {
				if (fsync (pData->fd) < 0){
					perror("fsync");
					exit(1);
				}
			}
		}
		else
			printf("File not found!\n");
	}
}

int create_write_pthread(struct create_thread *runThread)
{
	int i = 0;
	int j = 0;
	int ret = 0;
	pthread_t tid[MAX_SIZE];
	char fileName[MAX_SIZE];
	pthread_cond_init(&cntl_cond, NULL);

	while ( runThread->threadNum != 0 ) {
		for (i = 0; i < runThread->threadNum; i++){
			sprintf (fileName, "%s_%d", runThread->fName, i);
			strcpy(gData[j].fileName, fileName);

			gData[j].write_size = runThread->size;
			gData[j].write_time = runThread->time;

			gData[j].flag_show = runThread->flag_show;
			gData[j].flag_sync = runThread->flag_sync;

			ret = pthread_create (&tid[j], NULL, (void *)write_sd, (void *)&gData[j]);
			if (ret != 0){
				printf("Create pthread error!\n");
			}
			j++;
		}
		runThread ++;
	}

	return 0;
}

void send_sig(int signo)
{
	if (signo == SIGALRM){
		pthread_cond_broadcast(&cntl_cond);
		every_freq_time ++;
		total_time ++;
		if (every_freq_time == gFreq){
			every_freq_time = 0;
			printf("******************************\n");
		}
	}
}

int init_time(struct tim timer)
{
	struct itimerval value, ovalue;

	signal(SIGALRM, send_sig);
	value.it_value.tv_sec = timer.sec;
	value.it_value.tv_usec = timer.usec;
	value.it_interval = value.it_value;
	setitimer(ITIMER_REAL, &value, &ovalue);

	return 0;
}

//quit process when tpye 'q' + <Enter>.
int loop_parm(int argc, char **argv)
{
	char buffer[MAX_SIZE];
	 if (read (STDIN_FILENO, buffer, sizeof (buffer)) < 0)
	 	return -1;
	 else if ( buffer[0] == 'q' ){
	 	printf("Quit!\n");
	 }
	 return 0;
}

void usage(void)
{
	printf("sd_write: write data with regular speed, can be used by multi-pthread. \n");
	printf("Usag:\t sd_write -f [File Name] -n [Thread Num] \n");
	printf("\t -s [Buffer Size, Unit: Byte] -t [Write time] -r [Write frequency] \n");
	printf("\t -m <Whether show write speed> -c <Whether sync file after write>\n");
	printf("Quit program: q + <Enter> \n");
	printf("Example: 4 thread write \"/tmp/aa\" with 1.5MByte/s, show write speed, sync file after write,\n");
	printf("\t2 thread write \"/tmp/bb\" with 3MByte/s. don't show write speed, don't sync file after write.\n");
	printf("\t Note: /tmp/aa is prefix of file name, add thread num will generate complete file name.\n");
	printf("cmd: sd_write -f /tmp/aa -n 4 -s 4096 -t 3 -r 30 -m -c -f /tmp/bb -n 2 -s 4096 -t 10 -r 30\n");
	printf("\t 1.5MByte/s = 4096Byte x 25 x 15, 3MByte/s = 4096Byte x 25 x 30\n");
}

void show_config (struct create_thread *runThread)
{
	while ( runThread->threadNum != 0 ) {
		printf("Config: fName=%s, threadNum=%d, time=%d ,size=%d, freq=%d, show=%d, sync=%d, speed[%d]Byte/s.\n",
			runThread->fName, runThread->threadNum, runThread->time, runThread->size, gFreq,
			runThread->flag_show, runThread->flag_sync, runThread->size * runThread->time * gFreq);
		runThread++;
	}
}

static void sigstop()
 {
	unsigned long file_size;
	struct create_thread *runThread  = gRunThread;
	struct write_data *pData = (struct write_data*) gData;
	printf("All process spend about %d sec. \n", total_time/gFreq);
	while ( runThread->threadNum != 0 ) {
		file_size = runThread->size * runThread->time * total_time /1024;
		printf("[%s_*] size is about %ld KByte.\n", runThread->fName, file_size);
		runThread ++;
	}
	while ( pData->buff != NULL){
		free ( pData->buff );
		close ( pData->fd );
		pData ++;
	}
}

int init_opt(int argc, char **argv)
{
	int ch = 0;
	int n = -1;
	int flag_freq_received = 0;
	while ( ( ch = getopt(argc, argv, "f:n:s:t:r:mch" ) ) != -1) {
		switch ( ch ) {
			case 'f':
				n++;
				strcpy(gRunThread[n].fName, optarg);
				break;
			case 'n':
				gRunThread[n].threadNum = atoi(optarg);
				break;

			case 's':
				gRunThread[n].size = atoi(optarg);
				break;

			case 't':
				gRunThread[n].time= atoi(optarg);
				break;

			case 'r':
				if ( !flag_freq_received ) {
					gFreq = atof(optarg);
				}
				if ( gFreq == (float)30){
					gTimer.sec = 0;
					gTimer.usec = FRE_30;
				}else if (gFreq == (float)60){
					gTimer.sec = 0;
					gTimer.usec = FRE_60;
				}else if (gFreq == (float)1){
					gTimer.sec = 1;
					gTimer.usec = 0;
				}else {
					gTimer.sec = (int)  ( 1 / gFreq );
					gTimer.usec =(int) ( ((1 / gFreq) - gTimer.sec) * 1000000 );
				}
				break;

			case 'm':
				gRunThread[n].flag_show = 1;
				break;

			case 'c':
				gRunThread[n].flag_sync = 1;
				break;

			case 'h':
				usage();
				exit(0);

				break;
			default:
				printf("unknown option found %c or option requires an argument \n", ch);
				return -1;
				break;
		}
	}
	show_config(gRunThread);
	return 0;
}


int main(int argc, char ** argv)
{
	 //register signal handler for Ctrl+C,  Ctrl+'\'  ,  and "kill" sys cmd
	signal(SIGINT, 	sigstop);
	signal(SIGQUIT,	sigstop);
	signal(SIGTERM,	sigstop);

	if (argc < 2){
		usage();
		return -1;
	}

	init_opt(argc, argv);
	create_write_pthread(gRunThread);
	init_time(gTimer);

	while ( 1 ){
		if( !(loop_parm( argc, argv) ) ){
			sigstop();
			return 0;
		}
	}

	return 0;
}


