// Copyright (c) 2010 Atheros Communications Inc.
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
//
//

/*
 * This is the header file for Interrupt handling of the 
 * Anwi driver
 */
#ifndef __ANWIISR_H__
#define __ANWIISR_H__

#include "ntddk.h"

BOOLEAN AnwiInterruptHandler(PKINTERRUPT Interrupt, PVOID Context);

#endif /* __ANWI_ISR_H__ */
