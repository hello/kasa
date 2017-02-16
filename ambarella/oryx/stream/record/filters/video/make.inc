##
## /S2l/oryx/stream/record/filters/video/make.inc
##
## History:
##   2014-12-25 - [ypchang] created file
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

VIDEO_SRC_TARGET = filter-video-source.so
VIDEO_SRC_CONF   = filter-video-source.acs

LOCAL_TARGET := $(VIDEO_SRC_TARGET)
LOCAL_SRCS   := $(LOCAL_PATH)/am_video_source_config.cpp

ifeq ($(BUILD_AMBARELLA_ORYX_VIDEO_NEW), y)
LOCAL_SRCS += $(LOCAL_PATH)/am_video_source_new.cpp
else
LOCAL_SRCS += $(LOCAL_PATH)/am_video_source.cpp
endif

LOCAL_CFLAGS := -I$(LOCAL_PATH)                     \
                -I$(ORYX_DIR)/include               \
                -I$(ORYX_DIR)/include/configure     \
                -I$(ORYX_DIR)/include/utility       \
                -I$(ORYX_DIR)/include/stream        \
                -I$(ORYX_DIR)/stream/include/amf    \
                -I$(ORYX_DIR)/stream/include/common \
                -I$(ORYX_DIR)/stream/include/record
ifeq ($(BUILD_AMBARELLA_ORYX_VIDEO_NEW), y)
LOCAL_CFLAGS += -I$(ORYX_DIR)/include/video_new
else
LOCAL_CFLAGS += -I$(ORYX_DIR)/include/video
endif
LOCAL_CFLAGS += -std=c++11 -Werror

LOCAL_LIBS := libamutil.so              \
              libamosal.so              \
              libamconfig.so            \
              libamstream-iid-record.so \
              libamstream-amf.so
ifeq ($(BUILD_AMBARELLA_ORYX_VIDEO_NEW), y)
LOCAL_LIBS += libamvideo-reader.so libamvideo-address.so
else
LOCAL_LIBS += libamvideodsp.so
endif

VIDEO_SRC_SO_NAME   = $(VIDEO_SRC_TARGET)
VIDEO_SRC_SO_TARGET = $(VIDEO_SRC_TARGET)

LOCAL_SO_NAME := $(VIDEO_SRC_SO_NAME)

include $(BUILD_APP)

.PHONY: $(VIDEO_SRC_TARGET)

$(LOCAL_TARGET): V_SRC_PATH := $(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_FILTER_DIR)/
	@mkdir -p $(ORYX_CONF_DIR)/stream/filter/
	@cp -dpRf $(V_SRC_PATH)/$(VIDEO_SRC_CONF) $(ORYX_CONF_DIR)/stream/filter/$(VIDEO_SRC_CONF)
	@cp -dpRf $< $(ORYX_FILTER_DIR)/$(VIDEO_SRC_SO_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
