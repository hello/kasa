##
## unit_test/private/iav_test/auto_test/make.inc
##
## History:
##    2014/06/05 - [Jian Tang] Created file
##
## Copyright (C) 2013-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.


ifeq ($(BUILD_AMBARELLA_IAV_AUTO_TEST), y)

AUTO_TEST_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= encode_test
LOCAL_SRCS	:= $(AUTO_TEST_PATH)/encode_test.c
LOCAL_LDFLAGS	:= -lpthread

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $(AUTO_TEST_PATH)/*.sh $(UNIT_TEST_PATH)/
	@chmod +x $(UNIT_TEST_PATH)/*.sh
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
endif