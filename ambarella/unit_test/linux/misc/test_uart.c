/*
 * test_uart.c
 *
 * History:
 *	2010/12/08 - [Cao Rongrong ] Created file
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#ifdef __UCLIBC__
#include <bits/getopt.h>
#else
#include <getopt.h>
#endif
#include <pthread.h>
#include <signal.h>
#include <termios.h>
#include <sys/time.h>

#define BUF_SIZE	(512*1024)

static int __io_canceled_write = 0;
static int __io_canceled_read = 0;

static char uart_dev[128];
static int baudrate = 115200;
static int flowctrl = 0;
static int io_flag = 0; /* 0(default): loop;  1: read;  2: write  */
static int string = 0;
static int num = BUF_SIZE;
static int blk = 1;

static int uart_fd = -1;
static unsigned long total_read_count = 0;
static unsigned long total_write_count = 0;


#define NO_ARG				0
#define HAS_ARG				1

static struct option long_options[] = {
	{"baudrate", HAS_ARG, 0, 'r'},
	{"flow", NO_ARG, 0, 1},
	{"read", NO_ARG, 0, 2},
	{"write", NO_ARG, 0, 3},
	{"string", NO_ARG, 0, 4},
	{"blk", NO_ARG, 0, 'b'},
	{"num", NO_ARG, 0, 'n'},
	{0, 0, 0, 0}
};

static const char *short_options = "r:n:b:";

void usage(void)
{
	printf("\nUSAGE: test_uart [OPTION] device\n");
	printf("\t-h, --help		Help\n"
		"\t-r, --baudrate=#	Specify baud rate: 2400~1500000\n"
		"\t    --flow		Enable flow control\n"
		"\t    --read		Read from UART\n"
		"\t    --write		Write to UART\n"
		"\t    --string		Test string\n"
		"\t-b  --blk		block size per transfer\n"
		"\t-n  --num		Number to transfer\n");
	printf("\n");
}

int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;
	opterr = 0;

	uart_dev[0] = '\0';

	while ((ch = getopt_long(argc, argv, short_options, long_options,
		&option_index)) != -1) {
		switch (ch) {
		case 'r':
			baudrate = atoi(optarg);
			break;
		case 1:
			flowctrl = 1;
			break;
		case 2:
			io_flag = 1;
			break;
		case 3:
			io_flag = 2;
			break;
		case 4:
			string = 1;
			break;
		case 'b':
			blk = atoi(optarg);
			break;
		case 'n':
			num = atoi(optarg);
			break;

		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}

	if (num > BUF_SIZE || blk > num) {
		printf("Invalid num or blk: %d, %d\n", num, blk);
		return -1;
	}

	if(optind <= argc -1)
		strcpy(uart_dev, argv[optind]);

	if (strlen(uart_dev) == 0)
		return -1;

	return 0;
}

static int uart_speed(int s)
{
	switch (s) {
	case 2400:
		return B2400;
	case 4800:
		return B4800;
	case 9600:
		return B9600;
	case 19200:
		return B19200;
	case 38400:
		return B38400;
	case 57600:
		return B57600;
	case 115200:
		return B115200;
	case 230400:
		return B230400;
	case 460800:
		return B460800;
	case 500000:
		return B500000;
	case 576000:
		return B576000;
	case 921600:
		return B921600;
	case 1000000:
		return B1000000;
	case 1152000:
		return B1152000;
	case 1500000:
		return B1500000;
	default:
		return -1;
	}
}

static int uart_config(int fd, int speed, int flow)
{
	struct termios ti;

	tcflush(fd, TCIOFLUSH);

	if (tcgetattr(fd, &ti) < 0) {
		perror("Can't get port settings");
		return -1;
	}

	cfmakeraw(&ti);

	ti.c_cflag |= CLOCAL;
	if (flow)
		ti.c_cflag |= CRTSCTS;
	else
		ti.c_cflag &= ~CRTSCTS;

	cfsetospeed(&ti, speed);
	cfsetispeed(&ti, speed);

	if (tcsetattr(fd, TCSANOW, &ti) < 0) {
		perror("Can't set port settings");
		return -1;
	}

	tcflush(fd, TCIOFLUSH);

	return 0;
}

static int uart_restore(int fd)
{
	return uart_config(fd, B115200, 0);
}

static void *test_uart_read(void *args)
{
	int i = 0, j, n, rval;
	char buf_rd[BUF_SIZE];
	struct timeval base_time;
	struct timeval diff_time;
	int usec, sec;

	memset(buf_rd, 0, BUF_SIZE);

	gettimeofday(&base_time, NULL);
	while (!__io_canceled_read) {
		n = (BUF_SIZE - i) < blk ? (BUF_SIZE - 1) : blk;
		rval = read(uart_fd, buf_rd + i, n);
		if (rval < 0) {
			if (errno == EAGAIN) {
				usleep(10);
				continue;
			} else {
				break;
			}
		}

		total_read_count += n;

		if (string) {
			for (j = i; j < i + n; j++) {
				if (isprint(buf_rd[j]))
					printf("%c", buf_rd[j]);
				else
					printf("[%d]", buf_rd[j]);
			}
			if (total_read_count >= num) {
				__io_canceled_write = 1;
				__io_canceled_read = 1;
				printf("\n");
				break;
			}
		}

		/* Check data if we are in loop mode */
		if (io_flag == 0 && buf_rd[i] != (i % 256)) {
			__io_canceled_write = 1;
			__io_canceled_read = 1;
			printf("Invalid data: buf_rd[%d] = %d, should be %d\n",
				i, buf_rd[i], (i % 256));
			break;
		}

		i += n;
		if (i >= BUF_SIZE)
			i = 0;
	}
	gettimeofday(&diff_time, NULL);
	sec = (int)diff_time.tv_sec - (int)base_time.tv_sec;
	usec = (int)diff_time.tv_usec - (int)base_time.tv_usec;
	printf("Read %ld Bytes in [%8dus], about %8.1fbps \n",
		total_read_count, usec + sec * 1000000,
		((double)total_read_count * 1000000 * 8) /
		(usec + sec * 1000000));

	return NULL;
}

static void *test_uart_write(void *args)
{
	int i, j, k, n, rval;
	char buf_wr[BUF_SIZE];
	struct timeval base_time;
	struct timeval diff_time;
	int usec, sec;

	j = k = 0;
	for (i = 0; i < BUF_SIZE; i++) {
		if (string) {
			/* generate following character sequences:
			 * "abcde..xyzABCDE..XYZ0123456789abcd..." */
			switch (j % 3) {
			case 0:
				buf_wr[i] = 'a' + (k % 26);
				n = 26;
				break;
			case 1:
				buf_wr[i] = 'A' + (k % 26);
				n = 26;
				break;
			case 2:
				buf_wr[i] = '0' + (k % 10);
				n = 10;
				break;

			}
			if (++k >= n) {
				k = 0;
				j++;
			}
		} else {
			buf_wr[i] = i;
		}
	}

	/* Wait for read thread ready */
	for (i = 0; i < BUF_SIZE; i++)
		;

	i = 0;
	gettimeofday(&base_time, NULL);
	while (!__io_canceled_write) {
		n = (BUF_SIZE - i) < blk ? (BUF_SIZE - 1) : blk;
		rval = write(uart_fd, buf_wr + i, n);
		if (rval < 0) {
			if (errno == EAGAIN) {
				usleep(10);
				continue;
			} else {
				break;
			}
		}

		total_write_count += n;

		if (string && total_write_count >= num) {
			__io_canceled_write = 1;
			__io_canceled_read = 1;
			break;
		}

		i += n;
		if (i >= BUF_SIZE)
			i = 0;
	}
	gettimeofday(&diff_time, NULL);
	sec = (int)diff_time.tv_sec - (int)base_time.tv_sec;
	usec = (int)diff_time.tv_usec - (int)base_time.tv_usec;
	printf("Write %ld Bytes in [%8dus], about %8.1fbps \n",
		total_write_count, usec + sec * 1000000,
		((double)total_write_count * 1000000 * 8) /
		(usec + sec * 1000000));

	return NULL;
}

int main(int argc, char **argv)
{
	int rval, speed;
	pthread_t thread_rd;
	pthread_t thread_wr;

	if (init_param(argc, argv) < 0) {
		usage();
		return -1;
	}

	printf("Start to %s %s: baudrate = %d, %sflow control\n\n",
		io_flag == 0 ? "test(LOOP)" : io_flag == 1 ? "READ" : "WRITE",
		uart_dev, baudrate,
		flowctrl ? "" : "No ");

	uart_fd = open(uart_dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (uart_fd < 0) {
		perror("Can't open serial port");
		return -1;
	}

	speed = uart_speed(baudrate);
	if (speed < 0) {
		printf("Invalid baudrate!\n");
		return -1;
	}
	uart_config(uart_fd, speed, flowctrl);

	if (io_flag == 0 || io_flag == 1) {
		rval = pthread_create(&thread_rd, NULL, test_uart_read, NULL);
		if (rval < 0)
			perror("pthread_create test_uart_read failed\n");
	}

	if (io_flag == 0 || io_flag == 2) {
		rval = pthread_create(&thread_wr, NULL, test_uart_write, NULL);
		if (rval < 0)
			perror("pthread_create test_uart_write failed\n");
	}

	if (io_flag == 0 || io_flag == 2) {
		printf("Enter q[Q] to quit write\n");
		while (1) {
			char c;
			scanf("%c", &c);
			if (c == 'q' || c == 'Q' || (__io_canceled_write != 0))
				break;
		}
		__io_canceled_write = 1;
		pthread_join(thread_wr, NULL);
	}

	if (io_flag == 0 || io_flag == 1) {
		printf("Enter q[Q] to quit read\n");
		while (1) {
			char c;
			scanf("%c", &c);
			if (c == 'q' || c == 'Q' || (__io_canceled_read != 0))
				break;
		}
		__io_canceled_read = 1;
		pthread_join(thread_rd, NULL);
	}

	uart_restore(uart_fd);

	return 0;
}

