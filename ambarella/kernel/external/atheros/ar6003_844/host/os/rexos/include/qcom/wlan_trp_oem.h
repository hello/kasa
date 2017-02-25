#ifndef WLAN_TRP_OEM_H
#define WLAN_TRP_OEM_H
/*===========================================================================
 
										W L A N   TRANSPORT  / OEM MODULE
                           H E A D E R

DESCRIPTION
   This file declares some functions that the OEM Module should implement to 
	 operate within the WLAN transport task.
	 The implementation of the API defined below is at the discretion of the 
   particular OEM, based on mainly how it communicates with the firware 
   running on the 802.11 ASIC.

OEM DEPENDENT FUNCTIONS
   wlan_trp_oem_init()
   wlan_oem_tx_open()
   wlan_oem_cmd_open()
   wlan_oem_cmd_hdlr()
   wlan_oem_tx_req()
   wlan_oem_get_mac_iface()   
   
   Copyright (c) 2004 by QUALCOMM Incorporated.  All Rights Reserved.
===========================================================================*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

when       who     what, where, why
--------   ---     ----------------------------------------------------------
04/12/05   aku     Changed prototype of wlan_trp_oem_init() to return an 
                   error on failure to initialize OEM module.
03/25/05   hba     Advertized API for other OEM layer to retrieve WLAN OEM
                   IFACE. 
02/05/09   hba     Unified Tx request API using raw byte buffer with the one
                   using DSM item.
11/02/04   hba     Creation. 
===========================================================================*/

          
/*===========================================================================

                      INCLUDE FILES FOR MODULE

===========================================================================*/


#include "wlan_trp.h"
#include "wlan_adp_def.h"
#include "wlan_adp_oem.h"
#ifdef MATS
#include "dsm.h"
#endif


/*===========================================================================

															DATA DECLARATIONS

===========================================================================*/



/*===========================================================================

                      FONCTIONS PROTOTYPES

===========================================================================*/
/*===========================================================================
FUNCTION WLAN_TRP_OEM_INIT

DESCRIPTION
  This function is invokeed to perform any OEM specific initialization. This 
  implementation make sure this function is called prior to any command 
  arriving at the WLAN Transport task.
	Therefore, the OEM layer should implement here any internal initialization
	logic.

PARAMETERS
  None.
		
RETURN VALUE
  -1 on failure. 
  0 on success.
	 
SIDE EFFECTS
  None.
	
DEPENDENCIES
  None.
===========================================================================*/
int wlan_trp_oem_init
(
	void
);


/*===========================================================================
FUNCTION WLAN_TRP_OEM_GET_MAC_IFACE

DESCRIPTION
  This function is invoked to retrieve the MAC iface registered by WLAN TRP
  with the WLAN Adapter. 

PARAMETERS
  None.
		
RETURN VALUE
	A pointer to the mac iface
	 
SIDE EFFECTS
  None.
	
DEPENDENCIES
  None.
===========================================================================*/
wlan_oem_mac_iface_type* wlan_trp_oem_get_mac_iface
(
  void
);


/*===========================================================================
FUNCTION WLAN_OEM_TX_OPEN

DESCRIPTION
  This function queries the OEM Module to determine if it is opened for 
	transmission.

PARAMETERS
	None.
	  
RETURN VALUES
	TRUE:  OEM is open for Transmission
  FALSE: OEM is not opened for transmisson
SIDE EFFECTS
  None.
	
DEPENDENCIES
  
===========================================================================*/
boolean wlan_oem_tx_open
(
  void 
);


/*===========================================================================
FUNCTION WLAN_OEM_CMD_OPEN

DESCRIPTION
  This function queries the OEM Module to determine if it is opened for 
	commands processing.

PARAMETERS
	None.
	  
RETURN VALUES
	TRUE:  OEM is open for Command processing
  FALSE: OEM is not opened for Command processing
  
SIDE EFFECTS
  None.
	
DEPENDENCIES
  
===========================================================================*/
boolean wlan_oem_cmd_open
(
  void 
);

/*===========================================================================
FUNCTION WLAN_OEM_CMD_HDLR

DESCRIPTION
  This function is invoked to let the OEM handle a specific command comming
  from the upper layer

PARAMETERS
	A pointer to the command. Note that when this function returns, the cmd is
  recycled.
	  
RETURN VALUES
	None.
  
SIDE EFFECTS
  None.
	
DEPENDENCIES
  
===========================================================================*/
void wlan_oem_cmd_hdlr
(
  wlan_trp_adp_cmd_type* ptr_cmd
);



/*===========================================================================
FUNCTION WLAN_OEM_TX_REQ

DESCRIPTION
  This function gives an LLC PDU for transmission at the OEM Module.

PARAMETERS
	pdu_len: Length of PDU
	buf: pointer to buffer containing LLC PDU
	meta_info: pointer to meta info about PDU to transmit.
	  
RETURN VALUES
	 0: if successfull
  -1: if not
	 
SIDE EFFECTS
  None.
	
DEPENDENCIES
  
===========================================================================*/
int wlan_oem_tx_req
(
  uint16                          pdu_len,
  byte*                           buf,
  wlan_adp_tx_pkt_meta_info_type* meta_info
);


/*===========================================================================
FUNCTION WLAN_OEM_TX_PKT_REQ

DESCRIPTION
  This function gives an LLC PDU for transmission to the OEM Module. The
  LLC paket is formatted 

PARAMETERS
	pkt: dsm item containing the pkt to xmit 
	  
RETURN VALUES
   0: if successfull
  -1: if not
	 
SIDE EFFECTS
  None.
	
DEPENDENCIES
  
===========================================================================*/
int wlan_oem_tx_pkt_req
(
  uint16                          pdu_len,
  dsm_item_type*                  tx_pkt,
  wlan_adp_tx_pkt_meta_info_type* meta_info
);

#endif /* WLAN_TRP_OEM_H */
