##
## oryx/event/plugin/audio_alert/make.inc
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

ifeq ($(BUILD_AMBARELLA_ORYX_EVENT_PLUGIN_AUDIO_ALERT), y)

include $(CLEAR_VARS)

EVENT_AUDIO_ALERT_CONFIG = event-audio-alert.acs
AUDIO_ALERT_VER_FILE   = $(ORYX_DIR)/include/event/version.h
AUDIO_ALERT_VER_PREFIX = AUDIO_ALERT

AUDIO_ALERT_TARGET  = event-audio-alert.so
LOCAL_TARGET := $(AUDIO_ALERT_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS := -I$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/include \
                -I$(ORYX_DIR)/include/utility                \
                -I$(ORYX_DIR)/include/configure    			 \
                -I$(ORYX_DIR)/include/audio                  \
                -I$(ORYX_DIR)/include/audio/capture          \
                -I$(ORYX_DIR)/event/include                  \
                -I$(ORYX_DIR)/include/event                  \
                -I$(ORYX_DIR)/include                        \
                -std=c++11 -Werror


LOCAL_LIBS := libamutil.so libamconfig.so libamaudio-capture.so 

LOCAL_LDFLAGS := -L$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/usr/lib -lasound \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/pulseaudio/usr/lib

AUDIO_ALERT_SO_NAME   = $(AUDIO_ALERT_TARGET)
AUDIO_ALERT_SO_TARGET = $(AUDIO_ALERT_SO_NAME)

LOCAL_SO_NAME := $(AUDIO_ALERT_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): AUDIO_ALERT_PATH := $(LOCAL_PATH)/
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_EVENT_PLUGIN_DIR)/
	@mkdir -p $(FAKEROOT_DIR)/$(BUILD_AMBARELLA_ORYX_CONF_DIR)/event/
	@cp -dpRf $(AUDIO_ALERT_PATH)/$(EVENT_AUDIO_ALERT_CONFIG) \
		$(FAKEROOT_DIR)/$(BUILD_AMBARELLA_ORYX_CONF_DIR)/event/$(EVENT_AUDIO_ALERT_CONFIG)
	@cp -dpRf $< $(ORYX_EVENT_PLUGIN_DIR)/$(AUDIO_ALERT_SO_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

