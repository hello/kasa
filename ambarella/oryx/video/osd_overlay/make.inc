##
## oryx/video/osd_overlay/make.inc
##
## History:
##    2015/07/02 - [Huaiqing Wang] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH   := $(call my-dir)

## libamosd.so
ifeq ($(BUILD_AMBARELLA_ORYX_VIDEO), y)

include $(CLEAR_VARS)

VIDEOOSD_VER_FILE   = $(ORYX_DIR)/include/video/version.h
VIDEOOSD_VER_PREFIX = VIDEO

VIDEOOSD_TARGET  = libamosd.so
VIDEOOSD_CONF := $(ORYX_DIR)/video/etc/osd_overlay.acs

LOCAL_TARGET := $(VIDEOOSD_TARGET)
LOCAL_SRCS    := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_LIBS    := libamutil.so libamconfig.so liblbr.so libutils.so libtextinsert.a
LOCAL_LDFLAGS := -L$(PREBUILD_3RD_PARTY_DIR)/freetype/usr/lib -lfreetype \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/bzip2/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib
LOCAL_CFLAGS := -I$(ORYX_DIR)/include                             \
                -I$(ORYX_DIR)/include/utility                     \
                -I$(ORYX_DIR)/include/video                       \
                -I$(ORYX_DIR)/include/configure                   \
                -I$(ORYX_DIR)/include/utility/jpeg_snapshot       \
                -I$(ORYX_DIR)/video/include                       \
                -I$(ORYX_DIR)/include/event                       \
                -I$(AMB_TOPDIR)/packages/low_bitrate/include      \
                -I$(AMB_TOPDIR)/packages/utils/                   \
                -I$(AMB_TOPDIR)/packages/textinsert/              \
                -I$(PREBUILD_3RD_PARTY_DIR)/freetype/include \
                -std=c++11 -Werror

VIDEOOSD_SO_VER_MAJOR  := $(shell awk '/define\ $(VIDEOOSD_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(VIDEOOSD_VER_FILE))
VIDEOOSD_SO_VER_MINOR  := $(shell awk '/define\ $(VIDEOOSD_VER_PREFIX)_LIB_MINOR/{print $$3}' $(VIDEOOSD_VER_FILE))
VIDEOOSD_SO_VER_PATCH  := $(shell awk '/define\ $(VIDEOOSD_VER_PREFIX)_LIB_PATCH/{print $$3}' $(VIDEOOSD_VER_FILE))
VIDEOOSD_SO_VER_STRING := $(VIDEOOSD_SO_VER_MAJOR).$(VIDEOOSD_SO_VER_MINOR).$(VIDEOOSD_SO_VER_PATCH)

VIDEOOSD_SO_NAME   = $(patsubst %.so,%.so.$(VIDEOOSD_SO_VER_MAJOR), $(VIDEOOSD_TARGET))
VIDEOOSD_SO_TARGET = $(patsubst %.so,%.so.$(VIDEOOSD_SO_VER_STRING), $(VIDEOOSD_TARGET))

LOCAL_SO_NAME := $(VIDEOOSD_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)  COPY_CONFIG_SCRIPTS

$(LOCAL_TARGET): $(LOCAL_MODULE) COPY_CONFIG_SCRIPTS
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(VIDEOOSD_SO_TARGET)
	@ln -sf $(VIDEOOSD_SO_TARGET) $(ORYX_LIB_DIR)/$(VIDEOOSD_SO_NAME)
	@echo "Build $@ Done."
	@mkdir -p $(FAKEROOT_DIR)/etc/oryx/video
	@cp -dpRf $(VIDEOOSD_CONF) $(FAKEROOT_DIR)/etc/oryx/video/
	@mkdir -p $(FAKEROOT_DIR)/usr/share/oryx/video/overlay
	@cp -dpRf $(ORYX_DIR)/video/osd_overlay/data/Ambarella-256x128-8bit.bmp $(FAKEROOT_DIR)/usr/share/oryx/video/overlay/
	@echo "Copy config scripts Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

