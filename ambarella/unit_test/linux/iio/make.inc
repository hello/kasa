##
## unit_test/linux/iio/make.inc
##
## History:
##    2016/06/23 - [Cao Rongrong] Created file
##
## Copyright (C) 2011-2015, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_IIO), y)

LOCAL_PATH	:= $(call my-dir)


###
include $(CLEAR_VARS)

LOCAL_TARGET	:= iio_event_monitor
LOCAL_SRCS	:= $(LOCAL_PATH)/iio_event_monitor.c $(LOCAL_PATH)/iio_utils.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))


###
include $(CLEAR_VARS)

LOCAL_TARGET	:= lsiio
LOCAL_SRCS	:= $(LOCAL_PATH)/lsiio.c $(LOCAL_PATH)/iio_utils.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))


###
include $(CLEAR_VARS)

LOCAL_TARGET	:= iio_generic_buffer
LOCAL_SRCS	:= $(LOCAL_PATH)/iio_generic_buffer.c $(LOCAL_PATH)/iio_utils.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

