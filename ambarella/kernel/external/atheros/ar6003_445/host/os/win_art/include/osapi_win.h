/*
 * $Id: //depot/sw/releases/olca3.1-RC/host/os/win_art/include/osapi_win.h#1 $
 *
 * This file contains the definitions of the basic atheros data types.
 * It is used to map the data types in atheros files to a platform specific
 * type.
 *
 * Copyright 2009 Atheros Communications, Inc.,  All Rights Reserved.
 */

#ifndef _OSAPI_WIN_H_
#define _OSAPI_WIN_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <windows.h>
#include <mmsystem.h>

#include <winbase.h>
#include "athdefs.h"
#include "athtypes_win.h"
#include "MyMalloc.h"

#ifdef __GNUC__
#define __ATTRIB_PACK           __attribute__ ((packed))
#define __ATTRIB_PRINTF         __attribute__ ((format (printf, 1, 2)))
#define __ATTRIB_NORETURN       __attribute__ ((noreturn))
#ifndef INLINE
#define INLINE                  __inline__
#endif
#define PREPACK
#define POSTPACK				__ATTRIB_PACK
#else /* Not GCC */
#define __ATTRIB_PACK
#define __ATTRIB_PRINTF
#define __ATTRIB_NORETURN
#ifndef INLINE
#define INLINE                  __inline
#endif
#define PREPACK
#define POSTPACK
#endif /* End __GNUC__ */

/*
 * Atomic operations
 */

typedef unsigned int A_ATOMIC_T;

#define A_ATOMIC_SET(counter, val) (*counter)=val
#define A_ATOMIC_INC(counter) InterlockedIncrement(counter)
#define A_ATOMIC_DEC(counter) InterlockedDecrement(counter)
#define A_ATOMIC_DEC_AND_TEST(counter) (!(InterlockedDecrement(counter)))
#define A_ATOMIC_CNT(counter) (*counter)

void
logPrintf(A_INT32 mask, A_CHAR * format, ...);

void
logInit();

void
dbgIntialize (A_UINT32 dbgPrintMask);
/*
 * Mutual Exclusion
 */
typedef CRITICAL_SECTION A_MUTEX_T;

#define A_MUTEX_INIT(_nt)      		InitializeCriticalSection(_nt)
#define A_MUTEX_DESTROY(_nt)   		DeleteCriticalSection(_nt)
#define A_MUTEX_LOCK(_nt)      		EnterCriticalSection(_nt)
#define A_MUTEX_UNLOCK(_nt)    		LeaveCriticalSection(_nt)
#define A_MUTEX_LOCK_BH(_nt)   		EnterCriticalSection(_nt)
#define A_MUTEX_UNLOCK_BH(_nt) 		LeaveCriticalSection(_nt)
#define A_MUTEX_LOCK_ASSERT(_nt)
#define A_IS_MUTEX_VALID(_nt)     	TRUE  /* okay to return true, since A_MUTEX_DELETE does nothing */
#define A_MUTEX_DELETE(_nt)       	DeleteCriticalSection(_nt) /* spin locks are not kernel resources so nothing to free.. */


#define A_MEMZERO(addr,len) 		memset(addr, 0, len)
#define A_MEMCPY(dst, src, len) 	memcpy(dst, src, len)
#define A_MEMCMP(dst, src, len) 	memcmp(dst, src, len)
//#define A_PRINTF(args...)           printf(args)
//#define A_SPRINTF(buf, args...)		sprintf (buf, args)
#define A_PRINTF					printf
#define A_SPRINTF					sprintf

/*
 * Malloc macros
 */
///// see MyMalloc.h
//#define A_MALLOC(size)  malloc(size)
//#define A_MALLOC_NOWAIT(size) malloc(size)
//#define A_FREE(addr) free(addr)

/*
 * Endian Macros
 */
#define A_BE2CPU8(x)       (x)
#define A_BE2CPU16(x)      ntohs(x)
#define A_BE2CPU32(x)      ntohl(x)

#define A_LE2CPU8(x)       (x)
#define A_LE2CPU16(x)      (x)
#define A_LE2CPU32(x)      (x)

#define A_CPU2BE8(x)       (x)
#define A_CPU2BE16(x)      htons(x)
#define A_CPU2BE32(x)      htonl(x)

/*
 * Timer Functions
 */
#define A_MDELAY(msecs)     Sleep(msecs)

/* Get current time in ms adding a constant offset (in ms) */
#define A_GET_MS(offset)    (GetTickCount() + offset)

typedef void (*A_TIMER_FN) (A_ATH_TIMER arg);

typedef struct a_timer_t {
    MMRESULT    	timerId;
    A_TIMER_FN  	callback;
    A_ATH_TIMER     arg;
} A_TIMER;


extern A_STATUS
ar6000_init_timer(A_TIMER *pTimer, A_TIMER_FN callback, A_ATH_TIMER arg);

extern void
ar6000_set_timer(A_TIMER *pTimer, unsigned long timeout, A_INT32 periodic);

extern A_STATUS
ar6000_cancel_timer(A_TIMER *pTimer);

extern A_STATUS
ar6000_delete_timer(A_TIMER *pTimer);

extern A_UINT32
ar6000_ms_tickget();

#define A_INIT_TIMER ar6000_init_timer

#define A_TIMEOUT_MS ar6000_set_timer

#define A_UNTIMEOUT  ar6000_cancel_timer

#define A_DELETE_TIMER ar6000_delete_timer

#define A_MS_TICKGET  ar6000_ms_tickget

/*
 * Wait Queue related functions
 */
typedef HANDLE A_WAITQUEUE_HEAD;

#define A_INIT_WAITQUEUE_HEAD(pHandle)  do { \
    *pHandle = CreateEvent(NULL,FALSE,FALSE,NULL); \
} while (0)

#define A_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(pHandle, condition, timeout) do { \
    if (!(condition)) { \
        WaitForSingleObject(*pHandle,timeout); \
    } \
} while (0)

#define A_WAKE_UP(pHandle) SetEvent(*pHandle)

#define A_DELETE_WAITQUEUE_HEAD(pHandle)  CloseHandle(*pHandle)

#define A_RESET_WAIT_EVENT(pHandle) ResetEvent(*pHandle)

/* In WM/WinCE, WLAN Rx and Tx run in different contexts, so no need to check
 * for any commands/data queued for WLAN */
#define A_CHECK_DRV_TX()


#define A_GET_CACHE_LINE_BYTES()    32  /* TODO: figure this out progammatically if possible */

#define A_CACHE_LINE_PAD            128
     
static INLINE void *A_ALIGN_TO_CACHE_LINE(void *ptr) {   
    return (void *)(((A_UINT32)(ptr) + (A_GET_CACHE_LINE_BYTES() - 1)) & (~(A_GET_CACHE_LINE_BYTES() - 1)));
}

#ifndef A_ASSERT
#ifdef DEBUG
#define A_ASSERT(expr)  \
    if (!(expr)) {   \
        A_PRINTF("Debug Assert Caught, File %s, Line: %d, Test:%s \n",__FILE__, __LINE__,#expr); \
    }
#else
#define A_ASSERT(expr)
#endif
#endif


//void A_DbgPrintf(A_CHAR * format, ...);
//#define A_PRINTF       A_DbgPrintf

#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif /* _OSAPI_WIN_H_ */
