##
## oryx/event/plugin/key_input/make.inc
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

ifeq ($(BUILD_AMBARELLA_ORYX_EVENT_PLUGIN_KEY_INPUT), y)

include $(CLEAR_VARS)
EVENT_KEY_INPUT_CONFIG = event-key-input.acs

KEY_INPUT_TARGET  = event-key-input.so
LOCAL_TARGET := $(KEY_INPUT_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS := -I$(ORYX_DIR)/include/utility      \
                -I$(ORYX_DIR)/include/configure    \
                -I$(ORYX_DIR)/event/include        \
                -I$(ORYX_DIR)/include/event        \
                -I$(ORYX_DIR)/include              \
                -std=c++11 -Werror


LOCAL_LIBS := libamutil.so libamosal.so libamconfig.so


KEY_INPUT_SO_NAME   = $(KEY_INPUT_TARGET)
KEY_INPUT_SO_TARGET = $(KEY_INPUT_SO_NAME)

LOCAL_SO_NAME := $(KEY_INPUT_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): KEY_INPUT_PATH := $(LOCAL_PATH)/
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_EVENT_PLUGIN_DIR)/
	@mkdir -p $(FAKEROOT_DIR)/$(BUILD_AMBARELLA_ORYX_CONF_DIR)/event/
	@cp -dpRf $(KEY_INPUT_PATH)/$(EVENT_KEY_INPUT_CONFIG) \
		$(FAKEROOT_DIR)/$(BUILD_AMBARELLA_ORYX_CONF_DIR)/event/$(EVENT_KEY_INPUT_CONFIG)
	@cp -dpRf $< $(ORYX_EVENT_PLUGIN_DIR)/$(KEY_INPUT_SO_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

