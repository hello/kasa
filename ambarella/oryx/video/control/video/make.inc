##
## /S2l/oryx/video/common/make.inc
##
## History:
##   2015-7-22 - [ypchang] created file
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

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
VIDEO_VER_FILE   = $(ORYX_DIR)/include/video/version.h
VIDEO_VER_PREFIX = VIDEO

VIDEO_PATH = $(ORYX_DIR)/video

VIDEO_TARGET = libamvideo.so

VIDEO_CONFIG  := $(wildcard $(VIDEO_PATH)/config/video/$(AMBARELLA_ARCH)/*.acs)
SENSOR_CONFIG := $(wildcard $(VIDEO_PATH)/config/sensor/*.acs)

LOCAL_TARGET := $(VIDEO_TARGET)
LOCAL_SRCS   := $(wildcard $(VIDEO_PATH)/control/video/*.cpp    \
                           $(VIDEO_PATH)/sensor/*.cpp)

LOCAL_SRCS   += $(wildcard $(VIDEO_PATH)/control/video/$(AMBARELLA_ARCH)/*.cpp)
LOCAL_CFLAGS := -I$(ORYX_DIR)/include               \
                -I$(ORYX_DIR)/include/utility       \
                -I$(ORYX_DIR)/include/video         \
                -I$(ORYX_DIR)/include/configure     \
                -I$(VIDEO_PATH)/include             \
                -I$(AMB_TOPDIR)/packages/utils      \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so           \
              libamosal.so           \
              libamconfig.so         \
              libamvideo-platform.so
VIDEO_SO_VER_MAJOR  := $(shell awk '/define\ $(VIDEO_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(VIDEO_VER_FILE))
VIDEO_SO_VER_MINOR  := $(shell awk '/define\ $(VIDEO_VER_PREFIX)_LIB_MINOR/{print $$3}' $(VIDEO_VER_FILE))
VIDEO_SO_VER_PATCH  := $(shell awk '/define\ $(VIDEO_VER_PREFIX)_LIB_PATCH/{print $$3}' $(VIDEO_VER_FILE))
VIDEO_SO_VER_STRING := $(VIDEO_SO_VER_MAJOR).$(VIDEO_SO_VER_MINOR).$(VIDEO_SO_VER_PATCH)

VIDEO_SO_NAME   = $(patsubst %.so,%.so.$(VIDEO_SO_VER_MAJOR), $(VIDEO_TARGET))
VIDEO_SO_TARGET = $(patsubst %.so,%.so.$(VIDEO_SO_VER_STRING), $(VIDEO_TARGET))

LOCAL_SO_NAME := $(VIDEO_SO_NAME)

include $(BUILD_APP)

.PHONY: $(VIDEO_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(VIDEO_SO_TARGET)
	@ln -sf $(VIDEO_SO_TARGET) $(ORYX_LIB_DIR)/$(VIDEO_SO_NAME)
	@echo "Build $@ Done."
	@mkdir -p $(FAKEROOT_DIR)/etc/oryx/video
	@mkdir -p $(FAKEROOT_DIR)/etc/oryx/sensor
	@cp -dpRf $(VIDEO_CONFIG) $(FAKEROOT_DIR)/etc/oryx/video/
	@cp -dpRf $(SENSOR_CONFIG) $(FAKEROOT_DIR)/etc/oryx/sensor/
	@echo "Copy config scripts Done."

$(call add-target-into-build, $(LOCAL_TARGET))
