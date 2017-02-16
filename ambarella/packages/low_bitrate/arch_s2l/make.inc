##
## packages/lbr/arch_s2l/make.inc
##
## History:
##    2014/02/17 - [Louis Sun] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_LBR_PACKAGE), y)

LOCAL_PATH := $(call my-dir)

###
include $(CLEAR_VARS)
AIR_LIB_DIR   = $(FAKEROOT_DIR)/usr/lib

LBR_TARGET    := liblbr.so
LBR_SO_NAME   := liblbr.so.s2l
LOCAL_TARGET  := $(LBR_TARGET)

LOCAL_SRCS    := $(wildcard $(LOCAL_PATH)/*.c )
LOCAL_CFLAGS  := -I$(LOCAL_PATH)/../include
LOCAL_SO_NAME := $(LBR_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(AIR_LIB_DIR)/
	@cp -dpRf $< $(AIR_LIB_DIR)/$(LBR_SO_NAME)
	@ln -sf $(LBR_SO_NAME) $(AIR_LIB_DIR)/$(LBR_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

