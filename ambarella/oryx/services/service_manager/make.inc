##
## oryx/services/service_manager/make.inc
##
## History:
##    2014/09/09 - [Louis Sun] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##


ifeq ($(BUILD_AMBARELLA_ORYX_SERVICE_MANAGER), y)
LOCAL_PATH   := $(call my-dir)

include $(CLEAR_VARS)

SERVICES_VER_FILE   = $(ORYX_DIR)/services/service_manager/version.h
SERVICES_VER_PREFIX = SERVICE

SERVICES_TARGET  = libamservice-manager.so
LOCAL_TARGET := $(SERVICES_TARGET)
LOCAL_SRCS    := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_LIBS    := libamutil.so
LOCAL_CFLAGS := -I$(ORYX_DIR)/include                             \
                -I$(ORYX_DIR)/include/utility                     \
                -I$(ORYX_DIR)/include/ipc                         \
                -I$(ORYX_DIR)/include/services                    \
                -I$(ORYX_DIR)/services/include                    \
                -std=c++11 -Werror

SERVICES_SO_VER_MAJOR  := $(shell awk '/define\ $(SERVICES_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(SERVICES_VER_FILE))
SERVICES_SO_VER_MINOR  := $(shell awk '/define\ $(SERVICES_VER_PREFIX)_LIB_MINOR/{print $$3}' $(SERVICES_VER_FILE))
SERVICES_SO_VER_PATCH  := $(shell awk '/define\ $(SERVICES_VER_PREFIX)_LIB_PATCH/{print $$3}' $(SERVICES_VER_FILE))
SERVICES_SO_VER_STRING := $(SERVICES_SO_VER_MAJOR).$(SERVICES_SO_VER_MINOR).$(SERVICES_SO_VER_PATCH)

SERVICES_SO_NAME   = $(patsubst %.so,%.so.$(SERVICES_SO_VER_MAJOR), $(SERVICES_TARGET))
SERVICES_SO_TARGET = $(patsubst %.so,%.so.$(SERVICES_SO_VER_STRING), $(SERVICES_TARGET))

LOCAL_SO_NAME := $(SERVICES_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(SERVICES_SO_TARGET)
	@ln -sf $(SERVICES_SO_TARGET) $(ORYX_LIB_DIR)/$(SERVICES_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

