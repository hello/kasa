##
## oryx/services/video_service/make.inc
##
## History:
##    2014/09/09 - [Louis Sun] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##


ifeq ($(BUILD_AMBARELLA_ORYX_SERVICE_VIDEO), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

###############################################################################
LOCAL_TARGET := video_svc
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include             \
                 -I$(ORYX_DIR)/include/ipc         \
                 -I$(ORYX_DIR)/include/utility     \
                 -I$(ORYX_DIR)/include/services    \
                 -I$(ORYX_DIR)/include/video       \
                 -I$(ORYX_DIR)/include/event       \
                 -I$(ORYX_DIR)/include/utility/jpeg_snapshot \
                 -I$(ORYX_DIR)/video/include       \
                 -I$(ORYX_DIR)/services/include    \
                 -I$(AMB_TOPDIR)/packages/utils/                   \
                 -I$(AMB_TOPDIR)/packages/low_bitrate/include  \
                 -I$(LOCAL_PATH)                   \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib \
                 -Wl,-rpath-link=$(AMB_TOPDIR)/packages/dewarp/lib/arch_$(AMBARELLA_ARCH) \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/freetype/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/bzip2/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib

LOCAL_LIBS    := libamutil.so           \
                 libamipc.so            \
                 libamservice-frame.so  \
                 libamvideodsp.so \
                 libamosd.so \
                 libamjpeg-snapshot.so \
                 libamjpeg-encoder.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@cp -dpRf $(AMB_TOPDIR)/packages/dewarp/lib/arch_$(AMBARELLA_ARCH)/* $(FAKEROOT_DIR)/usr/lib/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
