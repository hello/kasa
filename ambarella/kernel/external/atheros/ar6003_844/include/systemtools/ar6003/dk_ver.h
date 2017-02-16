/* dk_ver.h - macros and definitions for memory management */

/*
 *  Copyright ?2000-2001 Atheros Communications, Inc.,  All Rights Reserved.
 */

#ident  "ACI $Id: //depot/sw/releases/olca3.1-RC/include/systemtools/ar6003/dk_ver.h#23 $, $Header: //depot/sw/releases/olca3.1-RC/include/systemtools/ar6003/dk_ver.h#23 $"

#ifndef __INCdk_verh
#define __INCdk_verh
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "AR6K_version.h"
#define ART_VERSION_MAJOR __VER_MAJOR_
#define ART_VERSION_MINOR __VER_MINOR_
#define ART_VERSION_PATCH __VER_PATCH_
#define ART_BUILD_NUM     __BUILD_NUMBER_
#define ART_BUILD_DATE    "07282011"

#define MAUIDK_VER1 ("\n   --- Atheros: MDK (multi-device version) ---\n")

#define MAUIDK_VER_SUB ("              - Revision %d.%d.%d.%d ")
#ifdef CUSTOMER_REL
#ifdef INDONESIA_BUILD
#define MAUIDK_VER2  ("    Revision %d.%d.%d BUILD %d_IN ART_11n")
#else
#define MAUIDK_VER2  ("    Revision %d.%d.%d BUILD %d ART_11n")
#endif
#ifdef ANWI
#define MAUIDK_VER3 ("\n    Customer Version (ANWI BUILD)-\n")
#else
#define MAUIDK_VER3 ("\n    Customer Version -\n")
#endif //ANWI
#else
#define MAUIDK_VER2 ("    Revision %d.%d.%d BUILD %d ART_11n")
#define MAUIDK_VER3 ("\n    --- Atheros INTERNAL USE ONLY ---\n")
#endif
#define DEVLIB_VER1 ("\n    Devlib Revision %d.%d.%d BUILD %d ART_11n\n")

#ifdef ART
#define MDK_CLIENT_VER1 ("\n    --- Atheros: ART Client (multi-device version) ---\n")
#else
#define MDK_CLIENT_VER1 ("\n    --- Atheros: MDK Client (multi-device version) ---\n")
#endif
#define MDK_CLIENT_VER2 ("    Revision %d.%d.%d BUILD %d ART_11n -\n")

#define NART_VERSION_MAJOR __VER_MAJOR_
#define NART_VERSION_MINOR __VER_MINOR_
#define NART_VERSION_PATCH __VER_PATCH_
#define NART_BUILD_NUM     __BUILD_NUMBER_
#define NART_BUILD_DATE    "08092011_SH"
#define NART_VER1           ("\n--- Atheros: NART (New Atheros Radio Test) ---\n")

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INCdk_verh */
