//------------------------------------------------------------------------------
// Copyright (c) 2006-2010 Atheros Corporation.  All rights reserved.
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

#ifndef __AR6002_REGDUMP_H__
#define __AR6002_REGDUMP_H__

#if !defined(__ASSEMBLER__)
/*
 * XTensa CPU state
 * This must match the state saved by the target exception handler.
 */
struct XTensa_exception_frame_s {
    A_UINT32 xt_pc;
    A_UINT32 xt_ps;
    A_UINT32 xt_sar;
    A_UINT32 xt_vpri;
    A_UINT32 xt_a2;
    A_UINT32 xt_a3;
    A_UINT32 xt_a4;
    A_UINT32 xt_a5;
    A_UINT32 xt_exccause;
    A_UINT32 xt_lcount;
    A_UINT32 xt_lbeg;
    A_UINT32 xt_lend;

    A_UINT32 epc1, epc2, epc3, epc4;

    /* Extra info to simplify post-mortem stack walkback */
#define AR6002_REGDUMP_FRAMES 10
    struct {
        A_UINT32 a0;  /* pc */
        A_UINT32 a1;  /* sp */
        A_UINT32 a2;
        A_UINT32 a3;
    } wb[AR6002_REGDUMP_FRAMES];
};
typedef struct XTensa_exception_frame_s CPU_exception_frame_t; 
#define RD_SIZE sizeof(CPU_exception_frame_t)

#endif
#endif /* __AR6002_REGDUMP_H__ */
