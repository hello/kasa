##
## oryx/services/api_proxy/make.inc
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


ifeq ($(BUILD_AMBARELLA_ORYX_SERVICE_API_PROXY), y)
LOCAL_PATH   := $(call my-dir)

include $(CLEAR_VARS)

API_PROXY_VER_FILE   = $(ORYX_DIR)/services/api_proxy/version.h
API_PROXY_VER_PREFIX = API_PROXY

API_PROXY_TARGET  = libamapxy.so
LOCAL_TARGET := $(API_PROXY_TARGET)
LOCAL_SRCS    := $(LOCAL_PATH)/am_api_proxy.cpp
LOCAL_LIBS    := libamutil.so
LOCAL_CFLAGS := -I$(ORYX_DIR)/include                             \
                -I$(ORYX_DIR)/include/utility                     \
                -I$(ORYX_DIR)/include/ipc                         \
                -I$(ORYX_DIR)/include/services                    \
                -I$(ORYX_DIR)/services/include                    \
                -std=c++11 -Werror

API_PROXY_SO_VER_MAJOR  := $(shell awk '/define\ $(API_PROXY_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(API_PROXY_VER_FILE))
API_PROXY_SO_VER_MINOR  := $(shell awk '/define\ $(API_PROXY_VER_PREFIX)_LIB_MINOR/{print $$3}' $(API_PROXY_VER_FILE))
API_PROXY_SO_VER_PATCH  := $(shell awk '/define\ $(API_PROXY_VER_PREFIX)_LIB_PATCH/{print $$3}' $(API_PROXY_VER_FILE))
API_PROXY_SO_VER_STRING := $(API_PROXY_SO_VER_MAJOR).$(API_PROXY_SO_VER_MINOR).$(API_PROXY_SO_VER_PATCH)

API_PROXY_SO_NAME   = $(patsubst %.so,%.so.$(API_PROXY_SO_VER_MAJOR), $(API_PROXY_TARGET))
API_PROXY_SO_TARGET = $(patsubst %.so,%.so.$(API_PROXY_SO_VER_STRING), $(API_PROXY_TARGET))

LOCAL_SO_NAME := $(API_PROXY_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(API_PROXY_SO_TARGET)
	@ln -sf $(API_PROXY_SO_TARGET) $(ORYX_LIB_DIR)/$(API_PROXY_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

########################################################################

API_PROXY_VER_FILE   = $(ORYX_DIR)/services/api_proxy/version.h
API_PROXY_VER_PREFIX = API_PROXY

AIR_API_TARGET  = libamapi.so
LOCAL_TARGET := $(AIR_API_TARGET)
LOCAL_SRCS    := $(LOCAL_PATH)/am_api_helper.cpp
LOCAL_LIBS    := libamutil.so
LOCAL_CFLAGS := -I$(ORYX_DIR)/include                             \
                -I$(ORYX_DIR)/include/utility                     \
                -I$(ORYX_DIR)/include/ipc                         \
                -I$(ORYX_DIR)/include/services                    \
                -I$(ORYX_DIR)/services/include                    \
                -std=c++11 -Werror

API_PROXY_SO_VER_MAJOR  := $(shell awk '/define\ $(API_PROXY_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(API_PROXY_VER_FILE))
API_PROXY_SO_VER_MINOR  := $(shell awk '/define\ $(API_PROXY_VER_PREFIX)_LIB_MINOR/{print $$3}' $(API_PROXY_VER_FILE))
API_PROXY_SO_VER_PATCH  := $(shell awk '/define\ $(API_PROXY_VER_PREFIX)_LIB_PATCH/{print $$3}' $(API_PROXY_VER_FILE))
API_PROXY_SO_VER_STRING := $(API_PROXY_SO_VER_MAJOR).$(API_PROXY_SO_VER_MINOR).$(API_PROXY_SO_VER_PATCH)

AIR_API_SO_NAME   = $(patsubst %.so,%.so.$(API_PROXY_SO_VER_MAJOR), $(AIR_API_TARGET))
AIR_API_SO_TARGET = $(patsubst %.so,%.so.$(API_PROXY_SO_VER_STRING), $(AIR_API_TARGET))

LOCAL_SO_NAME := $(AIR_API_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(AIR_API_SO_TARGET)
	@ln -sf $(AIR_API_SO_TARGET) $(ORYX_LIB_DIR)/$(AIR_API_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

