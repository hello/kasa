/*
 * Copyright (c) 2004-2006 Atheros Communications Inc.
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

#ifndef _DEBUG_WIN_H_
#define _DEBUG_WIN_H_

//#define DEBUG

#ifdef DEBUG
#define ATH_PRINTX_ARG(arg) arg
//#define AR_DEBUG_PRINTF(mask, args) do {        \
//    if (GET_ATH_MODULE_DEBUG_VAR_MASK(ATH_MODULE_NAME) & (mask)) {                    \
//        A_PRINTF(args);    \
//    }                                            \
//} while (0)

//#define AR_DEBUG_PRINTF(mask, args) do {        \
//    A_PRINTF(ATH_PRINTX_ARG args);     \
//} while (0)

#define AR_DEBUG_PRINTF(mask, args) do {        \
        printf args;    \
} while (0)

#else
#define AR_DEBUG_PRINTF(flags, args)
#endif

    /* compile specific macro to get the function name string */
#define _A_FUNCNAME_  __FUNCTION__


#endif /* _DEBUG_WIN_H_ */
