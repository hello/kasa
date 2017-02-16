##
## camera/modules/network/make.inc
##
## History:
##    2015-1-12 - [Tao Wu] created file
##
## Copyright (C) 2012-2015, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH   := $(call my-dir)

include $(CLEAR_VARS)

NETWORK_VER_FILE   = $(ORYX_DIR)/include/network/version.h
NETWORK_VER_PREFIX = NETWORK

NETWORKDEV_MODULE = libamnetwork-dev.so
LOCAL_TARGET := $(NETWORKDEV_MODULE)
LOCAL_SRCS   := $(LOCAL_PATH)/am_network_config.cpp \
                $(LOCAL_PATH)/am_network_dev.cpp
LOCAL_CFLAGS := -I$(PREBUILD_3RD_PARTY_DIR)/iniparser/include \
                -I$(ORYX_DIR)/include \
                -I$(ORYX_DIR)/include/utility \
                -I$(ORYX_DIR)/include/network \
                -std=c++11 -Werror

NETWORKDEV_SO_VER_MAJOR  := $(shell awk '/define\ $(NETWORK_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(NETWORK_VER_FILE))
NETWORKDEV_SO_VER_MINOR  := $(shell awk '/define\ $(NETWORK_VER_PREFIX)_LIB_MINOR/{print $$3}' $(NETWORK_VER_FILE))
NETWORKDEV_SO_VER_PATCH  := $(shell awk '/define\ $(NETWORK_VER_PREFIX)_LIB_PATCH/{print $$3}' $(NETWORK_VER_FILE))
NETWORKDEV_SO_VER_STRING := $(NETWORKDEV_SO_VER_MAJOR).$(NETWORKDEV_SO_VER_MINOR).$(NETWORKDEV_SO_VER_PATCH)

NETWORKDEV_SO_NAME   = $(patsubst %.so,%.so.$(NETWORKDEV_SO_VER_MAJOR), $(NETWORKDEV_MODULE))
NETWORKDEV_SO_TARGET = $(patsubst %.so,%.so.$(NETWORKDEV_SO_VER_STRING), $(NETWORKDEV_MODULE))

LOCAL_SO_NAME := $(NETWORKDEV_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(NETWORKDEV_SO_TARGET)
	@ln -sf $(NETWORKDEV_SO_TARGET) $(ORYX_LIB_DIR)/$(NETWORKDEV_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
