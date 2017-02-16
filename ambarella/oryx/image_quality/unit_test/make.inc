##
## oryx/image_quality/unit_test/make.inc
##
## History:
##    2015/1/1 - [Bin Wang] Created file
##
## Copyright (C) 2015-2019, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##


ifeq ($(BUILD_AMBARELLA_ORYX_IMAGE_QUALITY_UNIT_TEST), y)

LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

###############################################################################
LOCAL_TARGET := am_image_quality_test
LOCAL_SRCS   := $(LOCAL_PATH)/am_image_quality_test.cpp
LOCAL_CFLAGS := -I$(ORYX_DIR)/include/utility \
                -I$(ORYX_DIR)/image_quality/include \
                -I$(ORYX_DIR)/include/image_quality \
                -I$(ORYX_DIR)/include \
                -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib  \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/lua/usr/lib
LOCAL_LIBS := libamutil.so libamiq.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
###############################################################################

endif
