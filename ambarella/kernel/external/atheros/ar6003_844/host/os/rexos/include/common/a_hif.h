/*
 * Copyright (c) 2007 Atheros Communications Inc.
 * All rights reserved.
 *
 * $ATH_LICENSE_HOSTSDK0_C$
 *
 */

#ifndef _A_HIF_H_
#define _A_HIF_H_

#ifdef __cplusplus
extern "C" {
#endif

int HIFHandleInserted(void);
void HIFHandleRemoved(void);
void HIFHandleIRQ(void);
void HIFHandleRW(void);

#ifdef __cplusplus
}
#endif

#endif
