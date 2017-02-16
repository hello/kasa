/* dk_event.h - contains definitions for event.c */

/*
 * Copyright (c) 2000 Atheros Communications, Inc., All Rights Reserved
 * 
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
 */

#ifndef __DK_EVENT_H_
#define __DK_EVENT_H_

#include "dk.h"

#define INTERRUPT_F2    1
#define TIMEOUT         4
#define ISR_INTERRUPT   0x10
#define DEFAULT_TIMEOUT 0xff

typedef struct event_handle_ {
    UINT32 eventID;
    UINT32 f2Handle;
} event_handle;

struct event_struct_ {
	struct event_struct_ *pNext;         // pointer to next event
	struct event_struct_ *pLast;         // backward pointer to pervious event
	event_handle    eventHandle;
	UINT32            type;
	UINT32            persistent;
	UINT32            param1;
	UINT32            param2;
	UINT32            param3;
	UINT32            result[6];
};

typedef struct event_struct_ event_struct;
typedef struct event_struct_ *p_event_struct;

typedef struct event_queue_ {
	p_event_struct  pHead;     // pointer to first event in queue
	p_event_struct   pTail;     // pointer to last event in queue
	UINT16       queueSize;  // count of how many items are in queue
	UINT32		flags;
} event_queue, *p_event_queue;

void initEventQueue(p_event_queue);

void deleteEventQueue(p_event_queue);

p_event_struct createEvent
(
	UINT32    type,          // the event ID
	UINT32    persistent,    // set if want a persistent event
	UINT32    param1,        // optional args
	UINT32    param2,
	UINT32    param3,
	event_handle    eventHandle
);

p_event_struct copyEvent
(
	p_event_struct pExistingEvent // pointer to event to copy
);

UINT16 pushEvent
(
	p_event_struct pEvent,    // pointer to event to add
	p_event_queue pQueue,     // pointer to queue to add to
	BOOLEAN          protect
);

p_event_struct popEvent
(
	p_event_queue pQueue, // pointer to queue to add to
	BOOLEAN          protect
);

UINT16 removeEvent
(
	p_event_struct    pEvent,
	p_event_queue     pQueue,
	BOOLEAN          protect
);

UINT16 checkForEvents
(
	p_event_queue pQueue,
	BOOLEAN          protect
);

#endif
