#ifndef WLAN_ADP_OEM_H
#define WLAN_ADP_OEM_H
/*===========================================================================
                               WLAN_ADP_OEM . H

DESCRIPTION
  Header file for the WLAN ADAPTER interface with  MAC OEM Module. 

  Copyright (c) 2004 by QUALCOMM, Incorporated.  All Rights Reserved.
===========================================================================*/

/*===========================================================================

                            EDIT HISTORY FOR FILE

$Author: gijo $ 
$DateTime: 2012/03/29 04:00:18 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
05/05/05    hba    Merged WPA relatives definitions
02/09/05    hba    Unified API to get Tx data as raw bytes or DSM item
01/26/05    hba    Added MIB GET/SET Primitives and an extra API to
                   retrieve Tx data as DSM item for the PHG OEM.
09/27/04    hba    Creation
===========================================================================*/


/*===========================================================================

                                INCLUDE FILES

===========================================================================*/

#ifdef MATS
#include "comdef.h"
#endif
#include "wlan_adp_def.h"
#include "wlan_mib.h"
#include "dsm.h"
#include "wlan_adp_wpa.h"

/*===========================================================================

                              DATA DECLARATIONS

===========================================================================*/

/*---------------------------------------------------------------------------
 MAXIMUM LLC PDU SIZE
---------------------------------------------------------------------------*/
#define WLAN_ADP_MAX_LLC_PDU_SIZE             2304
#define WLAN_ADP_MIN_LLC_PDU_SIZE             8

/*===========================================================================

                    LOCAL DEFINITIONS AND DECLARATIONS FOR MODULE

===========================================================================*/
 
/*---------------------------------------------------------------------------
  Type definition for WLAN_OEM_START_REQ.
  This function is invoked by WLAN Adapter to request start of 802.11 MAC 
  MAC operations at the OEM layer.
  Return 0 if request is successfully posted to WLAN Transport task,
        -1 otherwise.
---------------------------------------------------------------------------*/
typedef int (*wlan_oem_start_req_f_ptr_type) ( void );


/*---------------------------------------------------------------------------
  Type definition for WLAN_OEM_STOP_REQ.
  This function is invoked by WLAN Adapter to stop 802.11 MAC operations at
  the OEM layer.
  Return 0 if request is successfully posted to WLAN Transport task,
        -1 otherwise.
---------------------------------------------------------------------------*/
typedef int (*wlan_oem_stop_req_f_ptr_type) ( void );


/*---------------------------------------------------------------------------
  Type definition for WLAN_OEM_SUSPEND_REQ.
  This function is invoked by WLAN Adapter to suspend 802.11 MAC operations
  at the OEM layer. The OEM Module should try to put the 802.11 ASIC in the
  lowest power consumption mode.
  Return 0 if request is successfully posted to WLAN Transport task,
        -1 otherwise.
---------------------------------------------------------------------------*/
typedef int (*wlan_oem_suspend_req_f_ptr_type) ( void );


/*---------------------------------------------------------------------------
  Type definition for WLAN_OEM_RESUME_REQ.
  This function is invoked by WLAN Adapter to resume 802.11 MAC operations
  at the OEM layer. The OEM module should switch back to the state in which
  it were when it receives the request for suspension.
  Return 0 if request is successfully posted to WLAN Transport task,
        -1 otherwise.
---------------------------------------------------------------------------*/
typedef int (*wlan_oem_resume_req_f_ptr_type) ( void );




/*---------------------------------------------------------------------------
  Type definition for WLAN_OEM_OPR_GET_REQ.
  This function is invoked by WLAN Adapter to retrieve operational parameter 
  at the OEM layer such as RSSI, statistics...
---------------------------------------------------------------------------*/
typedef int (*wlan_oem_opr_get_f_ptr_type) ( wlan_dot11_opr_param_id_enum);


/*---------------------------------------------------------------------------
  Type definition for WLAN_OEM_TX_REQ.
  This function is invoked by WLAN Adapter to request transmission of a 
  pending LLC PDU in its transit queue. Upon reception of this request, the 
  OEM Module should use the WLAN_OEM_GET_NEXT_TX_DATA funtion to retrieve the 
  LLC PDU to transmit.
  Return 0 if request is successfully posted to WLAN Transport task,
        -1 otherwise.
---------------------------------------------------------------------------*/
typedef int (*wlan_oem_tx_req_f_ptr_type)( void );



/*---------------------------------------------------------------------------
  Type definition for WLAN_OEM_CONFIG_REQ.
  This function is invoked by WLAN Adapter to configure the 802.11 MAC with
  specific parameters.
  
  The buffer holding the request information belongs to the WLAN Adapation 
  layer and should be copied by the OEM Module if necessary.
  Return 0 if request is successfully posted to WLAN Transport task,
        -1 otherwise.
---------------------------------------------------------------------------*/
typedef int (*wlan_oem_mac_config_req_f_ptr_type)
                (wlan_adp_mac_config_req_info_type *);

/*---------------------------------------------------------------------------
  Type definition for WLAN_OEM_SCAN_REQ.
  This function is invoked by WLAN Adapter to request a SCAN @ the OEM Layer
  The OEM should use the WLAN_ADP_OEM_SCAN_RSP function to deliver the 
  response to the Adaptation layer.
  The buffer holding the request information belongs to the WLAN Adapation 
  layer and should be copied by the OEM Module if necessary.
  Return 0 if request is successfully posted to WLAN Transport task,
        -1 otherwise.
---------------------------------------------------------------------------*/
typedef int (*wlan_oem_scan_req_f_ptr_type)(wlan_adp_scan_req_info_type *);


/*---------------------------------------------------------------------------
  Type definition for WLAN_OEM_JOIN_REQ.
  This function is invoked by WLAN Adapter to request a JOIN @ the OEM Layer.
  The OEM should use the WLAN_ADP_OEM_JOIN_RSP function to deliver the 
  response to the Adaptation layer.
  The buffer holding the request information belongs to the WLAN Adapation 
  layer and should be copied by the OEM Module if necessary.
  Return 0 if request is successfully posted to WLAN Transport task,
        -1 otherwise.
---------------------------------------------------------------------------*/
typedef int (*wlan_oem_join_req_f_ptr_type)(wlan_adp_join_req_info_type *);



/*---------------------------------------------------------------------------
  Type definition for WLAN_OEM_AUTH_REQ.
  This function is invoked by WLAN Adapter to request an AUTHENTICATION at 
  the OEM layer. The OEM should use the WLAN_ADP_OEM_AUTH_RSP function to 
  deliver the response to the Adaptation layer.
  The buffer holding the request information belongs to the WLAN Adapation 
  layer and should be copied by the OEM Module if necessary.
  Return 0 if request is successfully posted to WLAN Transport task,
        -1 otherwise.
---------------------------------------------------------------------------*/
typedef int (*wlan_oem_auth_req_f_ptr_type)(wlan_adp_auth_req_info_type *);



/*---------------------------------------------------------------------------
  Type definition for WLAN_OEM_ASSOC_REQ.
  This function is invoked by WLAN Adapter to request an ASSOCIATION at the 
  OEM layer. The OEM should use the WLAN_ADP_OEM_ASSOC_RSP function to 
  deliver the response to the Adaptation layer.
  The buffer holding the request information belongs to the WLAN Adapation 
  layer and should be copied by the OEM Module if necessary.
  Return 0 if request is successfully posted to WLAN Transport task,
        -1 otherwise.
---------------------------------------------------------------------------*/
typedef int (*wlan_oem_assoc_req_f_ptr_type)(wlan_adp_assoc_req_info_type *);




/*---------------------------------------------------------------------------
  Type definition for WLAN_OEM_REASSOC_REQ.
  This function is invoked by WLAN Adapter to request a REASSOCIATION at the 
  OEM layer. The OEM should use the WLAN_ADP_OEM_REASSOC_RSP function to 
  deliver the response to the Adaptation layer.
  The buffer holding the request information belongs to the WLAN Adapation 
  layer and should be copied by the OEM Module if necessary.
  Return 0 if request is successfully posted to WLAN Transport task,
        -1 otherwise.
---------------------------------------------------------------------------*/
typedef int (*wlan_oem_reassoc_req_f_ptr_type) 
              (wlan_adp_reassoc_req_info_type*);




/*---------------------------------------------------------------------------
  Type definition for WLAN_OEM_DISASSOC_REQ.
  This function is invoked by WLAN Adapter to request a DISASSOCIATION at the
  OEM layer. The OEM should use the WLAN_ADP_OEM_DISASSOC_RSP function to 
  deliver the response to the Adaptation layer.
  The buffer holding the request information belongs to the WLAN Adapation 
  layer and should be copied by the OEM Module if necessary.
  Return 0 if request is successfully posted to WLAN Transport task,
        -1 otherwise.
-----------------------------------------------------------------------------*/
typedef int (*wlan_oem_disassoc_req_f_ptr_type) 
              (wlan_adp_disassoc_req_info_type*);




/*---------------------------------------------------------------------------
  Type definition for WLAN_OEM_DEAUTH_REQ.
  This function is invoked by WLAN Adapter to request a DEAUTHENTICATION at 
  the OEM layer. The OEM should use the WLAN_ADP_OEM_DEAUTH_RSP function to 
  deliver the response to the Adaptation layer.
  The buffer holding the request information belongs to the WLAN Adapation 
  layer and should be copied by the OEM Module if necessary.
  Return 0 if request is successfully posted to WLAN Transport task,
        -1 otherwise.
---------------------------------------------------------------------------*/
typedef int (*wlan_oem_deauth_req_f_ptr_type) 
              (wlan_adp_deauth_req_info_type*);




/*---------------------------------------------------------------------------
  Type definition for WLAN_OEM_PWR_MGMT_REQ.
  This function is invoked by WLAN Adapter to request a Power Management mode 
  at the OEM layer. The OEM should use the WLAN_ADP_OEM_PWR_MGMT_RSP
  function to deliver the response to the Adaptation layer.
  The buffer holding the request information belongs to the WLAN Adapation 
  layer and should be copied by the OEM Module if necessary.
  Return 0 if request is successfully posted to WLAN Transport task,
        -1 otherwise.
---------------------------------------------------------------------------*/
typedef int (*wlan_oem_pwr_mgmt_req_f_ptr_type) 
              (wlan_adp_pwr_mgmt_req_info_type *);




/*---------------------------------------------------------------------------
  Type definition for WLAN_OEM_RESET_REQ.
  This function is invoked by WLAN Adapter to request a software RESET of 
  the WLAN ASIC at the OEM layer. The OEM should use the 
  WLAN_ADP_OEM_RESET_RSP function to deliver the response to the Adaptation 
  layer. The buffer holding the request information belongs to the WLAN 
  Adapation layer and should be copied by the OEM Module if necessary.
  Return 0 if request is successfully posted to WLAN Transport task,
        -1 otherwise.
---------------------------------------------------------------------------*/
typedef int (*wlan_oem_reset_req_f_ptr_type)(wlan_adp_reset_req_info_type*);



/*---------------------------------------------------------------------------
  Type definition for WLAN_OEM_MIB_GET_REQ.
  This function is invoked by WLAN Adapter to retrieve an MIB parameter at 
  the OEM layer. The OEM should use the WLAN_ADP_OEM_MIB_GET_RSP function 
  to deliver the response to the Adaptation layer. 
  Return 0 if request is successfully posted to WLAN Transport task,
        -1 otherwise.
---------------------------------------------------------------------------*/
typedef int (*wlan_oem_mib_get_req_f_ptr_type) 
                (wlan_mib_item_id_enum_type id);



/*---------------------------------------------------------------------------
  Type definition for WLAN_OEM_MIB_SET_REQ.
  This function is invoked by WLAN Adapter to set an MIB parameter at the OEM 
  layer. The OEM module should use the WLAN_ADP_OEM_MIB_SET_RSP function 
  to confirm that the target MIB parameter has effectivly been set.
	The buffer holding the MIB parameter value to set belongs to WLAN Adapter 
  and should be copied by the OEM Module if necessary.
	Return 0 if request is successfully posted to WLAN Transport task, 
	      -1 otherwise.
---------------------------------------------------------------------------*/
typedef int (*wlan_oem_mib_set_req_f_ptr_type)
               (wlan_mib_item_value_type* val);


/*---------------------------------------------------------------------------
  Type defintion for WLAN_OEM_WPA_SETKEYS_REQ.
  This function is invoked by WLAN Adapter to installed the keys derived by
  the WPA Module at the OEM layer. The OEM layer should use the 
  WLAN_ADP_OEM_WPA_SETKEYS_RSP function to confirm that the keys have been
  appropriately installed.
  The buffer holding the key descriptors belongs to WLAN Adapter and should 
  be copied by he OEM Module if necessary.
    Retur 0 if request successfully posted to WLAN Transport task;
	      -1 otherwise
---------------------------------------------------------------------------*/  
typedef int (*wlan_oem_wpa_setkeys_req_type)
               (wlan_adp_wpa_setkeys_req_info_type*);



/*---------------------------------------------------------------------------
  Type definition for WLAN_OEM_WPA_DELKEYS_REQ.
  This function is invoked by WLAN Adapter to destroyed the keys installed
  at the OEM layer. The OEM layer should use the WLAN_ADP_OEM_WPA_DELKEYS_RSP
  function to confirm that the keys have been appropriately destroyed.
  The buffer holding the delkey descriptors belongs to WLAN Adapter and  
  should be copied by he OEM Module if necessary.
    Retur 0 if request successfully posted to WLAN Transport task;
	      -1 otherwise
---------------------------------------------------------------------------*/  
typedef int (*wlan_oem_wpa_delkeys_req_type)
               (wlan_adp_wpa_delkeys_req_info_type*);


/*---------------------------------------------------------------------------
  Type definition for WLAN_OEM_WPA_SET_PROTECTION_REQ.
  This function is invoked by WLAN Adapter to trigger data ciphering and 
  deciphering acoording to precise rules and using the WPA keys preiously
  intalled at the OEM layer. The OEM layer should use the 
  WLAN_ADP_OEM_WPA_SET_PROTECTION_RSP function to confirm that the protection
  requested has been taken into account.
  The buffer holding the protection descriptors belongs to WLAN Adapter and  
  should be copied by he OEM Module if necessary.
    Retur 0 if request successfully posted to WLAN Transport task;
	      -1 otherwise
---------------------------------------------------------------------------*/  
typedef int (*wlan_oem_wpa_set_protection_req_type)
               (wlan_adp_wpa_protect_list_type*);

/*---------------------------------------------------------------------------
  Type definition for WLAN_OEM_ADDTS_REQ.
  This function is invoked by WLAN Adapter to request setup of a TSPEC at the 
  OEM layer. The OEM should use the WLAN_ADP_OEM_ADDTS_RSP function to 
  deliver the response to the Adaptation layer.
  The buffer holding the request information belongs to the WLAN Adapation 
  layer and should be copied by the OEM Module if necessary.
  Return 0 if request is successfully posted to WLAN Transport task,
        -1 otherwise.
---------------------------------------------------------------------------*/
typedef int (*wlan_oem_addts_req_f_ptr_type)(wlan_adp_addts_req_info_type *);


/*---------------------------------------------------------------------------
  Type definition for WLAN_OEM_DELTS_REQ.
  This function is invoked by WLAN Adapter to request teardown of a TSPEC at
  the OEM layer. The OEM should use the WLAN_ADP_OEM_DELTS_RSP function to 
  deliver the response to the Adaptation layer.
  The buffer holding the request information belongs to the WLAN Adapation 
  layer and should be copied by the OEM Module if necessary.
  Return 0 if request is successfully posted to WLAN Transport task,
        -1 otherwise.
---------------------------------------------------------------------------*/
typedef int (*wlan_oem_delts_req_f_ptr_type)(wlan_adp_delts_req_info_type *);



/*---------------------------------------------------------------------------
 WLAN MAC Interface.
 This data structure describes the OEM MAC interface in terms of the requests 
 it should implement.
 The WLAN transport task should register an instance of this interface with 
 WLAN Adapter at startup.
---------------------------------------------------------------------------*/
typedef struct wlan_oem_mac_iface_s
{

  wlan_oem_start_req_f_ptr_type      start_f_ptr;
  wlan_oem_stop_req_f_ptr_type       stop_f_ptr;
  wlan_oem_suspend_req_f_ptr_type    suspend_f_ptr;
  wlan_oem_resume_req_f_ptr_type     resume_f_ptr;
  wlan_oem_tx_req_f_ptr_type         tx_f_ptr;
  wlan_oem_scan_req_f_ptr_type       scan_f_ptr;
  wlan_oem_join_req_f_ptr_type       join_f_ptr;
  wlan_oem_auth_req_f_ptr_type       auth_f_ptr;
  wlan_oem_assoc_req_f_ptr_type      assoc_f_ptr;
  wlan_oem_reassoc_req_f_ptr_type    reassoc_f_ptr;
  wlan_oem_disassoc_req_f_ptr_type   disassoc_f_ptr;
  wlan_oem_deauth_req_f_ptr_type     deauth_f_ptr;
  wlan_oem_pwr_mgmt_req_f_ptr_type   pwr_mgmt_f_ptr;
  wlan_oem_reset_req_f_ptr_type      reset_f_ptr;
  wlan_oem_opr_get_f_ptr_type        opr_get_f_ptr;
  wlan_oem_mib_get_req_f_ptr_type    mib_get_f_ptr;
  wlan_oem_mib_set_req_f_ptr_type    mib_set_f_ptr;
  wlan_oem_mac_config_req_f_ptr_type cfg_f_ptr;
  wlan_oem_wpa_setkeys_req_type         setkeys_f_ptr;
  wlan_oem_wpa_delkeys_req_type         delkeys_f_ptr;
  wlan_oem_wpa_set_protection_req_type  protect_f_ptr;
  wlan_oem_addts_req_f_ptr_type      addts_f_ptr;
  wlan_oem_delts_req_f_ptr_type      delts_f_ptr;

} wlan_oem_mac_iface_type;




/*===========================================================================

                      PUBLIC FUNCTION DECLARATIONS

===========================================================================*/



/*===========================================================================
FUNCTION WLAN_ADP_REGISTER_OEM_MAC_IF()

DESCRIPTION
  This function is invoked by the OEM Module to register its MAC interface 
  with WLAN Adapter.

PARAMETERS
  ptr_mac_iface: Pointer to OEM WLAM MAC Interface.
  
RETURN VALUE
  0 if registration happend successfully
 -1 in case of error.

DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
int wlan_adp_register_oem_mac_if
(
  wlan_oem_mac_iface_type* ptr_mac_iface
);



/*===========================================================================
FUNCTION WLAN_ADP_OEM_READY_IND()

DESCRIPTION
  This function is invoked by the OEM Module to indicate to the WLAN Adapter
  that it is ready to process subsequent MAC commands. It is typically called
  by the OEM MAc implementation in response to the WLAN_OEM_START_REQ request.

PARAMETERS
  perror: -1 if the OEM MAC implementation fails for any reason.
           0 otherwise
  
RETURN VALUE
  None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_oem_ready_ind
(
  int perror
);


/*===========================================================================
FUNCTION WLAN_ADP_OEM_MAC_CFG_RSP()

DESCRIPTION
  This function is invoked by the OEM Module to indicate to the WLAN Adapter
  that it is has properly configure subsequently to a request to set config
  parameters from the upper layer.

PARAMETERS
  perror: -1 if the OEM MAC implementation fails for any reason.
           0 otherwise
  
RETURN VALUE
  None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_oem_mac_config_rsp
(
  int perror
);


/*===========================================================================
FUNCTION WLAN_ADP_OEM_SUSPEND_RSP()

DESCRIPTION
  This function is invoked by the OEM Module to indicate to the WLAN Adapter
  that it has effectively suspended 802.11 MAC operations.

PARAMETERS
  perror: -1 if the OEM MAC implementation fails to suspend its operation for
  any reason.
           0 otherwise
  
RETURN VALUE
  None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_oem_suspend_rsp
(
  int perror
);


/*===========================================================================
FUNCTION WLAN_ADP_OEM_RESUME_RSP()

DESCRIPTION
  This function is invoked by the OEM Module to indicate to the WLAN Adapter
  that it has effectively resume 802.11 MAC operations.

PARAMETERS
  perror: -1 if the OEM MAC implementation fails to resume for any reason.
           0 otherwise
  
RETURN VALUE
  None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_oem_resume_rsp
(
  int perror
);



/*===========================================================================
FUNCTION WLAN_ADP_RX_DATA()

DESCRIPTION
  This function should be invoked by the OEM MAC implementation to indicate 
  to the WLAN Adaptation layer that an LLC PDU has been received over the WM. 
  The owner of the buffer containing the received data is the OEM Module and 
  it is its responsibility to free it if it needs to.

PARAMETERS
  ptr_rx_pkt: pointer to buffer containing LLC PDU.
  ptr_meta_info: pointer to buffer containing meta information about the rx
                 data.
  
RETURN VALUE
  None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
int wlan_adp_oem_rx_data
(
  byte*                           ptr_rx_buffer,
  uint16                          buffer_len,
  wlan_adp_rx_pkt_meta_info_type* ptr_meta_info
);



/*===========================================================================
FUNCTION WLAN_ADP_OEM_GET_NEXT_TX_DATA()

DESCRIPTION
  This function should be invoked by the OEM MAC implementation to retrieve 
  the next LLC PDU pending for transmission at the Adapter Layer.

PARAMETERS
  ptr_tx_buffer: pointer to memeory where to copy the LLC PDU to transmit. 
  This memory should habe been allocated by the OEM Module.
  
  ptr_meta_info: pointer to buffer containing metat information about the PDU 
  to be transmitted.
  
RETURN VALUE
  Length of LLC PDU copied to supplied buffer.
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
int wlan_adp_oem_get_next_tx_data
(
  byte*                           ptr_tx_buffer,
  wlan_adp_tx_pkt_meta_info_type* ptr_meta_info  
);


/*===========================================================================
FUNCTION WLAN_ADP_OEM_GET_NEXT_TX_PKT()

DESCRIPTION
  This function should be invoked by the OEM MAC implementation to retrieve 
  the next LLC PDU pending for transmission at the Adapter Layer encapsulated
  as an Ethernet packet.

PARAMETERS
  tx_pkt: Address of pointer that should hold the address of the dsm item 
          containing the LLC PSU to xmit.
          
  ptr_meta_info: pointer to buffer containing meta information about the PDU 
          to be transmitted.
  
RETURN VALUE
  Length of packet to xmit...
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
int wlan_adp_oem_get_next_tx_pkt
(
   dsm_item_type **                tx_pkt,
   wlan_adp_tx_pkt_meta_info_type* ptr_meta_info
);



/*===========================================================================
FUNCTION WLAN_ADP_OEM_OPR_GET_RSP()

DESCRIPTION
  This function should be invoked by the OEM MAC implementation to deliver 
  the. value of an operational parameter per WLAN Adapter request.

PARAMETERS
  param_id: param id
  param_value: value of parameter.
  
RETURN VALUE
  None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
int wlan_adp_oem_opr_param_get_rsp
(
  wlan_dot11_opr_param_id_enum    param_id,
  wlan_dot11_opr_param_value_type param_value  
);



/*===========================================================================
FUNCTION WLAN_ADP_OEM_SCAN_RSP

DESCRIPTION
  This function is invoked by the OEM MAC implementation to deliver response 
  of a previous SCAN request.

PARAMETERS
  scan_rsp_info: pointer to data structure containing info  about the SCAN 
                 response.This buffer belongs to the caller and will not be 
                 freed by WLAN Adapter.
  
RETURN VALUE
   None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_oem_scan_rsp
(
  wlan_adp_scan_rsp_info_type* scan_rsp_info
);



/*===========================================================================
FUNCTION WLAN_ADP_OEM_AUTH_RSP

DESCRIPTION
  This function is invoked by the OEM MAC implementation to deliver response 
  of a previous Authentication request.

PARAMETERS
  result_code: contains the response result code.
  
RETURN VALUE
   None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_oem_auth_rsp
(
  wlan_dot11_result_code_enum result_code
);



/*===========================================================================
FUNCTION WLAN_ADP_OEM_ASSOC_RSP

DESCRIPTION
  This function is invoked by the OEM MAC implementation to deliver response 
  of a previous Association request.

PARAMETERS
  assoc_rsp_info: pointer to data structure containing info  about the assoc 
                 response.This buffer belongs to the caller and will not be 
                 freed by WLAN Adapter.
                 
RETURN VALUE
   None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_oem_assoc_rsp
(
  wlan_adp_assoc_rsp_info_type* assoc_rsp_info
);



/*===========================================================================
FUNCTION WLAN_ADP_OEM_JOIN_RSP

DESCRIPTION
  This function is invoked by the OEM MAC implementation to deliver response 
  of a previous JOIN request.

PARAMETERS
  result_code: contains the response result code.
  
RETURN VALUE
   None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_oem_join_rsp
(
  wlan_dot11_result_code_enum result_code
);



/*===========================================================================
FUNCTION WLAN_ADP_OEM_REASSOC_RSP

DESCRIPTION
  This function is invoked by the OEM MAC implementation to deliver response 
  of a previous Reassociation request.

PARAMETERS
  result_code: contains the response result code.
  
RETURN VALUE
   None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_oem_reassoc_rsp
(
  wlan_adp_reassoc_rsp_info_type* reassoc_rsp_info
);




/*===========================================================================
FUNCTION WLAN_ADP_OEM_DEAUTH_RSP

DESCRIPTION
  This function is invoked by the OEM MAC implementation to deliver response 
  of a previous Deauthentication request.

PARAMETERS
  result_code: contains the response result code.
  
RETURN VALUE
   None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_oem_deauth_rsp
(
  wlan_dot11_result_code_enum result_code
);


/*===========================================================================
FUNCTION WLAN_ADP_OEM_DISASSOC_RSP

DESCRIPTION
  This function is invoked by the OEM MAC implementation to deliver response 
  of a previous Disassociation request.

PARAMETERS
  result_code: contains the response result code.
  
RETURN VALUE
   None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_oem_disassoc_rsp
(
  wlan_dot11_result_code_enum result_code
);



/*===========================================================================
FUNCTION WLAN_ADP_OEM_MIB_GET_RSP

DESCRIPTION
  This function is invoked by the OEM MAC implementation to deliver the value
	of an MIB parameter.

PARAMETERS
  status: contains the response result code.
	item_id: identifier of MIB parameter.
	item_value: Value of MIB parameter.
  
RETURN VALUE
   None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_oem_mib_get_rsp
(
  wlan_dot11_result_code_enum status,                                        
  wlan_mib_item_value_type*   value
);



/*===========================================================================
FUNCTION WLAN_ADP_OEM_MIB_SET_RSP

DESCRIPTION
  This function is invoked by the OEM MAC implementation to deliver the value
	of an MIB parameter.

PARAMETERS
  status: contains the response result code.
	item_id: identifier of MIB parameter. 
  
RETURN VALUE
   None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_oem_mib_set_rsp
(
  wlan_dot11_result_code_enum   status,                                        
  wlan_mib_item_id_enum_type    item_id
);


/*===========================================================================
FUNCTION WLAN_ADP_OEM_RESET_RSP

DESCRIPTION
  This function is invoked by the OEM MAC implementation to deliver response 
  of a previous RESET request.

PARAMETERS
  result_code: contains the response result code.
  
RETURN VALUE
   None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_oem_reset_rsp
(
  wlan_dot11_result_code_enum result_code
);



/*===========================================================================
FUNCTION WLAN_ADP_OEM_PWR_MGMT_RSP

DESCRIPTION
  This function is invoked by the OEM MAC implementation to deliver response 
  of a previous Power Management mode request.

PARAMETERS
  result_code: contains the response result code.
  
RETURN VALUE
   None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_oem_pwr_mgmt_rsp
(
  wlan_dot11_result_code_enum result_code
);


/*===========================================================================
FUNCTION WLAN_ADP_OEM_WPA_SETKEYS_RSP

DESCRIPTION
  This function is invoked by the OEM MAC implementation to confirm that the
  action associated to setting the WPA keys has been completed.

PARAMETERS
  None
  
RETURN VALUE
  None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_oem_wpa_setkeys_rsp
(
  void
);


/*===========================================================================
FUNCTION WLAN_ADP_OEM_WPA_DELKEYS_RSP

DESCRIPTION
  This function is invoked by the OEM MAC implemention to confirm that the
  action associated to deleting the WPA keys has been completed..

PARAMETERS
  None
  
RETURN VALUE
  None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_oem_wpa_delkeys_rsp
(
  void
);


/*===========================================================================
FUNCTION WLAN_ADP_OEM_WPA_SET_PROTECTION_RSP

DESCRIPTION
  This function is invoked by the OEM MAC implementation to confirm that the
  action associated to setting the WPA protection has been completed. 

PARAMETERS
  None
  
RETURN VALUE
  None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_oem_wpa_set_protection_rsp
(
  void
);


/*===========================================================================
FUNCTION WLAN_ADP_OEM_ADDTS_RSP

DESCRIPTION
  This function is invoked by the OEM MAC implementation to deliver response 
  of a previous ADDTS request.

PARAMETERS
  addts_rsp_info: pointer to data structure containing info  about the ADDTS
                 response.This buffer belongs to the caller and will not be 
                 freed by WLAN Adapter.
  
RETURN VALUE
   None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_oem_addts_rsp
(
  wlan_adp_addts_rsp_info_type* addts_rsp_info
);


/*===========================================================================
FUNCTION WLAN_ADP_OEM_DELTS_RSP

DESCRIPTION
  This function is invoked by the OEM MAC implementation to deliver response 
  of a previous DELTS request.

PARAMETERS
  delts_rsp_info: pointer to data structure containing info  about the DELTS
                 response.This buffer belongs to the caller and will not be 
                 freed by WLAN Adapter.
  
RETURN VALUE
   None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_oem_delts_rsp
(
  wlan_adp_delts_rsp_info_type* delts_rsp_info
);


/*===========================================================================
FUNCTION WLAN_ADP_OEM_DISASSOC_IND

DESCRIPTION
  This function is invoked by the OEM MAC implementation to indicate a disas-
  sociation event to the WLAN Adaptation layer.

PARAMETERS
  ptr_evt_info: pointer to buffer containing info about the disassociation  
  event.

RETURN VALUE
   None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_oem_dissasoc_ind
(
  wlan_adp_disassoc_evt_info_type* ptr_evt_info
);



/*===========================================================================
FUNCTION WLAN_ADP_OEM_DEAUTH_IND

DESCRIPTION
  This function is invoked by the OEM MAC implementation to indicate a deau-
  thentication event to the WLAN Adaptation layer.

PARAMETERS
  ptr_evt_info: pointer to buffer containing info about the deauthentication  
  event.

RETURN VALUE
   None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_oem_deauth_ind
(
  wlan_adp_deauth_evt_info_type* ptr_evt_info
);


/*===========================================================================
FUNCTION WLAN_ADP_OEM_SYNC_LOST_IND

DESCRIPTION
  This function is invoked by the OEM MAC implementation to indicate a synch
  lost event to the WLAN Adaptation layer.

PARAMETERS
  None  

RETURN VALUE
   None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_oem_synch_lost_ind
(
  void
);



/*===========================================================================
FUNCTION WLAN_ADP_OEM_LOWER_LINK_FAILURE_IND

DESCRIPTION
  This function is invoked by the OEM MAC implementation to indicate a lower
  link failure at the WLAN Adaptation layer.

PARAMETERS
  None  

RETURN VALUE
  None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_oem_lower_link_failure_ind
(
  void
);




/*===========================================================================
FUNCTION WLAN_ADP_OEM_FATAL FAILURE_IND

DESCRIPTION
  This function is invoked by the OEM MAC implementation to indicate a fatal
  failure demanding the  owverall 802.11 SW to be restarted.

PARAMETERS
  None  

RETURN VALUE
   None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_oem_fatal_failure_ind
(
  void
);



/*===========================================================================
FUNCTION WLAN_ADP_OEM_PWR_STATE_IND

DESCRIPTION
  This function is invoked by the OEM MAC implementation to indicate a change
  of pwer save state within the firmware.

PARAMETERS
  ptr_evt_info: pointer to data describing the new power state.  

RETURN VALUE
   None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_oem_pwr_state_ind
(
  wlan_adp_pwr_state_evt_info_type* ptr_evt_info
);

/*===========================================================================
FUNCTION WLAN_ADP_OEM_WPA_MIC_FAILURE_IND

DESCRIPTION
  This function is invoked by the OEM MAC implementation to indicate a MIC 
  failure.

PARAMETERS
  ptr_evt_info: pointer to data describing the new power state.  

RETURN VALUE
   None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_oem_wpa_mic_failure_ind
(
  wlan_adp_wpa_mic_failure_info_type* ptr_info
);


/*===========================================================================
FUNCTION WLAN_ADP_OEM_WPA_PROTECT_FRAMEDRROPED_IND

DESCRIPTION
  This function is invoked by the OEM MAC implementation to indicate that a
  MAC frame has dropped due to protection frame.

PARAMETERS
  sa_addr1: MAC address of the Source Address.
  ra_addr2: MAC address of the Receiver Address.

RETURN VALUE
   None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_oem_wpa_protect_framedropped_ind
(
   byte* sa_addr1, 
   byte* da_addr2
);


/*===========================================================================
FUNCTION WLAN_ADP_OEM_DELTS_IND

DESCRIPTION
  This function is invoked by the OEM MAC implementation to indicate a DELTS
  event to the WLAN Adaptation layer.

PARAMETERS
  ptr_evt_info: pointer to buffer containing info about the delts event.

RETURN VALUE
   None
 
DEPENDENCIES
  wlan_adp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void wlan_adp_oem_delts_ind
(
  wlan_adp_delts_evt_info_type* ptr_evt_info
);

#ifdef CONFIG_HOST_TCMD_SUPPORT
void wlan_adp_oem_report_ftm_test_results
(
  byte*                           ptr_rx_buffer,
  uint32                          buffer_len
);
#endif
#endif /*WLAN_ADP_OEM_H */
