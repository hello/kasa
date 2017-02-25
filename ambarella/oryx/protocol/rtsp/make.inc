##
## /S2l/oryx/protocol/rtsp/make.inc
##
## History:
##   2015/01/26 - [ypchang] created file
##
## Copyright (c) 2015 Ambarella, Inc.
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

ifeq ($(BUILD_AMBARELLA_ORYX_PROTOCOL_RTSP), y)
LOCAL_PATH := $(call my-dir)

### libamprotocol-rtsp.so
include $(CLEAR_VARS)

RTSP_SVR_TARGET   = libamprotocol-rtsp.so
RTSP_SVR_CONF     = rtsp_server.acs
RTSP_SVR_VER_FILE = $(ORYX_DIR)/include/protocol/version.h
RTSP_SVR_PREFIX   = RTSP

LOCAL_TARGET := $(RTSP_SVR_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)                             \
                -I$(ORYX_DIR)/include                       \
                -I$(ORYX_DIR)/include/stream                \
                -I$(ORYX_DIR)/include/utility               \
                -I$(ORYX_DIR)/include/protocol              \
                -I$(ORYX_DIR)/include/configure             \
                -I$(ORYX_DIR)/stream/include/amf            \
                -I$(ORYX_DIR)/stream/include/common/media   \
                -I$(PREBUILD_3RD_PARTY_DIR)/openssl/include \
                -std=c++11 -Werror

LOCAL_LIBS   := libamutil.so  \
                libamosal.so  \
                libamconfig.so

LOCAL_LDFLAGS := -L$(PREBUILD_3RD_PARTY_DIR)/openssl/usr/lib -lssl -lcrypto \
                 -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib

RTSP_SVR_SO_VER_MAJ := $(shell awk '/define\ $(RTSP_SVR_PREFIX)_LIB_MAJOR/{print $$3}' $(RTSP_SVR_VER_FILE))
RTSP_SVR_SO_VER_MIN := $(shell awk '/define\ $(RTSP_SVR_PREFIX)_LIB_MINOR/{print $$3}' $(RTSP_SVR_VER_FILE))
RTSP_SVR_SO_VER_PAT := $(shell awk '/define\ $(RTSP_SVR_PREFIX)_LIB_PATCH/{print $$3}' $(RTSP_SVR_VER_FILE))
RTSP_SVR_SO_VER_STR := $(RTSP_SVR_SO_VER_MAJ).$(RTSP_SVR_SO_VER_MIN).$(RTSP_SVR_SO_VER_PAT)

RTSP_SVR_SO_NAME   := $(patsubst %.so,%.so.$(RTSP_SVR_SO_VER_MAJ), $(RTSP_SVR_TARGET))
RTSP_SVR_SO_TARGET := $(patsubst %.so,%.so.$(RTSP_SVR_SO_VER_STR), $(RTSP_SVR_TARGET))

LOCAL_SO_NAME := $(RTSP_SVR_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): prebuild_openssl
$(LOCAL_TARGET): RTSP_SVR_PATH := $(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)
	@mkdir -p $(ORYX_CONF_DIR)/apps/
	@cp -dpRf $(RTSP_SVR_PATH)/$(RTSP_SVR_CONF) $(ORYX_CONF_DIR)/apps/$(RTSP_SVR_CONF)
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(RTSP_SVR_SO_TARGET)
	@ln -sf $(RTSP_SVR_SO_TARGET) $(ORYX_LIB_DIR)/$(RTSP_SVR_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif