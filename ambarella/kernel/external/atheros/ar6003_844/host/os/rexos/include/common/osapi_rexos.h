/*==========================================================================

                           Q C O M _ O S A P I . H

DESCRIPTION
  Header of Module providing OS Abstraction wrappers of rex APIs to the Atheros
  modules.
   
EXTERNALIZED FUNCTIONS
  


Copyright (c) 2005 by QUALCOMM Incorporated.  All Rights Reserved.
===========================================================================*/

/*===========================================================================

                           EDIT HISTORY FOR FILE

$Author: gijo $   $Date: 2012/03/29 $
$Header: //depot/sw/releases/olca3.1-RC/host/os/rexos/include/common/osapi_rexos.h#3 $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
12/28/05   hba     Created Module

===========================================================================*/
#ifndef  _OSAPI_REXOS_H_
#define  _OSAPI_REXOS_H_

#include "a_config.h"

#ifdef ATHR_EMULATION
#include "emulation_osapi.h"
#else

#ifdef __cplusplus
extern "C" {
#endif

#define __ATTRIB_PACK
#define __ATTRIB_PRINTF
#define __ATTRIB_NORETURN
#ifndef INLINE
#define INLINE                  __inline
#define PREPACK					PACKED
#define POSTPACK
#endif

/*---------------------------------------------------------------------------
	QCOM specific include 
----------------------------------------------------------------------------*/
#include "customer.h"


#ifdef FEATURE_WLAN
#ifdef FEATURE_WLAN_ATH

#include "comdef.h"
#include "rex.h"
#include "assert.h"


#ifndef min
#define min(x,y)    ((x)>(y)?(y):(x))
#endif

#ifndef max
#define max(x,y)    ((x)>(y)?(x):(y))
#endif

#define A_NOT_IMPLEMENTED(func)\
do\
{\
  MSG_HIGH("Function %s not implemented", #func, 0, 0);\
  ASSERT(0);\
} while(0)


/*---------------------------------------------------------------------------
   Endianes macros
----------------------------------------------------------------------------*/
#define A_BE2CPU8(x)       (x)
#define A_BE2CPU16(x)      ((uint16)((((uint16)(x) & 0x00ff) << 8) | \
                            (((uint16)(x) & 0xff00) >> 8)))

#define A_BE2CPU32(x)      ((uint32)((((uint32)(x) & 0x000000ffU) << 24) | \
                            (((uint32)(x) & 0x0000ff00U) <<  8) | \
                            (((uint32)(x) & 0x00ff0000U) >>  8) | \
                            (((uint32)(x) & 0xff000000U) >> 24)))

#define A_LE2CPU8(x)       (x)
#define A_LE2CPU16(x)      ((((uint8*)(&x))[0]) | ((((uint8*)(&x))[1]) << 8));
#define A_LE2CPU32(x)      (x)

#define A_CPU2BE8(x)       (x)
#define A_CPU2BE16(x)      A_BE2CPU16(x)
#define A_CPU2BE32(x)      A_BE2CPU32(x)

#define A_HTONS(s)         A_BE2CPU16(s)

/*----------------------------------------------------------------------------
 Timer Management
-----------------------------------------------------------------------------*/
typedef rex_timer_type     A_TIMER;

/* 
**	Get current time in ms 
*/
#define A_GET_MS(offset)   wlan_ath_osapi_get_ms(offset)

/*
** Initialize a timer
*/
#define A_INIT_TIMER(pTimer, pFunction, pArg)                  \
do {                                                           \
    rex_def_timer_ex(pTimer, pFunction, (unsigned long)pArg);  \
} while (0)

/*
 * Start a Timer that elapses after 'periodMSec' milli-seconds
 * Support is provided for a one-shot timer. The 'repeatFlag' is
 * ignored.
 */
#define A_TIMEOUT_MS(pTimer, periodMSec, repeatFlag)           \
do {                                                           \
    if (!repeatFlag)                                           \
    {                                                          \
      rex_set_timer(pTimer, periodMSec);                       \
    }                                                          \
    else                                                       \
    {                                                          \
      A_NOT_IMPLEMENTED(TIMEOUT_MS);                           \
    }                                                          \
} while (0)

/*
 * Cancel the Timer. 
 */
#define A_UNTIMEOUT(pTimer)                                    \
do {                                                           \
    rex_clr_timer(pTimer);                                     \
} while (0)

#define A_DELETE_TIMER(pTimer)                                 \
do {                                                           \
    /* A_NOT_IMPLEMENTED( DELETE_TIMER); */                    \
    MSG_ERROR("Delete Timer Called", 0, 0,0);                  \
} while (0)

#define A_MDELAY(msecs)

/*----------------------------------------------------------------------------
Memory Management.
-----------------------------------------------------------------------------*/
#define A_MEMCPY(dst, src, len)         memcpy((void *)(dst), (void*)(src), (len))
#define A_MEMZERO(addr, len)            memset((void*) addr, 0, len)
#define A_MEMCMP(addr1, addr2, len)     memcmp((void*)(addr1), (void*)(addr2), (len))

#define A_MALLOC(size)                  wlan_ath_osapi_malloc((size))
#define A_MALLOC_NOWAIT(size)           wlan_ath_osapi_malloc((size))
#define A_FREE(addr)                    wlan_ath_osapi_free(addr)

/*----------------------------------------------------------------------------
 Logging
-----------------------------------------------------------------------------*/
#define A_PRINTF(...)              



/*----------------------------------------------------------------------------
  Mutual Exclusion 
-----------------------------------------------------------------------------*/
typedef rex_crit_sect_type              A_MUTEX_T;
#define A_MUTEX_INIT(mutex)             rex_init_crit_sect(mutex)
#define A_MUTEX_LOCK(mutex)             rex_enter_crit_sect(mutex)
#define A_MUTEX_UNLOCK(mutex)           rex_leave_crit_sect(mutex)

/*----------------------------------------------------------------------------
  File I/O API
-----------------------------------------------------------------------------*/
#define	A_FW_INITIALIZE(file, root)
#define	A_FW_UNINITIALIZE(h)
#define A_FW_GET_IMG_HDR(h, n, b, l)
#define A_FW_GET_IMG_DATA(w, n, b, l)
#define A_FW_GET_IMG_SIZE(file, root)

/*-----------------------------------------------------------------------------
** Wait Queue related functions
-------------------------------------------------------------------------------*/
#define A_INIT_WAITQUEUE_HEAD(head)	                                       \
	       A_NOT_IMPLEMENTED(THREAD_INIT_WAIT_QUEUE)

#define A_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(head, condition, timeout)       \
do {	                                                                   \
	       A_NOT_IMPLEMENTED(THREAD_WAIT_EVENT_INT_TIMEOUT)	               \
} while (0)

#define A_WAKE_UP(head)	\
	       A_NOT_IMPLEMENTED(THREAD_WAKE_UP)

#ifdef DEBUG
#define A_ASSERT(expr)  ASSERT(expr)
#else
#define A_ASSERT(expr)
#endif /* DEBUG */

typedef struct netbuf_queue
{
    void *head;
    void *tail;
    A_MUTEX_T lock;
    int size;
} A_NETBUF_QUEUE_T;

#define A_NETBUF_ENQUEUE(q, pkt) \
    a_netbuf_enqueue((q), (pkt))
#define A_NETBUF_PREQUEUE(q, pkt) \
    a_netbuf_prequeue((q), (pkt))
#define A_NETBUF_DEQUEUE(q) \
    ((void *) a_netbuf_dequeue(q))
#define A_NETBUF_QUEUE_SIZE(q)  \
    a_netbuf_queue_size(q)
#define A_NETBUF_INIT()\
    a_netbuf_init()
#define A_NETBUF_QUEUE_INIT(q)  \
    a_netbuf_queue_init(q)
#define A_NETBUF_ALLOC(size) \
    a_netbuf_alloc(size)
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
#define A_NETBUF_TRIM(bufPtr,len)\
    a_netbuf_trim(bufPtr, len)
#define A_NETBUF_PULL(bufPtr, len) \
    a_netbuf_pull(bufPtr, len)
#define A_NETBUF_HEADROOM(bufPtr)\
    a_netbuf_headroom(bufPtr)

/* Add data to end of a buffer  */
#define A_NETBUF_PUT_DATA(bufPtr, srcPtr,  len) \
	A_NOT_IMPLEMENTED(A_NETBUF_PUT_DATA)

/* Add data to start of the  buffer */
#define A_NETBUF_PUSH_DATA(bufPtr, srcPtr,  len) \
	A_NOT_IMPLEMENTED(A_NETBUF_PUSH_DATA)

/* Remove data at start of the buffer */
#define A_NETBUF_PULL_DATA(bufPtr, dstPtr, len) \
	A_NOT_IMPLEMENTED(A_NETBUF_PULL_DATA)

/* Remove data from the end of the buffer */
#define A_NETBUF_TRIM_DATA(bufPtr, dstPtr, len) \
	A_NOT_IMPLEMENTED(A_NETBUF_TRIM_DATA)

/* View data as "size" contiguous bytes of type "t" */
#define A_NETBUF_VIEW_DATA(bufPtr, t, size) \
	A_NOT_IMPLEMENTED(A_NETBUF_VIEW_DATA)

/*
 Utility functions
--------------------------------------------------------------------------------*/
typedef void* (wlan_ath_thread_start_func_type)(void* param);

void         wlan_ath_osapi_create_thread
             (
                rex_tcb_type** task_ptr, 
								void* param, 
								wlan_ath_thread_start_func_type start_func, 
								void* start_param
             );

void         wlan_ath_osapi_init(void);
void*        wlan_ath_osapi_malloc(unsigned int size);
void         wlan_ath_osapi_free(void *addr);
int          wlan_ath_osapi_print(const char *format, ...);
unsigned int wlan_ath_osapi_get_ms(unsigned int offset);


/*
 * OS specific network buffer acess routines
 */

void a_netbuf_init(void);
void *a_netbuf_alloc(int size);
void a_netbuf_free(void *bufPtr);
void *a_netbuf_to_data(void *bufPtr);
A_UINT32 a_netbuf_to_len(void *bufPtr);
A_STATUS a_netbuf_push(void *bufPtr, A_INT32 len);
A_STATUS a_netbuf_put(void *bufPtr, A_INT32 len);
A_STATUS a_netbuf_trim(void *bufPtr, A_INT32 len);
A_INT32 a_netbuf_headroom(void *bufPtr);
A_STATUS a_netbuf_pull(void *bufPtr, A_INT32 len);
A_STATUS a_netbuf_put_data(void *bufPtr, void *srcPtr, A_INT32 len);
A_UINT32 a_copy_to_user(void *to, const void *from, A_UINT32 n);
A_UINT32 a_copy_from_user(void *to, const void *from, A_UINT32 n);
void a_netbuf_enqueue(A_NETBUF_QUEUE_T *q, void *bufPtr);
void a_netbuf_prequeue(A_NETBUF_QUEUE_T *q, void *bufPtr);
struct void *a_netbuf_dequeue(A_NETBUF_QUEUE_T *q);
int a_netbuf_queue_size(A_NETBUF_QUEUE_T *q);
void a_netbuf_queue_init(A_NETBUF_QUEUE_T *q);

/* In RexOS, WLAN Rx and Tx run in same thread contexts, so WLAN Rx context needs to check
 * for any commands/data queued for WLAN */
void rex_check_drv_tx(void);

#define A_CHECK_DRV_TX()                     rex_check_drv_tx()

#define A_GET_CACHE_LINE_BYTES()    32  /* TODO: Rex runs on ARM processors */
     
#define A_CACHE_LINE_PAD            128

static inline void *A_ALIGN_TO_CACHE_LINE(void *ptr) {   
    return (void *)(((A_UINT32)(ptr) + (A_GET_CACHE_LINE_BYTES() - 1)) & (~(A_GET_CACHE_LINE_BYTES() - 1)))
}

#endif /* FEATURE_WLAN_ATH */
#endif /* FEATURE_WLAN     */

#ifdef __cplusplus
}
#endif

#endif /* ATHR_EMULATION */

#endif /* _OSAPI_REXOS_H_ */
