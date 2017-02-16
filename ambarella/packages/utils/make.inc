##
## packages/data_transfer/make.inc
##
## History:
##    2012/06/01 - [Cao Rongrong] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_UTILITIES_PACKAGE), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

UTILS_MODULE = libutils.so
LOCAL_TARGET  := $(UTILS_MODULE)
LOCAL_SRCS    := $(wildcard $(LOCAL_PATH)/*.c)
LOCAL_LDFLAGS := -lrt

UTILS_VER_PREFIX = UTILS
UTILS_VERSION_FILE = $(LOCAL_PATH)/version.h
UTILS_SO_VER_MAJOR  := $(shell awk '/define\ $(UTILS_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(UTILS_VERSION_FILE))
UTILS_SO_VER_MINOR  := $(shell awk '/define\ $(UTILS_VER_PREFIX)_LIB_MINOR/{print $$3}' $(UTILS_VERSION_FILE))
UTILS_SO_VER_PATCH  := $(shell awk '/define\ $(UTILS_VER_PREFIX)_LIB_PATCH/{print $$3}' $(UTILS_VERSION_FILE))
UTILS_SO_VER_STRING := $(UTILS_SO_VER_MAJOR).$(UTILS_SO_VER_MINOR).$(UTILS_SO_VER_PATCH)
UTILS_SO_NAME   = $(patsubst %.so,%.so.$(UTILS_SO_VER_MAJOR), $(UTILS_MODULE))
UTILS_SO_TARGET = $(patsubst %.so,%.so.$(UTILS_SO_VER_STRING), $(UTILS_MODULE))

LOCAL_SO_NAME := $(UTILS_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/usr/lib/
	@cp -dpRf $< $(FAKEROOT_DIR)/usr/lib/$(UTILS_SO_TARGET)
	@ln -sf $(UTILS_SO_TARGET) $(FAKEROOT_DIR)/usr/lib/$(UTILS_SO_NAME)
	@ln -sf $(UTILS_SO_NAME) $(FAKEROOT_DIR)/usr/lib/$(UTILS_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

