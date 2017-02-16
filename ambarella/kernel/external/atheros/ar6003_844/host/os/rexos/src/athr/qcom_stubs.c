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
#include <pthread.h>
#include <unistd.h>
#endif

/* Atheros include files */
#include <athdefs.h>
#include <a_types.h>
#include <a_osapi.h>

/* Qualcomm include files */
#ifdef ATHR_EMULATION
#include "qcom_stubs.h"
#endif
#include "wlan_dot11_sys.h"
#include "wlan_adp_def.h"
#include "wlan_adp_oem.h"
#include "wlan_trp.h"
#include "wlan_util.h"
#include "rex.h"

/* Atheros platform include files */
#include "a_debug.h"

#define MAX_TX_PACKETS	0

static int s_max_tx_packets = MAX_TX_PACKETS;

static int s_ready = FALSE;
static unsigned int s_signal = 0;

static rex_tcb_type htc_task;
rex_tcb_type *htc_task_ptr = &htc_task;
static pthread_cond_t s_cond = PTHREAD_COND_INITIALIZER;

static wlan_adp_scan_rsp_info_type s_scan_rsp_info;

/*****************************************************************************/
/** API functions exposed to the implementation                             **/
/*****************************************************************************/

void set_max_tx_packets(unsigned int max)
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter - max %d\n", DBGARG, max));

	s_max_tx_packets = max;

	return;
}

/*****************************************************************************/
/** Async indications                                                       **/
/*****************************************************************************/

void wlan_adp_oem_ready_ind(int perror)  
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter %d\n", DBGARG, perror));

	if(perror == 0)
	{
		DPRINTF(DBG_WLAN_ATH, (DBGFMT "***********************************\n", DBGARG));
		DPRINTF(DBG_WLAN_ATH, (DBGFMT "** READY READY READY READY READY **\n", DBGARG));
		DPRINTF(DBG_WLAN_ATH, (DBGFMT "***********************************\n", DBGARG));

		s_ready = TRUE;
	}
	else
	{
		DPRINTF(DBG_WLAN_ATH, (DBGFMT "***********************************\n", DBGARG));
		DPRINTF(DBG_WLAN_ATH, (DBGFMT "** FAIL  FAIL  FAIL  FAIL  FAIL  **\n", DBGARG));
		DPRINTF(DBG_WLAN_ATH, (DBGFMT "***********************************\n", DBGARG));
	}

	return;
}

void wlan_adp_oem_dissasoc_ind(wlan_adp_disassoc_evt_info_type* ptr_evt_info)
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter\n", DBGARG));

	return;
}

void wlan_adp_oem_deauth_ind(wlan_adp_deauth_evt_info_type* ptr_evt_info)
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter\n", DBGARG));

	return;
}

void wlan_adp_oem_synch_lost_ind(void)
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter\n", DBGARG));

	return;
}

void wlan_adp_oem_lower_link_failure_ind(void)
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter\n", DBGARG));

	return;
}

void wlan_adp_oem_fatal_failure_ind(void)
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter\n", DBGARG));

	DPRINTF(DBG_WLAN_ATH,
		(DBGFMT "***********************************\n", DBGARG));
	DPRINTF(DBG_WLAN_ATH,
		(DBGFMT "** FATAL FATAL FATAL FATAL FATAL **\n", DBGARG));
	DPRINTF(DBG_WLAN_ATH,
		(DBGFMT "***********************************\n", DBGARG));

	/* Wait for things to settle */
	sleep(2);

	exit(0);
}

void wlan_adp_oem_pwr_state_ind(wlan_adp_pwr_state_evt_info_type* ptr_evt_info)
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter\n", DBGARG));

	return;
}

void wlan_adp_oem_wpa_mic_failure_ind(wlan_adp_wpa_mic_failure_info_type* ptr_info)
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter\n", DBGARG));

	return;
}

void wlan_adp_oem_wpa_protect_framedropped_ind(byte* sa_addr1, byte* da_addr2)
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter\n", DBGARG));

	return;
}

void wlan_trp_adp_cmd_complete_ind(void)
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter\n", DBGARG));

	return;
}

/*****************************************************************************/
/** Responses                                                               **/
/*****************************************************************************/

void wlan_adp_oem_suspend_rsp(int perror)  
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter %d\n", DBGARG, perror));

	return;
}

void wlan_adp_oem_resume_rsp(int perror)  
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter %d\n", DBGARG, perror));

	return;
}

void wlan_adp_oem_scan_rsp(wlan_adp_scan_rsp_info_type *scan_rsp_info)  
{
	DPRINTF(DBG_WLAN_ATH,
		(DBGFMT "STUB: Enter 0x%08x\n", DBGARG, (unsigned int) scan_rsp_info));
	DPRINTF(DBG_WLAN_ATH,
		(DBGFMT "STUB: Found %d BSSs\n", DBGARG, scan_rsp_info->num_bss));

	A_MEMCPY(&s_scan_rsp_info, scan_rsp_info,
		sizeof(wlan_adp_scan_rsp_info_type));

	return;
}

void wlan_adp_oem_join_rsp(wlan_dot11_result_code_enum result_code)
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter 0x%08x\n", DBGARG, result_code));

	return;
}

void wlan_adp_oem_auth_rsp(wlan_dot11_result_code_enum result_code)
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter 0x%08x\n", DBGARG, result_code));

	return;
}

void wlan_adp_oem_assoc_rsp(wlan_adp_assoc_rsp_info_type *assoc_rsp_info)
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter 0x%08x\n", DBGARG,
		assoc_rsp_info->result_code));

	return;
}

void wlan_adp_oem_reassoc_rsp(wlan_adp_reassoc_rsp_info_type *reassoc_rsp_info)
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter 0x%08x\n", DBGARG,
		reassoc_rsp_info->result_code));

	return;
}

void wlan_adp_oem_deauth_rsp(wlan_dot11_result_code_enum result_code)
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter 0x%08x\n", DBGARG, result_code));

	return;
}

void wlan_adp_oem_pwr_mgmt_rsp(wlan_dot11_result_code_enum result_code)
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter 0x%08x\n", DBGARG, result_code));

	return;
}

void wlan_adp_oem_disassoc_rsp(wlan_dot11_result_code_enum result_code)
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter 0x%08x\n", DBGARG, result_code));

	return;
}

void wlan_adp_oem_mib_get_rsp(wlan_dot11_result_code_enum status,
	wlan_mib_item_value_type *value)
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter 0x%08x\n", DBGARG, status));

	return;
}

void wlan_adp_oem_mib_set_rsp(wlan_dot11_result_code_enum status,
	wlan_mib_item_id_enum_type item_id)
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter 0x%08x\n", DBGARG, status));

	return;
}

void wlan_adp_oem_reset_rsp(wlan_dot11_result_code_enum result_code)
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter 0x%08x\n", DBGARG, result_code));

	return;
}

void wlan_adp_oem_mac_config_rsp(int perror)  
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter %d\n", DBGARG, perror));

	return;
}

void wlan_adp_oem_wpa_setkeys_rsp(void)  
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter\n", DBGARG));

	return;
}

void wlan_adp_oem_wpa_delkeys_rsp(void)  
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter\n", DBGARG));

	return;
}

void wlan_adp_oem_wpa_set_protection_rsp(void)  
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter\n", DBGARG));

	return;
}

int wlan_adp_oem_opr_param_get_rsp(wlan_dot11_opr_param_id_enum param_id,
	wlan_dot11_opr_param_value_type param_value)
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter 0x%08x\n", DBGARG, param_id));

	return(0);
}

void wlan_adp_oem_addts_rsp(wlan_adp_addts_rsp_info_type *addts_rsp_info)  
{
	DPRINTF(DBG_WLAN_ATH,
		(DBGFMT "STUB: Enter %d\n", DBGARG, addts_rsp_info->result_code));

	return;
}

void wlan_adp_oem_delts_rsp(wlan_adp_delts_rsp_info_type *delts_rsp_info)
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter\n", DBGARG));

	return;
}

/*****************************************************************************/
/** Data RX/TX functions                                                    **/
/*****************************************************************************/
int wlan_adp_oem_rx_data(byte *ptr_rx_buffer, uint16 buffer_len,
	wlan_adp_rx_pkt_meta_info_type* ptr_meta_info)
{
	unsigned char srcaddr[6];
	unsigned char dstaddr[6];
	static int i = 0;

	i++;
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter data @ 0x%08x len %d\n", DBGARG,
		(unsigned int) ptr_rx_buffer, buffer_len));

	wlan_util_uint64_to_macaddress(srcaddr, ptr_meta_info->src_mac_addr);
	wlan_util_uint64_to_macaddress(dstaddr, ptr_meta_info->dst_mac_addr);

	DPRINTF(DBG_WLAN_ATH, (DBGFMT "src %02x:%02x:%02x:%02x:%02x:%02x => dst %02x:%02x:%02x:%02x:%02x:%02x RSSI %u %d\n", DBGARG,
		srcaddr[0], srcaddr[1], srcaddr[2],
		srcaddr[3], srcaddr[4], srcaddr[5],
		dstaddr[0], dstaddr[1], dstaddr[2],
		dstaddr[3], dstaddr[4], dstaddr[5],
		ptr_meta_info->rssi, i));

	return(0);
}


static unsigned char s_arp_pkt1[] =
{
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x03, 0x7f, 0x01, 0x60, 0xdf,
    0x08, 0x06, 0x00, 0x01,

    0x08, 0x00, 0x06, 0x04, 0x00, 0x01,
#if 0
    0x00, 0x03, 0x7f, 0x01, 0x60, 0xdf, 0xc0, 0xa8, 0x01, 0x0b,
#else
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0xa8, 0x01, 0x0b,
#endif

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xa8, 0x01, 0x0a,
};

int wlan_adp_oem_get_next_tx_data(byte *ptr_tx_buffer,
	wlan_adp_tx_pkt_meta_info_type* ptr_meta_info)
{
	static int i = 0;
	static wlan_adp_tx_pkt_meta_info_type meta_info;
	int len = 0;
	A_UINT8 dstaddr[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	const int offset = 14;

	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter %d\n", DBGARG, i));

	/* return(0); */

	if(i < s_max_tx_packets)
	{
		meta_info.dst_mac_addr = wlan_util_macaddress_to_uint64(dstaddr);
		meta_info.tx_rate = 0;

		A_MEMCPY(ptr_meta_info, &meta_info,
			sizeof(wlan_adp_tx_pkt_meta_info_type));

		/* Make each packet a bit different */
		s_arp_pkt1[31] = (i + 1) & 0xff;

		len = sizeof(s_arp_pkt1) - offset;

		A_MEMCPY(ptr_tx_buffer, &s_arp_pkt1[offset], len);

		i++;
	}
	
	return(len);
}

int wlan_adp_oem_get_next_tx_pkt(dsm_item_type **tx_pkt,
	wlan_adp_tx_pkt_meta_info_type* ptr_meta_info)
{
	static int i = 0;
	static wlan_adp_tx_pkt_meta_info_type meta_info;
	int len = 0;
	A_UINT8 dstaddr[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	const int offset = 14;

	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter %d\n", DBGARG, i));

	if(i < s_max_tx_packets)
	{
		meta_info.dst_mac_addr = wlan_util_macaddress_to_uint64(dstaddr);
		meta_info.tx_rate = 0;

		A_MEMCPY(ptr_meta_info, &meta_info,
			sizeof(wlan_adp_tx_pkt_meta_info_type));

		/* Make each packet a bit different */
		s_arp_pkt1[31] = (i + 1) & 0xff;

		len = sizeof(s_arp_pkt1) - offset;

		/* Give this out */
		*tx_pkt = (unsigned char **) &s_arp_pkt1[offset];

		i++;
	}
	
	return(len);
}

int dsm_pullup(dsm_item_type **pkt, char *data, int len)
{
	A_MEMCPY(data, *pkt, len);

	return(len);
}

void dsm_free_packet(dsm_item_type **pkt)
{
	return;
}

#ifdef CONFIG_HOST_TCMD_SUPPORT
void wlan_adp_oem_report_ftm_test_results(byte* ptr_rx_buffer,
	uint32 buffer_len)
{
  DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter\n", DBGARG));
}
#endif

/*****************************************************************************/
/** Utility functions                                                       **/
/*****************************************************************************/

void wlan_util_uint64_to_macaddress(byte *mac_hw_addr, uint64 uint64_addr)
{
	mac_hw_addr[0] = (uint64_addr >> 40) & 0xff;
	mac_hw_addr[1] = (uint64_addr >> 32) & 0xff;
	mac_hw_addr[2] = (uint64_addr >> 24) & 0xff;
	mac_hw_addr[3] = (uint64_addr >> 16) & 0xff;
	mac_hw_addr[4] = (uint64_addr >>  8) & 0xff;
	mac_hw_addr[5] = (uint64_addr >>  0) & 0xff;

	DPRINTF(DBG_HTC, (DBGFMT "BSS OUT %02x:%02x:%02x:%02x:%02x:%02x.\n", DBGARG,
		mac_hw_addr[0], mac_hw_addr[1], mac_hw_addr[2],
		mac_hw_addr[3], mac_hw_addr[4], mac_hw_addr[5]));

	return;
}

uint64 wlan_util_macaddress_to_uint64(byte *hw_addr)
{
	uint64 val;

	DPRINTF(DBG_HTC, (DBGFMT "BSS IN %02x:%02x:%02x:%02x:%02x:%02x.\n", DBGARG,
		hw_addr[0], hw_addr[1], hw_addr[2],
		hw_addr[3], hw_addr[4], hw_addr[5]));

	val = (uint64) hw_addr[0] <<  40 | 
		  (uint64) hw_addr[1] <<  32 |
		  (uint64) hw_addr[2] <<  24 |
		  (uint64) hw_addr[3] <<  16 |
		  (uint64) hw_addr[4] <<   8 |
		  (uint64) hw_addr[5] <<   0;

	return(val);
}

void wlan_util_set_hw_mac_addr(uint64  mac_address)
{
	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Enter\n", DBGARG));

	return;
}

/*****************************************************************************/
/** API functions exposed from the implementation                           **/
/*****************************************************************************/

int stubs_is_ready(void)
{
	return(s_ready);
}

wlan_dot11_bss_description_type *stubs_get_bss_desc_by_index(int i)
{
	if(s_scan_rsp_info.num_bss <= i)
	{
		return(NULL);
	}

	DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Found \"%d\".\n", DBGARG, i));

	return(&s_scan_rsp_info.bss_descs[i]);
}

wlan_dot11_bss_description_type *stubs_get_bss_desc_by_name(char *name)
{
	int l;
	int i;

	l = strlen(name);

	for(i = 0; i < s_scan_rsp_info.num_bss; i++)
	{
		if(A_MEMCMP(s_scan_rsp_info.bss_descs[i].ssid.ssid, name, l) == 0)
		{
			DPRINTF(DBG_WLAN_ATH, (DBGFMT "STUB: Found \"%s\".\n", DBGARG, name));
			return(&s_scan_rsp_info.bss_descs[i]);
		}
	}
	
	return(NULL);
}

static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;

void rex_clr_sigs(rex_tcb_type tcb, rex_sigs_type sigs)
{
	/* Clear the signal */
	A_MUTEX_LOCK(&s_mutex);
	s_signal &= ~((unsigned int) sigs);
	A_MUTEX_UNLOCK(&s_mutex);

	return;
}

rex_sigs_type rex_wait(rex_sigs_type sigs)
{
	static pthread_mutex_t my_mutex = PTHREAD_MUTEX_INITIALIZER;
    int rc;
	unsigned int signal;

again:

	/* Get the signal */
	A_MUTEX_LOCK(&s_mutex);
	signal = s_signal;
	s_signal = 0;   /* Clear out the signals */
	A_MUTEX_UNLOCK(&s_mutex);

	/* Wait for signals if there are no signals pending */
	if(signal == 0)
	{
		if((rc = pthread_mutex_lock(&my_mutex)) != 0)
		{
			goto error;
		}

		if((rc = pthread_cond_wait(&s_cond, &my_mutex)) != 0)
		{
			goto error;
		}

		pthread_mutex_unlock(&my_mutex);

		goto again;
	}

error:

	return(signal);
}

rex_sigs_type rex_get_sigs(rex_tcb_type tcb)
{
	unsigned int signal;

	/* Get the signal */
	A_MUTEX_LOCK(&s_mutex);
	signal = s_signal;
	A_MUTEX_UNLOCK(&s_mutex);

	return(signal);
}

void rex_set_sigs(rex_tcb_type tcb, rex_sigs_type sigs)
{
	/* Set the signal */
	A_MUTEX_LOCK(&s_mutex);
	s_signal |= (unsigned int) sigs;
	A_MUTEX_UNLOCK(&s_mutex);

	(void) pthread_cond_signal(&s_cond);

	return;
}

/* In RexOS, WLAN Rx and Tx run in same thread contexts, so WLAN Rx context needs to check
 * for any commands/data queued for WLAN */
void rex_check_drv_tx(void)
{
	/* dummy implementation in emulation */
}

void
dump_frame(A_UINT8 *frm, A_UINT32 len)
{
    unsigned int    i;
    printf("\n----------------------------------------------\n");
    for(i = 0; i < len; i++) {
        printf("0x%02x ", frm[i]);
        if((i+1) % 16 == 0) 
            printf("\n");
    }
    printf("\n===============================================\n\n");
}

int rex_evt_dispatcher_stub(A_UINT8* buf, A_UINT16 size)
{
    printf("PAL recv HCI event ->\n");
    dump_frame(buf, size);
    return 0;
}

int rex_data_dispatcher_stub(A_UINT8* buf, A_UINT16 size)
{
	static numpkt = 0;
	
    printf("PAL recv HCI data (%d) -> \n", numpkt++);
    dump_frame(buf, size);
    return 0;
}
