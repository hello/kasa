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
 * This file contains the function declarations for the client controlled
 * devices interface.
 *
 * Again all the structures are defined in anwi.h so that there would
 * be only one file to share between the client and the driver.
 */

#ifndef __ANWICLIENT_H__
#define __ANWICLIENT_H__

#include "ntddk.h"
#include "anwi.h"
#include "anwievent.h"

#define NUM_SUPPORTED_CLIENTS	8
#define UART_FN_DEV_START_NUM	4
#define INVALID_CLIENT -1

#define NETWORK_CLASS	0x2
#define SIMPLE_COMM_CLASS	0x7

// The following structure is maintatined by the WDM
// for each client that joins it. There could be only
// a definite number of clients that could be working
// with the WDM at any time


typedef struct anwiClientInfo_ {
	ULONG32 cliId; // Id for this client.
	ULONG32 busy;
	anwiAddrDesc memMap;
	anwiAddrDesc regMap[MAX_BARS];
    ULONG32 numBars;
	anwiFullResourceInfo resInfo;
	anwiIntrDesc intrDesc;
	anwiEventQueue isrEventQ;
	anwiEventQueue trigeredEventQ;	
	PKSPIN_LOCK sLock; // Pointer to the allocated spinlock for the IRQL
	PDEVICE_OBJECT pDevObj;	// Pointer to the device object for this client
	ULONG32 valid;
	ULONG32 device_class;
} anwiClientInfo, *pAnwiClientInfo;

// Function Declarations
LONG32 initClientTable
(
	VOID
);

VOID cleanupClientTable
(
	VOID
);

LONG32 findNextClientId
(
	LONG32 sIndex
);

LONG32 addClient
(
	ULONG32,
	PDEVICE_OBJECT,
	PCM_RESOURCE_LIST,
	PCM_RESOURCE_LIST
);

VOID removeClient
(
	ULONG32
);

LONG32 registerClient
(
	ULONG32
);

VOID unregisterClient
(
	ULONG32
);

pAnwiClientInfo getClient
(
	LONG32
);

#define VALID_CLIENT(x) ((x)->cliId != INVALID_CLIENT)
#define BUSY_CLIENT(x) ((x)->busy != 0)

#endif /* __CLIENT_DEVICES_H__ */
