//------------------------------------------------------------------------------
// <copyright file="netbuf.h" company="Atheros and Microsoft">
//    Copyright (c) 2004-2008 Microsoft Corporation.  All rights reserved.
//    Copyright (c) 2004-2008 Atheros Corporation.  All rights reserved.
// 
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
//------------------------------------------------------------------------------
//==============================================================================
// OS Independent Net Buffer interfaces
//
// Author(s): ="Atheros and Microsoft"
//==============================================================================

#ifndef _NETBUF_H_
#define _NETBUF_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pbuf
{
    struct pbuf *next;
    unsigned int len;
    unsigned int max;
    char *data;
    char *start;
} A_PBUF_T;

typedef struct netbuf_queue
{
	struct pbuf *head;
	struct pbuf *tail;
	A_MUTEX_T lock;
	int size;
} A_NETBUF_QUEUE_T;

#define AR6000_DATA_OFFSET 64
 
#define A_NETBUF_ENQUEUE(q, pkt) \
	a_netbuf_enqueue((q), (struct pbuf *) (pkt))
#define A_NETBUF_PREQUEUE(q, pkt) \
	a_netbuf_prequeue((q), (struct pbuf *) (pkt))
#define A_NETBUF_DEQUEUE(q) \
	((void *) a_netbuf_dequeue(q))
#define A_NETBUF_QUEUE_SIZE(q)	\
	a_netbuf_queue_size(q)
#define A_NETBUF_QUEUE_INIT(q)	\
	a_netbuf_queue_init(q)
#define A_NETBUF_ALLOC(size) \
    a_netbuf_alloc(AR6000_DATA_OFFSET + sizeof(HTC_PACKET) + size)
#define A_NETBUF_FREE(bufPtr) \
    a_netbuf_free(bufPtr)
#define A_NETBUF_DATA(bufPtr) \
    a_netbuf_to_data(bufPtr)
#define A_NETBUF_LEN(bufPtr) \
    a_netbuf_to_len(bufPtr)
#define A_NETBUF_PUSH(bufPtr, len) \
    a_netbuf_push(bufPtr, len)
#define A_NETBUF_PUT(bufPtr, len) \
    a_netbuf_put(bufPtr, len)
#define A_NETBUF_TRIM(bufPtr,len) \
    a_netbuf_trim(bufPtr, len)
#define A_NETBUF_PULL(bufPtr, len) \
    a_netbuf_pull(bufPtr, len)
#define A_NETBUF_HEADROOM(bufPtr)\
	a_netbuf_headroom(bufPtr)
#define A_NETBUF_HEAD(bufPtr)\
    a_netbuf_head(bufPtr)

/* Add data to end of a buffer  */
#define A_NETBUF_PUT_DATA(bufPtr, srcPtr,  len) \
	a_netbuf_put_data(bufPtr, dstPtr, len) 

/* Add data to start of the  buffer */
#define A_NETBUF_PUSH_DATA(bufPtr, srcPtr,  len)

/* Remove data at start of the buffer */
#define A_NETBUF_PULL_DATA(bufPtr, dstPtr, len)

/* Remove data from the end of the buffer */
#define A_NETBUF_TRIM_DATA(bufPtr, dstPtr, len)

/* View data as "size" contiguous bytes of type "t" */
#define A_NETBUF_VIEW_DATA(bufPtr, t, size) \
	(t )( ((struct pbuf *)(bufPtr))->data)

/*
 * OS specific network buffer acess routines
 */
void *a_netbuf_alloc(int size);
void a_netbuf_free(void *bufPtr);
void *a_netbuf_to_data(void *bufPtr);
A_UINT32 a_netbuf_to_len(void *bufPtr);
A_STATUS a_netbuf_push(void *bufPtr, A_INT32 len);
A_STATUS a_netbuf_put(void *bufPtr, A_INT32 len);
A_STATUS a_netbuf_trim(void *bufPtr, A_INT32 len);
A_INT32 a_netbuf_headroom(void *bufPtr);
void* a_netbuf_head(void *bufPtr);
A_STATUS a_netbuf_pull(void *bufPtr, A_INT32 len);
A_STATUS a_netbuf_put_data(void *bufPtr, void *srcPtr, A_INT32 len);
A_UINT32 a_copy_to_user(void *to, const void *from, A_UINT32 n);
A_UINT32 a_copy_from_user(void *to, const void *from, A_UINT32 n);
void a_netbuf_enqueue(A_NETBUF_QUEUE_T *q, struct pbuf *pkt);
void a_netbuf_prequeue(A_NETBUF_QUEUE_T *q, struct pbuf *pkt);
struct pbuf *a_netbuf_dequeue(A_NETBUF_QUEUE_T *q);
int a_netbuf_queue_size(A_NETBUF_QUEUE_T *q);
void a_netbuf_queue_init(A_NETBUF_QUEUE_T *q);


#ifdef __cplusplus
}
#endif

#endif //_NETBUF_H_