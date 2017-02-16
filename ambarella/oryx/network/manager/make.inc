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

NETWORK_MODULE = libamnetwork.so
LOCAL_TARGET := $(NETWORK_MODULE)
LOCAL_SRCS   := $(LOCAL_PATH)/am_network_manager_priv.cpp \
                $(LOCAL_PATH)/am_network_manager.cpp
LOCAL_CFLAGS := -I$(PREBUILD_3RD_PARTY_DIR)/iniparser/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/networkmanager/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/networkmanager/include/libnm-glib \
                -I$(PREBUILD_3RD_PARTY_DIR)/networkmanager/include/NetworkManager \
                -I$(PREBUILD_3RD_PARTY_DIR)/glib2/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/glib2/include/glib-2.0 \
                -I$(PREBUILD_3RD_PARTY_DIR)/dbus/include/dbus-1.0 \
                -I$(PREBUILD_3RD_PARTY_DIR)/dbus-glib/include/dbus-1.0 \
                -I$(ORYX_DIR)/include \
                -I$(ORYX_DIR)/include/utility \
                -I$(ORYX_DIR)/include/network \
                -I$(ORYX_DIR)/network/include \
                -std=c++11 -Werror

LOCAL_LDFLAGS :=-L$(PREBUILD_3RD_PARTY_DIR)/networkmanager/usr/lib             \
                -lnm-glib -lnm-util

LOCAL_LIBS := libamnetwork-dev.so

NETWORK_SO_VER_MAJOR  := $(shell awk '/define\ $(NETWORK_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(NETWORK_VER_FILE))
NETWORK_SO_VER_MINOR  := $(shell awk '/define\ $(NETWORK_VER_PREFIX)_LIB_MINOR/{print $$3}' $(NETWORK_VER_FILE))
NETWORK_SO_VER_PATCH  := $(shell awk '/define\ $(NETWORK_VER_PREFIX)_LIB_PATCH/{print $$3}' $(NETWORK_VER_FILE))
NETWORK_SO_VER_STRING := $(NETWORK_SO_VER_MAJOR).$(NETWORK_SO_VER_MINOR).$(NETWORK_SO_VER_PATCH)

NETWORK_SO_NAME   = $(patsubst %.so,%.so.$(NETWORK_SO_VER_MAJOR), $(NETWORK_MODULE))
NETWORK_SO_TARGET = $(patsubst %.so,%.so.$(NETWORK_SO_VER_STRING), $(NETWORK_MODULE))

LOCAL_SO_NAME := $(NETWORK_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(NETWORK_SO_TARGET)
	@ln -sf $(NETWORK_SO_TARGET) $(ORYX_LIB_DIR)/$(NETWORK_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
