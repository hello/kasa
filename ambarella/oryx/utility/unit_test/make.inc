##
## camera/modules/configure/make.inc
##
## History:
##    2012/05/30 - [Yupeng Chang] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_ORYX_UTILITY_UNIT_TEST), y)

LOCAL_PATH   := $(call my-dir)
### test_amlog
include $(CLEAR_VARS)

LOCAL_TARGET := test_amlog
LOCAL_SRCS   := $(LOCAL_PATH)/test_am_log_main.cpp \
                $(LOCAL_PATH)/test_am_log.cpp
LOCAL_CFLAGS := -I$(ORYX_DIR)/include         \
                -I$(ORYX_DIR)/include/utility \
                -std=c++11 -Werror
LOCAL_LDFLAGS:= -lpthread -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib

LOCAL_LIBS   := libamutil.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/test_amlog
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

### test_mutex
include $(CLEAR_VARS)

LOCAL_TARGET := test_mutex
LOCAL_SRCS   := $(LOCAL_PATH)/test_mutex.cpp
LOCAL_CFLAGS := -I$(ORYX_DIR)/include         \
                -I$(ORYX_DIR)/include/utility \
                -std=c++11 -Werror
LOCAL_LDFLAGS:= -lpthread -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib

LOCAL_LIBS   := libamutil.so libamosal.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/test_mutex
	@echo "Build $@ Done."


### test_am_pid_lock
include $(CLEAR_VARS)

LOCAL_TARGET := test_am_pid_lock
LOCAL_SRCS   := $(LOCAL_PATH)/test_am_pid_lock.cpp
LOCAL_CFLAGS := -I$(ORYX_DIR)/include         \
                -I$(ORYX_DIR)/include/utility \
                -std=c++11 -Werror
LOCAL_LDFLAGS:= -lpthread -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib

LOCAL_LIBS   := libamutil.so libamosal.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/test_mutex
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif

