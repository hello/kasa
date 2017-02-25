#ifndef WLAN_MIB_H
#define WLAN_MIB_H
/*===========================================================================
                               WLAN_MIB . H

DESCRIPTION
  Header file for conveying MIB parameters between the differents modules of 
  the WLAN stack.

  Copyright (c) 2004 by QUALCOMM, Incorporated.  All Rights Reserved.
===========================================================================*/

/*===========================================================================

                            EDIT HISTORY FOR FILE
                            
$Header: //depot/sw/releases/olca3.1-RC/host/os/rexos/include/qcom/wlan_mib.h#3 $        
$Author: gijo $ $DateTime: 2012/03/29 04:00:18 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
02/09/05    hba    Added SSID to supported MIB attributes
01/27/05    hba    Minor Cleanup
11/10/04    hba    Creation
===========================================================================*/


/*===========================================================================

                                INCLUDE FILES

===========================================================================*/
#ifdef MATS
#include "comdef.h"
#include "customer.h"
#endif


#include "wlan_dot11_sys.h"       /* To import some WLAN SYS data types    */

/*===========================================================================

                              DATA DECLARATIONS

===========================================================================*/

/*---------------------------------------------------------------------------
WLAN_ADP_MIB_ITEM_ID_ENUM_TYPE:

  This is an enumeration of all the MIB items which may be involved in 
  transaction between the differents modules of the WLAN Stack.
  This enumeration is not exhaustive and more identifiers can be added as we 
  support more parameters.
  
    Please, note that only the parameters which are in interest of the WLAN 
  stack are taken into account here. The others parameters that are in the 
  802.11 Std are supposed to be directly managed at the OEM level following 
  in a proprietary way. 
---------------------------------------------------------------------------*/
typedef enum
{
  WLAN_MIB_MIN_ID                   =-1,

  /*-------------------------------------------------------------------------
    Station Management relatives parameters:      
  -------------------------------------------------------------------------*/
  WLAN_MIB_PRIVACY_SUPPORTED_ID     = 0, /* Indicate if we're supportting 
                                            privacy. Value should be 
                                            represented as boolean         */
  WLAN_MIB_AUTH_ALGO_ID             = 1, /* Indicate which authentication 
                                            algorithm we support if the 
                                            privacy option is implemented.
                                            Value should be of type 
                                            wlan_dot11_algo_type_enum      */

  WLAN_MIB_WEP_DEFAULT_KEYS_ID      = 2, /* Default WEP keys table         
                                            Value representation defined 
                                            below                          */

  WLAN_MIB_WEP_DEFAULT_KEY_INDEX_ID = 3, /* Defaut WEP key Index. Value 
                                            should be represented as a char*/

  
  /*-------------------------------------------------------------------------
    MAC Attribute relatives parameters: 
  -------------------------------------------------------------------------*/
  WLAN_MIB_MAC_STA_UNICAST_ADDR_ID  = 10, /* This STA unicast MAC address  
                                             Value represented as an uint64*/

  WLAN_MIB_MAC_OPERATION_INFO_ID    = 11, /* A set of parameters the MAC 
                                            should operate on. Value
                                            representation defined below.  */

  WLAN_MIB_MAC_COUNTERS_ID          = 12, /* A set of values of MAC counters
                                            maintenained at the OEM level.
                                            Value representation defined 
                                            below                          */

  WLAN_MIB_MAC_STA_MULTICAST_ADDR   = 13, /* This STA multicast MAC address.
                                          Value represented as an uint64   */                                          

 
  /*-------------------------------------------------------------------------
   PHY Attributes relatives parameters
  -------------------------------------------------------------------------*/
  WLAN_MIB_PHY_OPERATION_INFO_ID    = 21, /* A set of parameters the PHY 
                                             should operate on. Value 
                                             representation defined below  */

  WLAN_MIB_PHY_ANTENNA_INFO_ID      = 22, /* PHY Antenna configuration. Value
                                             representation defined below  */

  WLAN_MIB_PHY_MAX_TX_POWER_ID      = 23, /* PHY Max Tx power. Value
                                             reprensation in mW on a uint16*/

  WLAN_MIB_SSID_ID                  = 24, /* SSID on which the OEM layer 
                                            should operate. Value represented
                                            as a wlan_dot11_ssid_type      */
  WLAN_MIB_MAX_ID

} wlan_mib_item_id_enum_type;


/*---------------------------------------------------------------------------
  Default value of the WEP Default key index! 
---------------------------------------------------------------------------*/
#define WLAN_MIB_WEP_DEFAULT_KEY_INDEX 0

/*---------------------------------------------------------------------------
WLAN_MIB_WEP_KEY_ENUM_TYPE
  
  This is an enumeration of the supported WEP keys
---------------------------------------------------------------------------*/
typedef enum
{
  WLAN_MIB_WEP_40_BITS  = 0,
  WLAN_MIB_WEP_104_BITS = 1

} wlan_mib_wep_key_enum_type;


#define WLAN_MIB_WEP_40_SIZE   5
#define WLAN_MIB_WEP_104_SIZE  13
/*---------------------------------------------------------------------------
WLAN_MIB_WEP_KEY_VALUE_TYPE
  
  This structure is defined to hold the value of a WEP key
---------------------------------------------------------------------------*/
typedef union
{
  uint8      wep_40  [WLAN_MIB_WEP_40_SIZE];
 
  uint8      wep_104 [WLAN_MIB_WEP_104_SIZE];

} wlan_mib_wep_key_value_type;


/*---------------------------------------------------------------------------
WLAN_MIB_WEP_DEFAULT_KEYS_TYPE

  This structure represents the default WEP keys
---------------------------------------------------------------------------*/
typedef struct
{

  wlan_mib_wep_key_enum_type  wep_type;
  wlan_mib_wep_key_value_type wep_keys[4];

} wlan_mib_wep_default_keys_type;


/*---------------------------------------------------------------------------
 WLAN_MIB_MAC_OPERATION_INFO_TYPE
 
 This structure defines different operational paramters on which is
 operating the 802.11 MAC implementation.
---------------------------------------------------------------------------*/
#define WLAN_MIB_DEFAULT_RTS_THRESHOLD             2347
#define WLAN_MIB_DEFAULT_SHORT_RETRY_LIMIT         7
#define WLAN_MIB_DEFAULT_LONG_RETRY_LIMIT          4
#define WLAN_MIB_DEFAULT_FRAGMENTATION_THRESHOLD   256
#define WLAN_MIB_DEFAULT_MAX_TX_LIFETIME           512
#define WLAN_MIB_DEFAULT_MAX_RX_LIFETIME           512

typedef struct
{

  uint32 rts_threshold;          /* RTS Treshold                           */ 
  uint32 short_retry_limit;      /* Short RETRY LIMIT                      */
  uint32 long_retry_limit;       /* LONG RETRY LIMIT                       */
  uint32 fragmentation_treshold; /* Fragmentation treshold                 */
  uint32 max_msdu_tx_lifetime;   /* Maximum MSDU transmission lifetime     */
  uint32 max_rx_lifetime;        /* Maximun time to receive an MSDU        */

} wlan_mib_mac_operation_info_type;


/*---------------------------------------------------------------------------
 WLAN_MIB_MAC_COUNTERS_TYPE
 
 This structure defines the various counters internally maintenained by the
 802.11 MAC implementation that the upper WLAN stack may query.
---------------------------------------------------------------------------*/
typedef struct
{

  uint32 tx_frag_count;            /* Number of successfully tx MPDU       */

  uint32 multicast_tx_frame_count; /* Number of multicast frame successfully
                                      transmitted                          */

  uint32 failed_count;             /* Number of MSDU failed to be txed     */

  uint32 retry_count;              /* Number of MSDU transmitted with one or
                                      more retransmissions                 */

  uint32 mutiple_retry_count;      /* Number of MSDU transmitted with more
                                      than one retransmissions             */

  uint32 frame_duplicate_count;    /* Number of duplicated frames received */

  uint32 rts_success_count;        /* Number of time CTS is receieved after
                                      RTS transmission                     */

  uint32 rts_failure_count;        /* Number of failed RTS transmission    */

  uint32 ack_failure_count;        /* Number of times, the MAC failed to 
                                      recieve an expected ACK              */

  uint32 rx_frag_count;            /* Number of MPDU successully received of
                                      type Data or management              */

  uint32 multicaxt_rx_frag_count;  /* Number of received multicast MSDU    */
  
  uint32 fcs_error_count;          /* Number of MPDU received with FCS 
                                      error                                */
  
  uint32 tx_frame_count;           /* Number of successfully transmitted 
                                      MSDU                                 */

  uint32 wep_undecrypted_count;    /* Number of MSDU that fails to be
                                      correctly decrypted                  */ 

} wlan_mib_mac_counters_type;


/*---------------------------------------------------------------------------
 WLAN_MIB_REG_DOMAIN_ENUM_TYPE
 
  Enumeration of the various regulatory domains that are supported within
  the 802.11 Std
---------------------------------------------------------------------------*/
typedef enum
{

  WLAN_MIB_REG_DOMAIN_FCC    = 0x10, /* USA                                */  
  WLAN_MIB_REG_DOMAIN_DOC    = 0x20, /* Canada                             */
  WLAN_MIB_REG_DOMAIN_ETSI   = 0x30, /* Most of Europe                     */
  WLAN_MIB_REG_DOMAIN_SPAIN  = 0x31, /* Spain  (Europe)                    */
  WLAN_MIB_REG_DOAMIN_FRANCE = 0x32, /* France (Europe)                    */
  WLAN_MIB_REG_DOMAIN_MKK    = 0x40  /* Japan                              */

} wlan_mib_reg_domain_enum_type;
 

/*---------------------------------------------------------------------------
 WLAN_MIB_PHY_ENUM_TYPE
  
   This is the enumeration of the different type of PHY
---------------------------------------------------------------------------*/
typedef enum
{

  WLAN_MIB_FHSS_PHY = 0,
  WLAN_MIB_DSSS_PHY = 1,
  WLAN_MIB_IR_PHY   = 2

} wlan_mib_phy_enum_type;


/*---------------------------------------------------------------------------
WLAN_MIB_PHY_TEMP_ENUM_TYPE
  
  This is an enumeration of the various operating temperature range of the 
  PHY.
---------------------------------------------------------------------------*/
typedef enum
{

  WLAN_MIB_PHY_TEMP_COMMERCIAL = 1, /* Commercial range of 0 to 40 degrees 
                                       Celsius                             */
  WLAN_MIB_PHY_TEMP_INDUSTRIAL = 2  /* Industrial range of -30 to 70 degrees
                                       Celsius                             */
} wlan_mib_phy_temp_enum_type;


/*---------------------------------------------------------------------------
 WLAN_MIB_DIVERSITY_ENUM_TYPE
 
    This is the enumeration of the various types of diversity support
---------------------------------------------------------------------------*/
typedef enum
{
  WLAN_MIB_DIVERSITY_FIXEDLIST     = 1, /* Diversity performed on a fixed
                                           list of  selected antenna       */
  WLAN_MIB_DIVERSITY_NOT_SUPPORTED = 2, /* Diversity not supported         */

  WLAN_MIB_DIVERSITY_DYNAMIC       = 3, /* Diversiy supported  on a dynamic
                                           list of selected antenna        */
} wlan_mib_diversity_enum_type;


/*---------------------------------------------------------------------------
WLAN_MIB_PHY_OPERATION_INFO

  This structure defines some parameters the under layer PHY should operate 
  on. Of course those parameters are not exhaustive and the OEM 
  implementation should manage the remainingone it considers relevnt.
---------------------------------------------------------------------------*/
typedef struct
{
   wlan_mib_phy_enum_type        phy_type;
   wlan_mib_reg_domain_enum_type reg_domain;
   wlan_mib_phy_temp_enum_type   phy_temp;

} wlan_mib_phy_operation_info_type;


/*---------------------------------------------------------------------------
WLAN_MIB_PHY_ANTENNA_INFO_TYPE

  This structure define configuration information for the WLAN implementation
  antenna.
---------------------------------------------------------------------------*/
typedef struct
{
  uint32                       tx_antenna; /* Identifier of current Tx 
                                              antenna                      */
  wlan_mib_diversity_enum_type diversity;  /* Supported Diversity          */

  uint32                       rx_antenna; /* Identifier of current Rx 
                                              antenna                      */  
 
} wlan_mib_phy_antenna_info_type;



/*---------------------------------------------------------------------------
WLAN_MIB_VALUE_TYPE

  This data  type is defined to hold the value of an MIB parameter crossing 
  the interface of the differents WLAN softwae modules.
  
  Here are the different possible data that can hold an MIB parameter value:
    - a  boolean
    - an uint8
    - an uint16
    - a integer 
    - an unsigned integer
    - a MAc address on 64 bits (uint64)
    - an authentication algo type
    - any of the data type defined above

 Please add to this list any newly defined data type that may hold an MIB 
 item value.
---------------------------------------------------------------------------*/
typedef struct
{
  wlan_mib_item_id_enum_type         item_id;
  
  union
  {
    boolean                          bool_value;
    uint8                            char_value;
    uint16                           short_value;
    sint31                           int_value;
    uint32                           uint32_value;
    wlan_ieee_mac_address_type       mac_addr;
    wlan_dot11_ssid_type             ssid;
    wlan_dot11_auth_algo_type_enum   auth_algo;
    wlan_mib_wep_default_keys_type   default_keys;
    wlan_mib_mac_operation_info_type mac_operation_info;
    wlan_mib_mac_counters_type       mac_counters;
    wlan_mib_phy_operation_info_type phy_operation_info;
    wlan_mib_phy_antenna_info_type   phy_antenna_info;
  } value;

} wlan_mib_item_value_type;


/*===========================================================================

                      PUBLIC FUNCTION DECLARATIONS

===========================================================================*/

/*===========================================================================
FUNCTION WLAN_ADP_MIB_GET_REQ()

DESCRIPTION
  This function is invoked by upper layer to retrieve the value of an MIB
  param as defined above.

PARAMETERS
  req_info: data structure containing parameters for the command.
  rsp_cback: pointer to calback for response notification.

RETURN VALUE
   0 if request has been taken into account the WLAN Adapter layer.
  -1 if an error occurs

DEPENDENCIES
  wlan_adp_start should have been called.

SIDE EFFECTS
  None
===========================================================================*/

/*---------------------------------------------------------------------------
 Definition of the response callback prototype. Arguments include
   - the MIB parameter ID.
   - a data structure containing the value of the MIB item. Please note that
     the pointer passed still belongs to the WLAN Adapter and the callee
     should not try to free it.
---------------------------------------------------------------------------*/
typedef void(*wlan_adp_mib_get_rsp_cback)(wlan_dot11_result_code_enum status, 
                                          wlan_mib_item_value_type*   value);

int wlan_adp_mib_get_req
(
  wlan_mib_item_id_enum_type req_info,
  wlan_adp_mib_get_rsp_cback rsp_cback
);



/*===========================================================================
FUNCTION WLAN_ADP_MIB_SET_REQ()

DESCRIPTION
  This function is invoked by upper layer to set the value of an MIB
  param as defined above.

PARAMETERS
  ptr_req_info: pointer to data structure containing parameters for the 
                command.
  rsp_cback: pointer to calback for response notification.

RETURN VALUE
   0 if request has been taken into account the WLAN Adapter layer.
  -1 if an error occurs

DEPENDENCIES
  wlan_adp_start should have been called.

SIDE EFFECTS
  None
===========================================================================*/

/*---------------------------------------------------------------------------
 Definition of the response callback prototype. Arguments include
   - the MIB parameter ID.
   - Result code giving the outcome of the SET operation.
---------------------------------------------------------------------------*/
typedef void (*wlan_adp_mib_set_rsp_cback)(wlan_dot11_result_code_enum res,
																					 wlan_mib_item_id_enum_type  id);

int wlan_adp_mib_set_req
(
  wlan_mib_item_value_type*  ptr_req_info,
  wlan_adp_mib_set_rsp_cback rsp_cback
);
#endif /* WLAN_MIB_H */
