/*
 * Copyright (c) 2004-2005 Atheros Communications Inc.
 * All rights reserved.
 *
 * $ATH_LICENSE_HOSTSDK0_C$
 *
 */

#ifndef _QCOM_HTC_H_
#define _QCOM_HTC_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************/
/** HTC API functions and definitions.                                      **/
/*****************************************************************************/

struct htc_request
{
	struct htc_request *next;
	unsigned int cmd;
	wlan_trp_adp_req_type req_info;
};
struct HTC_PACKET;

int htc_get_sid_up(int sId);
void htc_ready(void);
void htc_set_rssi(int rssi);
void htc_set_macaddress(A_UINT8 *datap);
void htc_enable_firmware_upload(void);
A_UINT8 *htc_get_macaddress(void);
void htc_set_failure(int failure);
#define FAIL_NONFATAL	1
#define FAIL_FATAL		2
void htc_set_status_data(int index, void *data);
void *htc_get_status_data(int index);
void htc_set_status(unsigned int status);
#ifdef CONFIG_HOST_TCMD_SUPPORT
void htc_set_test_mode(void);
#endif
A_UINT8 htc_get_test_mode(void);
unsigned int htc_get_status(void);
void htc_clear_status(unsigned int status);
struct htc_request *htc_alloc_request(void);
void htc_free_request(struct htc_request *req);
void htc_task_signal(unsigned int val);
void htc_task_command(struct htc_request *req);
void htc_task_tx_data(void *pkt, int up);
void htc_task_rx_data(void *pkt, A_INT8 rssi);
void htc_task_tx_bt_data(void *pkt);
int htc_task_query(int query);
#define HTC_QUERY_TX_OPEN	1
#define HTC_QUERY_CMD_OPEN	2
void *htc_device_handle(void);
int htc_task_init(void);
int htc_task_exit(void);
void htc_command_complete(void);
void htc_stop_data(void);
void htc_start_data(void);
wlan_adp_scan_req_info_type *htc_get_scan_filter(void);
void htc_set_wmm_info(wlan_adp_wmm_info_type *wmm);
void *htc_task(void* data);

/*****************************************************************************/
/** Signals                                                                 **/
/*****************************************************************************/

#define HTC_START_SIG			0x00000001	/* open          */
#define HTC_STOP_SIG			0x00000002	/* close         */

#define HTC_COMMAND_SIG			0x00000004
#define HTC_READY_SIG			0x00000008	/* start is done */
#define HTC_HIFDSR_SIG			0x00000010	/* Got an HIF DSR request */
#define HTC_FAIL_SIG			0x00000020
#define HTC_TX_DATA_SIG			0x00000040
#define HTC_RX_DATA_SIG			0x00000080

/*****************************************************************************/
/** Progress markers                                                        **/
/*****************************************************************************/

#define SCAN_IN_PROGRESS		0x00000001
#define STATS_IN_PROGRESS		0x00000002
#define BITRATE_IN_PROGRESS		0x00000004
#define CONNECT_IN_PROGRESS		0x00000008
#define DISCONNECT_IN_PROGRESS	0x00000010

#define NUM_STATUS_SLOTS		8

#define STATS_SLOT				0
#define CONNECT_SLOT			1
#define DISCONNECT_SLOT			2

/*****************************************************************************/
/** Event function registered with the HTC layer                            **/
/*****************************************************************************/

A_STATUS qcom_avail_ev(void *context, void *hif_handle);
A_STATUS qcom_unavail_ev(void *context, void *hif_handle);
void qcom_tx_complete(void *Context, HTC_PACKET *pPacket);
void qcom_rx_refill(void *Context, HTC_ENDPOINT_ID Endpoint);
void qcom_rx(void *Context, HTC_PACKET *pPacket);
void qcom_target_failure(void *Instance, A_STATUS Status);

void dbglog_parse_debug_logs(A_UINT8 *datap, A_UINT32 len, A_BOOL closefile);
int qcom_dbglog_get_debug_logs(void *arg, A_BOOL closefile);

#ifdef __cplusplus
}
#endif

#endif
