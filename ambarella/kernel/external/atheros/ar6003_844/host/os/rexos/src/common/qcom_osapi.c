/*
 * Copyright (c) 2004-2005 Atheros Communications Inc.
 * All rights reserved.
 *
 * $ATH_LICENSE_HOSTSDK0_C$
 *
 */

#ifdef ATHR_EMULATION
/* Linux/OS include files */
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#endif

/* Atheros include files */
#include <athdefs.h>
#include <a_types.h>
#include <a_osapi.h>
#include <wmi.h>

/* Atheros platform include files */
#include "a_debug.h"

#ifdef ATHR_EMULATION
/*****************************************************************************/
/** General OS abstractions                                                 **/
/*****************************************************************************/

void *qcom_malloc(unsigned int size)
{
	return(malloc(size));
}

void qcom_free(void *addr)
{
	free(addr);
}

int qcom_print(const char *format, ...)
{
	va_list ap;
	int rc;

	va_start(ap, format);

	/* Out with it ... */
	rc = vfprintf(stderr, format, ap);

	va_end(ap);

	return(rc);
}

int qcom_lock_init(A_MUTEX_T *mutex)
{
	return(pthread_mutex_init(mutex, NULL) ? -1 : 0);
}

int qcom_lock(A_MUTEX_T *mutex)
{
	DPRINTF(DBG_OSAPI,
		(DBGFMT "Enter 0x%08x\n", DBGARG, (unsigned int) mutex));
	return(pthread_mutex_lock(mutex) ? -1 : 0);
}

int qcom_unlock(A_MUTEX_T *mutex)
{
	DPRINTF(DBG_OSAPI,
		(DBGFMT "Enter 0x%08x\n", DBGARG, (unsigned int) mutex));
	return(pthread_mutex_unlock(mutex) ? -1 : 0);
}

void qcom_cond_wait_init(A_WAITQUEUE_HEAD *wait_channel)
{
	if(pthread_mutex_init(&wait_channel->mutex, NULL) != 0) 
	{
		return;
	}

	if(pthread_cond_init(&wait_channel->cond, NULL) != 0) 
	{
		return;
	}

	return;
}

/* Timeout in ms */
void qcom_cond_wait(A_WAITQUEUE_HEAD *wait_channel, int condition, int timeout)
{
	struct timeval now;         /* Time when we started waiting        */
	struct timespec expire;     /* Timeout value for the wait function */

	DPRINTF(DBG_OSAPI, (DBGFMT "Enter\n", DBGARG));
	if(!condition)
	{
		/* Get current time */
		gettimeofday(&now, NULL);

		/* Prepare timeout value - we need an absolute time. */
		expire.tv_sec = now.tv_sec + timeout / 1000;
		expire.tv_nsec = now.tv_usec * 1000;

		DPRINTF(DBG_OSAPI, (DBGFMT "wait %d\n", DBGARG, timeout));
		(void) pthread_cond_timedwait(&wait_channel->cond,
			&wait_channel->mutex, &expire);
		pthread_mutex_unlock(&wait_channel->mutex);
		DPRINTF(DBG_OSAPI, (DBGFMT "timeout\n", DBGARG));
	}

	return;
}

void qcom_cond_signal(A_WAITQUEUE_HEAD *wait_channel)
{
	(void) pthread_cond_signal(&wait_channel->cond);

	return;
}

unsigned int qcom_get_ms(unsigned int offset)
{
    unsigned int val;
    struct timeval tv = { 0, 0 };

    gettimeofday(&tv, NULL);

    val = tv.tv_sec * 1000 + tv.tv_usec / 1000;

    return(val + offset);
}

void qcom_delay(unsigned int msecs)
{
	struct timespec req;
	int nanosleep(const struct timespec *req, struct timespec *rem);

	DPRINTF(DBG_OSAPI, (DBGFMT "Enter\n", DBGARG));

	req.tv_sec = msecs / 1000;
	req.tv_nsec = (msecs - (req.tv_sec * 1000)) * 1000000;

	nanosleep(&req, NULL);

	DPRINTF(DBG_OSAPI, (DBGFMT "Exit\n", DBGARG));

	return;
}

#else
/*****************************************************************************/
/*****************************************************************************/
/**                                                                         **/
/** Qualcomm will define their versions of above functions here.            **/
/** Another alternative is to change the macros in a_osapi.h                **/
/** A hybrid between the two is also possible. The sky is the limit!        **/
/**                                                                         **/
/*****************************************************************************/
/*****************************************************************************/
#endif
