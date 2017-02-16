##
## /S2l/oryx/configure/unit_test/make.inc
##
## History:
##   2014-7-30 - [ypchang] created file
##
## Copyright (C) 2008-2014, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

LOCAL_PATH := $(call my-dir)

ifeq ($(BUILD_AMBARELLA_ORYX_CONFIG_UNIT_TEST), y)

include $(CLEAR_VARS)

CONFIG_FILE := $(LOCAL_PATH)/test_config.lua
TEST_CONFIG := test_config
LOCAL_TARGET := $(TEST_CONFIG)
LOCAL_SRCS   := $(LOCAL_PATH)/test_config.cpp

LOCAL_CFLAGS := -I$(ORYX_DIR)/include           \
                -I$(ORYX_DIR)/include/configure \
                -I$(ORYX_DIR)/include/utility   \
                -std=c++11 -Werror
LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib               \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/lua/usr/lib

LOCAL_LIBS := libamconfig.so \
              libamutil.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_CONF_DIR)
	@mkdir -p $(ORYX_BIN_DIR)
	@cp -dpRf $< $(ORYX_BIN_DIR)/$(TEST_CONFIG)
	@cp $(CONFIG_FILE) $(ORYX_CONF_DIR)

$(call add-target-into-build, $(LOCAL_TARGET))

endif
