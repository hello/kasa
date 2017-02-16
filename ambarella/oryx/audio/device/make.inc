##
## oryx/audio/device/make.inc
##
## History:
##    2014/12/15 - [Hanbo Xiao] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH   := $(call my-dir)

ifeq ($(BUILD_AMBARELLA_ORYX_AUDIO_DEVICE), y)

include $(CLEAR_VARS)

AUDIO_DEVICE_VER_FILE   = $(ORYX_DIR)/include/audio/device/version.h
AUDIO_DEVICE_VER_PREFIX = ADEVICE

AUDIO_DEVICE_TARGET = libamaudio-device.so
LOCAL_TARGET := $(AUDIO_DEVICE_TARGET)
LOCAL_SRCS   := $(LOCAL_PATH)/am_audio_device.cpp
LOCAL_CFLAGS := -I$(ORYX_DIR)/include                        \
                -I$(ORYX_DIR)/include/utility                \
                -I$(ORYX_DIR)/include/audio/device          \
                -I$(ORYX_DIR)/audio/include                  \
                -I$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/include \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so

ifeq ($(BUILD_AMBARELLA_ORYX_AUDIO_DEVICE_ALSA), y)
LOCAL_LDFLAGS := -L$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/usr/lib -lasound
endif

ifeq ($(BUILD_AMBARELLA_ORYX_AUDIO_DEVICE_PULSE), y)
LOCAL_SRCS += $(LOCAL_PATH)/am_audio_device_pulse.cpp
LOCAL_CFLAGS += -I$(PREBUILD_3RD_PARTY_DIR)/pulseaudio/include
LOCAL_LDFLAGS += -L$(PREBUILD_3RD_PARTY_DIR)/pulseaudio/usr/lib -lpulse
endif


AUDIO_DEVICE_SO_VER_MAJOR  := $(shell awk '/define\ $(AUDIO_DEVICE_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(AUDIO_DEVICE_VER_FILE))
AUDIO_DEVICE_SO_VER_MINOR  := $(shell awk '/define\ $(AUDIO_DEVICE_VER_PREFIX)_LIB_MINOR/{print $$3}' $(AUDIO_DEVICE_VER_FILE))
AUDIO_DEVICE_SO_VER_PATCH  := $(shell awk '/define\ $(AUDIO_DEVICE_VER_PREFIX)_LIB_PATCH/{print $$3}' $(AUDIO_DEVICE_VER_FILE))
AUDIO_DEVICE_SO_VER_STRING := $(AUDIO_DEVICE_SO_VER_MAJOR).$(AUDIO_DEVICE_SO_VER_MINOR).$(AUDIO_DEVICE_SO_VER_PATCH)

AUDIO_DEVICE_SO_NAME   = $(patsubst %.so,%.so.$(AUDIO_DEVICE_SO_VER_MAJOR), $(AUDIO_DEVICE_TARGET))
AUDIO_DEVICE_SO_TARGET = $(patsubst %.so,%.so.$(AUDIO_DEVICE_SO_VER_STRING), $(AUDIO_DEVICE_TARGET))

LOCAL_SO_NAME := $(AUDIO_DEVICE_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(AUDIO_DEVICE_SO_TARGET)
	@ln -sf $(AUDIO_DEVICE_SO_TARGET) $(ORYX_LIB_DIR)/$(AUDIO_DEVICE_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
