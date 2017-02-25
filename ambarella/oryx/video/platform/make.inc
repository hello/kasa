##
## /S2l/oryx/video/platform/make.inc
##
## History:
##   Aug 12, 2015 - [ypchang] created file
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
PLATFORM_PATH := $(call my-dir)
VIDEO_PATH = $(ORYX_DIR)/video

include $(CLEAR_VARS)

PLATFORM_TARGET := libamvideo-platform.so

ifeq ($(CONFIG_AMBARELLA_ORYX_VIDEO_IAV1), y)
IAV_PATH := $(PLATFORM_PATH)/iav1
IAV_SO_NAME := $(PLATFORM_TARGET).iav1
PLATFORM_CONFIG := $(wildcard $(IAV_PATH)/config/$(AMBARELLA_ARCH)/*.acs)
endif

ifeq ($(CONFIG_AMBARELLA_ORYX_VIDEO_IAV2), y)
IAV_PATH := $(PLATFORM_PATH)/iav2
IAV_SO_NAME := $(PLATFORM_TARGET).iav2
PLATFORM_CONFIG := $(wildcard $(IAV_PATH)/config/$(AMBARELLA_ARCH)/*.acs)
endif

ifeq ($(CONFIG_AMBARELLA_ORYX_VIDEO_IAV3), y)
IAV_PATH := $(PLATFORM_PATH)/iav3
IAV_SO_NAME := $(PLATFORM_TARGET).iav3
PLATFORM_CONFIG := $(wildcard $(IAV_PATH)/config/$(AMBARELLA_ARCH)/*.acs)
endif

LOCAL_TARGET := $(PLATFORM_TARGET)
LOCAL_SRCS   := $(wildcard $(IAV_PATH)/*.cpp    \
                           $(VIDEO_PATH)/utility/*.cpp)
LOCAL_CFLAGS := -I$(IAV_PATH)                   \
                -I$(ORYX_DIR)/include           \
                -I$(ORYX_DIR)/include/utility   \
                -I$(ORYX_DIR)/include/configure \
                -I$(ORYX_DIR)/include/video     \
                -I$(ORYX_DIR)/video/include     \
                -std=c++11 -Werror

LOCAL_SO_NAME := $(IAV_SO_NAME)

LOCAL_LIBS := libamconfig.so

include $(BUILD_APP)

.PHONY: $(PLATFORM_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(IAV_SO_NAME)
	@echo "Build $@ Done."
	@mkdir -p $(FAKEROOT_DIR)/etc/oryx/video
	@cp -dpRf $(PLATFORM_CONFIG) $(FAKEROOT_DIR)/etc/oryx/video/

$(call add-target-into-build, $(LOCAL_TARGET))
