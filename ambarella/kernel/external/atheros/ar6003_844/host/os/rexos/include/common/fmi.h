/*
 * Copyright (c) 2004-2005 Atheros Communications Inc.
 * All rights reserved.
 *
 * $ATH_LICENSE_HOSTSDK0_C$
 *
 * FMI declarations and prototypes
 */

#ifndef _FMI_H_
#define _FMI_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Header files */
#include "athdefs.h"
#include "a_types.h"
#include "hif.h"
#include "a_osapi.h"

void FMIInit(void);

A_STATUS FMIReadFlash(HIF_DEVICE *device, A_UINT32 address, A_UCHAR *buffer,
	A_UINT32 length);

A_STATUS FMIWriteFlash(HIF_DEVICE *device, A_UINT32 address, A_UCHAR *buffer,
	A_UINT32 length);

A_STATUS FMIEraseFlash(HIF_DEVICE *device);

A_STATUS FMIPartialEraseFlash(HIF_DEVICE *device, A_UINT32 address,
	A_UINT32 length);

A_STATUS FMIPartInitFlash(HIF_DEVICE *device, A_UINT32 address);

A_STATUS FMIDone(HIF_DEVICE *device);

#ifdef __cplusplus
}
#endif

#endif
