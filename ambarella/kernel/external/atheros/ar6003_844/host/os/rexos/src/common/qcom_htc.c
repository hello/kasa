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
#endif

/* Atheros include files */
#include <athdefs.h>
#include <a_types.h>
#include <a_osapi.h>
#include <a_hif.h>
#include <wlan_api.h>
#include <htc.h>
#include <htc_api.h>
#include <htc_packet.h>
#include <wmi.h>
#include <wmi_api.h>
#include <bmi.h>
#include <a_drv_api.h>

#include <common_drv.h>
#include <targaddrs.h>

#include <ar6000_diag.h>
#include <AR6002/AR6K_version.h>

/* Qualcomm include files */
#ifdef ATHR_EMULATION
#include "qcom_stubs.h"
#endif
#include "wlan_dot11_sys.h"
#include "wlan_adp_def.h"
#include "wlan_adp_oem.h"
#include "wlan_trp_oem.h"
#include "wlan_adp_qos.h"
#include "wlan_trp.h"
#include "wlan_util.h"
#include "dsm.h"
#include "rex.h"

/* Atheros platform include files */
#include "wlan_oem_ath.h"
#include "athdrv_rexos.h"
#include "qcom_htc.h"
#include "ieee80211.h"
#include "qcom_firmware.h"
#include "hif.h"
#include "a_debug.h"
#include "dbglog_api.h"
#include "targaddrs.h"
#include "targaddrs_rexos.h"

/* Define this to disable sleep */
#undef DISABLE_SLEEP

#define MAX_DATA_QUEUE_SIZE		5

#define FLAG_TX_READY			0x00000001
#define FLAG_CMD_READY			0x00000002
#define FLAG_FIRMWARE_UPLOAD	0x00000004
#ifdef CONFIG_HOST_TCMD_SUPPORT
#define FLAG_TEST_MODE			0x00000008
#endif

#define HTC_STATE_START				1
#define HTC_STATE_CONFIG			2
#define HTC_STATE_OPEN				3
#define HTC_STATE_DROP				4
#define HTC_STATE_IDLE				5
#define HTC_STATE_TEST 				6
#define HTC_STATE_TEST_OPEN			7

static A_MUTEX_T s_flags_lock;
static unsigned int s_flags = 0;
static unsigned int s_rssi = 0;
static unsigned int s_failure = 0; 
static AR_SOFTC_T s_ar;
static A_TIMER s_command_timeout_timer;
static unsigned int s_timeout_command = 0;
static void stop_command_timeout(void);
static void start_command_timeout(unsigned int cmd, unsigned int ms);
static void process_command_timeout(void *arg);

#define UNLOCK_COMMAND	\
	do {							\
	stop_command_timeout();         \
	A_MUTEX_LOCK(&s_flags_lock);	\
	s_flags |= FLAG_CMD_READY;		\
	A_MUTEX_UNLOCK(&s_flags_lock);	\
	} while(0)

#define LOCK_COMMAND	\
	do {							\
	A_MUTEX_LOCK(&s_flags_lock);	\
	s_flags &= ~FLAG_CMD_READY;		\
	A_MUTEX_UNLOCK(&s_flags_lock);	\
	} while(0)

#define UNLOCK_DATA	\
	do {							\
	/* A_MUTEX_LOCK(&s_flags_lock);	*/ \
	s_flags |= FLAG_TX_READY;		\
	/* A_MUTEX_UNLOCK(&s_flags_lock); */	\
	} while(0)

#define LOCK_DATA	\
	do {							\
	/* A_MUTEX_LOCK(&s_flags_lock); */	\
	s_flags &= ~FLAG_TX_READY;		\
	/* A_MUTEX_UNLOCK(&s_flags_lock); */	\
	} while(0)

extern rex_tcb_type*     htc_task_ptr;
extern int tspecCompliance;

static void set_state(int state);

#define NUM_HTC_REQUESTS	16
static struct htc_request s_htc_requests[NUM_HTC_REQUESTS];

static A_MUTEX_T s_htc_request_free_list_lock;
static A_MUTEX_T s_htc_request_queue_lock;
static A_MUTEX_T s_htc_data_queue_lock;

static struct htc_request *s_htc_request_free_list = NULL;

static struct htc_request *s_htc_request_queue_tail = NULL;
static struct htc_request *s_htc_request_queue_head = NULL;

static unsigned int s_status = 0;
static void *s_status_data[NUM_STATUS_SLOTS];

/* Save the device here */
static AR_SOFTC_T *s_device = NULL;

static wlan_adp_scan_req_info_type s_scan_filter;
static wlan_adp_join_req_info_type s_join_info;
static wlan_adp_auth_req_info_type s_auth_info;
static wlan_adp_assoc_req_info_type s_assoc_info;
static wlan_adp_wpa_protect_list_type s_protect_info;
static wlan_dot11_auth_algo_type_enum s_auth_algo;
static wlan_adp_wmm_info_type *s_wmm_info_p = NULL;

typedef enum {
    QCOM_Q_BK = 0,
    QCOM_Q_BE,
    QCOM_Q_VI,
    QCOM_Q_VO,
    QCOM_Q_BT
} QCOM_Q_ID;

static struct sid_info
{
	A_NETBUF_QUEUE_T queue;
}
s_sid_table[] =
{
	{ { 0 } },
	{ { 0 } },
	{ { 0 } },
	{ { 0 } },
	{ { 0 } },
};

#define NUM_SID_ENTRIES	(sizeof(s_sid_table) / sizeof(struct sid_info))

typedef struct
{
    A_UINT8   skipflash:1;
    A_UINT8   enableuartprint:1;
    A_UINT8   enabledbglog:1;
    A_UINT8   enabletimerwar:1;
    A_UINT8   setmacaddr:1;
    A_UINT8   reserved:3;
} miscFlagsStruct;

#ifdef DEBUG
static miscFlagsStruct miscFlags = { 0, 1, 1, 1, 0 };
#else
static miscFlagsStruct miscFlags = { 0, 0, 1, 1, 0 };
#endif

static unsigned int mbox_yield_limit = 99;
static unsigned int ar6000_clk_freq = 26000000; /* 26 MHz to 40 MHz */
static unsigned int wlan_node_age = 120000;
static unsigned int bt_ant_config = 0;

static char* fw_files[] = { 
    "athwlan1_0.bin",
    "athwlan2_0.bin.z77",
    "athtcmd1_0.bin",
    "athtcmd2_0.bin" };

extern int bypasswmi;

/*
 * WMM_AC_BE (UP 0,3) on HTC service mapped to QCOM_Q_BE
 * WMM_AC_BK (UP 1,2) on HTC service mapped to QCOM_Q_BK
 * WMM_AC_VI (UP 4,5) on HTC service mapped to QCOM_Q_VI
 * WMM_AC_VO (UP 6,7) on HTC service mapped to QCOM_Q_VO
 */
/* TODO : change mapping below once target f/w is fixed */
static QCOM_Q_ID s_up2qid_table[MAX_NUM_PRI] =
{
	QCOM_Q_BE,
	QCOM_Q_BK,
    QCOM_Q_BK,
    QCOM_Q_BE,
    QCOM_Q_VI,
    QCOM_Q_VI,
	QCOM_Q_VO,
	QCOM_Q_VO
};

#define AR6K_DBG_BUFFER_SIZE    1500
#define AR6K_DBG_FILE_SIZE      (sizeof(dbg_rec) * 30) /* 45K is a maximum size of dbglog */
#define AR6K_DBG_MIN_FILE_SIZE  (sizeof(dbg_rec) * 15)

static struct {
    A_UINT32 ts;
    A_UINT32 length;
    A_UCHAR  log[AR6K_DBG_BUFFER_SIZE];
} dbg_rec;

void* dbglog_fl_h = FL_NULL;
unsigned int dbglog_fl_sz = AR6K_DBG_FILE_SIZE;
unsigned int dbglog_fl_cursz = 0;
unsigned int processDot11Hdr = 0;

A_CHAR eepromFile[] = "\0";
A_UINT8 s_macaddress[ATH_MAC_LEN] = { 0x0, 0x03, 0x7f, 0x3, 0x4, 0x5 };
 
/*****************************************************************************/
/*****************************************************************************/
/**                                                                         **/
/** Local Routines                                                          **/
/**                                                                         **/
/** This section contains local routines used in this module                **/
/**                                                                         **/
/*****************************************************************************/
/*****************************************************************************/

static int up2qid(int up)
{
    A_ASSERT(up < MAX_NUM_PRI);
	return(s_up2qid_table[up & 0x7]);
}

static void enqueue_htc_request(struct htc_request *req)
{
	/* Put last in list */

	A_MUTEX_LOCK(&s_htc_request_queue_lock);

	/* Is list empty */
	if(s_htc_request_queue_head == NULL)
	{
		/* Put first in list */
		s_htc_request_queue_head = req;
	}
	else
	{
		/* Put last in list */
		s_htc_request_queue_tail->next = req;
	}
	
	/* Remember the tail */
	s_htc_request_queue_tail = req;
	
	A_MUTEX_UNLOCK(&s_htc_request_queue_lock);

	return;
}

static struct htc_request *dequeue_htc_request(void)
{
	struct htc_request *req;

	A_MUTEX_LOCK(&s_htc_request_queue_lock);

	/* Is list empty */
	if(s_htc_request_queue_head == NULL)
	{
		/* Nothing to be done here */
		A_MUTEX_UNLOCK(&s_htc_request_queue_lock);

		return(NULL);
	}
	else
	{
		/* Get first element */
		req = s_htc_request_queue_head;

		/* Advance the list */	
		s_htc_request_queue_head = req->next;

		/* Adjust tail pointer if list becomes empty */
		if(s_htc_request_queue_head == NULL)
		{
			s_htc_request_queue_tail = NULL;
		}
	}
	
	A_MUTEX_UNLOCK(&s_htc_request_queue_lock);

	return(req);
}

static void enqueue_htc_data(void *pkt, QCOM_Q_ID qId)
{
	A_NETBUF_QUEUE_T *q;

	DPRINTF(DBG_HTC, (DBGFMT "Enter pkt 0x%08x queue ID %d\n", DBGARG,
		(unsigned int) pkt, qId));
    
	/* Get our queue for this end point */
	q = &s_sid_table[qId].queue;

	/* Queue the packet on the queue */
	A_NETBUF_ENQUEUE(q, pkt);
}

static void prequeue_htc_data(void *pkt, QCOM_Q_ID qId)
{
	A_NETBUF_QUEUE_T *q;

	DPRINTF(DBG_HTC, (DBGFMT "Enter pkt 0x%08x queue ID %d\n", DBGARG,
		(unsigned int) pkt, qId));

	/* Get our queue for this end point */
	q = &s_sid_table[qId].queue;

	/* Pre-queue the packet on the queue */
	A_NETBUF_PREQUEUE(q, pkt);
}

static void *dequeue_htc_data(QCOM_Q_ID *out_qid)
{
	void *pkt = NULL;
	A_INT32 qId;

	DPRINTF(DBG_HTC, (DBGFMT "Enter\n", DBGARG));

	/* Go through the data mail boxes. Always start with 
       QCOM_Q_BT queue */
	for (qId = QCOM_Q_BT; qId >= QCOM_Q_BK; --qId)
	{
		DPRINTF(DBG_HTC,
			(DBGFMT "qId %d \n", DBGARG, qId));

		/* Dequeue the packet from the queue */
        pkt = A_NETBUF_DEQUEUE(&s_sid_table[qId].queue);
		if(pkt != NULL)
		{
			DPRINTF(DBG_HTC, (DBGFMT "Found\n", DBGARG));
			break;
		}
		else
		{
			DPRINTF(DBG_HTC, (DBGFMT "Empty\n", DBGARG));
		}
	}

	/* Let the caller know which qId we pulled this from */
	*out_qid = qId;

	if(pkt != NULL)
	{
		DPRINTF(DBG_HTC,
			(DBGFMT "Found pkt 0x%08x qId %d\n", DBGARG,
			(unsigned int) pkt, *out_qid));
	}
	
	return(pkt);
}

static int len_htc_data(QCOM_Q_ID qId)
{
	A_NETBUF_QUEUE_T *q;

	DPRINTF(DBG_HTC, (DBGFMT "Enter queue ID %d\n", DBGARG, qId));

    A_ASSERT(qId >= QCOM_Q_BK);

	/* Get our queue for this end point */
	q = &s_sid_table[qId].queue;
	
	return(A_NETBUF_QUEUE_SIZE(q));
}


static int adp_pull_data(void) 
{
	wlan_adp_tx_pkt_meta_info_type meta_info;
	int len;
	int pdu_len;
	char *p;
	void *pkt;
	A_UINT8 dstaddr[ATH_MAC_LEN]; 
	dsm_item_type* dsm_pkt = NULL;
	QCOM_Q_ID qId;

	DPRINTF(DBG_HTC,
		(DBGFMT "Enter.\n", DBGARG));

	if((pdu_len = wlan_adp_oem_get_next_tx_pkt(&dsm_pkt, &meta_info)) <= 0)
	{
		DPRINTF(DBG_HTC,
			(DBGFMT "No more packet from ADP layer %d.\n", DBGARG, pdu_len));
		return(0);
	}

	/* Convert this user priority to WMI stream id */
	qId = up2qid(meta_info.tid);

	/* How big is it */
	len = pdu_len + sizeof(ATH_MAC_HDR);

	if((pkt = A_NETBUF_ALLOC(len + sizeof(meta_info.tid))) == NULL)
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
	wlan_util_uint64_to_macaddress(dstaddr, meta_info.dst_mac_addr);
	A_MEMCPY(p, dstaddr, ATH_MAC_LEN);
	A_MEMCPY(p + ATH_MAC_LEN, s_macaddress, ATH_MAC_LEN);
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

	/* Add the data */
	if(dsm_pullup(&dsm_pkt, p, pdu_len) != pdu_len)
	{
		DPRINTF(DBG_WLAN_ATH,
			(DBGFMT "Failed to pullup DSM Item.\n", DBGARG));
		A_ASSERT(0);
	}

    /* save tid of this packet so that HTC task can use it later */
    A_MEMCPY(p + pdu_len, &meta_info.tid, sizeof(meta_info.tid));

	dsm_free_packet(&dsm_pkt);

	/* Enqueue the packet */
	enqueue_htc_data(pkt, qId);

	return(1);
}

static unsigned int htc_is_connected(AR_SOFTC_T *ar)
{
	return(ar->arConnected);
}

static void qcom_detect_error(unsigned long ptr)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *) ptr;

    A_MUTEX_LOCK(&ar->arLock);

    if (ar->arHBChallengeResp.outstanding) {
        ar->arHBChallengeResp.missCnt++;
    } else {
        ar->arHBChallengeResp.missCnt = 0;
    }

    if (ar->arHBChallengeResp.missCnt > ar->arHBChallengeResp.missThres) {
        /* Send Error Detect event to the application layer and do not 
         * reschedule the error detection module timer */
        ar->arHBChallengeResp.missCnt = 0;
        ar->arHBChallengeResp.seqNum = 0;
        
        A_MUTEX_UNLOCK(&ar->arLock);
        
        /* Signal the htc task */
        htc_task_signal(HTC_STOP_SIG);
        return;
    }

    /* Generate the sequence number for the next challenge */
    ar->arHBChallengeResp.seqNum++;
    ar->arHBChallengeResp.outstanding = TRUE;
    
    A_MUTEX_UNLOCK(&ar->arLock);
    
    /* Send the challenge on the control channel */
    if (wmi_get_challenge_resp_cmd(ar->arWmi, ar->arHBChallengeResp.seqNum,
                                   DRV_HB_CHALLENGE) != A_OK) {
        DPRINTF(DBG_HTC,
            (DBGFMT "Unable to send heart beat challenge\n", DBGARG));
    }

    /* Reschedule the timer for the next challenge */
    A_TIMEOUT_MS(&ar->arHBChallengeResp.timer, ar->arHBChallengeResp.frequency * 1000, 0);
}

void
qcom_init_profile_info(AR_SOFTC_T *ar)
{
    ar->arSsidLen            = 0;
    A_MEMZERO(ar->arSsid, sizeof(ar->arSsid));
    ar->arNetworkType        = INFRA_NETWORK;
    ar->arDot11AuthMode      = OPEN_AUTH;
    ar->arAuthMode           = NONE_AUTH;
    ar->arPairwiseCrypto     = NONE_CRYPT;
    ar->arPairwiseCryptoLen  = 0;
    ar->arGroupCrypto        = NONE_CRYPT;
    ar->arGroupCryptoLen     = 0;
    A_MEMZERO(ar->arWepKeyList, sizeof(ar->arWepKeyList));
    A_MEMZERO(ar->arBssid, sizeof(ar->arBssid));
    ar->arBssChannel = 0;
}

static void qcom_init_control_info(AR_SOFTC_T *ar)
{
	A_MEMZERO(ar, sizeof(AR_SOFTC_T));

    qcom_init_profile_info(ar);
	ar->arDefTxKeyIndex      = 0;
	A_MEMZERO(ar->arWepKeyList, sizeof(ar->arWepKeyList));
	ar->arChannelHint        = 0;
	ar->arListenInterval     = MAX_LISTEN_INTERVAL;
	ar->arRssi               = 0;
	ar->arTxPwr              = 0;
	ar->arTxPwrSet           = FALSE;
    ar->arBitRate            = 0;
    ar->arMaxRetries         = 0;
    ar->arConnectPending     = TRUE;
    ar->arWmmEnabled         = TRUE;
}

A_STATUS
qcom_set_host_app_area(void *arg)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *) arg;
    A_UINT32 address, data;
    struct host_app_area_s host_app_area;

    /* Fetch the address of the host_app_area_s instance in the host interest area */
    address = HOST_INTEREST_ITEM_ADDRESS(ar->arTargetType, hi_app_host_interest);
    if (ar6000_ReadRegDiag(ar->arHifDevice, &address, &data) != A_OK) {
        return A_ERROR;
    }
    address = data;
    host_app_area.wmi_protocol_ver = WMI_PROTOCOL_VERSION;
    if (ar6000_WriteDataDiag(ar->arHifDevice, address, 
                             (A_UCHAR *)&host_app_area, 
                             sizeof(struct host_app_area_s)) != A_OK)
    {
        return A_ERROR;
    }

    return A_OK;
}

A_UINT32
dbglog_get_debug_hdr_ptr(AR_SOFTC_T *ar)
{
    A_UINT32 param;
    A_UINT32 address;
    A_STATUS status;

    address = HOST_INTEREST_ITEM_ADDRESS(ar->arTargetType, hi_dbglog_hdr);
    if ((status = ar6000_ReadDataDiag(ar->arHifDevice, address, 
                                      (A_UCHAR *)&param, 4)) != A_OK)
    {
        param = 0;
    }

    return param;
}

void
dbglog_parse_debug_logs(A_UINT8 *datap, A_UINT32 len, A_BOOL closeFile)
{
#undef DBGLOG_TO_CONSOLE
#ifdef DBGLOG_TO_CONSOLE
    A_INT32 *buffer;
    A_UINT32 count;
    A_UINT32 timestamp;
    A_UINT32 debugid;
    A_UINT32 moduleid;
    A_UINT32 numargs;
    A_UINT32 length;

    count = 0;
    buffer = (A_INT32 *)datap;
    length = (len >> 2);
    while (count < length) {
        debugid = DBGLOG_GET_DBGID(buffer[count]);
        moduleid = DBGLOG_GET_MODULEID(buffer[count]);
        numargs = DBGLOG_GET_NUMARGS(buffer[count]);
        timestamp = DBGLOG_GET_TIMESTAMP(buffer[count]);
        switch (numargs) {
            case 0:
            DPRINTF(DBG_LOG, (DBGFMT "%d %d (%d)\n", DBGARG, moduleid, debugid, timestamp));
            break;

            case 1:
            DPRINTF(DBG_LOG, (DBGFMT "%d %d (%d): 0x%x\n", DBGARG, moduleid, debugid, 
                    timestamp, buffer[count+1]));
            break;

            case 2:
            DPRINTF(DBG_LOG, (DBGFMT "%d %d (%d): 0x%x, 0x%x\n", DBGARG, moduleid, debugid, 
                    timestamp, buffer[count+1], buffer[count+2]));
            break;

            default:
            DPRINTF(DBG_LOG, (DBGFMT "Invalid args: %d\n", DBGARG, numargs));
        }
        count += numargs + 1;
    }
#else /* LOG_TO_CONSOLE */

    if ((dbglog_fl_h != FL_NULL) && len && (len <= AR6K_DBG_BUFFER_SIZE))
    {
        if ( (dbglog_fl_cursz + sizeof(dbg_rec)) > dbglog_fl_sz)
        {
            /* rewind the file pointer to overwrite existing conetent */
            dbglog_fl_cursz = 0;
        }

        dbg_rec.length = len;
        dbg_rec.ts = A_GET_MS(0);
        A_MEMZERO(dbg_rec.log, sizeof(dbg_rec.log));
        A_MEMCPY(dbg_rec.log, datap, len);
        A_FL_SET_DATA(dbglog_fl_h, 0, (char*) &dbg_rec, sizeof(dbg_rec));
        dbglog_fl_cursz += sizeof(dbg_rec);
        
        if (closeFile == TRUE) {
            A_FL_UNINITIALIZE(dbglog_fl_h);
            dbglog_fl_h = NULL;
        }
    }
#endif
}

int
qcom_dbglog_get_debug_logs(void* arg, A_BOOL closeFile)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *) arg;
    struct dbglog_hdr_s debug_hdr;
    struct dbglog_buf_s debug_buf;
    A_UINT32 address;
    A_UINT32 length;
    A_UINT32 dropped;
    A_UINT32 firstbuf;
    A_UINT32 debug_hdr_ptr;

    if (!ar->dbglog_init_done) return A_ERROR;

    if (!miscFlags.enabledbglog) {
    	ar->dbgLogFetchInProgress = FALSE;
    	return A_OK;
    }
    	
    A_MUTEX_LOCK(&ar->arLock);
    
    if (ar->dbgLogFetchInProgress) {
        A_MUTEX_UNLOCK(&ar->arLock);
        return A_EBUSY;
    }
    
    /* block out others */
    ar->dbgLogFetchInProgress = TRUE;
        
    A_MUTEX_UNLOCK(&ar->arLock);
    
    debug_hdr_ptr = dbglog_get_debug_hdr_ptr(ar);
    DPRINTF(DBG_LOG, (DBGFMT "deubg_hdr_ptr: 0x%x\n", DBGARG, debug_hdr_ptr));

    /* Get the contents of the ring buffer */
    if (debug_hdr_ptr) {
        address = debug_hdr_ptr;
        length = sizeof(struct dbglog_hdr_s);
        ar6000_ReadDataDiag(ar->arHifDevice, address, 
                            (A_UCHAR *)&debug_hdr, length);
        address = (A_UINT32)debug_hdr.dbuf;
        firstbuf = address;
        dropped = debug_hdr.dropped;
        length = sizeof(struct dbglog_buf_s);
        ar6000_ReadDataDiag(ar->arHifDevice, address, 
                            (A_UCHAR *)&debug_buf, length);

        do {
            address = (A_UINT32)debug_buf.buffer;
            length = debug_buf.length;
            if ((length) && (debug_buf.length <= debug_buf.bufsize)) {
                /* Rewind the index if it is about to overrun the buffer */
                if (ar->log_cnt > (DBGLOG_HOST_LOG_BUFFER_SIZE - length)) {
                    ar->log_cnt = 0;
                }
                if(A_OK != ar6000_ReadDataDiag(ar->arHifDevice, address, 
                                    (A_UCHAR *)&ar->log_buffer[ar->log_cnt], length))
                {
                    break;
                }
                dbglog_parse_debug_logs((A_UINT8 *) &ar->log_buffer[ar->log_cnt], length, closeFile);
                ar->log_cnt += length;
            } else {
                DPRINTF(DBG_LOG, (DBGFMT "Length: %d (Total size: %d)\n", 
                        DBGARG, debug_buf.length, debug_buf.bufsize));
            }
    
            address = (A_UINT32)debug_buf.next;
            length = sizeof(struct dbglog_buf_s);
            if(A_OK != ar6000_ReadDataDiag(ar->arHifDevice, address, 
                    (A_UCHAR *)&debug_buf, length))
            {
                break;
            }
        } while (address != firstbuf);
    }

    ar->dbgLogFetchInProgress = FALSE;

    return A_OK;
}

static A_STATUS
wlan_set_snr_threshold(AR_SOFTC_T *ar, WMI_SNR_THRESHOLD_PARAMS_CMD* cmd)
{
    DPRINTF(DBG_HTC, (DBGFMT "Enter\n", DBGARG));

    if (ar->arWmiReady == FALSE) {
        DPRINTF(DBG_HTC, (DBGFMT "WMI is not ready\n", DBGARG));
        return A_ERROR;
    }

    return wmi_set_snr_threshold_params(ar->arWmi, cmd);
}

static int
wlan_set_rssi_threshold(AR_SOFTC_T *ar, USER_RSSI_PARAMS *rssiParams)
{
    A_INT32 i, j;
    WMI_RSSI_THRESHOLD_PARAMS_CMD cmd;

#define SWAP_THOLD(thold1, thold2) do { \
    USER_RSSI_THOLD tmpThold;           \
    tmpThold.tag = thold1.tag;          \
    tmpThold.rssi = thold1.rssi;        \
    thold1.tag = thold2.tag;            \
    thold1.rssi = thold2.rssi;          \
    thold2.tag = tmpThold.tag;          \
    thold2.rssi = tmpThold.rssi;        \
} while (0)

    DPRINTF(DBG_HTC, (DBGFMT "Enter\n", DBGARG));
    
    if (ar->arWmiReady == FALSE) {
        DPRINTF(DBG_HTC, (DBGFMT "WMI is not ready\n", DBGARG));
        return A_ERROR;
    }

    cmd.weight = rssiParams->weight;
    cmd.pollTime = rssiParams->pollTime;

    A_MEMCPY(ar->rssi_map, &rssiParams->tholds, sizeof(ar->rssi_map));
    /*
     *  only 6 elements, so use bubble sorting, in ascending order
     */
    for (i = 5; i > 0; i--) {
        for (j = 0; j < i; j++) { /* above tholds */
            if (ar->rssi_map[j+1].rssi < ar->rssi_map[j].rssi) {
                SWAP_THOLD(ar->rssi_map[j+1], ar->rssi_map[j]);
            } else if (ar->rssi_map[j+1].rssi == ar->rssi_map[j].rssi) {
                return A_ERROR;
            }
        }
    }
    for (i = 11; i > 6; i--) {
        for (j = 6; j < i; j++) { /* below tholds */
            if (ar->rssi_map[j+1].rssi < ar->rssi_map[j].rssi) {
                SWAP_THOLD(ar->rssi_map[j+1], ar->rssi_map[j]);
            } else if (ar->rssi_map[j+1].rssi == ar->rssi_map[j].rssi) {
                return A_ERROR;
            }
        }
    }

#ifdef DEBUG
    for (i = 0; i < 12; i++) {
        DPRINTF(DBG_HTC, (DBGFMT "thold[%d].tag: %d, thold[%d].rssi: %d \n", 
                DBGARG, i, ar->rssi_map[i].tag, i, ar->rssi_map[i].rssi));
    }
#endif

    cmd.thresholdAbove1_Val = ar->rssi_map[0].rssi;
    cmd.thresholdAbove2_Val = ar->rssi_map[1].rssi;
    cmd.thresholdAbove3_Val = ar->rssi_map[2].rssi;
    cmd.thresholdAbove4_Val = ar->rssi_map[3].rssi;
    cmd.thresholdAbove5_Val = ar->rssi_map[4].rssi;
    cmd.thresholdAbove6_Val = ar->rssi_map[5].rssi;
    cmd.thresholdBelow1_Val = ar->rssi_map[6].rssi;
    cmd.thresholdBelow2_Val = ar->rssi_map[7].rssi;
    cmd.thresholdBelow3_Val = ar->rssi_map[8].rssi;
    cmd.thresholdBelow4_Val = ar->rssi_map[9].rssi;
    cmd.thresholdBelow5_Val = ar->rssi_map[10].rssi;
    cmd.thresholdBelow6_Val = ar->rssi_map[11].rssi;

    return wmi_set_rssi_threshold_params(ar->arWmi, &cmd);
}

/*****************************************************************************/
/*****************************************************************************/
/**                                                                         **/
/** HTC event routines                                                      **/
/**                                                                         **/
/** This section contains the event routines that are registered with       **/
/** the HTC event mechanism                                                 **/
/**                                                                         **/
/*****************************************************************************/
/*****************************************************************************/

A_STATUS
qcom_avail_ev(void *context, void *hif_handle)
{
	AR_SOFTC_T *ar;
    struct bmi_target_info targ_info;
	int status;
	int upload;
    A_UINT32 param;
    char* filename = NULL;
    A_BOOL bcompressed = FALSE;
    HTC_INIT_INFO  htcInfo;
    
	DPRINTF(DBG_HTC, (DBGFMT "Enter\n", DBGARG));

	A_ASSERT(hif_handle != NULL);

	if(s_device != NULL)
	{
		DPRINTF(DBG_HTC, (DBGFMT "Only one device supported\n", DBGARG));
		return A_ERROR;
	}

	/* The control structure for this device is statically allocated */
	ar = &s_ar;

	/* Initialize the structure */
	qcom_init_control_info(ar);

	ar->arHifDevice          = hif_handle;
	ar->arWlanState			 = WLAN_ENABLED;
	
	A_MUTEX_INIT(&ar->arLock);
    A_INIT_TIMER(&ar->arHBChallengeResp.timer, qcom_detect_error, ar);
    ar->arHBChallengeResp.seqNum = 0;
    ar->arHBChallengeResp.outstanding = FALSE;
    ar->arHBChallengeResp.missCnt = 0;
    ar->arHBChallengeResp.frequency = AR6000_HB_CHALLENGE_RESP_FREQ_DEFAULT;
    ar->arHBChallengeResp.missThres = AR6000_HB_CHALLENGE_RESP_MISS_THRES_DEFAULT;

	/* Save the device */
	s_device = ar;

	DPRINTF(DBG_HTC, (DBGFMT "HIF target 0x%08x ar 0x%08x\n", DBGARG,
		(A_UINT32) hif_handle, (A_UINT32) ar));

start:

    /* reset the target */
    if (ar6000_reset_device(ar->arHifDevice, TARGET_TYPE_AR6002, TRUE, TRUE) != A_OK)
    {
        DPRINTF(DBG_HTC,(DBGFMT" Failed to reset target\n",DBGARG));
    }
    	
    /*
     * If requested, perform some magic which requires no cooperation from
     * the Target.  It causes the Target to ignore flash and execute the
     * OS from ROM.
     *
     * This code uses the Diagnostic Window to remap instructions at
     * the start of ROM in such a way that on the next CPU reset, the
     * ROM code avoids using flash.  Then it uses the Diagnostic
     * Window to force a CPU Warm Reset.
     *
     * This is intended to support recovery from a corrupted flash.
     */
    if (miscFlags.skipflash)
    {
        miscFlags.skipflash = 0;
	    DPRINTF(DBG_HTC,(DBGFMT" Skip flash feature not supported\n",DBGARG));
	    return A_ERROR;
    }
    
	/* Do BMI stuff */
	BMIInit();
	if((BMIGetTargetInfo(ar->arHifDevice, &targ_info)) != A_OK)
	{
		DPRINTF(DBG_HTC, (DBGFMT "BMIGetTargetInfo() failed\n", DBGARG));
        goto failure;
	}
    ar->arVersion.target_ver = targ_info.target_ver;
    ar->arTargetType = targ_info.target_type;

	DPRINTF(DBG_HTC, (DBGFMT "Got BMI target ID 0x%08x Type %d\n", DBGARG,
		    targ_info.target_ver, targ_info.target_type));

    /* do any target-specific preparation that can be done through BMI */
    if (ar6000_prepare_target(ar->arHifDevice,
                              targ_info.target_type,
                              targ_info.target_ver) != A_OK) {
        return A_ERROR;
    }

    if (TARGET_TYPE_AR6002 == ar->arTargetType)
    {
        #ifdef CONFIG_HOST_TCMD_SUPPORT
        A_MUTEX_LOCK(&s_flags_lock);
        status = s_flags & FLAG_TEST_MODE ? TRUE : FALSE;
        A_MUTEX_UNLOCK(&s_flags_lock);

        if(status)
        {
            if (AR6002_VERSION_REV2 == ar->arVersion.target_ver)
                filename = fw_files[3];
            else {
                DPRINTF(DBG_HTC, (DBGFMT "Target version 0x%08x Type %d not "
                        "supported\n", DBGARG, targ_info.target_ver,
                        targ_info.target_type));
                return A_ERROR;
            }
        }
        else {
        #endif
            if (AR6002_VERSION_REV2 == ar->arVersion.target_ver) {
                filename = fw_files[1];
                bcompressed = TRUE;
            } else {
                DPRINTF(DBG_HTC, (DBGFMT "Target version 0x%08x Type %d not "
                        "supported\n", DBGARG, targ_info.target_ver,
                        targ_info.target_type));
                return A_ERROR;
            }
        #ifdef CONFIG_HOST_TCMD_SUPPORT
        }
        #endif
        
        if (miscFlags.setmacaddr) {
            ar6000_set_softmac_addr(s_macaddress);
        }
    }
    else if (TARGET_TYPE_AR6001 == ar->arTargetType) {
        // No need to reserve RAM space for patch as olca/dragon is flash based
        param = 0;
        if (BMIWriteMemory(ar->arHifDevice,
            HOST_INTEREST_ITEM_ADDRESS(ar->arTargetType, hi_end_RAM_reserve_sz),
                           (A_UCHAR *)&param,
                           4)!= A_OK)
        {
            DPRINTF(DBG_HTC, (DBGFMT
                    "BMIWriteMemory for hi_end_RAM_reserve_sz failed \n", DBGARG));
             return A_ERROR;
        }
    }

    if (A_OK != configure_ar6000(ar->arHifDevice, ar->arTargetType, 
                                 ar->arVersion.target_ver, miscFlags.enableuartprint,
                                 miscFlags.enabletimerwar, ar6000_clk_freq,
                                 filename, "Images/", bcompressed,
                                 TRUE, eepromFile)) {
        DPRINTF(DBG_HTC, (DBGFMT "configure_ar6000() failed \n", DBGARG));
        goto failure;
    }
        
    /* since BMIInit is called in the driver layer, we have to set the block 
     * size here for the target */
    if (A_FAILED(ar6000_set_htc_params(ar->arHifDevice, 
                                       ar->arTargetType,
                                       mbox_yield_limit,
                                       0 /* use default number of control buffers */
                                       ))) {
        goto failure;
    }
    
    A_MEMZERO(&htcInfo,sizeof(htcInfo));
    htcInfo.pContext = ar;
    htcInfo.TargetFailure = qcom_target_failure;

    ar->arHtcTarget = HTCCreate(ar->arHifDevice,&htcInfo);

    if (ar->arHtcTarget == NULL) {
    	DPRINTF(DBG_HTC, (DBGFMT "HTCCreate() failed \n", DBGARG));
        return A_ERROR;
    }
    
    if (TARGET_TYPE_AR6001 == ar->arTargetType)
    {
	A_MUTEX_LOCK(&s_flags_lock);
	upload = s_flags & FLAG_FIRMWARE_UPLOAD ? TRUE : FALSE;
	A_MUTEX_UNLOCK(&s_flags_lock);
		
	if(upload == TRUE)
	{
		/* Do it ... Keep the flag set in case we need it ...*/
		DPRINTF(DBG_HTC, (DBGFMT "Do Firware Upload.\n", DBGARG));

		status = qcom_fw_upload(ar);

		/* Clear flag */
		A_MUTEX_LOCK(&s_flags_lock);
		s_flags &= ~FLAG_FIRMWARE_UPLOAD;
		A_MUTEX_UNLOCK(&s_flags_lock);

		/* Evaluate success or failure */
		if(status < 0)
		{
                goto failure;
		}

		/* Start over */
		goto start;
	}

#ifdef CONFIG_HOST_TCMD_SUPPORT
	/*If in test mode, set the target in test mode by enabling test bit
	  in scratch register 0 via BMI */

	A_MUTEX_LOCK(&s_flags_lock);
	status = s_flags & FLAG_TEST_MODE ? TRUE : FALSE;
	A_MUTEX_UNLOCK(&s_flags_lock);

	if(status)
	{
		/* Do it ... Keep the flag set in case we need it ...*/
		DPRINTF(DBG_HTC, (DBGFMT "Do Test Firware Upload.\n", DBGARG));

		status = qcom_test_fw_upload(ar);

		/* Do BMI stuff */
		BMIInit();
		
		/* Evaluate success or failure */
		if(status < 0)
		{
                goto failure;
		}
	}
#endif

	/* Make sure we run the right matching software */
	if(ar->arVersion.target_ver != AR6K_SW_VERSION)
	{
		DPRINTF(DBG_HTC|DBG_WARNING,
			(DBGFMT "ERROR - SW version mismatch - target 0x%08x host 0x%08x\n", DBGARG,
			ar->arVersion.target_ver, AR6K_SW_VERSION));
            goto failure;
        }
    }
    
    return A_OK;

failure:
    /* Clear the device */
    s_device = NULL;
    htc_set_failure(FAIL_FATAL);
    return A_ERROR;
}

A_STATUS
qcom_unavail_ev(void *context, void *hif_handle)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *) context;

	DPRINTF(DBG_HTC, (DBGFMT "Enter\n", DBGARG));

    /* Delete the pending timer and start a new one */
    A_UNTIMEOUT(&ar->arHBChallengeResp.timer);
    
	/* Clear the device */
	s_device = NULL;
	return A_OK;
}

void
qcom_target_failure(void *Instance, A_STATUS Status)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)Instance;

    DPRINTF(DBG_HTC, (DBGFMT "Enter\n", DBGARG));
    
    if (Status != A_OK) {
        if (A_TIMER_PENDING(&ar->arHBChallengeResp.timer)) {
            A_UNTIMEOUT(&ar->arHBChallengeResp.timer);
        }

        /* try dumping target assertion information (if any) */
        ar6000_dump_target_assert_info(ar->arHifDevice,ar->arTargetType);

        /* Report the error only once */
        htc_set_failure(FAIL_FATAL);
    }
}

void
qcom_tx_complete(void *Context, HTC_PACKET *pPacket)
{
	AR_SOFTC_T *ar = (AR_SOFTC_T *) Context;
	void *cookie = (void *) pPacket->pPktContext;
    struct ar_cookie * ar_cookie = (struct ar_cookie *)cookie;
	void *pkt, *pkt_data;
    A_STATUS status;
    HTC_ENDPOINT_ID   eid;
	A_UINT32 mapNo = 0;
    A_UINT32 pktlen;
    A_UINT8 up;
    WMI_DATA_HDR *dtHdr;
    
	DPRINTF(DBG_HTC, (DBGFMT "Enter\n", DBGARG));

    pkt = (void *) ar_cookie->arc_bp[0];
    mapNo = ar_cookie->arc_bp[1];
    status = pPacket->Status;
    eid = pPacket->Endpoint;
           
    DPRINTF(DBG_HTC, (DBGFMT "got cookie %p, %p, %d %d\n", DBGARG, cookie, pkt, mapNo, eid));
    
    pktlen = A_NETBUF_LEN(pkt);
    pkt_data = A_NETBUF_VIEW_DATA(pkt, void *, pktlen);

	A_ASSERT(pkt);
	A_ASSERT(pPacket->pBuffer == pkt_data);

    if (A_SUCCESS(status)) {
        A_ASSERT(pPacket->ActualLength == pktlen);
    }

	A_MUTEX_LOCK(&ar->arLock);

	ar->arTxPending[eid]--;

    if ((eid != ar->arControlEp) || bypasswmi)
	{
		ar->arTotalTxDataPending--;
	}

    if (eid == ar->arControlEp)
	{
        DPRINTF(DBG_HTC, (DBGFMT "Control MBOX\n", DBGARG));
        if (ar->arWMIControlEpFull) {
            /* since this packet completed, the WMI EP is no longer full */
            ar->arWMIControlEpFull = FALSE;
        }
	}

	if (A_FAILED(status))
	{
		AR6000_STAT_INC(ar, tx_errors);
        if (status != A_NO_RESOURCE) {
            DPRINTF(DBG_HTC, (DBGFMT "TX ERROR, status: 0x%x\n", DBGARG, status));
        }
	}
	else
	{
		DPRINTF(DBG_HTC, (DBGFMT "TX OK\n", DBGARG));
		AR6000_STAT_INC(ar, tx_packets);
	}
    
    // TODO this needs to be looked at 
    if ((ar->arNetworkType == ADHOC_NETWORK) && ar->arIbssPsEnable 
        && (eid != ar->arControlEp) && mapNo)
    {
        mapNo --;
        ar->arNodeMap[mapNo].txPending --;

        if (!ar->arNodeMap[mapNo].txPending && (mapNo == (ar->arNodeNum - 1))) {
            A_UINT32 i;
            for (i = ar->arNodeNum; i > 0; i --) {
                if (!ar->arNodeMap[i - 1].txPending) {
                    A_MEMZERO(&ar->arNodeMap[i - 1], sizeof(struct ar_node_mapping));
                    ar->arNodeNum --;
                } else {
                    break;
                }
            }
        }
    }

	/* Freeing a cookie should not be contingent on either of */
    /* these flags, just if we have a cookie or not.           */
    /* Can we even get here without a cookie? Fix later.       */
    if (ar->arWmiReady == TRUE || (bypasswmi))
    {
        qcom_free_cookie(ar, cookie);
    }

    dtHdr = (WMI_DATA_HDR *) pkt_data;

	if ((eid != ar->arControlEp) && (ar->arConnected == TRUE || bypasswmi) && 
        (((dtHdr->info >> WMI_DATA_HDR_MSG_TYPE_SHIFT) & WMI_DATA_HDR_MSG_TYPE_MASK)
         != SYNC_MSGTYPE))
	{
        int qLen;
        
        if (WMI_DATA_HDR_GET_DATA_TYPE(dtHdr) != WMI_DATA_HDR_DATA_TYPE_ACL) {
            up = *(A_NETBUF_VIEW_DATA(pkt, A_UINT8 *, pktlen) + pktlen);
            qLen = len_htc_data(up2qid(up));
        }
        else {
        	qLen = len_htc_data(QCOM_Q_BT);
        }
        
        DPRINTF(DBG_HTC, (DBGFMT "Q status %d %d\n", DBGARG, up, qLen));

		/* We might be ready to accept more data */
        if(qLen < MAX_DATA_QUEUE_SIZE)
		{
			/* Yes we are */
			htc_start_data();
		}

		/* Signal task that we can try to send some more */
		htc_task_signal(HTC_TX_DATA_SIG);
	}

    A_NETBUF_FREE(pkt);
	A_MUTEX_UNLOCK(&ar->arLock);
}

/*
 * Receive event handler.  This is called by HTC when a packet is received
 */
void
qcom_rx(void *Context, HTC_PACKET *pPacket)
{
	AR_SOFTC_T *ar = (AR_SOFTC_T *) Context;
	void *pbuf = (void *) pPacket->pPktContext;
	int minHdrLen;
    A_STATUS status = pPacket->Status;
    HTC_ENDPOINT_ID ept = pPacket->Endpoint;
    
	DPRINTF(DBG_HTC, (DBGFMT "Enter %d\n", DBGARG, ept));

    A_ASSERT((status != A_OK) || 
             (pPacket->pBuffer == (A_NETBUF_VIEW_DATA(pbuf, void *, 
                                                      A_NETBUF_LEN(pbuf)) + 
                                                      HTC_HEADER_LEN)));

	ar->arRxBuffers[ept]--;
    
    if (A_SUCCESS(status)) {
    	AR6000_STAT_INC(ar, rx_packets);
    	A_NETBUF_PUT(pbuf, pPacket->ActualLength +  HTC_HEADER_LEN);
    	A_NETBUF_PULL(pbuf, HTC_HEADER_LEN);
    }
    
	if (status != A_OK)
	{
		AR6000_STAT_INC(ar, rx_errors);
		A_NETBUF_FREE(pbuf);
	}
	else if (ar->arWmiEnabled == TRUE)
	{
		if (ept == ar->arControlEp)
		{
			/*
			 * this is a wmi control msg
			 */
			DPRINTF(DBG_HTC, (DBGFMT "WMI control message.\n", DBGARG));

			wmi_control_rx(ar->arWmi, pbuf);
		}
		else
		{
			WMI_DATA_HDR *dhdr = (WMI_DATA_HDR *) A_NETBUF_DATA(pbuf);
			A_UINT8 is_acl_data_frame = WMI_DATA_HDR_GET_DATA_TYPE(dhdr) == WMI_DATA_HDR_DATA_TYPE_ACL;
			
			/*
			 * this is a wmi data packet
			 */
			DPRINTF(DBG_HTC, (DBGFMT "WMI data packet.\n", DBGARG));

			minHdrLen = sizeof (WMI_DATA_HDR) + sizeof(ATH_MAC_HDR) +
				sizeof(ATH_LLC_SNAP_HDR);

			if (ar->arNetworkType != AP_NETWORK &&  !is_acl_data_frame && 
                ((pPacket->ActualLength < minHdrLen) ||
				 (pPacket->ActualLength > AR6000_BUFFER_SIZE)))
			{
				/*
				 * packet is too short or too long
				 */
				DPRINTF(DBG_HTC, (DBGFMT "Packet is too short or too long.\n", DBGARG));
				AR6000_STAT_INC(ar, rx_errors);
				AR6000_STAT_INC(ar, rx_length_errors);
				A_NETBUF_FREE(pbuf);
			}
			else
			{
        	    A_INT8 rssi = dhdr->rssi;

				DPRINTF(DBG_HTC, (DBGFMT "Data packet RX.\n", DBGARG));

				wmi_data_hdr_remove(ar->arWmi, pbuf);

				/* Deliver the data */
				if (is_acl_data_frame) {
					if (ar->data_dispatcher) {
				        ar->data_dispatcher(A_NETBUF_DATA(pbuf), A_NETBUF_LEN(pbuf));
				    }
					A_NETBUF_FREE(pbuf);
				}
				else {
				    htc_task_rx_data(pbuf, rssi);
				}
			}
		}
	}
	else
	{
		DPRINTF(DBG_HTC, (DBGFMT "WMI is not enabled.\n", DBGARG));
	}

	if (status != A_ECANCELED)
	{
		/*
		 * HTC provides A_ECANCELED status when it doesn't want to be refilled
		 * (probably due to a shutdown)
		 */
		qcom_rx_refill(Context, ept);
	}
}

void
qcom_rx_refill(void *Context, HTC_ENDPOINT_ID Endpoint)
{
	AR_SOFTC_T *ar = (AR_SOFTC_T *)Context;
	void *osBuf;
    int  RxBuffers;
    int  buffersToRefill;
    HTC_PACKET *pPacket;
        
	DPRINTF(DBG_HTC, (DBGFMT "Enter - eid %d\n", DBGARG, Endpoint));

    buffersToRefill = (int)AR6000_MAX_RX_BUFFERS -
                                    (int)ar->arRxBuffers[Endpoint];

    if (buffersToRefill <= 0) {
        /* fast return, nothing to fill */
        return;
    }
    
	for(RxBuffers = 0; RxBuffers < buffersToRefill; RxBuffers++)
	{
		osBuf = A_NETBUF_ALLOC(AR6000_BUFFER_SIZE);
        if (NULL == osBuf) {
            break;
        }
        
        /* the HTC packet wrapper is at the head of the reserved area
         * in the skb */
        pPacket = (HTC_PACKET *)(A_NETBUF_HEAD(osBuf));
        /* set re-fill info */
        SET_HTC_PACKET_INFO_RX_REFILL(pPacket, osBuf, A_NETBUF_DATA(osBuf), AR6000_BUFFER_SIZE, Endpoint);
        /* add this packet */
        HTCAddReceivePkt(ar->arHtcTarget, pPacket);
	}

    A_MUTEX_LOCK(&ar->arLock);
    ar->arRxBuffers[Endpoint] += RxBuffers;
    A_MUTEX_UNLOCK(&ar->arLock);
    
    DPRINTF(DBG_HTC, (DBGFMT "Got %d buffers for eid %d\n", DBGARG,
            RxBuffers, Endpoint));
}

/*****************************************************************************/
/*****************************************************************************/
/**                                                                         **/
/** HTC external API routines                                               **/
/**                                                                         **/
/** This section contains the routines that make up the exported API        **/
/** to this module and the HTC task.                                        **/
/**                                                                         **/
/*****************************************************************************/
/*****************************************************************************/

void htc_ready(void)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter\n", DBGARG));

	/* Signal the task */
	htc_task_signal(HTC_READY_SIG);
}

void htc_enable_firmware_upload(void)
{
	A_MUTEX_LOCK(&s_flags_lock);
	s_flags |= FLAG_FIRMWARE_UPLOAD;
	A_MUTEX_UNLOCK(&s_flags_lock);

	return;
}

void htc_set_rssi(int rssi)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter - New RSSI %d\n", DBGARG, rssi));

	s_rssi = rssi;

	return;
}

void htc_set_macaddress(A_UINT8 *datap)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter\n", DBGARG));

	A_MEMCPY(s_macaddress, datap, 6);

	return;
}

A_UINT8 *htc_get_macaddress(void)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter\n", DBGARG));

	return(s_macaddress);
}

wlan_adp_scan_req_info_type *htc_get_scan_filter(void)
{   
    return(&s_scan_filter);
}

void htc_set_failure(int failure)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter - %d\n", DBGARG, failure));

	/* Clear any timeouts if we had any */
	stop_command_timeout();

	s_failure = failure;

	htc_task_signal(HTC_FAIL_SIG);

	return;
}

void htc_set_status_data(int index, void *data)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter\n", DBGARG));

	s_status_data[index] = data;
		
	return;
}

void *htc_get_status_data(int index)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter\n", DBGARG));

	/* No checking since internal use only */
	return(s_status_data[index]);
}

void htc_set_status(unsigned int status)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter\n", DBGARG));

	s_status |= status;

	return;
}

unsigned int htc_get_status(void)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter\n", DBGARG));

	return(s_status);
}

void htc_clear_status(unsigned int status)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter\n", DBGARG));

	s_status &= ~status;

	return;
}

#ifdef CONFIG_HOST_TCMD_SUPPORT
void htc_set_test_mode(void)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter\n", DBGARG));
	A_MUTEX_LOCK(&s_flags_lock);
	s_flags |= FLAG_TEST_MODE ;
	A_MUTEX_UNLOCK(&s_flags_lock);
}
#endif

A_UINT8 htc_get_test_mode(void)
{
#ifdef CONFIG_HOST_TCMD_SUPPORT
	A_UINT8 ret;
	
	DPRINTF(DBG_HTC, (DBGFMT "Enter\n", DBGARG));

	A_MUTEX_LOCK(&s_flags_lock);
	if (s_flags & FLAG_TEST_MODE)
		ret = TRUE;
	else
		ret = FALSE;
	A_MUTEX_UNLOCK(&s_flags_lock);

	return ret;
#else
	return FALSE;
#endif
}

void htc_set_wmm_info(wlan_adp_wmm_info_type *wmm)
{
	/* Save the pointer */
	s_wmm_info_p = wmm;

	return;
}

struct htc_request *htc_alloc_request(void)
{
	struct htc_request *req = NULL;

	A_MUTEX_LOCK(&s_htc_request_free_list_lock);
	if(s_htc_request_free_list != NULL)
	{
		/* Allocate first in list */
		req = s_htc_request_free_list;
		s_htc_request_free_list = req->next;
	}
	A_MUTEX_UNLOCK(&s_htc_request_free_list_lock);

	/* Clear the next field for now.                      */
	req->next = NULL;

	return(req);
}

void htc_free_request(struct htc_request *req)
{
	/* Put first in list */
	A_MUTEX_LOCK(&s_htc_request_free_list_lock);
	req->next = s_htc_request_free_list;
	s_htc_request_free_list = req;
	A_MUTEX_UNLOCK(&s_htc_request_free_list_lock);

	return;
}

void htc_task_signal(unsigned int val)
{
	A_ASSERT(NULL != htc_task_ptr);
	rex_set_sigs(htc_task_ptr, (rex_sigs_type) val);
}

void htc_task_command(struct htc_request *req)
{
	/* Enqueue the command */
	enqueue_htc_request(req);
	
	/* We only accept one command at a time */
	LOCK_COMMAND;

	/* Signal the task */
	htc_task_signal(HTC_COMMAND_SIG);
}

void htc_task_tx_data(void *pkt, int up)
{
	QCOM_Q_ID qId;
	int len;
	
	DPRINTF(DBG_HTC, (DBGFMT "Enter\n", DBGARG));

	/* Convert this user priority to our endpoint id */
	qId = up2qid(up);

	/* Enqueue the packet */
	enqueue_htc_data(pkt, qId);
	
	/* Are we getting close to full */
	len = len_htc_data(qId);
	if(len >= MAX_DATA_QUEUE_SIZE)
	{
		DPRINTF(DBG_HTC, (DBGFMT "Queue ID %d queue length %d >= %d \n", DBGARG, qId, len, MAX_DATA_QUEUE_SIZE));
		
		/* We don't accept more data at this time */
		htc_stop_data();
	}

	/* Signal the task */
	htc_task_signal(HTC_TX_DATA_SIG);
}

void htc_task_tx_bt_data(void *pkt)
{
	int len;
	
	DPRINTF(DBG_HTC, (DBGFMT "Enter\n", DBGARG));
	
	/* Enqueue the packet */
	enqueue_htc_data(pkt, QCOM_Q_BT);
	
	/* Are we getting close to full */
	len = len_htc_data(QCOM_Q_BT);
	if(len >= MAX_DATA_QUEUE_SIZE)
	{
		DPRINTF(DBG_HTC, (DBGFMT "Queue ID %d queue length %d >= %d \n", DBGARG, QCOM_Q_BT, len, MAX_DATA_QUEUE_SIZE));
		
		/* We don't accept more data at this time */
		htc_stop_data();
	}

	/* Signal the task */
	htc_task_signal(HTC_TX_DATA_SIG);
}

void htc_task_rx_data(void *pkt, A_INT8 rssi)
{
	unsigned char *data;
	int len;
	wlan_adp_rx_pkt_meta_info_type meta;
    ATH_LLC_SNAP_HDR *llcHdr;
    
	DPRINTF(DBG_HTC, (DBGFMT "Enter\n", DBGARG));
	len = A_NETBUF_LEN(pkt);
	data = A_NETBUF_VIEW_DATA(pkt, unsigned char *, len);

	meta.src_mac_addr = wlan_util_macaddress_to_uint64(&data[6]);
	meta.dst_mac_addr = wlan_util_macaddress_to_uint64(&data[0]);
	meta.rssi = rssi;
    
    llcHdr = (ATH_LLC_SNAP_HDR *)(data + sizeof(ATH_MAC_HDR));
    if (llcHdr->etherType == A_HTONS(IP_ETHERTYPE)) 
    {
        meta.tid = ar6000_iptos_to_userPriority(((A_UINT8 *)llcHdr)
                                    + sizeof(ATH_LLC_SNAP_HDR));
    }
    else
        meta.tid = 0;
    
	/* Remove the Atheros mac header */
	A_NETBUF_PULL(pkt, sizeof(ATH_MAC_HDR));
	len -= sizeof(ATH_MAC_HDR);
    data = A_NETBUF_DATA(pkt);
    
	/* Deliver to the ADP layer */
	wlan_adp_oem_rx_data(data, len, &meta);

	/* We are done with this guy */
	A_NETBUF_FREE(pkt);
}

int htc_task_query(int query)
{
	int status = FALSE;
	AR_SOFTC_T *ar = (AR_SOFTC_T *) s_device;

	switch(query)
	{
		case HTC_QUERY_TX_OPEN:
			DPRINTF(DBG_HTC, (DBGFMT "HTC_QUERY_TX_OPEN\n", DBGARG));
			A_MUTEX_LOCK(&s_flags_lock);
			status = s_flags & FLAG_TX_READY ? TRUE : FALSE;
			A_MUTEX_UNLOCK(&s_flags_lock);

			/* Also check our ready flag and if we are connected */
			if(ar == NULL || ar->arWmiReady == FALSE || ar->arConnected == FALSE)
			{
				DPRINTF(DBG_HTC,
					(DBGFMT "WMI layer is not ready or not connected.\n",
																	 DBGARG));
				status = FALSE;
			}

			if(status == FALSE)
			{
				DPRINTF(DBG_HTC, (DBGFMT "HTC task is not ready.\n", DBGARG));
			}

			break;
		case HTC_QUERY_CMD_OPEN:
			DPRINTF(DBG_HTC, (DBGFMT "HTC_QUERY_CMD_OPEN\n", DBGARG));
			A_MUTEX_LOCK(&s_flags_lock);
			status = s_flags & FLAG_CMD_READY ? TRUE : FALSE;
			A_MUTEX_UNLOCK(&s_flags_lock);

			if(status == FALSE)
			{
				DPRINTF(DBG_HTC, (DBGFMT "HTC task is not ready.\n", DBGARG));
			}

			/* Also check our ready flag */
			if( (ar == NULL || ar->arWmiReady == FALSE) )
			{
				DPRINTF(DBG_HTC, (DBGFMT "WMI layer is not ready.\n", DBGARG));
				status = FALSE;
			}
			break;
	}

	return(status);
}

void *htc_device_handle(void)
{
	return((void *) s_device);
}

int htc_task_init(void)
{
	int i;

	/* Clear out our state */
	A_MUTEX_INIT(&s_htc_request_free_list_lock);
	A_MUTEX_INIT(&s_htc_request_queue_lock);
	A_MUTEX_INIT(&s_htc_data_queue_lock);
	A_MUTEX_INIT(&s_flags_lock);

	/* Clear some data structures */
	A_MEMZERO(&s_protect_info, sizeof(wlan_adp_wpa_protect_list_type));

	/* Clear out the flags */
	s_flags = 0;

	/* Populate the free list */
	for(i = 0; i < NUM_HTC_REQUESTS; i++)
	{
		htc_free_request(&s_htc_requests[i]);
	}

	return(0);
}

int htc_task_exit()
{
	/* The device is no longer active */
	s_device = NULL;

	return(0);
}

void htc_command_complete(void)
{
	/* We can accept more commands */
	UNLOCK_COMMAND;

	/* Let the upper layer know that we are done */
	wlan_trp_adp_cmd_complete_ind();

	return;
}

void htc_stop_data(void)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter\n", DBGARG));

	/* We cannot accept more data */
	LOCK_DATA;

	return;
}

void htc_start_data(void)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter\n", DBGARG));

	/* We can accept more data */
	UNLOCK_DATA;

	return;
}

/*****************************************************************************/
/*****************************************************************************/
/**                                                                         **/
/** Command and state functions                                             **/
/**                                                                         **/
/** This sections contains the command and state processing/manipulation    **/
/** functions (including command timeout processing)                        **/
/**                                                                         **/
/*****************************************************************************/
/*****************************************************************************/

/*****************************************************************************/
/** Timeout processing                                                      **/
/*****************************************************************************/

static void start_command_timeout(unsigned int cmd, unsigned int ms)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter\n", DBGARG));

	s_timeout_command = cmd;
	A_TIMEOUT_MS(&s_command_timeout_timer, ms, 0);

	return;
}

static void stop_command_timeout(void)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter\n", DBGARG));

	A_UNTIMEOUT(&s_command_timeout_timer);

	return;
}

static void process_command_timeout(void *arg)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter - TIMEOUT TIMEOUT TIMEOUT\n", DBGARG));

	/* Indicate that we have a fatal error */
	wlan_adp_oem_fatal_failure_ind();

	return;
}

/*****************************************************************************/
/** Signal table                                                            **/
/*****************************************************************************/

static struct signal_index_item
{
	unsigned int signal;
	char *description;
}
s_signal_index_table[] =
{
	{ HTC_START_SIG,       "HTC_START_SIG",     },
	{ HTC_STOP_SIG,        "HTC_STOP_SIG",      },
	{ HTC_COMMAND_SIG,     "HTC_COMMAND_SIG",   },
	{ HTC_READY_SIG,       "HTC_READY_SIG",     },
	{ HTC_HIFDSR_SIG,      "HTC_HIFDSR_SIG",    },
	{ HTC_FAIL_SIG,        "HTC_FAIL_SIG",      },
	{ HTC_TX_DATA_SIG,     "HTC_TX_DATA_SIG",   },
	{ HTC_RX_DATA_SIG,     "HTC_RX_DATA_SIG",   },
};

#define SIGNAL_INDEX_TABLE_SIZE \
	(sizeof(s_signal_index_table) / sizeof(struct signal_index_item))

/*****************************************************************************/
/** State init                                                              **/
/*****************************************************************************/

static int state_init_fail_sig(void)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	DPRINTF(DBG_HTC, (DBGFMT "******************************\n", DBGARG));
	DPRINTF(DBG_HTC, (DBGFMT "** FAIL FAIL FAIL FAIL FAIL **\n", DBGARG));
	DPRINTF(DBG_HTC, (DBGFMT "******************************\n", DBGARG));

	if(s_failure == FAIL_FATAL)
	{
		/* Switch state to something safe */
		set_state(HTC_STATE_IDLE);

		/* Indicate that we have a fatal error */
		wlan_adp_oem_fatal_failure_ind();
	}

	/* Clean it up */
	s_failure = 0;

	return(0);
}

static int state_init_start_sig(void)
{
	int i;
#ifdef CONFIG_HOST_TCMD_SUPPORT
	int status;
#endif
	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	/* Do initialization that needs to be done every time we start */
	
	/* Initialize the endpoint table for QoS */
	for(i = 0; i < NUM_SID_ENTRIES; i++)
	{
		A_NETBUF_QUEUE_INIT(&s_sid_table[i].queue);
	}

	/* The card is always assumed to be inserted */
	if(HIFHandleInserted() < 0)
	{
		DPRINTF(DBG_HTC, (DBGFMT "Failed to initialize card.\n", DBGARG));
		set_state(HTC_STATE_DROP);
		return(0);
	}

	/* Only start if we have a card inserted */
	if(s_device == NULL)
	{
		DPRINTF(DBG_HTC, (DBGFMT "No card is detected.\n", DBGARG));
		return(0);
	}

	/* Try to open */
	if(qcom_driver_open(s_device) == 0)
	{
#ifdef CONFIG_HOST_TCMD_SUPPORT
		A_MUTEX_LOCK(&s_flags_lock);
		status = s_flags & FLAG_TEST_MODE ? TRUE : FALSE;
		A_MUTEX_UNLOCK(&s_flags_lock);

		if(status)
		{
			set_state(HTC_STATE_TEST);
		}
		else
		{
			set_state(HTC_STATE_CONFIG);
		}

#else
		/* Looks like we are ready for configuration */
		set_state(HTC_STATE_CONFIG);
#endif
	}
	else
	{
		/* Tell the WLAN Adapter layer that we are not ready */
		wlan_adp_oem_ready_ind(-1);
	}

	return(0);
}

static int state_start_command_sig(void)
{
	struct htc_request *req;

	/* Make sure we ignore commands in this state */

	DPRINTF(DBG_HTC, (DBGFMT "Enter - Dropping command requests.\n", DBGARG));

	/* Drain the the whole command queue */
	while((req = dequeue_htc_request()) != NULL)
	{
		/* We are done with this guy */
		htc_free_request(req);
	}

	return(0);
}

static int (*s_state_start[SIGNAL_INDEX_TABLE_SIZE])(void) = 
{
	/* HTC_START_SIG      */	state_init_start_sig,
	/* HTC_STOP_SIG       */	NULL,
	/* HTC_COMMAND_SIG    */	state_start_command_sig,
	/* HTC_READY_SIG      */	NULL,
	/* HTC_HIFDSR_SIG     */	NULL,
	/* HTC_FAIL_SIG       */	state_init_fail_sig,
	/* HTC_TX_DATA_SIG    */	NULL,
	/* HTC_RX_DATA_SIG    */	NULL,
};

/*****************************************************************************/
/** State drop                                                              **/
/*****************************************************************************/

static int state_drop_command_sig(void)
{
	struct htc_request *req;

	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	/* Drain the the whole command queue */
	while((req = dequeue_htc_request()) != NULL)
	{
		/* We are done with this guy */
		htc_free_request(req);
	}

	/* Indicate that we have a fatal error */
	wlan_adp_oem_fatal_failure_ind();

	return(0);
}

static int state_drop_tx_data_sig(void)
{
	void *pkt;
	QCOM_Q_ID qId;

	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	while((pkt = dequeue_htc_data(&qId)) != NULL)
	{
		/* Just free it */
		A_NETBUF_FREE(pkt);
	}

	/* Indicate that we have a fatal error */
	wlan_adp_oem_fatal_failure_ind();

	return(0);
}

static int (*s_state_drop[SIGNAL_INDEX_TABLE_SIZE])(void) = 
{
	/* HTC_START_SIG      */	NULL,
	/* HTC_STOP_SIG       */	NULL,
	/* HTC_COMMAND_SIG    */	state_drop_command_sig,
	/* HTC_READY_SIG      */	NULL,
	/* HTC_HIFDSR_SIG     */	NULL,
	/* HTC_FAIL_SIG       */	state_init_fail_sig,
	/* HTC_TX_DATA_SIG    */	state_drop_tx_data_sig,
	/* HTC_RX_DATA_SIG    */	NULL,
};

/*****************************************************************************/
/** State config                                                            **/
/*****************************************************************************/

static int state_config_fail_sig(void)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	DPRINTF(DBG_HTC, (DBGFMT "******************************\n", DBGARG));
	DPRINTF(DBG_HTC, (DBGFMT "** FAIL FAIL FAIL FAIL FAIL **\n", DBGARG));
	DPRINTF(DBG_HTC, (DBGFMT "******************************\n", DBGARG));

	if(s_failure == FAIL_FATAL)
	{
		/* Close the device */
		qcom_driver_close(s_device);

		/* Switch state to something safe */
		set_state(HTC_STATE_IDLE);

		/* Tell the WLAN Adapter layer that we are not ready */
		wlan_adp_oem_ready_ind(-1);
	}

	/* Clean it up */
	s_failure = 0;

	return(0);
}

static int state_config_stop_sig(void)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	/* Clear any timeouts if we had any */
	stop_command_timeout();

	/* Close driver */
	qcom_driver_close(s_device);

	/* The card has been "removed" */
	HIFHandleRemoved();

	/* Set the initial state */
	set_state(HTC_STATE_START);

	return(0);
}

static int state_config_hifdsr_sig(void)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	HIFHandleIRQ();

	return(0);
}

static int state_config_ready_sig(void)
{
    A_UINT16 mmask = 0;
    A_UINT32 cfgvalid = 0xFFFFFFFF;
    
	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	/* Looks like we are open */
	set_state(HTC_STATE_OPEN);

	/* Let the ADP layer know our MAC address */
	wlan_util_set_hw_mac_addr(wlan_util_macaddress_to_uint64(s_macaddress));

    /* Communicate the wmi protocol version to the target */
    if (!bypasswmi && (qcom_set_host_app_area((AR_SOFTC_T *) s_device) != A_OK))
    {
        DPRINTF(DBG_HTC, (DBGFMT "Unable to set the host app area\n", DBGARG));
    }

#ifdef DISABLE_SLEEP
	/* Set the power mode */
	wmi_powermode_cmd(((AR_SOFTC_T *) s_device)->arWmi, MAX_PERF_POWER);
#endif

    /* Set the power mode */
    wmi_set_nodeage(((AR_SOFTC_T *) s_device)->arWmi, wlan_node_age);
    
    /* Set BT antenna configuration */
    #if 0
    if (bt_ant_config != 0) {
        WMI_SET_BT_PARAMS_CMD btParamsCmd;

        A_MEMZERO(&btParamsCmd, sizeof(btParamsCmd));
        btParamsCmd.paramType = BT_PARAM_ANTENNA_CONFIG;
        btParamsCmd.info.antType = (A_UINT8) bt_ant_config;
        
        /* set BT Coexistence Antenna configuration */
        wmi_set_bt_params_cmd(((AR_SOFTC_T *) s_device)->arWmi, &btParamsCmd);
    }
    #endif
    
    /* by default dbglog is enabled. Disable logging for all modules and
       disable log reporting */
    if (miscFlags.enabledbglog)
    {
        A_UINT32 size;
        
        mmask = 0xFFFF;
        cfgvalid = DBGLOG_MODULE_LOG_ENABLE_MASK|DBGLOG_TIMESTAMP_RESOLUTION_MASK;
        
        dbglog_fl_h = A_FL_INITIALIZE("dbglog.txt", "Images/", A_FL_CREATE | A_FL_RDWR);
        if (dbglog_fl_h == FL_NULL) {
            DPRINTF(DBG_HTC, (DBGFMT "failed to create dbglog file.\n", DBGARG));
        }
        A_FL_SEEK(dbglog_fl_h, 0, A_FL_SEEK_SET);

        size = (size / sizeof(dbg_rec) + 1) * sizeof(dbg_rec);
        dbglog_fl_sz = (size < AR6K_DBG_MIN_FILE_SIZE) ? AR6K_DBG_MIN_FILE_SIZE : size;
    }
    if (wmi_config_debug_module_cmd(((AR_SOFTC_T *) s_device)->arWmi,
                                    mmask, 0, 0, 0,
                                    cfgvalid) != A_OK)
    {
        DPRINTF(DBG_HTC, (DBGFMT "configuring debuglog failed.\n", DBGARG));
        return -1;
    }
    DPRINTF(DBG_HTC, (DBGFMT "Debug log from target %s\n", DBGARG, 
            (miscFlags.enabledbglog) ? "enabled" : "disabled"));
    
	/* Enable commands */
	UNLOCK_COMMAND;

	/* Tell the WLAN Adapter layer that we are ready */
	wlan_adp_oem_ready_ind(0);

	return(0);
}

static int (*s_state_config[SIGNAL_INDEX_TABLE_SIZE])(void) = 
{
	/* HTC_START_SIG      */	NULL,
	/* HTC_STOP_SIG       */	state_config_stop_sig,
	/* HTC_COMMAND_SIG    */	NULL,
	/* HTC_READY_SIG      */	state_config_ready_sig,
	/* HTC_HIFDSR_SIG     */	state_config_hifdsr_sig,
	/* HTC_FAIL_SIG       */	state_config_fail_sig,
	/* HTC_TX_DATA_SIG    */	NULL,
	/* HTC_RX_DATA_SIG    */	NULL,
};

/*****************************************************************************/
/** State open                                                              **/
/*****************************************************************************/

static int state_open_fail_sig(void)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	DPRINTF(DBG_HTC, (DBGFMT "******************************\n", DBGARG));
	DPRINTF(DBG_HTC, (DBGFMT "** FAIL FAIL FAIL FAIL FAIL **\n", DBGARG));
	DPRINTF(DBG_HTC, (DBGFMT "******************************\n", DBGARG));

	if(s_failure == FAIL_FATAL)
	{
		/* Close the device */
		qcom_driver_close(s_device);

		/* Switch state to something safe */
		set_state(HTC_STATE_IDLE);

		/* Indicate that we have a fatal error */
		wlan_adp_oem_fatal_failure_ind();
	}

	/* Clean it up */
	s_failure = 0;

	return(0);
}

static int state_open_stop_sig(void)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	/* Clear any timeouts if we had any */
	stop_command_timeout();

	/* Close driver */
	qcom_driver_close(s_device);

	/* The card has been "removed" */
	HIFHandleRemoved();

	/* Set the initial state */
	set_state(HTC_STATE_START);

	return(0);
}

static int state_open_hifdsr_sig(void)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	HIFHandleIRQ();

	return(0);
}

/*****************************************************************************/
/** Start of command functions                                              **/
/*****************************************************************************/

static void wlan_trp_adp_suspend_cmd(AR_SOFTC_T *ar, struct htc_request *req)
{
	int perror = 0;	/* 0 = success, -1 = failure */

	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	/* Disconnect */
	if(wmi_disconnect_cmd(ar->arWmi) != A_OK)
	{
		DPRINTF(DBG_HTC,
			(DBGFMT "wmi_disconnect_cmd() failed.\n", DBGARG));
		perror = -1;
	}

	/* Disable scanning */
	if(perror == 0 && wmi_scanparams_cmd(ar->arWmi,
		65535, 0, 65535, 0, 0, 0, 0, DEFAULT_SCAN_CTRL_FLAGS, 0, 0) != A_OK)
	{
		DPRINTF(DBG_HTC,
			(DBGFMT "wmi_scanparams_cmd() failed.\n", DBGARG));
		perror = -1;
	}

	/* We can accept more commands */
	UNLOCK_COMMAND;

	/* Tell the WLAN Adapter layer that we are done */
	wlan_adp_oem_suspend_rsp(perror);

	return;
}

static void wlan_trp_adp_resume_cmd(AR_SOFTC_T *ar, struct htc_request *req)
{
	int perror = 0;	/* 0 = success, -1 = failure */

	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	/* Enable scanning */
	if(wmi_scanparams_cmd(ar->arWmi, 0, 0, 0, 0, 0, 0, 0, DEFAULT_SCAN_CTRL_FLAGS, 0, 0) != A_OK)
	{
		DPRINTF(DBG_HTC,
			(DBGFMT "wmi_scanparams_cmd() failed.\n", DBGARG));
		perror = -1;
	}

	/* We can accept more commands */
	UNLOCK_COMMAND;

	/* Tell the WLAN Adapter layer that we are done */
	wlan_adp_oem_resume_rsp(perror);

	return;
}

static void wlan_trp_adp_scan_cmd(AR_SOFTC_T *ar, struct htc_request *req)
{	
	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	/* Remember that we have started a scan */
	htc_set_status(SCAN_IN_PROGRESS);
	
	/* Save the scan filter */
	A_MEMCPY(&s_scan_filter, &req->req_info.scan_info,
		sizeof(wlan_adp_scan_req_info_type));

	/* If we request the broadcast SSID the we set it to all */
	if(req->req_info.scan_info.ssid.len == 0)
	{
		DPRINTF(DBG_HTC,
			(DBGFMT "Setting ALL BSS Filter.\n", DBGARG));
		if(wmi_bssfilter_cmd(ar->arWmi, ALL_BSS_FILTER, 0) != A_OK)
		{
			DPRINTF(DBG_HTC,
				(DBGFMT "wmi_bssfilter_cmd() failed.\n", DBGARG));
			goto scan_error;
		}
	}
	else
	{
		if(wmi_bssfilter_cmd(ar->arWmi, PROBED_SSID_FILTER, 0) != A_OK)
		{
			DPRINTF(DBG_HTC,
				(DBGFMT "wmi_bssfilter_cmd() failed.\n", DBGARG));
			goto scan_error;
		}

		if (wmi_probedSsid_cmd(ar->arWmi, 0,
			SPECIFIC_SSID_FLAG,
			req->req_info.scan_info.ssid.len,
			(A_UCHAR *) req->req_info.scan_info.ssid.ssid) != A_OK)
		{
			DPRINTF(DBG_HTC, (DBGFMT "wmi_probedSsid_cmd() failed.\n", DBGARG));
			goto scan_error;
		}
	}
		
	/* Doing WMI_LONG_SCAN or WMI_SHORT_SCAN */
	if (wmi_startscan_cmd(ar->arWmi, WMI_LONG_SCAN, TRUE, FALSE, 105, 250, 0, NULL) != A_OK)
	{
		DPRINTF(DBG_HTC, (DBGFMT "wmi_startscan_cmd() failed.\n", DBGARG));
		goto scan_error;
	}

	return;

scan_error:
	/* Recover from failures here */
	{
		static wlan_adp_scan_rsp_info_type scan_rsp_info;

		scan_rsp_info.num_bss = 0;
		scan_rsp_info.result_code = WLAN_SYS_RES_INVALID_PARAMS;

		/* Reset the filter */
		if(wmi_bssfilter_cmd(ar->arWmi, NONE_BSS_FILTER, 0) != A_OK)
		{
			DPRINTF(DBG_WMI,
				(DBGFMT "wmi_bssfilter_cmd() failed.\n", DBGARG));
		}

		/* Get the result back to the WLAN Adapter layer */
		wlan_adp_oem_scan_rsp(&scan_rsp_info);

		/* Clear the status */
		htc_clear_status(SCAN_IN_PROGRESS);

		/* We can accept more commands */
		UNLOCK_COMMAND;
	}

	return;
}

static void wlan_trp_adp_join_cmd(AR_SOFTC_T *ar, struct htc_request *req)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	/* Save the information */
	A_MEMCPY(&s_join_info, &req->req_info.join_info,
		sizeof(wlan_adp_join_req_info_type));

	/* We can accept more commands */
	UNLOCK_COMMAND;

	/* Get the result back to the WLAN Adapter layer */
	wlan_adp_oem_join_rsp(WLAN_SYS_RES_SUCCESS);

	return;
}

static void wlan_trp_adp_auth_cmd(AR_SOFTC_T *ar, struct htc_request *req)
{
	wlan_dot11_result_code_enum rc = WLAN_SYS_RES_SUCCESS;
	A_UINT8 bssid[6];
	A_UINT8 zerobssid[6];
	NETWORK_TYPE network;
	AUTH_MODE akm;
	DOT11_AUTH_MODE auth;
	CRYPTO_TYPE pair;
	CRYPTO_TYPE group;
	A_UINT8 do_key = 0;
	A_UINT8 do_protect = FALSE;
	int i;

	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	/* Save the information */
	A_MEMCPY(&s_auth_info, &req->req_info.auth_info,
		sizeof(wlan_adp_auth_req_info_type));

	A_MEMZERO(zerobssid, sizeof(zerobssid));

	wlan_util_uint64_to_macaddress(bssid,
		req->req_info.auth_info.peer_mac_address);

	DPRINTF(DBG_HTC,
		(DBGFMT "Connect to %02x:%02x:%02x:%02x:%02x:%02x\n", DBGARG,
		bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]));

	/* Figure out if we need protection or not */
	for(i = 0; i < WLAN_ADP_WPA_MAX_PROTECT_INFO; i++)
	{
		 DPRINTF(DBG_HTC,
			(DBGFMT "Protect BSS %02x:%02x:%02x:%02x:%02x:%02x\n", DBGARG,
			s_protect_info.protect_list[i].address[0],
			s_protect_info.protect_list[i].address[1],
			s_protect_info.protect_list[i].address[2],
			s_protect_info.protect_list[i].address[3],
			s_protect_info.protect_list[i].address[4],
			s_protect_info.protect_list[i].address[5]));

		if(A_MEMCMP(s_protect_info.protect_list[i].address,
			zerobssid, 6) == 0)
		{
			/* Found and empty slot */
			continue;
		}

		if(A_MEMCMP(s_protect_info.protect_list[i].address,
			bssid, 6) == 0)
		{
			/* We found our MAC address */
			switch(s_protect_info.protect_list[i].protect_type)
			{
				case WLAN_DOT11_WPA_PROTECT_NONE:
					do_protect = FALSE;
					break;
				case WLAN_DOT11_WPA_PROTECT_RX:
				case WLAN_DOT11_WPA_PROTECT_TX:
				case WLAN_DOT11_WPA_PROTECT_RX_TX:
					do_protect = TRUE;
					break;
			}

			do_key = s_protect_info.protect_list[i].key_type;

			DPRINTF(DBG_HTC,
				(DBGFMT "Found %02x:%02x:%02x:%02x:%02x:%02x. Do protect \"%s\" key %d.\n", DBGARG,
				bssid[0], bssid[1], bssid[2],
				bssid[3], bssid[4], bssid[5],
				do_protect == TRUE ? "TRUE" : "FALSE",
				do_key));


			break;
		}
	}

	switch(s_join_info.ap_desc.bsstype)
	{
		case SYS_WLAN_BSS_TYPE_ADHOC:
			network = ADHOC_NETWORK;
			break;
		case SYS_WLAN_BSS_TYPE_INFRA:
			network = INFRA_NETWORK;
			break;
		case SYS_WLAN_BSS_TYPE_ANY:
			/* Not supported */
			rc = WLAN_SYS_RES_NOT_SUPPORTED;
			DPRINTF(DBG_HTC, (DBGFMT "BSS type ANY not supported.\n", DBGARG));
			goto auth_error;
		default:
			/* Some error here */
			DPRINTF(DBG_HTC, (DBGFMT "Unknown BSS type %d.\n", DBGARG,
				s_join_info.ap_desc.bsstype));
			rc = WLAN_SYS_RES_INVALID_PARAMS;
			goto auth_error;
	}

	/* We don't support shared key authentication */
	if(s_auth_info.auth_type == WLAN_SYS_AUTH_ALGO_SHARED_KEY)
	{
		DPRINTF(DBG_HTC,
			(DBGFMT "Shared key 802.11 authentication is not supported.\n",
			DBGARG));
		rc = WLAN_SYS_RES_NOT_SUPPORTED;
		goto auth_error;
	}

	if(s_auth_info.auth_type != WLAN_SYS_AUTH_ALGO_OPEN_SYSTEM)
	{
		/* Some error here */
		DPRINTF(DBG_HTC, (DBGFMT "Unknown auth type %d.\n", DBGARG,
			s_auth_info.auth_type));
		rc = WLAN_SYS_RES_INVALID_PARAMS;
		goto auth_error;
	}

	switch(s_join_info.akm)
	{
		case WPA_AKM_NONE:
			auth = OPEN_AUTH;
			akm = NONE_AUTH;
			break;
		case WPA_AKM_802_1X:
			auth = OPEN_AUTH;
			akm = WPA_AUTH;
			break;
		case WPA_AKM_PSK:
			auth = OPEN_AUTH;
			akm = WPA_PSK_AUTH;
			break;
		default:
			/* Some error here */
			DPRINTF(DBG_HTC, (DBGFMT "Unknown AKM type %d.\n", DBGARG,
				s_join_info.akm));
			rc = WLAN_SYS_RES_INVALID_PARAMS;
			goto auth_error;
	}

	switch(s_join_info.pairwise_cipher)
	{
		case WPA_CIPHER_SUITE_NONE:
			pair = NONE_CRYPT;
			break;
		case WPA_CIPHER_SUITE_WEP_40:
			pair = WEP_CRYPT;
			break;
		case WPA_CIPHER_SUITE_TKIP:
			pair = TKIP_CRYPT;
			break;
		case WPA_CIPHER_SUITE_CCMP:
			pair = AES_CRYPT;
			break;
		case WPA_CIPHER_SUITE_WEP_104:
			pair = WEP_CRYPT;
			break;
		default:
			/* Some error here */
			DPRINTF(DBG_HTC, (DBGFMT "Unknown pairwise cipher %d.\n", DBGARG,
				s_join_info.pairwise_cipher));
			rc = WLAN_SYS_RES_INVALID_PARAMS;
			goto auth_error;
	}

	switch(s_join_info.group_cipher)
	{
		case WPA_CIPHER_SUITE_NONE:
			group = NONE_CRYPT;
			break;
		case WPA_CIPHER_SUITE_WEP_40:
			group = WEP_CRYPT;
			break;
		case WPA_CIPHER_SUITE_TKIP:
			group = TKIP_CRYPT;
			break;
		case WPA_CIPHER_SUITE_CCMP:
			group = AES_CRYPT;
			break;
		case WPA_CIPHER_SUITE_WEP_104:
			group = WEP_CRYPT;
			break;
		default:
			/* Some error here */
			DPRINTF(DBG_HTC, (DBGFMT "Unknown group cipher %d.\n", DBGARG,
				s_join_info.group_cipher));
			rc = WLAN_SYS_RES_INVALID_PARAMS;
			goto auth_error;
	}

	/* Set the error reporting mask */
	if (wmi_set_error_report_bitmask(ar->arWmi,
		WMI_TARGET_PM_ERR_FAIL|WMI_TARGET_KEY_NOT_FOUND|
		WMI_TARGET_DECRYPTION_ERR) != A_OK)
	{
		DPRINTF(DBG_HTC,
			(DBGFMT "wmi_set_error_report_bitmask() failed.\n", DBGARG));
		rc = WLAN_SYS_RES_INVALID_PARAMS;
		goto auth_error;
	}

#if 0
	/* For testing */
	auth = OPEN_AUTH;
	akm = NONE_AUTH;
	group = NONE_CRYPT;
	pair = NONE_CRYPT;
#endif

	/* Save the security setup */
	ar->arDot11AuthMode = auth;
	ar->arAuthMode = akm;
	ar->arPairwiseCrypto = pair;
	ar->arPairwiseCryptoLen = 0;
	ar->arGroupCrypto = group;
	ar->arGroupCryptoLen = 0;
    
	/* Save the network type */
	ar->arNetworkType = network;

	/* Save the SSID */
	ar->arSsidLen = s_join_info.ap_desc.ssid.len;
	A_MEMCPY(ar->arSsid, s_join_info.ap_desc.ssid.ssid, ar->arSsidLen);

	htc_set_status(CONNECT_IN_PROGRESS);
	htc_set_status_data(CONNECT_SLOT, (void *) 1);

	if (wmi_connect_cmd(ar->arWmi, network,
		auth, akm,
		pair, ar->arPairwiseCryptoLen, group, ar->arGroupCryptoLen,
		s_join_info.ap_desc.ssid.len,
		(A_UCHAR *) s_join_info.ap_desc.ssid.ssid,
		bssid, 0, ar->arConnectCtrlFlags) != A_OK)
	{
		DPRINTF(DBG_HTC, (DBGFMT "wmi_connect_cmd() failed.\n", DBGARG));
		rc = WLAN_SYS_RES_INVALID_PARAMS;
		goto auth_error;
	}

	return;

auth_error:

	/* Clear the data */
	htc_set_status_data(CONNECT_SLOT, (void *) 0);

	/* Clear the status */
	htc_clear_status(CONNECT_IN_PROGRESS);

	/* We can accept more commands */
	UNLOCK_COMMAND;

	/* Get the result back to the WLAN Adapter layer */
	wlan_adp_oem_auth_rsp(rc);

	return;
}

static void wlan_trp_adp_assoc_cmd(AR_SOFTC_T *ar, struct htc_request *req)
{
	wlan_adp_assoc_rsp_info_type info;

	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	/* Save the information */
	A_MEMCPY(&s_assoc_info, &req->req_info.assoc_info,
		sizeof(wlan_adp_assoc_req_info_type));

	/* What return value should we give back */
	info.result_code = htc_is_connected(ar) == TRUE ?
		WLAN_SYS_RES_SUCCESS : WLAN_SYS_RES_INVALID_PARAMS;

	/* Copy the WMM info back in the response */
	if(s_wmm_info_p != NULL)
	{
		/* We have one */
		A_MEMCPY(&info.wmm_info, s_wmm_info_p, sizeof(wlan_adp_wmm_info_type));
	}
	else
	{
		/* We don't have one so clear it out */
		A_MEMZERO(&info.wmm_info, sizeof(wlan_adp_wmm_info_type));
		info.wmm_info.wmm_element_type = WLAN_ADP_WMM_ELEMENT_TYPE_NONE;
	}

	/* We can accept more commands */
	UNLOCK_COMMAND;

	/* Get the result back to the WLAN Adapter layer */
	wlan_adp_oem_assoc_rsp(&info);

	return;
}

static void wlan_trp_adp_reassoc_cmd(AR_SOFTC_T *ar, struct htc_request *req)
{
	wlan_adp_reassoc_rsp_info_type info;
	A_UINT8 bssid[ATH_MAC_LEN];

	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	/* This is good until we know otherwise */
	info.result_code = WLAN_SYS_RES_SUCCESS;

	wlan_util_uint64_to_macaddress(bssid,
		req->req_info.reassoc_info.new_ap_mac_address);

	DPRINTF(DBG_HTC, (DBGFMT "BSS %02x:%02x:%02x:%02x:%02x:%02x.\n", DBGARG,
		bssid[0], bssid[1], bssid[2],
		bssid[3], bssid[4], bssid[5]));

	htc_set_status(CONNECT_IN_PROGRESS);
	htc_set_status_data(CONNECT_SLOT, (void *) 2);

	/* Reconnect */
	if(wmi_reconnect_cmd(ar->arWmi, bssid, 0) != A_OK)
	{
		DPRINTF(DBG_HTC, (DBGFMT "wmi_reconnect_cmd() failed.\n", DBGARG));

		/* Now we know better */
		info.result_code = WLAN_SYS_RES_INVALID_PARAMS;

		/* Clear the data */
		htc_set_status_data(CONNECT_SLOT, (void *) 0);

		/* Clear the status */
		htc_clear_status(CONNECT_IN_PROGRESS);

		/* We can accept more commands */
		UNLOCK_COMMAND;

		/* Get the result back to the WLAN Adapter layer */
		wlan_adp_oem_reassoc_rsp(&info);
	}

	return;
}

static void wlan_trp_adp_deauth_cmd(AR_SOFTC_T *ar, struct htc_request *req)
{
	wlan_dot11_result_code_enum rc = WLAN_SYS_RES_SUCCESS;

	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	htc_set_status(DISCONNECT_IN_PROGRESS);
	htc_set_status_data(DISCONNECT_SLOT, (void *) 2);

	/* Disconnect */
	if(wmi_disconnect_cmd(ar->arWmi) != A_OK)
	{
		DPRINTF(DBG_HTC,
			(DBGFMT "wmi_disconnect_cmd() failed.\n", DBGARG));

		rc = WLAN_SYS_RES_INVALID_PARAMS;

		/* Clear the data */
		htc_set_status_data(DISCONNECT_SLOT, (void *) 0);

		/* Clear the status */
		htc_clear_status(DISCONNECT_IN_PROGRESS);

		/* We can accept more commands */
		UNLOCK_COMMAND;

		/* Get the result back to the WLAN Adapter layer */
		wlan_adp_oem_deauth_rsp(rc);
	}

	return;
}

static void wlan_trp_adp_disassoc_cmd(AR_SOFTC_T *ar, struct htc_request *req)
{
	wlan_dot11_result_code_enum rc = WLAN_SYS_RES_SUCCESS;

	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	htc_set_status(DISCONNECT_IN_PROGRESS);
	htc_set_status_data(DISCONNECT_SLOT, (void *) 1);

	/* Disconnect */
	if(wmi_disconnect_cmd(ar->arWmi) != A_OK)
	{
		DPRINTF(DBG_HTC, (DBGFMT "wmi_disconnect_cmd() failed.\n", DBGARG));

		rc = WLAN_SYS_RES_INVALID_PARAMS;

		/* Clear the data */
		htc_set_status_data(DISCONNECT_SLOT, (void *) 0);

		/* Clear the status */
		htc_clear_status(DISCONNECT_IN_PROGRESS);

		/* We can accept more commands */
		UNLOCK_COMMAND;

		/* Get the result back to the WLAN Adapter layer */
		wlan_adp_oem_disassoc_rsp(rc);
	}

	return;
}

static void wlan_trp_adp_pwr_mgmt_cmd(AR_SOFTC_T *ar, struct htc_request *req)
{
	wlan_dot11_result_code_enum rc = WLAN_SYS_RES_SUCCESS;
	WMI_POWER_MODE pwr_mode;
	WMI_DTIM_POLICY dtimPolicy;

	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));
	pwr_mode = (req->req_info.pwr_mgmt_info.pwr_mode == WLAN_SYS_PWR_SAVE_MODE)
			  ?REC_POWER:MAX_PERF_POWER;

	if(wmi_powermode_cmd(ar->arWmi, pwr_mode) != A_OK)
		{
			DPRINTF(DBG_HTC,
				(DBGFMT "wmi_powermode_cmd() failed.\n", DBGARG));
			rc = WLAN_SYS_RES_INVALID_PARAMS;
		}

	if(req->req_info.pwr_mgmt_info.receiveDTIMs == TRUE)
	{
		dtimPolicy = STICK_DTIM;
	}
	else
	{
		dtimPolicy = NORMAL_DTIM;
	}
	
	/* We are always in power save mode so just */
	/* change the DTIM policy.                  */
	if(wmi_pmparams_cmd(ar->arWmi, 200, 0, dtimPolicy, 1, 0, SEND_POWER_SAVE_FAIL_EVENT_ALWAYS) != A_OK)
	{
		DPRINTF(DBG_HTC,
			(DBGFMT "wmi_pmparams_cmd() failed.\n", DBGARG));
		rc = WLAN_SYS_RES_INVALID_PARAMS;
	}

	/* We can accept more commands */
	UNLOCK_COMMAND;

	/* Get the result back to the WLAN Adapter layer */
	wlan_adp_oem_pwr_mgmt_rsp(rc);

	return;
}

static void wlan_trp_adp_mib_get_cmd(AR_SOFTC_T *ar, struct htc_request *req)
{
	static wlan_mib_item_value_type value;

	value.item_id = 0;

	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	/* We can accept more commands */
	UNLOCK_COMMAND;

	/* Get the result back to the WLAN Adapter layer */
	wlan_adp_oem_mib_get_rsp(WLAN_SYS_RES_NOT_SUPPORTED, &value);

	return;
}

static void wlan_trp_adp_mib_set_cmd(AR_SOFTC_T *ar, struct htc_request *req)
{
	wlan_dot11_result_code_enum rc = WLAN_SYS_RES_SUCCESS;
	wlan_mib_item_id_enum_type item_id = req->req_info.mib_info.item_id;
	int i;
	int len;
	A_UINT8 *keyp;

	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	switch(item_id)
	{
		case WLAN_MIB_AUTH_ALGO_ID:
			DPRINTF(DBG_HTC,
				(DBGFMT "WLAN_MIB_AUTH_ALGO_ID\n", DBGARG));

			/* Check for validity */
			if(req->req_info.mib_info.value.auth_algo !=
				WLAN_SYS_AUTH_ALGO_OPEN_SYSTEM &&
				req->req_info.mib_info.value.auth_algo !=
				WLAN_SYS_AUTH_ALGO_SHARED_KEY)
			{
				rc = WLAN_SYS_RES_INVALID_PARAMS;
				break;
			}

			/* Shared key 802.11 authentication is not supported */
			if(req->req_info.mib_info.value.auth_algo ==
				WLAN_SYS_AUTH_ALGO_SHARED_KEY)
			{
				rc = WLAN_SYS_RES_NOT_SUPPORTED;
				break;
			}

			s_auth_algo = req->req_info.mib_info.value.auth_algo;

			break;
		case WLAN_MIB_WEP_DEFAULT_KEYS_ID:
			DPRINTF(DBG_HTC,
				(DBGFMT "WLAN_MIB_WEP_DEFAULT_KEYS_ID\n", DBGARG));

			/* Check for validity */
			if(req->req_info.mib_info.value.default_keys.wep_type !=
				WLAN_MIB_WEP_40_BITS &&
				req->req_info.mib_info.value.default_keys.wep_type !=
				WLAN_MIB_WEP_104_BITS)
			{
				rc = WLAN_SYS_RES_INVALID_PARAMS;
				break;
			}

			/* Copy the keys */
			for(i = WMI_MIN_KEY_INDEX; i <= WMI_MAX_KEY_INDEX; i++)
			{
				if(req->req_info.mib_info.value.default_keys.wep_type ==
					WLAN_MIB_WEP_40_BITS)
				{
					len = WLAN_MIB_WEP_40_SIZE;
					keyp = req->req_info.mib_info.value.default_keys.wep_keys[i].wep_40;
				}
				else if(req->req_info.mib_info.value.default_keys.wep_type ==
					WLAN_MIB_WEP_104_BITS)
				{
					len = WLAN_MIB_WEP_104_SIZE;
					keyp = req->req_info.mib_info.value.default_keys.wep_keys[i].wep_104;
				}
				else
				{
					rc = WLAN_SYS_RES_INVALID_PARAMS;
					break;
				}

				A_MEMZERO(ar->arWepKeyList[i].arKey,
					sizeof(ar->arWepKeyList[i].arKey));
				A_MEMCPY(ar->arWepKeyList[i].arKey, keyp, len);
				ar->arWepKeyList[i].arKeyLen = len;
			}

			break;
		case WLAN_MIB_WEP_DEFAULT_KEY_INDEX_ID:
			DPRINTF(DBG_HTC,
				(DBGFMT "WLAN_MIB_WEP_DEFAULT_KEY_INDEX_ID\n", DBGARG));

			/* Check for validity */
			if(req->req_info.mib_info.value.uint32_value > 3)
			{
				rc = WLAN_SYS_RES_INVALID_PARAMS;
				break;
			}

			ar->arDefTxKeyIndex = req->req_info.mib_info.value.uint32_value;

			DPRINTF(DBG_HTC,
				(DBGFMT "Default Tx index %d\n", DBGARG, ar->arDefTxKeyIndex));

			break;
		case WLAN_MIB_SSID_ID:
			DPRINTF(DBG_HTC,
				(DBGFMT "WLAN_MIB_SSID_ID\n", DBGARG));

#if 0
			ar->arSsidLen = req->req_info.mib_info.value.ssid.len;
			A_MEMCMP(ar->arSsid, req->req_info.mib_info.value.ssid.ssid,
				ar->arSsidLen);
#endif

			break;
		default:
			DPRINTF(DBG_HTC,
				(DBGFMT "Unsupported item id %d.\n", DBGARG, item_id));
			rc = WLAN_SYS_RES_NOT_SUPPORTED;
			break;
	}

	/* We can accept more commands */
	UNLOCK_COMMAND;

	/* Get the result back to the WLAN Adapter layer */
	wlan_adp_oem_mib_set_rsp(rc, item_id);

	return;
}

static void wlan_trp_adp_opr_get_cmd(AR_SOFTC_T *ar, struct htc_request *req)
{
	int perror = 0;	/* 0 = success, -1 = failure */
	wlan_dot11_opr_param_id_enum op = req->req_info.opr_param_id;
	wlan_dot11_opr_param_value_type value;
	int what = 0;
	int stati = 0;

	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	/* Clear the value */
	value.u32_value = 0;

	switch(op)
	{
		case WLAN_RSSI:
			what = 1; stati = 1;
			break;
		case WAN_LOGS:
			/* Not supported for now */
			perror = -1;
			break;
		case WLAN_TX_FRAME_CNT:
			what = 1; stati = 2;
			break;
		case WLAN_RX_FRAME_CNT:
			what = 1; stati = 3;
			break;
		case WLAN_TX_RATE:
			what = 2;
			break;
		case WLAN_TOTAL_RX_BYTE:
			what = 1; stati = 4;
			break;
		default:
			DPRINTF(DBG_HTC, (DBGFMT "Unsupported parameter %d\".\n", DBGARG,
				op));
			perror = -1;
			break;
	}
			
	/* What should we do ? */
	if(what == 1)
	{
		/* We need to get statistics to answer this */

		/* We are waiting for stats */
		htc_set_status(STATS_IN_PROGRESS);
		htc_set_status_data(STATS_SLOT, (void *) stati);

		if(wmi_get_stats_cmd(ar->arWmi) != A_OK)
		{
			DPRINTF(DBG_HTC,
				(DBGFMT "wmi_get_stats_cmd() failed.\n", DBGARG));
			htc_clear_status(STATS_IN_PROGRESS);
			perror = -1;
		}
	}
	else if(what == 2)
	{
		/* How handy with this bitrate support */

		/* We are waiting bit rate information */
		htc_set_status(BITRATE_IN_PROGRESS);

		if(wmi_get_bitrate_cmd(ar->arWmi) != A_OK)
		{
			DPRINTF(DBG_HTC, (DBGFMT "wmi_get_bitrate_cmd() failed.\n", DBGARG));
			htc_clear_status(BITRATE_IN_PROGRESS);
			perror = -1;
		}
	}
	else
	{
		/* We can accept more commands */
		UNLOCK_COMMAND;

		/* Get the value back to the WLAN Adapter layer */
		if(perror == 0)
		{
			wlan_adp_oem_opr_param_get_rsp(op, value);
		}
	}

	return;
}

static void wlan_trp_adp_reset_cmd(AR_SOFTC_T *ar, struct htc_request *req)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	/* Not used currently */

	/* We can accept more commands */
	UNLOCK_COMMAND;

	/* Get the result back to the WLAN Adapter layer */
	wlan_adp_oem_reset_rsp(WLAN_SYS_RES_SUCCESS);

	return;
}

static void wlan_trp_adp_mac_cfg_cmd(AR_SOFTC_T *ar, struct htc_request *req)
{
	int perror = 0;	/* 0 = success, -1 = failure */

	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

#if 0
	/* Not supported */
	req->req_info.config_info.frag_threshold;
#endif

	/* Only in disconnected state */
	if(htc_is_connected(ar) == FALSE)
	{
        A_UINT8 lpreamble = (req->req_info.config_info.preamble_type 
                             == WLAN_DOT11_PHY_PREAMBLE_LONG) ? TRUE : FALSE;

        if (wmi_set_lpreamble_cmd(ar->arWmi, lpreamble, WMI_IGNORE_BARKER_IN_ERP) != A_OK)
        {
            DPRINTF(DBG_HTC, (DBGFMT "wmi_set_lpreamble_cmd() failed.\n", 
                    DBGARG));
            perror = -1;
        }
        
		/* In dbM, 0 => Max allowed by domain */
		if(wmi_set_txPwr_cmd(ar->arWmi,
			req->req_info.config_info.max_tx_pwr) != A_OK)
		{
			DPRINTF(DBG_HTC, (DBGFMT "wmi_set_txPwr_cmd() failed.\n", DBGARG));
			perror = -1;
		}
		
		ar->arTxPwr = req->req_info.config_info.max_tx_pwr;
		ar->arTxPwrSet = TRUE;
		
        /* Qcom passes in terms of number of beacon period */
        if(wmi_listeninterval_cmd(ar->arWmi, 0,
           req->req_info.config_info.listen_interval) != A_OK)
		{
			DPRINTF(DBG_HTC, (DBGFMT "wmi_listeninterval_cmd() failed.\n", 
					DBGARG));
			perror = -1;
		}
        
        if (wmi_set_rts_cmd(ar->arWmi,
            req->req_info.config_info.rts_threshold) != A_OK)
        {
            DPRINTF(DBG_HTC, (DBGFMT "wmi_set_rts_cmd() failed.\n", 
                    DBGARG));
            perror = -1;
        }
	}
	else
	{
		DPRINTF(DBG_HTC,
			(DBGFMT "Operation not supported in connected state.\n", DBGARG));
		perror = -1;
	}

	/* We can accept more commands */
	UNLOCK_COMMAND;

	/* Tell the WLAN Adapter layer that we are done */
	wlan_adp_oem_mac_config_rsp(perror);

	return;
}

static void wlan_trp_adp_wpa_setkeys_cmd(AR_SOFTC_T *ar, struct htc_request *req)
{
	int perror = 0;	/* 0 = success, -1 = failure */
	int num_keys;
	int i;

	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	num_keys = min(req->req_info.setkeys_info.num_keys, 4);

	/* Add the keys */
	for(i = 0; i < num_keys; i++)
	{
		CRYPTO_TYPE keyType;
		KEY_USAGE keyUsage;

		if(req->req_info.setkeys_info.keys[i].key_type ==
			WLAN_DOT11_WPA_KEY_TYPE_P)
		{
			keyUsage = PAIRWISE_USAGE;
		}
		else if(req->req_info.setkeys_info.keys[i].key_type ==
			WLAN_DOT11_WPA_KEY_TYPE_G_S)
		{
			keyUsage = GROUP_USAGE;
		}
		else
		{
			DPRINTF(DBG_HTC, (DBGFMT "Invalid key type %d.\n", DBGARG,
				req->req_info.setkeys_info.keys[i].key_type));
			perror = -1;
			break;
		}

		if(req->req_info.setkeys_info.keys[i].cipher_suite_selector[3] == 1)
		{
			/* WEP-40 */	/* Defined by key length */
			keyType = WEP_CRYPT;
		}
		else if(req->req_info.setkeys_info.keys[i].cipher_suite_selector[3] == 2)
		{
			/* TKIP */
			keyType = TKIP_CRYPT;
		}
		else if(req->req_info.setkeys_info.keys[i].cipher_suite_selector[3] == 4)
		{
			/* CCMP */
			keyType = AES_CRYPT;
		}
		else if(req->req_info.setkeys_info.keys[i].cipher_suite_selector[3] == 1)
		{
			/* WEP-104 */	/* Defined by key length */
			keyType = WEP_CRYPT;
		}
		else
		{
			DPRINTF(DBG_HTC, (DBGFMT "Unsupported cipher %d.\n", DBGARG,
				req->req_info.setkeys_info.keys[i].cipher_suite_selector[3]));
			perror = -1;
			break;
		}

		if (IEEE80211_CIPHER_CCKM_KRK != keyType) {
			if(wmi_addKey_cmd(ar->arWmi,
				req->req_info.setkeys_info.keys[i].key_id & 0x3,
				keyType,
				keyUsage,
				req->req_info.setkeys_info.keys[i].key_length,
				req->req_info.setkeys_info.keys[i].rsc,
				req->req_info.setkeys_info.keys[i].key,
				KEY_OP_INIT_VAL, NULL, SYNC_BEFORE_WMIFLAG) != A_OK)
			{
				DPRINTF(DBG_HTC, (DBGFMT "wmi_addKey_cmd() failed.\n", DBGARG));
	
				perror = -1;
				break;
			}
		}
		else
		{
			if (wmi_add_krk_cmd(ar->arWmi, 
								req->req_info.setkeys_info.keys[i].key) != A_OK)
			{
				DPRINTF(DBG_HTC, (DBGFMT "wmi_add_krk_cmd() failed.\n", DBGARG));
	
				perror = -1;
				break;
			}
		}
	}

	/* We can accept more commands */
	UNLOCK_COMMAND;

	/* Notify the WLAN Adapter layer that the */
	/* keys are appropriately installed        */
	if(perror == 0)
	{
		wlan_adp_oem_wpa_setkeys_rsp();
	}

	return;
}

static void wlan_trp_adp_wpa_delkeys_cmd(AR_SOFTC_T *ar, struct htc_request *req)
{
	int perror = 0;	/* 0 = success, -1 = failure */
	int num_keys;
	int i;

	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	num_keys = min(req->req_info.delkeys_info.num_keys, 4);

	/* Delete the keys */
	for(i = 0; i < num_keys; i++)
	{
		if(wmi_deleteKey_cmd(ar->arWmi,
			req->req_info.delkeys_info.keys[i].key_id & 0x3))
		{
			DPRINTF(DBG_HTC,
				(DBGFMT "wmi_deleteKey_cmd() failed.\n", DBGARG));
			perror = -1;
			break;
		}
	}

	/* We can accept more commands */
	UNLOCK_COMMAND;

	/* Notify the WLAN Adapter layer */
	if(perror == 0)
	{
		wlan_adp_oem_wpa_delkeys_rsp();
	}

	return;
}

static void wlan_trp_adp_wpa_setprotect_cmd(AR_SOFTC_T *ar, struct htc_request *req)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	/* Save the information */
	A_MEMCPY(&s_protect_info, &req->req_info.protect_info,
		sizeof(wlan_adp_wpa_protect_list_type));

	/* We can accept more commands */
	UNLOCK_COMMAND;

	/* Notify the WLAN Adapter layer */
	wlan_adp_oem_wpa_set_protection_rsp();

	return;
}

static void wlan_trp_adp_addts_cmd(AR_SOFTC_T *ar, struct htc_request *req)
{
	wlan_adp_addts_rsp_info_type info;
	A_UINT8  trafficClass;
	A_UINT8  trafficDirection;
	A_UINT8  voicePSCapability;
    QCOM_Q_ID qId;
	WMI_CREATE_PSTREAM_CMD cmd;
	wlan_adp_wmm_tspec_type* tspec = &req->req_info.addts_info.tspec;
	A_STATUS ret;
	
	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	info.dialog_token = req->req_info.addts_info.dialog_token;
	A_MEMCPY(&info.tspec, &req->req_info.addts_info.tspec,
		sizeof(wlan_adp_wmm_tspec_type));
	info.result_code = WLAN_SYS_RES_SUCCESS;

	/* Which mailbox/endpoint does this request map to */
	qId = up2qid(req->req_info.addts_info.tspec.ts_info.up);
    
	/* No need to do anything if the service maps to the */
	/* best effort service                               */
	if(qId == QCOM_Q_BE)
	{
		/* We can accept more commands */
		UNLOCK_COMMAND;

		/* Notify the WLAN Adapter layer */
		wlan_adp_oem_addts_rsp(&info);

		return;
	}

    trafficClass = convert_userPriority_to_trafficClass(
                       req->req_info.addts_info.tspec.ts_info.up);

	switch(req->req_info.addts_info.tspec.ts_info.direction)
	{ 
		case WLAN_ADP_WMM_TS_DIR_UPLINK:
			trafficDirection = UPLINK_TRAFFIC ;
			break;
		case WLAN_ADP_WMM_TS_DIR_DOWNLINK:
			trafficDirection = DNLINK_TRAFFIC  ;
			break;
		case  WLAN_ADP_WMM_TS_DIR_BOTH:
			trafficDirection =  BIDIR_TRAFFIC  ;
			break;
		case WLAN_ADP_WMM_TS_DIR_RESV:
		case WLAN_ADP_WMM_TS_DIR_MAX:
			DPRINTF( DBG_HTC, (DBGFMT "Invalid Traffic Direction\n",DBGARG));
			break;
		
	}

	if(req->req_info.addts_info.tspec.ts_info.psb)
	{
		voicePSCapability = ENABLE_FOR_THIS_AC;
	}
	else
	{
		voicePSCapability = DISABLE_FOR_THIS_AC;
	}

	DPRINTF(DBG_HTC, (DBGFMT "Sending ADDTS for ac %d.\n", DBGARG, trafficClass));

	cmd.trafficClass = trafficClass;
	cmd.trafficDirection = trafficDirection;
	cmd.trafficType = 1;   /* TODO: no trafficType in QCOM */
	cmd.voicePSCapability = voicePSCapability;
    cmd.tsid = tspec->ts_info.tid;
	cmd.userPriority = tspec->ts_info.up;
    cmd.nominalMSDU = tspec->nominal_msdu_size;             /* in octects */
    cmd.maxMSDU = tspec->maximum_msdu_size;                 /* in octects */
    cmd.minServiceInt = tspec->min_service_interval;           /* in milli-sec */
    cmd.maxServiceInt = tspec->max_service_interval;           /* in milli-sec */
    cmd.inactivityInt = tspec->inactivity_interval;           /* in milli-sec */
    cmd.suspensionInt = tspec->suspension_interval;           /* in milli-sec */
    cmd.serviceStartTime = tspec->svc_start_time;
    cmd.minDataRate = tspec->min_data_rate;             /* in bps */
    cmd.meanDataRate = tspec->mean_data_rate;            /* in bps */
    cmd.peakDataRate = tspec->peak_data_rate;            /* in bps */
    cmd.maxBurstSize = tspec->max_burst_size;
    cmd.delayBound = tspec->delay_bound;
    cmd.minPhyRate = tspec->min_phy_rate;              /* in bps */
    cmd.sba = tspec->surplus_bw_allowance;
    cmd.mediumTime = tspec->medium_time;    

    ret = wmi_verify_tspec_params(&cmd, tspecCompliance);
    
	/* Create the stream */
	if((ret != A_OK) || 
	   (A_OK != wmi_create_pstream_cmd(ar->arWmi, &cmd)))
	{
		DPRINTF(DBG_HTC,
			(DBGFMT "wmi_create_pstream_cmd() failed.\n", DBGARG));

        /* Report an error */
        info.result_code = WLAN_SYS_RES_INVALID_PARAMS;
    }

    /* We can accept more commands */
    UNLOCK_COMMAND;

    wlan_adp_oem_addts_rsp(&info);
}

static void wlan_trp_adp_delts_cmd(AR_SOFTC_T *ar, struct htc_request *req)
{
	wlan_adp_delts_rsp_info_type info;
	A_UINT8 direction;
    A_UINT8 trafficClass;

	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	DPRINTF(DBG_HTC, (DBGFMT "Reason %d.\n", DBGARG,
		req->req_info.delts_info.reason_code));

	A_MEMCPY(&info.ts_info, &req->req_info.delts_info.ts_info,
		sizeof(wlan_adp_wmm_ts_info_type));
	info.result_code = WLAN_SYS_RES_SUCCESS;

    trafficClass = convert_userPriority_to_trafficClass(
                       req->req_info.delts_info.ts_info.up);
    
	DPRINTF(DBG_HTC, (DBGFMT "Deleting stream for traffic class %u %d.\n",
            DBGARG, req->req_info.delts_info.ts_info.up, trafficClass));

	switch (req->req_info.delts_info.ts_info.direction)
	{
		case WLAN_ADP_WMM_TS_DIR_UPLINK:
			direction = UPLINK_TRAFFIC;
			break;
		
		case WLAN_ADP_WMM_TS_DIR_DOWNLINK:
			direction = DNLINK_TRAFFIC;
			break;
		
		case WLAN_ADP_WMM_TS_DIR_BOTH:
			direction = BIDIR_TRAFFIC;
			break;

		default:
			DPRINTF(DBG_HTC, (DBGFMT "Invalid Traffic direction.\n", DBGARG));
			
			/* Tell the guy upstairs */
			info.result_code = WLAN_SYS_RES_INVALID_PARAMS;
			goto out;
	}
	
	/* Delete the stream */
	if(wmi_delete_pstream_cmd(ar->arWmi,
							  trafficClass,
							  req->req_info.delts_info.ts_info.tid))
	{
		DPRINTF(DBG_HTC,
			(DBGFMT "wmi_delete_pstream_cmd() failed.\n", DBGARG));
	}

  out:
    
	/* We can accept more commands */
	UNLOCK_COMMAND;

	/* Notify the WLAN Adapter layer */
	wlan_adp_oem_delts_rsp(&info);

	return;
}

#ifdef CONFIG_HOST_TCMD_SUPPORT
static void wlan_trp_adp_test_cmd( AR_SOFTC_T  *ar, struct htc_request * req)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));
	if(wmi_test_cmd(ar->arWmi, (A_UINT8 *)&req->req_info.ftm_test_data.data, 
		req->req_info.ftm_test_data.len) != A_OK)
	{
		DPRINTF(DBG_HTC,
			(DBGFMT "wmi_test_cmd() failed.\n", DBGARG));
	}
	
	/* We can accept more commands */
	UNLOCK_COMMAND;
}
#endif

static void wlan_trp_adp_bthci_cmd(AR_SOFTC_T *ar, struct htc_request *req)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	wmi_send_hci_cmd(ar->arWmi, req->req_info.bthci_info.buf,
                     req->req_info.bthci_info.size);

	/* We can accept more commands */
	UNLOCK_COMMAND;
}

static void wlan_trp_adp_bt_dispatcher_cmd(AR_SOFTC_T *ar, struct htc_request *req)
{
    DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

    ar->evt_dispatcher = req->req_info.btdispatcher_info.evt_dispatcher;
    ar->data_dispatcher = req->req_info.btdispatcher_info.data_dispatcher;   

    /* We can accept more commands */
    UNLOCK_COMMAND;
}

/*****************************************************************************/
/** End of command functions                                                **/
/*****************************************************************************/

static int state_open_command_sig(void)
{
	struct htc_request *req;
	AR_SOFTC_T *ar = (AR_SOFTC_T *) s_device;

	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	/* Make sure WMI layer is ready */
	if(ar->arWmiReady == FALSE)
	{
		/* It is not ready */
		DPRINTF(DBG_HTC, (DBGFMT "WMI layer is not ready.\n", DBGARG));

		/* Make sure we indicate a failure */
		return(-1);
	}

	/* Process the whole queue */
	while((req = dequeue_htc_request()) != NULL)
	{
		switch(req->cmd)
		{
			case WLAN_TRP_ADP_SUSPEND_CMD:
				DPRINTF(DBG_HTC,
					(DBGFMT "Command \"WLAN_TRP_ADP_SUSPEND_CMD\".\n", DBGARG));
				start_command_timeout(WLAN_TRP_ADP_SUSPEND_CMD, 1000);
				wlan_trp_adp_suspend_cmd(ar, req);
				break;
			case WLAN_TRP_ADP_RESUME_CMD:
				DPRINTF(DBG_HTC,
					(DBGFMT "Command \"WLAN_TRP_ADP_RESUME_CMD\".\n", DBGARG));
				start_command_timeout(WLAN_TRP_ADP_RESUME_CMD, 1000);
				wlan_trp_adp_resume_cmd(ar, req);
				break;
			case WLAN_TRP_ADP_SCAN_CMD:
				DPRINTF(DBG_HTC,
					(DBGFMT "Command \"WLAN_TRP_ADP_SCAN_CMD\".\n", DBGARG));
				start_command_timeout(WLAN_TRP_ADP_SCAN_CMD, 1000);
				wlan_trp_adp_scan_cmd(ar, req);
				break;
			case WLAN_TRP_ADP_JOIN_CMD:
				DPRINTF(DBG_HTC,
					(DBGFMT "Command \"WLAN_TRP_ADP_JOIN_CMD\".\n", DBGARG));
				start_command_timeout(WLAN_TRP_ADP_JOIN_CMD, 1000);
				wlan_trp_adp_join_cmd(ar, req);
				break;
			case WLAN_TRP_ADP_AUTH_CMD:
				DPRINTF(DBG_HTC,
					(DBGFMT "Command \"WLAN_TRP_ADP_AUTH_CMD\".\n", DBGARG));
				start_command_timeout(WLAN_TRP_ADP_AUTH_CMD, 1000);
				wlan_trp_adp_auth_cmd(ar, req);
				break;
			case WLAN_TRP_ADP_ASSOC_CMD:
				DPRINTF(DBG_HTC,
					(DBGFMT "Command \"WLAN_TRP_ADP_ASSOC_CMD\".\n", DBGARG));
				start_command_timeout(WLAN_TRP_ADP_ASSOC_CMD, 1000);
				wlan_trp_adp_assoc_cmd(ar, req);
				break;
			case WLAN_TRP_ADP_REASSOC_CMD:
				DPRINTF(DBG_HTC,
					(DBGFMT "Command \"WLAN_TRP_ADP_REASSOC_CMD\".\n", DBGARG));
				start_command_timeout(WLAN_TRP_ADP_REASSOC_CMD, 1000);
				wlan_trp_adp_reassoc_cmd(ar, req);
				break;
			case WLAN_TRP_ADP_DEAUTH_CMD:
				DPRINTF(DBG_HTC,
					(DBGFMT "Command \"WLAN_TRP_ADP_DEAUTH_CMD\".\n", DBGARG));
				start_command_timeout(WLAN_TRP_ADP_DEAUTH_CMD, 1000);
				wlan_trp_adp_deauth_cmd(ar, req);
				break;
			case WLAN_TRP_ADP_DISASSOC_CMD:
				DPRINTF(DBG_HTC,
					(DBGFMT "Command \"WLAN_TRP_ADP_DISASSOC_CMD\".\n", DBGARG));
				start_command_timeout(WLAN_TRP_ADP_DISASSOC_CMD, 1000);
				wlan_trp_adp_disassoc_cmd(ar, req);
				break;
			case WLAN_TRP_ADP_PWR_MGMT_CMD:
				DPRINTF(DBG_HTC,
					(DBGFMT "Command \"WLAN_TRP_ADP_PWR_MGMT_CMD\".\n", DBGARG));
				start_command_timeout(WLAN_TRP_ADP_PWR_MGMT_CMD, 1000);
				wlan_trp_adp_pwr_mgmt_cmd(ar, req);
				break;
			case WLAN_TRP_ADP_MIB_GET_CMD:
				DPRINTF(DBG_HTC,
					(DBGFMT "Command \"WLAN_TRP_ADP_MIB_GET_CMD\".\n", DBGARG));
				start_command_timeout(WLAN_TRP_ADP_MIB_GET_CMD, 1000);
				wlan_trp_adp_mib_get_cmd(ar, req);
				break;
			case WLAN_TRP_ADP_MIB_SET_CMD:
				DPRINTF(DBG_HTC,
					(DBGFMT "Command \"WLAN_TRP_ADP_MIB_SET_CMD\".\n", DBGARG));
				start_command_timeout(WLAN_TRP_ADP_MIB_SET_CMD, 1000);
				wlan_trp_adp_mib_set_cmd(ar, req);
				break;
			case WLAN_TRP_ADP_OPR_GET_CMD:
				DPRINTF(DBG_HTC,
					(DBGFMT "Command \"WLAN_TRP_ADP_OPR_GET_CMD\".\n", DBGARG));
				start_command_timeout(WLAN_TRP_ADP_OPR_GET_CMD, 1000);
				wlan_trp_adp_opr_get_cmd(ar, req);
				break;
			case WLAN_TRP_ADP_RESET_CMD:
				DPRINTF(DBG_HTC,
					(DBGFMT "Command \"WLAN_TRP_ADP_RESET_CMD\".\n", DBGARG));
				start_command_timeout(WLAN_TRP_ADP_RESET_CMD, 1000);
				wlan_trp_adp_reset_cmd(ar, req);
				break;
			case WLAN_TRP_ADP_MAC_CFG_CMD:
				DPRINTF(DBG_HTC,
					(DBGFMT "Command \"WLAN_TRP_ADP_MAC_CFG_CMD\".\n", DBGARG));
				start_command_timeout(WLAN_TRP_ADP_MAC_CFG_CMD, 1000);
				wlan_trp_adp_mac_cfg_cmd(ar, req);
				break;
			case WLAN_TRP_ADP_WPA_SETKEYS_CMD:
				DPRINTF(DBG_HTC,
					(DBGFMT "Command \"WLAN_TRP_ADP_WPA_SETKEYS_CMD\".\n", DBGARG));
				start_command_timeout(WLAN_TRP_ADP_WPA_SETKEYS_CMD, 1000);
				wlan_trp_adp_wpa_setkeys_cmd(ar, req);
				break;
			case WLAN_TRP_ADP_WPA_DELKEYS_CMD:
				DPRINTF(DBG_HTC,
					(DBGFMT "Command \"WLAN_TRP_ADP_WPA_DELKEYS_CMD\".\n", DBGARG));
				start_command_timeout(WLAN_TRP_ADP_WPA_DELKEYS_CMD, 1000);
				wlan_trp_adp_wpa_delkeys_cmd(ar, req);
				break;
			case WLAN_TRP_ADP_WPA_SETPROTECT_CMD:
				DPRINTF(DBG_HTC,
					(DBGFMT "Command \"WLAN_TRP_ADP_WPA_SETPROTECT_CMD\".\n", DBGARG));
				start_command_timeout(WLAN_TRP_ADP_WPA_SETPROTECT_CMD, 1000);
				wlan_trp_adp_wpa_setprotect_cmd(ar, req);
				break;
			case WLAN_TRP_ADP_ADDTS_CMD:
				DPRINTF(DBG_HTC,
					(DBGFMT "Command \"WLAN_TRP_ADP_ADDTS_CMD\".\n", DBGARG));
				start_command_timeout(WLAN_TRP_ADP_ADDTS_CMD, 1000);
				wlan_trp_adp_addts_cmd(ar, req);
				break;
			case WLAN_TRP_ADP_DELTS_CMD:
				DPRINTF(DBG_HTC,
					(DBGFMT "Command \"WLAN_TRP_ADP_DELTS_CMD\".\n", DBGARG));
				start_command_timeout(WLAN_TRP_ADP_DELTS_CMD, 1000);
				wlan_trp_adp_delts_cmd(ar, req);
				break;
			case WLAN_TRP_ADP_BTHCI_CMD:
				DPRINTF(DBG_HTC,
					(DBGFMT "Command \"WLAN_TRP_ADP_BTHCI_CMD\".\n", DBGARG));
				start_command_timeout(WLAN_TRP_ADP_BTHCI_CMD, 1000);
				wlan_trp_adp_bthci_cmd(ar, req);
				break;
			case WLAN_TRP_ADP_BT_DISPATCHER_CMD:
				DPRINTF(DBG_HTC,
					(DBGFMT "Command \"WLAN_TRP_ADP_BT_DISPATCHER_CMD\".\n", DBGARG));
				start_command_timeout(WLAN_TRP_ADP_BT_DISPATCHER_CMD, 1000);
				wlan_trp_adp_bt_dispatcher_cmd(ar, req);
				break;
			default:
				DPRINTF(DBG_HTC,
					(DBGFMT "Command <unknown> %d.\n", DBGARG, req->cmd));

				/* We can accept more commands */
				UNLOCK_COMMAND;

				break;
		}

		/* We are done with this guy */
		htc_free_request(req);
	}

	return(0);
}

static int state_open_rx_data_sig(void)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	return(0);
}

static int state_open_tx_data_sig(void)
{
	unsigned int return_signal = 0;
	QCOM_Q_ID qId;
	void *pkt;
    A_UINT8 up;
    A_UINT32 len;
    A_UINT8 *pkt_data;
    int ret;
    
	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	if((pkt = dequeue_htc_data(&qId)) != NULL)
	{
		if (qId != QCOM_Q_BT) {
            len = A_NETBUF_LEN(pkt);
            pkt_data = A_NETBUF_VIEW_DATA(pkt, A_UINT8 *, len);
            up = *(pkt_data + len);
        
            ret = qcom_driver_tx(pkt, s_device, up);
		}
		else {
			ret = qcom_driver_tx_bt(pkt, s_device);
		}
		
		/* Send this packet */
		switch(ret)
		{
			case 1:
				/* Retransmit */
				DPRINTF(DBG_HTC, (DBGFMT "Retransmit.\n", DBGARG));
				prequeue_htc_data(pkt, qId);
				break;
			case 2:
				/* Drop */
				DPRINTF(DBG_HTC, (DBGFMT "Drop.\n", DBGARG));
				return_signal |= HTC_TX_DATA_SIG;
				break;
			default:
				/* All is good */
				return_signal |= HTC_TX_DATA_SIG;
				break;
		}
	}
	else
	{
		int status;

		/* Make sure we can accept more data */
		A_MUTEX_LOCK(&s_flags_lock);
		status = s_flags & FLAG_TX_READY ? TRUE : FALSE;
		A_MUTEX_UNLOCK(&s_flags_lock);

		/* See if we have more data from the ADP layer */
		if(status == TRUE && adp_pull_data() == 1)
		{
			DPRINTF(DBG_HTC,
				(DBGFMT "Got more data from ADP layer.\n", DBGARG));
			return_signal |= HTC_TX_DATA_SIG;
		}
	}

	DPRINTF(DBG_HTC, (DBGFMT "Exit.\n", DBGARG));

	return(return_signal);
}

static int (*s_state_open[SIGNAL_INDEX_TABLE_SIZE])(void) = 
{
	/* HTC_START_SIG      */	NULL,
	/* HTC_STOP_SIG       */	state_open_stop_sig,
	/* HTC_COMMAND_SIG    */	state_open_command_sig,
	/* HTC_READY_SIG      */	NULL,
	/* HTC_HIFDSR_SIG     */	state_open_hifdsr_sig,
	/* HTC_FAIL_SIG       */	state_open_fail_sig,
	/* HTC_TX_DATA_SIG    */	state_open_tx_data_sig,
	/* HTC_RX_DATA_SIG    */	state_open_rx_data_sig,
};

#ifdef CONFIG_HOST_TCMD_SUPPORT
static int state_test_ready_sig(void)
{
	DPRINTF(DBG_HTC, (DBGFMT " Enter. \n", DBGARG));

	A_MUTEX_LOCK(&s_flags_lock);
	s_flags |= FLAG_CMD_READY ;
	A_MUTEX_UNLOCK(&s_flags_lock);

	set_state(HTC_STATE_TEST_OPEN);
	
	/* Tell the WLAN Adapter layer that we are ready */
    wlan_adp_oem_ready_ind(0);

	return (0);
}

static int state_test_hifdsr_sig(void)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));
	HIFHandleIRQ();
	return(0);
}

/* HTC TASK stays in state, until we get ready signal from the target 
   that HTC is ready.
*/

static int (*s_state_test[SIGNAL_INDEX_TABLE_SIZE])(void)=
{
    /* HTC_START_SIG      */    NULL,
    /* HTC_STOP_SIG       */    NULL,
    /* HTC_COMMAND_SIG    */    NULL,
    /* HTC_READY_SIG      */    state_test_ready_sig,
    /* HTC_HIFDSR_SIG     */    state_test_hifdsr_sig,
    /* HTC_FAIL_SIG       */    state_init_fail_sig,
    /* HTC_TX_DATA_SIG    */    NULL,
    /* HTC_RX_DATA_SIG    */    NULL,
};

static int state_test_open_cmd_sig(void)
{
	struct htc_request *req;
	AR_SOFTC_T *ar = (AR_SOFTC_T *) s_device;

	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	while((req = dequeue_htc_request()) != NULL)
	{
		switch(req->cmd)
		{
			case WLAN_TRP_ADP_TEST_CMD:
				DPRINTF(DBG_HTC,
				(DBGFMT "Command \"WLAN_TRP_ADP_TEST_CMD\".\n", DBGARG));
				start_command_timeout(WLAN_TRP_ADP_TEST_CMD, 1000);
				wlan_trp_adp_test_cmd(ar, req);
				break;
			default:
				DPRINTF(DBG_HTC,
					(DBGFMT "Command <unknown> %d in test mode.\n", DBGARG,
					req->cmd));
				
				/* We can accept more commands */
				UNLOCK_COMMAND;

				break;
		}

		/* We are done with this guy */
		htc_free_request(req);
	}

	return (0);
}

static int state_test_open_hifdsr_sig(void)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	HIFHandleIRQ();

	return (0);
}

static int state_test_open_stop_sig(void)
{
	DPRINTF(DBG_HTC, (DBGFMT "Enter.\n", DBGARG));

	/* Clear any timeouts if we had any */
	stop_command_timeout();

	/* Close driver */
	qcom_driver_close(s_device);

	/* The card has been "removed" */
	HIFHandleRemoved();

	/* Set the initial state */
	set_state(HTC_STATE_START);

	return(0);
}

/* This state is analogous to s_state_open and accepts only test commands */
static int (*s_state_test_open[SIGNAL_INDEX_TABLE_SIZE])(void) =
{
    /* HTC_START_SIG      */    NULL,
    /* HTC_STOP_SIG       */    state_test_open_stop_sig,
    /* HTC_COMMAND_SIG    */    state_test_open_cmd_sig,
    /* HTC_READY_SIG      */    NULL,
    /* HTC_HIFDSR_SIG     */    state_test_open_hifdsr_sig,
    /* HTC_FAIL_SIG       */    state_init_fail_sig,
    /* HTC_TX_DATA_SIG    */    NULL,
    /* HTC_RX_DATA_SIG    */    NULL,
};
#endif

/*****************************************************************************/
/** State idle                                                              **/
/*****************************************************************************/

static int (*s_state_idle[SIGNAL_INDEX_TABLE_SIZE])(void) = 
{
	/* HTC_START_SIG      */	NULL,
	/* HTC_STOP_SIG       */	NULL,
	/* HTC_COMMAND_SIG    */	NULL,
	/* HTC_READY_SIG      */	NULL,
	/* HTC_HIFDSR_SIG     */	NULL,
	/* HTC_FAIL_SIG       */	NULL,
	/* HTC_TX_DATA_SIG    */	NULL,
	/* HTC_RX_DATA_SIG    */	NULL,
};

/*****************************************************************************/
/** State processing and state manipulation functions                       **/
/*****************************************************************************/

static struct state_item
{
	unsigned int state;
	char *description;
	int (*(*table)[SIGNAL_INDEX_TABLE_SIZE])(void);
}
s_state_table[] =
{
	{
		/* We want to keep this state first in the table */
		/* so we can find it easily for initialization   */
		HTC_STATE_IDLE,
		"HTC_STATE_IDLE",
		&s_state_idle,
	},
	{
		HTC_STATE_START,
		"HTC_STATE_START",
		&s_state_start,
	},
	{
		HTC_STATE_CONFIG,
		"HTC_STATE_CONFIG",
		&s_state_config,
	},
	{
		HTC_STATE_OPEN,
		"HTC_STATE_OPEN",
		&s_state_open,
	},
	{
		HTC_STATE_DROP,
		"HTC_STATE_DROP",
		&s_state_drop,
	},
#ifdef CONFIG_HOST_TCMD_SUPPORT
	{
		HTC_STATE_TEST,
		"HTC_STATE_TEST",
		&s_state_test,
	},
	{
		HTC_STATE_TEST_OPEN,
		"HTC_STATE_TEST_OPEN",
		&s_state_test_open,
	},
#endif
};

#define STATE_TABLE_SIZE (sizeof(s_state_table) / sizeof(struct state_item))

/* Initialize the state to the idle state at startup */
/* just so we don't end up with a NULL pointer.      */
static struct state_item *s_current_state = &s_state_table[0];

static void set_state(int state)
{
	int i;

	/* Search our state table */
	for(i = 0; i < STATE_TABLE_SIZE; i++)
	{
		/* Is this the requested state */
		if(s_state_table[i].state == state)
		{
			/* Yes it is so set the state */

			DPRINTF(DBG_HTC, (DBGFMT "State \"%s\"\n", DBGARG,
				s_state_table[i].description));

			/* Set the current state */
			s_current_state = &s_state_table[i];
			
			/* We are done */
			return;
		}
	}

	DPRINTF(DBG_HTC, (DBGFMT "Invalid state %d.\n", DBGARG, state));

	return;
}

/*****************************************************************************/
/*****************************************************************************/
/**                                                                         **/
/** HTC task                                                                **/
/**                                                                         **/
/** This section is the body of the HTC task itself                         **/
/**                                                                         **/
/*****************************************************************************/
/*****************************************************************************/
				
#define HTC_SIGNALS	\
				(HTC_START_SIG   | \
				 HTC_STOP_SIG    | \
				 HTC_COMMAND_SIG | \
				 HTC_READY_SIG   | \
				 HTC_HIFDSR_SIG  | \
				 HTC_FAIL_SIG    | \
				 HTC_TX_DATA_SIG | \
				 HTC_RX_DATA_SIG)

void *htc_task(void* data)
{
	rex_sigs_type signal;
	int more_signal = 0;
	int i;

	DPRINTF(DBG_HTC, (DBGFMT "Starting\n", DBGARG));

	/* Set the initial state */
	set_state(HTC_STATE_START);

	/* Initialize the command timer */
	A_INIT_TIMER(&s_command_timeout_timer, process_command_timeout, 0);

	/* Initialize the driver - HTC and HIF layers */
	qcom_driver_init();

	/* We are now ready */
	A_MUTEX_LOCK(&s_flags_lock);
	s_flags |= FLAG_TX_READY | FLAG_CMD_READY;
	A_MUTEX_UNLOCK(&s_flags_lock);

	for(;;)
	{
		/* See if we have any pending signals */
		signal = rex_get_sigs(htc_task_ptr);

		/* Mask of signals we are not interested in */
		signal &= HTC_SIGNALS;

		/* Add signals from last run */
		signal |= more_signal;

		/* Reset signals from last run */
		more_signal = 0;

		/* If we have nothing else todo, wait for signals */
		if(signal == 0)
		{
			signal = rex_wait(HTC_SIGNALS);
		}

		/*********************************************************************/
		/** Process the signal                                              **/
		/*********************************************************************/
		
		DPRINTF(DBG_HTC, (DBGFMT "Got signal 0x%08x\n", DBGARG, signal));

		/* Process each signal */
		for(i = 0; i < SIGNAL_INDEX_TABLE_SIZE; i++)
		{
			/* Seach the index table */
			if(s_signal_index_table[i].signal & signal)
			{
				/* Clear the signal before processing is done */
				rex_clr_sigs(htc_task_ptr, s_signal_index_table[i].signal);

				/* Does this state have a handler for this signal */
				if((*s_current_state->table)[i] != NULL)
				{
					int status;

					DPRINTF(DBG_HTC,
						(DBGFMT "Processing \"%s\".\n", DBGARG,
						s_signal_index_table[i].description));

					/* Call the state function for this signal */
					if((status = (*(*s_current_state->table)[i])()) < 0 )
					{
						DPRINTF(DBG_HTC,
							(DBGFMT "Failed processing signal \"%s\".\n",
							DBGARG, s_signal_index_table[i].description));
						htc_set_failure(FAIL_FATAL);
					}
					else
					{
						more_signal |= status;
					}
				}
				else
				{
					DPRINTF(DBG_HTC,
						(DBGFMT "Ignoring \"%s\".\n", DBGARG,
						s_signal_index_table[i].description));
				}
			}
		}
	}

	DPRINTF(DBG_HTC, (DBGFMT "Ending\n", DBGARG));
}
