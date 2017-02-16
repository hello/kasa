/*
 * Copyright (c) 2004-2005 Atheros Communications Inc.
 * All rights reserved.
 *
 * $ATH_LICENSE_HOSTSDK0_C$
 *
 */

#ifdef ATHR_EMULATION
/* Linux/OS include files */
#include <stdio.h>
#include <pthread.h>
#endif

/* Atheros include files */
#include <athdefs.h>
#include <a_types.h>
#include <a_osapi.h>
#include <wlan_api.h>
#include <htc.h>
#include <htc_api.h>
#include <htc_packet.h>
#include <wmi.h>
#include <wmi_api.h>
#include <a_hci.h>

#include <common_drv.h>

/* Qualcomm include files */
#ifdef ATHR_EMULATION
#include "qcom_stubs.h"
#endif
#include "wlan_dot11_sys.h"
#include "wlan_adp_def.h"
#include "wlan_adp_oem.h"
#include "wlan_trp.h"
#include "wlan_util.h"

/* Atheros platform include files */
#include "wlan_oem_ath.h"
#include "athdrv_rexos.h"
#include "qcom_htc.h"
#include "a_debug.h"

#ifdef ATHR_EMULATION
static int s_htc_thread_id;     /* Thread ID HTC task */
static pthread_t s_p_thread;    /* Thread's structure */
#else
unsigned int g_dbg_flags;
rex_tcb_type*   htc_task_ptr = NULL;
#endif

/*===========================================================================
FUNCTION WLAN_TRP_OEM_INIT

DESCRIPTION
  This function is invokeed to perform any OEM specific initialization. This 
  implementation make sure this function is called prior to any command 
  arriving at the WLAN Transport task.
    Therefore, the OEM layer should implement here any internal initialization
    logic.

PARAMETERS
  None.
        
RETURN VALUE
  -1 on failure. 
  0 on success.
     
SIDE EFFECTS
  None.
    
DEPENDENCIES
  None.
===========================================================================*/

int wlan_trp_oem_init(void)
{
#ifdef QCOM_INTEGRATION
	/*
	 * Turn on all debugging
	 */
	g_dbg_flags = DBG_HTC      |
				  DBG_WMI      |
				  DBG_DRIVER   |
                  ATH_DEBUG_TRC |
#if 1
	              DBG_HIF      |
				  DBG_OSAPI    |
				  DBG_NETBUF   |
#endif
				  DBG_WLAN_ATH |
                  DBG_INFO     |
                  DBG_ERROR    |
                  DBG_WARNING  ;
#endif
    
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "Enter\n", DBGARG));

#ifdef QCOM_INTEGRATION
	/*
	** First Initialize OS API implementation for Atheros
    */
	wlan_ath_osapi_init();
#endif

	/* Initialize the HTC task - All the API's to the task are set up */
	if(htc_task_init() < 0)
	{
		/* Failed to initialize the HTC task */
		return(-1);
	}

	/* Create a the HTC task */
#ifdef ATHR_EMULATION
	s_htc_thread_id = pthread_create(&s_p_thread, NULL, htc_task, NULL);
#else
	wlan_ath_osapi_create_thread(&htc_task_ptr, NULL, htc_task, NULL);

	if ( NULL == htc_task_ptr)
	{
		/* Failed to create thread */
		DPRINTF(DBG_WLAN_ATH, (DBGFMT "Failed to create HTC Task\n", DBGARG));
		return -1;
	}
#endif

	/* We are good and the task is ready to respond to API calls */
	return(0);
}

/*===========================================================================
FUNCTION WLAN_OEM_TX_OPEN

DESCRIPTION
  This function queries the OEM Module to determine if it is opened for 
	transmission.

PARAMETERS
	None.
	  
RETURN VALUES
	TRUE:  OEM is open for Transmission
  FALSE: OEM is not opened for transmisson
SIDE EFFECTS
  None.
	
DEPENDENCIES
  
===========================================================================*/
boolean wlan_oem_tx_open(void)
{
	boolean rc;

	DPRINTF(DBG_WLAN_ATH, (DBGFMT "Enter\n", DBGARG));

	rc = (boolean) htc_task_query(HTC_QUERY_TX_OPEN);

	DPRINTF(DBG_WLAN_ATH, (DBGFMT "Exit \"%s\"\n", DBGARG, rc ? "TRUE" : "FALSE"));

	return(rc);
}

/*===========================================================================
FUNCTION WLAN_OEM_CMD_OPEN

DESCRIPTION
  This function queries the OEM Module to determine if it is opened for 
	commands processing.

PARAMETERS
	None.
	  
RETURN VALUES
	TRUE:  OEM is open for Command processing
  FALSE: OEM is not opened for Command processing
  
SIDE EFFECTS
  None.
	
DEPENDENCIES
  
===========================================================================*/
boolean wlan_oem_cmd_open(void)
{
	boolean rc;

	DPRINTF(DBG_WLAN_ATH, (DBGFMT "Enter\n", DBGARG));

	rc = (boolean) htc_task_query(HTC_QUERY_CMD_OPEN);

	DPRINTF(DBG_WLAN_ATH, (DBGFMT "Exit \"%s\"\n", DBGARG, rc ? "TRUE" : "FALSE"));

	return(rc);
}

/*===========================================================================
FUNCTION WLAN_OEM_CMD_HDLR

DESCRIPTION
  This function is invoked to let the OEM handle a specific command comming
  from the upper layer

PARAMETERS
	A pointer to the command. Note that when this function returns, the cmd is
  recycled.
	  
RETURN VALUES
	None.
  
SIDE EFFECTS
  None.
	
DEPENDENCIES
  
===========================================================================*/
void wlan_oem_cmd_hdlr(wlan_trp_adp_cmd_type* ptr_cmd)
{
	struct htc_request *req;

	DPRINTF(DBG_WLAN_ATH, (DBGFMT "Enter - cmd %d\n", DBGARG, ptr_cmd->hdr.cmd_id));

	if((req = htc_alloc_request()) == NULL)
	{
		return;
	}

	req->cmd = ptr_cmd->hdr.cmd_id;
	A_MEMCPY(&req->req_info, &ptr_cmd->req_info,
		sizeof(wlan_trp_adp_req_type));
	htc_task_command(req);

	return;
}

/*===========================================================================
FUNCTION WLAN_OEM_TX_REQ

DESCRIPTION
  This function gives an LLC PDU for transmission at the OEM Module.

PARAMETERS
	pdu_len: Length of PDU
	buf: pointer to buffer containing LLC PDU
	meta_info: pointer to meta info about PDU to transmit.
	  
RETURN VALUES
	 0: if successfull
  -1: if not
	 
SIDE EFFECTS
  None.
	
DEPENDENCIES
  
===========================================================================*/
int wlan_oem_tx_req(uint16 pdu_len, byte* buf,
	 wlan_adp_tx_pkt_meta_info_type* meta_info)
{
	char *pkt, *p;
	int len;
	A_UINT8 dstaddr[ATH_MAC_LEN];

	DPRINTF(DBG_WLAN_ATH, (DBGFMT "Enter len %d\n", DBGARG, pdu_len));

	/* How big is it */
	len = pdu_len + sizeof(ATH_MAC_HDR);

    if((pkt = A_NETBUF_ALLOC(len + sizeof(meta_info->tid))) == NULL)
	{
		DPRINTF(DBG_WLAN_ATH,
			(DBGFMT "Failed to allocate network buffer.\n", DBGARG));
		return(-1);
	}

	/* Start to populate */

	/* Add space */
	A_NETBUF_PUT(pkt, len);

	/* Get the data pointer */
	p = A_NETBUF_VIEW_DATA(pkt, char *, len);

	/* Add the 802.3 header */
	wlan_util_uint64_to_macaddress(dstaddr, meta_info->dst_mac_addr);
	A_MEMCPY(p, dstaddr, ATH_MAC_LEN);
	A_MEMCPY(p + ATH_MAC_LEN, htc_get_macaddress(), ATH_MAC_LEN);
#ifdef ATHR_EMULATION
	/* Set the type to an ARP packet */
	p[12] = 0x08;
	p[13] = 0x06;
#else
	p[12] = (pdu_len >> 8) & 0xff;
	p[13] = (pdu_len >> 0) & 0xff;
#endif

	/* Move past the header */
	p += sizeof(ATH_MAC_HDR);

	/* Copy the data */
	A_MEMCPY(p, buf, pdu_len);

    /* save tid of this packet so that HTC task can use it later */
    A_MEMCPY(p + pdu_len, &meta_info->tid, sizeof(meta_info->tid));
    
	/* Give it to the HTC task */
	htc_task_tx_data(pkt, meta_info->tid);

	return(0);
}

/*===========================================================================
FUNCTION WLAN_OEM_TX_PKT_REQ

DESCRIPTION
  This function gives an LLC PDU for transmission to the OEM Module. The
  LLC paket is formatted 

PARAMETERS
	pkt: dsm item containing the pkt to xmit 
	  
RETURN VALUES
   0: if successfull
  -1: if not
	 
SIDE EFFECTS
  None.
	
DEPENDENCIES
  
===========================================================================*/
int wlan_oem_tx_pkt_req(uint16 pdu_len, dsm_item_type* tx_pkt,
  wlan_adp_tx_pkt_meta_info_type* meta_info)
{
	int len;
	char *p;
	void *pkt;
	A_UINT8 dstaddr[ATH_MAC_LEN];

	DPRINTF(DBG_WLAN_ATH, (DBGFMT "Enter len %d\n", DBGARG, pdu_len));

	/* How big is it */
	len = pdu_len + sizeof(ATH_MAC_HDR);

    if((pkt = A_NETBUF_ALLOC(len + sizeof(meta_info->tid))) == NULL)
	{
		DPRINTF(DBG_HTC,
			(DBGFMT "Failed to allocate network buffer.\n", DBGARG));
		return(-1);
	}

	/* Add space */
	A_NETBUF_PUT(pkt, len);

	/* Get the data pointer */
	p = A_NETBUF_VIEW_DATA(pkt, char *, len);

	/* Add the 802.3 header */
	wlan_util_uint64_to_macaddress(dstaddr, meta_info->dst_mac_addr);
	A_MEMCPY(p, dstaddr, ATH_MAC_LEN);
	A_MEMCPY(p + ATH_MAC_LEN, htc_get_macaddress(), ATH_MAC_LEN);
#ifdef ATHR_EMULATION
	/* Set the type to an ARP packet */
	p[12] = 0x08;
	p[13] = 0x06;
#else
	p[12] = (pdu_len >> 8) & 0xff;
	p[13] = (pdu_len >> 0) & 0xff;
#endif

	/* Move past header */
	p += sizeof(ATH_MAC_HDR);

	/* Add the data */
	if(dsm_pullup(&tx_pkt, p, pdu_len) != pdu_len)
	{
		DPRINTF(DBG_WLAN_ATH,
			(DBGFMT "Failed to pullup DSM Item.\n", DBGARG)); 
		A_ASSERT(0);  
	}

    /* save tid of this packet so that HTC task can use it later */
    A_MEMCPY(p + pdu_len, &meta_info->tid, sizeof(meta_info->tid));
    
	/* Make sure we free the DSM item */
	dsm_free_packet(&tx_pkt);

	/* Give it to the HTC task */
	htc_task_tx_data(pkt, meta_info->tid);

	return(0);
}

void wlan_oem_send_acl_data(void* buf)
{
    void *osbuf = NULL;
    HCI_ACL_DATA_PKT *acl;
    A_UINT8 hdr_size, *datap=NULL;

	DPRINTF(DBG_WLAN_ATH, (DBGFMT "Enter\n", DBGARG));

    acl = (HCI_ACL_DATA_PKT *)buf;
    hdr_size = sizeof(acl->hdl_and_flags) + sizeof(acl->data_len);

    /* Headroom is for creating 802.11 4-addr frame in PAL in f/w */
    osbuf = A_NETBUF_ALLOC(hdr_size + acl->data_len);
    if (osbuf == NULL) {
       return;
    }
    
    A_NETBUF_PUT(osbuf, hdr_size + acl->data_len);
    datap = (A_UINT8 *)A_NETBUF_DATA(osbuf);
    
    /* Real copy to osbuf */
    acl = (HCI_ACL_DATA_PKT *)(datap);
    A_MEMCPY(acl, buf, hdr_size + ((HCI_ACL_DATA_PKT *)buf)->data_len);

	/* Give it to the HTC task */
	htc_task_tx_bt_data(osbuf);
}

/*****************************************************************************/
/**                                                                         **/
/** WLAN interface functions                                                **/
/**                                                                         **/
/*****************************************************************************/

int wlan_ath_start(void)
{

	DPRINTF(DBG_WLAN_ATH, (DBGFMT "Enter\n", DBGARG));

	/* Signal the htc task */
	htc_task_signal(HTC_START_SIG);

	return(0);
}

int wlan_ath_stop(void)
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "Enter\n", DBGARG));

	/* Signal the htc task */
	htc_task_signal(HTC_STOP_SIG);

	return(0);
}

#ifdef CONFIG_HOST_TCMD_SUPPORT
int wlan_ath_test_start(void)
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "Enter\n", DBGARG));
	htc_set_test_mode();
	htc_task_signal(HTC_START_SIG);
	return(0);
}
#endif

int wlan_ath_fw_upload(void)
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "Enter\n", DBGARG));

	htc_enable_firmware_upload();

	return(0);
}
