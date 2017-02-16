##
## utility/jpeg_encoder/make.inc
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

## libamjpeg-encoder.so
ifeq ($(BUILD_AMBARELLA_ORYX_JPEG_ENCODER), y)

include $(CLEAR_VARS)

JPEG_ENCODER_VER_FILE   = $(ORYX_DIR)/include/utility/jpeg_encoder/version.h
JPEG_ENCODER_VER_PREFIX = JPEG_ENCODER

JPEG_ENCODER_TARGET  = libamjpeg-encoder.so
LOCAL_TARGET := $(JPEG_ENCODER_TARGET)
LOCAL_SRCS    := $(LOCAL_PATH)/am_jpeg_encoder.cpp \
                 $(LOCAL_PATH)/chrome_convert.S

LOCAL_LDFLAGS := -L$(PREBUILD_3RD_PARTY_DIR)/libjpeg-turbo/usr/lib -ljpeg \
                 -lpthread

LOCAL_CFLAGS := -I$(ORYX_DIR)/include                             \
                -I$(ORYX_DIR)/include/utility                     \
                -I$(ORYX_DIR)/utility/include                     \
                -I$(ORYX_DIR)/include/utility/jpeg_encoder         \
                -I$(PREBUILD_3RD_PARTY_DIR)/libjpeg-turbo/include \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so

JPEG_ENCODER_SO_VER_MAJOR  := $(shell awk '/define\ $(JPEG_ENCODER_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(JPEG_ENCODER_VER_FILE))
JPEG_ENCODER_SO_VER_MINOR  := $(shell awk '/define\ $(JPEG_ENCODER_VER_PREFIX)_LIB_MINOR/{print $$3}' $(JPEG_ENCODER_VER_FILE))
JPEG_ENCODER_SO_VER_PATCH  := $(shell awk '/define\ $(JPEG_ENCODER_VER_PREFIX)_LIB_PATCH/{print $$3}' $(JPEG_ENCODER_VER_FILE))
JPEG_ENCODER_SO_VER_STRING := $(JPEG_ENCODER_SO_VER_MAJOR).$(JPEG_ENCODER_SO_VER_MINOR).$(JPEG_ENCODER_SO_VER_PATCH)

JPEG_ENCODER_SO_NAME   = $(patsubst %.so,%.so.$(JPEG_ENCODER_SO_VER_MAJOR), $(JPEG_ENCODER_TARGET))
JPEG_ENCODER_SO_TARGET = $(patsubst %.so,%.so.$(JPEG_ENCODER_SO_VER_STRING), $(JPEG_ENCODER_TARGET))

LOCAL_SO_NAME := $(JPEG_ENCODER_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(JPEG_ENCODER_SO_TARGET)
	@ln -sf $(JPEG_ENCODER_SO_TARGET) $(ORYX_LIB_DIR)/$(JPEG_ENCODER_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
