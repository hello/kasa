##
## ambarella/oryx/make.inc
##
## History:
##    2015/01/16 - [Hanbo Xiao] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

## ifeq ($(BUILD_AMBARELLA_ORYX_APP_WIFI_SETUP), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_TARGET := test_cooee
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.c)
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include               \
                 -I$(ORYX_DIR)/include/configure     \
                 -I$(ORYX_DIR)/include/utility       \
                 -I$(LOCAL_PATH)                     \
                 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib                          \
                 -lpthread

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): WIFI_SETUP_SRC_PATH := $(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/

	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

## endif
