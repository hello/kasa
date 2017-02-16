/*
 * Copyright (c) 2004-2005 Atheros Communications Inc.
 * All rights reserved.
 *
 * $ATH_LICENSE_HOSTSDK0_C$
 *
 */

#ifndef _CONFIG_REXOS_H_
#define _CONFIG_REXOS_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef QCOM
#define QCOM
#endif

#ifndef ATHR_EMULATION
#ifndef QCOM_INTEGRATION
#define QCOM_INTEGRATION
#endif
#endif

#ifndef SDIO
#define SDIO
#endif

#undef CONFIG_HOST_GPIO_SUPPORT
#undef  CONFIG_TARGET_PROFILE_SUPPORT
#define CONFIG_HOST_TCMD_SUPPORT

#define RSSI_QUERY_PERIOD    1000

#undef USE_4BYTE_REGISTER_ACCESS
#undef EMBEDDED_AR6K_FIRMWARE

#define FL_NULL             ((void *) -1)

#ifdef __cplusplus
}
#endif

#endif
