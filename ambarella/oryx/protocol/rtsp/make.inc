##
## /S2l/oryx/protocol/rtsp/make.inc
##
## History:
##   2015/01/26 - [ypchang] created file
##
## Copyright (C) 2008-2015, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

ifeq ($(BUILD_AMBARELLA_ORYX_PROTOCOL_RTSP), y)
LOCAL_PATH := $(call my-dir)

### libamprotocol-rtsp.so
include $(CLEAR_VARS)

RTSP_SVR_TARGET   = libamprotocol-rtsp.so
RTSP_SVR_CONF     = rtsp_server.acs
RTSP_SVR_VER_FILE = $(ORYX_DIR)/include/protocol/version.h
RTSP_SVR_PREFIX   = RTSP

LOCAL_TARGET := $(RTSP_SVR_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)                             \
                -I$(ORYX_DIR)/include                       \
                -I$(ORYX_DIR)/include/stream                \
                -I$(ORYX_DIR)/include/utility               \
                -I$(ORYX_DIR)/include/protocol              \
                -I$(ORYX_DIR)/include/configure             \
                -I$(ORYX_DIR)/stream/include/amf            \
                -I$(ORYX_DIR)/stream/include/common/media   \
                -I$(PREBUILD_3RD_PARTY_DIR)/openssl/include \
                -std=c++11 -Werror

LOCAL_LIBS   := libamutil.so  \
                libamosal.so  \
                libamconfig.so

LOCAL_LDFLAGS := -L$(PREBUILD_3RD_PARTY_DIR)/openssl/usr/lib -lssl -lcrypto \
                 -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib

RTSP_SVR_SO_VER_MAJ := $(shell awk '/define\ $(RTSP_SVR_PREFIX)_LIB_MAJOR/{print $$3}' $(RTSP_SVR_VER_FILE))
RTSP_SVR_SO_VER_MIN := $(shell awk '/define\ $(RTSP_SVR_PREFIX)_LIB_MINOR/{print $$3}' $(RTSP_SVR_VER_FILE))
RTSP_SVR_SO_VER_PAT := $(shell awk '/define\ $(RTSP_SVR_PREFIX)_LIB_PATCH/{print $$3}' $(RTSP_SVR_VER_FILE))
RTSP_SVR_SO_VER_STR := $(RTSP_SVR_SO_VER_MAJ).$(RTSP_SVR_SO_VER_MIN).$(RTSP_SVR_SO_VER_PAT)

RTSP_SVR_SO_NAME   := $(patsubst %.so,%.so.$(RTSP_SVR_SO_VER_MAJ), $(RTSP_SVR_TARGET))
RTSP_SVR_SO_TARGET := $(patsubst %.so,%.so.$(RTSP_SVR_SO_VER_STR), $(RTSP_SVR_TARGET))

LOCAL_SO_NAME := $(RTSP_SVR_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): prebuild_openssl
$(LOCAL_TARGET): RTSP_SVR_PATH := $(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)
	@mkdir -p $(ORYX_CONF_DIR)/apps/
	@cp -dpRf $(RTSP_SVR_PATH)/$(RTSP_SVR_CONF) $(ORYX_CONF_DIR)/apps/$(RTSP_SVR_CONF)
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(RTSP_SVR_SO_TARGET)
	@ln -sf $(RTSP_SVR_SO_TARGET) $(ORYX_LIB_DIR)/$(RTSP_SVR_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif