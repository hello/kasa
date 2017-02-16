/*
 * Copyright (c) 2004-2005 Atheros Communications Inc.
 * All rights reserved.
 *
 * 
// The software source and binaries included in this development package are
// licensed, not sold. You, or your company, received the package under one
// or more license agreements. The rights granted to you are specifically
// listed in these license agreement(s). All other rights remain with Atheros
// Communications, Inc., its subsidiaries, or the respective owner including
// those listed on the included copyright notices.  Distribution of any
// portion of this package must be in strict compliance with the license
// agreement(s) terms.
// </copyright>
// 
// <summary>
// 	Wifi driver for AR6003
// </summary>
//
 *
 */

#include <stdlib.h>
#include <stdio.h>

#include "athdefs.h"
#include "a_types.h"
#include "a_osapi.h"
#include "netbuf.h"
//#define ATH_MODULE_NAME netbuf
#include "a_debug.h"

#ifdef DEBUG
ATH_DEBUG_INSTANTIATE_MODULE_VAR(netbuf,
                                 "netbuf",
                                 "net buffer",
                                 ATH_DEBUG_MASK_DEFAULTS,
                                 0,
                                 NULL);
#endif //DEBUG

static int s_allocs = 0;
static int s_frees = 0;

void *
a_netbuf_alloc(int size)
{
	struct pbuf *p;

	AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("a_netbuf_alloc - size %d\n", size));

	/* 1k of extra room (512 headroom + 512 tailroom) */
	p = (struct pbuf *) A_MALLOC(size + sizeof(struct pbuf) + 1024);

	if(p == NULL)
	{
		return(NULL);
	}

	p->start = (char *) p + sizeof(struct pbuf);
	p->data = p->start + 512;
	p->len = 0;
	p->max = size;
	p->next = NULL;	/* Make sure this is clean */

	s_allocs++;

	AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("PBUF ALLOC 0x%08x %d %d\n", (unsigned int)p, s_allocs, s_allocs - s_frees));

    return ((void *)p);
}

void
a_netbuf_free(void *bufPtr)
{
	AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("a_netbuf_free\n"));

	s_frees++;

	AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("PBUF FREE 0x%08x %d %d\n", (unsigned int)bufPtr, s_frees, s_allocs - s_frees));

	A_FREE(bufPtr);
}

A_UINT32
a_netbuf_to_len(void *bufPtr)
{
	struct pbuf *p = (struct pbuf *) bufPtr;

	AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("a_netbuf_to_len - len %d\n", p->len));

    return (p->len);
}

void *
a_netbuf_to_data(void *bufPtr)
{
	struct pbuf *p = (struct pbuf *) bufPtr;

	AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("a_netbuf_to_data - data 0x%08x\n", (unsigned int)(p->data)));

    return (p->data);
}

/*
 * Add len # of bytes to the beginning of the network buffer
 * pointed to by bufPtr
 */
A_STATUS
a_netbuf_push(void *bufPtr, A_INT32 len)
{
	struct pbuf *p = (struct pbuf *) bufPtr;

	AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("a_netbuf_push - len %d\n", len));
	p->data -= len;
	p->len += len;

    return A_OK;
}

/*
 * Add len # of bytes to the end of the network buffer
 * pointed to by bufPtr
 */
A_STATUS
a_netbuf_put(void *bufPtr, A_INT32 len)
{
	struct pbuf *p = (struct pbuf *) bufPtr;

	p->len += len;

	AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("a_netbuf_put - len %d %d\n", len, p->len));

    return A_OK;
}

/*
 * Trim the network buffer pointed to by bufPtr to len # of bytes 
 */
A_STATUS
a_netbuf_trim(void *bufPtr, A_INT32 len)
{
	struct pbuf *p = (struct pbuf *) bufPtr;

	AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("a_netbuf_trim - len %d @ 0x%08x\n", len, (unsigned int) p));

    return A_OK;
}

/*
 * Returns the number of bytes available to a a_netbuf_push()
 */
A_INT32
a_netbuf_headroom(void *bufPtr)
{
	struct pbuf *p = (struct pbuf *) bufPtr;
	int room;

	
	room = p->data - p->start;

	AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("a_netbuf_headroom - headroom %d\n", room));

    return (room);
}

void *
a_netbuf_head(void *bufPtr)
{
    struct pbuf *p = (struct pbuf *) bufPtr;
    
    AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("a_netbuf_head\n"));
    
    return (void *) p->start;
}

/*
 * Removes specified number of bytes from the beginning of the buffer
 */
A_STATUS
a_netbuf_pull(void *bufPtr, A_INT32 len)
{
	struct pbuf *p = (struct pbuf *) bufPtr;

	p->len -= len;
	p->data += len;

	AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("a_netbuf_pull - len %d\n", len));

    return A_OK;
}
/* 
 * Combines a_netbuf_to_data() and memcpy functionality.
 */ 

A_STATUS
a_netbuf_put_data(void *bufPtr, void *srcPtr,  A_INT32 len)
{
	struct pbuf *p = (struct pbuf *) bufPtr;

	AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("a_netbuf_put_data - data 0x%08x\n", (unsigned int) (p->data))); 
    
    A_MEMCPY(p->data, srcPtr, len);
    return A_OK;
}

A_UINT32
a_copy_to_user(void *to, const void *from, A_UINT32 n)
{
	AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("a_copy_to_user - len %d\n", n));

    return((A_UINT32) memcpy(to, from, n));
}

A_UINT32
a_copy_from_user(void *to, const void *from, A_UINT32 n)
{
	AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("a_copy_from_user - len %d\n", n));

    return((A_UINT32) memcpy(to, from, n));
}
 
void a_netbuf_enqueue(A_NETBUF_QUEUE_T *q, struct pbuf *pkt)
{
	A_MUTEX_LOCK(&q->lock);

	/* Is list empty */
	if(q->head == NULL)
	{
		/* Put first in list */
		q->head = pkt;
		q->size = 0;	/* defensive */
		AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("a_netbuf_enqueue - First\n")); 
	}
	else
	{
		/* Put last in list */
		q->tail->next = pkt;
		AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("a_netbuf_enqueue - Last\n")); 
	}
	q->tail = pkt;
	q->size++;

	AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("a_netbuf_enqueue - Head 0x%08x Tail 0x%08x Next 0x%08x Size %d\n",
		(unsigned int) q->head, (unsigned int) q->tail,
		(unsigned int) (pkt != NULL ? pkt->next : 0), q->size)); 

	A_MUTEX_UNLOCK(&q->lock);

	return;
}

void a_netbuf_prequeue(A_NETBUF_QUEUE_T *q, struct pbuf *pkt)
{
	A_MUTEX_LOCK(&q->lock);

	/* Adjust tail if list is empty */
	if(q->head == NULL)
	{
		/* Put first in list */
		q->tail = pkt;
		q->size = 0;	/* defensive */
		AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("a_netbuf_prequeue - First\n")); 
	}

	/* Remember next guy */
	pkt->next = q->head;

	q->head = pkt;
	q->size++;

	AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("a_netbuf_prequeue - Head 0x%08x Tail 0x%08x Next 0x%08x Size %d \n",
		(unsigned int) q->head, (unsigned int) q->tail,
		(unsigned int) (pkt != NULL ? pkt->next : 0), q->size)); 

	A_MUTEX_UNLOCK(&q->lock);

	return;
}

struct pbuf *a_netbuf_dequeue(A_NETBUF_QUEUE_T *q)
{
	struct pbuf *pkt;

	A_MUTEX_LOCK(&q->lock);

	/* Is list empty */
	if(q->head == NULL)
	{
		pkt = NULL;
		AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("a_netbuf_dequeue - Empty\n")); 
	}
	else
	{
		/* Get first element */
		pkt = q->head;

		/* Advance the list */
		q->head = pkt->next;
		q->size--;

		/* Make sure the next pointer is clean */
		pkt->next = NULL;

		AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("a_netbuf_dequeue - First\n")); 

		/* Adjust tail pointer if list becomes empty */
		if(q->head == NULL)
		{
			AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("a_netbuf_dequeue - Single\n")); 

			q->tail = NULL;
			q->size = 0; /* defensive */
		}
	}

	AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("a_netbuf_dequeue - Head 0x%08x Tail 0x%08x Next 0x%08x Size %d\n",
		(unsigned int) q->head, (unsigned int) q->tail,
		(unsigned int) (pkt != NULL ? pkt->next : 0), q->size)); 

	A_MUTEX_UNLOCK(&q->lock);

	return(pkt);
}

int a_netbuf_queue_size(A_NETBUF_QUEUE_T *q)
{
	int size;

	A_MUTEX_LOCK(&q->lock);
	size = q->size;
	A_MUTEX_UNLOCK(&q->lock);

	return(size);
}

void a_netbuf_queue_init(A_NETBUF_QUEUE_T *q)
{
	A_MEMZERO(q, sizeof(A_NETBUF_QUEUE_T));
	A_MUTEX_INIT(&q->lock);

	return;
}

