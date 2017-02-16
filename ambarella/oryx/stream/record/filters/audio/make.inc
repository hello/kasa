##
## /S2l/oryx/stream/record/filters/audio/make.inc
##
## History:
##   2014-12-8 - [ypchang] created file
##
## Copyright (C) 2008-2014, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

AUDIO_SRC_TARGET = filter-audio-source.so
AUDIO_SRC_CONF   = filter-audio-source.acs
AUDIO_SRC_SCRIPT = switch-audio-profile

LOCAL_TARGET := $(AUDIO_SRC_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)                     \
                -I$(ORYX_DIR)/include               \
                -I$(ORYX_DIR)/include/configure     \
                -I$(ORYX_DIR)/include/utility       \
                -I$(ORYX_DIR)/include/stream        \
                -I$(ORYX_DIR)/include/audio         \
                -I$(ORYX_DIR)/include/audio/capture \
                -I$(ORYX_DIR)/stream/include/amf    \
                -I$(ORYX_DIR)/stream/include/common \
                -I$(ORYX_DIR)/stream/include/record \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so              \
              libamosal.so              \
              libamconfig.so            \
              libamstream-iid-record.so \
              libamstream-amf.so        \
              libamaudio-capture.so

AUDIO_SRC_SO_NAME   = $(AUDIO_SRC_TARGET)
AUDIO_SRC_SO_TARGET = $(AUDIO_SRC_TARGET)

LOCAL_SO_NAME := $(AUDIO_SRC_SO_NAME)

include $(BUILD_APP)

.PHONY: $(AUDIO_SRC_TARGET)

$(LOCAL_TARGET): A_SRC_PATH := $(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_FILTER_DIR)/
	@mkdir -p $(ORYX_BIN_DIR)/
	@mkdir -p $(ORYX_CONF_DIR)/stream/filter/
	@cp -dpRf $(A_SRC_PATH)/$(AUDIO_SRC_SCRIPT).sh $(ORYX_BIN_DIR)/$(AUDIO_SRC_SCRIPT)
	@cp -dpRf $(A_SRC_PATH)/$(AUDIO_SRC_CONF) $(ORYX_CONF_DIR)/stream/filter/$(AUDIO_SRC_CONF)
	@cp -dpRf $< $(ORYX_FILTER_DIR)/$(AUDIO_SRC_SO_TARGET)
	@chmod 755 $(ORYX_BIN_DIR)/$(AUDIO_SRC_SCRIPT)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
