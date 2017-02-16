/*******************************************************************************
 * log.c
 *
 * History:
 *  Jun 5, 2013 - [qianshen] created file
 *
 * Copyright (C) 2012-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include "utils.h"

#define LOG_STRING_SIZE_MAX        (1024)
#define LOG_FORMAT_SIZE_MAX        (512)
#define LOG_FILENAME_SIZE_MAX      (256)

typedef enum {
	LOG_TARGET_STDERR,
	LOG_TARGET_FILE,
} LOG_TARGET;

static LOG_LEVEL log_level = LOG_INFO;
static LOG_TARGET log_target = LOG_TARGET_STDERR;
static char log_file[LOG_FILENAME_SIZE_MAX] = "stderr";

static int print_log(const char *type, const char* func, const char *format,
                     va_list vlist, LOG_LEVEL level)
{
	int fd = -1;
	char str[LOG_STRING_SIZE_MAX];
	char fmt[LOG_FORMAT_SIZE_MAX];
	int color = (level < LOG_INFO ? 31 : 34);
	snprintf(fmt, sizeof(fmt), "\033[%dm[%s]\t%s: %s\033[39m", color, type,
	         func, format);
	vsnprintf(str, sizeof(str), fmt, vlist);

	switch (log_target) {
	case LOG_TARGET_FILE:
		fd = open(log_file, O_WRONLY|O_TRUNC|O_CREAT, S_IRUSR|S_IWUSR);
		if (fd < 0 || write(fd, str, strlen(str)) < 0) {
			fprintf(stderr, "Fail to open file [%s]."
					"Set log file to stderr.\n", log_file);
			log_target = LOG_TARGET_STDERR;
			snprintf(log_file, sizeof(log_file), "stderr");
		}
		if (fd >= 0) {
			close(fd);
			fd = -1;
		}
		if (level <= LOG_INFO)
			vfprintf(stderr, fmt, vlist);
		break;
	case LOG_TARGET_STDERR:
	default:
		vfprintf(stderr, fmt, vlist);
		break;
	}
	return 0;
}

int log_error(const char* func, const char *format, ...)
{
	va_list vlist;
	if (log_level >= LOG_ERROR) {
	    va_start(vlist, format);
	    print_log("ERROR", func, format, vlist, LOG_ERROR);
	    va_end(vlist);
	}
	return 0;
}

int log_info(const char* func, const char *format, ...)
{
	va_list vlist;
	if (log_level >= LOG_INFO) {
	    va_start(vlist, format);
	    print_log("INFO", func, format, vlist, LOG_INFO);
	    va_end(vlist);
	}
	return 0;
}

int log_debug(const char* func, const char *format, ...)
{
	va_list vlist;
	if (log_level >= LOG_DEBUG) {
	    va_start(vlist, format);
	    print_log("DEBUG", func, format, vlist, LOG_DEBUG);
	    va_end(vlist);
	}
	return 0;
}

int log_trace(const char* func, const char *format, ...)
{
	va_list vlist;
	if (log_level >= LOG_TRACE) {
	    va_start(vlist, format);
	    print_log("TRACE", func, format, vlist, LOG_TRACE);
	    va_end(vlist);
	}
	return 0;
}

int set_log(int level, char *p_file)
{
	if (p_file == NULL || strcmp(p_file, "stderr") == 0) {
		snprintf(log_file, sizeof(log_file), "stderr");
		log_target = LOG_TARGET_STDERR;
	}
	if (level < 0) {
	  level = 0;
	} else if (level >= LOG_MAX) {
	  level = LOG_MAX - 1;
	}
	log_level = (LOG_LEVEL)level;
	DEBUG("Set log level [%d], target [%s]\n", log_level, log_file);
	return 0;
}
