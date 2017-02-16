/*
 * Copyright (c) 2004-2005 Atheros Communications Inc.
 * All rights reserved.
 *
 * $ATH_LICENSE_HOSTSDK0_C$
 *
 */

#ifndef _WLAN_ATH_OEM_H
#define _WLAN_ATH_OEM_H

int wlan_ath_start(void);
int wlan_ath_stop(void);
#ifdef CONFIG_HOST_TCMD_SUPPORT
int wlan_ath_test_start(void);
#endif
int wlan_ath_fw_upload(void);

#endif
