##
## /S2l/oryx/stream/record/filters/avqueue/make.inc
##
## History:
##   2015-3-9 - [Shupeng Ren] created file
##
## Copyright (C) 2008-2015, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

EVENT_SENDER_SRC_TARGET = filter-event-sender.so
EVENT_SENDER_SRC_CONF   = filter-event-sender.acs

LOCAL_TARGET := $(EVENT_SENDER_SRC_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)                     \
                -I$(ORYX_DIR)/include               \
                -I$(ORYX_DIR)/include/configure     \
                -I$(ORYX_DIR)/include/utility       \
                -I$(ORYX_DIR)/include/stream        \
                -I$(ORYX_DIR)/stream/include/amf    \
                -I$(ORYX_DIR)/stream/include/common \
                -I$(ORYX_DIR)/stream/include/record \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so              \
              libamosal.so              \
              libamconfig.so            \
              libamstream-iid-record.so \
              libamstream-amf.so

EVENT_SENDER_SRC_SO_NAME   = $(EVENT_SENDER_SRC_TARGET)
EVENT_SENDER_SRC_SO_TARGET = $(EVENT_SENDER_SRC_TARGET)

LOCAL_SO_NAME := $(EVENT_SENDER_SRC_SO_NAME)

include $(BUILD_APP)

.PHONY: $(EVENT_SENDER_SRC_TARGET)

$(LOCAL_TARGET): EVENT_SENDER_SRC_PATH := $(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_FILTER_DIR)/
	@mkdir -p $(ORYX_CONF_DIR)/stream/filter/
	@cp -dpRf $(EVENT_SENDER_SRC_PATH)/$(EVENT_SENDER_SRC_CONF) $(ORYX_CONF_DIR)/stream/filter/$(EVENT_SENDER_SRC_CONF)
	@cp -dpRf $< $(ORYX_FILTER_DIR)/$(EVENT_SENDER_SRC_SO_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
