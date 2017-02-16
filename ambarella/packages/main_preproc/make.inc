##
## packages/main_preproc/make.inc
##
## History:
##    2013/06/05 - [qianshen] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_MAINPREPROC_PACKAGE), y)

LOCAL_PATH := $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET  := libmainpp.so

LOCAL_SRCS    := $(wildcard $(LOCAL_PATH)/*.c)
LOCAL_CFLAGS  := -I$(AMB_TOPDIR)/build/include \
                 -I$(AMB_TOPDIR)/packages/utils

MAINPP_MODULE	= libmainpp.so
MAINPP_VER_PREFIX = MAINPP
MAINPP_VERSION_FILE = $(LOCAL_PATH)/version.c
MAINPP_SO_VER_MAJOR  := $(shell awk '/define\ $(MAINPP_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(MAINPP_VERSION_FILE))
MAINPP_SO_VER_MINOR  := $(shell awk '/define\ $(MAINPP_VER_PREFIX)_LIB_MINOR/{print $$3}' $(MAINPP_VERSION_FILE))
MAINPP_SO_VER_PATCH  := $(shell awk '/define\ $(MAINPP_VER_PREFIX)_LIB_PATCH/{print $$3}' $(MAINPP_VERSION_FILE))
MAINPP_SO_VER_STRING := $(MAINPP_SO_VER_MAJOR).$(MAINPP_SO_VER_MINOR).$(MAINPP_SO_VER_PATCH)
MAINPP_SO_NAME   = $(patsubst %.so,%.so.$(MAINPP_SO_VER_MAJOR), $(MAINPP_MODULE))
MAINPP_SO_TARGET = $(patsubst %.so,%.so.$(MAINPP_SO_VER_STRING), $(MAINPP_MODULE))

LOCAL_SO_NAME := $(MAINPP_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/usr/lib/
	@cp -dpRf $< $(FAKEROOT_DIR)/usr/lib/$(MAINPP_SO_TARGET)
	@ln -sf $(MAINPP_SO_TARGET) $(FAKEROOT_DIR)/usr/lib/$(MAINPP_SO_NAME)
	@ln -sf $(MAINPP_SO_NAME) $(FAKEROOT_DIR)/usr/lib/$(MAINPP_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
