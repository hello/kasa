/*******************************************************************************
 * log.c
 *
 * History:
 *  Jun 5, 2013 - [qianshen] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ( "Software" ) are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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

static AMBA_LOG_LEVEL log_level = AMBA_LOG_INFO;
static LOG_TARGET log_target = LOG_TARGET_STDERR;
static char log_file[LOG_FILENAME_SIZE_MAX] = "stderr";

static int print_log(const char *type, const char* func, const char *format,
                     va_list vlist, AMBA_LOG_LEVEL level)
{
	int fd = -1;
	char str[LOG_STRING_SIZE_MAX];
	char fmt[LOG_FORMAT_SIZE_MAX];
	int color = (level < AMBA_LOG_INFO ? 31 : 34);
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
		if (level <= AMBA_LOG_INFO)
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
	if (log_level >= AMBA_LOG_ERROR) {
	    va_start(vlist, format);
	    print_log("ERROR", func, format, vlist, AMBA_LOG_ERROR);
	    va_end(vlist);
	}
	return 0;
}

int log_info(const char* func, const char *format, ...)
{
	va_list vlist;
	if (log_level >= AMBA_LOG_INFO) {
	    va_start(vlist, format);
	    print_log("INFO", func, format, vlist, AMBA_LOG_INFO);
	    va_end(vlist);
	}
	return 0;
}

int log_debug(const char* func, const char *format, ...)
{
	va_list vlist;
	if (log_level >= AMBA_LOG_DEBUG) {
	    va_start(vlist, format);
	    print_log("DEBUG", func, format, vlist, AMBA_LOG_DEBUG);
	    va_end(vlist);
	}
	return 0;
}

int log_trace(const char* func, const char *format, ...)
{
	va_list vlist;
	if (log_level >= AMBA_LOG_TRACE) {
	    va_start(vlist, format);
	    print_log("TRACE", func, format, vlist, AMBA_LOG_TRACE);
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
	} else if (level >= AMBA_LOG_MAX) {
	  level = AMBA_LOG_MAX - 1;
	}
	log_level = (AMBA_LOG_LEVEL)level;
	DEBUG("Set log level [%d], target [%s]\n", log_level, log_file);
	return 0;
}
