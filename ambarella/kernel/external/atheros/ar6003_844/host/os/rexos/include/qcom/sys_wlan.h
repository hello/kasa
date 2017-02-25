#ifndef SYS_WLAN_H
#define SYS_WLAN_H

/*===========================================================================

              WLAN S Y S T E M   H E A D E R   F I L E

DESCRIPTION
  This header file contains definitions that are shared between Call Manager,
  Call Manager clients and the WLAN protocol stacks.

Copyright (c) 1991 - 2005 by QUALCOMM INCORPORATED. All Rights Reserved.

Export of this technology or software is regulated by the U.S. Government.
Diversion contrary to U.S. law prohibited.

===========================================================================*/


/*===========================================================================

                      EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

  $Header: //depot/sw/releases/olca3.1-RC/host/os/rexos/include/qcom/sys_wlan.h#3 $

when       who     what, where, why
--------   ---    ----------------------------------------------------------
10/08/05   ic     Added Header: field 
05/11/05   ic     Added metacomments under FEATURE_HTORPC_METACOMMENTS
10/28/04   dk     Initial version
===========================================================================*/

/**--------------------------------------------------------------------------
** Includes
** --------------------------------------------------------------------------
*/
#ifdef MATS
#ifndef AEE_SIMULATOR
#include "comdef.h"    /* Common definitions such as byte, uint16, etc. */
#endif
#endif
/*---------------------------------------------------------------------------
   General macros
---------------------------------------------------------------------------*/

#define SYS_WLAN_PROFILE_MAX_SIZE  120

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/* Enumeration of Technology type/version.
*/
typedef enum
{

  /* FOR INTERNAL USE OF CM ONLY!.
  */
  SYS_TECH_NONE                        = -1,

  /* WLAN 802.11a technology.
  */
  SYS_TECH_WLAN_80211A                 = 0,

  /* WLAN 802.11b technology.
  */
  SYS_TECH_WLAN_80211B                 = 1,

  /* WLAN 802.11g technology.
  */
  SYS_TECH_WLAN_80211G                 = 2,


  /* Reserved values for CM use.
  */
  SYS_TECH_RESERVED                    = 30,

  /* FOR INTERNAL USE OF CM ONLY!.
  */
  SYS_TECH_MAX

} sys_tech_e_type;


/*
** Enumeration of technology type mask.
** It converts bit position of sys_tech_e_type to a bit mask.
** To keep numbering scheme consistent,
** use sys_tech_e_type instead of numbers directly.
**
*/
/*
** Technology type bit mask data type. It is a combination of enumeration of
** system band class mask.
*/
typedef uint32                          sys_tech_mask_type;


#define SYS_BM_32BIT( val )            (sys_tech_mask_type)(1<<(int)(val))

#define SYS_TECH_MASK_EMPTY             0
     /* No technology selected */

/* Acquire wlan 80211a systems only.
*/
#define SYS_TECH_PREF_MASK_80211A       SYS_BM_32BIT(SYS_TECH_WLAN_80211A)

/* Acquire wlan 80211b systems only.
*/
#define SYS_TECH_PREF_MASK_80211B       SYS_BM_32BIT(SYS_TECH_WLAN_80211B)

/* Acquire wlan 80211g systems only.
*/
#define SYS_TECH_PREF_MASK_80211G       SYS_BM_32BIT(SYS_TECH_WLAN_80211G)

/* Add new technology/versions here.
*/



/* Start of Reserved values.
*/
#define SYS_TECH_PREF_MASK_ANY         SYS_BM_32BIT(SYS_TECH_RESERVED)
#define SYS_TECH_PREF_MASK_NO_CHANGE   (sys_tech_mask_type)(SYS_BM_32BIT(SYS_TECH_MASK_ANY)+1)

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/* Enumeration of WLAN Scan preference.
*/
typedef enum
{

  /* FOR INTERNAL USE OF CM ONLY!.
  */
  SYS_WLAN_SCAN_PREF_NONE               = -1,

  /* Active WLAN Scan.
  */
  SYS_WLAN_SCAN_PREF_ACTIVE              = 0,

  /* Passive WLAN Scan.
  */
  SYS_WLAN_SCAN_PREF_PASSIVE             = 1,

  /* WLAN Scanning to be decided automatically.
  */
  SYS_WLAN_SCAN_PREF_AUTO                = 2,

  /* FOR INTERNAL USE OF CM ONLY!.
  */
  SYS_WLAN_SCAN_PREF_MAX

} sys_wlan_scan_pref_e_type;


/* Enumeration of WLAN BSS types.
*/
typedef enum {

  /* FOR INTERNAL USE ONLY!.
  */
  SYS_WLAN_BSS_TYPE_NONE = -1,

  /* Ad-Hoc Type BSS.
  */
  SYS_WLAN_BSS_TYPE_ADHOC,

  /* Infrastructure Mode BSS.
  */
  SYS_WLAN_BSS_TYPE_INFRA,

  /* Any BSS Mode Type.
  */
  SYS_WLAN_BSS_TYPE_ANY,

  /* FOR INTERNAL USE ONLY!.
  */
  SYS_WLAN_BSS_TYPE_MAX

} sys_wlan_bss_e_type;

/* Enumeration of WLAN Rates.
*/
typedef enum {

  /* FOR INTERNAL USE ONLY!.
  */
  SYS_WLAN_RATE_NONE = -1,

  /* 1 Mbps.
  */
  SYS_WLAN_RATE_1_MBPS,

  /* 2 Mbps.
  */
  SYS_WLAN_RATE_2_MBPS,

  /* 5.5 Mbps.
  */
  SYS_WLAN_RATE_5_5_MBPS,

  /* 11 Mbps.
  */
  SYS_WLAN_RATE_11_MBPS,

  /* FOR INTERNAL USE ONLY!.
  */
  SYS_WLAN_RATE_MAX

} sys_wlan_rate_e_type;



/*==============================================================================

                     S Y S T E M   I D E N T I F I E R

==============================================================================*/


/*------------------------------------------------------------------------------
   System Identifier Enums
------------------------------------------------------------------------------*/


/* Maximum size of the SSID.
*/
#define SYS_WLAN_SSID_MAX_SIZE         (int)(32)


/* Type that defines the SSID of WLAN system.
*/
typedef struct sys_wlan_ssid_s
{
  /* Length of the SSID, if == 0, then SSID = broadcast SSID.
  */
  uint8                                len;

  /* SSID of the wlan system.
  */
  char                                 ssid[SYS_WLAN_SSID_MAX_SIZE];

} sys_wlan_ssid_s_type;
#ifdef FEATURE_HTORPC_METACOMMENTS
/*~ FIELD sys_wlan_ssid_s.ssid LENGTH sys_wlan_ssid_s.len */
#endif /* FEATURE_HTORPC_METACOMMENTS */

/* WLAN Statistics
*/

typedef struct sys_wlan_stats_s
{
  sys_wlan_rate_e_type    current_xmit_rate;
  /* Xmit rate of the last packet successfully transmitted */

  uint32                  total_tx_bytes;
  /* Number of bytes transmitted over the WLAN interface  */

  uint32                  total_rx_bytes;
  /* Number of bytes received over the WLAN interface     */

} sys_wlan_stats_s_type;


/* Type for WLAN BSS id.
*/
typedef uint64                        sys_wlan_bssid_type;


/*===========================================================================

FUNCTION sys_is_wlan_ssid_undefined

DESCRIPTION
  This function checks if WLAN ssid is undefined.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
extern boolean sys_is_wlan_ssid_undefined(

  const sys_wlan_ssid_s_type     *sys_id_ptr
);

/*===========================================================================

FUNCTION sys_is_wlan_ssid_match

DESCRIPTION
  This function compares two WLAN SSIDs.

DEPENDENCIES
  None

RETURN VALUE
  TRUE  = if the two system identifiers are equal
  FALSE = if the two system identifiers are not equal

  Note: TRUE is returned if both system identifiers are undefined.

SIDE EFFECTS
  None

===========================================================================*/
extern boolean sys_is_wlan_ssid_match(

  const sys_wlan_ssid_s_type     *ssid_1_ptr,
  const sys_wlan_ssid_s_type     *ssid_2_ptr
);

/*===========================================================================

FUNCTION sys_undefine_wlan_ssid

DESCRIPTION
  This function undefines or initializes a WLAN SSID.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
extern void sys_undefine_wlan_ssid(

  sys_wlan_ssid_s_type     *sys_id_ptr
);

/*===========================================================================

FUNCTION sys_is_wlan_bssid_undefined

DESCRIPTION
  This function checks if WLAN bssid is undefined.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
extern boolean sys_is_wlan_bssid_undefined(

  const sys_wlan_bssid_type    *bss_id_ptr
);

/*===========================================================================

FUNCTION sys_undefine_wlan_bssid

DESCRIPTION
  This function undefines or initializes a WLAN BSSID.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
extern void sys_undefine_wlan_bssid(

  sys_wlan_bssid_type     *bss_id_ptr
);

/*===========================================================================

FUNCTION sys_is_wlan_bssid_match

DESCRIPTION
  This function compares two WLAN BSSIDs.

DEPENDENCIES
  None

RETURN VALUE
  TRUE  = if the two system identifiers are equal
  FALSE = if the two system identifiers are not equal


SIDE EFFECTS
  None

===========================================================================*/
extern boolean sys_is_wlan_bssid_match(

  const sys_wlan_bssid_type     *bssid_1_ptr,
  const sys_wlan_bssid_type     *bssid_2_ptr
);


#endif /* #ifndef SYS_WLAN_H */
