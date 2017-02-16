##
## packages/img_mw/dev/arch_s2l/make.inc
##
## History:
##    2014/09/17 - [Bin Wang] Created file
##
## Copyright (C) 2014-2018, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)
AMDEV_TARGET := libamdev.so
LOCAL_BOARD	:= $(shell grep CONFIG_BSP_BOARD $(DOT_CONFIG) | \
        grep -v '^\# CONFIG_BSP_BOARD' | sed -e s/^CONFIG_BSP_BOARD_// | sed -e s/=y//)
LOCAL_BSP_DIR := $(AMB_TOPDIR)/boards/$(shell echo $(LOCAL_BOARD) | tr [:upper:] [:lower:])/bsp
LOCAL_TARGET  := $(AMDEV_TARGET)
LOCAL_SRCS    := $(wildcard $(LOCAL_PATH)/*.c)
LOCAL_CFLAGS  := -I$(LOCAL_BSP_DIR)
LOCAL_SO_NAME := $(AMDEV_TARGET)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/usr/lib/
	@cp -dpRf $< $(FAKEROOT_DIR)/usr/lib/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))