/*
 * Copyright (c) 2004-2005 Atheros Communications Inc.
 * All rights reserved.
 *
 * $ATH_LICENSE_HOSTSDK0_C$
 *
 */

#ifndef _DEBUG_REXOS_H
#define _DEBUG_REXOS_H

#define DBG_SIMULATOR	ATH_DEBUG_MAKE_MODULE_MASK(0)
#define DBG_WLAN_ATH	ATH_DEBUG_MAKE_MODULE_MASK(1)
#define DBG_OSAPI		ATH_DEBUG_MAKE_MODULE_MASK(2)
#define DBG_NETBUF	    ATH_DEBUG_MAKE_MODULE_MASK(3)
#define DBG_FIRMWARE	ATH_DEBUG_MAKE_MODULE_MASK(4)
#define DBG_LOG         ATH_DEBUG_MAKE_MODULE_MASK(5)
#define DBG_FMI         ATH_DEBUG_MAKE_MODULE_MASK(6)
#define DBG_HTC         ATH_DEBUG_MAKE_MODULE_MASK(7)
#define DBG_WMI         ATH_DEBUG_MAKE_MODULE_MASK(8)
#define DBG_DRIVER      ATH_DEBUG_MAKE_MODULE_MASK(9)
#define DBG_INFO        ATH_DEBUG_MAKE_MODULE_MASK(10)
#define DBG_ERROR       ATH_DEBUG_MAKE_MODULE_MASK(11)
#define DBG_WARNING     ATH_DEBUG_MAKE_MODULE_MASK(12)
#define DBG_SDIO        ATH_DEBUG_MAKE_MODULE_MASK(13)
#define DBG_HIF         ATH_DEBUG_MAKE_MODULE_MASK(14)
#define DBG_WMI2        DBG_WMI
    
#ifdef ATHR_EMULATION
/* Linux/OS include files */
#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/types.h>
#include <linux/unistd.h>

extern unsigned int g_dbg_flags;
extern pthread_mutex_t g_dbg_lock;

#define DBGTIME				dbg_time()
#define GETID				(unsigned int)(pthread_self() & 0x0000ffff)

char *dbg_time(void);

#define DBGFMT              stderr,"%s-%04x-%s() : "
#define DBGFMT_NOARG        stderr,
#define DBGARG              DBGTIME,GETID,__func__
#define DBGFN	            fprintf	

#define DPRINTF(f, a) \
	if(g_dbg_flags & (f)) \
	{	\
		DBGFN a ; \
	}

#else // ATHR_EMULATION
#include <stdio.h>
#include <stdarg.h>

extern unsigned int g_dbg_flags;

#define DBGFMT               "%s() : "
#define DBGFMT_NOARG         ""
#define DBGARG               __func__
#define DBGFN                wlan_ath_osapi_print

#define DPRINTF(f, a)\
	if(g_dbg_flags & (f)) \
	{	\
		DBGFN a ; \
	}

#endif

#ifdef DEBUG
#define PRINTX_ARG(arg...) arg

#define AR_DEBUG_LVL_CHECK(lvl)         (g_dbg_flags & (lvl))

#define AR_DEBUG_PRINTBUF(buffer, length, desc) do {   \
    if (AR_DEBUG_LVL_CHECK(ATH_DEBUG_DUMP)) {          \
        DebugDumpBytes(buffer, length, desc);          \
    }                                                  \
} while(0)

#define AR_DEBUG_PRINTF(flags, args)    do {  \
    if (AR_DEBUG_LVL_CHECK(flags)) {          \
        DBGFN (DBGFMT_NOARG PRINTX_ARG args); \
    }                                         \
} while (0)

#else
#define AR_DEBUG_LVL_CHECK(lvl)         0
#define AR_DEBUG_PRINTBUF(buffer, length, desc)
#define AR_DEBUG_PRINTF(flags, args)
#endif

#define A_DPRINTF                       DPRINTF

#endif // _DEBUG_REXOS_H
