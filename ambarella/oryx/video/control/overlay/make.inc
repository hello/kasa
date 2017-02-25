##
## /S2l/oryx/video/control/overlay/make.inc
##
## History:
##   Mar 28, 2016 - [ypchang] created file
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
##

ifeq ($(BUILD_AMBARELLA_VIDEO_PLUGIN_OVERLAY), y)
OL_PLUGIN_PATH := $(call my-dir)
LOCAL_PATH := $(call my-dir)

VPLUGIN_OL = video-overlay.so
VPLUGIN_OL_CONF_NAME = overlay.acs
VPLUGIN_OL_CONF = $(OL_PLUGIN_PATH)/$(VPLUGIN_OL_CONF_NAME)

OL_DATA = $(OL_PLUGIN_PATH)/Ambarella-256x128-8bit.bmp \
          $(OL_PLUGIN_PATH)/Animation-logo.bmp

OL_DATA_DIR = /usr/share/oryx/video/overlay

LOCAL_TARGET := $(VPLUGIN_OL)
LOCAL_SRCS   := $(wildcard $(OL_PLUGIN_PATH)/*.cpp)

LOCAL_CFLAGS := -std=c++11 -Werror              \
                -I$(ORYX_DIR)/include           \
                -I$(ORYX_DIR)/include/event     \
                -I$(ORYX_DIR)/include/utility   \
                -I$(ORYX_DIR)/include/video     \
                -I$(ORYX_DIR)/include/configure \
                -I$(ORYX_DIR)/video/include     \
                -I$(AMB_TOPDIR)/packages/textinsert

LOCAL_LDFLAGS := -L$(PREBUILD_3RD_PARTY_DIR)/freetype/usr/lib -lfreetype \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/bzip2/usr/lib

LOCAL_LIBS := libamvideo.so          \
              libamconfig.so         \
              libamosal.so           \
              libamvideo-platform.so \
              libtextinsert.a

VPLUGIN_OL_SO_NAME = $(VPLUGIN_OL).$(AMBARELLA_ARCH)
VPLUGIN_OL_SO_TARGET = $(VPLUGIN_OL)
LOCAL_SO_NAME := $(VPLUGIN_OL_SO_NAME)

include $(BUILD_APP)

.PHONY: $(VPLUGIN_OL)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/$(BUILD_AMBARELLA_ORYX_CONF_DIR)/video
	@mkdir -p $(FAKEROOT_DIR)/$(OL_DATA_DIR)
	@cp -dpRf $(VPLUGIN_OL_CONF) $(FAKEROOT_DIR)/$(BUILD_AMBARELLA_ORYX_CONF_DIR)/video
	@sed -i -e "s/@AMBARELLA_ARCH@/$$(echo "$(AMBARELLA_ARCH)" | awk '{print toupper($$0)}')/g" \
	$(FAKEROOT_DIR)/$(BUILD_AMBARELLA_ORYX_CONF_DIR)/video/$(VPLUGIN_OL_CONF_NAME)
	@mkdir -p $(ORYX_VIDEO_PLUGIN_DIR)
	@cp -dpRf $< $(ORYX_VIDEO_PLUGIN_DIR)/$(VPLUGIN_OL_SO_TARGET)
	@cp -dpRf $(OL_DATA) $(FAKEROOT_DIR)/$(OL_DATA_DIR)/
	@echo "Build $(VPLUGIN_OL) Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
