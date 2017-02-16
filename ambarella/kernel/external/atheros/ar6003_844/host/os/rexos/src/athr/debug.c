/*
 * Copyright (c) 2004-2005 Atheros Communications Inc.
 * All rights reserved.
 *
 * $ATH_LICENSE_HOSTSDK0_C$
 *
 */

/* Linux/OS include files */
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>

/* Atheros platform include files */
#include "athdefs.h"
#include "a_debug.h"

pthread_mutex_t g_dbg_lock = PTHREAD_MUTEX_INITIALIZER;

#if 0
unsigned int g_dbg_flags = 0xffffffff;
#else
#define THE_DEBUG	\
		(\
			DBG_SIMULATOR | \
			DBG_HTC | \
			DBG_WMI | \
			DBG_DRIVER | \
			DBG_WLAN_ATH | \
			DBG_INFO | \
			DBG_ERROR | \
			DBG_WARNING | \
			DBG_FIRMWARE | \
            ATH_DEBUG_ANY \
		)

unsigned int g_dbg_flags = (THE_DEBUG & ~DBG_SDIO);
#endif

char *dbg_time(void)
{
	static char buff[512];
	char sbuff[16];
	time_t t;
	struct timeval tv;
	struct tm *tm;

	gettimeofday(&tv, NULL);
	t = tv.tv_sec;
	tm = localtime(&t);
	strftime(buff, sizeof(buff), "%H:%M:%S", tm);
	sprintf(sbuff, ".%3.3lu", tv.tv_usec / 1000);
	strcat(buff, sbuff);

	return(buff);
}

int dbg_print(const char *fmt, ... )
{
	va_list ap;
	int len;

	pthread_mutex_lock(&g_dbg_lock);

	va_start(ap, fmt);

	len = vfprintf(stderr, fmt, ap);

	va_end(ap);

	pthread_mutex_unlock(&g_dbg_lock);

	return(len);
}

