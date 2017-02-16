##
## /S2l/oryx/stream/record/filters/muxer/make.inc
##
## History:
##   2014-12-29 - [ypchang] created file
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

MUXER_SRC_TARGET  = filter-muxer.so
MUXER_FILE_CONF   = filter-file-muxer.acs
MUXER_DIRECT_CONF = filter-direct-muxer.acs

LOCAL_TARGET := $(MUXER_SRC_TARGET)
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

MUXER_SRC_SO_NAME   = $(MUXER_SRC_TARGET)
MUXER_SRC_SO_TARGET = $(MUXER_SRC_TARGET)

LOCAL_SO_NAME := $(MUXER_SRC_SO_NAME)

include $(BUILD_APP)

.PHONY: $(MUXER_SRC_TARGET)

$(LOCAL_TARGET): MUXER_SRC_PATH := $(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_FILTER_DIR)/
	@mkdir -p $(ORYX_CONF_DIR)/stream/filter/
	@cp -dpRf $(MUXER_SRC_PATH)/$(MUXER_FILE_CONF) $(ORYX_CONF_DIR)/stream/filter/$(MUXER_FILE_CONF)
	@cp -dpRf $(MUXER_SRC_PATH)/$(MUXER_DIRECT_CONF) $(ORYX_CONF_DIR)/stream/filter/$(MUXER_DIRECT_CONF)
	@cp -dpRf $< $(ORYX_FILTER_DIR)/$(MUXER_SRC_SO_TARGET)
	@ln -sf $(MUXER_SRC_SO_TARGET) $(ORYX_FILTER_DIR)/filter-file-muxer.so
	@ln -sf $(MUXER_SRC_SO_TARGET) $(ORYX_FILTER_DIR)/filter-direct-muxer.so
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
