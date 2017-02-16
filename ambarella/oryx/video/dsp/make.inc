##
## oryx/video/dsp/make.inc
##
## History:
##    2014/07/25 - [Louis Sun] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH   := $(call my-dir)

## libamvideodsp.so
ifeq ($(BUILD_AMBARELLA_ORYX_VIDEO), y)

include $(CLEAR_VARS)

VIDEODSP_VER_FILE   = $(ORYX_DIR)/include/video/version.h
VIDEODSP_VER_PREFIX = VIDEO

VIDEODSP_TARGET  = libamvideodsp.so
VIDEODSP_CONF := $(ORYX_DIR)/video/etc/vin.acs            \
                 $(ORYX_DIR)/video/etc/vout.acs           \
                 $(ORYX_DIR)/video/etc/stream_fmt.acs     \
                 $(ORYX_DIR)/video/etc/stream_cfg.acs     \
                 $(ORYX_DIR)/video/etc/source_buffer.acs  \
                 $(ORYX_DIR)/video/etc/normal_mode_resource_limit.acs  \
                 $(ORYX_DIR)/video/etc/adv_iso_mode_resource_limit.acs \
                 $(ORYX_DIR)/video/etc/adv_hdr_mode_resource_limit.acs \
                 $(ORYX_DIR)/video/etc/dewarp_mode_resource_limit.acs  \
                 $(ORYX_DIR)/video/etc/features.acs       \
                 $(ORYX_DIR)/video/etc/lbr.acs            \
                 $(ORYX_DIR)/video/etc/dptz_warp.acs

LOCAL_TARGET := $(VIDEODSP_TARGET)
LOCAL_SRCS    := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_LIBS    := libamutil.so libamconfig.so liblbr.so libutils.so libamosd.so libamjpeg-encoder.so libamjpeg-snapshot.so
LOCAL_LDFLAGS := -L$(PREBUILD_3RD_PARTY_DIR)/libjpeg-turbo/usr/lib -ljpeg \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/freetype/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/bzip2/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib \
                 -lpthread

LOCAL_CFLAGS := -I$(ORYX_DIR)/include                             \
                -I$(ORYX_DIR)/include/utility                     \
                -I$(ORYX_DIR)/include/video                       \
                -I$(ORYX_DIR)/include/configure                   \
                -I$(ORYX_DIR)/include/utility/jpeg_snapshot       \
                -I$(ORYX_DIR)/video/include                       \
                -I$(ORYX_DIR)/include/event                       \
                -I$(AMB_TOPDIR)/packages/low_bitrate/include      \
                -I$(AMB_TOPDIR)/packages/utils/                   \
                -I$(AMB_TOPDIR)/packages/dewarp/                  \
                -I$(AMB_TOPDIR)/packages/dewarp/arch_$(AMBARELLA_ARCH)/  \
                -I$(PREBUILD_3RD_PARTY_DIR)/libjpeg-turbo/include \
                -std=c++11 -Werror

ifeq ($(BUILD_AMBARELLA_PACKAGES_PROPRIETARY_SOURCES), y)
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/dewarp -ldewarp
else
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/packages/dewarp/lib/arch_$(AMBARELLA_ARCH) -ldewarp
endif

VIDEODSP_SO_VER_MAJOR  := $(shell awk '/define\ $(VIDEODSP_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(VIDEODSP_VER_FILE))
VIDEODSP_SO_VER_MINOR  := $(shell awk '/define\ $(VIDEODSP_VER_PREFIX)_LIB_MINOR/{print $$3}' $(VIDEODSP_VER_FILE))
VIDEODSP_SO_VER_PATCH  := $(shell awk '/define\ $(VIDEODSP_VER_PREFIX)_LIB_PATCH/{print $$3}' $(VIDEODSP_VER_FILE))
VIDEODSP_SO_VER_STRING := $(VIDEODSP_SO_VER_MAJOR).$(VIDEODSP_SO_VER_MINOR).$(VIDEODSP_SO_VER_PATCH)

VIDEODSP_SO_NAME   = $(patsubst %.so,%.so.$(VIDEODSP_SO_VER_MAJOR), $(VIDEODSP_TARGET))
VIDEODSP_SO_TARGET = $(patsubst %.so,%.so.$(VIDEODSP_SO_VER_STRING), $(VIDEODSP_TARGET))

LOCAL_SO_NAME := $(VIDEODSP_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)  COPY_CONFIG_SCRIPTS

$(LOCAL_TARGET): $(LOCAL_MODULE) COPY_CONFIG_SCRIPTS
ifneq ($(BUILD_AMBARELLA_PACKAGES_PROPRIETARY_SOURCES), y)
	@cp -dpRf $(AMB_TOPDIR)/packages/dewarp/lib/arch_$(AMBARELLA_ARCH)/* $(FAKEROOT_DIR)/usr/lib/
endif
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(VIDEODSP_SO_TARGET)
	@ln -sf $(VIDEODSP_SO_TARGET) $(ORYX_LIB_DIR)/$(VIDEODSP_SO_NAME)
	@echo "Build $@ Done."
	@mkdir -p $(FAKEROOT_DIR)/etc/oryx/video
	@cp -dpRf $(VIDEODSP_CONF) $(FAKEROOT_DIR)/etc/oryx/video/
	@echo "Copy config scripts Done."


$(call add-target-into-build, $(LOCAL_TARGET))

endif

