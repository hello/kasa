##
## unit_test/linux/benchmark/nbench/make.inc
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
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_NBENCH), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= nbench
LOCAL_SRCS	:= $(LOCAL_PATH)/emfloat.c \
			$(LOCAL_PATH)/misc.c \
			$(LOCAL_PATH)/nbench0.c \
			$(LOCAL_PATH)/nbench1.c \
			$(LOCAL_PATH)/sysspec.c \
			$(LOCAL_PATH)/hardware.c
LOCAL_CFLAGS	:=-DLINUX -s -static -Wall -O3
LOCAL_LDFLAGS	:=-lm

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
