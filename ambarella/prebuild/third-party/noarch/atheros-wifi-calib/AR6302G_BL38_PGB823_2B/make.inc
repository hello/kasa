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

ifeq ($(BUILD_CALIBRATION_DATA_AR6302G_BL38_PGB823_2B), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_atheros_wifi_calib_AR6302G_BL38_PGB823_2B
LOCAL_SRCS	:= lib

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): prebuild_atheros_wifi
$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
ifneq ($(CONFIG_ATHEROS_FIRMWARE_VERSION_6003_796), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/ath6k/AR6003_796
endif
ifneq ($(CONFIG_ATHEROS_FIRMWARE_VERSION_6003_844), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/ath6k/AR6003_844
endif
ifneq ($(CONFIG_ATHEROS_FIRMWARE_VERSION_6003_PA10), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/ath6k/AR6003_Pa10
endif
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

endif

