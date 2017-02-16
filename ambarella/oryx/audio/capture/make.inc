##
## oryx/audio/io/make.inc
##
## History:
##    2014/11/17 - [Dongge Wu] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH   := $(call my-dir)

ifeq ($(BUILD_AMBARELLA_ORYX_AUDIO_CAPTURE), y)

include $(CLEAR_VARS)

AUDIO_CAPTURE_VER_FILE   = $(ORYX_DIR)/include/audio/capture/version.h
AUDIO_CAPTURE_VER_PREFIX = AUDIO_CAPTURE

AUDIO_CAPTURE_TARGET = libamaudio-capture.so
LOCAL_TARGET := $(AUDIO_CAPTURE_TARGET)
LOCAL_SRCS   := $(LOCAL_PATH)/am_audio_capture.cpp
LOCAL_CFLAGS := -I$(ORYX_DIR)/include                        \
                -I$(ORYX_DIR)/include/utility                \
                -I$(ORYX_DIR)/include/audio                  \
                -I$(ORYX_DIR)/include/audio/capture          \
                -I$(ORYX_DIR)/audio/include                  \
                -I$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/include \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so libamosal.so

ifeq ($(BUILD_AMBARELLA_ORYX_AUDIO_CAPTURE_ALSA), y)
LOCAL_LDFLAGS := -L$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/usr/lib -lasound
endif

ifeq ($(BUILD_AMBARELLA_ORYX_AUDIO_CAPTURE_PULSE), y)
LOCAL_SRCS += $(LOCAL_PATH)/am_audio_capture_pulse.cpp
LOCAL_CFLAGS += -I$(PREBUILD_3RD_PARTY_DIR)/pulseaudio/include
LOCAL_LDFLAGS += -L$(PREBUILD_3RD_PARTY_DIR)/pulseaudio/usr/lib -lpulse \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/systemd/usr/lib
endif


AUDIO_CAPTURE_SO_VER_MAJOR  := $(shell awk '/define\ $(AUDIO_CAPTURE_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(AUDIO_CAPTURE_VER_FILE))
AUDIO_CAPTURE_SO_VER_MINOR  := $(shell awk '/define\ $(AUDIO_CAPTURE_VER_PREFIX)_LIB_MINOR/{print $$3}' $(AUDIO_CAPTURE_VER_FILE))
AUDIO_CAPTURE_SO_VER_PATCH  := $(shell awk '/define\ $(AUDIO_CAPTURE_VER_PREFIX)_LIB_PATCH/{print $$3}' $(AUDIO_CAPTURE_VER_FILE))
AUDIO_CAPTURE_SO_VER_STRING := $(AUDIO_CAPTURE_SO_VER_MAJOR).$(AUDIO_CAPTURE_SO_VER_MINOR).$(AUDIO_CAPTURE_SO_VER_PATCH)

AUDIO_CAPTURE_SO_NAME   = $(patsubst %.so,%.so.$(AUDIO_CAPTURE_SO_VER_MAJOR), $(AUDIO_CAPTURE_TARGET))
AUDIO_CAPTURE_SO_TARGET = $(patsubst %.so,%.so.$(AUDIO_CAPTURE_SO_VER_STRING), $(AUDIO_CAPTURE_TARGET))

LOCAL_SO_NAME := $(AUDIO_CAPTURE_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(AUDIO_CAPTURE_SO_TARGET)
	@ln -sf $(AUDIO_CAPTURE_SO_TARGET) $(ORYX_LIB_DIR)/$(AUDIO_CAPTURE_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
