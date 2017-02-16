##
## utility/jpeg_snapshot/make.inc
##
## History:
##    2015/09/18 - [zfgong] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH   := $(call my-dir)

## libamjpeg-snapshot.so
ifeq ($(BUILD_AMBARELLA_ORYX_JPEG_SNAPSHOT), y)

include $(CLEAR_VARS)

JPEG_SNAPSHOT_VER_FILE   = $(ORYX_DIR)/include/utility/jpeg_snapshot/version.h
JPEG_SNAPSHOT_VER_PREFIX = JPEG_SNAPSHOT

JPEG_SNAPSHOT_TARGET  = libamjpeg-snapshot.so
JPEG_SNAPSHOT_CONF   := $(ORYX_DIR)/utility/jpeg_snapshot/jpeg_snapshot.acs

LOCAL_TARGET := $(JPEG_SNAPSHOT_TARGET)
LOCAL_SRCS    := $(LOCAL_PATH)/am_jpeg_snapshot.cpp
LOCAL_LIBS    := libamutil.so libamconfig.so libamjpeg-encoder.so libutils.so
LOCAL_LDFLAGS := -lpthread

LOCAL_CFLAGS := -I$(ORYX_DIR)/include                             \
                -I$(ORYX_DIR)/include/utility                     \
                -I$(ORYX_DIR)/include/video                       \
                -I$(ORYX_DIR)/include/event                       \
                -I$(ORYX_DIR)/include/configure                   \
                -I$(ORYX_DIR)/include/utility/jpeg_encoder            \
                -I$(ORYX_DIR)/include/utility/jpeg_snapshot          \
                -I$(ORYX_DIR)/utility/include                        \
                -std=c++11 -Werror

JPEG_SNAPSHOT_SO_VER_MAJOR  := $(shell awk '/define\ $(JPEG_SNAPSHOT_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(JPEG_SNAPSHOT_VER_FILE))
JPEG_SNAPSHOT_SO_VER_MINOR  := $(shell awk '/define\ $(JPEG_SNAPSHOT_VER_PREFIX)_LIB_MINOR/{print $$3}' $(JPEG_SNAPSHOT_VER_FILE))
JPEG_SNAPSHOT_SO_VER_PATCH  := $(shell awk '/define\ $(JPEG_SNAPSHOT_VER_PREFIX)_LIB_PATCH/{print $$3}' $(JPEG_SNAPSHOT_VER_FILE))
JPEG_SNAPSHOT_SO_VER_STRING := $(JPEG_SNAPSHOT_SO_VER_MAJOR).$(JPEG_SNAPSHOT_SO_VER_MINOR).$(JPEG_SNAPSHOT_SO_VER_PATCH)

JPEG_SNAPSHOT_SO_NAME   = $(patsubst %.so,%.so.$(JPEG_SNAPSHOT_SO_VER_MAJOR), $(JPEG_SNAPSHOT_TARGET))
JPEG_SNAPSHOT_SO_TARGET = $(patsubst %.so,%.so.$(JPEG_SNAPSHOT_SO_VER_STRING), $(JPEG_SNAPSHOT_TARGET))

LOCAL_SO_NAME := $(JPEG_SNAPSHOT_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)  COPY_CONFIG_SCRIPTS

$(LOCAL_TARGET): $(LOCAL_MODULE) COPY_CONFIG_SCRIPTS
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(JPEG_SNAPSHOT_SO_TARGET)
	@ln -sf $(JPEG_SNAPSHOT_SO_TARGET) $(ORYX_LIB_DIR)/$(JPEG_SNAPSHOT_SO_NAME)
	@echo "Build $@ Done."
	@mkdir -p $(FAKEROOT_DIR)/etc/oryx/utility
	@cp -dpRf $(JPEG_SNAPSHOT_CONF) $(FAKEROOT_DIR)/etc/oryx/utility/
	@echo "Copy config scripts Done."


$(call add-target-into-build, $(LOCAL_TARGET))

endif

