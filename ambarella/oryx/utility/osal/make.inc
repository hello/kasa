##
## oryx/utility/make.inc
##
## History:
##    2014/07/21 - [Yupeng Chang] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH   := $(call my-dir)

## libamosal.so
ifeq ($(BUILD_AMBARELLA_ORYX_OSAL), y)

include $(CLEAR_VARS)

OSAL_VER_FILE   = $(ORYX_DIR)/include/utility/version.h
OSAL_VER_PREFIX = UTILITY

OSAL_TARGET   = libamosal.so
LOCAL_TARGET := $(OSAL_TARGET)
LOCAL_SRCS   := $(LOCAL_PATH)/am_fd.cpp     \
                $(LOCAL_PATH)/am_file.cpp   \
                $(LOCAL_PATH)/am_event.cpp  \
                $(LOCAL_PATH)/am_mutex.cpp  \
                $(LOCAL_PATH)/am_plugin.cpp \
                $(LOCAL_PATH)/am_thread.cpp

LOCAL_LDFLAGS := -lpthread -ldl

LOCAL_CFLAGS := -I$(ORYX_DIR)/include/utility \
                -I$(ORYX_DIR)/include         \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so

OSAL_SO_VER_MAJOR  := $(shell awk '/define\ $(OSAL_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(OSAL_VER_FILE))
OSAL_SO_VER_MINOR  := $(shell awk '/define\ $(OSAL_VER_PREFIX)_LIB_MINOR/{print $$3}' $(OSAL_VER_FILE))
OSAL_SO_VER_PATCH  := $(shell awk '/define\ $(OSAL_VER_PREFIX)_LIB_PATCH/{print $$3}' $(OSAL_VER_FILE))
OSAL_SO_VER_STRING := $(OSAL_SO_VER_MAJOR).$(OSAL_SO_VER_MINOR).$(OSAL_SO_VER_PATCH)

OSAL_SO_NAME   = $(patsubst %.so,%.so.$(OSAL_SO_VER_MAJOR), $(OSAL_TARGET))
OSAL_SO_TARGET = $(patsubst %.so,%.so.$(OSAL_SO_VER_STRING), $(OSAL_TARGET))

LOCAL_SO_NAME := $(OSAL_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(OSAL_SO_TARGET)
	@ln -sf $(OSAL_SO_TARGET) $(ORYX_LIB_DIR)/$(OSAL_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
