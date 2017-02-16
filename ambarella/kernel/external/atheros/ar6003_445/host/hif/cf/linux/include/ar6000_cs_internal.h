#ifndef _AR6000_CS_INTERNAL_H_
#define _AR6000_CS_INTERNAL_H_
/*
 *
 * Copyright (c) 2004-2007 Atheros Communications Inc.
 * All rights reserved.
 *
 * 
// The software source and binaries included in this development package are
// licensed, not sold. You, or your company, received the package under one
// or more license agreements. The rights granted to you are specifically
// listed in these license agreement(s). All other rights remain with Atheros
// Communications, Inc., its subsidiaries, or the respective owner including
// those listed on the included copyright notices.  Distribution of any
// portion of this package must be in strict compliance with the license
// agreement(s) terms.
// </copyright>
// 
// <summary>
// 	Wifi driver for AR6002
// </summary>
//
 *
 */

#include "ar6000_cs.h"

/* Device structure maintained for each instance of the device
* in the driver.
*/
typedef struct _CFDEVICE_ {
	
	PCFFUNCTION pFunction; //Upper layer driver supporting this device.
	CF_PNP_INFO pId; // PNP Info for this device.
	void (*pIrqFunction)(void * pContext, A_BOOL *callDSR);/* interrupt routine, synchronous calls allowed */
    void * IrqContext;         /* irq context */  
	struct tasklet_struct tasklet;

	A_UINT32 mem_start;
	A_UINT32 mem_end;
	A_UINT8 irq;

	void * backPtr; /* backptr to dev_link_t struct */	

} CF_DEVICE;

struct ar6000_pccard {
	dev_link_t link;
	dev_node_t node;

	CF_DEVICE CfDevice;//ref. to the CFDevice structure for this device.
#ifdef POLLED_MODE
	struct timer_list poll_timer;
#endif
};

#ifdef KERNEL_2_4
typedef void irqreturn_t;
#define IRQ_NONE
#define IRQ_HANDLED
#endif

static void insert_dev_list(struct ar6000_pccard *);
static struct dev_link_t * remove_dev_list(struct ar6000_pccard *);
static void insert_drv_list(PCFFUNCTION);
static PCFFUNCTION remove_drv_list(PCFFUNCTION); 

#endif //_AR6000_CS_INTERNAL_H_
