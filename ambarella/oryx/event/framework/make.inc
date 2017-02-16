##
## oryx/event/framework/make.inc
##
## History:
##    2014/10/21 - [Louis Sun] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH   := $(call my-dir)

ifeq ($(BUILD_AMBARELLA_ORYX_EVENTMONITOR), y)

include $(CLEAR_VARS)

EVENT_MONITOR_VER_FILE   = $(ORYX_DIR)/include/event/version.h
EVENT_MONITOR_VER_PREFIX = EVENT_MONITOR

EVENT_MONITOR_TARGET  = libamevent-monitor.so
LOCAL_TARGET := $(EVENT_MONITOR_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS := -I$(ORYX_DIR)/include/utility \
                -I$(ORYX_DIR)/include/event \
                -I$(ORYX_DIR)/include \
                -I$(ORYX_DIR)/event/include \
                -std=c++11 -Werror

LOCAL_LIBS    := libamutil.so
LOCAL_LDFLAGS := -ldl

EVENT_MONITOR_SO_VER_MAJOR  := $(shell awk '/define\ $(EVENT_MONITOR_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(EVENT_MONITOR_VER_FILE))
EVENT_MONITOR_SO_VER_MINOR  := $(shell awk '/define\ $(EVENT_MONITOR_VER_PREFIX)_LIB_MINOR/{print $$3}' $(EVENT_MONITOR_VER_FILE))
EVENT_MONITOR_SO_VER_PATCH  := $(shell awk '/define\ $(EVENT_MONITOR_VER_PREFIX)_LIB_PATCH/{print $$3}' $(EVENT_MONITOR_VER_FILE))
EVENT_MONITOR_SO_VER_STRING := $(EVENT_MONITOR_SO_VER_MAJOR).$(EVENT_MONITOR_SO_VER_MINOR).$(EVENT_MONITOR_SO_VER_PATCH)

EVENT_MONITOR_SO_NAME   = $(patsubst %.so,%.so.$(EVENT_MONITOR_SO_VER_MAJOR), $(EVENT_MONITOR_TARGET))
EVENT_MONITOR_SO_TARGET = $(patsubst %.so,%.so.$(EVENT_MONITOR_SO_VER_STRING), $(EVENT_MONITOR_TARGET))

LOCAL_SO_NAME := $(EVENT_MONITOR_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(EVENT_MONITOR_SO_TARGET)
	@ln -sf $(EVENT_MONITOR_SO_TARGET) $(ORYX_LIB_DIR)/$(EVENT_MONITOR_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))


endif

