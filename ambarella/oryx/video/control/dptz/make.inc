##
## /S2l/oryx/video/control/dptz/make.inc
##
## History:
##   Mar 28, 2016 - [zfgong] created file
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

ifeq ($(BUILD_AMBARELLA_VIDEO_PLUGIN_DPTZ), y)

DPTZ_PLUGIN_PATH := $(call my-dir)
LOCAL_PATH := $(call my-dir)

VPLUGIN_DPTZ = video-dptz.so

LOCAL_TARGET := $(VPLUGIN_DPTZ)
LOCAL_SRCS   := $(DPTZ_PLUGIN_PATH)/am_dptz.cpp

LOCAL_CFLAGS := -std=c++11 -Werror              \
                -I$(ORYX_DIR)/include           \
                -I$(ORYX_DIR)/include/event     \
                -I$(ORYX_DIR)/include/utility   \
                -I$(ORYX_DIR)/include/video     \
                -I$(ORYX_DIR)/include/configure \
                -I$(ORYX_DIR)/video/include

LOCAL_LIBS := libamvideo.so libamconfig.so libamosal.so

VPLUGIN_DPTZ_SO_NAME = $(VPLUGIN_DPTZ).$(AMBARELLA_ARCH)
VPLUGIN_DPTZ_SO_TARGET = $(VPLUGIN_DPTZ)
LOCAL_SO_NAME := $(VPLUGIN_DPTZ_SO_NAME)

include $(BUILD_APP)

.PHONY: $(VPLUGIN_DPTZ)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_VIDEO_PLUGIN_DIR)
	@cp -dpRf $< $(ORYX_VIDEO_PLUGIN_DIR)/$(VPLUGIN_DPTZ_SO_TARGET)

$(call add-target-into-build, $(LOCAL_TARGET))

endif

