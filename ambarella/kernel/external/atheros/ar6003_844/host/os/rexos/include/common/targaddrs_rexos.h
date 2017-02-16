/*
 * Copyright (c) 2004-2005 Atheros Communications Inc.
 * All rights reserved.
 *
 * $ATH_LICENSE_HOSTSDK0_C$
 *
 */

#ifndef _TARGADDRS_REXOS_H_
#define _TARGADDRS_REXOS_H_

#ifdef __cplusplus
extern "C" {
#endif

A_STATUS eeprom_ar6000_transfer (HIF_DEVICE *device,
                                 A_UINT32 TargetType,
                                 A_CHAR *fileRoot,
                                 A_CHAR *eepromFile);

A_STATUS early_init_ar6000 (HIF_DEVICE *hifDevice,
                           A_UINT32    TargetType,
                           A_UINT32    TargetVersion);

A_STATUS configure_ar6000(HIF_DEVICE *hifDevice,
                          A_UINT32    TargetType,
                          A_UINT32    TargetVersion,
                          A_BOOL      enableUART,
                          A_BOOL      timerWAR,
                          A_UINT32    clkFreq,
                          A_CHAR      *fileName,
                          A_CHAR      *fileRoot,
                          A_BOOL      bCompressed,
                          A_BOOL      isColdBoot,
                          A_CHAR      *eepromFile);

void ar6000_set_softmac_addr(A_UCHAR* p);

void config_exit(void);

#ifdef __cplusplus
}
#endif

#endif
