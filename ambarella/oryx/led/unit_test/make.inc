##
## /S2L/oryx/led/unit_test/make.inc
##
## History:
##   2015-2-9 - [longli] created file
##
## Copyright (C) 2008-2015, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

ifeq ($(BUILD_AMBARELLA_ORYX_LED_UNIT_TEST), y)

LOCAL_PATH   := $(call my-dir)
###
include $(CLEAR_VARS)

LOCAL_BOARD	:= $(shell grep CONFIG_BSP_BOARD $(DOT_CONFIG) | \
        grep -v '^\# CONFIG_BSP_BOARD' | sed -e s/^CONFIG_BSP_BOARD_// | \
        sed -e s/=y//)
LOCAL_BSP_DIR	:= $(AMB_TOPDIR)/boards/$(shell echo $(LOCAL_BOARD) | \
        tr [:upper:] [:lower:])/bsp

LOCAL_TARGET := test_led_handler
LOCAL_SRCS   := $(LOCAL_PATH)/test_led_handler.cpp
LOCAL_CFLAGS := -I$(LOCAL_PATH)\
                -I$(ORYX_DIR)/include \
                -I$(ORYX_DIR)/include/led \
                -I$(ORYX_DIR)/include/utility \
                -I$(LOCAL_BSP_DIR) \
                -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib

LOCAL_LIBS    := libamutil.so \
                 libamled.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif