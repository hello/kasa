##
## /S2l/oryx/stream/record/muxers/muxer-av3/normal/make.inc
##
## History:
##   2016-09-09 - [ccjing] created file
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

### muxer-av3.so
include $(CLEAR_VARS)

MUXER_AV3 = muxer-av3.so
MUXER_AV3_NORMAL_0_CONF = muxer-av3-0.acs
MUXER_AV3_NORMAL_1_CONF = muxer-av3-1.acs

LOCAL_TARGET := $(MUXER_AV3)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_CFLAGS := -std=c++11 -Werror                                          \
                -I$(LOCAL_PATH)                                             \
                -I$(ORYX_DIR)/include                                       \
                -I$(ORYX_DIR)/include/stream                                \
                -I$(ORYX_DIR)/include/configure                             \
                -I$(ORYX_DIR)/include/utility                               \
                -I$(ORYX_DIR)/include/audio                                 \
                -I$(ORYX_DIR)/include/video                                 \
                -I$(ORYX_DIR)/stream/record/muxers/muxer-av3/base           \
                -I$(ORYX_DIR)/stream/include/amf                            \
                -I$(ORYX_DIR)/stream/include/common                         \
                -I$(ORYX_DIR)/stream/include/common/io                      \
                -I$(ORYX_DIR)/stream/include/stream                         \
                -I$(ORYX_DIR)/stream/include/record                         \
                -I$(ORYX_DIR)/stream/include/common/media                   \
                -I$(ORYX_DIR)/stream/include/common/video_param_sets_parser \
                -I$(ORYX_DIR)/stream/include/common/AV3_structure

LOCAL_LDFLAGS := -Wl,-rpath=$(patsubst $(FAKEROOT_DIR)/%,%,$(ORYX_MUXER_DIR))
LOCAL_LIBS := libammuxer-av3-base.so

MUXER_AV3_NAME   = $(MUXER_AV3)
MUXER_AV3_TARGET = $(MUXER_AV3)
LOCAL_SO_NAME := $(MUXER_AV3)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): MUXER_AV3_SRC_PATH := $(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_MUXER_DIR)/
	@mkdir -p $(ORYX_CONF_DIR)/stream/muxer
	@cp -dpRf $(MUXER_AV3_SRC_PATH)/$(MUXER_AV3_NORMAL_0_CONF) $(ORYX_CONF_DIR)/stream/muxer/$(MUXER_AV3_NORMAL_0_CONF)
	@cp -dpRf $(MUXER_AV3_SRC_PATH)/$(MUXER_AV3_NORMAL_1_CONF) $(ORYX_CONF_DIR)/stream/muxer/$(MUXER_AV3_NORMAL_1_CONF)
	@cp -dpRf $(MUXER_AV3_SRC_PATH)/private.pem $(ORYX_CONF_DIR)/stream/muxer/private.pem
	@cp -dpRf $(MUXER_AV3_SRC_PATH)/public.pem $(ORYX_CONF_DIR)/stream/muxer/public.pem
	@cp -dpRf $< $(ORYX_MUXER_DIR)/$(MUXER_AV3_TARGET)
	@ln -sf $(MUXER_AV3) $(ORYX_MUXER_DIR)/muxer-av3-0.so
	@ln -sf $(MUXER_AV3) $(ORYX_MUXER_DIR)/muxer-av3-1.so
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
