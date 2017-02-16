##
## /S2l/oryx/stream/playback/filters/demuxer/make.inc
##
## History:
##   2014-9-10 - [ypchang] created file
##
## Copyright (C) 2008-2014, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

LOCAL_PATH := $(call my-dir)

include $(CLAER_VARS)

DEMUXER_TARGET = filter-demuxer.so
DEMUXER_CONF   = filter-demuxer.acs

LOCAL_TARGET := $(DEMUXER_TARGET)
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

DEMUXER_SO_NAME   = $(DEMUXER_TARGET)
DEMUXER_SO_TARGET = $(DEMUXER_TARGET)

LOCAL_SO_NAME := $(DEMUXER_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): DEMUXER_SRC_PATH := $(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_FILTER_DIR)/
	@mkdir -p $(ORYX_CONF_DIR)/stream/filter/
	@cp -dpRf $(DEMUXER_SRC_PATH)/$(DEMUXER_CONF) $(ORYX_CONF_DIR)/stream/filter/$(DEMUXER_CONF)
	@cp -dpRf $< $(ORYX_FILTER_DIR)/$(DEMUXER_SO_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
