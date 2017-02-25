##
## /S2l/oryx/stream/playback/filters/player/make.inc
##
## History:
##   2014-9-22 - [ypchang] created file
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

PLAYER_TARGET = filter-player.so
PLAYER_CONF   = filter-player.acs

LOCAL_TARGET := $(PLAYER_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)                                \
                -I$(ORYX_DIR)/include                          \
                -I$(ORYX_DIR)/include/audio                    \
                -I$(ORYX_DIR)/include/stream                   \
                -I$(ORYX_DIR)/include/utility                  \
                -I$(ORYX_DIR)/include/configure                \
                -I$(ORYX_DIR)/include/audio/player             \
                -I$(ORYX_DIR)/stream/include/amf               \
                -I$(ORYX_DIR)/stream/include/common            \
                -I$(ORYX_DIR)/stream/include/playback          \
                -I$(PREBUILD_3RD_PARTY_DIR)/pulseaudio/include \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so                \
              libamosal.so                \
              libamconfig.so              \
              libamstream-iid-playback.so \
              libamstream-amf.so          \
              libamaudio-player.so

PLAYER_SO_NAME   = $(PLAYER_TARGET)
PLAYER_SO_TARGET = $(PLAYER_TARGET)

LOCAL_SO_NAME := $(PLAYER_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PLAYER_SRC_PATH := $(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_FILTER_DIR)/
	@mkdir -p $(ORYX_CONF_DIR)/stream/filter/
	@cp -dpRf $(PLAYER_SRC_PATH)/$(PLAYER_CONF) $(ORYX_CONF_DIR)/stream/filter/$(PLAYER_CONF)
	@cp -dpRf $< $(ORYX_FILTER_DIR)/$(PLAYER_SO_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
