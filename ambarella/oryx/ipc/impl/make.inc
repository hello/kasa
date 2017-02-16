##
## oryx/ipc/make.inc
##
## History:
##    2014/09/05 - [Louis Sun] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH   := $(call my-dir)

## libamipc.so
ifeq ($(BUILD_AMBARELLA_ORYX_IPC), y)

include $(CLEAR_VARS)

IPC_VER_FILE   = $(ORYX_DIR)/include/ipc/version.h
IPC_VER_PREFIX = IPC

IPC_TARGET  = libamipc.so
LOCAL_TARGET := $(IPC_TARGET)
LOCAL_SRCS    := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_LIBS    := libamutil.so
LOCAL_CFLAGS := -I$(ORYX_DIR)/include                             \
                -I$(ORYX_DIR)/include/utility                     \
                -I$(ORYX_DIR)/include/ipc                         \
                -std=c++11 -Werror
LOCAL_LDFLAGS := -lrt

IPC_SO_VER_MAJOR  := $(shell awk '/define\ $(IPC_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(IPC_VER_FILE))
IPC_SO_VER_MINOR  := $(shell awk '/define\ $(IPC_VER_PREFIX)_LIB_MINOR/{print $$3}' $(IPC_VER_FILE))
IPC_SO_VER_PATCH  := $(shell awk '/define\ $(IPC_VER_PREFIX)_LIB_PATCH/{print $$3}' $(IPC_VER_FILE))
IPC_SO_VER_STRING := $(IPC_SO_VER_MAJOR).$(IPC_SO_VER_MINOR).$(IPC_SO_VER_PATCH)

IPC_SO_NAME   = $(patsubst %.so,%.so.$(IPC_SO_VER_MAJOR), $(IPC_TARGET))
IPC_SO_TARGET = $(patsubst %.so,%.so.$(IPC_SO_VER_STRING), $(IPC_TARGET))

LOCAL_SO_NAME := $(IPC_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(IPC_SO_TARGET)
	@ln -sf $(IPC_SO_TARGET) $(ORYX_LIB_DIR)/$(IPC_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

