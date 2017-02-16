#ifndef _AR6000_CS_INTERNAL_H_
#define _AR6000_CS_INTERNAL_H_
/*
 *
 * Copyright (c) 2004-2007 Atheros Communications Inc.
 * All rights reserved.
 *
 * 
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//
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
