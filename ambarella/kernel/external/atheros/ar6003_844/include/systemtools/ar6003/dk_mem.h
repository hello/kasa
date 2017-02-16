// dk_mem.h - macros and definitions for memory management

// Copyright © 2000-2001 Atheros Communications, Inc.,  All Rights Reserved.
// 
//
// The software source and binaries included in this development package are
// licensed, not sold. You, or your company, received the package under one
// or more license agreements. The rights granted to you are specifically
// listed in these license agreement(s). All other rights remain with Atheros
// Communications, Inc., its subsidiaries, or the respective owner including
// those listed on the included copyright notices.  Distribution of any
// portion of this package must be in strict compliance with the license
// agreement(s) terms.
//
//

// modification history
// --------------------
// 01Dec01 	sharmat		created (copied from windows client)

#ifndef __INCdk_memh
#define __INCdk_memh


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus 
#include "wlantype.h"

#if defined(VXWORKS) 
#include "hw.h"
#else
#include "common_hw.h"
#endif

// conflicting with a function declared in mAlloc.c using the suffix #2
A_STATUS memGetIndexForBlock2 
(
	MDK_WLAN_DEV_INFO *pdevInfo, // pointer to device info structure 
	A_UCHAR *mapBytes, // pointer to the map bits to reference 
	A_UINT16 numBlocks, // number of blocks want to allocate
	A_UINT16 *pIndex // gets updated with index
);

// conflicting with a function declared in mAlloc.c using the suffix #2
void memMarkIndexesFree2
(
	MDK_WLAN_DEV_INFO *pdevInfo, // pointer to device info structure 
	A_UINT16 index, // the index to free 
	A_UCHAR *mapBytes // pointer to the map bits to reference 
);

#ifdef __cplusplus
}
#endif // __cplusplus 

#endif // __INCdk_memh
