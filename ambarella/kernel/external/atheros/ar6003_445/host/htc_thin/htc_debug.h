//------------------------------------------------------------------------------
// <copyright file="htc_debug.h" company="Atheros">
//    Copyright (c) 2007-2008 Atheros Corporation.  All rights reserved.
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
//------------------------------------------------------------------------------
//==============================================================================
// Author(s): ="Atheros"
//==============================================================================
#ifndef HTC_DEBUG_H_
#define HTC_DEBUG_H_

#define ATH_MODULE_NAME htc
#include "a_debug.h"

/* ------- Debug related stuff ------- */

#define  ATH_DEBUG_SEND ATH_DEBUG_MAKE_MODULE_MASK(0)
#define  ATH_DEBUG_RECV ATH_DEBUG_MAKE_MODULE_MASK(1)
#define  ATH_DEBUG_SYNC ATH_DEBUG_MAKE_MODULE_MASK(2)
#define  ATH_DEBUG_DUMP ATH_DEBUG_MAKE_MODULE_MASK(3)
#define  ATH_DEBUG_IRQ  ATH_DEBUG_MAKE_MODULE_MASK(4)


#endif /*HTC_DEBUG_H_*/
