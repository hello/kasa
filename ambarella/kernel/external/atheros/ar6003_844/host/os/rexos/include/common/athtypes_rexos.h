/*
 * Copyright (c) 2004-2005 Atheros Communications Inc.
 * All rights reserved.
 *
 * $ATH_LICENSE_HOSTSDK0_C$
 *
 */

#ifndef _ATHTYPES_REXOS_H_
#define _ATHTYPES_REXOS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* This doesn't really belong here - MATS */
/* We need it since all files do not include osapi.h yet */
#include "a_config.h"

typedef char            A_INT8;
typedef short           A_INT16;
typedef int             A_INT32;
typedef signed long long A_INT64;

typedef unsigned char   A_UINT8;
typedef unsigned short  A_UINT16;
typedef unsigned int    A_UINT32;
typedef unsigned long long A_UINT64;

typedef int             A_BOOL;
typedef char            A_CHAR;
typedef unsigned char   A_UCHAR;
typedef void*           A_ATH_TIMER;

#ifdef __cplusplus
}
#endif

#endif
