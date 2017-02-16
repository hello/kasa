##
## prebuild/third-party/armv7-a-hf/broadcom-tool/make.inc
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

ifeq ($(CONFIG_BROADCOM_TOOL_SUPPORT), y)

LOCAL_PATH	:= $(call my-dir)
BRCM_PATH 	:= $(LOCAL_PATH)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_broadcom_tools
LOCAL_SRCS	:= usr

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
	@echo "Copy $@ Done"

ifeq ($(BUILD_BROADCOM_WIFI_SDIO_MODULE_AP6212), y)
	@cp -r $(BRCM_PATH)/bcm43438/wl $(FAKEROOT_DIR)/usr/sbin/wl
	@cp -r $(BRCM_PATH)/bcm43438/brcm_patchram_plus_fake_open $(FAKEROOT_DIR)/usr/sbin/brcm_patchram_plus_fake_open
else
	@cp -r $(BRCM_PATH)/bcm43340/wl $(FAKEROOT_DIR)/usr/sbin/wl
endif

$(call add-target-into-build, $(LOCAL_TARGET))

endif

