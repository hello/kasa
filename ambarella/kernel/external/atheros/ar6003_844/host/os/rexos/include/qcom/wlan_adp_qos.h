#ifndef WLAN_ADP_QOS_H
#define WLAN_ADP_QOS_H
/*===========================================================================
                       W L A N _ A D P _ Q O S . H

DESCRIPTION
  Header file for the WLAN ADAPTER interface for QoS support

  Copyright (c) 2004 by QUALCOMM, Incorporated.  All Rights Reserved.
===========================================================================*/

/*===========================================================================

                            EDIT HISTORY FOR FILE

           $Author: gijo $ $DateTime: 2012/03/29 04:00:18 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
12/17/05    lyr    Creation
===========================================================================*/


/*===========================================================================

                                INCLUDE FILES

===========================================================================*/

#ifdef MATS
#include "comdef.h"
#endif



/*===========================================================================

                              DATA DECLARATIONS

===========================================================================*/

/*---------------------------------------------------------------------------
  General QoS types
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
 WLAN_ADP_QOS_TYPE_ENUM
   Enumeration of the various QoS types
---------------------------------------------------------------------------*/
typedef enum
{
  WLAN_ADP_QOS_NONE_TYPE = 0,
  WLAN_ADP_QOS_WMM_TYPE  = 1,

  WLAN_ADP_QOS_MAX_TYPE
} wlan_adp_qos_type_enum;

/*---------------------------------------------------------------------------
 WLAN_ADP_QOS_EDCA_AC_ENUM
   Enumeration of the various EDCA Access Categories:
   Based on AC to ACI mapping in 802.11e spec (identical to WMM)
---------------------------------------------------------------------------*/
typedef enum
{
  WLAN_ADP_QOS_EDCA_AC_BE = 0,  /* Best effort access category             */
  WLAN_ADP_QOS_EDCA_AC_BK = 1,  /* Background access category              */
  WLAN_ADP_QOS_EDCA_AC_VI = 2,  /* Video access category                   */
  WLAN_ADP_QOS_EDCA_AC_VO = 3,  /* Voice access category                   */
  
  WLAN_ADP_QOS_EDCA_AC_MAX
} wlan_adp_qos_edca_ac_enum;


/*---------------------------------------------------------------------------
  WMM-related types
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
 WLAN_ADP_WMM_ELEMENT_TYPE_ENUM
   Enumeration of the various WMM Element types
---------------------------------------------------------------------------*/
typedef enum
{
  WLAN_ADP_WMM_ELEMENT_TYPE_NONE = 0,
  WLAN_ADP_WMM_ELEMENT_TYPE_IE   = 1,
  WLAN_ADP_WMM_ELEMENT_TYPE_PE   = 2,
  
  WLAN_ADP_WMM_ELEMENT_MAX
} wlan_adp_wmm_element_type_enum;

/*---------------------------------------------------------------------------
 WLAN_ADP_WMM_QOS_INFO_TYPE:
    This structure describes the QoS information from a WMM AP
---------------------------------------------------------------------------*/
typedef struct
{
  boolean   wmm_ps;            /* Is WMM power-save supported?             */
  uint16  param_set_cnt;       /* Parameter set count                      */
} wlan_adp_wmm_qos_info_type;


/*---------------------------------------------------------------------------
 WLAN_ADP_WMM_AC_PARAMS_TYPE
   The WMM Access Category Parameters
---------------------------------------------------------------------------*/
typedef struct
{ 
  uint8   aci;                 /* AC index (see wmm_ac_enum                */
  boolean acm;                 /* admission control mandatory?             */
} wlan_adp_wmm_ac_params_type;

/*---------------------------------------------------------------------------
 WLAN_ADP_WMM_IE_TYPE
   Information from the WMM IE
---------------------------------------------------------------------------*/
typedef struct
{
  wlan_adp_wmm_qos_info_type qos_info;
} wlan_adp_wmm_ie_type;

/*---------------------------------------------------------------------------
 WLAN_ADP_WMM_PE_TYPE
   Information from the WMM PE
---------------------------------------------------------------------------*/
typedef struct
{
  wlan_adp_wmm_qos_info_type   qos_info;
  wlan_adp_wmm_ac_params_type  ac_params[WLAN_ADP_QOS_EDCA_AC_MAX];
} wlan_adp_wmm_pe_type;

/*---------------------------------------------------------------------------
 WLAN_ADP_WMM_INFO_TYPE
   Information from the WMM IE or PE
---------------------------------------------------------------------------*/
typedef struct
{
  wlan_adp_wmm_element_type_enum wmm_element_type;

  union
  {
    wlan_adp_wmm_ie_type wmm_ie;
    wlan_adp_wmm_pe_type wmm_pe;
  } element;

} wlan_adp_wmm_info_type;

/*---------------------------------------------------------------------------
 WLAN_ADP_WMM_UP_ENUM
   Enumeration of the various User priority (UP) types
   
   From 802.1D/802.11e/WMM specifications (all refer to same table)
---------------------------------------------------------------------------*/
typedef enum
{
  WLAN_ADP_WMM_UP_BE      = 0,
  WLAN_ADP_WMM_UP_BK      = 1,
  WLAN_ADP_WMM_UP_RESV    = 2,    /* Reserved                              */
  WLAN_ADP_WMM_UP_EE      = 3,
  WLAN_ADP_WMM_UP_CL      = 4,
  WLAN_ADP_WMM_UP_VI      = 5,
  WLAN_ADP_WMM_UP_VO      = 6,
  WLAN_ADP_WMM_UP_NC      = 7,
  
  WLAN_ADP_WMM_UP_MAX
} wlan_adp_wmm_up_type_enum;

/*---------------------------------------------------------------------------
  WLAN_ADP_WMM_APSD_TYPE_ENUM
    Enumeration of the APSD types
---------------------------------------------------------------------------*/
typedef enum
{
  WLAN_ADP_WMM_APSD_OFF   = 0,
  WLAN_ADP_WMM_APSD_ON    = 1,

  WLAN_ADP_WMM_APSD_MAX
} wlan_adp_wmm_apsd_type_enum;

/*---------------------------------------------------------------------------
 WLAN_ADP_WMM_TS_DIR_ENUM
   Enumeration of the various TSPEC directions
   
   From 802.11e/WMM specifications
---------------------------------------------------------------------------*/
typedef enum
{
  WLAN_ADP_WMM_TS_DIR_UPLINK   = 0,
  WLAN_ADP_WMM_TS_DIR_DOWNLINK = 1,
  WLAN_ADP_WMM_TS_DIR_RESV     = 2,   /* Reserved                          */
  WLAN_ADP_WMM_TS_DIR_BOTH     = 3,

  WLAN_ADP_WMM_TS_DIR_MAX
} wlan_adp_wmm_ts_dir_enum;

/*---------------------------------------------------------------------------
 WLAN_ADP_WMM_TS_INFO_TYPE
   TS Info field in the WMM TSPEC
   
   See suggestive values above
---------------------------------------------------------------------------*/
typedef struct
{
  wlan_adp_wmm_up_type_enum    up;        /* User priority                 */
  wlan_adp_wmm_apsd_type_enum  psb;       /* power-save bit                */
  wlan_adp_wmm_ts_dir_enum     direction; /* Direction                     */
  uint8                        tid;       /* TID                           */
} wlan_adp_wmm_ts_info_type;

/*---------------------------------------------------------------------------
 WLAN_ADP_WMM_TSPEC_TYPE
   The WMM TSPEC Element (from the WMM spec)
---------------------------------------------------------------------------*/
typedef struct
{ 
  wlan_adp_wmm_ts_info_type      ts_info;
  uint16                         nominal_msdu_size;
  uint16                         maximum_msdu_size;
  uint32                         min_service_interval;
  uint32                         max_service_interval;
  uint32                         inactivity_interval;
  uint32                         suspension_interval;
  uint32                         svc_start_time;
  uint32                         min_data_rate;
  uint32                         mean_data_rate;
  uint32                         peak_data_rate;
  uint32                         max_burst_size;
  uint32                         delay_bound;
  uint32                         min_phy_rate;
  uint16                         surplus_bw_allowance;
  uint16                         medium_time;
} wlan_adp_wmm_tspec_type;


/*---------------------------------------------------------------------------
 WLAN_ADP_QOS_PARAMS_TYPE
   QoS information for the given association
---------------------------------------------------------------------------*/
typedef struct
{
  wlan_adp_qos_type_enum qos_type; /* Type of QoS supported for curr assoc */
  
  wlan_adp_wmm_pe_type   wmm_pe;   /* WMM info (valid for WMM QoS type)    */

} wlan_adp_qos_params_type;


/*---------------------------------------------------------------------------
 WLAN_ADP_QOS_MI_TYPE
   QoS meta info associated with a packet
---------------------------------------------------------------------------*/
typedef uint8 wlan_adp_qos_mi_type;




#endif // WLAN_ADP_QOS_H
