/*
 * test_nand.c
 *
 * History:
 *	2008/7/10 - [Cao Rongrong] created file
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */


#include "test_nand.h"

pthread_t wf_fid, rf_fid, df_fid;
FILE *fd_null;
int   fd_result;
char goon_flag;

void usage(void)
{

	printf("\nusage:\n");
	printf("\tThis program just do the Fatigue Testing for Nand Driver.\n");
	printf("\tFor functional test, please take MTD-tools.\n");
	printf("\nThe result of test times will be written to /mnt/result.txt\n");
	printf("\nPress 'CTRL + C' to quit\n\n");
}

int main(int argc,char *argv[])
{
         int error, i;
         Fname fn[10];
	 char file_name[10][30];

	 goon_flag = 1;

	 signal(SIGABRT, signal_handler);

	 if(argc > 1){
	 	usage();
		return 1;
	 }

	 for(i = 0; i < 10; i++){
	 	sprintf(file_name[i], "/home/default/%d", i);
		pthread_mutex_init(&fn[i].op_mutex, NULL);
	 	fn[i].op_file = file_name[i];
		fn[i].rd_flag = 0;
	 }

         if ((error = pthread_create(&wf_fid ,NULL, writefile, fn))<0)
         {
                  perror("can't create pthread");
                  return 1;
         }

	 if ((error = pthread_create(&rf_fid, NULL, readfile, fn))<0)
         {
                  perror("can't create pthread");
                  return 1;
         }

	 if ((error = pthread_create(&df_fid, NULL, deletefile, fn))<0)
         {
                  perror("can't create pthread");
                  return 1;
         }

         while(1);

         return 0;
}


void *writefile(void *arg)
{
	Fname *fn = (Fname *)arg;
        FILE *fd;
	int i, j;

	fprintf(stderr, "Enter writefile\n");

	while(goon_flag) {

		for(i = 0; i < 10 && goon_flag; i++){

			pthread_mutex_lock(&fn[i].op_mutex);

			fd = fopen(fn[i].op_file, "r");
			if(fd != NULL) {
				fclose(fd);
				fd = NULL;
				pthread_mutex_unlock(&fn[i].op_mutex);
		                continue;
			}

			fd = fopen(fn[i].op_file, "w+");
			if(fd == NULL) {
				fprintf(stderr, "Can't write or create file %d\n", i);
		                exit(1);
			}

			fprintf(stderr, "write file: %s\n", fn[i].op_file);
			
			fn[i].rd_flag = 0;
			for(j = 0; j < 10000; j++){
				fputc(j, fd);
			}
			
			fclose(fd);
			fd = NULL;
			
			pthread_mutex_unlock(&fn[i].op_mutex);
		}
	 }

	 pthread_exit(NULL);

         return NULL;
}

void *readfile(void *arg)
{
         Fname *fn = (Fname *)arg;
         FILE *fd;
	 int i, c;

	 fprintf(stderr, "Enter readfile\n");

	 fd_null = fopen("/dev/null", "w");
	 if(fd_null == NULL) {
	 	fprintf(stderr, "Can't open /dev/null\n");
		exit(1);
	 }

	 while(goon_flag) {

		for(i = 0; i < 10 && goon_flag; i++){
			
			pthread_mutex_lock(&fn[i].op_mutex);

			fd = fopen(fn[i].op_file, "r");
			if(fd == NULL) {
				// file is not exist
				pthread_mutex_unlock(&fn[i].op_mutex);
				continue;
			}

			fprintf(stderr, "read file: %s\n", fn[i].op_file);

	                while (!feof(fd)){
				c = fgetc(fd);
				fputc(c, fd_null);
	                }

			// have been read
			fn[i].rd_flag = 1;

			fclose(fd);
			fd = NULL;

			pthread_mutex_unlock(&fn[i].op_mutex);
		}
	 }

	 pthread_exit(NULL);

         return NULL;
}

void *deletefile(void *arg)
{
         Fname *fn = (Fname *)arg;
	 FILE *fd;
	 int i, err;
	 char read_result[50];
 	 unsigned long long times = 0;

	 fprintf(stderr, "Enter deletefile\n");

	 fd_result = open("/mnt/result.txt", O_WRONLY | O_CREAT, 0644);
	 if(fd_result < 0) {
		fprintf(stderr, "Can't create result file\n");
		exit(1);
	 }

	 while(goon_flag) {

		for(i = 0; i < 10 && goon_flag; i++){
			
			pthread_mutex_lock(&fn[i].op_mutex);

			fd = fopen(fn[i].op_file, "r");
			if(fd == NULL) {
				// file is not exist
				pthread_mutex_unlock(&fn[i].op_mutex);
		                continue;
			}
			fclose(fd);
			fd = NULL;

			if(fn[i].rd_flag == 0) {
				// file have not been read
				pthread_mutex_unlock(&fn[i].op_mutex);
				continue;
			}

			err = remove(fn[i].op_file);
			if(err < 0) {
				fprintf(stderr, "Can't delete file %d\n", i);
				exit(1);
			}

			fn[i].rd_flag = 0;

			fprintf(stderr, "delete file: %s\n", fn[i].op_file);
			
			// Written the result
			times++;
			sprintf(read_result, "Test times: %lld\r\n", times);
			err = write(fd_result, read_result, strlen(read_result));
			if(err != strlen(read_result))
				fprintf(stderr, "write result error\n");

			pthread_mutex_unlock(&fn[i].op_mutex);
		}
	 }

	 pthread_exit(NULL);
	 
         return NULL;
}


void signal_handler(int sig)
{
	fprintf(stderr, "Aborted by signal...\n");	

	goon_flag = 0;

	pthread_join(wf_fid, NULL);
	pthread_join(df_fid, NULL);
	pthread_join(df_fid, NULL);

	fclose(fd_null);
	close(fd_result);

	exit(1);
}


