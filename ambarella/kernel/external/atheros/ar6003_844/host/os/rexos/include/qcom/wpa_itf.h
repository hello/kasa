#ifndef WPA_ITF_H
#define WPA_ITF_H

/*===========================================================================
                               WPA_ITF. H

DESCRIPTION

   Header file describing the WPA external API, detailing its interface with
   WLAN Mode Controller, 802.1X module and the WLAN Adapter. 
   The WPA module is envisionned to run with the WLAN Security task context.
   
EXTERNALIZED FUNCTIONS
   wpa_init()         : To initialize the WPA module. Should be called at 
                        WLAN Sec task intitialization.

   wpa_assoc_ind()    : Invoked when association is completed. This will
                        trigger the 802.1X state machine.
                    
   wpa_disassoc_ind() : Invoked when WLAN stack is explicitely disassociated
                        by AP.

   wpa_deauth_req()   : Invoked when upper layer wants to request an explicit 
                        deauthentication through the EAP/802.1X path.

   wpa_disassoc_req() : Invoked when upper layer is about to disassociate 
                        with current AP.

   wpa_preauth_req()  : Invoked by MC to request the WPA module to preauthen-
                        ticate with a specific AP. Not currently supported!

   wpa_reg_ind_cback(): Invoked by upper layer to register with WPA events
                        notification. Not currently supported.
    
  Copyright (c) 2005 by QUALCOMM, Incorporated.  All Rights Reserved.
===========================================================================*/

/*===========================================================================

                            EDIT HISTORY FOR FILE
$Header: //depot/sw/releases/olca3.1-RC/host/os/rexos/include/qcom/wpa_itf.h#3 $ 
$Author: gijo $ $DateTime: 2012/03/29 04:00:18 $
    

when        who    what, where, why
--------    ---    ----------------------------------------------------------
06/09/05    aku    Added WPA_MAX_EVT to wpa_event_enum.
05/10/05    hba    Added WPA Authentication type and net user info
                   to wpa_info_type!
04/11/05    hba    Creation
===========================================================================*/


/*===========================================================================
                                INCLUDE FILES
===========================================================================*/
#ifdef MATS
#include "comdef.h"

#include "net.h"
#endif

/*===========================================================================
                        DATA STRUCTURE DECLARATIONS
===========================================================================*/
#define WPA_IE_MAX_LEN      40                 /* Arbitrarly for now       */

typedef enum
{
  WPA_AUTH_TYPE_NONE  = 0,
  WPA_AUTH_TYPE_TLS   = 1,
  WPA_AUTH_TYPE_MD5   = 2
} wpa_auth_type_enum;

/*---------------------------------------------------------------------------
  WPA_CIPHER_SUITE_ENUM
    This is the enumeration of the different WPA Group Cipher suite. Note 
	that there is a slight difference between the cipher suites as specified
	by WPA and 802.11i 
---------------------------------------------------------------------------*/
typedef enum
{
	WPA_CIPHER_SUITE_NONE    = 0, /* Corresponds to Use Group Cipher suite */
	WPA_CIPHER_SUITE_WEP_40  = 1, /* WEP 40                                */
	WPA_CIPHER_SUITE_TKIP    = 2, /* TKIP                                  */
	WPA_CIPHER_SUITE_CCMP    = 4, /* CCMP - Note this value is different from
	                                the one specified in the 802.11i Spec  */
	WPA_CIPHER_SUITE_WEP_104 = 5

} wpa_cipher_suite_enum;


/*---------------------------------------------------------------------------
  WPA_AKM_SUITE_ENUM
    This is the enumeration of the different Authentication and Key  
	Management suite.
---------------------------------------------------------------------------*/
typedef enum
{
	WPA_AKM_NONE   = -1, /* No Authentication or Key Management            */
	WPA_AKM_802_1X =  1, /* IEEE 802.1X                                    */
	WPA_AKM_PSK    =  2  /* Pre-Shared Key                                 */

} wpa_akm_suite_enum;


/*---------------------------------------------------------------------------
  WPA_INFO_TYPE
    This data structure conveys the selected cipher and AKM suites for
	security association establishment with a paticular AP 
---------------------------------------------------------------------------*/
typedef struct
{
  wpa_auth_type_enum     auth;
	wpa_cipher_suite_enum  multicast;  /* Group Cipher suite                 */
	wpa_cipher_suite_enum  unicast;    /* Unicast Cipher suite               */
 	wpa_akm_suite_enum     akm;        /* Authentication & Key Mgmt suite    */
 
  struct
  {
    uint16     ie_len;
    uint8      ie_data[ WPA_IE_MAX_LEN ];
  } raw_ie;

  net_wlan_8021x_info_u_type net_user_info;

} wpa_info_type;


/*---------------------------------------------------------------------------
  WPA_IE_TYPE:
    This data structure describes the WPA IE element as in the WPA Specs.
---------------------------------------------------------------------------*/ 
#define WPA_MAX_PAIRWISE_CIPHER_SUITE   5
#define WPA_MAX_AKM_SUITE               2

typedef struct
{ 
	uint8        oui[4];

  /*
  ** if ie_len is 0 then WPA IE is not part of BSS Descriptor
  */ 
  uint16       ie_len;
	
  uint16       version;
	uint8        grp_cipher_suite;
	uint16       pairwise_cipher_count;
	struct
	{
		uint8    oui[4];
	} pairwise_cipher_suites[WPA_MAX_PAIRWISE_CIPHER_SUITE];

	uint16       akm_count;

	struct
	{
		uint8 oui[4];
	} akm_suites[WPA_MAX_AKM_SUITE];

	uint16       rsn_cap;
   
  /*
	** Not supported  yet!
	** Include here PMKID Count and List
	*/
} wpa_ie_type;


/*---------------------------------------------------------------------------
  WPA_EVENT_ENUM
    This enumerates the various events that may be notified by the WPA
	module. 
---------------------------------------------------------------------------*/
typedef enum
{
    WPA_AKM_SUCCESS_EVT      = 0/* Indicate that Authentication and Key
	                                Derivation were performed successfully   */ 
   ,WPA_AKM_FAILURE_EVT      = 1/* Indicate that Authenticatiion or Key
	                                Derivation failed                        */
   ,WPA_ASSOC_BLOCKED_EVT    = 2/* Request upper layer to block subsequently
	                                any request for WLAN acquisition till
					                        WPA_ASSOC_UNBLOCKED_EVT is received      */
   ,WPA_ASSOC_UNBLOCKED_EVT  = 3/* Indicate that request for WLAN acquisition
	                                may resume                               */
   ,WPA_DEAUTH_REQ_EVT       = 4/* Indicate a request from WPA that upper
	                                layer should explicitly invoke an 
									                MLME DEAUTHENTICATION REQUEST to 
									                deauthenticate with AP                   */
   ,WPA_DISASSOC_REQ_EVT     = 5/* Indicate a request from WPA that upper
	                                layer should explicitly invoke an 
									                MLME DISASSOCIATION REQUEST to 
									                disasociate with AP                     */

   ,WPA_MC_DATA_BLOCKED_EVT  = 6/* Notify Mode Controller that the data port
                                   should be blocked... PS should change the
                                   PS_IAFCE state to prevent any data flow */
   ,WPA_MC_DATA_UNBLOCKED_EVT= 7/* Notify Mode Controller that the data port
                                   should be open and that MC may re-enable
                                   data flow on the PS_IFACE               */
   ,WPA_TKIP_BLOCKED_EVT    =  8/* Request upper layer to disable TKIP assoc
                                     for the countermeasures               */


  /*------------------------------------------------------------------------
                   Pre-authentication not yet supported                  
   ,WPA_PREAUTH_SUCCESS_EVT  = 8
   ,WPA_PREAUTH_FAILURE_EVT  = 9
   ------------------------------------------------------------------------*/ 
  ,WPA_MAX_EVT
} wpa_event_enum;

/*---------------------------------------------------------------------------
  WPA_IND_CBACK_TYPE
    This is the prototype of the callback that should be registered with the 
	WPA module to get the previous events notification.
	Note: Third parameter carries info about event. It is NULL in most cases
	except for pre-authentication when it will contain the PMKID 
---------------------------------------------------------------------------*/
typedef void (wpa_ind_cback_type)(void*          user_data,
								                  wpa_event_enum evt,
								                  void*          evt_data);

/*===========================================================================
                        PUBLIC FUNCTIONS
===========================================================================*/

/*===========================================================================
FUNCTION WPA_INIT()

DESCRIPTION
  Invoked at WLAN Sec task to initalize the WPA Module.  

PARAMETERS
  None.

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void wpa_init
(
  void
);


/*===========================================================================
FUNCTION wpa_assoc_ind()

DESCRIPTION
  Invoke to indicate to WPA that association is done and that 802.1X state 
  machine should be triggered. 

PARAMETERS
  ptr_wpa_info: Pointer to WPA INFO indicating the selected cipher & AKM
                suites.
  ptr_bssid   : Pointer to BSSID MAC address.

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void wpa_assoc_ind 
(
  wpa_info_type* ptr_wpa_info,
  byte*          ptr_bssid
);


/*===========================================================================
FUNCTION wpa_disassoc_ind()

DESCRIPTION
  Invoke to indicate to WPA that an AP oiginated disacossiation.

PARAMETERS
  None

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void wpa_disassoc_ind 
(
  void
);


/*===========================================================================
FUNCTION wpa_deauth_req()

DESCRIPTION
  Invoked when upper layer wants to request an explicit deauthentication 
  through the EAP/802.1X path.

PARAMETERS
  None

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void wpa_deauth_req 
(
  void
);


/*===========================================================================
FUNCTION wpa_disassoc_req()

DESCRIPTION
   Invoked when upper layer is about to disassociate with current AP.

PARAMETERS
  None

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void wpa_disassoc_req 
(
  void
);


/*===========================================================================
FUNCTION wpa_reg_ind_cback()

DESCRIPTION
   Invoked by upper layer to register with WPA events notification.

PARAMETERS
  None

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void wpa_reg_ind_cback 
(
  wpa_ind_cback_type*  ptr_ind_cbak,
  void*                user_data
);


#endif // WPA_ITF.H
