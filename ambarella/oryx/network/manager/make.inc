##
## camera/modules/network/make.inc
##
## History:
##    2015-1-12 - [Tao Wu] created file
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

LOCAL_PATH   := $(call my-dir)

include $(CLEAR_VARS)

NETWORK_VER_FILE   = $(ORYX_DIR)/include/network/version.h
NETWORK_VER_PREFIX = NETWORK

NETWORK_MODULE = libamnetwork.so
LOCAL_TARGET := $(NETWORK_MODULE)
LOCAL_SRCS   := $(LOCAL_PATH)/am_network_manager_priv.cpp \
                $(LOCAL_PATH)/am_network_manager.cpp
LOCAL_CFLAGS := -I$(PREBUILD_3RD_PARTY_DIR)/iniparser/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/NetworkManager/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/NetworkManager/include/libnm-glib \
                -I$(PREBUILD_3RD_PARTY_DIR)/NetworkManager/include/NetworkManager \
                -I$(PREBUILD_3RD_PARTY_DIR)/glib2/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/glib2/include/glib-2.0 \
                -I$(PREBUILD_3RD_PARTY_DIR)/dbus/include/dbus-1.0 \
                -I$(PREBUILD_3RD_PARTY_DIR)/dbus-glib/include/dbus-1.0 \
                -I$(ORYX_DIR)/include \
                -I$(ORYX_DIR)/include/utility \
                -I$(ORYX_DIR)/include/network \
                -I$(ORYX_DIR)/network/include \
                -std=c++11 -Werror

LOCAL_LDFLAGS :=-L$(PREBUILD_3RD_PARTY_DIR)/NetworkManager/usr/lib             \
                -lnm-glib -lnm-util

LOCAL_LIBS := libamnetwork-dev.so

NETWORK_SO_VER_MAJOR  := $(shell awk '/define\ $(NETWORK_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(NETWORK_VER_FILE))
NETWORK_SO_VER_MINOR  := $(shell awk '/define\ $(NETWORK_VER_PREFIX)_LIB_MINOR/{print $$3}' $(NETWORK_VER_FILE))
NETWORK_SO_VER_PATCH  := $(shell awk '/define\ $(NETWORK_VER_PREFIX)_LIB_PATCH/{print $$3}' $(NETWORK_VER_FILE))
NETWORK_SO_VER_STRING := $(NETWORK_SO_VER_MAJOR).$(NETWORK_SO_VER_MINOR).$(NETWORK_SO_VER_PATCH)

NETWORK_SO_NAME   = $(patsubst %.so,%.so.$(NETWORK_SO_VER_MAJOR), $(NETWORK_MODULE))
NETWORK_SO_TARGET = $(patsubst %.so,%.so.$(NETWORK_SO_VER_STRING), $(NETWORK_MODULE))

LOCAL_SO_NAME := $(NETWORK_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(NETWORK_SO_TARGET)
	@ln -sf $(NETWORK_SO_TARGET) $(ORYX_LIB_DIR)/$(NETWORK_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
