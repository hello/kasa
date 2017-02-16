/*
 * Copyright (c) 2004-2005 Atheros Communications Inc.
 * All rights reserved.
 *
 * $ATH_LICENSE_HOSTSDK0_C$
 *
 */

/*****************************************************************************/
/**                                                                         **/
/** This file implements the WMI callbacks                                  **/
/**                                                                         **/
/*****************************************************************************/

#ifdef ATHR_EMULATION
/* Linux/OS include files */
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#endif

/* Atheros include files */
#include <athdefs.h>
#include <a_types.h>
#include <a_osapi.h>
#include <wmi.h>
#include <wlan_api.h>
#include <htc.h>
#include <htc_api.h>
#include <htc_packet.h>
#include <wmi_api.h>
#include <dbglog_api.h>
#include <ieee80211.h>

#include <common_drv.h>

/* Qualcomm include files */
#ifdef ATHR_EMULATION
#include "qcom_stubs.h"
#endif
#include "wlan_dot11_sys.h"
#include "wlan_adp_def.h"
#include "wlan_adp_oem.h"
#include "wlan_trp_oem.h"
#include "wlan_trp.h"
#include "wlan_util.h"

/* Atheros platform include files */
#include "athdrv_rexos.h"
#include "qcom_htc.h"
#include "a_drv_api.h"
#include "a_debug.h"

#ifdef CONFIG_HOST_TCMD_SUPPORT
#include "testcmd.h"
#endif

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN
#endif

#define MAX_ALLOWED_TXQ_DEPTH   (HTC_DATA_REQUEST_RING_BUFFER_SIZE - 2)

#undef BIG_ENDIAN

PREPACK struct iphdr
{
#ifdef LITTLE_ENDIAN
    unsigned int ihl:4;
    unsigned int version:4;
#endif
#ifdef BIG_ENDIAN
    unsigned int version:4;
    unsigned int ihl:4;
#endif
    A_UINT8 tos;
    A_UINT16 tot_len;
    A_UINT16 id;
    A_UINT16 frag_off;
    A_UINT8 ttl;
    A_UINT8 protocol;
    A_UINT16 check;
    A_UINT32 saddr;
    A_UINT32 daddr;
    /*The options start here. */
} POSTPACK;

/* Here is the WMM info */
static wlan_adp_wmm_info_type s_wmm_info;

static char s_hex_table[16] =
{
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'a', 'b', 'c', 'd', 'e', 'f'
};
static A_TIMER s_rssi_timer;
static int s_rssi_timeout = RSSI_QUERY_PERIOD;
static void  process_rssi_timeout(unsigned long);

void 
ar6000_channelList_rx(void *devt, A_INT8 numChan, A_UINT16 *chanList)
{
	AR_SOFTC_T *ar = (AR_SOFTC_T *) devt;

	DPRINTF(DBG_WMI, (DBGFMT "**WMI** Enter\n", DBGARG));

	A_MEMCPY(ar->arChannelList, chanList, numChan * sizeof (A_UINT16));
	ar->arNumChannels = numChan;
}

void ar6000_set_numdataendpts(void *devt, A_UINT32 num) 
{
	AR_SOFTC_T *ar = (AR_SOFTC_T *) devt;

	DPRINTF(DBG_WMI, (DBGFMT "**WMI** Enter %d\n", DBGARG, num));

	A_ASSERT(num <= (HTC_MAILBOX_NUM_MAX - 1));
}

A_STATUS
ar6000_control_tx(void *devt, void *osbuf, HTC_ENDPOINT_ID eid)
{
	AR_SOFTC_T *ar = (AR_SOFTC_T *) devt;
	struct ar_cookie *cookie = NULL;
    
	DPRINTF(DBG_WMI, (DBGFMT "**WMI** Enter - eid ID %d\n", DBGARG, eid));

    /* take lock to protect qcom_alloc_cookie() */
    A_MUTEX_LOCK(&ar->arLock);

    if ((ar->arWMIControlEpFull) && (eid == ar->arControlEp)) {
        /* control endpoint is full, don't allocate resources, we
         * are just going to drop this packet */
        DPRINTF(DBG_WMI, (DBGFMT " WMI Control EP full, dropping packet :"
                " 0x%X, len:%d \n", DBGARG, (A_UINT32)osbuf, A_NETBUF_LEN(osbuf)));
    } else {
        cookie = qcom_alloc_cookie(ar);
    }

    if (cookie != NULL) {
	   ar->arTxPending[eid]++;

    	if (eid != ar->arControlEp) {
    		ar->arTotalTxDataPending++;
    	}
    }

    A_MUTEX_UNLOCK(&ar->arLock);
    
    if (cookie == NULL) {
        DPRINTF(DBG_WMI, (DBGFMT " Dropping control packet :"
                " 0x%X, len:%d \n", DBGARG, (A_UINT32)osbuf, A_NETBUF_LEN(osbuf)));
        A_NETBUF_FREE(osbuf);
        return A_NO_MEMORY;
    }

    cookie->arc_bp[0] = (A_UINT32)osbuf;
    cookie->arc_bp[1] = 0;
    
    DPRINTF(DBG_WMI, (DBGFMT "CNTRL Tx cookie %p, %p.\n", DBGARG, cookie, osbuf));

    SET_HTC_PACKET_INFO_TX(&cookie->HtcPkt,
                           cookie,
                           A_NETBUF_DATA(osbuf),
                           A_NETBUF_LEN(osbuf),
                           eid,
                           AR6K_CONTROL_PKT_TAG);
    
    /* this interface is asynchronous, if there is an error, cleanup will 
       happen in the TX completion callback */
    HTCSendPkt(ar->arHtcTarget, &cookie->HtcPkt);

    return A_OK;
}

void ar6000_indicate_tx_activity(void *devt, A_UINT8 TrafficClass, A_BOOL Active)
{
    AR_SOFTC_T  *ar = (AR_SOFTC_T *)devt;
    HTC_ENDPOINT_ID eid ;
    int i;
            
    DPRINTF(DBG_WMI, (DBGFMT "**WMI** Enter - Traffic Class %d\n", DBGARG, TrafficClass));

    A_MUTEX_LOCK(&ar->arLock);
    
    /* we always assume that BE stream exists */
    if (TrafficClass != WMM_AC_BE)
    {
        if (Active)
            ar->arNumDataEndPts++;
        else
            ar->arNumDataEndPts--;
    }

    A_ASSERT(ar->arNumDataEndPts <= WMM_NUM_AC);
    
    if (ar->arWmiEnabled) {
        eid = arAc2EndpointID(ar, TrafficClass);
        
        ar->arAcStreamActive[TrafficClass] = Active;
        
        if (Active) {
            /* when a stream goes active, keep track of the active stream with the highest priority */
        
            if (ar->arAcStreamPriMap[TrafficClass] > ar->arHiAcStreamActivePri) {
                    /* set the new highest active priority */
                ar->arHiAcStreamActivePri = ar->arAcStreamPriMap[TrafficClass];   
            }
        
        } else {
            /* when a stream goes inactive, we may have to search for the next active stream
             * that is the highest priority */
            
            if (ar->arHiAcStreamActivePri == ar->arAcStreamPriMap[TrafficClass]) {
                
                /* the highest priority stream just went inactive */
                
                    /* reset and search for the "next" highest "active" priority stream */
                ar->arHiAcStreamActivePri = 0;
                for (i = 0; i < WMM_NUM_AC; i++) {
                    if (ar->arAcStreamActive[i]) {
                        if (ar->arAcStreamPriMap[i] > ar->arHiAcStreamActivePri) {
                            /* set the new highest active priority */
                            ar->arHiAcStreamActivePri = ar->arAcStreamPriMap[i];   
                        }    
                    }  
                }
            }
        }
    }
    else {
        /* for mbox ping testing, the traffic class is mapped directly as a 
         * stream ID*/
        eid = (HTC_ENDPOINT_ID)TrafficClass;
    }

    A_MUTEX_UNLOCK(&ar->arLock);
    
    /* notify HTC, this may cause credit distribution changes */
    HTCIndicateActivityChange(ar->arHtcTarget, 
                              eid,
                              Active);
}

void
ar6000_targetStats_event(void *devt, A_UINT8 *ptr, A_UINT32 len)
{
	int why;
	wlan_dot11_opr_param_value_type value;
    AR_SOFTC_T  *ar = (AR_SOFTC_T *)devt;
	WMI_TARGET_STATS *pTargetStats = (WMI_TARGET_STATS *)ptr;
	
	DPRINTF(DBG_WMI, (DBGFMT "**WMI** Enter\n", DBGARG));

	if(ar->arNetworkType == AP_NETWORK) {
		return;
	}
	
	/* Always update the RSSI when we get statistics */
	htc_set_rssi(pTargetStats->cservStats.cs_aveBeacon_snr);

	if(htc_get_status() & STATS_IN_PROGRESS)
	{
		why = (int) htc_get_status_data(STATS_SLOT);

		DPRINTF(DBG_WMI, (DBGFMT "Active stats - deliver results %d.\n", DBGARG, why));

		switch(why)
		{
			case 1:
				DPRINTF(DBG_WMI, (DBGFMT "WLAN_RSSI %u\n", DBGARG,
					pTargetStats->cservStats.cs_aveBeacon_snr));
				value.u32_value = pTargetStats->cservStats.cs_aveBeacon_snr;
				wlan_adp_oem_opr_param_get_rsp(WLAN_RSSI, value);
				break;
			case 2:
				DPRINTF(DBG_WMI, (DBGFMT "WLAN_TX_FRAME_CNT %u\n", DBGARG,
					pTargetStats->txrxStats.tx_stats.tx_packets));
				value.u32_value = pTargetStats->txrxStats.tx_stats.tx_packets;
				wlan_adp_oem_opr_param_get_rsp(WLAN_TX_FRAME_CNT, value);
				break;
			case 3:
				DPRINTF(DBG_WMI, (DBGFMT "WLAN_RX_FRAME_CNT %u\n", DBGARG,
					pTargetStats->txrxStats.rx_stats.rx_packets));
				value.u32_value = pTargetStats->txrxStats.rx_stats.rx_packets;
				wlan_adp_oem_opr_param_get_rsp(WLAN_RX_FRAME_CNT, value);
				break;
			case 4:
				DPRINTF(DBG_WMI, (DBGFMT "WLAN_TOTAL_RX_BYTE %u\n", DBGARG,
					pTargetStats->txrxStats.rx_stats.rx_bytes));
				value.u32_value = pTargetStats->txrxStats.rx_stats.rx_bytes;
				wlan_adp_oem_opr_param_get_rsp(WLAN_TOTAL_RX_BYTE, value);
				break;
			default:
				DPRINTF(DBG_WMI, (DBGFMT "Invalid response indicator %d.\n", DBGARG, why));
				break;
		}

		/* Clear the data */
		htc_set_status_data(STATS_SLOT, (void *) 0);

		/* Clear the status */
		htc_clear_status(STATS_IN_PROGRESS);

		/* Command is complete */
		htc_command_complete();
	}
}

static int s_headers_done = 0;

static void dump_me_bss(void *arg, bss_t *ni)
{
	char buff[64];
	int *ip = (int *) arg;

	if(s_headers_done == 0)
	{
		DPRINTF(DBG_WMI, (DBGFMT "#  : RSSI BSS\n", DBGARG));
		s_headers_done++;
	}

	A_MEMCPY(buff, &ni->ni_cie.ie_ssid[2], ni->ni_cie.ie_ssid[1]);
	buff[ni->ni_cie.ie_ssid[1]] = '\0';

	DPRINTF(DBG_WMI, (DBGFMT "%02d : %02x   %02x:%02x:%02x:%02x:%02x:%02x %d \"%s\" 0x%04x %d:%d\n", DBGARG,
		*ip, (unsigned char) ni->ni_rssi,
		ni->ni_macaddr[0], ni->ni_macaddr[1],
		ni->ni_macaddr[2], ni->ni_macaddr[3],
		ni->ni_macaddr[4], ni->ni_macaddr[5],
		ni->ni_cie.ie_ssid[1], buff,
		ni->ni_cie.ie_capInfo,
		ni->ni_cie.ie_chan, wlan_freq2ieee(ni->ni_cie.ie_chan)));

	(*ip)++;

	return;
}

static A_UINT8 generate_scan_wpa_rsn_response(A_UINT8 *ie, 
                                              wlan_dot11_bss_description_type* rsp_bss)
{
    int j, k, l, max;
    
    /* ie 48 => RSN ie 221 = WPA */
    DPRINTF(DBG_WMI, (DBGFMT "Found \"%s\" %d:%d\n", DBGARG,
            ie[0] == IEEE80211_ELEMID_RSN ? "RSN" : "WPA", ie[0], ie[1]));

    /* Copy the whole thing */
    max = rsp_bss->wpa_ie.ie_len = min(ie[1], WPA_IE_MAX_LEN) + 2;
    A_MEMCPY(rsp_bss->raw_ie_data, &ie[0], rsp_bss->wpa_ie.ie_len);

#ifdef MATS
    for(j = 0; j < rsp_bss->wpa_ie.ie_len; j++)
    {
        DPRINTF(DBG_WMI, (DBGFMT "Data[%d] = %d 0x%02x\n", DBGARG, j,
                ie[j], ie[j]));
    }
#endif

    /* Let's see what we have - RSN or WPA */
    switch(ie[0])
    {
        case IEEE80211_ELEMID_RSN:
            j = 2;	/* Start with version */
            break;
        case IEEE80211_ELEMID_VENDOR:
            j = 6;	/* Start with version */
            break;
        default:
            j = 0;	/* This might be reasonable to do ... or ?? */
            break;
    }

    rsp_bss->wpa_ie.version = A_LE2CPU16(*((A_UINT16 *) &ie[j]));
    j += 5; /* Move to group chiper suite _type_ */
    if(j >= max) goto format_error;	/* Make sure we don't over index */
    rsp_bss->wpa_ie.grp_cipher_suite = ie[j];
    j += 1; /* Move to pairwise chiper count */
    if(j >= max) goto format_error;	/* Make sure we don't over index */
    l = rsp_bss->wpa_ie.pairwise_cipher_count =
        A_LE2CPU16(*((A_UINT16 *) &ie[j]));
    j += 2; /* Move to first pairwise chipher */
    if(j >= max) goto format_error;	/* Make sure we don't over index */
    for(k = 0; k < l; k++)
    {
        /* Can we hold this one */
        if(k < WPA_MAX_PAIRWISE_CIPHER_SUITE)
        {
            /* There is enough room for one more */
            rsp_bss->wpa_ie.pairwise_cipher_suites[k].oui[0] = ie[j + 0];
            rsp_bss->wpa_ie.pairwise_cipher_suites[k].oui[1] = ie[j + 1];
            rsp_bss->wpa_ie.pairwise_cipher_suites[k].oui[2] = ie[j + 2];
            rsp_bss->wpa_ie.pairwise_cipher_suites[k].oui[3] = ie[j + 3];
        }

        j += 4;	/* Move to next pairwise chipher */
        if(j >= max) goto format_error;	/* Make sure we don't over index */
    }
    l = rsp_bss->wpa_ie.akm_count = A_LE2CPU16(*((A_UINT16 *) &ie[j]));
    j += 2; /* Move to first AKM suite */
    for(k = 0; k < l; k++)
    {
        if(j >= max) goto format_error;	/* Make sure we don't over index */

        /* Can we hold this one */
        if(k < WPA_MAX_AKM_SUITE)
        {
            /* There is enough room for one more */
            rsp_bss->wpa_ie.akm_suites[k].oui[0] = ie[j + 0];
            rsp_bss->wpa_ie.akm_suites[k].oui[1] = ie[j + 1];
            rsp_bss->wpa_ie.akm_suites[k].oui[2] = ie[j + 2];
            rsp_bss->wpa_ie.akm_suites[k].oui[3] = ie[j + 3];
        }

        j += 4;	/* Move to next AKM suite */
    }

    /* Capabilities field is optional */
    if(j >= max)
    {
        rsp_bss->wpa_ie.rsn_cap = 0;
    }
    else
    {
        rsp_bss->wpa_ie.rsn_cap = A_LE2CPU16(*((A_UINT16 *) &ie[j]));
    }

    return TRUE;
    
format_error:

	DPRINTF(DBG_WMI, (DBGFMT "Format error at index %d. SKIPPING.\n", DBGARG, j));
	return FALSE;
}

static void generate_scan_response(void *arg, bss_t *ni)
{
	wlan_adp_scan_rsp_info_type *rsp = (wlan_adp_scan_rsp_info_type *) arg;
	int i, j, k;
	char buff[64];
	static wlan_adp_scan_req_info_type *filter;

	i = rsp->num_bss;

	if(i >= WLAN_SYS_MAX_SCAN_BSS)
	{
		/* We can not report more than this so just truncate the list */
		DPRINTF(DBG_WMI, (DBGFMT "To many BSSs (%d). SKIPPING.\n", DBGARG, i));
		return;
	}

	A_MEMCPY(buff, &ni->ni_cie.ie_ssid[2], ni->ni_cie.ie_ssid[1]);
	buff[ni->ni_cie.ie_ssid[1]] = '\0';

	DPRINTF(DBG_WMI, (DBGFMT "%02d - %02x:%02x:%02x:%02x:%02x:%02x %d:%d \"%s\"\n", DBGARG,
		i,
		ni->ni_macaddr[0], ni->ni_macaddr[1],
		ni->ni_macaddr[2], ni->ni_macaddr[3],
		ni->ni_macaddr[4], ni->ni_macaddr[5],
		ni->ni_cie.ie_chan, wlan_freq2ieee(ni->ni_cie.ie_chan),
		buff));

	/*****************************************************************/
	/* Filter results                                                */
	/*****************************************************************/

	if((filter = htc_get_scan_filter()) != NULL)
	{
		/* Keep track of the filter number for debugging purposes */ 
		j = 1;

		if(filter->ssid.len != 0)
		{
			if(A_MEMCMP(filter->ssid.ssid, &ni->ni_cie.ie_ssid[2],
				filter->ssid.len) != 0)
			{
				goto frame_filtered;
			}
		}

		/* Keep track of the filter number for debugging purposes */ 
		j = 2;

		if(filter->bssid != 0 && filter->bssid != 0x0000FFFFFFFFFFFFLL)
		{
			A_UINT8 ni_macaddr[6];

			wlan_util_uint64_to_macaddress(ni_macaddr, filter->bssid);

			if(A_MEMCMP(ni_macaddr, ni->ni_macaddr,
				sizeof(ni->ni_macaddr)) != 0)
			{
				goto frame_filtered;
			}
		}

		if(filter->bss_type != SYS_WLAN_BSS_TYPE_NONE &&
			filter->bss_type != SYS_WLAN_BSS_TYPE_ANY)
		{
			/* Keep track of the filter number for debugging purposes */ 
			j = 3;

			DPRINTF(DBG_WMI, (DBGFMT "Specific BSS 0x%04x\n", DBGARG,
				ni->ni_cie.ie_capInfo));

			if(filter->bss_type == SYS_WLAN_BSS_TYPE_INFRA &&
				((ni->ni_cie.ie_capInfo & 0x0001) == 0))
			{
				DPRINTF(DBG_WMI, (DBGFMT "No INFRA\n", DBGARG));
				goto frame_filtered;
			}

			/* Keep track of the filter number for debugging purposes */ 
			j = 4;

			if(filter->bss_type == SYS_WLAN_BSS_TYPE_ADHOC &&
				((ni->ni_cie.ie_capInfo & 0x0001) != 0))
			{
				DPRINTF(DBG_WMI, (DBGFMT "No ADHOC\n", DBGARG));
				goto frame_filtered;
			}
		}

		/* Keep track of the filter number for debugging purposes */ 
		j = 5;

		if(filter->channel_list.nb_channels > 0)
		{
			for(k = 0; k < filter->channel_list.nb_channels; k++)
			{
				if(wlan_freq2ieee(ni->ni_cie.ie_chan) ==
					filter->channel_list.channel_list[k])
				{
					break;
				}
			}

			/* Check if we found it */
			if(k == filter->channel_list.nb_channels)
			{
				/* No we didn't */
				goto frame_filtered;
			}
		}
	}

	/*****************************************************************/
	/* Copy data                                                     */
	/*****************************************************************/

	A_MEMCPY(rsp->bss_descs[i].ssid.ssid, &ni->ni_cie.ie_ssid[2], ni->ni_cie.ie_ssid[1]);
	rsp->bss_descs[i].ssid.len = ni->ni_cie.ie_ssid[1];
	rsp->bss_descs[i].bssid = wlan_util_macaddress_to_uint64(ni->ni_macaddr);

	/* Bits are mutually exclusive */
	if(ni->ni_cie.ie_capInfo & 0x0001)
	{
		rsp->bss_descs[i].bsstype = SYS_WLAN_BSS_TYPE_INFRA;
	}
	else
	{
		rsp->bss_descs[i].bsstype = SYS_WLAN_BSS_TYPE_ADHOC;
	}

	rsp->bss_descs[i].channel_id = wlan_freq2ieee(ni->ni_cie.ie_chan);

	A_ASSERT(sizeof(rsp->bss_descs[i].capability) ==
		sizeof(ni->ni_cie.ie_capInfo));
	A_MEMCPY(&rsp->bss_descs[i].capability,
		&ni->ni_cie.ie_capInfo, sizeof(ni->ni_cie.ie_capInfo));

	if(ni->ni_cie.ie_rates != NULL)
	{
		rsp->bss_descs[i].supported_rates.nb_rates = ni->ni_cie.ie_rates[1];
		A_MEMCPY(rsp->bss_descs[i].supported_rates.rates,
			&ni->ni_cie.ie_rates[2], ni->ni_cie.ie_rates[1]);
	}

	rsp->RSSI[i] = ni->ni_rssi;

	/* Fill in the WMM stuff */
	if(ni->ni_cie.ie_wmm == NULL)
	{
		rsp->bss_descs[i].wmm_info.wmm_element_type =
			WLAN_ADP_WMM_ELEMENT_TYPE_NONE;
	}
	else
	{
		/* Check OUI type and version */
		if(ni->ni_cie.ie_wmm[5] != 2 && ni->ni_cie.ie_wmm[7] != 1)
		{
			DPRINTF(DBG_WMI,
				(DBGFMT "WMM wrong OUI %u and/or version %u\n", DBGARG,
				ni->ni_cie.ie_wmm[5], ni->ni_cie.ie_wmm[7]));

			rsp->bss_descs[i].wmm_info.wmm_element_type =
				WLAN_ADP_WMM_ELEMENT_TYPE_NONE;
		}
		else
		{
			/* Check sub type */
			if(ni->ni_cie.ie_wmm[6] == 0)
			{
				/* This is WMM Information Element Field Values */
				DPRINTF(DBG_WMI, (DBGFMT "WMM IE\n", DBGARG));

				/* The QoS info field is in byte 9 */
				rsp->bss_descs[i].wmm_info.wmm_element_type =
					WLAN_ADP_WMM_ELEMENT_TYPE_IE;
				rsp->bss_descs[i].wmm_info.element.wmm_ie.qos_info.wmm_ps =
					ni->ni_cie.ie_wmm[8] & 0x80 ? TRUE : FALSE;
				rsp->bss_descs[i].wmm_info.element.wmm_ie.qos_info.param_set_cnt =
					ni->ni_cie.ie_wmm[8] & 0x0f;
			}
			else if(ni->ni_cie.ie_wmm[6] == 1)
			{
				/* This is WMM Parameter Element Field Values */
				DPRINTF(DBG_WMI, (DBGFMT "WMM PE\n", DBGARG));

				/* The QoS info field is in byte 9 */
				rsp->bss_descs[i].wmm_info.wmm_element_type =
					WLAN_ADP_WMM_ELEMENT_TYPE_PE;
				rsp->bss_descs[i].wmm_info.element.wmm_pe.qos_info.wmm_ps =
					ni->ni_cie.ie_wmm[8] & 0x80 ? TRUE : FALSE;
				rsp->bss_descs[i].wmm_info.element.wmm_pe.qos_info.param_set_cnt =
					ni->ni_cie.ie_wmm[8] & 0x0f;

				rsp->bss_descs[i].wmm_info.element.wmm_pe.ac_params[WLAN_ADP_QOS_EDCA_AC_BE].aci =
					(ni->ni_cie.ie_wmm[10] >> 5) & 0x03;
				rsp->bss_descs[i].wmm_info.element.wmm_pe.ac_params[WLAN_ADP_QOS_EDCA_AC_BE].acm =
					ni->ni_cie.ie_wmm[10] & 0x10 ? TRUE : FALSE;
				rsp->bss_descs[i].wmm_info.element.wmm_pe.ac_params[WLAN_ADP_QOS_EDCA_AC_BK].aci =
					(ni->ni_cie.ie_wmm[14] >> 5) & 0x03;
				rsp->bss_descs[i].wmm_info.element.wmm_pe.ac_params[WLAN_ADP_QOS_EDCA_AC_BK].acm =
					ni->ni_cie.ie_wmm[14] & 0x10 ? TRUE : FALSE;
				rsp->bss_descs[i].wmm_info.element.wmm_pe.ac_params[WLAN_ADP_QOS_EDCA_AC_VI].aci =
					(ni->ni_cie.ie_wmm[18] >> 5) & 0x03;
				rsp->bss_descs[i].wmm_info.element.wmm_pe.ac_params[WLAN_ADP_QOS_EDCA_AC_VI].acm =
					ni->ni_cie.ie_wmm[18] & 0x10 ? TRUE : FALSE;
				rsp->bss_descs[i].wmm_info.element.wmm_pe.ac_params[WLAN_ADP_QOS_EDCA_AC_VO].aci =
					(ni->ni_cie.ie_wmm[22] >> 5) & 0x03;
				rsp->bss_descs[i].wmm_info.element.wmm_pe.ac_params[WLAN_ADP_QOS_EDCA_AC_VO].acm =
					ni->ni_cie.ie_wmm[22] & 0x10 ? TRUE : FALSE;
			}
			else
			{
				DPRINTF(DBG_WMI,
					(DBGFMT "WMM invalid subtype %u\n", DBGARG,
					ni->ni_cie.ie_wmm[6]));

				rsp->bss_descs[i].wmm_info.wmm_element_type =
					WLAN_ADP_WMM_ELEMENT_TYPE_NONE;
			}
		}
	}

    if((ni->ni_cie.ie_wpa == NULL) && (ni->ni_cie.ie_rsn == NULL))
    {
        /* No information available */
		rsp->bss_descs[i].wpa_ie.ie_len = 0;
	}
	else
	{
        if(ni->ni_cie.ie_wpa != NULL)
        {
            if (FALSE == generate_scan_wpa_rsn_response(ni->ni_cie.ie_wpa, 
                            &(rsp->bss_descs[i])))
                return;
        }
        
        if (ni->ni_cie.ie_rsn != NULL)
        {
            if (ni->ni_cie.ie_wpa != NULL)
            {
                A_MEMCPY(&(rsp->bss_descs[i+1]), &(rsp->bss_descs[i]),
                         sizeof(wlan_dot11_bss_description_type));
                i++;
                rsp->num_bss++;
            }
            
            if (FALSE == generate_scan_wpa_rsn_response(ni->ni_cie.ie_rsn,
                            &(rsp->bss_descs[i])))
                return;
        }
    }
    
	/* We have one more to report */
	rsp->num_bss++;

	return;

frame_filtered:

	DPRINTF(DBG_WMI, (DBGFMT "Frame filtered by %d. SKIPPING.\n", DBGARG, j));

	return;
}

static void
process_rssi_timeout(unsigned long devt)
{
   AR_SOFTC_T *ar = (AR_SOFTC_T *)devt;
   DPRINTF(DBG_WMI,(DBGFMT "Enter - rssi timeout\n",DBGARG));

   if(wmi_get_stats_cmd(ar->arWmi) != A_OK)
    {
        DPRINTF(DBG_WMI,
                (DBGFMT "wmi_get_stats_cmd() failed.\n", DBGARG));
    }
   /* Re-initialize the timeout, since periodic timer is not supported */
   A_TIMEOUT_MS(&s_rssi_timer, s_rssi_timeout, 0);

}

void
ar6000_scanComplete_event(void *devt, A_STATUS status)
{
	AR_SOFTC_T *ar = (AR_SOFTC_T *) devt;
	int i = 0;

	DPRINTF(DBG_WMI, (DBGFMT "**WMI** Enter\n", DBGARG));

	if(htc_get_status() & SCAN_IN_PROGRESS)
	{
		static wlan_adp_scan_rsp_info_type scan_rsp_info;

		DPRINTF(DBG_WMI, (DBGFMT "Active scan - deliver results.\n", DBGARG));

		s_headers_done = 0;
		wmi_iterate_nodes(ar->arWmi, dump_me_bss, &i);
		DPRINTF(DBG_WMI, (DBGFMT "Generate response.\n", DBGARG));

		/* Start populating the structure */
		scan_rsp_info.num_bss = 0;
		scan_rsp_info.result_code = WLAN_SYS_RES_SUCCESS;
		wmi_iterate_nodes(ar->arWmi, generate_scan_response, &scan_rsp_info);

		/* Reset the filter */
		if(wmi_bssfilter_cmd(ar->arWmi, NONE_BSS_FILTER, 0) != A_OK)
		{
			DPRINTF(DBG_WMI, (DBGFMT "wmi_bssfilter_cmd() failed.\n", DBGARG));
		}

		/* Get the result back to the WLAN Adapter layer */
		wlan_adp_oem_scan_rsp(&scan_rsp_info);

		/* Clear the status */
		htc_clear_status(SCAN_IN_PROGRESS);

		/* Command is complete */
		htc_command_complete();
	}
}

static void qcom_install_static_wep_keys(AR_SOFTC_T *ar)
{
	A_UINT8 index;
	A_UINT8 keyUsage;

	for (index = WMI_MIN_KEY_INDEX; index <= WMI_MAX_KEY_INDEX; index++)
	{
		if (ar->arWepKeyList[index].arKeyLen)
		{
			keyUsage = GROUP_USAGE;
			if (index == ar->arDefTxKeyIndex)
			{
				keyUsage |= TX_USAGE;
			}

			wmi_addKey_cmd(ar->arWmi, index, WEP_CRYPT, keyUsage,
				ar->arWepKeyList[index].arKeyLen, NULL,
				ar->arWepKeyList[index].arKey, KEY_OP_INIT_VAL, NULL, NO_SYNC_WMIFLAG);
		}
	}
}

void
ar6000_connect_event(void *devt, A_UINT16 channel, A_UINT8 *bssid,
                     A_UINT16 listenInterval, A_UINT16 beaconInterval,
                     NETWORK_TYPE networkType, A_UINT8 beaconIeLen, 
                     A_UINT8 assocReqLen, A_UINT8 assocRespLen, 
                     A_UINT8 *assocInfo)
{
	AR_SOFTC_T *ar = (AR_SOFTC_T *) devt;
	int i, beacon_ie_pos, assoc_resp_ie_pos, assoc_req_ie_pos;
	static const char tag1[] = "ASSOCINFO(ReqIEs=";
	static const char tag2[] = "ASSOCRESPIE=";
	static const char *beaconIetag = "BEACONIE=";
	char buf[WMI_CONTROL_MSG_MAX_LEN * 2 + sizeof(tag1)];
	char *pos;
	unsigned char *rsp_ie_data;
	int rsp_ie_len = 0;

	DPRINTF(DBG_WMI, (DBGFMT "**WMI** Enter\n", DBGARG));

	DPRINTF(DBG_WMI, (DBGFMT "BSS %02x:%02x:%02x:%02x:%02x:%02x channel %u\n", DBGARG,
		bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5],
		wlan_freq2ieee(channel)));

    if (networkType & ADHOC_NETWORK) {
        if (networkType & ADHOC_CREATOR) {
            DPRINTF(DBG_WMI, (DBGFMT "Network: Adhoc (Creator)\n", DBGARG));
        } else {
            DPRINTF(DBG_WMI, (DBGFMT "Network: Adhoc (Joiner)\n",DBGARG));
        }
    } else {
        DPRINTF(DBG_WMI, (DBGFMT "Network: Infrastructure\n",DBGARG));
    }

	/* Indicate that we are connected */
	A_MEMCPY(ar->arBssid, bssid, sizeof(ar->arBssid));
	ar->arBssChannel = channel;

	DPRINTF(DBG_WMI, (DBGFMT "listenInterval %d beaconInterval %d beaconIeLen %d assocReqLen %d assocRespLen %d\n", DBGARG,
		listenInterval, beaconInterval, beaconIeLen, assocReqLen, assocRespLen));

    if (beaconIeLen && (sizeof(buf) > (9+1 + beaconIeLen * 2))) {
        DPRINTF(DBG_WMI, (DBGFMT "\nBeaconIEs= ", DBGARG));
        beacon_ie_pos = 0;
        A_MEMZERO(buf, sizeof(buf));
        A_SPRINTF(buf, "%s", beaconIetag);
        pos = buf + 9;
        
        for (i = beacon_ie_pos; i < beacon_ie_pos + beaconIeLen; i++) {
            DPRINTF(DBG_WMI, (DBGFMT "%2.2x ", DBGARG, assocInfo[i]));
            A_SPRINTF(pos, "%2.2x", assocInfo[i]);
            pos += 2;
        }
        DPRINTF(DBG_WMI, (DBGFMT "\n", DBGARG));
    }
    
	if (assocRespLen && (sizeof(buf) > (12+1 + (assocRespLen * 2))))
	{
		assoc_resp_ie_pos = beaconIeLen + assocReqLen +
			sizeof(A_UINT16)  +  /* capinfo*/
			sizeof(A_UINT16)  +  /* status Code */
			sizeof(A_UINT16)  ;  /* associd */

		A_MEMZERO(buf, sizeof(buf));

		A_MEMCPY(buf, tag2, 12); 
		pos = buf + 12;

		/* Save the start of the responce ie(s) */
		rsp_ie_data = &assocInfo[assoc_resp_ie_pos];
		rsp_ie_len = assocRespLen;
		
		/*
		* The complete Association Response Frame is delivered to the  
		* host, so skip over to the IEs
		*/
		for (i = assoc_resp_ie_pos; i < (assoc_resp_ie_pos + assocRespLen); i++)
		{
			*pos++ = s_hex_table[(assocInfo[i] >> 4) & 0xf];
			*pos++ = s_hex_table[assocInfo[i] & 0xf];
		}

		*pos = '\0';

		DPRINTF(DBG_WMI, (DBGFMT "To host \"%s\".\n", DBGARG, buf));
	}

	if (assocReqLen && (sizeof(buf) > (17+1 + (assocReqLen * 2))))
	{
		/*
		* Assoc request includes capability and listen interval. Skip these.
		*/
		assoc_req_ie_pos = beaconIeLen + 
		    sizeof(A_UINT16)  +  /* capinfo*/
			sizeof(A_UINT16);    /* listen interval */

		A_MEMZERO(buf, sizeof(buf));

		A_MEMCPY(buf, tag1, 17); 
		pos = buf + 17;

		for (i = assoc_req_ie_pos; i < assoc_req_ie_pos + assocReqLen; i++)
		{
			*pos++ = s_hex_table[(assocInfo[i] >> 4) & 0xf];
			*pos++ = s_hex_table[assocInfo[i] & 0xf];
		}

		*pos = '\0';

		DPRINTF(DBG_WMI, (DBGFMT "To host \"%s\".\n", DBGARG, buf));
	}

    /* flush data queues */
    qcom_tx_data_cleanup(ar);
    
	if ((OPEN_AUTH == ar->arDot11AuthMode) &&
		(NONE_AUTH == ar->arAuthMode)      &&
		(WEP_CRYPT == ar->arPairwiseCrypto))
	{
		if (!ar->arConnected) {
			qcom_install_static_wep_keys(ar);
		}
	}
	
	ar->arConnected  = TRUE;
	ar->arConnectPending = FALSE;
	A_INIT_TIMER(&s_rssi_timer, &process_rssi_timeout, (unsigned long)&devt);
    A_TIMEOUT_MS(&s_rssi_timer, s_rssi_timeout, 0);

    if ((ar->arNetworkType == ADHOC_NETWORK) && ar->arIbssPsEnable) {
        A_MEMZERO(ar->arNodeMap, sizeof(ar->arNodeMap));
        ar->arNodeNum = 0;
        ar->arNexEpId = ENDPOINT_2;
    }

	/* Check if we need to turn on the transmit flow */
	(void) check_flow_on((void *) ar);

	if(htc_get_status() & CONNECT_IN_PROGRESS)
	{
		int why;

		why = (int) htc_get_status_data(CONNECT_SLOT);

		DPRINTF(DBG_WMI,
			(DBGFMT "Active connect - deliver results %d.\n", DBGARG, why));

		/* Parse the IEs */
		for(i = 0; i < rsp_ie_len;)
		{
			unsigned char ie;
			unsigned char len;

			ie = rsp_ie_data[0];
			len = rsp_ie_data[1];

			DPRINTF(DBG_WMI,
				(DBGFMT "i %d ie %d len %d\n", DBGARG, i, ie, len));

			/* Clear the WMM information structure */
			A_MEMZERO(&s_wmm_info, sizeof(wlan_adp_wmm_info_type));

			/* Look for the "Vendor" EI to get WMM PE */
			if((ie == IEEE80211_ELEMID_VENDOR) &&
				(rsp_ie_data[2] == 0x00 && rsp_ie_data[3] == 0x50 &&
				rsp_ie_data[4] == 0xf2 && rsp_ie_data[5] == 0x02 &&
				rsp_ie_data[6] == 0x01 && rsp_ie_data[7] == 0x01))
			{
				DPRINTF(DBG_WMI,
					(DBGFMT "Found WMM PE\n", DBGARG));
				
				s_wmm_info.wmm_element_type = WLAN_ADP_WMM_ELEMENT_TYPE_PE;
				s_wmm_info.element.wmm_pe.qos_info.wmm_ps =
					rsp_ie_data[8] & 0x80 ? TRUE : FALSE;
				s_wmm_info.element.wmm_pe.qos_info.param_set_cnt =
					rsp_ie_data[8] & 0x0f;

				s_wmm_info.element.wmm_pe.ac_params[WLAN_ADP_QOS_EDCA_AC_BE].aci =
					(rsp_ie_data[10] >> 5) & 0x03;
				s_wmm_info.element.wmm_pe.ac_params[WLAN_ADP_QOS_EDCA_AC_BE].acm =
					rsp_ie_data[10] & 0x10 ? TRUE : FALSE;
				s_wmm_info.element.wmm_pe.ac_params[WLAN_ADP_QOS_EDCA_AC_BK].aci =
					(rsp_ie_data[14] >> 5) & 0x03;
				s_wmm_info.element.wmm_pe.ac_params[WLAN_ADP_QOS_EDCA_AC_BK].acm =
					rsp_ie_data[14] & 0x10 ? TRUE : FALSE;
				s_wmm_info.element.wmm_pe.ac_params[WLAN_ADP_QOS_EDCA_AC_VI].aci =
					(rsp_ie_data[18] >> 5) & 0x03;
				s_wmm_info.element.wmm_pe.ac_params[WLAN_ADP_QOS_EDCA_AC_VI].acm =
					rsp_ie_data[18] & 0x10 ? TRUE : FALSE;
				s_wmm_info.element.wmm_pe.ac_params[WLAN_ADP_QOS_EDCA_AC_VO].aci =
					(rsp_ie_data[22] >> 5) & 0x03;
				s_wmm_info.element.wmm_pe.ac_params[WLAN_ADP_QOS_EDCA_AC_VO].acm =
					rsp_ie_data[22] & 0x10 ? TRUE : FALSE;
			}
			else
			{
				/* We don't want anything but the PE */
				s_wmm_info.wmm_element_type = WLAN_ADP_WMM_ELEMENT_TYPE_NONE;
			}

			/* Tell the HTC task about our WMM info */
			htc_set_wmm_info(&s_wmm_info);

			i += 2 + len;
			rsp_ie_data += 2 + len;
		}

		/* Ask for stats so we can get the RSSI updated */
		if(wmi_get_stats_cmd(ar->arWmi) != A_OK)
		{
			DPRINTF(DBG_WMI,
				(DBGFMT "wmi_get_stats_cmd() failed.\n", DBGARG));
		}

		/* Deliver the right response */
		switch(why)
		{
			case 1:
				/* Get the result back to the WLAN Adapter layer */
				wlan_adp_oem_auth_rsp(WLAN_SYS_RES_SUCCESS);
				break;
			case 2:
				{
					wlan_adp_reassoc_rsp_info_type info;

					info.result_code = WLAN_SYS_RES_SUCCESS;

					/* Copy the WMM info back in the response */
					A_MEMCPY(&info.wmm_info, &s_wmm_info,
						sizeof(wlan_adp_wmm_info_type));

					/* Get the result back to the WLAN Adapter layer */
					wlan_adp_oem_reassoc_rsp(&info);
				}
				break;
			default:
				DPRINTF(DBG_WMI, (DBGFMT "Invalid response indicator %d.\n", DBGARG, why));
				break;
		}

		/* Clear the data */
		htc_set_status_data(CONNECT_SLOT, (void *) 0);

		/* Clear the status */
		htc_clear_status(CONNECT_IN_PROGRESS);

		/* Command is complete */
		htc_command_complete();
	}
}

void
ar6000_regDomain_event(void *devt, A_UINT32 regCode)
{
	AR_SOFTC_T *ar = (AR_SOFTC_T *) devt;

	DPRINTF(DBG_WMI, (DBGFMT "**WMI** Enter - regCode 0x%08x\n", DBGARG, regCode));

	ar->arRegCode = regCode;
}

void 
ar6000_neighborReport_event(void *devt, int numAps, WMI_NEIGHBOR_INFO *info)
{
	static const char tag[] = "PRE-AUTH";
	char buf[128];
	int i, j;
	char *pos;

	DPRINTF(DBG_WMI, (DBGFMT "**WMI** Enter\n", DBGARG));

	for(i = 0; i < numAps; info++, i++)
	{
		DPRINTF(DBG_WMI,
			(DBGFMT "bssid %02x:%02x:%02x:%02x:%02x:%02x%s%s\n", DBGARG,
			info->bssid[0], info->bssid[1], info->bssid[2],
			info->bssid[3], info->bssid[4], info->bssid[5],
			info->bssFlags & WMI_PREAUTH_CAPABLE_BSS ? " preauth-cap" : "",
			info->bssFlags & WMI_PMKID_VALID_BSS ? " pmkid-valid" : ""));

		if(info->bssFlags & WMI_PMKID_VALID_BSS)
		{
			/* we skip bss if the pmkid is already valid */
			continue;
		}

		A_MEMZERO(buf, sizeof(buf));

		A_MEMCPY(buf, tag, 8); 
		pos = buf + 8;

		for(j = 0; j < 5; j++)
		{
			*pos++ = s_hex_table[(info->bssid[j] >> 4) & 0xf];
			*pos++ = s_hex_table[info->bssid[j] & 0xf];
		}

		*pos++ = s_hex_table[(i >> 4) & 0xf];
		*pos++ = s_hex_table[i & 0xf];

		*pos++ = s_hex_table[(info->bssFlags >> 4) & 0xf];
		*pos++ = s_hex_table[info->bssFlags & 0xf];

		*pos = '\0';

		DPRINTF(DBG_WMI, (DBGFMT "To host \"%s\".\n", DBGARG, buf));
	}

	return;
}

void
ar6000_disconnect_event(void *devt, A_UINT8 reason,
	A_UINT8 *bssid, A_UINT8 assocRespLen, A_UINT8 *assocInfo, A_UINT16 protocolReasonStatus)
{
	AR_SOFTC_T *ar = (AR_SOFTC_T *) devt;
	unsigned int process = 0;
#define DISCONNECT_PROCESS_CLEAR_COMMAND		0x00000001
#define DISCONNECT_PROCESS_FORCE_DISCONNECT		0x00000002

	DPRINTF(DBG_WMI, (DBGFMT "**WMI** Enter\n", DBGARG));

	DPRINTF(DBG_WMI,
		(DBGFMT "BSS %02x:%02x:%02x:%02x:%02x:%02x reason %u\n", DBGARG,
		bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5],
		reason));

    /*
     * If the event is due to disconnect cmd from the host, only they the target
     * would stop trying to connect. Under any other condition, target would  
     * keep trying to connect.
     *
     */
    if(reason == DISCONNECT_CMD)
    {
        ar->arConnectPending = FALSE;
    } else {
        ar->arConnectPending = TRUE;
        if ((reason == ASSOC_FAILED) && (protocolReasonStatus == 0x11)) {
            ar->arConnected = TRUE;
            return;
        }
    }

	/* Indicate that we are disconnected */
	ar->arConnected = FALSE;
	A_MEMZERO(ar->arBssid, sizeof(ar->arBssid));
	ar->arBssChannel = 0;

	/* Stop data traffic */
	htc_stop_data();

	if(htc_get_status() & DISCONNECT_IN_PROGRESS)
	{
		int why;

		why = (int) htc_get_status_data(DISCONNECT_SLOT);

		DPRINTF(DBG_WMI,
			(DBGFMT "Active disconnect - deliver results %d.\n", DBGARG, why));

		/* Deliver the right response */
		switch(why)
		{
			case 1:
				/* Get the result back to the WLAN Adapter layer */
				wlan_adp_oem_disassoc_rsp(WLAN_SYS_RES_SUCCESS);
				process |= DISCONNECT_PROCESS_CLEAR_COMMAND;
				break;
			case 2:
				/* Get the result back to the WLAN Adapter layer */
				wlan_adp_oem_deauth_rsp(WLAN_SYS_RES_SUCCESS);
				process |= DISCONNECT_PROCESS_CLEAR_COMMAND;
				break;
			case 3:
				/* This is an internal disconnect                */
				break;
			default:
				DPRINTF(DBG_WMI,
					(DBGFMT "Invalid response indicator %d.\n", DBGARG, why));
				break;
		}

		/* Clear the data */
		htc_set_status_data(DISCONNECT_SLOT, (void *) 0);

		/* Clear the status */
		htc_clear_status(DISCONNECT_IN_PROGRESS);

		/* Command is complete */
		if(process & DISCONNECT_PROCESS_CLEAR_COMMAND)
		{
			htc_command_complete();
		}
	}
	else
	{
		/* MATS - The mappings provided here might not be completely accurate */

		wlan_adp_disassoc_evt_info_type info;
		wlan_adp_deauth_evt_info_type ainfo;

		info.peer_mac_address = wlan_util_macaddress_to_uint64(bssid);
		ainfo.peer_mac_address = wlan_util_macaddress_to_uint64(bssid);

#if 0
		WLAN_SYS_REASON_UNSPECIFIED                  = 1,
		WLAN_SYS_REASON_PREV_AUTH_NOT_VALID          = 2,
		WLAN_SYS_REASON_DEAUTH_STA_LEFT              = 3,
		WLAN_SYS_REASON_DISASSOC_INACTIF             = 4,
		WLAN_SYS_REASON_DISASSOC_AP_CANNOT_SUP_STA   = 5,
		WLAN_SYS_REASON_CLS2_FRM_RX_FROM_UNAUTH_STA  = 6,
		WLAN_SYS_REASON_CLS3_FRM_RX_FROM_UNASSOC_STA = 7,
		WLAN_SYS_REASON_DISASSOC_STA_LEFT            = 8,
		WLAN_SYS_REASON_REQ_FROM_NOT_AUTH_STA        = 9
#endif

		/* Report the right thing if we are trying to connect */
		if(htc_get_status() & CONNECT_IN_PROGRESS)
		{
			int why;

			why = (int) htc_get_status_data(CONNECT_SLOT);

			DPRINTF(DBG_WMI,
				(DBGFMT "Disconnect with active connect %d.\n", DBGARG, why));

			/* Get the result back to the WLAN Adapter layer */
			switch(why)
			{
				case 1:
					wlan_adp_oem_auth_rsp(WLAN_SYS_RES_TIMEOUT);
					break;
				case 2:
					{
						wlan_adp_reassoc_rsp_info_type info;

						info.result_code = WLAN_SYS_RES_TIMEOUT;

						wlan_adp_oem_reassoc_rsp(&info);
					}
					break;
				default:
					DPRINTF(DBG_WMI,
						(DBGFMT "Invalid response indicator %d.\n",
						DBGARG, why));
					break;
			}

			/* Clear the data */
			htc_set_status_data(CONNECT_SLOT, (void *) 0);

			/* Clear the status */
			htc_clear_status(CONNECT_IN_PROGRESS);

			/* Accept more commands */
			htc_command_complete();

			/* Do this for sure */
			process |= DISCONNECT_PROCESS_FORCE_DISCONNECT;
		}
		else
		{
			switch(reason)
			{
				case NO_NETWORK_AVAIL:
					/* Scan gave no results with candidates */
					DPRINTF(DBG_WMI,
						(DBGFMT "NO_NETWORK_AVAIL (%d)\n", DBGARG, reason));
					info.reason = WLAN_SYS_REASON_UNSPECIFIED;
					wlan_adp_oem_dissasoc_ind(&info);
					break;
				case LOST_LINK:
					/* Got many beacon misses */
					DPRINTF(DBG_WMI,
						(DBGFMT "LOST_LINK (%d)\n", DBGARG, reason));
					process |= DISCONNECT_PROCESS_FORCE_DISCONNECT;
					wlan_adp_oem_lower_link_failure_ind();
					break;
				case DISCONNECT_CMD:
					/* Explicit disconnect from host */
					DPRINTF(DBG_WMI,
						(DBGFMT "DISCONNECT_CMD (%d)\n", DBGARG, reason));
					break;
				case BSS_DISCONNECTED:
					/* Other AP told us to go away */
					DPRINTF(DBG_WMI,
						(DBGFMT "BSS_DISCONNECTED (%d)\n", DBGARG, reason));
					process |= DISCONNECT_PROCESS_FORCE_DISCONNECT;
					info.reason = WLAN_SYS_REASON_UNSPECIFIED;
					wlan_adp_oem_dissasoc_ind(&info);
					break;
				case AUTH_FAILED:
					/* Authentication failed */
					DPRINTF(DBG_WMI,
						(DBGFMT "AUTH_FAILED (%d)\n", DBGARG, reason));
					ainfo.reason = WLAN_SYS_REASON_UNSPECIFIED;
					wlan_adp_oem_deauth_ind(&ainfo);
					break;
				case ASSOC_FAILED:
					/* Association failed */
					DPRINTF(DBG_WMI,
						(DBGFMT "ASSOC_FAILED (%d)\n", DBGARG, reason));
					ainfo.reason = WLAN_SYS_REASON_UNSPECIFIED;
					wlan_adp_oem_deauth_ind(&ainfo);
					break;
				case NO_RESOURCES_AVAIL:
					/* Internal resourse problem */
					DPRINTF(DBG_WMI,
						(DBGFMT "NO_RESOURCES_AVAIL (%d)\n", DBGARG, reason));
					info.reason = WLAN_SYS_REASON_UNSPECIFIED;
					wlan_adp_oem_dissasoc_ind(&info);
					break;
				case CSERV_DISCONNECT:
					/* Connectivity with current AP is bad */
					/* Need a better candidate.            */
					DPRINTF(DBG_WMI,
						(DBGFMT "CSERV_DISCONNECT (%d)\n", DBGARG, reason));
					process |= DISCONNECT_PROCESS_FORCE_DISCONNECT;
					wlan_adp_oem_synch_lost_ind();
					break;
				case INVALID_PROFILE:
					/* Bad connect request */
					DPRINTF(DBG_WMI,
						(DBGFMT "INVALID_PROFILE (%d)\n", DBGARG, reason));
					info.reason = WLAN_SYS_REASON_UNSPECIFIED;
					wlan_adp_oem_dissasoc_ind(&info);
					break;
				case 0:
					/* Connect trouble */
					DPRINTF(DBG_WMI,
						(DBGFMT "0 (%d)\n", DBGARG, reason));
					break;
				default:
					/* What is this .... */
					DPRINTF(DBG_WMI,
						(DBGFMT "<default> (%d)\n", DBGARG, reason));
					info.reason = WLAN_SYS_REASON_UNSPECIFIED;
					wlan_adp_oem_dissasoc_ind(&info);
					break;
			}
		}

		/* Do we need to do a real disconnect to prevent the card to */
		/* try to connect autonomously.                              */
		if(process & DISCONNECT_PROCESS_FORCE_DISCONNECT)
		{
			DPRINTF(DBG_WMI, (DBGFMT "Force disconnect.\n", DBGARG));

			/* Disconnect */
			if(wmi_disconnect_cmd(ar->arWmi) != A_OK)
			{
				DPRINTF(DBG_HTC,
					(DBGFMT "wmi_disconnect_cmd() failed.\n", DBGARG));
			}
			else
			{
				htc_set_status(DISCONNECT_IN_PROGRESS);
				htc_set_status_data(DISCONNECT_SLOT, (void *) 3);
			}
		}
	}
    
    qcom_tx_data_cleanup(ar);
}

void
ar6000_tkip_micerr_event(void *devt, A_UINT8 keyid, A_BOOL ismcast)
{
	static const char tag[] = "MLME-MICHAELMICFAILURE.indication";
	char buf[128];
	char *pos;
	wlan_adp_wpa_mic_failure_info_type info;

	DPRINTF(DBG_WMI, (DBGFMT "**WMI** Enter\n", DBGARG));

	DPRINTF(DBG_WMI,
		(DBGFMT "TKIP MIC error received for keyid %d %scast\n", DBGARG,
		keyid, ismcast ? "multi": "uni"));

	A_MEMZERO(buf, sizeof(buf));

	A_MEMCPY(buf, tag, 33); 
	pos = buf + 33;

	*pos++ = s_hex_table[(keyid >> 4) & 0xf];
	*pos++ = s_hex_table[keyid & 0xf];
	*pos++ = ' ';
	*pos++ = s_hex_table[(ismcast>> 4) & 0xf];
	*pos++ = s_hex_table[ismcast & 0xf];
	*pos = '\0';

	DPRINTF(DBG_WMI, (DBGFMT "To host \"%s\".\n", DBGARG, buf));

	/* Fill in the response structure the best we can */
	info.count = 1;
	info.mac_address[0] = 0;
	info.mac_address[1] = 0;
	info.mac_address[2] = 0;
	info.mac_address[3] = 0;
	info.mac_address[4] = 0;
	info.mac_address[5] = 0;
	info.key_id = keyid;
	info.key_type = 0;
	info.tsc[0] = 0;
	info.tsc[1] = 0;
	info.tsc[2] = 0;
	info.tsc[3] = 0;
	info.tsc[4] = 0;
	info.tsc[5] = 0;
	
	/* The the man upstairs .... */
	wlan_adp_oem_wpa_mic_failure_ind(&info);

	return;
}

void
ar6000_bitrate_rx(void *devt, A_INT32 rateKbps)
{
	AR_SOFTC_T *ar = (AR_SOFTC_T *) devt;
	wlan_dot11_opr_param_value_type value;

	DPRINTF(DBG_WMI, (DBGFMT "**WMI** Enter - rateKbps %u\n", DBGARG, rateKbps));

	ar->arBitRate = rateKbps;

	if(htc_get_status() & BITRATE_IN_PROGRESS)
	{
		DPRINTF(DBG_WMI, (DBGFMT "Active bitrate - deliver results.\n", DBGARG));

		switch(rateKbps)
		{
			case 1000: value.rate = SYS_WLAN_RATE_1_MBPS; break;
			case 2000: value.rate = SYS_WLAN_RATE_2_MBPS; break;
			case 5500: value.rate = SYS_WLAN_RATE_5_5_MBPS; break;
			case 11000: value.rate = SYS_WLAN_RATE_11_MBPS; break;
			default: value.rate = rateKbps; break;
		}
				
		/* Get the result back to the WLAN Adapter layer */
		wlan_adp_oem_opr_param_get_rsp(WLAN_TX_RATE, value);

		/* Clear the status */
		htc_clear_status(BITRATE_IN_PROGRESS);

		/* Command is complete */
		htc_command_complete();
	}
}

void
ar6000_txPwr_rx(void *devt, A_UINT8 txPwr)
{
	AR_SOFTC_T *ar = (AR_SOFTC_T *) devt;

	DPRINTF(DBG_WMI, (DBGFMT "**WMI** Enter - txPwr %u\n", DBGARG, txPwr));

	ar->arTxPwr = txPwr;
}

void
ar6000_ready_event(void *devt, A_UINT8 *datap, A_UINT8 phyCap, A_UINT32 sw_ver, A_UINT32 abi_ver)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *) devt;

    ar->arPhyCapability = phyCap;

    /* WMI is ready */
    ar->arWmiReady = TRUE;

    DPRINTF(DBG_WMI, (DBGFMT "**WMI** Enter\n", DBGARG));

    DPRINTF(DBG_WMI, (DBGFMT "MAC address = %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n", DBGARG,
        datap[0], datap[1], datap[2], datap[3], datap[4], datap[5]));
    DPRINTF(DBG_WMI, (DBGFMT "Target f/w version = 0x%x\n", DBGARG, sw_ver));

    DPRINTF(DBG_WMI, (DBGFMT "===============================> WMI READY\n", DBGARG));

    /* Set our MAC address to be used by the HTC task */
    htc_set_macaddress(datap);

    /* Signal HTC task that we are ready now when WMI is up */
    htc_ready();
}

void
ar6000_rssiThreshold_event(void *devt, WMI_RSSI_THRESHOLD_VAL newThreshold, A_INT16 rssi)
{
    A_PRINTF("rssi Threshold range = %d rssi = %d\n", newThreshold, 
             rssi + SIGNAL_QUALITY_NOISE_FLOOR);
    
    /* Send an event to upper layers */
}

void
ar6000_reportError_event(void *devt, WMI_TARGET_ERROR_VAL errorVal)
{
	DPRINTF(DBG_WMI,
		(DBGFMT "**WMI** Enter - Error = 0x%08x\n", DBGARG, errorVal));
	
	/* One error is reported at a time, and errorval is a bitmask */
	if(errorVal & (errorVal - 1))
	{
		return;
	}

	switch(errorVal)
	{
		case WMI_TARGET_DECRYPTION_ERR:
			DPRINTF(DBG_WMI,
				(DBGFMT "Error WMI_TARGET_DECRYPTION_ERR\n", DBGARG));
			{
				unsigned char s_addr[6];
				unsigned char d_addr[6];

				A_MEMZERO(s_addr, sizeof(s_addr));
				A_MEMZERO(d_addr, sizeof(d_addr));

				/* MATS - This might not be right */
				wlan_adp_oem_wpa_protect_framedropped_ind(s_addr, d_addr);
			}
			break;
		case WMI_TARGET_KEY_NOT_FOUND:
			DPRINTF(DBG_WMI,
				(DBGFMT "Error \"WMI_TARGET_KEY_NOT_FOUND\"\n", DBGARG));
			break;
		case WMI_TARGET_PM_ERR_FAIL:
			DPRINTF(DBG_WMI,
				(DBGFMT "Error \"WMI_TARGET_PM_ERR_FAIL\"\n", DBGARG));
			break;
		case WMI_TARGET_BMISS:
			DPRINTF(DBG_WMI,
				(DBGFMT "Error \"WMI_TARGET_BMISS\"\n", DBGARG));
			break;
		case WMI_PSDISABLE_NODE_JOIN:
			DPRINTF(DBG_WMI,
				(DBGFMT "Error \"WMI_PSDISABLE_NODE_JOIN\"\n", DBGARG));
			break;
		default:
			DPRINTF(DBG_WMI, (DBGFMT "Error <unknown>\n", DBGARG));
			break;
	}
}

void ar6000_hbChallengeResp_event(void* devt, A_UINT32 cookie, A_UINT32 source)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *) devt;

    if (source == APP_HB_CHALLENGE) {
        /* TODO: Report it to upper layers someway in case it wants
         * a positive acknowledgement */
    } else {
        /* This would ignore the replys that come in after their due time */
        if (cookie == ar->arHBChallengeResp.seqNum) {
            ar->arHBChallengeResp.outstanding = FALSE;
        }
    }
}

void ar6000_roam_tbl_event(void *devt, WMI_TARGET_ROAM_TBL *pTbl)
{
    A_UINT8 i;

    A_PRINTF("ROAM TABLE NO OF ENTRIES is %d ROAM MODE is %d\n",
              pTbl->numEntries, pTbl->roamMode);
    for (i= 0; i < pTbl->numEntries; i++) {
        A_PRINTF("[%d]bssid %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x ", i,
            pTbl->bssRoamInfo[i].bssid[0], pTbl->bssRoamInfo[i].bssid[1], 
            pTbl->bssRoamInfo[i].bssid[2],
            pTbl->bssRoamInfo[i].bssid[3],
            pTbl->bssRoamInfo[i].bssid[4],
            pTbl->bssRoamInfo[i].bssid[5]);
        A_PRINTF("RSSI %d RSSIDT %d LAST RSSI %d UTIL %d ROAM_UTIL %d"
                 " BIAS %d\n",
            pTbl->bssRoamInfo[i].rssi,
            pTbl->bssRoamInfo[i].rssidt,
            pTbl->bssRoamInfo[i].last_rssi,
            pTbl->bssRoamInfo[i].util,
            pTbl->bssRoamInfo[i].roam_util,
            pTbl->bssRoamInfo[i].bias);
    }
}

#define AR6000_PRINT_BSSID(_pBss)  do {     \
        A_PRINTF("%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x ",\
                 (_pBss)[0],(_pBss)[1],(_pBss)[2],(_pBss)[3],\
                 (_pBss)[4],(_pBss)[5]);  \
} while(0)

/*
 * Report the Roaming related data collected on the target
 */
void
ar6000_display_roam_time(WMI_TARGET_ROAM_TIME *p)
{
    A_PRINTF("Disconnect Data : BSSID: ");
    AR6000_PRINT_BSSID(p->disassoc_bssid);
    A_PRINTF(" RSSI %d DISASSOC Time %d NO_TXRX_TIME %d\n", 
             p->disassoc_bss_rssi,p->disassoc_time,
             p->no_txrx_time);
    A_PRINTF("Connect Data: BSSID: ");
    AR6000_PRINT_BSSID(p->assoc_bssid);
    A_PRINTF(" RSSI %d ASSOC Time %d TXRX_TIME %d\n", 
             p->assoc_bss_rssi,p->assoc_time,
             p->allow_txrx_time);
}

void ar6000_roam_data_event(void **devt, WMI_TARGET_ROAM_DATA *p)
{
    switch (p->roamDataType) {
        case ROAM_DATA_TIME:
            ar6000_display_roam_time(&p->u.roamTime);
            break;
        default:
            break;
    }
}

void ar6000_cac_event(void *devt, A_UINT8 ac, A_UINT8 cacIndication,
                    A_UINT8 statusCode, A_UINT8 *tspecSuggestion)
{
    WMM_TSPEC_IE    *tspecIe;
    
    /* 
     * This is the TSPEC IE suggestion from AP.
     * Suggestion provided by AP under some error
     * cases, could be helpful for the host app.
     * Check documentation.
     */
    tspecIe = (WMM_TSPEC_IE *)tspecSuggestion;

    /*
     * What do we do, if we get TSPEC rejection? One thought
     * that comes to mind is implictly delete the pstream...
     */
    A_PRINTF("AR6000 CAC notification. "
             "AC = %d, cacIndication = 0x%x, statusCode = 0x%x\n",
             ac, cacIndication, statusCode);
}

void
ar6000_channel_change_event(void *devt, A_UINT16 oldChannel, 
                            A_UINT16 newChannel)
{
    A_PRINTF("Channel Change notification\nOld Channel: %d, New Channel: %d\n",
             oldChannel, newChannel);
}

A_UINT8
ar6000_iptos_to_userPriority(A_UINT8 *pkt)
{
    struct iphdr *ipHdr = (struct iphdr *)pkt;
    A_UINT8 userPriority;
    
    /*
     * IP Tos format :
     *      (Refer Pg 57 WMM-test-plan-v1.2)
     * IP-TOS - 8bits
     *          : DSCP(6-bits) ECN(2-bits)
     *          : DSCP - P2 P1 P0 X X X
     *              where (P2 P1 P0) form 802.1D
     */
    userPriority = ipHdr->tos >> 5;
    
    DPRINTF(DBG_WMI, (DBGFMT "**WMI** Enter\n", DBGARG));
    
    return (userPriority & 0x7);
}

#ifdef CONFIG_HOST_TCMD_SUPPORT
void ar6000_tcmd_rx_report_event(void *devt, A_UINT8 * results, int len)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *) devt;
    TCMD_CONT_RX * rx_rep = (TCMD_CONT_RX *) results;

    ar->tcmdRxTotalPkt = rx_rep->u.report.totalPkt;
    ar->tcmdRxRssi = rx_rep->u.report.rssiInDBm;
    ar->tcmdRxReport = 1;
    
    wlan_adp_oem_report_ftm_test_results(results, len);
}
#endif

void ar6000_tx_retry_err_event(void *devt)
{
    DPRINTF(DBG_WMI, (DBGFMT "Tx retries reach maximum!\n", DBGARG));
}

void ar6000_snrThresholdEvent_rx(void *devt, WMI_SNR_THRESHOLD_VAL newThreshold, A_UINT8 snr)
{
    /* Send event to the upper layers */
}

void ar6000_lqThresholdEvent_rx(void *devt, WMI_LQ_THRESHOLD_VAL newThreshold, A_UINT8 lq)
{
    DPRINTF(DBG_WMI, (DBGFMT "lq threshold range %d, lq %d\n", DBGARG, newThreshold, lq));
}

void ar6000_ratemask_rx(void *devt, A_UINT16 ratemask)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)devt;
    
    ar->arRateMask = ratemask;
}

void ar6000_keepalive_rx(void *devt, A_UINT8 configured)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)devt;

    ar->arKeepaliveConfigured = configured;
}

void ar6000_bssInfo_event_rx(void *devt, A_UINT8 *datap, int len)
{
    /* TODO : need to customize */
    DPRINTF(DBG_WMI, (DBGFMT "bssinfo event\n", DBGARG));
}

A_STATUS ar6000_get_driver_cfg(void* devt, A_UINT16 cfgParam, void *result)
{
   A_STATUS ret = A_OK;

    switch(cfgParam)
    {
        case AR6000_DRIVER_CFG_GET_WLANNODECACHING:
           *((A_UINT32 *)result) = TRUE;
           break;
        case AR6000_DRIVER_CFG_LOG_RAW_WMI_MSGS:
           *((A_UINT32 *)result) = FALSE;
            break;
        default:
           ret = A_ERROR;
           break;
    }

    return ret;
}

void
ar6000_dbglog_init_done(void *devt)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)devt;
    
    ar->dbglog_init_done = TRUE;
}

void
ar6000_dbglog_event(void* devt, A_UINT32 dropped, 
                  A_UINT8 *buffer, A_UINT32 length)
{
    DPRINTF(DBG_WMI, (DBGFMT "Dropped logs: 0x%x\nDebug info length: %d\n", 
            DBGARG, dropped, length));

    /* Interpret the debug logs */
    dbglog_parse_debug_logs(buffer, length, FALSE);
}

void
ar6000_wow_list_event(void* devt, A_UINT8 num_filters, WMI_GET_WOW_LIST_REPLY *wow_reply)
{
    A_UINT8 i,j;
    
    /*Each event now contains exactly one filter, see bug 26613*/ 
    DPRINTF(DBG_WMI, (DBGFMT "WOW pattern %d of %d patterns\n",
            DBGARG, wow_reply->this_filter_num, wow_reply->num_filters));
    DPRINTF(DBG_WMI, (DBGFMT "wow mode = %s host mode = %s\n", DBGARG,
            (wow_reply->wow_mode == 0? "disabled":"enabled"),
            (wow_reply->host_mode == 1 ? "awake":"asleep")));

    /*If there are no patterns, the reply will only contain generic
      WoW information. Pattern information will exist only if there are
      patterns present. Bug 26716*/

    /* If this event contains pattern information, display it*/
    if (wow_reply->this_filter_num) { 
        i=0;
        DPRINTF(DBG_WMI, (DBGFMT "id=%d size=%d offset=%d\n", DBGARG,
                    wow_reply->wow_filters[i].wow_filter_id,
                    wow_reply->wow_filters[i].wow_filter_size, 
                    wow_reply->wow_filters[i].wow_filter_offset));
        DPRINTF(DBG_WMI, (DBGFMT "wow pattern = ", DBGARG));
        for (j=0; j< wow_reply->wow_filters[i].wow_filter_size; j++) {
            DPRINTF(DBG_WMI, (DBGFMT "%2.2x", DBGARG,
                    wow_reply->wow_filters[i].wow_filter_pattern[j]));
        } 

        DPRINTF(DBG_WMI, (DBGFMT "\nwow mask = ", DBGARG));
        for (j=0; j< wow_reply->wow_filters[i].wow_filter_size; j++) {
            DPRINTF(DBG_WMI, (DBGFMT "%2.2x", DBGARG,
                    wow_reply->wow_filters[i].wow_filter_mask[j]));
        } 
        DPRINTF(DBG_WMI, (DBGFMT "\n", DBGARG));
    }
}

HTC_ENDPOINT_ID
ar6000_ac2_endpoint_id ( void * devt, A_UINT8 ac)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *) devt;
    return(arAc2EndpointID(ar, ac));
}

A_UINT8
ar6000_endpoint_id2_ac(void * devt, HTC_ENDPOINT_ID ep )
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *) devt;
    return(arEndpoint2Ac(ar, ep ));
}

void
ar6000_pmkid_list_event(void *devt, A_UINT8 numPMKID, WMI_PMKID *pmkidList,
                        A_UINT8 *bssidList)
{
    A_UINT8 i, j;

    A_PRINTF("Number of Cached PMKIDs is %d\n", numPMKID);
    
    for (i = 0; i < numPMKID; i++) {
        A_PRINTF("\nPMKID %d ", i);
            for (j = 0; j < WMI_PMKID_LEN; j++) {
                A_PRINTF("%2.2x", pmkidList->pmkid[j]);
            }
        pmkidList++;
    }
}

void ar6000_dset_data_req(void *devt, A_UINT32 access_cookie, A_UINT32 offset,
                          A_UINT32 length, A_UINT32 targ_buf, A_UINT32 targ_reply_fn,
                          A_UINT32 targ_reply_arg)
{
}

void ar6000_dset_close(void *devt, A_UINT32 access_cookie)
{
}

void ar6000_dset_open_req(void *devt, A_UINT32 id, A_UINT32 targ_handle,
                          A_UINT32 targ_reply_fn, A_UINT32 targ_reply_arg)
{
}

/* WM7 event - ignored for now */
void ar6000_peer_event(void *devt, A_UINT8 eventCode, A_UINT8 *macAddr)
{
}

/* AP mode events - ignored for now */
void
ar6000_pspoll_event (void *context, A_UINT16 aid)
{
}

void
ar6000_dtimexpiry_event (void *context)
{
}

void 
ar6000_hci_event_rcv_evt(void *devt, WMI_HCI_EVENT *cmd)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *) devt;
    if (ar->evt_dispatcher) {
        ar->evt_dispatcher(cmd->buf, cmd->evt_buf_sz);
    }
}

NETWORK_TYPE ar6000_get_network_type(void *context)
{
    return 0;
}
