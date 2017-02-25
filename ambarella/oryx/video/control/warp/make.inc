##
## oryx/video/control/warp/make.inc
##
## History:
##   Nov 14, 2015 - [ypchang] created file
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
##

ifeq ($(BUILD_AMBARELLA_VIDEO_PLUGIN_WARP), y)

WARP_PLUGIN_PATH  := $(call my-dir)
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
VPLUGIN_WARP = video-warp.so
VPLUGIN_WARP_CONF = $(WARP_PLUGIN_PATH)/$(AMBARELLA_ARCH)/warp.acs

LOCAL_TARGET := $(VPLUGIN_WARP)
LOCAL_SRCS   := $(wildcard $(WARP_PLUGIN_PATH)/base/*.cpp \
                           $(WARP_PLUGIN_PATH)/$(AMBARELLA_ARCH)/*.cpp)

LOCAL_CFLAGS := -std=c++11 -Werror                      \
                -I$(ORYX_DIR)/include                   \
                -I$(ORYX_DIR)/include/utility           \
                -I$(ORYX_DIR)/include/video             \
                -I$(ORYX_DIR)/include/configure         \
                -I$(ORYX_DIR)/video/include             \
                -I$(WARP_PLUGIN_PATH)/base              \
                -I$(WARP_PLUGIN_PATH)/$(AMBARELLA_ARCH) \
                -I$(AMB_TOPDIR)/packages/utils          \
                -I$(AMB_TOPDIR)/prebuild/ambarella/library/dewarp/include/arch_$(AMBARELLA_ARCH)

LOCAL_LIBS := libamvideo.so libamvideo-platform.so libutils.so

LOCAL_LDFLAGS += -L$(AMB_TOPDIR)/prebuild/ambarella/library/dewarp/lib/arch_$(AMBARELLA_ARCH) -ldewarp

VPLUGIN_WARP_SO_NAME   = $(VPLUGIN_WARP).$(AMBARELLA_ARCH)
VPLUGIN_WARP_SO_TARGET = $(VPLUGIN_WARP)
LOCAL_SO_NAME := $(VPLUGIN_WARP_SO_NAME)

include $(BUILD_APP)

.PHONY: $(VPLUGIN_WARP)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/$(BUILD_AMBARELLA_ORYX_CONF_DIR)/video
	@cp -dpRf $(VPLUGIN_WARP_CONF) $(FAKEROOT_DIR)/$(BUILD_AMBARELLA_ORYX_CONF_DIR)/video
	@mkdir -p $(ORYX_VIDEO_PLUGIN_DIR)
	@cp -dpRf $< $(ORYX_VIDEO_PLUGIN_DIR)/$(VPLUGIN_WARP_SO_TARGET)
	@echo "Build $(VPLUGIN_WARP) Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

