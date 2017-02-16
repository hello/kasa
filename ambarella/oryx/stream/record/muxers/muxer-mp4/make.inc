##
## /S2l/oryx/stream/record/muxers/muxer-mp4/make.inc
##
## History:
##   2015-1-13 - [ypchang] created file
##
## Copyright (C) 2008-2015, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

LOCAL_PATH := $(call my-dir)

### muxer-mp4.so
include $(CLEAR_VARS)

MUXER_MP4 = muxer-mp4.so
MUXER_MP4_NORMAL_CONF = muxer-mp4.acs
MUXER_MP4_EVENT_CONF  = muxer-mp4-event.acs

LOCAL_TARGET := $(MUXER_MP4)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)                           \
                -I$(ORYX_DIR)/include                     \
                -I$(ORYX_DIR)/include/configure           \
                -I$(ORYX_DIR)/include/utility             \
                -I$(ORYX_DIR)/include/video               \
                -I$(ORYX_DIR)/include/audio               \
                -I$(ORYX_DIR)/stream/include/amf          \
                -I$(ORYX_DIR)/stream/include/common       \
                -I$(ORYX_DIR)/stream/include/common/io    \
                -I$(ORYX_DIR)/stream/include/stream       \
                -I$(ORYX_DIR)/stream/include/record       \
                -I$(ORYX_DIR)/stream/include/common/media \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so    \
              libamosal.so    \
              libamfile-sink.so

MUXER_MP4_NAME   = $(MUXER_MP4)
MUXER_MP4_TARGET = $(MUXER_MP4)
LOCAL_SO_NAME := $(MUXER_MP4)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): MUXER_MP4_SRC_PATH := $(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_MUXER_DIR)/
	@mkdir -p $(ORYX_CONF_DIR)/stream/muxer
	@cp -dpRf $(MUXER_MP4_SRC_PATH)/$(MUXER_MP4_NORMAL_CONF) $(ORYX_CONF_DIR)/stream/muxer/$(MUXER_MP4_NORMAL_CONF)
	@cp -dpRf $(MUXER_MP4_SRC_PATH)/$(MUXER_MP4_EVENT_CONF) $(ORYX_CONF_DIR)/stream/muxer/$(MUXER_MP4_EVENT_CONF)
	@cp -dpRf $< $(ORYX_MUXER_DIR)/$(MUXER_MP4_TARGET)
	@ln -sf $(MUXER_MP4) $(ORYX_MUXER_DIR)/muxer-mp4-event.so
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
