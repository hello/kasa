##
## oryx/amf/make.inc
##
## History:
##    2014/07/22 - [Yupeng Chang] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_ORYX_STREAM), y)

LOCAL_PATH   := $(call my-dir)

## libamf.so
include $(CLEAR_VARS)

AMF_VER_FILE   = $(ORYX_DIR)/stream/include/amf/version.h
AMF_VER_PREFIX = AMF

AMF_TARGET  = libamstream-amf.so
LOCAL_TARGET := $(AMF_TARGET)
LOCAL_SRCS   := $(LOCAL_PATH)/am_amf_base.cpp          \
                $(LOCAL_PATH)/am_amf_queue.cpp         \
                $(LOCAL_PATH)/am_amf_packet.cpp        \
                $(LOCAL_PATH)/am_amf_msgsys.cpp        \
                $(LOCAL_PATH)/am_amf_interface.cpp     \
                $(LOCAL_PATH)/am_amf_packet_pin.cpp    \
                $(LOCAL_PATH)/am_amf_packet_pool.cpp   \
                $(LOCAL_PATH)/am_amf_engine_frame.cpp  \
                $(LOCAL_PATH)/am_amf_packet_filter.cpp

LOCAL_CFLAGS := -I$(ORYX_DIR)/stream/include/amf \
                -I$(ORYX_DIR)/include/utility    \
                -I$(ORYX_DIR)/include            \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so \
              libamosal.so

AMF_SO_VER_MAJOR  := $(shell awk '/define\ $(AMF_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(AMF_VER_FILE))
AMF_SO_VER_MINOR  := $(shell awk '/define\ $(AMF_VER_PREFIX)_LIB_MINOR/{print $$3}' $(AMF_VER_FILE))
AMF_SO_VER_PATCH  := $(shell awk '/define\ $(AMF_VER_PREFIX)_LIB_PATCH/{print $$3}' $(AMF_VER_FILE))
AMF_SO_VER_STRING := $(AMF_SO_VER_MAJOR).$(AMF_SO_VER_MINOR).$(AMF_SO_VER_PATCH)

AMF_SO_NAME   = $(patsubst %.so,%.so.$(AMF_SO_VER_MAJOR), $(AMF_TARGET))
AMF_SO_TARGET = $(patsubst %.so,%.so.$(AMF_SO_VER_STRING), $(AMF_TARGET))

LOCAL_SO_NAME := $(AMF_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(AMF_SO_TARGET)
	@ln -sf $(AMF_SO_TARGET) $(ORYX_LIB_DIR)/$(AMF_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
