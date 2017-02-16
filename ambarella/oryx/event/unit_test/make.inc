##
## oryx/event/unit_test/make.inc
##
## History:
##    2014/11/21 - [Dongge Wu] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##


ifeq ($(BUILD_AMBARELLA_ORYX_EVENTMONITOR), y)

LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

###############################################################################
LOCAL_TARGET := test_event_plugin

ifeq ($(BUILD_AMBARELLA_ORYX_VIDEO_NEW), y)
LOCAL_SRCS   := $(LOCAL_PATH)/test_event_plugin_new.cpp
else
LOCAL_SRCS   := $(LOCAL_PATH)/test_event_plugin.cpp
endif


LOCAL_CFLAGS := -I$(ORYX_DIR)/include/utility \
                -I$(ORYX_DIR)/event/include \
                -I$(ORYX_DIR)/include/event \
                -I$(ORYX_DIR)/include/utility/jpeg_encoder \
                -I$(ORYX_DIR)/include
ifeq ($(BUILD_AMBARELLA_ORYX_VIDEO_NEW), y)
LOCAL_CFLAGS += -I$(ORYX_DIR)/include/video_new
LOCAL_CFLAGS += -I$(ORYX_DIR)/video_new/include
else
LOCAL_CFLAGS += -I$(ORYX_DIR)/include/video
endif
LOCAL_CFLAGS += -std=c++11 -Werror

LOCAL_LDFLAGS := -ldl \
                 -lpthread

LOCAL_LDFLAGS += -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib

LOCAL_LIBS := libamutil.so libamosal.so libamevent-monitor.so libamjpeg-encoder.so
ifeq ($(BUILD_AMBARELLA_ORYX_VIDEO_NEW), y)
LOCAL_LIBS += libamvideo-reader.so libamvideo-address.so
else
LOCAL_LIBS += libamvideodsp.so
endif

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
###############################################################################

endif
