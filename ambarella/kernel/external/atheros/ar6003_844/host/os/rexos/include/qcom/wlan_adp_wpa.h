#ifndef WLAN_ADP_WPA_H
#define WLAN_ADP_WPA_H
/*===========================================================================
                               WLAN_ADP_WPA . H

DESCRIPTION
  Header file for the WLAN ADAPTER interface with  WPA Module. 

  Copyright (c) 2004 by QUALCOMM, Incorporated.  All Rights Reserved.
===========================================================================*/

/*===========================================================================

                            EDIT HISTORY FOR FILE

           $Author: gijo $ $DateTime: 2012/03/29 04:00:18 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
04/13/05    hba    Creation
===========================================================================*/


/*===========================================================================

                                INCLUDE FILES

===========================================================================*/

#ifdef MATS
#include "comdef.h"
#include "dsm.h"
#endif
#include "wlan_adp_def.h"


/*===========================================================================

                              DATA DECLARATIONS

===========================================================================*/



/*===========================================================================

                       WPA MAC REQUESTS/RESPONSES PRIMITIVES

===========================================================================*/

/*---------------------------------------------------------------------------
 WLAN_ADP_WPA_SET_KEY_REQ_INFO:
    This structure contains information to direct WLAN Adapter to install
    keys in the MAC.
---------------------------------------------------------------------------*/

#define WLAN_ADP_WPA_MAX_KEYS        4

typedef struct wlan_adp_wpa_set_keys_req_info_s
{
   uint32                                num_keys;
   wlan_dot11_wpa_setkey_descriptor_type keys[WLAN_ADP_WPA_MAX_KEYS];

} wlan_adp_wpa_setkeys_req_info_type;

/*---------------------------------------------------------------------------
 WLAN_ADP_WPA_DEL_KEY_REQ_INFO:
    This structure contains information to direct WLAN Adapter to destroy
    keys from the MAC.
---------------------------------------------------------------------------*/
typedef struct wlan_adp_wpa_delkeys_req_info_s
{
   uint32                                num_keys;
   wlan_dot11_wpa_delkey_descriptor_type keys[WLAN_ADP_WPA_MAX_KEYS];

} wlan_adp_wpa_delkeys_req_info_type;


/*---------------------------------------------------------------------------
 WLAN_ADP_WPA_PROTECT_LIST
   This describe a list of protection info to apply at the MAC
---------------------------------------------------------------------------*/
#define WLAN_ADP_WPA_MAX_PROTECT_INFO   4

typedef struct
{
	wlan_dot11_wpa_protect_info_type 
		protect_list[WLAN_ADP_WPA_MAX_PROTECT_INFO];

} wlan_adp_wpa_protect_list_type;

/*---------------------------------------------------------------------------
 WLAN_ADP_WPA_MIC_FAILURE_IND_INFO:
   This structure conatins information about a MICHAEL MIC Failure
---------------------------------------------------------------------------*/
typedef struct wlan_adp_wpa_mic_failure_info_s
{
	uint8     count;
	uint8     mac_address[6];
	uint8     key_type;
	uint8     key_id;
	uint8     tsc[6];
} wlan_adp_wpa_mic_failure_info_type;

/*===========================================================================

                      PUBLIC FUNCTION DECLARATIONS

===========================================================================*/


/*===========================================================================

                      MAC REQUEST/RESPONSES API

===========================================================================*/


/*===========================================================================
FUNCTION WLAN_ADP_WPA_SETKEYS_REQ()

DESCRIPTION
  This function requests WLAN Adapter to installed the keys passed as
  parameters.

PARAMETERS
  rsp_cback: Callback registered for response notification.

RETURN VALUE
  0 if the request has been taken into account
 -1 in case of error.

DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/

/*---------------------------------------------------------------------------
  Definition of the setkeys response callback prototype.
---------------------------------------------------------------------------*/
typedef void (*wlan_adp_wpa_setkeys_rsp_cback_f_ptr)( void );

int wlan_adp_wpa_setkeys_req
(
  wlan_adp_wpa_setkeys_req_info_type* ptr_req_info,   
  wlan_adp_wpa_setkeys_rsp_cback_f_ptr rsp_cback
);


/*===========================================================================
FUNCTION WLAN_ADP_WPA_DELKEYS_REQ()

DESCRIPTION
  This function requests WLAN Adapter to delete keys previously installed in
  the MAC.

PARAMETERS
  rsp_cback: Callback registered for response notification.

RETURN VALUE
  0 if the request has been taken into account
 -1 in case of error.

DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/

/*---------------------------------------------------------------------------
  Definition of the setkeys response callback prototype.
---------------------------------------------------------------------------*/
typedef void (*wlan_adp_wpa_delkeys_rsp_cback_f_ptr)( void );

int wlan_adp_wpa_delkeys_req
(
  wlan_adp_wpa_delkeys_req_info_type* ptr_req_info,   
  wlan_adp_wpa_delkeys_rsp_cback_f_ptr rsp_cback
);



/*===========================================================================
FUNCTION WLAN_ADP_WPA_SET_PROTECTION_REQ()

DESCRIPTION
  This function requests WLAN Adapter to set the appropriate protection at
  the OEM layer.

PARAMETERS
  rsp_cback: Callback registered for response notification.

RETURN VALUE
  0 if the request has been taken into account
 -1 in case of error.

DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/

/*---------------------------------------------------------------------------
  Definition of the setkeys response callback prototype.
---------------------------------------------------------------------------*/
typedef void (*wlan_adp_wpa_set_protection_rsp_cback_f_ptr)( void );

int wlan_adp_wpa_set_protection_req
(
  wlan_adp_wpa_protect_list_type*             ptr_req_info,   
  wlan_adp_wpa_set_protection_rsp_cback_f_ptr rsp_cback
);



/*===========================================================================

                    WPA  MAC EVENTS HANDLER REGISTRATION API

===========================================================================*/

/*---------------------------------------------------------------------------
 Definition of MIC Failure Indication  handler protototype.
-----------------------------------------------------------------------------*/
typedef void (*wlan_adp_wpa_mic_failure_ind)
                (wlan_adp_wpa_mic_failure_info_type* ptr_info);

/*===========================================================================
FUNCTION WLAN_ADP_WPA_MIC_FAILURE_HDLR)

DESCRIPTION
  This function register an event handler invokes by WLAN Adpater when it.
  receives a Michael MIC Failure.
  
PARAMETERS
  evt_hdlr: Pointer to callback invoked by WLAN Adapter...
  
RETURN VALUE
  None.

DEPENDENCIES
  wlan_adp_start should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_wpa_mic_failure_hdlr
(
  wlan_adp_wpa_mic_failure_ind evt_hdlr
);



/*---------------------------------------------------------------------------
 Definition of Protection frame dropped handler protototype.
-----------------------------------------------------------------------------*/
typedef void (*wlan_adp_wpa_protect_framedropped_ind)
                (byte * mac_addr1, byte* mac_addr2);

/*===========================================================================
FUNCTION WLAN_ADP_WPA_PROTECT_FRAMEDROPPED_IND

DESCRIPTION
  This function register an event handler invokes by WLAN Adpater when it 
  receives a notification that a protected frame was dropped.
  
PARAMETERS
  evt_hdlr: Pointer to callback invoked by WLAN Adapter...
  
RETURN VALUE
  None.

DEPENDENCIES
  wlan_adp_start should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_wpa_protect_framedropped_hdlr
(
  wlan_adp_wpa_protect_framedropped_ind evt_hdlr
);

#endif // WLAN_ADP_WPA.H
