/*
 * Copyright (c) 2004-2005 Atheros Communications Inc.
 * All rights reserved.
 *
 * $ATH_LICENSE_HOSTSDK0_C$
 *
 */

#ifdef ATHR_EMULATION
/* Linux/OS include files */
#include <stdlib.h>
#include <stdio.h>
#endif

/* Atheros include files */
#include <athdefs.h>
#include <a_types.h>
#include <a_osapi.h>
#include <wmi.h>

/* Atheros platform include files */
#include "a_debug.h"

#ifdef ATHR_EMULATION
static int s_allocs = 0;
static int s_frees = 0;

void *
a_netbuf_alloc(int size)
{
	struct pbuf *p;

	DPRINTF(DBG_NETBUF, (DBGFMT "Enter - size %d\n", DBGARG, size));

	/* 1k of extra room (512 headroom + 512 tailroom) */
	p = (struct pbuf *) malloc(size + sizeof(struct pbuf) + 1024);

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

	DPRINTF(DBG_NETBUF, (DBGFMT "PBUF ALLOC 0x%08x %d %d\n", DBGARG,
		(unsigned int) p, s_allocs, s_allocs - s_frees));

    return ((void *)p);
}

void
a_netbuf_free(void *bufPtr)
{
	DPRINTF(DBG_NETBUF, (DBGFMT "Enter\n", DBGARG));

	s_frees++;

	DPRINTF(DBG_NETBUF, (DBGFMT "PBUF FREE 0x%08x %d %d\n", DBGARG,
		(unsigned int) bufPtr, s_frees, s_allocs - s_frees));

	free(bufPtr);
}

A_UINT32
a_netbuf_to_len(void *bufPtr)
{
	struct pbuf *p = (struct pbuf *) bufPtr;

	DPRINTF(DBG_NETBUF, (DBGFMT "Enter - len %d\n", DBGARG, p->len));

    return (p->len);
}

void *
a_netbuf_to_data(void *bufPtr)
{
	struct pbuf *p = (struct pbuf *) bufPtr;

	DPRINTF(DBG_NETBUF,
		(DBGFMT "Enter - data 0x%08x\n", DBGARG, (unsigned int) (p->data)));

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

	DPRINTF(DBG_NETBUF, (DBGFMT "Enter - len %d\n", DBGARG, len));
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

	DPRINTF(DBG_NETBUF, (DBGFMT "Enter - len %d %d\n", DBGARG, len, p->len));

    return A_OK;
}

/*
 * Trim the network buffer pointed to by bufPtr to len # of bytes 
 */
A_STATUS
a_netbuf_trim(void *bufPtr, A_INT32 len)
{
	struct pbuf *p = (struct pbuf *) bufPtr;

	DPRINTF(DBG_NETBUF,
		(DBGFMT "Enter - len %d @ 0x%08x\n", DBGARG, len, (unsigned int) p));

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

	DPRINTF(DBG_NETBUF, (DBGFMT "Enter - headroom %d\n", DBGARG, room));

    return (room);
}

void *
a_netbuf_head(void *bufPtr)
{
    struct pbuf *p = (struct pbuf *) bufPtr;
    
    DPRINTF(DBG_NETBUF, (DBGFMT "Enter - head\n", DBGARG));
    
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

	DPRINTF(DBG_NETBUF, (DBGFMT "Enter - len %d\n", DBGARG, len));

    return A_OK;
}
/* 
 * Combines a_netbuf_to_data() and memcpy functionality.
 */ 

A_STATUS
a_netbuf_put_data(void *bufPtr, void *srcPtr,  A_INT32 len)
{
	struct pbuf *p = (struct pbuf *) bufPtr;

	DPRINTF(DBG_NETBUF, 
		(DBGFMT "Enter - data 0x%08x\n", DBGARG, (unsigned int) (p->data))); 
    
   A_MEMCPY(p->data, srcPtr, len);
   return A_OK;
}

A_UINT32
a_copy_to_user(void *to, const void *from, A_UINT32 n)
{
	DPRINTF(DBG_NETBUF, (DBGFMT "Enter - len %d\n", DBGARG, n));

    return((A_UINT32) memcpy(to, from, n));
}

A_UINT32
a_copy_from_user(void *to, const void *from, A_UINT32 n)
{
	DPRINTF(DBG_NETBUF, (DBGFMT "Enter - len %d\n", DBGARG, n));

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
		DPRINTF(DBG_NETBUF, (DBGFMT "First\n", DBGARG)); 
	}
	else
	{
		/* Put last in list */
		q->tail->next = pkt;
		DPRINTF(DBG_NETBUF, (DBGFMT "Last\n", DBGARG)); 
	}
	q->tail = pkt;
	q->size++;

	DPRINTF(DBG_NETBUF, (DBGFMT "Head 0x%08x Tail 0x%08x Next 0x%08x Size %d\n", DBGARG,
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
		DPRINTF(DBG_NETBUF, (DBGFMT "First\n", DBGARG)); 
	}

	/* Remember next guy */
	pkt->next = q->head;

	q->head = pkt;
	q->size++;

	DPRINTF(DBG_NETBUF, (DBGFMT "Head 0x%08x Tail 0x%08x Next 0x%08x Size %d \n", DBGARG,
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
		DPRINTF(DBG_NETBUF, (DBGFMT "Empty\n", DBGARG)); 
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

		DPRINTF(DBG_NETBUF, (DBGFMT "First\n", DBGARG)); 

		/* Adjust tail pointer if list becomes empty */
		if(q->head == NULL)
		{
			DPRINTF(DBG_NETBUF, (DBGFMT "Single\n", DBGARG)); 

			q->tail = NULL;
			q->size = 0; /* defensive */
		}
	}

	DPRINTF(DBG_NETBUF, (DBGFMT "Head 0x%08x Tail 0x%08x Next 0x%08x Size %d\n", DBGARG,
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

#else 
/*****************************************************************************/
/*****************************************************************************/
/**                                                                         **/
/** Qualcomm will define their versions of above functions here.            **/
/**                                                                         **/
/*****************************************************************************/
/*****************************************************************************/ 
 
 
/* Atheros include files */ 
#include <athdefs.h> 
#include <a_types.h> 
#include <a_osapi.h> 
#include <wmi.h> 
 
/* Atheros platform include files */ 
#include "debug.h" 
#include "queue.h"
 
/* QCOM platform include files    */ 
#include "assert.h" 
#include "task.h" 
#include "msg.h" 
 
static int ath_s_allocs_0 = 0; 
static int ath_s_frees_0  = 0; 
 
static int ath_s_allocs_1 = 0; 
static int ath_s_frees_1  = 0; 
 
static int ath_s_allocs_2 = 0; 
static int ath_s_frees_2  = 0; 
 
static int ath_s_allocs_3 = 0; 
static int ath_s_frees_3  = 0; 
 
static int ath_s_allocs_4 = 0; 
static int ath_s_frees_4  = 0; 
 
 
 
 
 
#define   ATH_BUF_POOL_0_ID   0 
#define   ATH_BUF_POOL_1_ID   1 
#define   ATH_BUF_POOL_2_ID   2 
#define   ATH_BUF_POOL_3_ID   3 
#define   ATH_BUF_POOL_4_ID   4 
 
 
#define   ATH_BUF_POOL_0_SIZ  1552 
#define   ATH_BUF_POOL_1_SIZ  1024 
#define   ATH_BUF_POOL_2_SIZ  512 
#define   ATH_BUF_POOL_3_SIZ  256 
#define   ATH_BUF_POOL_4_SIZ  128 
 
#define  ATH_EXTRA_BYTE       128 
 
 
 
#define   ATH_BUF_POOL_0_NUM  32
#define   ATH_BUF_POOL_1_NUM  32 
#define   ATH_BUF_POOL_2_NUM  10 
#define   ATH_BUF_POOL_3_NUM  10 
#define   ATH_BUF_POOL_4_NUM  10 
 
typedef struct pbuf  
{ 
  q_link_type   link; 
	uint32        pool_id; 
	unsigned int  len; 
	unsigned int  max; 
  char*         data; 
	char*         start; 
} pbuf_hdr_type; 
 
typedef struct  
{ 
  q_link_type   link; 
	uint32        pool_id; 
	unsigned int  len; 
	unsigned int  max; 
	char*         data; 
	char*         start; 
  char          _data[ATH_BUF_POOL_0_SIZ + 2*ATH_EXTRA_BYTE]; 
} pbuf_0_type; 
 
 
 
typedef struct  
{ 
	q_link_type  link; 
	uint32       pool_id; 
	unsigned int len; 
	unsigned int max; 
	char*        data; 
	char*        start; 
	char         _data[ATH_BUF_POOL_1_SIZ + 2*ATH_EXTRA_BYTE]; 
} pbuf_1_type; 
 
 
typedef struct  
{ 
  q_link_type  link; 
	uint32       pool_id; 
	unsigned int len; 
	unsigned int max; 
	char*        data; 
	char*        start; 
	char         _data[ATH_BUF_POOL_2_SIZ + 2*ATH_EXTRA_BYTE]; 
} pbuf_2_type; 
 
 
typedef struct  
{ 
	q_link_type  link; 
	uint32       pool_id; 
	unsigned int len; 
	unsigned int max; 
	char*        data; 
	char*        start; 
	char         _data[ATH_BUF_POOL_3_SIZ + 2*ATH_EXTRA_BYTE]; 
} pbuf_3_type; 
 
 
 
typedef struct  
{ 
	q_link_type   link; 
	uint32        pool_id; 
	unsigned int  len; 
	unsigned int  max; 
	char*         data; 
	char*         start; 
	char          _data[ATH_BUF_POOL_4_SIZ + 2*ATH_EXTRA_BYTE]; 
} pbuf_4_type; 
 
 
 
STATIC  pbuf_0_type  ath_pbuf_0_pool[ATH_BUF_POOL_0_NUM] ; 
 
STATIC  pbuf_1_type  ath_pbuf_1_pool[ATH_BUF_POOL_1_NUM] ; 
 
STATIC  pbuf_2_type  ath_pbuf_2_pool[ATH_BUF_POOL_2_NUM] ; 
 
STATIC  pbuf_3_type  ath_pbuf_3_pool[ATH_BUF_POOL_3_NUM] ; 
 
STATIC  pbuf_4_type  ath_pbuf_4_pool[ATH_BUF_POOL_4_NUM] ; 
 
 
STATIC  q_type  ath_pbuf_0_free_q; 
 
STATIC  q_type  ath_pbuf_1_free_q; 
 
STATIC  q_type  ath_pbuf_2_free_q; 
 
STATIC  q_type  ath_pbuf_3_free_q; 
 
STATIC  q_type  ath_pbuf_4_free_q; 
 
 
void 
a_netbuf_init( void ) 
{ 
  int i;

  /*------------------------------------------------------------------------- 
     Initialize POOL 0 buffer  
  -------------------------------------------------------------------------*/ 
  (void) q_init( &ath_pbuf_0_free_q); 
 
  /* 
  **Place all available buffer on the ath_pbuf_0_free_q  
  */ 
  for (i = 0; i < ATH_BUF_POOL_0_NUM; i++ )  
  { 
    ath_pbuf_0_pool[i].pool_id  = ATH_BUF_POOL_0_ID; 
    q_put(&ath_pbuf_0_free_q, 
          q_link(&ath_pbuf_0_pool[i], 
		             &ath_pbuf_0_pool[i].link)); 
 
  } 
 
   
   /*------------------------------------------------------------------------- 
     Initialize POOL 1 buffer  
  -------------------------------------------------------------------------*/ 
  (void) q_init( &ath_pbuf_1_free_q); 
 
  /* 
  **Place all available buffer on the ath_pbuf_1_free_q  
  */ 
  for (i = 0; i < ATH_BUF_POOL_1_NUM ; i++ )  
  { 
    ath_pbuf_1_pool[i].pool_id  = ATH_BUF_POOL_1_ID; 
	  q_put(&ath_pbuf_1_free_q, 
          q_link(&ath_pbuf_1_pool[i], 
		             &ath_pbuf_1_pool[i].link)); 
  } 
 
 
   /*------------------------------------------------------------------------- 
     Initialize POOL 2 buffer  
  -------------------------------------------------------------------------*/ 
  (void) q_init( &ath_pbuf_2_free_q); 
 
  /* 
  **Place all available buffer on the ath_pbuf_2_free_q  
  */ 
  for (i = 0; i < ATH_BUF_POOL_2_NUM; i++ )  
  { 
    ath_pbuf_2_pool[i].pool_id  = ATH_BUF_POOL_2_ID; 
    q_put(&ath_pbuf_2_free_q, 
          q_link(&ath_pbuf_2_pool[i], 
		             &ath_pbuf_2_pool[i].link)); 
  } 
 
 
   /*------------------------------------------------------------------------- 
     Initialize POOL 3 buffer  
  -------------------------------------------------------------------------*/ 
  (void) q_init( &ath_pbuf_3_free_q); 
 
  /* 
  **Place all available buffer on the ath_pbuf_3_free_q  
  */ 
  for (i = 0; i < ATH_BUF_POOL_3_NUM; i++ )  
  { 
	ath_pbuf_3_pool[i].pool_id  = ATH_BUF_POOL_3_ID; 
        q_put(&ath_pbuf_3_free_q, 
          q_link(&ath_pbuf_3_pool[i], 
		             &ath_pbuf_3_pool[i].link)); 
  } 
 
 
 
   /*------------------------------------------------------------------------- 
     Initialize POOL 4 buffer  
  -------------------------------------------------------------------------*/ 
  (void) q_init( &ath_pbuf_4_free_q); 
 
  /* 
  **Place all available buffer on the ath_pbuf_4_free_q  
  */ 
  for (i = 0; i < ATH_BUF_POOL_4_NUM; i++ )  
  { 
    ath_pbuf_4_pool[i].pool_id  = ATH_BUF_POOL_4_ID; 
	q_put(&ath_pbuf_4_free_q, 
          q_link(&ath_pbuf_4_pool[i], 
		             &ath_pbuf_4_pool[i].link)); 
  } 
 
 
} /* a_netbuf_int() */ 
 
 
void * 
a_netbuf_alloc(int size) 
{ 
	pbuf_hdr_type *p = NULL; 
  uint32         pool_id  = ATH_BUF_POOL_4_SIZ; 
	int*           s_alloc_ptr = NULL; 
	int*           s_frees_ptr = NULL; 
 
	DPRINTF(DBG_NETBUF, (DBGFMT "Enter - size %d\n", DBGARG, size)); 
 
	/* 
	** Allocate a pbuf here 
	*/ 
	if (size <= ATH_BUF_POOL_4_SIZ ) 
	{ 
    p = (pbuf_hdr_type*) q_get(&ath_pbuf_4_free_q); 
		pool_id = ATH_BUF_POOL_4_ID; 
		s_alloc_ptr = &ath_s_allocs_4; 
		s_frees_ptr = &ath_s_frees_4; 
	} 
  else if (size <= ATH_BUF_POOL_3_SIZ ) 
	{ 
    p = (pbuf_hdr_type*) q_get(&ath_pbuf_3_free_q); 
		pool_id = ATH_BUF_POOL_3_ID; 
		s_alloc_ptr = &ath_s_allocs_3; 
		s_frees_ptr = &ath_s_frees_3; 
	} 
	else if (size <= ATH_BUF_POOL_2_SIZ ) 
	{ 
    p = (pbuf_hdr_type*) q_get(&ath_pbuf_2_free_q); 
		pool_id = ATH_BUF_POOL_2_ID; 
		s_alloc_ptr = &ath_s_allocs_2; 
		s_frees_ptr = &ath_s_frees_2; 
	} 
	else if (size <= ATH_BUF_POOL_1_SIZ ) 
	{ 
    p = (pbuf_hdr_type*) q_get(&ath_pbuf_1_free_q); 
		pool_id = ATH_BUF_POOL_1_ID; 
		s_alloc_ptr = &ath_s_allocs_1; 
		s_frees_ptr = &ath_s_frees_1; 
	} 
  else if (size <= ATH_BUF_POOL_0_SIZ ) 
	{ 
    p = (pbuf_hdr_type*) q_get(&ath_pbuf_0_free_q); 
		pool_id = ATH_BUF_POOL_0_ID; 
		s_alloc_ptr = &ath_s_allocs_0; 
		s_frees_ptr = &ath_s_frees_0; 
	} 
	else 
	{ 
     DPRINTF(DBG_NETBUF, (DBGFMT " Too big size to allocate", DBGARG )); 
	   ASSERT(0); 
	} 
 
	if(p == NULL) 
	{ 
		DPRINTF(DBG_NETBUF, (DBGFMT " POOL_ID : % d No memory available", DBGARG, 
			                 pool_id)); 
		return(NULL); 
	} 
 
	p->start = (char *) p + sizeof(pbuf_hdr_type); 
	p->data = p->start + ATH_EXTRA_BYTE; 
	p->len = 0; 
	p->max = size; 
 
	*s_alloc_ptr +=1; 
 
	DPRINTF(DBG_NETBUF, (DBGFMT "POOL_ID %d PBUF ALLOC 0x%08x %d %d\n", DBGARG, 
		pool_id, (unsigned int) p, (*s_alloc_ptr), ((*s_alloc_ptr) - (*s_frees_ptr)))); 
 
    return ((void *)p); 
} 
 
void 
a_netbuf_free(void *bufPtr) 
{ 
	pbuf_hdr_type  *p = NULL; 
  int*           s_alloc_ptr = NULL; 
	int*           s_frees_ptr = NULL; 
	q_type*        this_q = NULL; 
 
	DPRINTF(DBG_NETBUF, (DBGFMT "Enter\n", DBGARG)); 
 
  if ( NULL == bufPtr) 
	{ 
	   DPRINTF(DBG_NETBUF, (DBGFMT "Trying to free NULL pointer", DBGARG)); 
	   ASSERT(0); 
     return; 
	} 
 
	p  = (pbuf_hdr_type*) bufPtr; 
 
	switch(p->pool_id) 
	{ 
	  case ATH_BUF_POOL_0_ID: 
		{ 
      s_alloc_ptr = &ath_s_allocs_0; 
	    s_frees_ptr = &ath_s_frees_0; 
			this_q = &ath_pbuf_0_free_q; 
			break; 
		} 
 
		case ATH_BUF_POOL_1_ID: 
		{ 
      s_alloc_ptr = &ath_s_allocs_1; 
	    s_frees_ptr = &ath_s_frees_1; 
			this_q = &ath_pbuf_1_free_q; 
			break; 
		} 
 
		case ATH_BUF_POOL_2_ID: 
		{ 
      s_alloc_ptr = &ath_s_allocs_2; 
	    s_frees_ptr = &ath_s_frees_2; 
			this_q = &ath_pbuf_2_free_q; 
			break; 
		} 
 
		case ATH_BUF_POOL_3_ID: 
		{ 
      s_alloc_ptr = &ath_s_allocs_3; 
	    s_frees_ptr = &ath_s_frees_3; 
			this_q = &ath_pbuf_3_free_q; 
			break; 
		} 
 
		case ATH_BUF_POOL_4_ID: 
		{ 
      s_alloc_ptr = &ath_s_allocs_4; 
	    s_frees_ptr = &ath_s_frees_4; 
     	this_q = &ath_pbuf_4_free_q; 
			break; 
		} 
 
		default: 
		{ 
		   DPRINTF(DBG_NETBUF, (DBGFMT "Trying to free unknown net buff fomr POOL_ID %d",  
			        DBGARG, p->pool_id)); 
 
		   ASSERT(0); 
		} 
	} 
 
	(*s_frees_ptr)++; 
 
	DPRINTF(DBG_NETBUF, (DBGFMT "POOL_ID %d PBUF FREE 0x%08x %d %d\n", DBGARG, 
		      p->pool_id, (unsigned int) bufPtr, (*s_frees_ptr), 
          ((*s_alloc_ptr) - (*s_frees_ptr)))); 
 
	/* 
	** Return item to free list 
	*/ 
	q_put(this_q, &p->link); 
 
} /* a_net_buf_free() */ 
 
 
 
A_UINT32 
a_netbuf_to_len(void *bufPtr) 
{ 
	struct pbuf *p = ( struct pbuf *) bufPtr; 
 
	DPRINTF(DBG_NETBUF, (DBGFMT "Enter - len %d\n", DBGARG, p->len)); 
 
    return (p->len); 
} 
 
void * 
a_netbuf_to_data(void *bufPtr) 
{ 
	struct pbuf *p = ( struct pbuf *) bufPtr; 
 
	DPRINTF(DBG_NETBUF, 
		(DBGFMT "Enter - data 0x%08x\n", DBGARG, (unsigned int) (p->data))); 
 
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
 
	DPRINTF(DBG_NETBUF, (DBGFMT "Enter - len %d\n", DBGARG, len)); 
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
 
	DPRINTF(DBG_NETBUF, (DBGFMT "Enter - len %d %d\n", DBGARG, len, p->len)); 
 
  return A_OK; 
} 
 
/* 
 * Trim the network buffer pointed to by bufPtr to len # of bytes  
 */ 
A_STATUS 
a_netbuf_trim(void *bufPtr, A_INT32 len) 
{ 
	struct pbuf *p = (struct pbuf *) bufPtr; 
 
	DPRINTF(DBG_NETBUF, 
		(DBGFMT "Enter - len %d @ 0x%08x\n", DBGARG, len, (unsigned int) p)); 
 
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
 
	DPRINTF(DBG_NETBUF, (DBGFMT "Enter - headroom %d\n", DBGARG, room)); 
 
    return (room); 
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
 
	DPRINTF(DBG_NETBUF, (DBGFMT "Enter - len %d\n", DBGARG, len)); 
 
    return A_OK; 
} 
 
A_UINT32 
a_copy_to_user(void *to, const void *from, A_UINT32 n) 
{ 
	DPRINTF(DBG_NETBUF, (DBGFMT "Enter - len %d\n", DBGARG, n)); 
 
  return((A_UINT32) memcpy(to, from, n)); 
} 
 
A_UINT32 
a_copy_from_user(void *to, const void *from, A_UINT32 n) 
{ 
	DPRINTF(DBG_NETBUF, (DBGFMT "Enter - len %d\n", DBGARG, n)); 
 
  return((A_UINT32) memcpy(to, from, n)); 
}

void a_netbuf_enqueue(A_NETBUF_QUEUE_T *q, struct pbuf *pkt)
{
	A_MUTEX_LOCK(&q->lock);

#error FIX_ME_HENRI

	A_MUTEX_UNLOCK(&q->lock);

	return;
}

void a_netbuf_prequeue(A_NETBUF_QUEUE_T *q, struct pbuf *pkt)
{
	A_MUTEX_LOCK(&q->lock);

#error FIX_ME_HENRI

	A_MUTEX_UNLOCK(&q->lock);

	return;
}

void *a_netbuf_dequeue(A_NETBUF_QUEUE_T *q)
{
	A_MUTEX_LOCK(&q->lock);

#error FIX_ME_HENRI

	A_MUTEX_UNLOCK(&q->lock);

	return(NULL);
}

int a_netbuf_queue_size(A_NETBUF_QUEUE_T *q)
{
	A_MUTEX_LOCK(&q->lock);

#error FIX_ME_HENRI

	A_MUTEX_UNLOCK(&q->lock);

	return(0);
}

void a_netbuf_queue_init(A_NETBUF_QUEUE_T *q)
{
	A_MEMZERO(q, sizeof(A_NETBUF_QUEUE_T));

#error FIX_ME_HENRI

	return;
}


#endif
