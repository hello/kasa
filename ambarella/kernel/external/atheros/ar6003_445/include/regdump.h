//------------------------------------------------------------------------------
// <copyright file="regdump.h" company="Atheros">
//    Copyright (c) 2004-2010 Atheros Corporation.  All rights reserved.
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

#ifndef __REGDUMP_H__
#define __REGDUMP_H__

#ifndef ATH_TARGET
#include "athstartpack.h"
#endif

#if defined(AR6001)
#include "AR6001/AR6001_regdump.h"
#endif
#if defined(AR6002)
#include "AR6002/AR6002_regdump.h"
#endif

#if !defined(__ASSEMBLER__)
/*
 * Target CPU state at the time of failure is reflected
 * in a register dump, which the Host can fetch through
 * the diagnostic window.
 */
PREPACK struct register_dump_s {
    A_UINT32 target_id;               /* Target ID */
    A_UINT32 assline;                 /* Line number (if assertion failure) */
    A_UINT32 pc;                      /* Program Counter at time of exception */
    A_UINT32 badvaddr;                /* Virtual address causing exception */
    CPU_exception_frame_t exc_frame;  /* CPU-specific exception info */

    /* Could copy top of stack here, too.... */
} POSTPACK;
#endif /* __ASSEMBLER__ */

#ifndef ATH_TARGET
#include "athendpack.h"
#endif

#endif /* __REGDUMP_H__ */
