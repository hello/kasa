##
## oryx/configure/make.inc
##
## History:
##   2014-7-30 - [ypchang] created file
##
## Copyright (C) 2008-2014, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

LOCAL_PATH := $(call my-dir)

## libamconfig.so

ifeq ($(BUILD_AMBARELLA_ORYX_CONFIG), y)

include $(CLEAR_VARS)

CONFIG_VER_FILE = $(ORYX_DIR)/include/configure/version.h
CONFIG_VER_PREFIX = CONFIG

CONFIG_TARGET = libamconfig.so
LOCAL_TARGET := $(CONFIG_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_CFLAGS := -I$(ORYX_DIR)/include                   \
                -I$(ORYX_DIR)/include/configure         \
                -I$(ORYX_DIR)/include/utility           \
                -I$(PREBUILD_3RD_PARTY_DIR)/lua/include \
                -std=c++11 -Werror

LOCAL_LDFLAGS := -L$(PREBUILD_3RD_PARTY_DIR)/lua/usr/lib -llua

LOCAL_LIBS := libamutil.so \
              libamosal.so

CONFIG_SO_VER_MAJOR  := $(shell awk '/define\ $(CONFIG_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(CONFIG_VER_FILE))
CONFIG_SO_VER_MINOR  := $(shell awk '/define\ $(CONFIG_VER_PREFIX)_LIB_MINOR/{print $$3}' $(CONFIG_VER_FILE))
CONFIG_SO_VER_PATCH  := $(shell awk '/define\ $(CONFIG_VER_PREFIX)_LIB_PATCH/{print $$3}' $(CONFIG_VER_FILE))
CONFIG_SO_VER_STRING := $(CONFIG_SO_VER_MAJOR).$(CONFIG_SO_VER_MINOR).$(CONFIG_SO_VER_PATCH)

CONFIG_SO_NAME   = $(patsubst %.so,%.so.$(CONFIG_SO_VER_MAJOR), $(CONFIG_TARGET))
CONFIG_SO_TARGET = $(patsubst %.so,%.so.$(CONFIG_SO_VER_STRING), $(CONFIG_TARGET))

LOCAL_SO_NAME := $(CONFIG_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): prebuild_lua
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(CONFIG_SO_TARGET)
	@ln -sf $(CONFIG_SO_TARGET) $(ORYX_LIB_DIR)/$(CONFIG_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

include $(LOCAL_PATH)/unit_test/make.inc

endif
