##
## oryx/audio/device/make.inc
##
## History:
##    2014/12/15 - [Shupeng Ren] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH   := $(call my-dir)

include $(CLEAR_VARS)

RTMP_BLS_VER_FILE   = $(LOCAL_PATH)/version.h
RTMP_BLS_VER_PREFIX = RTMP_BLS

RTMP_BLS_TARGET  = librtmp-bls.so
LOCAL_TARGET    := $(RTMP_BLS_TARGET)
LOCAL_SRCS      :=
LOCAL_CFLAGS    := -Werror
LOCAL_LIBS      :=

LOCAL_LDFLAGS := -Wl,--whole-archive -L$(LOCAL_PATH) -lrtmp -Wl,--no-whole-archive

RTMP_BLS_SO_VER_MAJOR  := $(shell awk '/define\ $(RTMP_BLS_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(RTMP_BLS_VER_FILE))
RTMP_BLS_SO_VER_MINOR  := $(shell awk '/define\ $(RTMP_BLS_VER_PREFIX)_LIB_MINOR/{print $$3}' $(RTMP_BLS_VER_FILE))
RTMP_BLS_SO_VER_PATCH  := $(shell awk '/define\ $(RTMP_BLS_VER_PREFIX)_LIB_PATCH/{print $$3}' $(RTMP_BLS_VER_FILE))
RTMP_BLS_SO_VER_STRING := $(RTMP_BLS_SO_VER_MAJOR).$(RTMP_BLS_SO_VER_MINOR).$(RTMP_BLS_SO_VER_PATCH)

RTMP_BLS_SO_NAME   = $(patsubst %.so,%.so.$(RTMP_BLS_SO_VER_MAJOR), $(RTMP_BLS_TARGET))
RTMP_BLS_SO_TARGET = $(patsubst %.so,%.so.$(RTMP_BLS_SO_VER_STRING), $(RTMP_BLS_TARGET))

LOCAL_SO_NAME := $(RTMP_BLS_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(RTMP_BLS_SO_TARGET)
	@ln -sf $(RTMP_BLS_SO_TARGET) $(ORYX_LIB_DIR)/$(RTMP_BLS_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
