/*
 * Copyright (c) 2004-2005 Atheros Communications Inc.
 * All rights reserved.
 *
 * $ATH_LICENSE_HOSTSDK0_C$
 *
 */

#ifndef _EMULATION_OSAPI_H_
#define _EMULATION_OSAPI_H_

#ifdef __cplusplus
extern "C" {
#endif

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

#ifndef min
#define min(x,y)    ((x)>(y)?(y):(x))
#endif

#ifndef max
#define max(x,y)    ((x)>(y)?(x):(y))
#endif

/* Linux/OS include files */
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <netinet/in.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/*
 * Endianes macros
 */
#define A_BE2CPU8(x)       ntohb(x)
#define A_BE2CPU16(x)      ntohs(x)
#define A_BE2CPU32(x)      ntohl(x)

#define A_LE2CPU8(x)       (x)
#define A_LE2CPU16(x)      (x)
#define A_LE2CPU32(x)      (x)

#define A_CPU2BE8(x)       htonb(x)
#define A_CPU2BE16(x)      htons(x)
#define A_CPU2BE32(x)      htonl(x)

/* file macros */
#define A_FL_RDONLY        O_RDONLY
#define A_FL_RDWR          O_RDWR
#define A_FL_CREATE        O_CREAT
#define A_FL_SEEK_SET      SEEK_SET

typedef int A_TIMER;

#define A_MEMCPY(dst, src, len)         memcpy((A_UINT8 *)(dst), (src), (len))
#define A_MEMZERO(addr, len)            memset(addr, 0, len)
#define A_MEMCMP(addr1, addr2, len)     memcmp((addr1), (addr2), (len))
#define A_MALLOC(size)                  qcom_malloc((size))
#define A_MALLOC_NOWAIT(size)           qcom_malloc((size))
#define A_FREE(addr)                    qcom_free(addr)
#define A_PRINTF(args...)               qcom_print(args)
#define A_HTONS(s)                      htons(s)

/* String functions */
#define A_SPRINTF(buf, args...)         sprintf(buf, args)
#define A_STRCMP(str1, str2)            strcmp(str1, str2)

/* Mutual Exclusion */
typedef pthread_mutex_t A_MUTEX_T;
#define A_MUTEX_INIT(mutex)             qcom_lock_init(mutex)
#define A_MUTEX_LOCK(mutex)             qcom_lock(mutex)
#define A_MUTEX_UNLOCK(mutex)           qcom_unlock(mutex)
#define A_IS_MUTEX_VALID(mutex)         TRUE  /* okay to return true, since A_MUTEX_DELETE does nothing */
#define A_MUTEX_DELETE(mutex)           /* nothing to free for pthreads */

/* Get current time in ms */
#define A_GET_MS(offset)    \
	qcom_get_ms((offset))

/*
 * Timer Functions
 */
#define A_MDELAY(msecs)                 qcom_delay(msecs)
#ifdef MATS
typedef struct timer_list               A_TIMER;
#endif

#define A_INIT_TIMER(pTimer, pFunction, pArg) do {              \
} while (0)

/*
 * Start a Timer that elapses after 'periodMSec' milli-seconds
 * Support is provided for a one-shot timer. The 'repeatFlag' is
 * ignored.
 */
#define A_TIMEOUT_MS(pTimer, periodMSec, repeatFlag) do {                   \
} while (0)

/*
 * Cancel the Timer. 
 */
#define A_UNTIMEOUT(pTimer) do {                                \
} while (0)

#define A_DELETE_TIMER(pTimer) do {                             \
} while (0)

#define A_TIMER_PENDING(pTimer)   FALSE

typedef struct { pthread_cond_t cond ; pthread_mutex_t mutex; } A_WAITQUEUE_HEAD;

/*
 * Wait Queue related functions
 */

#define A_INIT_WAITQUEUE_HEAD(head)	\
	qcom_cond_wait_init(head)

#define A_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(head, condition, timeout) do { \
	qcom_cond_wait(&head, condition, timeout);	\
} while (0)

#define A_WAKE_UP(head)	\
	qcom_cond_signal(head)

#ifdef DEBUG
#define A_ASSERT(expr)  \
    if (!(expr)) {   \
        printf( "\n" __FILE__ ":%d: Assertion " #expr " failed!\n",__LINE__); \
        exit((int) #expr); \
    }

#else
#define A_ASSERT(expr) \
    if (!(expr)) {   \
        DPRINTF(DBG_ERROR, ( DBGFMT "ASSERT %s:%d: Assertion " #expr " failed!\n", DBGARG, __FILE__, __LINE__)); \
    }
#endif /* DEBUG */


void *qcom_malloc(unsigned int size);
void qcom_free(void *addr);
int qcom_print(const char *format, ...);
int qcom_lock_init(A_MUTEX_T *mutex);
int qcom_lock(A_MUTEX_T *mutex);
int qcom_unlock(A_MUTEX_T *mutex);
void qcom_cond_wait_init(A_WAITQUEUE_HEAD *wait_channel);
void qcom_cond_wait(A_WAITQUEUE_HEAD *wait_channel, int condition, int timeout);
void qcom_cond_signal(A_WAITQUEUE_HEAD *wait_channel);
unsigned int qcom_get_ms(unsigned int offset);
void qcom_delay(unsigned int msecs);

struct pbuf
{
    struct pbuf *next;
    unsigned int len;
    unsigned int max;
    char *data;
    char *start;
};

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
#define A_NETBUF_INIT()\
    a_netbuf_init()
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

#define	A_FL_INITIALIZE(file, root, flags) \
	do_fl_initialize((file), (root), (flags))
#define	A_FL_UNINITIALIZE(h) \
	do_fl_uninitialize((h))
#define A_FL_SEEK(file, offset, whence) \
    do_fl_seek((file), (offset), (whence))
#define A_FL_GET_IMG_HDR(h, n, b, l)	\
	get_img_hdr((h), (n), (b), (l))
#define A_FL_GET_DATA(h, n, b, l)	\
	get_fl_data((h), (n), (b), (l))
#define A_FL_SET_DATA(h, n, b, l)    \
    set_fl_data((h), (n), (b), (l))
#define A_FL_GET_SIZE(file, root, size)   \
    get_fl_size(file, root, size)

void *do_fl_initialize(char* file, char* root, unsigned int flags);
void do_fl_uninitialize(void *handle);
int do_fl_seek(void* handle, unsigned int offset, int whence);
int get_img_hdr(void *handle, unsigned int flags, void* h, int reset);
int get_fl_data(void *handle, unsigned int flags, char *buff, int len);
int set_fl_data(void *handle, unsigned int flags, char *buff, int len);
A_STATUS get_fl_size(A_CHAR* file, A_CHAR* root, A_UINT32* size);

/* In RexOS, WLAN Rx and Tx run in same thread contexts, so WLAN Rx context needs to check
 * for any commands/data queued for WLAN */
void rex_check_drv_tx(void);

#define A_CHECK_DRV_TX()                     rex_check_drv_tx()

#define A_GET_CACHE_LINE_BYTES()    32  /* TODO: Rex runs on ARM processors */
     
static inline void *A_ALIGN_TO_CACHE_LINE(void *ptr) {   
    return (void *)(((A_UINT32)(ptr) + (A_GET_CACHE_LINE_BYTES() - 1)) & (~(A_GET_CACHE_LINE_BYTES() - 1)));
}

#ifdef __cplusplus
}
#endif

#endif
