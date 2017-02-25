##
## ambarella/oryx/make.inc
##
## History:
##    2016/01/16 - [Shupeng Ren] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(BUILD_AMBARELLA_ORYX_APP_APPS_SETUP), y)

LOCAL_TARGET := apps_setup
LOCAL_SRCS   := $(LOCAL_PATH)/apps_setup.cpp
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include               \
                 -I$(ORYX_DIR)/include/utility       \
                 -I$(ORYX_DIR)/include/event         \
                 -I$(ORYX_DIR)/include/audio         \
                 -I$(ORYX_DIR)/include/ipc           \
                 -I$(ORYX_DIR)/include/stream        \
                 -I$(ORYX_DIR)/include/services      \
                 -I$(ORYX_DIR)/services/include      \
                 -I$(ORYX_DIR)/include/audio/device  \
                 -I$(LOCAL_PATH)                     \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib \
                 -lpthread

LOCAL_LIBS    := libamipc.so              \
                 libamapi.so              \
                 libamutil.so             \
                 libamosal.so             \
                 libamstream-playback.so  \
                 libamstream-amf.so       \
                 libamevent-monitor.so    \
                 libamservice-manager.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): WIFI_SETUP_SRC_PATH := $(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif

