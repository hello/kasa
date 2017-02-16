##
## unit_test/private/vsync_test/make.inc
##
## History:
##    2013/12/20 - [BingLiang Hu] Created file
##
## Copyright (C) 2011-2015, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_SHMOO), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

.PHONY: shmoo.sh

shmoo.sh: PRIVATE_PATH:=$(LOCAL_PATH)
shmoo.sh:
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $(PRIVATE_PATH)/shmoo.sh $(UNIT_TEST_PATH)/
	@cp -dpRf $(PRIVATE_PATH)/shmoo_memtester.sh $(UNIT_TEST_PATH)/
	@chmod +x $(FAKEROOT_DIR)/usr/local/bin/shmoo_memtester.sh
	@echo "Copy $@ Done."

$(call add-target-into-build, shmoo.sh)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_shmoo
LOCAL_SRCS	:= $(LOCAL_PATH)/test_shmoo.c
LOCAL_LDFLAGS	:= -lm

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
endif

