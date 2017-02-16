##
## oryx/video/unit_test/make.inc
##
## History:
##    2014/06/24 - [Louis Sun] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##


ifeq ($(BUILD_AMBARELLA_ORYX_VIDEO_UNIT_TEST), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

###############################################################################
LOCAL_TARGET := am_video_camera_test
LOCAL_SRCS   := $(LOCAL_PATH)/am_video_camera_test.cpp
LOCAL_CFLAGS :=  -I$(LOCAL_PATH)               \
                 -I$(ORYX_DIR)/include         \
                 -I$(ORYX_DIR)/include/video   \
                 -I$(ORYX_DIR)/include/utility \
                 -I$(LOCAL_PATH)             \
                 -I$(ORYX_DIR)/video/include \
                 -I$(AMB_TOPDIR)/packages/utils/                   \
                 -I$(AMB_TOPDIR)/packages/dewarp/                  \
                 -I$(AMB_TOPDIR)/packages/dewarp/arch_$(AMBARELLA_ARCH)/  \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib  \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/lua/usr/lib \
                 -Wl,-rpath-link=$(AMB_TOPDIR)/packages/dewarp/lib/arch_$(AMBARELLA_ARCH) \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/freetype/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/bzip2/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib

LOCAL_LIBS    := libamvideodsp.so libamosd.so libamutil.so libamjpeg-snapshot.so libamjpeg-encoder.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@cp -dpRf $(AMB_TOPDIR)/packages/dewarp/lib/arch_$(AMBARELLA_ARCH)/* $(FAKEROOT_DIR)/usr/lib/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###############################################################################

LOCAL_TARGET := am_video_stream_test
LOCAL_SRCS   := $(LOCAL_PATH)/am_video_stream_test.cpp
LOCAL_CFLAGS := -I$(LOCAL_PATH)               \
                -I$(ORYX_DIR)/include         \
                -I$(ORYX_DIR)/include/video   \
                -I$(ORYX_DIR)/include/utility \
                -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib \
                 -Wl,-rpath-link=$(AMB_TOPDIR)/packages/dewarp/lib/arch_$(AMBARELLA_ARCH)

LOCAL_LIBS    := libamvideodsp.so libamutil.so libamjpeg-snapshot.so libamjpeg-encoder.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@cp -dpRf $(AMB_TOPDIR)/packages/dewarp/lib/arch_$(AMBARELLA_ARCH)/* $(FAKEROOT_DIR)/usr/lib/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
