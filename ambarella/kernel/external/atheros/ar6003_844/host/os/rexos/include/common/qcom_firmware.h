/*
 * Copyright (c) 2004-2005 Atheros Communications Inc.
 * All rights reserved.
 *
 * $ATH_LICENSE_HOSTSDK0_C$
 *
 */

#ifndef _QCOM_FIRMWARE_H_
#define _QCOM_FIRMWARE_H_

#ifdef __cplusplus
extern "C" {
#endif

#define FIRMWARE_IMAGE_NAME		"AR6000.img"
#define TST_FIRMWARE_IMAGE_NAME "AR6000-tcmd.img"

int qcom_fw_upload(AR_SOFTC_T *ar);

#ifdef CONFIG_HOST_TCMD_SUPPORT
int qcom_test_fw_upload(AR_SOFTC_T *ar);
#endif

#ifdef __cplusplus
}
#endif

#endif
