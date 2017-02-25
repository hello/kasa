##
## kernel/external/broadcom/make.inc
##
## History:
##    2014/1/23 - [Tao Wu] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

BCM_DRV_PATH := $(call my-dir)

######## SDIO WiFi ########

ifeq ($(BUILD_BROADCOM_WIFI_SDIO_OOB), y)
## use GPIO interrupt
export CONFIG_BCMDHD_OOB = y
else
## use SDIO interrupt
export CONFIG_BCMDHD_OOB = n
endif

ifeq ($(BUILD_BROADCOM_WIFI_ARP_OFFLOAD), y)
export ARP_OFFLOAD_SUPPORT = y
endif

ifeq ($(BUILD_BROADCOM_WIFI_PROC_PM_STATE), y)
export PROC_PM_STAT_SUPPORT = y
endif

ifeq ($(BUILD_BROADCOM_WIFI_TXGLOM), y)
export TXGLOM_SUPPORT = y
endif

ifeq ($(BUILD_BROADCOM_WIFI_INSMOD_NO_FW_LOAD), y)
export INSMOD_NO_FW_LOAD_SUPPORT = y
endif

ifeq ($(BUILD_BROADCOM_WIFI_SDIO_V3_0), y)
######## SDIO WiFi ########
include $(BCM_DRV_PATH)/bcmdhd_sdio_k3_0/make.inc
else ifeq ($(BUILD_BROADCOM_WIFI_SDIO_V2_V3_0), y)
######## SDIO V2 WiFi ########
include $(BCM_DRV_PATH)/bcmdhd_sdio_v2_k3_0/make.inc
endif

######## Choose SDIO WiFi Modules ########

ifeq ($(BUILD_BROADCOM_WIFI_SDIO_CHIP_BCM43362), y)
export BCMDHD_FW_PATH="\"/lib/firmware/broadcom/bcm43362/fw_bcm40181a2.bin\""
export BCMDHD_NVRAM_PATH="\"/lib/firmware/broadcom/bcm43362/nvram_gb9662.txt\""
else ifeq ($(BUILD_BROADCOM_WIFI_SDIO_MODULE_AP6441), y)
export BCMDHD_FW_PATH="\"/lib/firmware/broadcom/ap6441/fw_bcm43341b0_ag.bin\""
export BCMDHD_NVRAM_PATH="\"/lib/firmware/broadcom/ap6441/nvram_ap6441.txt\""
else ifeq ($(BUILD_BROADCOM_WIFI_SDIO_MODULE_AP6234), y)
export BCMDHD_FW_PATH="\"/lib/firmware/broadcom/ap6234/fw_bcm43341b0_ag.bin\""
ifeq ($(BUILD_BROADCOM_WIFI_SDIO_TCPKA), y)
export BCMDHD_NVRAM_PATH="\"/lib/firmware/broadcom/ap6234/nvram_ap6234_tcpka.txt\""
else
export BCMDHD_NVRAM_PATH="\"/lib/firmware/broadcom/ap6234/nvram_ap6234.txt\""
endif
else ifeq ($(BUILD_BROADCOM_WIFI_SDIO_MODULE_AP6234_PS), y)
export BCMDHD_FW_PATH="\"/lib/firmware/broadcom/ap6234_ps/fw_bcm43341b0_ag.bin\""
ifeq ($(BUILD_BROADCOM_WIFI_SDIO_TCPKA), y)
export BCMDHD_NVRAM_PATH="\"/lib/firmware/broadcom/ap6234_ps/nvram_ap6234_tcpka.txt\""
else
export BCMDHD_NVRAM_PATH="\"/lib/firmware/broadcom/ap6234_ps/nvram_ap6234.txt\""
endif
else ifeq ($(BUILD_BROADCOM_WIFI_SDIO_MODULE_AP6212), y)
export BCMDHD_FW_PATH="\"/lib/firmware/broadcom/ap6212/fw_bcm43438a0.bin\""
ifeq ($(BUILD_BROADCOM_WIFI_SDIO_TCPKA), y)
export BCMDHD_NVRAM_PATH="\"/lib/firmware/broadcom/ap6212/nvram_ap6212_tcpka.txt\""
else
export BCMDHD_NVRAM_PATH="\"/lib/firmware/broadcom/ap6212/nvram_ap6212.txt\""
endif
else ifeq ($(BUILD_BROADCOM_WIFI_SDIO_MODULE_AP6452), y)
export BCMDHD_FW_PATH="\"/lib/firmware/broadcom/ap6452/fw_bcm43438a0.bin\""
ifeq ($(BUILD_BROADCOM_WIFI_SDIO_TCPKA), y)
export BCMDHD_NVRAM_PATH="\"/lib/firmware/broadcom/ap6452/nvram_ap6452_tcpka.txt\""
else
export BCMDHD_NVRAM_PATH="\"/lib/firmware/broadcom/ap6452/nvram_ap6452.txt\""
endif
else ifeq ($(BUILD_BROADCOM_WIFI_SDIO_MODULE_AP6212A), y)
export BCMDHD_FW_PATH="\"/lib/firmware/broadcom/ap6212a/fw_bcm43438a1.bin\""
ifeq ($(BUILD_BROADCOM_WIFI_SDIO_TCPKA), y)
export BCMDHD_NVRAM_PATH="\"/lib/firmware/broadcom/ap6212a/nvram_ap6212a_tcpka.txt\""
else
export BCMDHD_NVRAM_PATH="\"/lib/firmware/broadcom/ap6212a/nvram_ap6212a.txt\""
endif
else ifeq ($(BUILD_BROADCOM_WIFI_SDIO_MODULE_AP6255), y)
export BCMDHD_FW_PATH="\"/lib/firmware/broadcom/ap6255/fw_bcm43455c0_ag.bin\""
ifeq ($(BUILD_BROADCOM_WIFI_SDIO_TCPKA), y)
export BCMDHD_NVRAM_PATH="\"/lib/firmware/broadcom/ap6255/nvram_ap6255_tcpka.txt\""
else
export BCMDHD_NVRAM_PATH="\"/lib/firmware/broadcom/ap6255/nvram_ap6255.txt\""
endif
else ifeq ($(BUILD_BROADCOM_WIFI_SDIO_MODULE_AP62X2), y)
export BCMDHD_FW_PATH="\"/lib/firmware/broadcom/ap62x2/fw_bcm43241b4_ag.bin\""
export BCMDHD_NVRAM_PATH="\"/lib/firmware/broadcom/ap62x2/nvram_ap62x2.txt\""
endif

######## USB WiFi ########

ifeq ($(BUILD_BROADCOM_WIFI_USB_V3_0), y)
include $(BCM_DRV_PATH)/bcmdhd_usb_k3_0/make.inc
endif
