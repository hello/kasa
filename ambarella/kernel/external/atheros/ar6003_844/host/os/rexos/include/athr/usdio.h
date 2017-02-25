/*
 * Copyright (c) 2004-2005 Atheros Communications Inc.
 * All rights reserved.
 *
 * $ATH_LICENSE_HOSTSDK0_C$
 * 
 */

#ifndef _USDIO_H
#define _USDIO_H

/* Linux/OS include files */
#include <linux/ioctl.h>

#define USDIO_MAJOR_NUMBER		65
#define THIS_DEV_NAME			"usdio"

#define USDIO_EVENT_INSERTED    0x00000001
#define USDIO_EVENT_REMOVED     0x00000002
#define USDIO_EVENT_ROK         0x00000004
#define USDIO_EVENT_RERROR      0x00000008
#define USDIO_EVENT_WOK         0x00000010
#define USDIO_EVENT_WERROR      0x00000020
#define USDIO_EVENT_IRQ         0x00000040

struct usdio_req
{
	void *handle;	/* This is the SDIO handle */
	int cmd;
#define	USDIO_UMASK_IRQ			1
#define	USDIO_MASK_IRQ			2
#define	USDIO_ACK_IRQ			3
#define	USDIO_INIT				4
#define	USDIO_UNINIT			5
#define	USDIO_READ				6
#define	USDIO_WRITE				7
#define	USDIO_COMPLETE			8
#define	USDIO_DBG				9
	union
	{
		struct
		{
			unsigned int address;
			unsigned char *buffer;
			int length;
#ifdef MATS
			HIF_REQUEST *request;
#endif
			void *context;
			unsigned int flags;
#define RW_SYNCHRONOUS				0x00000001
#define RW_ASYNCHRONOUS				0x00000002
#define RW_EXTENDED_IO				0x00000004
#define RW_BLOCK_BASIS				0x00000008
#define RW_BYTE_BASIS				0x00000010
#define RW_READ						0x00000020
#define RW_WRITE					0x00000040
#define RW_FIXED_ADDRESS			0x00000080
#define RW_INCREMENTAL_ADDRESS		0x00000100
#define RW_RAW_ADDRESSING			0x00000200
			int blocksize;
			int mboxbaseaddress;
			int mboxwidth;
		} rw;
		struct
		{
			int blocksize;
		} init;
		struct
		{
			unsigned int flags;
		} dbg;
	} u;
};

#define USDIO_IOC_MAGIC		'k'

#define USDIO_IOCTL_CMD		_IOWR(USDIO_IOC_MAGIC, 0, struct usdio_req)

#endif
