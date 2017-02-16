/*
 * Copyright (c) 2004-2005 Atheros Communications Inc.
 * All rights reserved.
 *
 * $ATH_LICENSE_HOSTSDK0_C$
 *
 */

/* Linux/OS include files */
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>

/* Atheros include files */
#include <athdefs.h>
#include <a_types.h>
#include <a_osapi.h>
#include <htc.h>
#include <htc_api.h>
#include <hif.h>
#include <hif_internal.h>

/* Qualcomm include files */
#include "qcom_stubs.h"
#include "wlan_dot11_sys.h"
#include "wlan_adp_def.h"
#include "wlan_adp_oem.h"
#include "wlan_trp.h"
#include "sdcc_api.h"

/* Atheros platform include files */
#include "qcom_htc.h"
#include "usdio.h"
#include "a_debug.h"

#define DEVICE	"/dev/usdio0"

#if 0
#define HIF_MBOX_BASE_ADDR              0x800
#define HIF_MBOX_BLOCK_SIZE            	128 
#define HIF_MBOX_WIDTH                  0x800
#endif

#define USE_SEMAPHORE

void sdcc_sdio_complete(uint16 dev_manfid);
static void *s_handle = NULL;	/* MATS - Not used */
static int s_fd = 0;
static void (*s_fn)(int fd) = NULL;

#ifndef USE_SEMAPHORE
static pthread_cond_t s_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif
static sem_t s_sem;
static int s_thread_id;	
static pthread_t s_p_thread;

/*****************************************************************************/
/** Internal functions                                                      **/
/*****************************************************************************/

static int sdio_signal(void)
{
	int val;
	sem_getvalue(&s_sem, &val);
	DPRINTF(DBG_SDIO, (DBGFMT "Enter %d\n", DBGARG, val));

#ifdef USE_SEMAPHORE
	if(sem_post(&s_sem) < 0)
	{
		return(-1);
	}
#else
	(void) pthread_cond_signal(&s_cond);
#endif

	sem_getvalue(&s_sem, &val);
	DPRINTF(DBG_SDIO, (DBGFMT "Exit %d\n", DBGARG, val));

	return(0);
}

static int sdio_wait(void)
{
	int val;
	sem_getvalue(&s_sem, &val);
	DPRINTF(DBG_SDIO, (DBGFMT "Enter %d\n", DBGARG, val));

#ifdef USE_SEMAPHORE
	if(sem_wait(&s_sem) < 0)
	{
		return(-1);
	}
#else
	if(pthread_mutex_lock(&s_mutex) != 0)
	{
		return(-1);
	}

	if(pthread_cond_wait(&s_cond, &s_mutex) != 0)
	{
		return(-1);
	}

	pthread_mutex_unlock(&s_mutex);
#endif

	sem_getvalue(&s_sem, &val);
	DPRINTF(DBG_SDIO, (DBGFMT "Exit %d\n", DBGARG, val));

	return(0);
}

static void handle_event(unsigned int val)
{
	DPRINTF(DBG_SDIO, (DBGFMT "Enter\n", DBGARG));

	if(val & USDIO_EVENT_INSERTED)
	{
		DPRINTF(DBG_SDIO, (DBGFMT "USDIO_EVENT_INSERTED\n", DBGARG));
	}

	if(val & USDIO_EVENT_REMOVED)
	{
		DPRINTF(DBG_SDIO, (DBGFMT "USDIO_EVENT_REMOVED\n", DBGARG));
	}

	if(val & USDIO_EVENT_WOK)
	{
		DPRINTF(DBG_SDIO, (DBGFMT "USDIO_EVENT_WOK\n", DBGARG));
		sdio_signal();
	}

	if(val & USDIO_EVENT_WERROR)
	{
		DPRINTF(DBG_SDIO, (DBGFMT "USDIO_EVENT_WERROR\n", DBGARG));
		sdio_signal();
	}

	if(val & USDIO_EVENT_ROK)
	{
		DPRINTF(DBG_SDIO, (DBGFMT "USDIO_EVENT_ROK\n", DBGARG));
		sdcc_sdio_complete(0);
		sdio_signal();
	}

	if(val & USDIO_EVENT_RERROR)
	{
		DPRINTF(DBG_SDIO, (DBGFMT "USDIO_EVENT_RERROR\n", DBGARG));
		sdio_signal();
	}

	if(val & USDIO_EVENT_IRQ)
	{
		DPRINTF(DBG_SDIO, (DBGFMT "USDIO_EVENT_IRQ\n", DBGARG));
		htc_task_signal(HTC_HIFDSR_SIG);
	}

	DPRINTF(DBG_SDIO, (DBGFMT "Exit\n", DBGARG));

	return;
}

static char s_table[16] =
{
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'a', 'b', 'c', 'd', 'e', 'f'
};

static void data(char *d, int l)
{
	int i;
	char buff[128];
	char *s = buff;
	int n = 0;

	DPRINTF(DBG_WARNING,
		(DBGFMT "******************************\n", DBGARG));

	for(i = 0; i < l; i++)
	{
		if(i > 0 && (i % 16) == 0)
		{
			DPRINTF(DBG_WARNING, (DBGFMT "%4d - %s\n", DBGARG, n, buff));
			s = buff;
			n = i;
		}

		*s++ = s_table[(*d >> 4) & 0xf];
		*s++ = s_table[*d & 0xf];
		*s++ = ' ';
		*s = '\0';
		d++;
	}

	if(s != buff)
	{
		DPRINTF(DBG_WARNING, (DBGFMT "%4d - %s\n", DBGARG, n, buff));
	}

	DPRINTF(DBG_WARNING,
		(DBGFMT "******************************\n", DBGARG));
}

void dump_data(char *d, int l)
{
	data(d, l);

	return;
}

static void do_wait(int val)
{
	sdio_wait();
}

void *sdio_task(void* data)
{
	int nfds;
	fd_set read_set;
	unsigned int val = 0;
	int n;
	int fd = (int) data;

	DPRINTF(DBG_SDIO, (DBGFMT "Starting\n", DBGARG));

	for(;;)
	{
		FD_ZERO(&read_set);
		FD_SET(fd, &read_set);

		nfds = select(fd + 1, &read_set, NULL, NULL, NULL);

		if(nfds < 0)
		{
			if(errno == EINTR)
			{
				DPRINTF(DBG_SDIO, (DBGFMT "EINTR\n", DBGARG));
				continue;
			}
			else
			{
				DPRINTF(DBG_SDIO, (DBGFMT "select() failed\n", DBGARG));
				exit(-1);
			}
		}

		if(FD_ISSET(fd, &read_set))
		{
			n = read(fd, (char *) &val, sizeof(unsigned int));

			DPRINTF(DBG_SDIO, (DBGFMT "=*=*=*=> Events - Read %d bytes = 0x%08x.\n", DBGARG, n, val));

			if(n < 0)
			{
				DPRINTF(DBG_SDIO, (DBGFMT "read() failed\n", DBGARG));
				exit(-1);
			}

			/* Process the events */
			handle_event(val);
		}
	}

	DPRINTF(DBG_SDIO, (DBGFMT "Ending\n", DBGARG));

    /* Terminate the thread */
    pthread_exit(NULL);
}

int sdio_task_init(int fd)
{
	DPRINTF(DBG_SDIO, (DBGFMT "Enter\n", DBGARG));

#ifdef USE_SEMAPHORE
	if(sem_init(&s_sem, 0, 0) < 0)
	{
		DPRINTF(DBG_SDIO, (DBGFMT "%s() Failed to initialize semaphore.\n", DBGARG, __func__));
		return(-1);
	}
#endif

    /* Create a the IRQ task */
    s_thread_id = pthread_create(&s_p_thread, NULL, sdio_task, (void *) fd);

	DPRINTF(DBG_SDIO, (DBGFMT "Exit\n", DBGARG));

	/* We are good */
	return(0);
}

/*****************************************************************************/
/** API functions                                                           **/
/*****************************************************************************/

void sdcc_sdio_connect_intr(uint8 devfn, void *isr, void *isr_param)
{
	struct usdio_req r;

	DPRINTF(DBG_SDIO, (DBGFMT "Enter\n", DBGARG));

	r.handle = s_handle;
	r.cmd = USDIO_UMASK_IRQ;

	if(ioctl(s_fd, USDIO_IOCTL_CMD, &r) < 0)
	{
		perror("ioctl()");
	}

	DPRINTF(DBG_SDIO, (DBGFMT "Exit\n", DBGARG));

	return;
}

void sdcc_sdio_disconnect_intr(uint8 devfn)
{
	struct usdio_req r;

	DPRINTF(DBG_SDIO, (DBGFMT "Enter\n", DBGARG));

	r.handle = s_handle;
	r.cmd = USDIO_MASK_IRQ;

	if(ioctl(s_fd, USDIO_IOCTL_CMD, &r) < 0)
	{
		perror("ioctl()");
	}

	DPRINTF(DBG_SDIO, (DBGFMT "Exit\n", DBGARG));

	return;
}

void sdcc_sdio_ack_intr(uint8 devfn)
{
	struct usdio_req r;

	DPRINTF(DBG_SDIO, (DBGFMT "Enter\n", DBGARG));

	r.handle = s_handle;
	r.cmd = USDIO_ACK_IRQ;

	if(ioctl(s_fd, USDIO_IOCTL_CMD, &r) < 0)
	{
		perror("ioctl()");
	}

	DPRINTF(DBG_SDIO, (DBGFMT "Exit\n", DBGARG));

	return;
}

SDCC_STATUS sdcc_sdio_init(uint16 dev_manfid)
{
	struct usdio_req r;
	int fd;

	DPRINTF(DBG_SDIO, (DBGFMT "Enter\n", DBGARG));

	if((fd = open(DEVICE, O_RDWR)) < 0)
	{
		perror("open()");
		return(-1);
	}

	sdio_task_init(fd);

	s_fd = fd;
	s_fn = do_wait;

	r.handle = s_handle;
	r.cmd = USDIO_INIT;
	r.u.init.blocksize = HIF_MBOX_BLOCK_SIZE;

	if(ioctl(s_fd, USDIO_IOCTL_CMD, &r) < 0)
	{
		perror("ioctl()");
	}

	DPRINTF(DBG_SDIO, (DBGFMT "Exit\n", DBGARG));

	return(SDCC_NO_ERROR);
}

boolean sdcc_close(int16 driveno)
{
	struct usdio_req r;

	DPRINTF(DBG_SDIO, (DBGFMT "Enter\n", DBGARG));

	r.handle = s_handle;
	r.cmd = USDIO_UNINIT;

	if(ioctl(s_fd, USDIO_IOCTL_CMD, &r) < 0)
	{
		perror("ioctl()");
	}

	DPRINTF(DBG_SDIO, (DBGFMT "Exit\n", DBGARG));

	return(TRUE);
}

SDCC_STATUS sdcc_sdio_write(uint8 devfn, uint32 reg_addr, uint16 count,
	uint8 *buff)
{
	struct usdio_req r;

	DPRINTF(DBG_SDIO, (DBGFMT "Enter\n", DBGARG));

	r.handle = s_handle;
	r.cmd = USDIO_WRITE;
	r.u.rw.address = reg_addr;
	r.u.rw.context = (void *) 0x11111111;
	r.u.rw.blocksize = HIF_MBOX_BLOCK_SIZE;
	r.u.rw.mboxwidth = HIF_MBOX_WIDTH;
	r.u.rw.mboxbaseaddress = HIF_MBOX_BASE_ADDR;
	r.u.rw.flags = RW_ASYNCHRONOUS | RW_EXTENDED_IO | RW_BLOCK_BASIS | RW_WRITE |
		RW_INCREMENTAL_ADDRESS | RW_RAW_ADDRESSING;
	r.u.rw.buffer = buff;
	r.u.rw.length = count * HIF_MBOX_BLOCK_SIZE;

	if(ioctl(s_fd, USDIO_IOCTL_CMD, &r) < 0)
	{
		perror("ioctl()");
	}

	if(s_fn != NULL)
	{
		(*s_fn)(s_fd);
	}

	DPRINTF(DBG_SDIO, (DBGFMT "Exit\n", DBGARG));

	return(SDCC_NO_ERROR);
}

SDCC_STATUS sdcc_sdio_bwrite(uint8 devfn, uint32 reg_addr, uint16 count,
	uint8 *buff, unsigned int flags)
{
	struct usdio_req r;

	DPRINTF(DBG_SDIO, (DBGFMT "Enter - count %d\n", DBGARG, count));

	r.handle = s_handle;
	r.cmd = USDIO_WRITE;
	r.u.rw.address = reg_addr;
	r.u.rw.context = (void *) 0x11111111;
	r.u.rw.blocksize = HIF_MBOX_BLOCK_SIZE;
	r.u.rw.mboxwidth = HIF_MBOX_WIDTH;
	r.u.rw.mboxbaseaddress = HIF_MBOX_BASE_ADDR;
	r.u.rw.flags = flags | RW_EXTENDED_IO | RW_WRITE | RW_RAW_ADDRESSING;
	r.u.rw.flags &= ~RW_SYNCHRONOUS;
	r.u.rw.flags |= RW_ASYNCHRONOUS;
	r.u.rw.buffer = buff;
	if(flags & RW_BLOCK_BASIS)
	{
		r.u.rw.length = count * HIF_MBOX_BLOCK_SIZE;
	}
	else
	{
		r.u.rw.length = count;
	}

	data((char *) buff, r.u.rw.length);

	if(ioctl(s_fd, USDIO_IOCTL_CMD, &r) < 0)
	{
		perror("ioctl()");
	}

	if(s_fn != NULL)
	{
		(*s_fn)(s_fd);
	}

	DPRINTF(DBG_SDIO, (DBGFMT "Exit\n", DBGARG));

	return(SDCC_NO_ERROR);
}

SDCC_STATUS sdcc_sdio_bread(uint8 devfn, uint32 reg_addr, uint16 count,
	uint8 *buff, unsigned int flags)
{
	struct usdio_req r;

	DPRINTF(DBG_SDIO, (DBGFMT "Enter - count %d\n", DBGARG, count));

	r.handle = s_handle;
	r.cmd = USDIO_READ;
	r.u.rw.address = reg_addr;
	r.u.rw.context = (void *) 0x22222222;
	r.u.rw.blocksize = HIF_MBOX_BLOCK_SIZE;
	r.u.rw.mboxwidth = HIF_MBOX_WIDTH;
	r.u.rw.mboxbaseaddress = HIF_MBOX_BASE_ADDR;
	r.u.rw.flags = flags | RW_EXTENDED_IO | RW_READ | RW_RAW_ADDRESSING;
	r.u.rw.flags &= ~RW_SYNCHRONOUS;
	r.u.rw.flags |= RW_ASYNCHRONOUS;
	if(flags & RW_BLOCK_BASIS)
	{
		r.u.rw.length = count * HIF_MBOX_BLOCK_SIZE;
	}
	else
	{
		r.u.rw.length = count;
	}
	r.u.rw.buffer = buff;

	if(ioctl(s_fd, USDIO_IOCTL_CMD, &r) < 0)
	{
		perror("ioctl()");
	}

	if(s_fn != NULL)
	{
		(*s_fn)(s_fd);
	}

	data((char *) buff, r.u.rw.length);

	DPRINTF(DBG_SDIO, (DBGFMT "Exit\n", DBGARG));

	return(SDCC_NO_ERROR);
}

SDCC_STATUS sdcc_sdio_read(uint8 devfn, uint32 reg_addr, uint16 count,
	uint8 *buff)
{
	struct usdio_req r;

	DPRINTF(DBG_SDIO, (DBGFMT "Enter\n", DBGARG));

	r.handle = s_handle;
	r.cmd = USDIO_READ;
	r.u.rw.address = reg_addr;
	r.u.rw.context = (void *) 0x22222222;
	r.u.rw.blocksize = HIF_MBOX_BLOCK_SIZE;
	r.u.rw.mboxwidth = HIF_MBOX_WIDTH;
	r.u.rw.mboxbaseaddress = HIF_MBOX_BASE_ADDR;
	r.u.rw.flags = RW_ASYNCHRONOUS | RW_EXTENDED_IO | RW_BLOCK_BASIS | RW_READ |
		RW_FIXED_ADDRESS | RW_RAW_ADDRESSING;
	r.u.rw.length = count * HIF_MBOX_BLOCK_SIZE;
	r.u.rw.buffer = buff;

	if(ioctl(s_fd, USDIO_IOCTL_CMD, &r) < 0)
	{
		perror("ioctl()");
	}

	if(s_fn != NULL)
	{
		(*s_fn)(s_fd);
	}

	DPRINTF(DBG_SDIO, (DBGFMT "Exit\n", DBGARG));

	return(SDCC_NO_ERROR);
}

SDCC_STATUS  sdcc_sdio_cmd52(uint32  devfn, uint32  dir, uint32  reg_addr,
	uint8  *pdata)
{
	struct usdio_req r;

	if(dir == 0)
	{
		DPRINTF(DBG_SDIO, (DBGFMT "Enter - Read\n", DBGARG));

		r.handle = s_handle;
		r.cmd = USDIO_READ;
		r.u.rw.address = reg_addr;
		r.u.rw.context = (void *) 0x33333333;
		r.u.rw.blocksize = HIF_MBOX_BLOCK_SIZE;
		r.u.rw.mboxwidth = HIF_MBOX_WIDTH;
		r.u.rw.mboxbaseaddress = HIF_MBOX_BASE_ADDR;
		r.u.rw.flags = RW_ASYNCHRONOUS | RW_EXTENDED_IO | RW_BYTE_BASIS |
			RW_READ | RW_INCREMENTAL_ADDRESS | RW_RAW_ADDRESSING;
		r.u.rw.length = 1;
		r.u.rw.buffer = pdata;
	}
	else
	{
		DPRINTF(DBG_SDIO, (DBGFMT "Enter - Write\n", DBGARG));

		r.handle = s_handle;
		r.cmd = USDIO_WRITE;
		r.u.rw.address = reg_addr;
		r.u.rw.context = (void *) 0x44444444;
		r.u.rw.blocksize = HIF_MBOX_BLOCK_SIZE;
		r.u.rw.mboxwidth = HIF_MBOX_WIDTH;
		r.u.rw.mboxbaseaddress = HIF_MBOX_BASE_ADDR;
		r.u.rw.flags = RW_ASYNCHRONOUS | RW_EXTENDED_IO | RW_BYTE_BASIS |
			RW_WRITE | RW_INCREMENTAL_ADDRESS | RW_RAW_ADDRESSING;
		r.u.rw.length = 1;
		r.u.rw.buffer = pdata;
	}

	if(ioctl(s_fd, USDIO_IOCTL_CMD, &r) < 0)
	{
		perror("ioctl()");
	}

	if(s_fn != NULL)
	{
		(*s_fn)(s_fd);
	}

	DPRINTF(DBG_SDIO, (DBGFMT "Exit\n", DBGARG));

	return(SDCC_NO_ERROR);
}

SDCC_STATUS sdcc_sdio_set(uint8 devfn,
	SDIO_SET_FEATURE_TYPE type, void *pdata)
{
	DPRINTF(DBG_SDIO, (DBGFMT "Enter\n", DBGARG));
	DPRINTF(DBG_SDIO, (DBGFMT "Exit\n", DBGARG));

	return(SDCC_NO_ERROR);
}

void sdcc_sdio_complete(uint16 dev_manfid)
{
	struct usdio_req r;

	DPRINTF(DBG_SDIO, (DBGFMT "Enter\n", DBGARG));

	r.handle = s_handle;
	r.cmd = USDIO_COMPLETE;

	if(ioctl(s_fd, USDIO_IOCTL_CMD, &r) < 0)
	{
		perror("ioctl()");
	}

	DPRINTF(DBG_SDIO, (DBGFMT "Exit\n", DBGARG));

	return;
}
