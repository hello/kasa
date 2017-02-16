/*
 * dsplog.c
 *
 * History:
 *	2008/5/5 - [Oliver Li] created file
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <libbb.h>

#include "../../../ambarella/build/include/basetypes.h"
#include "../../../ambarella/build/include/amba_debug.h"
#include "../../../ambarella/build/include/iav_drv.h"

#define	AMBA_DEBUG_DSP	(1 << 1)

static int enable_dsplog(void);
static int setup_log(char **argv);
int dsplog_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;

static int enable_dsplog(void)
{
	int fd;
	int debug_flag;

	if ((fd = open("/dev/ambad", O_RDWR, 0)) < 0) {
		perror("/dev/ambad");
		return -1;
	}

	if (ioctl(fd, AMBA_DEBUG_IOC_GET_DEBUG_FLAG, &debug_flag) < 0) {
		perror("AMBA_DEBUG_IOC_GET_DEBUG_FLAG");
		return -1;
	}

	debug_flag |= AMBA_DEBUG_DSP;

	if (ioctl(fd, AMBA_DEBUG_IOC_SET_DEBUG_FLAG, &debug_flag) < 0) {
		perror("AMBA_DEBUG_IOC_SET_DEBUG_FLAG");
		return -1;
	}

	close(fd);
	return 0;
}

static int setup_log(char **argv)
{
	int iav_fd;

	u8 module = atoi(argv[0]);
	u8 level = atoi(argv[1]);
	u8 thread = atoi(argv[2]);

	if ((iav_fd = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (ioctl(iav_fd, IAV_IOC_LOG_SETUP, (module | (level << 8) | (thread << 16))) < 0) {
		perror("IAV_IOC_LOG_SETUP");
		return -1;
	}

	return 0;
}

// dsplog log-filename [ 1000 (max log size) ]
// dsplog d module debuglevel coding_thread_printf_disable_mask
int dsplog_main(int argc, char **argv)
{
	int maxlogs = 0;
	const char *logfile;
	int fd;
	int fd_write;
	int bytes_not_sync = 0;
	int bytes_written = 0;
	char *buffer;

	if (argc > 1) {
		logfile = argv[1];
		if (logfile[0] == 'd' && logfile[1] == '\0') {
			if (argc != 5) {
				printf("usage: dsplog d x y z\n");
				return -1;
			}
			setup_log(&argv[2]);
			return 0;
		}

		if (argc >= 2) {
			maxlogs = atoi(argv[2]);
			if (maxlogs < 0) {
				printf("usage: dsplog log-filename [ 1000 (max log size) ] \n");
				maxlogs = 0;
			}
		}
	} else {
		logfile = "/tmp/dsplog.dat";
	}

	if ((fd_write = open(logfile, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
		perror(logfile);
		return -1;
	}
//	printf("save dsp log data to %s\n", logfile);

	if (enable_dsplog() < 0)
		return -1;

	if ((fd = open("/dev/dsplog", O_RDONLY, 0)) < 0) {
		perror("/dev/dsplog");
		return -1;
	}

	buffer = (char *)malloc(32 * 1024);
	if (!buffer) {
		printf("%s: Not enough memory!\n", __func__);
		return -1;
	}

	while (1) {
		ssize_t size = (ssize_t)read(fd, buffer, sizeof(buffer));
		if (size < 0) {
			perror("read");
			return -1;
		}

		if (size > 0) {
			if ((ssize_t)write(fd_write, buffer, size) != size) {
				perror("write");
				return -1;
			}
			bytes_not_sync += size;
		}

		if ((size == 0 && bytes_not_sync > 0) || bytes_not_sync >= 64 * 1024) {
			fsync(fd_write);
			bytes_written += bytes_not_sync;
			bytes_not_sync = 0;
		}

		if ((maxlogs != 0) && (bytes_written >= (maxlogs * sizeof(u32) * 8))) {
			lseek(fd_write, 0, SEEK_SET);
			bytes_written = 0;
		}
	}

	if (buffer)
		free(buffer);

	return 0;
}

