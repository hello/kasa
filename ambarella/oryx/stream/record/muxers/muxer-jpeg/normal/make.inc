##
## /S2l/oryx/stream/record/muxers/muxer-mp4/make.inc
##
## History:
##   2015-1-13 - [ypchang] created file
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

### muxer-jpeg.so
include $(CLEAR_VARS)

MUXER_JPEG = muxer-jpeg.so
MUXER_JPEG_NORMAL_CONF = muxer-jpeg.acs

LOCAL_TARGET := $(MUXER_JPEG)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_CFLAGS := -std=c++11 -Werror                                 \
                -I$(LOCAL_PATH)                                    \
                -I$(ORYX_DIR)/include                              \
                -I$(ORYX_DIR)/include/configure                    \
                -I$(ORYX_DIR)/include/utility                      \
                -I$(ORYX_DIR)/include/video                        \
                -I$(ORYX_DIR)/include/stream                       \
                -I$(ORYX_DIR)/include/image_quality                \
                -I$(ORYX_DIR)/stream/record/muxers/muxer-jpeg/base \
                -I$(ORYX_DIR)/stream/include/amf                   \
                -I$(ORYX_DIR)/stream/include/common                \
                -I$(ORYX_DIR)/stream/include/common/io             \
                -I$(ORYX_DIR)/stream/include/stream                \
                -I$(ORYX_DIR)/stream/include/record                \
                -I$(ORYX_DIR)/stream/include/common/media

LOCAL_LDFLAGS := -Wl,-rpath=$(patsubst $(FAKEROOT_DIR)/%,%,$(ORYX_MUXER_DIR))
LOCAL_LIBS := libammuxer-jpeg-base.so

MUXER_JPEG_NAME   = $(MUXER_JPEG)
MUXER_JPEG_TARGET = $(MUXER_JPEG)
LOCAL_SO_NAME := $(MUXER_JPEG)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): MUXER_JPEG_SRC_PATH := $(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_MUXER_DIR)/
	@mkdir -p $(ORYX_CONF_DIR)/stream/muxer
	@cp -dpRf $(MUXER_JPEG_SRC_PATH)/$(MUXER_JPEG_NORMAL_CONF) $(ORYX_CONF_DIR)/stream/muxer/$(MUXER_JPEG_NORMAL_CONF)
	@cp -dpRf $< $(ORYX_MUXER_DIR)/$(MUXER_JPEG_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
