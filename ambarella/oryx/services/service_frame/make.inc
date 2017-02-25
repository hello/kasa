##
## /S2l/oryx/services/service_frame/make.inc
##
## History:
##   2015-1-26 - [ypchang] created file
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

ifeq ($(BUILD_AMBARELLA_ORYX_SERVICE_FRAME), y)
LOCAL_PATH := $(call my-dir)

### libamservice-frame.so
include $(CLEAR_VARS)

SERVICE_FRAME_TARGET   = libamservice-frame.so
SERVICE_FRAME_VER_FILE = $(ORYX_DIR)/services/service_frame/version.h
SERVICE_FRAME_PREFIX   = SERVICE_FRAME

LOCAL_TARGET := $(SERVICE_FRAME_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)                \
                -I$(ORYX_DIR)/include          \
                -I$(ORYX_DIR)/include/utility  \
                -I$(ORYX_DIR)/include/watchdog \
                -I$(ORYX_DIR)/services/include \
                -std=c++11 -Werror

LOCAL_LIBS   := libamutil.so \
                libamosal.so

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib

SERVICE_FRAME_SO_VER_MAJ := $(shell awk '/define\ $(SERVICE_FRAME_PREFIX)_LIB_MAJOR/{print $$3}' $(SERVICE_FRAME_VER_FILE))
SERVICE_FRAME_SO_VER_MIN := $(shell awk '/define\ $(SERVICE_FRAME_PREFIX)_LIB_MINOR/{print $$3}' $(SERVICE_FRAME_VER_FILE))
SERVICE_FRAME_SO_VER_PAT := $(shell awk '/define\ $(SERVICE_FRAME_PREFIX)_LIB_PATCH/{print $$3}' $(SERVICE_FRAME_VER_FILE))
SERVICE_FRAME_SO_VER_STR := $(SERVICE_FRAME_SO_VER_MAJ).$(SERVICE_FRAME_SO_VER_MIN).$(SERVICE_FRAME_SO_VER_PAT)

SERVICE_FRAME_SO_NAME   := $(patsubst %.so,%.so.$(SERVICE_FRAME_SO_VER_MAJ), $(SERVICE_FRAME_TARGET))
SERVICE_FRAME_SO_TARGET := $(patsubst %.so,%.so.$(SERVICE_FRAME_SO_VER_STR), $(SERVICE_FRAME_TARGET))

LOCAL_SO_NAME := $(SERVICE_FRAME_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): SERVICE_FRAME_PATH := $(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(SERVICE_FRAME_SO_TARGET)
	@ln -sf $(SERVICE_FRAME_SO_TARGET) $(ORYX_LIB_DIR)/$(SERVICE_FRAME_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif
