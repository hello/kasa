##
## unit_test/linux/audio/make.inc
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

ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_AUDIO), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_audio
LOCAL_SRCS	:= $(LOCAL_PATH)/test_audio.cpp \
			$(LOCAL_PATH)/audio_encode.cpp
LOCAL_CFLAGS	:= -I$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/include \
				-I$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/usr/include \
				-I$(LOCAL_PATH)/alsautils
LOCAL_LDFLAGS	:= -L$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/usr/lib -lasound \
                 -lpthread -lrt -ldl

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

