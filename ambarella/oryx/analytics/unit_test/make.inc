##
## oryx/analytics/unit_test/make.inc
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

ifeq ($(BUILD_AMBARELLA_ORYX_ANALYTICS_UNIT_TEST), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

###############################################################################
LOCAL_TARGET := test_qrcode
LOCAL_SRCS   := $(LOCAL_PATH)/test_qrcode.cpp
LOCAL_CFLAGS := -I$(LOCAL_PATH)\
                -I$(ORYX_DIR)/include \
                -I$(ORYX_DIR)/include/qrcode \
                -I$(ORYX_DIR)/include/utility \
                -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/zbar/usr/lib

LOCAL_LIBS    := libamqrcode-detection.so \
                 libamutil.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
