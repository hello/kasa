##
## prebuild/third-party/atheros-wifi/make.inc
##
## History:
##    2012/05/30 - [Cao Rongrong] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(CONFIG_AMBARELLA_BROADCOM_SUPPORT), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_broadcom_wifi
LOCAL_SRCS	:= lib

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
	@echo "Copy $@ Done"

#### WiFi SDIO Interface
ifneq ($(BUILD_BROADCOM_WIFI_SDIO_CHIP_BCM43362), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/broadcom/bcm43362
endif
ifneq ($(BUILD_BROADCOM_WIFI_SDIO_MODULE_AP6441), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/broadcom/ap6441
endif
ifneq ($(BUILD_BROADCOM_WIFI_SDIO_MODULE_AP6234), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/broadcom/ap6234
endif
ifneq ($(BUILD_BROADCOM_WIFI_SDIO_MODULE_AP6234_PS), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/broadcom/ap6234_ps
endif
ifneq ($(BUILD_BROADCOM_WIFI_SDIO_MODULE_AP6212), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/broadcom/ap6212
endif

#### WiFi USB Interface
ifneq ($(BUILD_BROADCOM_WIFI_USB_BCM43143), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/broadcom/bcm43143
endif

$(call add-target-into-build, $(LOCAL_TARGET))
endif

