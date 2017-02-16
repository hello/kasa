//------------------------------------------------------------------------------
// Copyright (c) 2004-2010 Atheros Communications Inc.
// All rights reserved.
//
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
// 	Wifi driver for AR6002
// </summary>
//
//
// Author(s): ="Atheros"
//------------------------------------------------------------------------------

#ifndef _DEBUG_LINUX_H_
#define _DEBUG_LINUX_H_

    /* macro to remove parens */
#define ATH_PRINTX_ARG(arg...) arg

#ifdef DEBUG
    /* NOTE: the AR_DEBUG_PRINTF macro is defined here to handle special handling of variable arg macros
     * which may be compiler dependent. */
#define AR_DEBUG_PRINTF(mask, args) do {        \
    if (GET_ATH_MODULE_DEBUG_VAR_MASK(ATH_MODULE_NAME) & (mask)) {                    \
        A_LOGGER(mask, ATH_MODULE_NAME, ATH_PRINTX_ARG args);    \
    }                                            \
} while (0)
#else
    /* on non-debug builds, keep in error and warning messages in the driver, all other
     * message tracing will get compiled out */
#define AR_DEBUG_PRINTF(mask, args) \
    if ((mask) & (ATH_DEBUG_ERR | ATH_DEBUG_WARN)) { A_PRINTF(ATH_PRINTX_ARG args); }

#endif

    /* compile specific macro to get the function name string */
#define _A_FUNCNAME_  __func__


#endif /* _DEBUG_LINUX_H_ */
