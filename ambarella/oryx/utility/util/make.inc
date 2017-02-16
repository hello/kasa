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

## libamutil.so
ifeq ($(BUILD_AMBARELLA_ORYX_UTIL), y)

include $(CLEAR_VARS)

AM_UTILS_VER_FILE   = $(ORYX_DIR)/include/utility/version.h
AM_UTILS_VER_PREFIX = UTILI

AM_UTILS_TARGET  = libamutil.so
LOCAL_TARGET := $(AM_UTILS_TARGET)
LOCAL_SRCS   := $(LOCAL_PATH)/am_define.cpp \
                $(LOCAL_PATH)/am_log.cpp  \
                $(LOCAL_PATH)/am_pid_lock.cpp

LOCAL_CFLAGS := -I$(ORYX_DIR)/include/utility \
                -I$(ORYX_DIR)/include         \
                -std=c++11 -Werror

AM_UTILS_SO_VER_MAJOR  := $(shell awk '/define\ $(AM_UTILS_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(AM_UTILS_VER_FILE))
AM_UTILS_SO_VER_MINOR  := $(shell awk '/define\ $(AM_UTILS_VER_PREFIX)_LIB_MINOR/{print $$3}' $(AM_UTILS_VER_FILE))
AM_UTILS_SO_VER_PATCH  := $(shell awk '/define\ $(AM_UTILS_VER_PREFIX)_LIB_PATCH/{print $$3}' $(AM_UTILS_VER_FILE))
AM_UTILS_SO_VER_STRING := $(AM_UTILS_SO_VER_MAJOR).$(AM_UTILS_SO_VER_MINOR).$(AM_UTILS_SO_VER_PATCH)

AM_UTILS_SO_NAME   = $(patsubst %.so,%.so.$(AM_UTILS_SO_VER_MAJOR), $(AM_UTILS_TARGET))
AM_UTILS_SO_TARGET = $(patsubst %.so,%.so.$(AM_UTILS_SO_VER_STRING), $(AM_UTILS_TARGET))

LOCAL_SO_NAME := $(AM_UTILS_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(AM_UTILS_SO_TARGET)
	@ln -sf $(AM_UTILS_SO_TARGET) $(ORYX_LIB_DIR)/$(AM_UTILS_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

