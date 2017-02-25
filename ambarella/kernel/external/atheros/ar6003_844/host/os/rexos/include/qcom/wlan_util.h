#ifndef WLAN_UTIL_H
#define WLAN_UTIL_H

#include "wpa_itf.h"

/*===========================================================================
                               WLAN_UTIL . H

DESCRIPTION
  Header file for the PS 802 LLC protocol suite Internal Operaions.

  Copyright (c) 2004 by QUALCOMM, Incorporated.  All Rights Reserved.
===========================================================================*/

/*===========================================================================

                            EDIT HISTORY FOR FILE

           $Author: gijo $ $DateTime: 2012/03/29 04:00:18 $
          

when        who    what, where, why
--------    ---    ----------------------------------------------------------
11/08/05    hba    Added new API for WPA IE generation
06/07/05    hba    Creation
===========================================================================*/


#ifndef WLAN_INTERNAL_ASSERT

#define WLAN_ASSERT(_statement_ )                              \
        do                                                     \
        {                                                      \
          if (!(_statement_))                                    \
          {                                                    \
            ERR( "Assertion " #_statement_ " failed", 0,0,0 ); \
          }                                                    \
        } while ( 0 )
 
#else

#define WLAN_ASSERT(_statement_)                               \
          ASSERT(_statement_ )

#endif



/*===========================================================================
FUNCTION WLAN_UTIL_UINT64_TO_BYTEARRAY

DESCRIPTION
  This function is invoked to convert an uint64 to a byte array.

PARAMETERS
  mac_hw_addr: pointer to destination where to copy the byte array  
  addr: hardware address on 64 bits

RETURN VALUE
   None
 
DEPENDENCIES
   None

SIDE EFFECTS
  None
===========================================================================*/
void wlan_util_uint64_to_macaddress
(
  byte*   mac_hw_addr,
  uint64  uint64_addr
);


/*===========================================================================
FUNCTION WLAN_ADPI_BYTEARRAY_TO_UINT64

DESCRIPTION
   This function convert an array of byte into a uint64

PARAMETERS
   hw_addr: pointer to the MAC address to be converted.  

RETURN VALUE
   The value of the mac address as an uint64
 
DEPENDENCIES
   None

SIDE EFFECTS
  None
===========================================================================*/
uint64 wlan_util_macaddress_to_uint64
(
  byte * hw_addr
);


/*===========================================================================
FUNCTION WLAN_ADPI_BYTEARRAY_TO_UINT64

DESCRIPTION
   This function is used to generate a WPA IE 

PARAMETERS
   group_cipher:     The selected group cipher suite 
   pairwise_cipher:  The selected pairwise cipher
   akm:              The seleced authentication suite
   ptr_wpa_ie:       Destination to copy WPA IE
   
RETURN VALUE
   The value of lenght of IE generated.
 
DEPENDENCIES
   None

SIDE EFFECTS
  None
===========================================================================*/
uint16  wlan_util_generate_wpa_ie
(
  wpa_cipher_suite_enum  group_cipher,
  wpa_cipher_suite_enum  pairwise_cipher,
  wpa_akm_suite_enum     akm,
  uint8*                 ptr_wpa_ie
);



/*===========================================================================
FUNCTION WLAN_UTIL_STORE_HW_MAC_ADDR

DESCRIPTION
   This function is used to store the device HW MAC address in DB in case the
   MAC address is not configurable

PARAMETERS
   64 bits value containing MAC address to store.
   
RETURN VALUE
   The value of lenght of IE generated.
 
DEPENDENCIES
   None

SIDE EFFECTS
  None
===========================================================================*/
void wlan_util_set_hw_mac_addr
(
  uint64  mac_address
);

#endif // WLAN_UTIL_H
