##
## oryx/analytics/qrcode/make.inc
##
## History:
##    2014/12/16 - [Long Li] Created file
##
## Copyright (C) 2008-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_ORYX_ANALYTICS_QRCODE), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

QRCODE_VER_FILE   = $(ORYX_DIR)/include/qrcode/version.h
QRCODE_VER_PREFIX = QRCODE

QRCODE_TARGET  = libamqrcode-detection.so
LOCAL_TARGET := $(QRCODE_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS := -I$(PREBUILD_3RD_PARTY_DIR)/zbar/include \
                -I$(ORYX_DIR)/include/utility \
                -I$(ORYX_DIR)/include/qrcode \
                -I$(ORYX_DIR)/include/video \
                -I$(ORYX_DIR)/analytics/include \
                -I$(ORYX_DIR)/include \
                -std=c++11 -Werror

LOCAL_LDFLAGS := -L$(PREBUILD_3RD_PARTY_DIR)/zbar/usr/lib -lzbar

LOCAL_LIBS := libamutil.so \
              libamvideodsp.so

ifeq ($(BUILD_AMBARELLA_ORYX_DEBUG), y)
LOCAL_CFLAGS += -O0 -g
endif

QRCODE_SO_VER_MAJOR  := $(shell awk '/define\ $(QRCODE_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(QRCODE_VER_FILE))
QRCODE_SO_VER_MINOR  := $(shell awk '/define\ $(QRCODE_VER_PREFIX)_LIB_MINOR/{print $$3}' $(QRCODE_VER_FILE))
QRCODE_SO_VER_PATCH  := $(shell awk '/define\ $(QRCODE_VER_PREFIX)_LIB_PATCH/{print $$3}' $(QRCODE_VER_FILE))
QRCODE_SO_VER_STRING := $(QRCODE_SO_VER_MAJOR).$(QRCODE_SO_VER_MINOR).$(QRCODE_SO_VER_PATCH)

QRCODE_SO_NAME   = $(patsubst %.so,%.so.$(QRCODE_SO_VER_MAJOR), $(QRCODE_TARGET))
QRCODE_SO_TARGET = $(patsubst %.so,%.so.$(QRCODE_SO_VER_STRING), $(QRCODE_TARGET))

LOCAL_SO_NAME := $(QRCODE_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(QRCODE_SO_TARGET)
	@ln -sf $(QRCODE_SO_TARGET) $(ORYX_LIB_DIR)/$(QRCODE_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif
