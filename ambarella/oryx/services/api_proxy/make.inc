##
## oryx/services/api_proxy/make.inc
##
## History:
##    2014/09/09 - [Louis Sun] Created file
##
## Copyright (c) 2016 Ambarella, Inc.
##
## This file and its contents ("Software") are protected by intellectual
## property rights including, without limitation, U.S. and/or foreign
## copyrights. This Software is also the confidential and proprietary
## information of Ambarella, Inc. and its licensors. You may not use, reproduce,
## disclose, distribute, modify, or otherwise prepare derivative works of this
## Software or any portion thereof except pursuant to a signed license agreement
## or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
## In the absence of such an agreement, you agree to promptly notify and return
## this Software to Ambarella, Inc.
##
## THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
## INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
## MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
## IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
## INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
## (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
## LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
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

