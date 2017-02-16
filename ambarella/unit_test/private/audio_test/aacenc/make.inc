##
## unit_test/private/audio_test/aacenc/make.inc
##
## History:
##    2015/03/03 - [Zhaoyang Chen] Created file
##
## Copyright (C) 2015-2019, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_aacenc
LOCAL_SRCS	:= $(LOCAL_PATH)/test_aacenc.c
LOCAL_CFLAGS	:= -I$(PREBUILD_3RD_PARTY_DIR)/aac/include
LOCAL_LDFLAGS	:= -L$(PREBUILD_3RD_PARTY_DIR)/aac/lib -laacenc

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

