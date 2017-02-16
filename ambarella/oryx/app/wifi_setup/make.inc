##
## ambarella/oryx/make.inc
##
## History:
##    2015/01/16 - [Hanbo Xiao] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_ORYX_APP_WIFI_SETUP), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

WIFI_SETUP_CONF = wifi_setup.acs

LOCAL_TARGET := wifi_setup
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include               \
                 -I$(ORYX_DIR)/include/configure     \
                 -I$(ORYX_DIR)/include/utility       \
                 -I$(ORYX_DIR)/include/video         \
                 -I$(ORYX_DIR)/include/audio         \
                 -I$(ORYX_DIR)/include/event         \
                 -I$(ORYX_DIR)/include/qrcode        \
                 -I$(ORYX_DIR)/include/stream        \
                 -I$(ORYX_DIR)/include/audio/device  \
                 -I$(LOCAL_PATH)                     \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib                          \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/zbar/usr/lib           \
                 -lpthread


LOCAL_LIBS    := libamutil.so             \
                 libamconfig.so           \
                 libamvideodsp.so         \
                 libamqrcode-detection.so \
                 libamaudio-device.so     \
                 libamstream-playback.so  \
                 libamstream-amf.so       \
                 libamevent-monitor.so    \
                 libamosal.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): WIFI_SETUP_SRC_PATH := $(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_CONF_DIR)/app/wifi_setup/
	@cp -dpRf $(WIFI_SETUP_SRC_PATH)/$(WIFI_SETUP_CONF) $(ORYX_CONF_DIR)/app/wifi_setup/
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/

	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
