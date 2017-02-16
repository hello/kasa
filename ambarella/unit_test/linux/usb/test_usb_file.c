/*
 * test_usb_file.c
 *
 * History:
 *	2009/01/04 - [Cao Rongrong] created file
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
#include <sys/time.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include "basetypes.h"

#include "amba_usb.h"

char *device = "/dev/amb_gadget";

char *default_file_name = "/mnt/usb_up_file.dat";
char file_to_write[256];// = "/mnt/file_usb.dat";
//char *file_to_read = "/mnt/media/test_main.264";

int fd_usb = 0;

#define PORT_NUM 2000
#define MAX_MEASURE_NUM	500 * 1024 * 1024

static u32 rx_state;
static int total_length = 0;

char filename[256];
struct amb_usb_buf *usb_buf = NULL;

u32 pkt_size;

void usage(void)
{

	printf("\nusage:\n");
	printf("\tThis program is used to verify the feasibility of data"
		" transfer with usb.\n");
	printf("\tIt use a simple protocol to transfer data between host"
		" and the platform.\n");
	printf("\nPress 'CTRL + C' to quit\n\n");
}

int send_data(const char *buf, int length)
{
	int rval = 0;

	rval = write(fd_usb, buf, length);
	if(rval < 0)
		perror("send_data error\n");
	
	return rval;
}

int send_file()
{
	int rval;
	int fd = 0;
	static unsigned long num = 0;

	total_length = 0;

	if(rx_state != MEASURE_SPEED_PHASE) {
		fd = open(filename, O_RDONLY);
		if (fd < 0) {
			perror("open file to read");
			return fd;
		}
	}

	while(1){
		if(rx_state == MEASURE_SPEED_PHASE){
			/*for(i = 0; i < pkt_size - USB_HEAD_SIZE; i++)
				usb_buf->buffer[i] = i;
			*/

			num += (pkt_size - USB_HEAD_SIZE);
			if(num < MAX_MEASURE_NUM)
				rval = pkt_size - USB_HEAD_SIZE;
			else
				rval = 0;
		} else {
			rval = read(fd, usb_buf->buffer, pkt_size - USB_HEAD_SIZE);
			if (rval < 0)
				break;
		}

		total_length += rval;

		usb_buf->head.port_id = PORT_NUM;
		usb_buf->head.size = rval;
		rval = send_data((char *)usb_buf, pkt_size);
		if(rval < 0)
			break;

		/* read the file end */
		if(usb_buf->head.size == 0){
			num = 0;
			break;
		}
	}

	if(rx_state != MEASURE_SPEED_PHASE) {
		close(fd);
	}
	return rval;
	
}

int recv_data(char *buf, int length)
{
	int rval = 0;
	struct amb_usb_head *head;

	rval = read(fd_usb, buf, length);
	if(rval < 0){
		perror("read usb error");
		return rval;
	}

	head = (struct amb_usb_head *)buf;
	if(head->port_id != PORT_NUM){
		printf("port_id error: %d\n", head->port_id);
		return -1;
	}

	return rval;
}

int recv_file()
{
	int rval = 0, fd = 0;

	total_length = 0;

	if(rx_state != MEASURE_SPEED_PHASE){
		fd = open(file_to_write, O_CREAT | O_TRUNC | O_WRONLY, 0644);
		if (fd < 0) {
			perror("can't open file to write");
			return fd;
		}
	}

	while(1){
		rval = recv_data((char *)usb_buf, pkt_size);
		if (rval < 0){
			printf("recv_data error\n");
			break;
		}

		if(rx_state == MEASURE_SPEED_PHASE){
			rval = usb_buf->head.size;
		} else {
			rval = write(fd, usb_buf->buffer, usb_buf->head.size);
			if(rval < 0){
				perror("write data");
				break;
			}
		}

		total_length += rval;
		
		if(rval == 0){
			break;
		}
	}

	if(rx_state != MEASURE_SPEED_PHASE)
		close(fd);
	return rval;
}


int get_command(struct amb_usb_cmd *cmd)
{
	int rval = 0;

	rval = ioctl(fd_usb, AMB_DATA_STREAM_RD_CMD, cmd);
	if (rval < 0) {
		printf("read command error");
 		return rval;
	}

	if (cmd->signature != AMB_COMMAND_TOKEN) {
		printf("Wrong signature: %08x\n", cmd->signature);
		return -EINVAL;
	}

	//printf("signature = 0x%x, command = 0x%x, parameter[0] = %d\n",
	//	cmd->signature, cmd->command, cmd->parameter[0]);

	return 0;
}

int set_response(int response)
{
	struct amb_usb_rsp rsp;

	rsp.signature = AMB_STATUS_TOKEN;
	rsp.response = response;

	if (ioctl(fd_usb, AMB_DATA_STREAM_WR_RSP, &rsp) < 0) {
		perror("ioctl error");
		return -1;
	}

	return 0;
}


int main(int argc,char *argv[])
{
	int rval = 0;
	char *file_tmp;
	struct timeval tm_begin, tm_end;
	int run_time;
	
	if( argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)){
		usage();
		return 1;
	}

	if(argc == 2)
		file_tmp = argv[1];
	else
		file_tmp = default_file_name;

	rx_state = COMMAND_PHASE;

	fd_usb = open(device, O_RDWR);
	if (fd_usb < 0) {
		perror("can't open device");
		exit(1);
	}

	rval = 0;
	while(1){
		switch(rx_state)
		{
		case COMMAND_PHASE:
		{
			struct amb_usb_cmd cmd;
			u32 rsp;
			
			strcpy(filename, file_tmp);
			
			get_command(&cmd);
			/* Process command */
			switch (cmd.command)
			{
			case USB_CMD_SET_MODE:
				rsp = AMB_RSP_SUCCESS;

				if (cmd.parameter[0] == AMB_CMD_PARA_UP)
					rx_state = SND_FILENAME_PHASE;
				else if(cmd.parameter[0] == AMB_CMD_PARA_DOWN)
					rx_state = RCV_FILENAME_PHASE;
				else if(cmd.parameter[0] == AMB_CMD_PARA_MEASURE_SPD)
					rx_state = MEASURE_SPEED_PHASE;					
				else {
					printf("parameter error: %d\n", cmd.parameter[0]);
					rsp = AMB_RSP_FAILED;
				}

				pkt_size = cmd.parameter[1];
				usb_buf = malloc(pkt_size);

				/* response to the host */
				set_response(rsp);					
				break;

			default:
				printf ("Unknown command: %08x\n", cmd.command);
				rval = -1;
				break;
			}

			break;
		}

		case SND_FILENAME_PHASE:
			usb_buf->head.port_id = PORT_NUM;
			usb_buf->head.size = sizeof(filename);
			strcpy(usb_buf->buffer, filename);
			
			rval = send_data((char *)usb_buf, pkt_size);
			if(rval < 0)
				break;
			
			rx_state = SND_DATA_PHASE;
			break;
			
		case SND_DATA_PHASE:
			gettimeofday(&tm_begin, NULL);
			rval = send_file();
			gettimeofday(&tm_end, NULL);
			run_time = (tm_end.tv_sec - tm_begin.tv_sec) * 1000000 +
				(tm_end.tv_usec - tm_begin.tv_usec);
			printf("send done, total size = %d\n", total_length);
			if(run_time > 0){
				printf("USB transfer speed is %.2f MB/s\n\n", 
					(float)total_length / run_time);
			}
			else {
				printf("the file size is too small to calculate "
					"transfer speed\n\n");
			}
			
			if(usb_buf){
				free(usb_buf);
				usb_buf = NULL;
			}
			rx_state = COMMAND_PHASE;
			break;

		case RCV_FILENAME_PHASE:
			rval = recv_data((char *)usb_buf, pkt_size);
			if(rval < 0){
				printf("recv filename error\n");
				break;
			}

			sprintf(filename, "%s", usb_buf->buffer);
			sprintf(file_to_write, "/mnt/%s", usb_buf->buffer);
			rx_state = RCV_DATA_PHASE;
			break;

		case RCV_DATA_PHASE:
			gettimeofday(&tm_begin, NULL);
			rval = recv_file();
			gettimeofday(&tm_end, NULL);
			run_time = (tm_end.tv_sec - tm_begin.tv_sec) * 1000000 +
				(tm_end.tv_usec - tm_begin.tv_usec);
			printf("recv done, total size = %d\n", total_length);
			if(run_time > 0){
				printf("USB transfer speed is %.2f MB/s\n\n",
					(float)total_length / run_time);
			}
			else {
				printf("the file size is too small to calculate "
					"transfer speed\n\n");
			}
			
			if(usb_buf){
				free(usb_buf);
				usb_buf = NULL;
			}
			rx_state = COMMAND_PHASE;		
			break;

		case MEASURE_SPEED_PHASE:
		{
			char measure[256] = "measure data";
			
			/* download from PC */
			gettimeofday(&tm_begin, NULL);
			rval = recv_file();
			gettimeofday(&tm_end, NULL);
			printf("recv done, total size = %d\n", total_length);
			run_time = (tm_end.tv_sec - tm_begin.tv_sec) * 1000000 +
				(tm_end.tv_usec - tm_begin.tv_usec);
			printf("USB download speed is %.2f MB/s\n\n",
					(float)total_length / run_time);

			/* upload to PC */
			usb_buf->head.port_id = PORT_NUM;
			usb_buf->head.size = sizeof(measure);
			strcpy(usb_buf->buffer, measure);			
			rval = send_data((char *)usb_buf, pkt_size);
			if(rval < 0)
				break;

			gettimeofday(&tm_begin, NULL);
			rval = send_file();
			gettimeofday(&tm_end, NULL);
			printf("send done, total size = %d\n", total_length);
			run_time = (tm_end.tv_sec - tm_begin.tv_sec) * 1000000 +
				(tm_end.tv_usec - tm_begin.tv_usec);
			printf("USB upload speed is %.2f MB/s\n\n",
					(float)total_length / run_time);

			if(usb_buf){
				free(usb_buf);
				usb_buf = NULL;
			}
			rx_state = COMMAND_PHASE;
			break;
		}

		default:
			printf("rx_state (PHASE) error\n");
			break;

		}

		if(rval < 0){
			printf("some error happened\n");
			break;
		}
	}

	close(fd_usb);

	
	return 0;
}


