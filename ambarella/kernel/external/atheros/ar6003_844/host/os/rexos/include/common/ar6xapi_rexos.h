/*
 * Copyright (c) 2004-2005 Atheros Communications Inc.
 * All rights reserved.
 *
 * $ATH_LICENSE_HOSTSDK0_C$
 * 
 */
#ifndef _AR6XAPI_REXOS_H_
#define _AR6XAPI_REXOS_H_

void ar6000_channelList_rx(void *devt, A_INT8 numChan, A_UINT16 *chanList);
void ar6000_set_numdataendpts(void *devt, A_UINT32 num);
A_STATUS ar6000_control_tx(void *devt, void *osbuf, HTC_ENDPOINT_ID eid);
void ar6000_targetStats_event(void *devt, A_UINT8 *ptr, A_UINT32 len);
void ar6000_scanComplete_event(void *devt, A_STATUS status);
void ar6000_dset_data_req(void *devt, A_UINT32 access_cookie, A_UINT32 offset,
    A_UINT32 length, A_UINT32 targ_buf, A_UINT32 targ_reply_fn,
    A_UINT32 targ_reply_arg);
void ar6000_dset_close(void *devt, A_UINT32 access_cookie);
void ar6000_dset_open_req(void *devt, A_UINT32 id, A_UINT32 targ_handle,
    A_UINT32 targ_reply_fn, A_UINT32 targ_reply_arg);
A_UINT8 ar6000_iptos_to_userPriority(A_UINT8 *pkt);
void ar6000_ready_event(void *devt, A_UINT8 *datap, A_UINT8 phyCap, A_UINT32 sw_ver, A_UINT32 abi_ver);
void ar6000_connect_event(void *devt, A_UINT16 channel, A_UINT8 *bssid,
    A_UINT16 listenInterval, A_UINT16 beaconInterval,
    NETWORK_TYPE networkType, A_UINT8 beaconIeLen,
    A_UINT8 assocReqLen, A_UINT8 assocRespLen, A_UINT8 *assocInfo);
void ar6000_regDomain_event(void *devt, A_UINT32 regCode);
void ar6000_neighborReport_event(void *devt, int numAps, WMI_NEIGHBOR_INFO *info);
void ar6000_disconnect_event(void *devt, A_UINT8 reason,
    A_UINT8 *bssid, A_UINT8 assocRespLen, A_UINT8 *assocInfo,
    A_UINT16 protocolReasonStatus);
void ar6000_tkip_micerr_event(void *devt, A_UINT8 keyid, A_BOOL ismcast);
void ar6000_bitrate_rx(void *devt, A_INT32 rateKbps);
void ar6000_txPwr_rx(void *devt, A_UINT8 txPwr);
void ar6000_rssiThreshold_event(void *devt, WMI_RSSI_THRESHOLD_VAL newThreshold, A_INT16 rssi);
void ar6000_reportError_event(void *devt, WMI_TARGET_ERROR_VAL errorVal);
void ar6000_cac_event(void *devt, A_UINT8 ac, A_UINT8 cac_indication,
                    A_UINT8 statusCode, A_UINT8 *tspecSuggestion);
void ar6000_channel_change_event(void *devt, A_UINT16 oldChannel, A_UINT16 newChannel);
void ar6000_roam_tbl_event(void *devt, WMI_TARGET_ROAM_TBL *pTbl);
void ar6000_roam_data_event(void **devt, WMI_TARGET_ROAM_DATA *p);

#ifdef CONFIG_HOST_TCMD_SUPPORT
void ar6000_tcmd_rx_report_event(void *devt, A_UINT8 * results, int len);
#endif

void ar6000_dbglog_init_done(void *devt);
void ar6000_hbChallengeResp_event(void* devt, A_UINT32 cookie, A_UINT32 source);
void ar6000_tx_retry_err_event(void *devt);
void ar6000_snrThresholdEvent_rx(void *devt, WMI_SNR_THRESHOLD_VAL newThreshold, A_UINT8 snr);
void ar6000_lqThresholdEvent_rx(void *devt, WMI_LQ_THRESHOLD_VAL newThreshold, A_UINT8 lq);
void ar6000_ratemask_rx(void *devt, A_UINT16 ratemask);
void ar6000_keepalive_rx(void *devt, A_UINT8 configured);
void ar6000_bssInfo_event_rx(void *devt, A_UINT8 *datap, int len);
void ar6000_dbglog_event(void* devt, A_UINT32 dropped, A_UINT8 *buffer, A_UINT32 length);
void ar6000_indicate_tx_activity(void *devt, A_UINT8 TrafficClass, A_BOOL Active);
A_STATUS ar6000_get_driver_cfg(void* devt, A_UINT16 cfgParam, void *result);
void ar6000_wow_list_event(void *devt, A_UINT8 num_filters, WMI_GET_WOW_LIST_REPLY *wow_reply);
HTC_ENDPOINT_ID  ar6000_ac2_endpoint_id ( void * devt, A_UINT8 ac);
A_UINT8 ar6000_endpoint_id2_ac (void * devt, HTC_ENDPOINT_ID ep );
void ar6000_pmkid_list_event(void *devt, A_UINT8 numPMKID, 
                             WMI_PMKID *pmkidList, A_UINT8 *bssidList);
void ar6000_peer_event(void *devt, A_UINT8 eventCode, A_UINT8 *macAddr);
void ar6000_pspoll_event (void *context, A_UINT16 aid);
void ar6000_dtimexpiry_event (void *context);

void ar6000_hci_event_rcv_evt(void *devt, WMI_HCI_EVENT *cmd);
NETWORK_TYPE ar6000_get_network_type(void *context);

#endif /* _AR6XAPI_REXOS_H_ */
