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
#include <termios.h>

static int uart_config(int fd)
{
	struct termios ti;

	tcflush(fd, TCIOFLUSH);

	if (tcgetattr(fd, &ti) < 0) {
		perror("Can't get port settings");
		return -1;
	}

	cfmakeraw(&ti);

	ti.c_cflag |= CLOCAL;
	ti.c_cflag &= ~CRTSCTS;

	cfsetospeed(&ti, B115200);
	cfsetispeed(&ti, B115200);

	if (tcsetattr(fd, TCSANOW, &ti) < 0) {
		perror("Can't set port settings");
		return -1;
	}

	tcflush(fd, TCIOFLUSH);

	return 0;
}

int main(int argc, char **argv)
{
	int rval, fd;
	char buf[128];

	fd = open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd < 0) {
		perror("Can't open serial port!\n");
		return -1;
	}

	uart_config(fd);

	snprintf(buf, sizeof(buf), "\r\n!!!USER SPACE: Hello, Cruel World!!!\r\n\r\n");

	rval = write(fd, buf, strlen(buf));
	if (rval < 0) {
		perror("Can't write serial port!\n");
		return -1;
	}

	while (1);

	return 0;
}

