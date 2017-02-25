##
## unit_test/linux/benchmark/bandwidth/make.inc
##
## History:
##    2012/05/31 - [Cao Rongrong] Created file
##
## Copyright (C) 2011-2015, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_BANDWIDTH), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= bandwidth-arm
ifeq ($(CPU_ARCH), arm64)
LOCAL_SRCS	:= $(LOCAL_PATH)/routinesARM64.S
else
LOCAL_SRCS	:= $(LOCAL_PATH)/routinesARM.S
endif
LOCAL_SRCS	+= $(LOCAL_PATH)/main.c $(LOCAL_PATH)/BMP.c
LOCAL_CFLAGS	:= -O3

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= bandwidth-arm-thread
ifeq ($(CPU_ARCH), arm64)
LOCAL_SRCS	:= $(LOCAL_PATH)/routinesARM64.S
else
LOCAL_SRCS	:= $(LOCAL_PATH)/routinesARM.S
endif
LOCAL_SRCS	+= $(LOCAL_PATH)/main_thread.c $(LOCAL_PATH)/BMP.c
LOCAL_CFLAGS	:= -O3
LOCAL_LDFLAGS	:= -lpthread

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
