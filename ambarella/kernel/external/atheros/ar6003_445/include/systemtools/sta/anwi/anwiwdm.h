// Copyright (c) 2010 Atheros Communications Inc.
// All rights reserved.
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

/*
 * anwiWdm.h - this header file contains all the
 * declarations for the anwi WDM driver. 
 *
 * Revisions
 *
 */

#ifndef __ANWI_WDM__
#define __ANWI_WDM__

#include "ntddk.h"
#include "anwi.h"

// The device Extension for the anwi wdm dummy device
typedef struct DEVICE_EXTENSION_ {
	PDEVICE_OBJECT pDevice; // Back pointer to device object
	PDRIVER_OBJECT pDriver; // Pointer to the driver object
	PDEVICE_OBJECT pLowerDevice; // Pointer to the next lower decice in the stack
	ULONG  deviceNumber;
	UNICODE_STRING symLinkName;
	UNICODE_STRING symLinkName_uart;
	ULONG	dummy;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

#endif /* __ANWI_WDM__ */
