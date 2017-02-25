#ifndef WLAN_ADP_DEF_H
#define WLAN_ADP_DEF_H
/*===========================================================================
                               WLAN_ADP_DEF . H

DESCRIPTION
  Header file for the WLAN ADAPTER Interfacing Data Structures.

  Copyright (c) 2004 by QUALCOMM, Incorporated.  All Rights Reserved.
===========================================================================*/

/*===========================================================================

                            EDIT HISTORY FOR FILE

           $Author: gijo $ $DateTime: 2012/03/29 04:00:18 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
11/15/05    sri    Added support for extended operational rates for 802.11g.
05/05/05    hba    Merged WPA relatives definition.
05/26/04    hba    Added MAC Configuration parameters holder
09/27/04    hba    Creation
===========================================================================*/


/*===========================================================================

                                INCLUDE FILES

===========================================================================*/

#ifdef MATS
#include "comdef.h"
#endif
#include "wlan_dot11_sys.h"
#include "wpa_itf.h"

/*===========================================================================

                              DATA DECLARATIONS

===========================================================================*/



/*===========================================================================

                       MAC REQUESTS/RESPONSES PRIMITIVES

===========================================================================*/

/*---------------------------------------------------------------------------
 WLAN_ADP_MAC_CONFIG_REQ_INFO_TYPE
   Contains MAC Configuration parameters!
---------------------------------------------------------------------------*/
typedef struct wlan_adp_mac_cfg_req_info_s
{

  wlan_dot11_preamble_enum  preamble_type;
  uint16                    listen_interval;
  uint16                    rts_threshold;
  uint16                    frag_threshold; /* Fragmentation threshold     */
  uint16                    max_tx_pwr;     /* Max Tx Power                */

} wlan_adp_mac_config_req_info_type;



/*---------------------------------------------------------------------------
 WLAN_ADP_SCAN_REQ_INFO_TYPE:
    This structure contains information to direct WLAN Adapter to initiate a
    SCAN Request.
---------------------------------------------------------------------------*/
typedef struct wlan_adp_scan_req_info_s
{
    wlan_dot11_bss_type_enum     bss_type;
    wlan_ieee_mac_address_type   bssid;
    wlan_dot11_ssid_type         ssid;
    wlan_dot11_scan_type_enum    scan_type;
    wlan_dot11_channel_list_type channel_list;
    /* Probe Delay is not included : it is supposed to be choosen
       appropriately at the OEM layer */
    uint16                       min_channel_time;
    uint16                       max_channel_time;
} wlan_adp_scan_req_info_type;



/*---------------------------------------------------------------------------
 WLAN_ADP_SCAN_RSP_INFO_TYPE:
    This structure contains information about the results of a SCAN Request.
---------------------------------------------------------------------------*/
#define  WLAN_SYS_MAX_SCAN_BSS        16    /*Arbitraly*/

typedef struct wlan_adp_scan_rsp_info_s
{
    wlan_dot11_bss_description_type   bss_descs[WLAN_SYS_MAX_SCAN_BSS];
    uint8                             RSSI[WLAN_SYS_MAX_SCAN_BSS];
    wlan_dot11_result_code_enum       result_code;
    uint16                            num_bss; /* Number of BSS descriptions 
                                                  carried within this structure*/
} wlan_adp_scan_rsp_info_type;


/*---------------------------------------------------------------------------
 WLAN_ADP_AUTH_REQ_INFO_TYPE:
    This structure contains information to initiate an Authentication Request.
---------------------------------------------------------------------------*/
typedef struct wlan_adp_auth_req_info_s
{
    wlan_ieee_mac_address_type      peer_mac_address; 
    wlan_dot11_auth_algo_type_enum  auth_type;
    uint16                          auth_failure_timeout;
} wlan_adp_auth_req_info_type;

/* wlan_dot11_result_code_enum is used to convey the result of an AUTH request */


/*---------------------------------------------------------------------------
 WLAN_ADP_ASSO_REQ_INFO_TYPE:
    This structure contains information to initiate an Association Request.
-----------------------------------------------------------------------------*/
typedef struct wlan_adp_assoc_req_info_s
{
    wlan_ieee_mac_address_type     peer_mac_address;
    wlan_dot11_cap_info_type       cap_info;
    uint16                         assoc_failure_timeout;
    uint16                         listen_interval;
    wpa_ie_type                    wpa_ie;
} wlan_adp_assoc_req_info_type;


/*---------------------------------------------------------------------------
 WLAN_ADP_ASSOC_RSP_INFO_TYPE:
    This structure contains information about the results of an assoc Request
    
    Note: When the WLAN DB is enabled, this would not be needed: for
    reference only
---------------------------------------------------------------------------*/
typedef struct wlan_adp_assoc_rsp_info_s
{
  wlan_adp_wmm_info_type            wmm_info;
  wlan_dot11_result_code_enum       result_code;
} wlan_adp_assoc_rsp_info_type;

/*---------------------------------------------------------------------------
 WLAN_ADP_REASSOC_RSP_INFO_TYPE:
    This structure contains information about the results of an reassoc
    Request
    
    Note: When the WLAN DB is enabled, this would not be needed: for
    reference only
---------------------------------------------------------------------------*/
typedef wlan_adp_assoc_rsp_info_type wlan_adp_reassoc_rsp_info_type;


/*---------------------------------------------------------------------------
 WLAN_ADP_DISASSOC_REQ_INFO_TYPE:
    This structure contains information to initiate a Disassociation Request.
---------------------------------------------------------------------------*/
typedef struct wlan_adp_disassoc_req_info_s
{
    wlan_ieee_mac_address_type      peer_mac_address; 
    wlan_dot11_reason_code_enum     reason_code;
} wlan_adp_disassoc_req_info_type;

/* wlan_dot11_result_code_enum is used to convey the result of an AUTH request */


/*---------------------------------------------------------------------------
 WLAN_ADP_DEAUTH_REQ_INFO_TYPE:
    This structure contains information to initiate a Deauthentication Request.
-----------------------------------------------------------------------------*/
typedef struct wlan_adp_deauth_req_info_s
{
    wlan_ieee_mac_address_type   peer_mac_address;
    wlan_dot11_reason_code_enum  reason_code;
} wlan_adp_deauth_req_info_type;


/*---------------------------------------------------------------------------
 WLAN_ADP_JOIN_REQ_INFO_TYPE:
    This structure contains information to initiate a Join Request.
---------------------------------------------------------------------------*/
typedef struct wlan_adp_join_req_info_s
{
    wlan_dot11_bss_description_type      	    ap_desc;
    wlan_dot11_supported_rates_type             operational_rates_set;
    wlan_dot11_extended_supported_rates_type    extended_operational_rates;
    uint16                               		join_failure_timeout;
    /*
    ** Indicate to the lower layer the selected AKM for this AP
    ** WPA_AKM_NONE means no WPA support for this AP
    */
    wpa_akm_suite_enum               akm;
    wpa_cipher_suite_enum            pairwise_cipher;
    wpa_cipher_suite_enum            group_cipher;

    wlan_adp_qos_type_enum           qos_type;
    wlan_adp_wmm_ie_type             wmm_ie;

} wlan_adp_join_req_info_type;

/* wlan_dot11_result_code_enum is used to convey the result of a Deauth request */


/*-------------------------------------------------------------------------
 WLAN_ADP_REASSOC_REQ_INFO_TYPE:
    This structure contains information to initiate a Reassociation Request.
--------------------------------------------------------------------------*/
typedef struct wlan_adp_reassoc_req_info_s
{
    wlan_ieee_mac_address_type   new_ap_mac_address;
    wlan_dot11_cap_info_type     cap_info;
    uint16                       reassoc_failure_timeout;
    uint16                       listen_interval;
    wpa_ie_type                  wpa_ie;
} wlan_adp_reassoc_req_info_type;

/* wlan_dot11_result_code_enum is used to convey the result of a Reassoc request */



/*-------------------------------------------------------------------------
 WLAN_ADP_RESET_REQ_INFO_TYPE:
    This structure contains information to initiate a Reset Request.
--------------------------------------------------------------------------*/
typedef struct wlan_adp_reset_req_info_s
{
    wlan_ieee_mac_address_type   sta_address;
    boolean                      set_default_mib;
} wlan_adp_reset_req_info_type;

/* wlan_dot11_result_code_enum is used to convey the result of a Reassoc request */


/*---------------------------------------------------------------------------
 WLAN_ADP_PWR_MGMT_REQ_INFO_TYPE:
    This structure contains information to initiate a Reset Request.
---------------------------------------------------------------------------*/
typedef struct wlan_adp_pwr_mgmt_req_info_s
{
    wlan_dot11_pwr_mgmt_mode_enum pwr_mode;
    boolean                       wake_up;  /*If true, STA wakes up immediately */
    boolean                       receiveDTIMs; /* STA should receives all DTIMs
                                              regardless of power mode */
} wlan_adp_pwr_mgmt_req_info_type;

/* wlan_dot11_result_code_enum is used to convey the result of a Power Management
   Request */

/*---------------------------------------------------------------------------
 WLAN_ADP_ADDTS_REQ_INFO_TYPE:
    This structure contains information to initiate an ADDTS Request.
-----------------------------------------------------------------------------*/
typedef struct wlan_adp_addts_req_info_s
{
  uint8                   dialog_token;
  wlan_adp_wmm_tspec_type tspec;
  uint16                  addts_failure_timeout;
} wlan_adp_addts_req_info_type;


/*---------------------------------------------------------------------------
 WLAN_ADP_ADDTS_RSP_INFO_TYPE:
    This structure contains information about the results of an ADDTS resp.
---------------------------------------------------------------------------*/
typedef struct wlan_adp_addts_rsp_info_s
{
  uint8                         dialog_token;
  wlan_adp_wmm_tspec_type       tspec;
  wlan_dot11_result_code_enum   result_code;
} wlan_adp_addts_rsp_info_type;


/*---------------------------------------------------------------------------
 WLAN_ADP_DELTS_REQ_INFO_TYPE:
    This structure contains information to initiate an DELTS Request.
-----------------------------------------------------------------------------*/
typedef struct wlan_adp_delts_req_info_s
{
  wlan_adp_wmm_ts_info_type   ts_info;
  wlan_dot11_reason_code_enum reason_code;
} wlan_adp_delts_req_info_type;


/*---------------------------------------------------------------------------
 WLAN_ADP_DELTS_RSP_INFO_TYPE:
    This structure contains information about the results of an DELTS response
---------------------------------------------------------------------------*/
typedef struct wlan_adp_delts_rsp_info_s
{
  wlan_adp_wmm_ts_info_type   ts_info;
  wlan_dot11_result_code_enum result_code;
} wlan_adp_delts_rsp_info_type;

#ifdef CONFIG_HOST_TCMD_SUPPORT
#define MAX_DATA_LEN  256
typedef struct 
{
  uint32  len;
  char data[MAX_DATA_LEN];
} wlan_adp_trp_test_data;
#endif

typedef struct wlan_adp_bthci_req_info_s
{
  uint8*  buf;
  uint16  size;
} wlan_adp_bthci_req_info_type;

typedef int (*wlan_adp_evt_dispatcher_f_ptr_type)(uint8*, uint16);
typedef int (*wlan_adp_data_dispatcher_f_ptr_type)(uint8*, uint16);

typedef struct wlan_adp_bt_dispatcher_req_info_s
{
  wlan_adp_evt_dispatcher_f_ptr_type  evt_dispatcher;
  wlan_adp_data_dispatcher_f_ptr_type data_dispatcher;
} wlan_adp_bt_dispatcher_req_info_type;

/*===========================================================================

                                    MAC EVENTS 

============================================================================*/

/*---------------------------------------------------------------------------
 WLAN_ADP_DISASSOC_EVT_INFO_TYPE:
    This structure contains information about a Disassociation event.
----------------------------------------------------------------------------*/
typedef struct wlan_adp_disassoc_evt_info_s
{
    wlan_ieee_mac_address_type   peer_mac_address;
    wlan_dot11_reason_code_enum  reason;
} wlan_adp_disassoc_evt_info_type;

/*--------------------------------------------------------------------------
 WLAN_ADP_DEAUTH_EVT_INFO_TYPE:
    This structure contains information about a Deauthentication event.
---------------------------------------------------------------------------*/
typedef struct wlan_adp_deauth_evt_info_s
{
    wlan_ieee_mac_address_type   peer_mac_address;
    wlan_dot11_reason_code_enum  reason;
} wlan_adp_deauth_evt_info_type;


/*--------------------------------------------------------------------------
 WLAN_ADP_POWER_STATE_EVT_INFO_TYPE:
    This structure contains information about a Disassociation event.
---------------------------------------------------------------------------*/
typedef struct wlan_adp_pwr_state_evt_info_s
{
  wlan_dot11_pwr_state_enum pwr_state;  
    
} wlan_adp_pwr_state_evt_info_type;


/*--------------------------------------------------------------------------
 WLAN_ADP_DELTS_EVT_INFO_TYPE:
    This structure contains information about a DELTS event.
---------------------------------------------------------------------------*/
typedef struct wlan_adp_delts_evt_info_s
{
  wlan_adp_wmm_ts_info_type   ts_info;
  wlan_dot11_reason_code_enum reason_code;
} wlan_adp_delts_evt_info_type;


/*---------------------------------------------------------------------------
 WLAN_ADP_RX_PKT_META_INFO:
   Containing meta information of a received LLC PDU
---------------------------------------------------------------------------*/
typedef struct wlan_adp_rx_pkt_metat_info_s
{
    wlan_ieee_mac_address_type src_mac_addr;
    wlan_ieee_mac_address_type dst_mac_addr;
    uint16                     rssi;
    uint8                      tid;           /* EDCA: UP; HCCA: TSID      */
} wlan_adp_rx_pkt_meta_info_type;



/*---------------------------------------------------------------------------
 WLAN_ADP_TX_PKT_META_INFO:
   Containing meta information of a LLC PDU to transmit
---------------------------------------------------------------------------*/
typedef struct wlan_adp_tx_pkt_metat_info_s
{
    wlan_ieee_mac_address_type dst_mac_addr;
    uint16                     tx_rate;
    uint8                      tid;           /* EDCA: UP; HCCA: TSID      */
} wlan_adp_tx_pkt_meta_info_type;

#endif /*WLAN_ADP_DEF_H */




