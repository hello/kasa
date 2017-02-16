##
## /S2l/oryx/stream/record/muxers/muxer-ts/make.inc
##
## History:
##   2015-1-14 - [ypchang] created file
##
## Copyright (C) 2008-2015, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

LOCAL_PATH := $(call my-dir)

### muxer-ts.so
include $(CLEAR_VARS)

MUXER_TS      = muxer-ts.so
MUXER_TS_CONF = muxer-ts.acs
MUXER_TS_EVENT_CONF = muxer-ts-event.acs

LOCAL_TARGET := $(MUXER_TS)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)/                          \
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

LOCAL_LIBS := libamutil.so       \
              libamosal.so       \
              libamfile-sink.so

MUXER_TS_NAME   = $(MUXER_TS)
MUXER_TS_TARGET = $(MUXER_TS)
LOCAL_SO_NAME := $(MUXER_TS)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): MUXER_TS_SRC_PATH := $(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_MUXER_DIR)/
	@mkdir -p $(ORYX_CONF_DIR)/stream/muxer/
	@cp -dpRf $(MUXER_TS_SRC_PATH)/$(MUXER_TS_CONF) $(ORYX_CONF_DIR)/stream/muxer/$(MUXER_TS_CONF)
	@cp -dpRf $(MUXER_TS_SRC_PATH)/$(MUXER_TS_EVENT_CONF) $(ORYX_CONF_DIR)/stream/muxer/$(MUXER_TS_EVENT_CONF)
	@cp -dpRf $< $(ORYX_MUXER_DIR)/$(MUXER_TS_TARGET)
	@ln -sf $(MUXER_TS) $(ORYX_MUXER_DIR)/muxer-ts-event.so
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
