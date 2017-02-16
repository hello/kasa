##
## unit_test/linux/i2ctools/tools/make.inc
##
## History:
##    2012/05/31 - [Cao Rongrong] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_I2C_TOOL), y)
LOCAL_PATH	:= $(call my-dir)
I2C_TOOL_PATH	:= $(LOCAL_PATH)
###
include $(CLEAR_VARS)

LOCAL_TARGET	:= i2cset
LOCAL_SRCS	:= $(I2C_TOOL_PATH)/i2cset.c $(I2C_TOOL_PATH)/i2cbusses.c $(I2C_TOOL_PATH)/util.c $(I2C_TOOL_PATH)/ReadWrite16.c
LOCAL_CFLAGS	:= -I$(I2C_TOOL_PATH) -I$(I2C_TOOL_PATH)/../include
LOCAL_CFLAGS	+= -O3

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= i2cget
LOCAL_SRCS	:= $(I2C_TOOL_PATH)/i2cget.c $(I2C_TOOL_PATH)/i2cbusses.c $(I2C_TOOL_PATH)/util.c $(I2C_TOOL_PATH)/ReadWrite16.c
LOCAL_CFLAGS	:= -I$(I2C_TOOL_PATH) -I$(I2C_TOOL_PATH)/../include
LOCAL_CFLAGS	+= -O3

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif