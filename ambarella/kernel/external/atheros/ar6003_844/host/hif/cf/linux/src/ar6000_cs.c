/*
 * Copyright (c) 2004-2006 Atheros Communications Inc.
 * Portions based on orinoco_cs.c with portions copyright:
 * (C) Copyright David Gibson, IBM Corporation 2001-2003.
 * Copyright (C) 2000 David Gibson, Linuxcare Australia. With some help 
 * from :
 * Copyright (C) 2001 Jean Tourrilhes, HP Labs Copyright (C) 2001 
 * Benjamin Herrenschmidt Portions based on wvlan_cs.c 1.0.6, Copyright 
 * Andreas Neuhaus
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License
 * at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and
 * limitations under the License.
 *
 * The initial developer of the original code is David A. Hinds
 * <dahinds AT users.sourceforge.net>. Portions created by David
 * A. Hinds are Copyright (C) 1999 David A. Hinds. All Rights
 * Reserved.
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License version 2 (the "GPL"), in
 * which case the provisions of the GPL are applicable instead of the
 * above. If you wish to allow the use of your version of this file
 * only under the terms of the GPL and not to allow others to use your
 * version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice and
 * other provisions required by the GPL. If you do not delete the
 * provisions above, a recipient may use your version of this file
 * under either the MPL or the GPL.
 */

#include <linux/config.h>
#ifdef  __IN_PCMCIA_PACKAGE__
#include <pcmcia/k_compat.h>
#endif /* __IN_PCMCIA_PACKAGE__ */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/wireless.h>
#include <linux/interrupt.h>

#include <pcmcia/version.h>
#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/cisreg.h>
#include <pcmcia/ds.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/system.h>

#include "ar6000_cs_internal.h"

/********************************************************************/
/* Module stuff							    */
/********************************************************************/

MODULE_DESCRIPTION("Driver for Atheros PCMCIA WLAN Card");

#define CS_CHECK(fn, ret) \
do { last_fn = (fn); if ((last_ret = (ret)) != 0) goto cs_failed; } while (0)

/*
 * The dev_info variable is the "key" that is used to match up this
 * device driver with appropriate cards, through the card
 * configuration database.
 */
static dev_info_t dev_info = "ar6000_cs";

/*
 * A linked list of "instances" of the device.  Each actual PCMCIA
 * card corresponds to one device instance, and is described by one
 * dev_link_t structure (defined in ds.h).
 */
static dev_link_t *dev_list;

/* List of upper driver instances */
static CFFUNCTION *drv_list;

static struct rw_semaphore dev_lock;
static struct rw_semaphore drv_lock;

static struct work_struct hotPlugTask;

#ifdef DEBUG
A_UINT32 debugbusdrv=0;
enum {
    ATH_LOG_SEND = 0x0001,
    ATH_LOG_RECV = 0x0002,
    ATH_LOG_SYNC = 0x0004,
    ATH_LOG_DUMP = 0x0008,
    ATH_LOG_INF  = 0x0010,
    ATH_LOG_TRC  = 0x0020,
    ATH_LOG_WARN = 0x0040,
    ATH_LOG_ERR  = 0x0080,
    ATH_LOG_ANY  = 0xFFFF,
};

#define BUSDRV_DEBUG_PRINTF(flag, args...) do {        \
    if (debugbusdrv) {                    \
        printk(KERN_ALERT args);      \
    }                                            \
} while (0)
#endif //DEBUG

/********************************************************************/
/* Function prototypes						    */
/********************************************************************/

static void ar6000_cs_config(dev_link_t * link);
static void ar6000_cs_release(dev_link_t * link);
static int ar6000_cs_event(event_t event, int priority,
			    event_callback_args_t * args);

static dev_link_t *ar6000_cs_attach(void);
static void ar6000_cs_detach(dev_link_t *);

/********************************************************************
* API exposed to Upper layer.
********************************************************************/
/* Go thru the list of devices maintained in this layer and if it
* matches the upper layer module, call the module's probe function.
* Also pass a device's instance as an opaque reference to the upper
* layer module.
*/
CF_STATUS CF_RegisterFunction(PCFFUNCTION pFunction) {
	struct dev_link_t *ptr = NULL;
	CF_DEVICE *pCfDevice = NULL;
	PCF_PNP_INFO pnpPtr = NULL;
	CF_STATUS status = CF_STATUS_SUCCESS;

	//Insert the upper layer driver ctx in the list maintained in this layer.
	insert_drv_list(pFunction);
	
	// Acquire Lock
	down_read(&dev_lock);

	//traverse the dev_link_t list
	for ( ptr=dev_list; (ptr); ptr=ptr->next ) {

	/* check if ptr->priv->pCfDevice.pId matches that provided by the 
	 * upper layer If so insert the pFunction ctx into that devices 
	 * function list. and call that devices probe function & return.
	 */
		pCfDevice = (CF_DEVICE *)&(((struct ar6000_pccard *)(ptr->priv))->CfDevice);
		// Go thru the list of PNP Ids & if any of them match, call probe().
		/* Check for NULL Manf-Code to traverse the Pnp List that is 
		 * null terminated.
		 */

		for ( pnpPtr = pFunction->pIds; pnpPtr->CF_ManufacturerCode != 0 ; \
					pnpPtr++ ) {

			BUSDRV_DEBUG_PRINTF(ATH_LOG_INF, "CFRegisterFunc b4 manfid comp %04x, %04x\n", pCfDevice->pId.CF_ManufacturerID, pnpPtr->CF_ManufacturerID);

			if( pCfDevice->pId.CF_ManufacturerID == pnpPtr->CF_ManufacturerID )
			{
				pCfDevice->pFunction = pFunction;
				if ( !((*pFunction->pProbe)(pFunction, pCfDevice)) ) {
					/* The device is not successfuly probed by this
					* fuction driver. disassociate them.
					*/
					pCfDevice->pFunction = NULL;
					status = CF_STATUS_ERROR;
				}
				status = CF_STATUS_SUCCESS;
				break;
			}
		}
	}
	up_read(&dev_lock);
	return status;
}

CF_STATUS CF_UnregisterFunction(PCFFUNCTION pFunction) {

	/* Release the device structure if a card is present or else
	* remove the function instance from the FuncList
	*/
	struct dev_link_t *ptr = NULL;
	CF_DEVICE *pCfDevice = NULL;
	PCF_PNP_INFO pnpPtr = NULL;

	down_read(&dev_lock);

	//traverse the dev_link_t list
	for ( ptr=dev_list; (ptr); ptr=ptr->next ) {

	/* check if ptr->priv->pCfDevice.pId matches that provided by the 
	 * upper layer If so remove the association between the device and the
     * upper layer driver.
 	 */
		pCfDevice = (CF_DEVICE *)&(((struct ar6000_pccard *)(ptr->priv))->CfDevice);
		for ( pnpPtr = pFunction->pIds; pnpPtr->CF_ManufacturerCode != 0 ; \
					pnpPtr++ ) {

			BUSDRV_DEBUG_PRINTF(ATH_LOG_INF, "CF_UnRegisterFunc manfid comp %04x, %04x\n", pCfDevice->pId.CF_ManufacturerID, pFunction->pIds->CF_ManufacturerID);

			if( pCfDevice->pId.CF_ManufacturerID == pnpPtr->CF_ManufacturerID) 
			{
				pCfDevice->pFunction = NULL;
				break;
			}
		}
	}
	up_read(&dev_lock);
	remove_drv_list(pFunction);
	return CF_STATUS_SUCCESS;
}

static CF_STATUS ar6000_cs_read_byte(CF_DEVICE *pDev, PCFREQUEST pReq) {

	A_UINT32 len = pReq->length;
	A_UCHAR *buff = pReq->pDataBuffer;
	A_UINT32 ctr=0;
	A_UCHAR *port = ((A_UCHAR *)pDev->mem_start) + pReq->address;

	for (ctr=0;ctr<len;ctr++) {
		*buff = readb(port);
		BUSDRV_DEBUG_PRINTF(ATH_DEBUG_DUMP, "R: data: %x, address: %p\n",*buff,port);
		buff++;
		if (!(pReq->Flags & CFREQ_FLAGS_FIXED_ADDRESS))
			port += 1;
	}

	return CF_STATUS_SUCCESS;
}

static CF_STATUS ar6000_cs_write_byte(CF_DEVICE *pDev, PCFREQUEST pReq) {

	A_UINT32 ctr = 0;
	A_UINT32 len = pReq->length;
	A_UCHAR *buff = pReq->pDataBuffer;
	A_UCHAR *port = ((A_UCHAR *)pDev->mem_start) + pReq->address;

	for (ctr=0;ctr<len;ctr++) {
		BUSDRV_DEBUG_PRINTF(ATH_DEBUG_DUMP, "W: data: %x, address: %p\n",*buff,port);
		writeb(*buff, port);
		buff++;
		if (!(pReq->Flags & CFREQ_FLAGS_FIXED_ADDRESS))
			port += 1;
	}

	return CF_STATUS_SUCCESS;
}

static CF_STATUS ar6000_cs_read_word(CF_DEVICE *pDev, PCFREQUEST pReq) {

	A_UINT32 len = pReq->length;
	A_UINT16 *buff = pReq->pDataBuffer;
	A_UINT32 ctr=0;
	A_UCHAR *port = ((A_UCHAR *)pDev->mem_start) + pReq->address;

	for (ctr=0;(ctr+1)<len;) {
		*buff = readw(port);
		BUSDRV_DEBUG_PRINTF(ATH_DEBUG_DUMP,"R: data: %x, address: %p\n",*buff,port);
		buff++;
		if (!(pReq->Flags & CFREQ_FLAGS_FIXED_ADDRESS))
			port += 2;
		ctr += 2;
	}

	//Read the last byte
	if ( ctr < len ) {
		*((unsigned char *)buff) = readb(port);
		BUSDRV_DEBUG_PRINTF(ATH_DEBUG_DUMP,"R: data: %x, address: %p\n",*((unsigned char *)buff),port);
	}

	return CF_STATUS_SUCCESS;
}

static CF_STATUS ar6000_cs_write_word(CF_DEVICE *pDev, PCFREQUEST pReq) {

	A_UINT32 ctr = 0;
	A_UINT32 len = pReq->length;
	A_UINT16 *buff = pReq->pDataBuffer;
	A_UCHAR *port = ((A_UCHAR *)pDev->mem_start) + pReq->address;

	for (ctr=0;(ctr+1)<len;) {
		BUSDRV_DEBUG_PRINTF(ATH_DEBUG_DUMP,"W: data: %x, address: %p\n",*buff,port);
		writew(*buff, port);
		buff++;
		if (!(pReq->Flags & CFREQ_FLAGS_FIXED_ADDRESS))
			port += 2;
		ctr += 2;
	}

	// Write the last byte
	if ( ctr < len ) {
		BUSDRV_DEBUG_PRINTF(ATH_DEBUG_DUMP,"W: data: %x, address: %p\n",*((unsigned char *)buff),port);
		writeb(*((unsigned char *)buff), port);
	}

	return CF_STATUS_SUCCESS;
}


CF_STATUS CF_BusRequest_Word(PCFDEVICE pDev, PCFREQUEST pReq)
{
	CF_STATUS status;

	if ( pReq->Flags & CFREQ_FLAGS_DATA_WRITE ) {
		status = ar6000_cs_write_word((CF_DEVICE *)pDev, pReq);
	} else {
		status = ar6000_cs_read_word((CF_DEVICE *)pDev, pReq);
	}

	return status;
}

CF_STATUS CF_BusRequest_Byte(PCFDEVICE pDev, PCFREQUEST pReq)
{
	CF_STATUS status;

	if ( pReq->Flags & CFREQ_FLAGS_DATA_WRITE ) {
		status = ar6000_cs_write_byte((CF_DEVICE *)pDev, pReq);
	} else {
		status = ar6000_cs_read_byte((CF_DEVICE *)pDev, pReq);
	}

	return status;
}

void CF_SetIrqHandler(PCFDEVICE pDev, pIsrHandler pFn1, pDsrHandler pFn2, void * pContext) {
    ((CF_DEVICE *)pDev)->pIrqFunction = (pFn1);
    ((CF_DEVICE *)pDev)->IrqContext = (void *)(pContext);
	if (pFn2) {
		tasklet_init(&(((CF_DEVICE *)pDev)->tasklet),pFn2,(unsigned long)pContext);
	}
}

/********************************************************************/
/* PCMCIA stuff     						    */
/********************************************************************/

/* For 2.4 kernels, cs_error is not exported. providing our own...*/
static void
ar6000_cs_error(client_handle_t handle, int func, int ret)
{
	error_info_t err = { func, ret };
	pcmcia_report_error(handle, &err);
}

/* Device List manipulation routines */
static void insert_dev_list(struct ar6000_pccard *dev)
{
	dev_link_t *temp=NULL;

	down_write(&dev_lock);

	//Check for empty dev list.
	if(!dev_list) {
		dev_list = &dev->link;
	} else {
		//traverse to the end of the list.
		for(temp=dev_list;(temp->next);temp=temp->next);
		temp->next = &dev->link;
	}
	up_write(&dev_lock);
	return;
}

static struct dev_link_t * remove_dev_list(struct ar6000_pccard *dev)
{
	struct dev_link_t *curr,*prev;
	curr = prev = NULL;
	
	down_write(&dev_lock);

	for(curr=prev=dev_list;(curr);prev=curr,curr=curr->next) {
		if(curr->priv == dev) {
			if(curr!=prev)
				prev->next = curr->next;
			else
				dev_list = curr->next;
			break;
		}
	}
	up_write(&dev_lock);
	return curr;
}

/* Upper driver instance list manipulation routines
*/
static void insert_drv_list(PCFFUNCTION pFunction) {
	
	CFFUNCTION *temp=NULL;

	down_write(&drv_lock);

	//Check for empty dev list.
	if(!drv_list) {
		drv_list = pFunction;
	} else {
		//traverse to the end of the list.
		for(temp=drv_list;(temp->next);temp=temp->next);
		temp->next = pFunction;
	}
	pFunction->next = NULL;
	up_write(&drv_lock);
	return;
}

static PCFFUNCTION remove_drv_list(PCFFUNCTION pFunction)
{
	CFFUNCTION *curr,*prev;
	curr = prev = NULL;
	
	down_write(&drv_lock);

	for(curr=prev=drv_list;(curr);prev=curr,curr=curr->next) {
		if(curr == pFunction) {
			if(curr!=prev)
				prev->next = curr->next;
			else
				drv_list = curr->next;
			break;
		}
	}
	up_write(&drv_lock);
	return curr;
}

/*
 * Create an instance of the card and register with Card Services.
 */
static dev_link_t *
ar6000_cs_attach(void)
{
	struct ar6000_pccard *info;
	dev_link_t *link;
	client_reg_t client_reg;
	A_UINT32 ret;

	BUSDRV_DEBUG_PRINTF(ATH_DEBUG_TRC, "Enter - ar6000_cs_attach\n");

	/* Create new device */
   	info = A_MALLOC(sizeof(struct ar6000_pccard));
   	if (!info) return NULL;
   	A_MEMZERO(info, sizeof(*info));
   	link = &info->link; link->priv = info;

	/* Initialize the CF device structure */
	info->CfDevice.backPtr = link;

	/* Interrupt setup */
	link->irq.Attributes = IRQ_TYPE_EXCLUSIVE;
	link->irq.IRQInfo1 = IRQ_LEVEL_ID;
	link->irq.Handler = NULL;

	link->conf.Attributes = 0;
	link->conf.IntType = INT_MEMORY_AND_IO;

	/* Insert into the global devlist */
	insert_dev_list(info);

	/* Register with Card Services */
	client_reg.dev_info = &dev_info;
	client_reg.Attributes = INFO_IO_CLIENT | INFO_CARD_SHARE;
	client_reg.EventMask =
		CS_EVENT_CARD_INSERTION | CS_EVENT_CARD_REMOVAL |
		CS_EVENT_RESET_PHYSICAL | CS_EVENT_CARD_RESET |
		CS_EVENT_PM_SUSPEND | CS_EVENT_PM_RESUME;
	client_reg.event_handler = &ar6000_cs_event;
	client_reg.Version = 0x0210; /* FIXME: what does this mean? */
	client_reg.event_callback_args.client_data = link;

	ret = pcmcia_register_client(&link->handle, &client_reg);
	if (ret != CS_SUCCESS) {
		ar6000_cs_error(link->handle, RegisterClient, ret);
		BUSDRV_DEBUG_PRINTF(ATH_LOG_ERR, "pcmcia register failed\n");
		ar6000_cs_detach(link);
		return NULL;
	}

	BUSDRV_DEBUG_PRINTF(ATH_DEBUG_TRC, "Exit - ar6000_cs_attach\n");

	return link;
} /* ar6000_cs_attach */

/*
 * Deregister with card services & free the device structure.
 */
static void
ar6000_cs_detach(dev_link_t * link)
{
	CF_DEVICE *pCfDevice = NULL;

	BUSDRV_DEBUG_PRINTF(ATH_DEBUG_TRC, "Enter - ar6000_cs_detach\n");

	/* Call the pRemove function of the top level driver */
	pCfDevice = (CF_DEVICE *)&(((struct ar6000_pccard *)(link->priv))->CfDevice);
	if (pCfDevice->pFunction) {
		(*pCfDevice->pFunction->pRemove)(pCfDevice->pFunction, pCfDevice);
		pCfDevice->pFunction = NULL;
	}

	if (link->state & DEV_CONFIG)
		ar6000_cs_release(link);

	/* Unregister with Card Services */
	if (link->handle)
		pcmcia_deregister_client(link->handle);

	/* Unlink device structure, and free it */
	BUSDRV_DEBUG_PRINTF(ATH_LOG_INF, "ar6000_cs: detach: link=%p link->dev=%p\n", link, link->dev);

	remove_dev_list(link->priv);
	A_FREE(link->priv);

	BUSDRV_DEBUG_PRINTF(ATH_DEBUG_TRC, "Exit - ar6000_cs_detach\n");
}	/* ar6000_cs_detach */

static irqreturn_t ar6000_interrupt(A_UINT32 irq, void *dev_id, struct pt_regs *regs)
{
	struct ar6000_pccard *card = (struct ar6000_pccard *)dev_id;
	A_BOOL callDSR;

	//Call the Interrupt handler registered by the upper layer
	if (card->CfDevice.pFunction && card->CfDevice.pIrqFunction)
		(*(card->CfDevice.pIrqFunction))(card->CfDevice.IrqContext, &callDSR);
	
	/* Schedule a tasklet & return */
	if (card->CfDevice.pFunction && card->CfDevice.pIrqFunction && callDSR)
		tasklet_hi_schedule(&card->CfDevice.tasklet);

	return IRQ_HANDLED;
}

#ifdef POLLED_MODE
static void ar6000_interrupt_wrapper(unsigned long dev_id)
{
	struct ar6000_pccard *card = (struct ar6000_pccard *)dev_id;

	//Call the Interrupt handler registered by the upper layer
	if (card->CfDevice.pIrqFunction)
		(*(card->CfDevice.pIrqFunction))(card->CfDevice.IrqContext);

	card->poll_timer.expires = jiffies + HZ;
	add_timer(&card->poll_timer);
}
#endif

static A_UINT32 mem_speed = 0;

/*
 * ar6000_cs_config() is scheduled to run after a CARD_INSERTION
 * event is received, to configure the PCMCIA socket, and to make the
 * device available to the system.
 */

static void
ar6000_cs_config(dev_link_t *link)
{
	client_handle_t handle = link->handle;
	struct ar6000_pccard *card = link->priv;
	A_UINT32 last_fn, last_ret;
	A_UCHAR buf[64];
	config_info_t conf;
	cisinfo_t info;
	tuple_t tuple;
	cisparse_t parse;
  	win_req_t		req;
  	memreq_t		mem;
	A_UINT32 status;

	BUSDRV_DEBUG_PRINTF(ATH_DEBUG_TRC, "Enter ar6000_cs_config\n");

	CS_CHECK(ValidateCIS, pcmcia_validate_cis(handle, &info));

	/*
	 * This reads the card's CONFIG tuple to find its
	 * configuration registers.
	 */
	tuple.DesiredTuple = CISTPL_CONFIG;
	tuple.Attributes = 0;
	tuple.TupleData = buf;
	tuple.TupleDataMax = sizeof(buf);
	tuple.TupleOffset = 0;
	CS_CHECK(GetFirstTuple, pcmcia_get_first_tuple(handle, &tuple));
	CS_CHECK(GetTupleData, pcmcia_get_tuple_data(handle, &tuple));
	CS_CHECK(ParseTuple, pcmcia_parse_tuple(handle, &tuple, &parse));
	link->conf.ConfigBase = parse.config.base;
	link->conf.Present = parse.config.rmask[0];

	BUSDRV_DEBUG_PRINTF(ATH_LOG_INF, "link->conf.ConfigBase: %04x, \
					   link->conf.Present: %0x\n", link->conf.ConfigBase,
					   link->conf.Present);

	/*Read the card's manf-id, manf-code, func no & func class
	* and initialize the device structure
	*/
	tuple.DesiredTuple = CISTPL_MANFID;
	CS_CHECK(GetFirstTuple, pcmcia_get_first_tuple(handle, &tuple));
	CS_CHECK(GetTupleData, pcmcia_get_tuple_data(handle, &tuple));
	CS_CHECK(ParseTuple, pcmcia_parse_tuple(handle, &tuple, &parse));

	card->CfDevice.pId.CF_ManufacturerID = parse.manfid.card;
	card->CfDevice.pId.CF_ManufacturerCode = parse.manfid.manf;

	/*TODO: change this tuple.DesiredTuple = CISTPL_FUNCNO;
	CS_CHECK(GetFirstTuple, pcmcia_get_first_tuple(handle, &tuple));
	CS_CHECK(GetTupleData, pcmcia_get_tuple_data(handle, &tuple));
	CS_CHECK(ParseTuple, pcmcia_parse_tuple(handle, &tuple, &parse));

	card->CfDevice.pId.CF_FunctionNo = parse.manfid.manf;
	*/

	/* Configure card */
	link->state |= DEV_CONFIG;

	/* Look up the current Vcc */
	CS_CHECK(GetConfigurationInfo, pcmcia_get_configuration_info(handle, &conf));
	link->conf.Vcc = conf.Vcc;

	/*
	 * In this loop, we scan the CIS for configuration table
	 * entries, each of which describes a valid card
	 * configuration, including voltage, IO window, memory window,
	 * and interrupt settings.
	 *
	 * We make no assumptions about the card to be configured: we
	 * use just the information available in the CIS.  In an ideal
	 * world, this would work for any PCMCIA card, but it requires
	 * a complete and accurate CIS.  In practice, a driver usually
	 * "knows" most of these things without consulting the CIS,
	 * and most client drivers will only use the CIS to fill in
	 * implementation-defined details.
	 */
	tuple.DesiredTuple = CISTPL_CFTABLE_ENTRY;
	CS_CHECK(GetFirstTuple, pcmcia_get_first_tuple(handle, &tuple));
	while (1) {
		cistpl_cftable_entry_t *cfg = &(parse.cftable_entry);
		cistpl_cftable_entry_t dflt = { .index = 0 };

		if (pcmcia_get_tuple_data(handle, &tuple) != 0 ||
				pcmcia_parse_tuple(handle, &tuple, &parse) != 0)
			goto next_entry;

		if (cfg->flags & CISTPL_CFTABLE_DEFAULT)
			dflt = *cfg;
		if (cfg->index == 0)
			goto next_entry;
		link->conf.ConfigIndex = cfg->index;

		/* Does this card need audio output? */
		if (cfg->flags & CISTPL_CFTABLE_AUDIO) {
			link->conf.Attributes |= CONF_ENABLE_SPKR;
			link->conf.Status = CCSR_AUDIO_ENA;
		}

		/* Use power settings for Vcc and Vpp if present */
		/* Note that the CIS values need to be rescaled */
		/*
		if (cfg->vcc.present & (1 << CISTPL_POWER_VNOM)) {
			if (conf.Vcc != cfg->vcc.param[CISTPL_POWER_VNOM] / 10000) {
				if (!ignore_cis_vcc)
					goto next_entry;
			}
		} else if (dflt.vcc.present & (1 << CISTPL_POWER_VNOM)) {
			if (conf.Vcc != dflt.vcc.param[CISTPL_POWER_VNOM] / 10000) {
				if(!ignore_cis_vcc)
					goto next_entry;
			}
		}

		if (cfg->vpp1.present & (1 << CISTPL_POWER_VNOM))
			link->conf.Vpp1 = link->conf.Vpp2 =
			    cfg->vpp1.param[CISTPL_POWER_VNOM] / 10000;
		else if (dflt.vpp1.present & (1 << CISTPL_POWER_VNOM))
			link->conf.Vpp1 = link->conf.Vpp2 =
			    dflt.vpp1.param[CISTPL_POWER_VNOM] / 10000;
		*/
		
		/* Do we need to allocate an interrupt? */
		if (cfg->irq.IRQInfo1 || dflt.irq.IRQInfo1)
			link->conf.Attributes |= CONF_ENABLE_IRQ;

		/* IO window settings */
		link->io.NumPorts1 = link->io.NumPorts2 = 0;
		if ((cfg->io.nwin > 0) || (dflt.io.nwin > 0)) {
			cistpl_io_t *io =
			    (cfg->io.nwin) ? &cfg->io : &dflt.io;
			BUSDRV_DEBUG_PRINTF(ATH_LOG_INF, "io->nwin: %d, io->win[0].base: %d, io->win[0].len: %d\n", io->nwin, io->win[0].base, io->win[0].len);

			link->io.Attributes1 = IO_DATA_PATH_WIDTH_AUTO;
			if (!(io->flags & CISTPL_IO_8BIT))
				link->io.Attributes1 =
				    IO_DATA_PATH_WIDTH_16;
			if (!(io->flags & CISTPL_IO_16BIT))
				link->io.Attributes1 =
				    IO_DATA_PATH_WIDTH_8;
			link->io.IOAddrLines =
			    io->flags & CISTPL_IO_LINES_MASK;
			link->io.BasePort1 = io->win[0].base;
			link->io.NumPorts1 = io->win[0].len;
			if (io->nwin > 1) {
				link->io.Attributes2 =
				    link->io.Attributes1;
				link->io.BasePort2 = io->win[1].base;
				link->io.NumPorts2 = io->win[1].len;
			}
			/* This reserves IO space but doesn't actually enable it */
			if (pcmcia_request_io(link->handle, &link->io) != 0)
				goto next_entry;
		}

		break;
		
	next_entry:
		if (link->io.NumPorts1)
			pcmcia_release_io(link->handle, &link->io);
		last_ret = pcmcia_get_next_tuple(handle, &tuple);
		if (last_ret  == CS_NO_MORE_ITEMS) {
			BUSDRV_DEBUG_PRINTF(ATH_LOG_INF, "GetNextTuple().  No matching CIS configuration, "
			       "maybe you need the ignore_cis_vcc=1 parameter.\n");
			goto cs_failed;
		}
	}

	/*
	 * Allocate an interrupt line.  Note that this does not assign
	 * a handler to the interrupt, unless the 'Handler' member of
	 * the irq structure is initialized.
	 */

	if (link->conf.Attributes & CONF_ENABLE_IRQ) {

		link->irq.Attributes = IRQ_TYPE_EXCLUSIVE | IRQ_HANDLE_PRESENT;
		link->irq.IRQInfo1 = IRQ_LEVEL_ID;
		
  		link->irq.Handler = ar6000_interrupt; 
		link->irq.Instance = card; 

#ifdef POLLED_MODE
		init_timer(&card->poll_timer);
		card->poll_timer.function = ar6000_interrupt_wrapper;
		card->poll_timer.data = (unsigned long)card;
		card->poll_timer.expires = jiffies + HZ;
		add_timer(&card->poll_timer);
#else
		CS_CHECK(RequestIRQ, pcmcia_request_irq(link->handle, &link->irq));
		card->CfDevice.irq = link->irq.AssignedIRQ;
#endif
	}

	CS_CHECK(RequestConfiguration, pcmcia_request_configuration(link->handle, &link->conf));

	/*
     * Allocate a small memory window.  Note that the dev_link_t
     * structure provides space for one window handle -- if your
     * device needs several windows, you'll need to keep track of
     * the handles in your private data structure, link->priv.
     */
    req.Attributes = WIN_DATA_WIDTH_16|WIN_MEMORY_TYPE_CM|WIN_ENABLE;
    req.Base = req.Size = 0;
    req.AccessSpeed = mem_speed;
    status = pcmcia_request_window(&link->handle, &req, &link->win);
    if(status != CS_SUCCESS)
	{
#ifdef KERNEL_2_4
		ar6000_cs_error(link->handle, RequestWindow, status);
#else
		cs_error(link->handle, RequestWindow, status);
#endif
	  	goto cs_failed;
	}

    card->CfDevice.mem_start = (u_long)ioremap(req.Base, req.Size);
    card->CfDevice.mem_end = card->CfDevice.mem_start + req.Size;

    mem.CardOffset = 0; mem.Page = 0;
    status = pcmcia_map_mem_page(link->win, &mem);
    if(status != CS_SUCCESS)
	{
#ifdef KERNEL_2_4
		ar6000_cs_error(link->handle, MapMemPage, status);
#else
		cs_error(link->handle, MapMemPage, status);
		goto cs_failed;
#endif
	}

	card->node.major = card->node.minor = 0;

	/* At this point, the dev_node_t structure(s) needs to be
	 * initialized and arranged in a linked list at link->dev. */
	link->dev = &card->node; 
				   /* link->dev being non-NULL is also
                                    used to indicate that the
                                    net_device has been registered */
	link->state &= ~DEV_CONFIG_PENDING;

	/* Finally, report what we've done */
	BUSDRV_DEBUG_PRINTF(ATH_LOG_INF, "%s: index 0x%02x: Vcc %d.%d",
	       card->node.dev_name, link->conf.ConfigIndex,
	       link->conf.Vcc / 10, link->conf.Vcc % 10);
	if (link->conf.Vpp1)
		BUSDRV_DEBUG_PRINTF(ATH_LOG_INF, ", Vpp %d.%d", link->conf.Vpp1 / 10,
		       link->conf.Vpp1 % 10);
	if (link->conf.Attributes & CONF_ENABLE_IRQ)
		BUSDRV_DEBUG_PRINTF(ATH_LOG_INF, ", irq %d", link->irq.AssignedIRQ);
	if (link->io.NumPorts1)
		BUSDRV_DEBUG_PRINTF(ATH_LOG_INF, ", io 0x%04x-0x%04x", link->io.BasePort1,
		       link->io.BasePort1 + link->io.NumPorts1 - 1);
	if (link->io.NumPorts2)
		BUSDRV_DEBUG_PRINTF(ATH_LOG_INF, " & 0x%04x-0x%04x", link->io.BasePort2,
		       link->io.BasePort2 + link->io.NumPorts2 - 1);
	BUSDRV_DEBUG_PRINTF(ATH_LOG_INF, "\n");

	BUSDRV_DEBUG_PRINTF(ATH_DEBUG_TRC, "Exiting ar6000_cs_config\n");

	return;

 cs_failed:
	ar6000_cs_error(link->handle, last_fn, last_ret);

	ar6000_cs_release(link);
}	/* ar6000_cs_config */

/*
 * Release the IO/irq resources assigned to this device.
 */
static void
ar6000_cs_release(dev_link_t *link)
{
	pcmcia_release_configuration(link->handle);
	if (link->io.NumPorts1)
		pcmcia_release_io(link->handle, &link->io);
	if (link->irq.AssignedIRQ)
		pcmcia_release_irq(link->handle, &link->irq);
	link->state &= ~DEV_CONFIG;
} /* ar6000_cs_release */

static void HotPlugHdlr(void *data)
{
	struct ar6000_pccard *card = dev_list->priv;
	PCF_PNP_INFO pnpPtr = NULL;
	CF_DEVICE *pCfDevice = &card->CfDevice;
	A_BOOL isProbed = FALSE;
	PCFFUNCTION pFunction = NULL;

	/* The above code assumes only 1 card is present. In the case of
    * multiple cards, we need to get the data ptr to a Queue and from that
    * Queue get the dev link.
    */
	/* Hotplug */
	for (pFunction = drv_list;(pFunction);pFunction = pFunction->next) {

		/*Determine the CfFunction that has the same PNP info
		* & call its probe function.
		*/
		for ( pnpPtr = pFunction->pIds; pnpPtr->CF_ManufacturerCode != 0 ; \
				pnpPtr++ ) {

			BUSDRV_DEBUG_PRINTF(ATH_LOG_INF, "ar6000_cs_event manfid comp %04x, %04x\n", pCfDevice->pId.CF_ManufacturerID, pnpPtr->CF_ManufacturerID);

			if( pCfDevice->pId.CF_ManufacturerID == pnpPtr->CF_ManufacturerID )
			{
				pCfDevice->pFunction = pFunction;

				/* delay here for the card to stabilize. BMI Comm.
                 * times out otherwise.
                 */
				A_MDELAY(1000);
				if ( !((*pFunction->pProbe)(pFunction, pCfDevice)) ) {
					/* The device is not successfuly probed by this
					* fuction driver. disassociate them.
					*/
					pCfDevice->pFunction = NULL;
				}
				isProbed = TRUE;
				break;
			}
		}
		if (isProbed)
			break;
    }
}

/*
 * The card status event handler.  Mostly, this schedules other stuff
 * to run after an event is received.
 */
static int
ar6000_cs_event(event_t event, int priority,
		       event_callback_args_t * args)
{
	dev_link_t *link = args->client_data;
	A_UINT32 err = 0;

	BUSDRV_DEBUG_PRINTF(ATH_DEBUG_TRC, "AR6000_CS Event Handler Enter\n");

	switch (event) {
	case CS_EVENT_CARD_REMOVAL:
		BUSDRV_DEBUG_PRINTF(ATH_DEBUG_TRC, "Card Removal Event\n");
		link->state &= ~DEV_PRESENT;
		break;

	case CS_EVENT_CARD_INSERTION:

		BUSDRV_DEBUG_PRINTF(ATH_DEBUG_TRC, "Card Insertion Event\n");

		link->state |= DEV_PRESENT | DEV_CONFIG_PENDING;
		ar6000_cs_config(link);

		/* Indicate device available to upper drv in a work task Asynchronously
		*/
		if (drv_list)
			schedule_work(&hotPlugTask);
		break;

	case CS_EVENT_PM_SUSPEND:
		link->state |= DEV_SUSPEND;
		/* Fall through... */
	case CS_EVENT_RESET_PHYSICAL:
		break;

	case CS_EVENT_PM_RESUME:
		link->state &= ~DEV_SUSPEND;
		/* Fall through... */
	case CS_EVENT_CARD_RESET:
		break;
	}

	BUSDRV_DEBUG_PRINTF(ATH_DEBUG_TRC, "AR6000_CS Event Handler Exit\n");

	return err;
}	/* ar6000_cs_event */

/********************************************************************/
/* Module initialization					    */
/********************************************************************/

/* Can't be declared "const" or the whole __initdata section will
 * become const */
static char version[] __initdata = "ar6000_cs.c 1.0";

#ifndef KERNEL_2_4
static struct pcmcia_driver ar6000_driver = {
	.owner		= THIS_MODULE,
	.drv		= {
		.name	= "ar6000_cs",
	},
	.attach		= ar6000_cs_attach,
	.detach		= ar6000_cs_detach,
};
#endif

static int __init
init_ar6000_cs(void)
{
	BUSDRV_DEBUG_PRINTF(ATH_LOG_INF, "%s\n", version);
	BUSDRV_DEBUG_PRINTF(ATH_DEBUG_TRC, "Enter - init_ar6000_cs");

	/* Init Mutex's */
	init_rwsem(&dev_lock);
	init_rwsem(&drv_lock);

	/* We dont pass the data ptr now. Will be useful when multiple
     * devices are supported to pass a Queue to this
     */
	INIT_WORK(&hotPlugTask, HotPlugHdlr, NULL);
#ifdef KERNEL_2_4
	return register_pccard_driver("ar6000_cs", &ar6000_cs_attach, &ar6000_cs_detach);
#else
	return pcmcia_register_driver(&ar6000_driver);
#endif
	BUSDRV_DEBUG_PRINTF(ATH_DEBUG_TRC, "Exit - init_ar6000_cs");
}

static void __exit
exit_ar6000_cs(void)
{
	BUSDRV_DEBUG_PRINTF(ATH_DEBUG_TRC, "Enter - exit_ar6000_cs");
#ifdef KERNEL_2_4
	unregister_pccard_driver("ar6000_cs");
#else
	pcmcia_unregister_driver(&ar6000_driver);
#endif
	//down_read(&dev_lock);
	while (dev_list != NULL) {
		if (dev_list->state & DEV_CONFIG)
			ar6000_cs_release(dev_list);
		ar6000_cs_detach(dev_list);
	}
	//up_read(&dev_lock);

	
	
	BUSDRV_DEBUG_PRINTF(ATH_DEBUG_TRC, "Exit - exit_ar6000_cs");
}

module_init(init_ar6000_cs);
module_exit(exit_ar6000_cs);

#ifdef KERNEL_2_4
MODULE_PARM(debugbusdrv, "i");
#else
module_param(debugbusdrv, int, 0644);
#endif
EXPORT_SYMBOL(CF_RegisterFunction);
EXPORT_SYMBOL(CF_UnregisterFunction);
EXPORT_SYMBOL(CF_SetIrqHandler);
EXPORT_SYMBOL(CF_BusRequest_Word);
EXPORT_SYMBOL(CF_BusRequest_Byte);
