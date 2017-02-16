##
## /S2l/oryx/stream/playback/filters/decoder/make.inc
##
## History:
##   2014-10-9 - [ypchang] created file
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

DECODER_TARGET = filter-decoder.so
DECODER_CONF   = filter-decoder.acs

LOCAL_TARGET := $(DECODER_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)                       \
                -I$(ORYX_DIR)/include                 \
                -I$(ORYX_DIR)/include/audio           \
                -I$(ORYX_DIR)/include/configure       \
                -I$(ORYX_DIR)/include/utility         \
                -I$(ORYX_DIR)/include/stream          \
                -I$(ORYX_DIR)/stream/include/amf      \
                -I$(ORYX_DIR)/stream/include/common   \
                -I$(ORYX_DIR)/stream/include/playback \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so                \
              libamosal.so                \
              libamconfig.so              \
              libamstream-iid-playback.so \
              libamstream-amf.so

DECODER_SO_NAME   = $(DECODER_TARGET)
DECODER_SO_TARGET = $(DECODER_TARGET)

LOCAL_SO_NAME := $(DECODER_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): DECODER_SRC_PATH := $(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_FILTER_DIR)/
	@mkdir -p $(ORYX_CONF_DIR)/stream/filter/
	@cp -dpRf $(DECODER_SRC_PATH)/$(DECODER_CONF) $(ORYX_CONF_DIR)/stream/filter/$(DECODER_CONF)
	@cp -dpRf $< $(ORYX_FILTER_DIR)/$(DECODER_SO_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
