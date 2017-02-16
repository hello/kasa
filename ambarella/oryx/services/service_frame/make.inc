##
## /S2l/oryx/services/service_frame/make.inc
##
## History:
##   2015-1-26 - [ypchang] created file
##
## Copyright (C) 2008-2015, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

ifeq ($(BUILD_AMBARELLA_ORYX_SERVICE_FRAME), y)
LOCAL_PATH := $(call my-dir)

### libamservice-frame.so
include $(CLEAR_VARS)

SERVICE_FRAME_TARGET   = libamservice-frame.so
SERVICE_FRAME_VER_FILE = $(ORYX_DIR)/services/service_frame/version.h
SERVICE_FRAME_PREFIX   = SERVICE_FRAME

LOCAL_TARGET := $(SERVICE_FRAME_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)                \
                -I$(ORYX_DIR)/include          \
                -I$(ORYX_DIR)/include/utility  \
                -I$(ORYX_DIR)/include/watchdog \
                -I$(ORYX_DIR)/services/include \
                -std=c++11 -Werror

LOCAL_LIBS   := libamutil.so \
                libamosal.so

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib

SERVICE_FRAME_SO_VER_MAJ := $(shell awk '/define\ $(SERVICE_FRAME_PREFIX)_LIB_MAJOR/{print $$3}' $(SERVICE_FRAME_VER_FILE))
SERVICE_FRAME_SO_VER_MIN := $(shell awk '/define\ $(SERVICE_FRAME_PREFIX)_LIB_MINOR/{print $$3}' $(SERVICE_FRAME_VER_FILE))
SERVICE_FRAME_SO_VER_PAT := $(shell awk '/define\ $(SERVICE_FRAME_PREFIX)_LIB_PATCH/{print $$3}' $(SERVICE_FRAME_VER_FILE))
SERVICE_FRAME_SO_VER_STR := $(SERVICE_FRAME_SO_VER_MAJ).$(SERVICE_FRAME_SO_VER_MIN).$(SERVICE_FRAME_SO_VER_PAT)

SERVICE_FRAME_SO_NAME   := $(patsubst %.so,%.so.$(SERVICE_FRAME_SO_VER_MAJ), $(SERVICE_FRAME_TARGET))
SERVICE_FRAME_SO_TARGET := $(patsubst %.so,%.so.$(SERVICE_FRAME_SO_VER_STR), $(SERVICE_FRAME_TARGET))

LOCAL_SO_NAME := $(SERVICE_FRAME_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): SERVICE_FRAME_PATH := $(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(SERVICE_FRAME_SO_TARGET)
	@ln -sf $(SERVICE_FRAME_SO_TARGET) $(ORYX_LIB_DIR)/$(SERVICE_FRAME_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif
