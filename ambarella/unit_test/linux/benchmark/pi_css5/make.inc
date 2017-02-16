##
## unit_test/linux/benchmark/pi_css5/make.inc
##
## History:
##    2012/06/19 - [Cao Rongrong] Created file
##
## Copyright (C) 2011-2015, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_PI_CSS5), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= pi_css5
LOCAL_SRCS	:= $(LOCAL_PATH)/pi_fftcs.c $(LOCAL_PATH)/fftsg_h.c

LOCAL_CFLAGS	:= -pedantic -fomit-frame-pointer -funroll-loops
LOCAL_LDFLAGS	:= -lm

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
