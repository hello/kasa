##
## unit_test/linux/benchmark/mem_test/make.inc
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
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_MEM), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_mem
LOCAL_SRCS	:= $(LOCAL_PATH)/test_mem.c
LOCAL_CFLAGS  := -I$(PREBUILD_3RD_PARTY_DIR)/openssl/include
LOCAL_LDFLAGS := -L$(PREBUILD_3RD_PARTY_DIR)/openssl/usr/lib -lcrypto -ldl

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif