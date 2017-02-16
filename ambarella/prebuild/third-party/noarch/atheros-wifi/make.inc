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

ifeq ($(CONFIG_AMBARELLA_ATHEROS_WIFI_SUPPORT), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_atheros_wifi
LOCAL_SRCS	:= lib etc

ifeq ($(CONFIG_AMBARELLA_ATHEROS_WIFI_UTILITY_SUPPORT), y)
LOCAL_SRCS += usr
endif

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): ATH_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
	@rm -rf $(FAKEROOT_DIR)/etc/modprobe.d/ath6kl_sdio.conf
	@rm -rf $(FAKEROOT_DIR)/etc/modprobe.d/ath6kl_usb.conf
ifneq ($(CONFIG_ATHEROS_FIRMARE_AR3K), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/ar3k
endif
ifneq ($(CONFIG_ATHEROS_FIRMWARE_VERSION_6002), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/ath6k/AR6002
endif
ifneq ($(CONFIG_ATHEROS_FIRMWARE_VERSION_6002_25), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/ath6k/AR6002_25
else
	@cd $(FAKEROOT_DIR)/lib/firmware/ath6k && ln -s AR6002_25 AR6002
	@cd $(ATH_PATH)
endif
ifneq ($(CONFIG_ATHEROS_FIRMWARE_VERSION_6003), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/ath6k/AR6003
endif
ifneq ($(CONFIG_ATHEROS_FIRMWARE_VERSION_6003_1), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/ath6k/AR6003.1
else
	@cd $(FAKEROOT_DIR)/lib/firmware/ath6k && ln -s AR6003.1 AR6003
	@cd $(ATH_PATH)
endif
ifneq ($(CONFIG_ATHEROS_FIRMWARE_VERSION_6003_445), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/ath6k/AR6003_445
else
	@cd $(FAKEROOT_DIR)/lib/firmware/ath6k && ln -s AR6003_445 AR6003
	@cd $(ATH_PATH)
endif
ifneq ($(CONFIG_ATHEROS_FIRMWARE_VERSION_6003_796), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/ath6k/AR6003_796
else
	@cd $(FAKEROOT_DIR)/lib/firmware/ath6k && ln -s AR6003_796 AR6003
	@cd $(ATH_PATH)
endif
ifneq ($(CONFIG_ATHEROS_FIRMWARE_VERSION_6003_844), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/ath6k/AR6003_844
else
	@cd $(FAKEROOT_DIR)/lib/firmware/ath6k && ln -s AR6003_844 AR6003
	@cd $(ATH_PATH)
endif
ifneq ($(CONFIG_ATHEROS_FIRMWARE_VERSION_6003_PA10), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/ath6k/AR6003_Pa10
else
	@cd $(FAKEROOT_DIR)/lib/firmware/ath6k && ln -s AR6003_Pa10 AR6003
	@cd $(ATH_PATH)
endif
ifneq ($(CONFIG_ATHEROS_FIRMWARE_VERSION_6004), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/ath6k/AR6004
endif
ifneq ($(CONFIG_ATHEROS_FIRMWARE_VERSION_6004_350374), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/ath6k/AR6004_3.5.0.374
else
	@cd $(FAKEROOT_DIR)/lib/firmware/ath6k && ln -s AR6004_3.5.0.374 AR6004
	@cp $(ATH_PATH)/etc/modprobe.d/ath6kl_sdio.conf $(FAKEROOT_DIR)/etc/modprobe.d/ath6kl_sdio.conf
	@cd $(ATH_PATH)
endif
ifneq ($(CONFIG_ATHEROS_FIRMWARE_VERSION_6004_35028), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/ath6k/AR6004_3.5.0.28
else
	@cd $(FAKEROOT_DIR)/lib/firmware/ath6k && ln -s AR6004_3.5.0.28 AR6004
	@cp $(ATH_PATH)/etc/modprobe.d/ath6kl_sdio.conf $(FAKEROOT_DIR)/etc/modprobe.d/ath6kl_usb.conf
	@cd $(ATH_PATH)
endif
ifneq ($(CONFIG_ATHEROS_FIRMWARE_VERSION_6004_50080), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/ath6k/AR6004_5.0.0.80
else
	@cd $(FAKEROOT_DIR)/lib/firmware/ath6k && ln -s AR6004_5.0.0.80 AR6004
	@cp $(ATH_PATH)/etc/modprobe.d/ath6kl_sdio.conf $(FAKEROOT_DIR)/etc/modprobe.d/ath6kl_usb.conf
	@cd $(ATH_PATH)
endif
ifneq ($(CONFIG_ATHEROS_FIRMWARE_VERSION_6004_500110), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/ath6k/AR6004_5.0.0.110
else
	@cd $(FAKEROOT_DIR)/lib/firmware/ath6k && ln -s AR6004_5.0.0.110 AR6004
	@cp $(ATH_PATH)/etc/modprobe.d/ath6kl_sdio.conf $(FAKEROOT_DIR)/etc/modprobe.d/ath6kl_usb.conf
	@cd $(ATH_PATH)
endif
	@echo "Copy $@ Done"

ifneq ($(CONFIG_ATHEROS_FIRMARE_ATH9K), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/ath9k
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/htc_9271.fw
endif
$(call add-target-into-build, $(LOCAL_TARGET))

endif
