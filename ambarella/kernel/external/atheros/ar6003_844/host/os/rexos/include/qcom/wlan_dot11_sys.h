#ifndef WLAN_DOT11_SYS_H
#define WLAN_DOT11_SYS_H
/*===========================================================================
                               WLAN_DOT11_SYS.H

DESCRIPTION
  Header file for the WLAN 802.11 SYSTEM Data Structures.

  Copyright (c) 2004 by QUALCOMM, Incorporated.  All Rights Reserved.
===========================================================================*/

/*===========================================================================

                            EDIT HISTORY FOR FILE

           $Author: gijo $ $DateTime: 2012/03/29 04:00:18 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
12/17/05    lyr    Added QoS definitions
11/09/05    sri	   Added support for extended rates supported in 802.11g
09/26/05    hba    Added PREAMBLE_AUTO to let the OEM layer chooses the right
                   PREAMBLE in case the user chooses an AUTOMATIC preamble.
05/05/05    hba    Merged WPA relative definitions.
04/26/05    hba    Added Preamble type and change max channel number
                   back to 14 
02/22/05    hba    Change max channels number to 11 to accommodate with PHG.
12/01/04    lyr    Changed BSS Type to reflect sys_wlan.h type
09/27/04    hba    Creation

===========================================================================*/


/*===========================================================================

                                INCLUDE FILES

===========================================================================*/
#ifdef MATS
#include "comdef.h"
#endif

#include "sys_wlan.h"        /* For data types shared with CM/SD/MMoC      */

#include "wpa_itf.h"

#include "wlan_adp_qos.h"

/*===========================================================================

                              DATA DECLARATIONS

===========================================================================*/

/* IEEE MAC ADDRESS FOR WLAN                                               */
typedef uint64   wlan_ieee_mac_address_type;

/*---------------------------------------------------------------------------
 WLAN_AUTH_ALGO_TYPE_ENUM:
    This is an enumeration of the different types of authentication 
    algorithms supported within the 802.11 standard at the MAC level.
---------------------------------------------------------------------------*/
typedef enum
{
    WLAN_SYS_AUTH_ALGO_OPEN_SYSTEM = 0,
    WLAN_SYS_AUTH_ALGO_SHARED_KEY  = 1
} wlan_dot11_auth_algo_type_enum;


/*---------------------------------------------------------------------------
 WLAN_PWR_MGMT_MODE_ENUM:
    This is an enumeration of the various WLAN power mode.
---------------------------------------------------------------------------*/
typedef enum
{
    WLAN_SYS_PWR_ACTIVE_MODE = 0,
    WLAN_SYS_PWR_SAVE_MODE   = 1
} wlan_dot11_pwr_mgmt_mode_enum;

/*---------------------------------------------------------------------------
 WLAN_PWR_STATE_ENUM:
    This is an enumeration of the various WLAN power mode.
---------------------------------------------------------------------------*/
typedef enum
{
    WLAN_SYS_PWR_STATE_AWAKE = 0,
    WLAN_SYS_PWR_STATE_SLEEP = 1
} wlan_dot11_pwr_state_enum;

/*---------------------------------------------------------------------------
 WLAN_AUTH_BSS_TYPE_ENUM:
    This is an enumeration of the different types of Basic Service Set (BSS).
---------------------------------------------------------------------------*/

typedef sys_wlan_bss_e_type wlan_dot11_bss_type_enum;


/*---------------------------------------------------------------------------
 WLAN_SCAN_TYPE_ENUM:
    This is an enumeration of the different SCAN Types.
---------------------------------------------------------------------------*/
typedef enum
{
    WLAN_SYS_SCAN_PASSIVE = 0,
    WLAN_SYS_SCAN_ACTIVE  = 1
} wlan_dot11_scan_type_enum;


/*---------------------------------------------------------------------------
 WLAN_RESULT_CODE_ENUM:
    This is an enumeration of the possible result code.
---------------------------------------------------------------------------*/
typedef enum
{
    WLAN_SYS_RES_SUCCESS           = 0,
    WLAN_SYS_RES_INVALID_PARAMS    = 1,
    WLAN_SYS_RES_NOT_SUPPORTED     = 2,
    WLAN_SYS_RES_TOO_MANY_REQUESTS = 3,
    WLAN_SYS_RES_TIMEOUT           = 4,
    WLAN_SYS_RES_REFUSED           = 5
} wlan_dot11_result_code_enum;



/*---------------------------------------------------------------------------
 WLAN_STATUS_CODE_ENUM:
    This is an enumeration of the possible status codes.
---------------------------------------------------------------------------*/
typedef enum
{
    WLAN_SYS_STATUS_MSG_SUCCESS               = 0,
    WLAN_SYS_STATUS_UNSPECIFIED_FAILURE       = 1,
    WLAN_SYS_STATUS_REQ_CAP_NOT_SUPPORTED     = 10,
    WLAN_SYS_STATUS_REASSOC_DENIED            = 11,
    WLAN_SYS_STATUS_ASSOC_DENIED              = 12,
    WLAN_SYS_STATUS_AUTH_ALGO_NOT_SUPPORTED   = 13,
    WLAN_SYS_STATUS_AUTH_OUT_OF_SEQUENCE      = 14,
    WLAN_SYS_STATUS_AUTH_CHLNG_TEXT_FAIL      = 15,
    WLAN_SYS_STATUS_AUTH_TIMEOUT              = 16,
    WLAN_SYS_STATUS_AP_CANNOT_SUPP_STA        = 17,
    WLAN_SYS_STATUS_BASIC_RATES_NOT_SUPPORTED = 18,
    WLAN_SYS_STATUS_UNSPECIFIED_QOS_FAILURE   = 32,
    WLAN_SYS_STATUS_ASSOC_DENIED_INSUFF_BW    = 33,
    WLAN_SYS_STATUS_ASSOC_DENIED_BAD_CHANNEL  = 34,
    WLAN_SYS_STATUS_ASSOC_DENIED_NON_QSTA     = 35,
    WLAN_SYS_STATUS_REQUEST_DECLINED          = 37,
    WLAN_SYS_STATUS_REQ_FAIL_INVALID_PARAMS   = 38,
    WLAN_SYS_STATUS_TS_REJECT_MODIFIED        = 39,
    WLAN_SYS_STATUS_TS_REJECT_DELAYED         = 47
} wlan_dot11_status_code_enum;


/*---------------------------------------------------------------------------
 WLAN_REASON_CODE_ENUM
  This the enumeration of the possible reason codes
---------------------------------------------------------------------------*/
typedef enum
{
    WLAN_SYS_REASON_UNSPECIFIED                  = 1,
    WLAN_SYS_REASON_PREV_AUTH_NOT_VALID          = 2,
    WLAN_SYS_REASON_DEAUTH_STA_LEFT              = 3,
    WLAN_SYS_REASON_DISASSOC_INACTIF             = 4,
    WLAN_SYS_REASON_DISASSOC_AP_CANNOT_SUP_STA   = 5,
    WLAN_SYS_REASON_CLS2_FRM_RX_FROM_UNAUTH_STA  = 6,
    WLAN_SYS_REASON_CLS3_FRM_RX_FROM_UNASSOC_STA = 7,
    WLAN_SYS_REASON_DISASSOC_STA_LEFT            = 8,
    WLAN_SYS_REASON_REQ_FROM_NOT_AUTH_STA        = 9,
    WLAN_SYS_REASON_DISASSOC_UNSPECIFIED_QOS_REL = 32,
    WLAN_SYS_REASON_DISASSOC_INSUFF_BW_AT_QAP    = 33,
    WLAN_SYS_REASON_DISASSOC_BAD_CHANNEL         = 34,
    WLAN_SYS_REASON_DISASSOC_QSTA_OVERSHOOT_TXOP = 35,
    WLAN_SYS_REASON_QSTA_LEAVING_QBSS            = 36,
    WLAN_SYS_REASON_QSTA_DOES_NOT_WANT_MECHANISM = 37,
    WLAN_SYS_REASON_MECHANISM_REQUIRE_QSTA_SETUP = 38,
    WLAN_SYS_REASON_QSTA_TIMEOUT                 = 39,
    WLAN_SYS_REASON_UNSUPP_CIPHER_SUITE_AT_QSTA  = 45
} wlan_dot11_reason_code_enum;


/*---------------------------------------------------------------------------
 WLAN_CHANNEL_LIST:
    This structure describes a channel list
---------------------------------------------------------------------------*/
#define WLAN_SYS_MAX_CHANNEL    14

typedef struct
{

	uint16 nb_channels;              /* Number of channels set in the list */
	uint8  channel_list[WLAN_SYS_MAX_CHANNEL];

} wlan_dot11_channel_list_type;



/*---------------------------------------------------------------------------
 WLAN_CAP_INFO_TYPE:
    This structure describes the capability information of an AP
---------------------------------------------------------------------------*/
typedef PACKED struct
{
  uint16  ess    : 1;  /* Extended Service Set?                            */
  uint16  ibss   : 1;  /* Independent Base Service Set?                    */
  uint16  cf_p   : 1;  /* Support contention free Poll?                    */
  uint16  cf_pr  : 1;  /* Content free Poll Request?                       */
  uint16  priv   : 1;  /* Privacy?                                         */
  uint16  sh_pr  : 1;  /* Short Preamble?                                  */ 
  uint16  pbcc   : 1;  /* Support PBCC modulation?                         */
  uint16  ch_ag  : 1;  /* Support channel agility?                         */
  uint16  qos    : 1;  /* Support QOS?                                     */
  uint16  fec    : 1;  /* Support FEC?                                     */
  uint16  bridge : 1;  /* Is this AP a bridge portal                       */
  uint16  rsn    : 1;  /* Does this STA support RSN                        */
  uint16  rsvr   : 3;  /* Reserved                                         */
  uint16  ex_cap : 1;  /* Is Extended Capability available                 */

} wlan_dot11_cap_info_type;



/*---------------------------------------------------------------------------
 WLAN_SSID_TYPE:
    This structure describes an SSID
---------------------------------------------------------------------------*/
typedef sys_wlan_ssid_s_type wlan_dot11_ssid_type;


/*---------------------------------------------------------------------------
 WLAN_SUPPORTED_RATES:
    This structure describes the rates that a given STA suports.
---------------------------------------------------------------------------*/
#define WLAN_SYS_MAX_NB_SUPPORTED_RATES     8

#define WLAN_SYS_DOT11B_RATE_1_MBPS         2 
#define WLAN_SYS_DOT11B_RATE_2_MBPS         4
#define WLAN_SYS_DOT11B_RATE_5P5_MBPS      11
#define WLAN_SYS_DOT11B_RATE_11_MBPS       22
	

typedef struct
{
	uint32 nb_rates;                     /* Number of rates set in the array */
	char   rates[WLAN_SYS_MAX_NB_SUPPORTED_RATES];

} wlan_dot11_supported_rates_type;


/*---------------------------------------------------------------------------
 WLAN_EXTENDED_SUPPORTED_RATES:
    This structure describes the rates that a given STA suports.
---------------------------------------------------------------------------*/
#define WLAN_SYS_MAX_NB_EXTENDED_SUPPORTED_RATES    32  

#define WLAN_SYS_DOT11G_RATE_6_MBPS        12 
#define WLAN_SYS_DOT11G_RATE_9_MBPS        18 
#define WLAN_SYS_DOT11G_RATE_12_MBPS       24
#define WLAN_SYS_DOT11G_RATE_18_MBPS       36
#define WLAN_SYS_DOT11G_RATE_24_MBPS       48
#define WLAN_SYS_DOT11G_RATE_36_MBPS       72
#define WLAN_SYS_DOT11G_RATE_48_MBPS       96
#define WLAN_SYS_DOT11G_RATE_54_MBPS      108 
	

typedef struct
{
	uint32 nb_rates;                     /* Number of rates set in the array */
	char   rates[WLAN_SYS_MAX_NB_EXTENDED_SUPPORTED_RATES];

} wlan_dot11_extended_supported_rates_type;


/*---------------------------------------------------------------------------
 WLAN_DOT11_PREAMBLE_ENUM
   Enumeration of the various 802.11 PHY preambles
---------------------------------------------------------------------------*/
typedef enum
{
  WLAN_DOT11_PHY_PREAMBLE_SHORT = 0,
  WLAN_DOT11_PHY_PREAMBLE_LONG  = 1,
  WLAN_DOT11_PHY_PREAMBLE_AUTO  = 2
} wlan_dot11_preamble_enum;


/*---------------------------------------------------------------------------
 WLAN_BSS_DESCRIPTION_TYPE:
    This structure describes a BSS
---------------------------------------------------------------------------*/
typedef struct
{
    wlan_ieee_mac_address_type                  bssid; /*For BSS, MAC addr */
    wlan_dot11_ssid_type                        ssid;
    wlan_dot11_bss_type_enum                    bsstype;
    wlan_dot11_supported_rates_type             supported_rates;
    wlan_dot11_extended_supported_rates_type    extended_supported_rates;
    uint8                                       channel_id;
    wlan_dot11_cap_info_type                    capability;
    wpa_ie_type                                 wpa_ie;

    /*
    ** Raw WPA IE since needed by WPA supplicant for 4 way procedure
    */
    uint8                           raw_ie_data[WPA_IE_MAX_LEN ];

    wlan_adp_wmm_info_type          wmm_info;

} wlan_dot11_bss_description_type;



/*---------------------------------------------------------------------------
 WLAN _OPR_PARAM_ID_ENUM
    This is an enumeration of identifiers of various operational parameters 
    that may be retrieved at the OEM layer. This list is not exhaustive and 
    should be update at wish.
---------------------------------------------------------------------------*/
typedef enum
{

	WLAN_RSSI          = 0,
	WAN_LOGS           = 1, /* General Log                                   */ 
	WLAN_TX_FRAME_CNT  = 2, /* Number of MPDU effectively transmitted        */  
	WLAN_RX_FRAME_CNT  = 3, /* Number of MPDU effectively received           */          
	WLAN_TX_RATE,
	WLAN_TOTAL_RX_BYTE

} wlan_dot11_opr_param_id_enum;


/*---------------------------------------------------------------------------
WLAN_OPR_PARAM_VALUE_TYPE
     This is an union of the poosible values of an operational parameter.
     //To rework
---------------------------------------------------------------------------*/
typedef union
{
	uint8                u8_value;
	uint16               u16_value;
	uint32               u32_value;
	sys_wlan_rate_e_type rate;
} wlan_dot11_opr_param_value_type;


#define WLAN_DOT11_WPA_MAX_KEY_LENGTH    32
#define WLAN_DOT11_WPA_KEY_TYPE_P        1        /* Pairwise              */
#define WLAN_DOT11_WPA_KEY_TYPE_G_S      0        /* Group/Stakey          */
/*---------------------------------------------------------------------------
 WLAN_DOT11_WPA_SET_KEY_DESCRIPTOR
   This is a key descriptor per 802.11i specs.
---------------------------------------------------------------------------*/
typedef struct
{
  uint8       key[WLAN_DOT11_WPA_MAX_KEY_LENGTH];
  uint8       key_length;
  uint8       key_id;
  uint8       key_type;
  uint8       mac_address[6];
  uint8       rsc[8]; 
  boolean     initiator;
  uint8       cipher_suite_selector[4];
 
} wlan_dot11_wpa_setkey_descriptor_type;

/*---------------------------------------------------------------------------
 WLAN_DOT11_WPA_DEL_KEY_DESCRIPTOR
  This is the delete key descriptor per 802.11i specs
---------------------------------------------------------------------------*/
typedef struct
{
  uint8      key_id;
  uint8      key_type; 
  uint8      mac_address[6];
} wlan_dot11_wpa_delkey_descriptor_type;

/*---------------------------------------------------------------------------
 WLAN_DOT11_WPA_PROTECT_TYPE_ENUM
  Enumeration of the the differents 802.11i defined protection type as
  used within WPA.
---------------------------------------------------------------------------*/
typedef enum
{
  WLAN_DOT11_WPA_PROTECT_NONE  = 0,
  WLAN_DOT11_WPA_PROTECT_RX    = 1,
  WLAN_DOT11_WPA_PROTECT_TX    = 2,
  WLAN_DOT11_WPA_PROTECT_RX_TX = 3
} wlan_dot11_wpa_protect_type_enum;

/*---------------------------------------------------------------------------
 WLAN_DOT11_WPA_PROTECT_INFO_TYPE
   This structure describe a protection to be set at the MAC
---------------------------------------------------------------------------*/
typedef struct
{
  uint8                             address[6];
  wlan_dot11_wpa_protect_type_enum  protect_type;
  uint8                             key_type;
} wlan_dot11_wpa_protect_info_type;


#endif /* WLAN_DOT11_SYS_H */
