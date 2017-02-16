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
#include <stdio.h>
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
#include <bmi.h>
#include <ieee80211.h>

#include <common_drv.h>

/* Qualcomm include files */
#ifdef ATHR_EMULATION
#include "qcom_stubs.h"
#endif
#include "wlan_dot11_sys.h"
#include "wlan_adp_def.h"
#include "wlan_adp_oem.h"
#include "wlan_trp.h"

/* Atheros platform include files */
#include "athdrv_rexos.h"
#include "targaddrs_rexos.h"
#include "qcom_htc.h"
#include "a_debug.h"

#define HTC_DATA_REQUEST_RING_BUFFER_SIZE  32
#define MAX_ALLOWED_TXQ_DEPTH              (HTC_DATA_REQUEST_RING_BUFFER_SIZE - 2)
#define HTC_TXFLOW_CTRL_THRESH(numpts)     (numpts * (MAX_ALLOWED_TXQ_DEPTH - 1))

static int reduce_credit_dribble = 1 + HTC_CONNECT_FLAGS_THRESHOLD_LEVEL_ONE_HALF;

int tspecCompliance = ATHEROS_COMPLIANCE;
int bypasswmi = 0;

static struct ar_cookie s_ar_cookie_mem[MAX_COOKIE_NUM];

extern void qcom_init_profile_info(AR_SOFTC_T *ar);

/*****************************************************************************/
/*****************************************************************************/
/**                                                                         **/
/** Support functions                                                       **/
/**                                                                         **/
/*****************************************************************************/
/*****************************************************************************/

int check_flow_on(void *device)
{
	AR_SOFTC_T *ar = (AR_SOFTC_T *) device;

	DPRINTF(DBG_DRIVER, (DBGFMT "Enter\n", DBGARG));

	A_ASSERT(ar != NULL);

    if (ar->arTotalTxDataPending < HTC_TXFLOW_CTRL_THRESH(ar->arNumDataEndPts))
    {
        /* netif_wake_queue(ar->arNetDev); */
        DPRINTF(DBG_WMI, (DBGFMT "Start transmit.\n", DBGARG));
        htc_start_data();
    }

	return(0);
}

int check_flow_off(void *device)
{
	AR_SOFTC_T *ar = (AR_SOFTC_T *) device;

	DPRINTF(DBG_DRIVER, (DBGFMT "Enter\n", DBGARG));

	A_ASSERT(ar != NULL);

	/* If all data queues are full, notify upper layer to stop. */
	if (ar->arTotalTxDataPending >= HTC_TXFLOW_CTRL_THRESH(ar->arNumDataEndPts))
	{
		DPRINTF(DBG_DRIVER, (DBGFMT "Stop transmit.\n", DBGARG));
		htc_stop_data();
		return(1);
	}

	return(0);
}

/*****************************************************************************/
/*****************************************************************************/
/**                                                                         **/
/** Driver routines                                                         **/
/**                                                                         **/
/*****************************************************************************/
/*****************************************************************************/

void qcom_driver_init(void)
{
    OSDRV_CALLBACKS osdrvCallbacks;
    
	DPRINTF(DBG_DRIVER, (DBGFMT "Enter\n", DBGARG));
	
    A_MEMZERO(&osdrvCallbacks,sizeof(osdrvCallbacks));
    osdrvCallbacks.deviceInsertedHandler = qcom_avail_ev;
    osdrvCallbacks.deviceRemovedHandler = qcom_unavail_ev;
    
    if (HIFInit(&osdrvCallbacks) != A_OK)
    {
    	DPRINTF(DBG_DRIVER, (DBGFMT "Couldn't initialize HIF.\n", DBGARG));
        return;
    }
}

void qcom_driver_exit(void)
{
	void *handle;

	DPRINTF(DBG_DRIVER, (DBGFMT "Enter\n", DBGARG));

	/* Get the handle */
	handle = htc_device_handle();

	if(handle != NULL)
	{
		qcom_driver_destroy(handle);
	}

	/* Clear up the htc_task */
	(void) htc_task_exit();
}

void qcom_driver_destroy(void *device)
{
	AR_SOFTC_T *ar = (AR_SOFTC_T *) device;

	DPRINTF(DBG_DRIVER, (DBGFMT "Enter\n", DBGARG));

	A_ASSERT(ar != NULL);

	/* Do a target stop first */
	qcom_driver_close(device);
}

/* cleanup cookie queue */
struct ar_cookie * qcom_alloc_cookie(AR_SOFTC_T  *ar)
{
    struct ar_cookie   *cookie;

    cookie = ar->arCookieList;
    if(cookie != NULL)
    {
        ar->arCookieList = cookie->arc_list_next;
    }

    DPRINTF(DBG_DRIVER, (DBGFMT "allocated cookie %p.\n", DBGARG, cookie));
    return cookie;
}

/* Init cookie queue */
void qcom_free_cookie(AR_SOFTC_T *ar, struct ar_cookie * cookie)
{
    /* Insert first */
    cookie->arc_list_next = ar->arCookieList;
    ar->arCookieList = cookie;
    
    DPRINTF(DBG_DRIVER, (DBGFMT "freed cookie %p.\n", DBGARG, cookie));
}

/* Init cookie queue */
void qcom_cookie_init(AR_SOFTC_T *ar)
{
    A_UINT32    i;

    ar->arCookieList = NULL;
    A_MEMZERO(s_ar_cookie_mem, sizeof(s_ar_cookie_mem));

    for (i = 0; i < MAX_COOKIE_NUM; i++) {
        qcom_free_cookie(ar, &s_ar_cookie_mem[i]);
    }
}

/* cleanup cookie queue */
void qcom_cookie_cleanup(AR_SOFTC_T *ar)
{
    /* It is gone .... */
    ar->arCookieList = NULL;
}

/* connect to a service */
static 
A_STATUS ar6000_connectservice(AR_SOFTC_T               *ar,
                               HTC_SERVICE_CONNECT_REQ  *pConnect,
                               char                     *pDesc)
{
    A_STATUS                 status;
    HTC_SERVICE_CONNECT_RESP response;
    
    do {      
        
        A_MEMZERO(&response,sizeof(response));
        
        status = HTCConnectService(ar->arHtcTarget, 
                                   pConnect,
                                   &response);
        
        if (A_FAILED(status)) {
            AR_DEBUG_PRINTF(DBG_DRIVER, (" Failed to connect to %s service status:%d \n", pDesc, status));
            break;    
        }

        switch (pConnect->ServiceID) {
            case WMI_CONTROL_SVC :
                if (ar->arWmiEnabled) {
                        /* set control endpoint for WMI use */
                    wmi_set_control_ep(ar->arWmi, response.Endpoint);
                }
                    /* save EP for fast lookup */    
                ar->arControlEp = response.Endpoint;
                break;
            case WMI_DATA_BE_SVC :
                arSetAc2EndpointIDMap(ar, WMM_AC_BE, response.Endpoint);
                break;
            case WMI_DATA_BK_SVC :
                arSetAc2EndpointIDMap(ar, WMM_AC_BK, response.Endpoint);
                break;
            case WMI_DATA_VI_SVC :
                arSetAc2EndpointIDMap(ar, WMM_AC_VI, response.Endpoint);
                 break;
           case WMI_DATA_VO_SVC :
                arSetAc2EndpointIDMap(ar, WMM_AC_VO, response.Endpoint);
                break;
           default:
                AR_DEBUG_PRINTF(DBG_DRIVER, ("ServiceID not mapped %d\n", pConnect->ServiceID));
                status = A_EINVAL;
            break;
        }
    } while (FALSE);
    
    return status;
}

void qcom_tx_data_cleanup(AR_SOFTC_T *ar)
{
        /* flush all the data (non-control) streams */
    HTCFlushEndpoint(ar->arHtcTarget,
                     arAc2EndpointID(ar, WMM_AC_BE),
                     AR6K_DATA_PKT_TAG);
    HTCFlushEndpoint(ar->arHtcTarget,
                     arAc2EndpointID(ar, WMM_AC_BK),
                     AR6K_DATA_PKT_TAG);    
    HTCFlushEndpoint(ar->arHtcTarget,
                     arAc2EndpointID(ar, WMM_AC_VI),
                     AR6K_DATA_PKT_TAG);
    HTCFlushEndpoint(ar->arHtcTarget,
                     arAc2EndpointID(ar, WMM_AC_VO),
                     AR6K_DATA_PKT_TAG);
}

static
HTC_SEND_FULL_ACTION qcom_tx_queue_full(void *Context, HTC_PACKET *pPacket)
{
    AR_SOFTC_T     *ar = (AR_SOFTC_T *)Context;
    HTC_SEND_FULL_ACTION    action = HTC_SEND_FULL_KEEP;
    A_BOOL                  stopNet = FALSE;
    HTC_ENDPOINT_ID         Endpoint = HTC_GET_ENDPOINT_FROM_PKT(pPacket);

    do {

        if (bypasswmi) {
            /* for endpointping testing no other checks need to be made
             * we can however still allow the network to stop */
            stopNet = TRUE;
            break;
        }

        if (Endpoint == ar->arControlEp) {
            /* under normal WMI if this is getting full, then something is running rampant
             * the host should not be exhausting the WMI queue with too many commands
             * the only exception to this is during testing using endpointping */
            A_MUTEX_LOCK(&ar->arLock);
                /* set flag to handle subsequent messages */
            ar->arWMIControlEpFull = TRUE;
            A_MUTEX_UNLOCK(&ar->arLock);
            AR_DEBUG_PRINTF(DBG_DRIVER, ("WMI Control Endpoint is FULL!!! \n"));
                /* no need to stop the network */
            stopNet = FALSE;
            break;
        }

        /* if we get here, we are dealing with data endpoints getting full */
        if (HTC_GET_TAG_FROM_PKT(pPacket) == AR6K_CONTROL_PKT_TAG) {
            /* don't drop control packets issued on ANY data endpoint */
            break;
        }

        if (ar->arNetworkType == ADHOC_NETWORK) {
            /* in adhoc mode, we cannot differentiate traffic priorities so there is no need to
             * continue, however we should stop the network */
            stopNet = TRUE;
            break;
        }

        if (ar->arAcStreamPriMap[arEndpoint2Ac(ar,Endpoint)] < ar->arHiAcStreamActivePri) {
                /* this stream's priority is less than the highest active priority, we
                 * give preference to the highest priority stream by directing
                 * HTC to drop the packet that overflowed */
            action = HTC_SEND_FULL_DROP;
                /* since we are dropping packets, no need to stop the network */
            stopNet = FALSE;
            break;
        }

    } while (FALSE);

    if (stopNet) {
    	/* 
         * TODO need feedback mechanism for rexos to stop sending data just like
         * netif_stop_queue().
         * The queue will resume in qcom_tx_complete().
         */
    }

    return action;
}

int qcom_driver_open(void *device)
{
	AR_SOFTC_T *ar = (AR_SOFTC_T *) device;
	A_STATUS status;

	DPRINTF(DBG_DRIVER, (DBGFMT "Enter\n", DBGARG));

	A_ASSERT(ar != NULL);

	/* BMIDone() is already done by qcom_test_fw_upload() for AR6001.
       qcom_test_fw_upload() isn't called by AR6002 */
	if ((FALSE == htc_get_test_mode()) || 
        (TARGET_TYPE_AR6002 == ar->arTargetType))
	{
		status = BMIDone(ar->arHifDevice);
		if (status != A_OK)
		{
			DPRINTF(DBG_DRIVER, (DBGFMT "Failed in BMIDone()\n", DBGARG));
			return(-1);
		}
	}

    if (!bypasswmi)
    {   
        /* Indicate that WMI is enabled (although not ready yet) */
        ar->arWmiEnabled = TRUE;
        if ((ar->arWmi = wmi_init(ar)) == NULL)
        {
            DPRINTF(DBG_DRIVER, (DBGFMT "Failed to initialize WMI.\n", DBGARG));
            return(-1);
        }

        DPRINTF(DBG_DRIVER, (DBGFMT "Got WMI @ 0x%08x.\n", DBGARG,
            (unsigned int) ar->arWmi));
    }
    
    do {
        HTC_SERVICE_CONNECT_REQ connect;
        
        /* the reason we have to wait for the target here is that the driver layer
         * has to init BMI in order to set the host block size, 
         */
        status = HTCWaitTarget(ar->arHtcTarget);
        
        if (A_FAILED(status)) {
            DPRINTF(DBG_DRIVER, (DBGFMT "Failed in HTCWaitTarget()\n", DBGARG));
            break;
        }
        
        A_MEMZERO(&connect, sizeof(connect));
        
        /* meta data is unused for now */
        connect.pMetaData = NULL;
        connect.MetaDataLength = 0;
        
        /* these fields are the same for all service endpoints */
        connect.EpCallbacks.pContext = ar;
        connect.EpCallbacks.EpTxComplete = qcom_tx_complete;   
        connect.EpCallbacks.EpRecv = qcom_rx;
        connect.EpCallbacks.EpRecvRefill = qcom_rx_refill;
        connect.EpCallbacks.EpSendFull = qcom_tx_queue_full;
        
        /* set the max queue depth so that our qcom_tx_queue_full handler gets called.
         * Linux has the peculiarity of not providing flow control between the
         * NIC and the network stack. There is no API to indicate that a TX packet
         * was sent which could provide some back pressure to the network stack.
         * Under linux you would have to wait till the network stack consumed all sk_buffs
         * before any back-flow kicked in. Which isn't very friendly. 
         * So we have to manage this ourselves */
        connect.MaxSendQueueDepth = 32;  
        connect.EpCallbacks.RecvRefillWaterMark = AR6000_MAX_RX_BUFFERS / 4; /* set to 25 % */
        if (0 == connect.EpCallbacks.RecvRefillWaterMark) {
            connect.EpCallbacks.RecvRefillWaterMark++;        
        }
        
        /* connect to control service */
        connect.ServiceID = WMI_CONTROL_SVC;
        status = ar6000_connectservice(ar,
                                       &connect,
                                       "WMI CONTROL");
        if (A_FAILED(status)) {
            DPRINTF(DBG_DRIVER, (DBGFMT "Failed to connect to WMI_CONTROL service\n", DBGARG));
            break;    
        }

        /* for the remaining data services set the connection flag to reduce dribbling,
         * if configured to do so */
        if (reduce_credit_dribble) {    
            connect.ConnectionFlags |= HTC_CONNECT_FLAGS_REDUCE_CREDIT_DRIBBLE; 
            /* the credit dribble trigger threshold is (reduce_credit_dribble - 1) for a value
             * of 0-3 */
            connect.ConnectionFlags &= ~HTC_CONNECT_FLAGS_THRESHOLD_LEVEL_MASK;
            connect.ConnectionFlags |= 
                        ((A_UINT16)reduce_credit_dribble - 1) & HTC_CONNECT_FLAGS_THRESHOLD_LEVEL_MASK;
        }
        
        /* connect to best-effort service */
        connect.ServiceID = WMI_DATA_BE_SVC;  
        
        status = ar6000_connectservice(ar,
                                       &connect,
                                       "WMI DATA BE"); 
        if (A_FAILED(status)) {
            DPRINTF(DBG_DRIVER, (DBGFMT "Failed to connect to WMI_DATA_BE service\n", DBGARG));
            break;    
        }
        
        /* connect to back-ground 
         * map this to WMI LOW_PRI */
        connect.ServiceID = WMI_DATA_BK_SVC;  
        status = ar6000_connectservice(ar,
                                       &connect,
                                       "WMI DATA BK"); 
        if (A_FAILED(status)) {
            DPRINTF(DBG_DRIVER, (DBGFMT "Failed to connect to WMI_DATA_BK service\n", DBGARG));
            break;
        }
        
        /* connect to Video service, map this to 
         * to HI PRI */
        connect.ServiceID = WMI_DATA_VI_SVC;  
        status = ar6000_connectservice(ar,
                                       &connect,
                                       "WMI DATA VI"); 
        if (A_FAILED(status)) {
            DPRINTF(DBG_DRIVER, (DBGFMT "Failed to connect to WMI_DATA_VI service\n", DBGARG));
            break;    
        }
        
        /* connect to VO service, this is currently not
         * mapped to a WMI priority stream due to historical reasons.
         * WMI originally defined 3 priorities over 3 mailboxes
         * We can change this when WMI is reworked so that priorities are not
         * dependent on mailboxes */
        connect.ServiceID = WMI_DATA_VO_SVC;  
        status = ar6000_connectservice(ar,
                                       &connect,
                                       "WMI DATA VO"); 
        if (A_FAILED(status)) {
            DPRINTF(DBG_DRIVER, (DBGFMT "Failed to connect to WMI_DATA_VO service\n", DBGARG));
            break;
        }

        A_ASSERT(arAc2EndpointID(ar,WMM_AC_BE) != 0);
        A_ASSERT(arAc2EndpointID(ar,WMM_AC_BK) != 0);
        A_ASSERT(arAc2EndpointID(ar,WMM_AC_VI) != 0);
        A_ASSERT(arAc2EndpointID(ar,WMM_AC_VO) != 0);
  
            /* setup access class priority mappings */
        ar->arAcStreamPriMap[WMM_AC_BK] = 0; /* lowest  */
        ar->arAcStreamPriMap[WMM_AC_BE] = 1; /*         */
        ar->arAcStreamPriMap[WMM_AC_VI] = 2; /*         */
        ar->arAcStreamPriMap[WMM_AC_VO] = 3; /* highest */        
    } while (FALSE);

    if (A_FAILED(status)) {
        return (-1);
    }
    
    /*
     * give our connected endpoints some buffers
     */
    qcom_rx_refill(ar, ar->arControlEp);
    qcom_rx_refill(ar, arAc2EndpointID(ar,WMM_AC_BE));

    /* 
     * We will post the receive buffers only for SPE testing and so we are
     * making it conditional on the 'bypasswmi' flag.
     */
    if (bypasswmi) {
        qcom_rx_refill(ar,arAc2EndpointID(ar,WMM_AC_BK));
        qcom_rx_refill(ar,arAc2EndpointID(ar,WMM_AC_VI));
        qcom_rx_refill(ar,arAc2EndpointID(ar,WMM_AC_VO));
    }

    /* setup credit distribution */
    ar6000_setup_credit_dist(ar->arHtcTarget, &ar->arCreditStateInfo);
    
    /* Since cookies are used for HTC transports, they should be */
    /* initialized prior to enabling HTC.                        */
    qcom_cookie_init(ar);
    
	/* Enable the target and the interrupts associated with it */
	status = HTCStart(ar->arHtcTarget);

    if(status != A_OK)
	{
		DPRINTF(DBG_DRIVER, (DBGFMT "HTCStart() failed with %d\n", DBGARG, status));
        if (ar->arWmiEnabled == TRUE) {
			wmi_shutdown(ar->arWmi);
			ar->arWmiEnabled = FALSE;
			ar->arWmi = NULL;
		}
		qcom_cookie_cleanup(ar);
		return -1;
	}

    ar->arNumDataEndPts = 1;
    
	return 0;
}

int qcom_driver_close(void *device)
{
	AR_SOFTC_T *ar = (AR_SOFTC_T *) device;

	A_ASSERT(ar != NULL);

	DPRINTF(DBG_DRIVER, (DBGFMT "Enter\n", DBGARG));

    config_exit();
    
	/* Check to see if we are enabled */
	if(ar == NULL || ar->arWmiEnabled == FALSE)
	{
		DPRINTF(DBG_DRIVER,
			(DBGFMT "WARNING - driver is not opened\n", DBGARG));
		return(0);
	}

    /* Disable the target and the interrupts associated with it */
    if (ar->arWmiReady == TRUE)
    {
        if (!bypasswmi)
        {
            if (ar->arConnected == TRUE || ar->arConnectPending == TRUE)
            {
                DPRINTF(DBG_DRIVER, (DBGFMT "Disconnect\n", DBGARG));
                wmi_disconnect_cmd(ar->arWmi);
                A_MUTEX_LOCK(&ar->arLock);
                qcom_init_profile_info(ar);
                A_MUTEX_UNLOCK(&ar->arLock);
            }

            qcom_dbglog_get_debug_logs(ar, TRUE);
			ar->arWmiReady  = FALSE;
            ar->arConnected = FALSE;
            ar->arConnectPending = FALSE;
            wmi_shutdown(ar->arWmi);
            ar->arWmiEnabled = FALSE;
            ar->arWmi = NULL;
            ar->arWlanState = WLAN_ENABLED;
    	}
    }
    else {
    	/* Shut down WMI if we have started it */
        if(ar->arWmiEnabled == TRUE)
        {
            DPRINTF(DBG_DRIVER, (DBGFMT "Shut down WMI\n", DBGARG));
            wmi_shutdown(ar->arWmi);
            ar->arWmiEnabled = FALSE;
            ar->arWmi = NULL;
        }
    }
    
    if (ar->arHtcTarget != NULL) {
	    HTCStop(ar->arHtcTarget);
	    /* destroy HTC */
        HTCDestroy(ar->arHtcTarget);
    }
    
    if (ar->arHifDevice != NULL) {
        /*release the device so we do not get called back on remove incase we
         * we're explicity destroyed by module unload */
        HIFReleaseDevice(ar->arHifDevice);
        HIFShutDownDevice(ar->arHifDevice);
    }
    
    /* Done with cookies */
    qcom_cookie_cleanup(ar);

	return(0);
}

A_UINT8 qcom_ibss_map_epid(void* buff, void* device, A_UINT32 * mapNo)
{
    AR_SOFTC_T      *ar = (AR_SOFTC_T *)device;
    A_UINT8         *datap;
    ATH_MAC_HDR     *macHdr;
    A_UINT32         i, eptMap;

    (*mapNo) = 0;
    datap = A_NETBUF_DATA(buff);
    macHdr = (ATH_MAC_HDR *)(datap + sizeof(WMI_DATA_HDR));
    if (IEEE80211_IS_MULTICAST(macHdr->dstMac)) {
        return ENDPOINT_2;
    }

    eptMap = -1;
    for (i = 0; i < ar->arNodeNum; i ++) {
        if (IEEE80211_ADDR_EQ(macHdr->dstMac, ar->arNodeMap[i].macAddress)) {
            (*mapNo) = i + 1;
            ar->arNodeMap[i].txPending ++;
            return ar->arNodeMap[i].epId;
        }

        if ((eptMap == -1) && !ar->arNodeMap[i].txPending) {
            eptMap = i;
        }
    }

    if (eptMap == -1) {
        eptMap = ar->arNodeNum;
        ar->arNodeNum ++;
        A_ASSERT(ar->arNodeNum <= MAX_NODE_NUM);
    }

    A_MEMCPY(ar->arNodeMap[eptMap].macAddress, macHdr->dstMac, IEEE80211_ADDR_LEN);

    for (i = ENDPOINT_2; i <= ENDPOINT_5; i ++) {
        if (!ar->arTxPending[i]) {
            ar->arNodeMap[eptMap].epId = i;
            break;
        }
        // No free endpoint is available, start redistribution on the inuse endpoints.
        if (i == ENDPOINT_5) {
            ar->arNodeMap[eptMap].epId = ar->arNexEpId;
            ar->arNexEpId ++;
            if (ar->arNexEpId > ENDPOINT_5) {
                ar->arNexEpId = ENDPOINT_2;
            }
        }
    }

    (*mapNo) = eptMap + 1;
    ar->arNodeMap[eptMap].txPending ++;

    return ar->arNodeMap[eptMap].epId;
}

int qcom_driver_tx(void *buff, void *device, A_UINT8 up)
{
#define AC_NOT_MAPPED   99
    AR_SOFTC_T *ar = (AR_SOFTC_T *) device;
    A_UINT8            ac = AC_NOT_MAPPED;
    HTC_ENDPOINT_ID    eid = ENDPOINT_UNUSED;
	A_UINT32        mapNo = 0;
	static int i = 0;
	struct ar_cookie *cookie;
    A_BOOL            checkAdHocPsMapping = FALSE;
    A_INT16 buffOffset = 0;
    
	i++;
	DPRINTF(DBG_DRIVER, (DBGFMT "Enter %d\n", DBGARG, i));

	A_MUTEX_LOCK(&ar->arLock);

	/* Check if we need to turn off the transmit flow */
	if(check_flow_off(device) != 0)
	{
		DPRINTF(DBG_DRIVER, (DBGFMT "All queues are full.\n", DBGARG));
        A_MUTEX_UNLOCK(&ar->arLock);
        return (1);
	}

    A_MUTEX_UNLOCK(&ar->arLock);
    
    do {
    	if (ar->arWmiEnabled == TRUE)
    	{
    		/* Might want to check with A_NETBUF_HEADROOM(buff) if */
    		/* we have enough space to add headers                 */
    
    		if (wmi_dix_2_dot3(ar->arWmi, buff) != A_OK)
    		{
    			DPRINTF(DBG_DRIVER, (DBGFMT "wmi_dix_2_dot3() failed.\n", DBGARG));
                break;
    		}
    
    		if (wmi_data_hdr_add(ar->arWmi, buff, DATA_MSGTYPE, 0, 0, 0, NULL) != A_OK)
    		{
    			DPRINTF(DBG_DRIVER, (DBGFMT "wmi_data_hdr_add() failed.\n", DBGARG));
                break;
    		}
    
    		if ((ar->arNetworkType == ADHOC_NETWORK) && 
                ar->arIbssPsEnable && ar->arConnected) 
            {
                checkAdHocPsMapping = TRUE;
            } else {
                    /* Extract the end point information */
    			ac = wmi_implicit_create_pstream(ar->arWmi, buff, up, ar->arWmmEnabled);
    
    			DPRINTF(DBG_DRIVER,
    				(DBGFMT "Tx on ac %d.\n", DBGARG, ac));
            }
    	}
    	else
    	{
    		DPRINTF(DBG_DRIVER, (DBGFMT "WMI is not enabled.\n", DBGARG));
    		break;
    	}
    } while (FALSE);
    
    /* did we succeed ? */
    if ((ac == AC_NOT_MAPPED) && !checkAdHocPsMapping)
    {
        /* cleanup and exit */
        DPRINTF(DBG_DRIVER, (DBGFMT "Dropping the frame\n", DBGARG));
        A_NETBUF_FREE(buff);
        AR6000_STAT_INC(ar, tx_dropped);
        AR6000_STAT_INC(ar, tx_aborted_errors);
        return (2);
    }

    cookie = NULL;
    
    A_MUTEX_LOCK(&ar->arLock);
    
    do {
        if (checkAdHocPsMapping) {
            eid = qcom_ibss_map_epid(buff, device, &mapNo);
        }else {
            eid = arAc2EndpointID (ar, ac);
        }
            /* validate that the endpoint is connected */
        if (eid == 0 || eid == ENDPOINT_UNUSED ) {
            DPRINTF(DBG_DRIVER, (DBGFMT " eid %d is NOT mapped!\n", DBGARG, eid));
            break;
        }
        
        cookie = qcom_alloc_cookie(ar);
        
        if (cookie != NULL) {
            /* update counts while the lock is held */
            ar->arTxPending[ac]++;
            ar->arTotalTxDataPending++;
        }
    } while (FALSE);

	A_MUTEX_UNLOCK(&ar->arLock);

    if (cookie != NULL) {
        cookie->arc_bp[0] = (A_UINT32) buff;
        cookie->arc_bp[1] = mapNo;
    
        DPRINTF(DBG_DRIVER, (DBGFMT "Tx cookie %p, %p, %d .\n", DBGARG, cookie, buff, mapNo));
        
        SET_HTC_PACKET_INFO_TX(&cookie->HtcPkt,
                               cookie,
                               A_NETBUF_DATA(buff),
                               A_NETBUF_LEN(buff),
                               eid,
                               AR6K_DATA_PKT_TAG);
        /* HTC interface is asynchronous, if this fails, cleanup will happen in
         * the qcom_tx_complete callback */
        HTCSendPkt(ar->arHtcTarget, &cookie->HtcPkt);
    }
    else {
        /* no packet to send, cleanup */
        A_NETBUF_FREE(buff);
        AR6000_STAT_INC(ar, tx_dropped);
        AR6000_STAT_INC(ar, tx_aborted_errors);        
    }

	return(0);
}

int qcom_driver_tx_bt(void *buff, void *device)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *) device;
    HTC_ENDPOINT_ID    eid = ENDPOINT_UNUSED;
	struct ar_cookie *cookie;
	static int i = 0;
    
	i++;
	DPRINTF(DBG_DRIVER, (DBGFMT "Enter %d\n", DBGARG, i));
    
    if (wmi_data_hdr_add(ar->arWmi, buff, DATA_MSGTYPE, 0, WMI_DATA_HDR_DATA_TYPE_ACL, 0, NULL) != A_OK) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("XIOCTL_ACL_DATA - wmi_data_hdr_add failed\n"));
        return (2);
    }
    
	A_MUTEX_LOCK(&ar->arLock);

	/* Check if we need to turn off the transmit flow */
	if(check_flow_off(device) != 0)
	{
		DPRINTF(DBG_DRIVER, (DBGFMT "All queues are full.\n", DBGARG));
        A_MUTEX_UNLOCK(&ar->arLock);
        return (1);
	}

    /* For now we send ACL on BE endpoint: We can also have a dedicated EP */
    eid = arAc2EndpointID (ar, 0);
    /* allocate resource for this packet */
    cookie = qcom_alloc_cookie(ar);
    
    if (cookie != NULL) {
        /* update counts while the lock is held */
        ar->arTxPending[eid]++;
        ar->arTotalTxDataPending++;
    }

	A_MUTEX_UNLOCK(&ar->arLock);

    if (cookie != NULL) {
        cookie->arc_bp[0] = (A_UINT32) buff;
        cookie->arc_bp[1] = 0;
    
        DPRINTF(DBG_DRIVER, (DBGFMT "Tx cookie %p, %p.\n", DBGARG, cookie, buff));
        
        SET_HTC_PACKET_INFO_TX(&cookie->HtcPkt,
                               cookie,
                               A_NETBUF_DATA(buff),
                               A_NETBUF_LEN(buff),
                               eid,
                               AR6K_DATA_PKT_TAG);
        /* HTC interface is asynchronous, if this fails, cleanup will happen in
         * the qcom_tx_complete callback */
        HTCSendPkt(ar->arHtcTarget, &cookie->HtcPkt);
    }
    else {
        /* no packet to send, cleanup */
        A_NETBUF_FREE(buff);
        AR6000_STAT_INC(ar, tx_dropped);
        AR6000_STAT_INC(ar, tx_aborted_errors);        
    }

	return(0);
}
