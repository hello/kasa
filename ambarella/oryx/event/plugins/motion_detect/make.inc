##
## oryx/event/plugin/motion_detect/make.inc
##
## History:
##    Dec 11, 2014 - [binwang] created file
##
## Copyright (C) 2014-2018, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH := $(call my-dir)

ifeq ($(BUILD_AMBARELLA_ORYX_EVENT_PLUGIN_MOTION_DETECT), y)

include $(CLEAR_VARS)

EVENT_MOTION_DETECT_CONFIG = event-motion-detect.acs
MOTION_DETECT_VER_FILE = $(ORYX)/include/event/version.h
MOTION_DETECT_VER_PREFIX = MOTION_DETECT

MOTION_DETECT_TARGET = event-motion-detect.so
LOCAL_TARGET := $(MOTION_DETECT_TARGET)
LOCAL_SRCS   := $(LOCAL_PATH)/am_motion_detect_config.cpp

ifeq ($(BUILD_AMBARELLA_ORYX_VIDEO_NEW), y)
LOCAL_SRCS  += $(LOCAL_PATH)/am_motion_detect_new.cpp
else
LOCAL_SRCS  += $(LOCAL_PATH)/am_motion_detect.cpp
endif

LOCAL_CFLAGS := -I$(ORYX_DIR)/include/utility                      \
                -I$(ORYX_DIR)/event/include                        \
                -I$(ORYX_DIR)/include/event                        \
                -I$(ORYX_DIR)/include/configure                    \
                -I$(ORYX_DIR)/include                              \
                -I$(AMB_TOPDIR)/packages/smartcam/mdet_lib/include

ifeq ($(BUILD_AMBARELLA_ORYX_VIDEO_NEW), y)
LOCAL_CFLAGS += -I$(ORYX_DIR)/include/video_new
else
LOCAL_CFLAGS += -I$(ORYX_DIR)/include/video
endif
LOCAL_CFLAGS += -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib  \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/freetype/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/bzip2/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib

LOCAL_LIBS := libamutil.so libamosal.so

ifeq ($(BUILD_AMBARELLA_ORYX_VIDEO_NEW), y)
LOCAL_LIBS += libamvideo-reader.so libamvideo-address.so
else
LOCAL_LIBS += libamvideodsp.so
endif

ifeq ($(BUILD_AMBARELLA_PACKAGES_PROPRIETARY_SOURCES), y)
LOCAL_LIBS += libmdet.so
else
LOCAL_LDFLAGS += -L$(AMB_TOPDIR)/packages/smartcam/mdet_lib/lib -lmdet
endif

MOTION_DETECT_SO_NAME   = $(MOTION_DETECT_TARGET)
MOTION_DETECT_SO_TARGET = $(MOTION_DETECT_SO_NAME)

LOCAL_SO_NAME := $(MOTION_DETECT_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): MOTION_DETECT_PATH := $(LOCAL_PATH)/
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_EVENT_PLUGIN_DIR)/
	@mkdir -p $(FAKEROOT_DIR)/$(BUILD_AMBARELLA_ORYX_CONF_DIR)/event/
ifneq ($(BUILD_AMBARELLA_PACKAGES_PROPRIETARY_SOURCES), y)
	@cp -dpRf $(AMB_TOPDIR)/packages/smartcam/mdet_lib/lib/* $(FAKEROOT_DIR)/usr/lib/
endif
	@cp -dpRf $(MOTION_DETECT_PATH)/$(EVENT_MOTION_DETECT_CONFIG) \
		$(FAKEROOT_DIR)/$(BUILD_AMBARELLA_ORYX_CONF_DIR)/event/$(EVENT_MOTION_DETECT_CONFIG)
	@cp -dpRf $< $(ORYX_EVENT_PLUGIN_DIR)/$(MOTION_DETECT_SO_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
