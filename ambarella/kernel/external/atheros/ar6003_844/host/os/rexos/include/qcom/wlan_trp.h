#ifndef WLAN_TRP_H
#define WLAN_TRP_H
/*===========================================================================
 
										W L A N   TRANSPORT  T A S K
                           H E A D E R

DESCRIPTION
   This file declares functions prototypes and other external
   definitions to support the WLAN Transport task.
   
   Copyright (c) 2004 by QUALCOMM Incorporated.  All Rights Reserved.
===========================================================================*/

/*===========================================================================

                      EDIT HISTORY FOR FILE
                      
  $Header: //depot/sw/releases/olca3.1-RC/host/os/rexos/include/qcom/wlan_trp.h#3 $
  $Author: gijo $ $DateTime: 2012/03/29 04:00:18 $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
05/23/05   hba     Added WPA related data strutures.
04/26/05   hba     Added MAC Configuration setting primitive.
01/17/05   hba     Modification to support the Philip WLAN Module.
10/28/04   hba     Creation. 
===========================================================================*/

          
/*===========================================================================

                      INCLUDE FILES FOR MODULE

===========================================================================*/

#ifdef MATS
#include "customer.h"
#include "comdef.h"

#include "queue.h"
#endif
#include "wlan_dot11_sys.h"
#include "wlan_adp_def.h"
#include "wlan_mib.h"

/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/
   
/*---------------------------------------------------------------------------
    Signals for the transport task:
		 Please, note that this is not an exhaustive list of all the signals that 
     may be supported by the WLAN transport task.
     You may add here other signal the WLAN Transport task should handle
		 and update appropriately the wlan_trp_task() function...
		 WARMING: Note that the values 0x8000, 0x4000 and 0x2000 are reserved for
		 the TMC and respectively used as the TASK_START, TASK_STOP and 
		 TASK_OFFLINE signals.
---------------------------------------------------------------------------*/

#define WLAN_TRP_RPT_TIMER_SIG            0x0001
#define WLAN_TRP_RX_SIG                   0x0002
#define WLAN_TRP_TX_SIG                   0x0004
#define WLAN_TRP_ADP_CMD_Q_SIG            0x0008
#define WLAN_TRP_START_SIG                0x0010
#define WLAN_TRP_STOP_SIG                 0x0020

#ifdef FEATURE_WLAN_PHG

#define WLAN_TRP_PHG_HHAL_MGMT_CNF_Q_SIG  0x0040
#define WLAN_TRP_PHG_HHAL_EVT_IND_Q_SIG   0x0080
#define WLAN_TRP_PHG_HHAL_TX_CNF_SIG      0x0100
#define WLAN_TRP_PHG_HHAL_CNF_TIMER_SIG   0x0200
#define WLAN_TRP_PHG_HHAL_RSSI_UPD_SIG    0x0400

#endif //FEATURE_WLAN_PHG
   


/*---------------------------------------------------------------------------
TYPEDEF WLAN_TRP_ADP_CMD_ENUM:

DESCRIPTION:
  Enumeration of the differents requests supported by the WLAN transport task
---------------------------------------------------------------------------*/
typedef enum
{

  WLAN_TRP_ADP_MIN_CMD      = -1,

  WLAN_TRP_ADP_SUSPEND_CMD  =  0,
  WLAN_TRP_ADP_RESUME_CMD   =  1,
  WLAN_TRP_ADP_SCAN_CMD     =  2,
  WLAN_TRP_ADP_JOIN_CMD     =  3,
  WLAN_TRP_ADP_AUTH_CMD     =  4,
  WLAN_TRP_ADP_ASSOC_CMD    =  5,
  WLAN_TRP_ADP_REASSOC_CMD  =  6,
  WLAN_TRP_ADP_DEAUTH_CMD   =  7,
  WLAN_TRP_ADP_DISASSOC_CMD =  8,
  WLAN_TRP_ADP_PWR_MGMT_CMD =  9,
  WLAN_TRP_ADP_MIB_GET_CMD  = 10,
  WLAN_TRP_ADP_MIB_SET_CMD  = 11,
  WLAN_TRP_ADP_OPR_GET_CMD  = 12,
  WLAN_TRP_ADP_RESET_CMD    = 13,
  WLAN_TRP_ADP_MAC_CFG_CMD  = 14,

  WLAN_TRP_ADP_WPA_SETKEYS_CMD    = 15,
  WLAN_TRP_ADP_WPA_DELKEYS_CMD    = 16,
  WLAN_TRP_ADP_WPA_SETPROTECT_CMD = 17,

  WLAN_TRP_ADP_ADDTS_CMD    = 18,
  WLAN_TRP_ADP_DELTS_CMD    = 19,
#ifdef CONFIG_HOST_TCMD_SUPPORT
  WLAN_TRP_ADP_TEST_CMD =20,
#endif
  WLAN_TRP_ADP_BTHCI_CMD    = 21,
  WLAN_TRP_ADP_BT_DISPATCHER_CMD = 22,
  WLAN_TRP_ADP_MAX_CMD

} wlan_trp_adp_cmd_enum_type;


/*---------------------------------------------------------------------------
TYPEDEF WLAN_TRP_REQ_INFO_TYPE

DESCRIPTION:
	Union representing the info carried by a WLAN transport task command. Any
	data type carrying a request that is translated into a command enqueue for
	the transport task should be part of this data structure.
---------------------------------------------------------------------------*/
typedef union
{
  wlan_adp_scan_req_info_type        scan_info;
  wlan_adp_join_req_info_type        join_info;
  wlan_adp_auth_req_info_type        auth_info;
  wlan_adp_assoc_req_info_type       assoc_info;
  wlan_adp_reassoc_req_info_type     reassoc_info;
  wlan_adp_disassoc_req_info_type    disassoc_info;
  wlan_adp_deauth_req_info_type      deauth_info;
  wlan_adp_pwr_mgmt_req_info_type    pwr_mgmt_info;
  wlan_adp_reset_req_info_type       reset_info;
  wlan_mib_item_value_type           mib_info;
  wlan_dot11_opr_param_id_enum       opr_param_id;
  wlan_adp_mac_config_req_info_type  config_info;

  wlan_adp_wpa_setkeys_req_info_type setkeys_info;
  wlan_adp_wpa_delkeys_req_info_type delkeys_info;
  wlan_adp_wpa_protect_list_type     protect_info;
  
  wlan_adp_addts_req_info_type       addts_info;
  wlan_adp_delts_req_info_type       delts_info;
#ifdef CONFIG_HOST_TCMD_SUPPORT
  wlan_adp_trp_test_data         ftm_test_data;
#endif
  wlan_adp_bthci_req_info_type       bthci_info;
  wlan_adp_bt_dispatcher_req_info_type  btdispatcher_info;
} wlan_trp_adp_req_type;



/*---------------------------------------------------------------------------
TYPDEDEF WLAN_TRP_ADP_HDR_TYPE:
 
DESCRIPTION:
	WLAN Transport task common command header. Should be included here any info
	that help identified the command and facilitates its processing.
---------------------------------------------------------------------------*/
typedef struct 
{
  q_link_type                link;   /* Queue link                         */     
	wlan_trp_adp_cmd_enum_type cmd_id; /* Control task Command               */    
} wlan_trp_adp_hdr_type;              



/*---------------------------------------------------------------------------
TYPE DEFINITION WLAN_TRP_ADP_CMD_TYPE

DECRIPTION
	 WLAN Transport task command structure.
---------------------------------------------------------------------------*/
typedef struct
{
  wlan_trp_adp_hdr_type hdr;      /* Command header                        */
  wlan_trp_adp_req_type req_info; /* Information about the request received*/
} wlan_trp_adp_cmd_type;




/*===========================================================================

                      FONCTIONS PROTOTYPES

===========================================================================*/


/*===========================================================================
FUNCTION WLAN_TRP_ADP_GET_CMD_BUF()

DESCRIPTION
  This function is called to retrieved a free command buffer. The buffer is 
  latter filled with the appropriate command and the caller 
  uses WLAN_TRP_POST_CMD to post it to the WLAN transport task.

DEPENDENCIES
  Should be called after proper initialization of the WLAN transport task.

RETURN VALUE
  A pointer to a WLAN TRP command buffer.

SIDE EFFECTS
  None.
===========================================================================*/
wlan_trp_adp_cmd_type* wlan_trp_adp_get_cmd_buf
(
	void
);


/*===========================================================================
FUNCTION WLAN_TRP_ADP_POST_CMD()

DESCRIPTION
  This function posts a command to the WLAN transport task.

DEPENDENCIES
  Must be called after proper initialization of the WLAN Transport context.

RETURN VALUE
  None.

SIDE EFFECTS
  None.
===========================================================================*/
void wlan_trp_adp_post_cmd
(
	wlan_trp_adp_cmd_type* trp_cmd
);




/*===========================================================================
FUNCTION WLAN_TRP_ADP_CMD_COMPLETE_IND()

DESCRIPTION
  This function notifies the ADP that a command that it has posted has ben 
  completed. 

DEPENDENCIES
  Must be called after proper initialization of the WLAN Transport context.

RETURN VALUE
  None.

SIDE EFFECTS
  None.
===========================================================================*/
void wlan_trp_adp_cmd_complete_ind
(
  void
);


/*===========================================================================
FUNCTION WLAN_TRP_TX_SIG_HDLR()

DESCRIPTION
  This is the WLAN_TRP_TX_SIG signal handler. This function is called by the 
	WLAN Transport task main task loop when an LLC PDU is available at the WLAN
	Adapter layer for transmission.
	
	This function depends on the OEM Module implementation logic.

DEPENDENCIES
  Must be called in the WLAN Transport context.

RETURN VALUE
  TRUE:  TX signal should be cleared.
	FASLE: TX signal should not be cleared.
	
SIDE EFFECTS
  None.
===========================================================================*/
boolean wlan_trp_tx_sig_hdlr
(
	void
);


/*===========================================================================
FUNCTION WLAN_TRP_RX_SIG_HDLR()

DESCRIPTION
  This is the WLAN_TRP_RX_SIG signal handler. Contrary to the Tx sig handle, 
  there is no reason to postpone the processing of received data. 
  
	This function depends on the OEM implentation logic.

DEPENDENCIES
  Must be called in the WLAN Transport context.

RETURN VALUE
  None
	
SIDE EFFECTS
  None.
===========================================================================*/
void wlan_trp_rx_sig_hdlr
(
	void
);




/*===========================================================================
FUNCTION WLAN_TRP_START_SIG_HDLR()

DESCRIPTION
  This is the WLAN_TRP_START_SIG signal handler. This function start 
  by clearing the WLAN_TRP_START_SIG and then invokes the OEM specific 
  startup function.

DEPENDENCIES
  Must be called in the WLAN Transport context.

RETURN VALUE
  None 
	
SIDE EFFECTS
  None.
===========================================================================*/
void wlan_trp_start_sig_hdlr
(
	void
);



/*===========================================================================
FUNCTION WLAN_TRP_STOP_SIG_HDLR()

DESCRIPTION
  This is the WLAN_TRP_STOP_SIG signal handler. This function start 
  by clearing the WLAN_TRP_STOP_SIG and then invokes the OEM specific 
  stop function.

DEPENDENCIES
  Must be called in the WLAN Transport context.

RETURN VALUE
  None 
	
SIDE EFFECTS
  None.
===========================================================================*/
void wlan_trp_stop_sig_hdlr
(
	void
);


#endif /* WLAN_TRP_H */
