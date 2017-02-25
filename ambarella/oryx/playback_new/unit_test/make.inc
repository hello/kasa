##
## /S2l/oryx/playback_new/unit_test/make.inc
##
## History:
##   2015-8-10 - [Zhi He] created file
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

ifeq ($(BUILD_AMBARELLA_ORYX_PLAYBACK_NEW_UNIT_TEST), y)

PB_UT_DIR := $(call my-dir)
LOCAL_PATH := $(PB_UT_DIR)

###############################################################################
LOCAL_TARGET := test_oryx_playback_new
LOCAL_SRCS   := $(PB_UT_DIR)/test_oryx_playback_new.cpp
LOCAL_LIBS   := libam_playback_new.so
LOCAL_CFLAGS := -I$(PB_UT_DIR)/../include -Werror

LOCAL_LDFLAGS :=

ifeq ($(BUILD_AMBARELLA_ORYX_PLAYBACK_NEW_ALSA_LIBRARY), y)
LOCAL_LDFLAGS += -L$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/usr/lib -lasound
endif

ifeq ($(BUILD_AMBARELLA_ORYX_PLAYBACK_NEW_FFMPEG_LIBRARY), y)
LOCAL_LDFLAGS += -L$(PREBUILD_3RD_PARTY_DIR)/ffmpeg/usr/lib -lavformat -lavcodec -lavutil
endif

ifeq ($(BUILD_AMBARELLA_ORYX_PLAYBACK_NEW_AUDIO_AMBA_AAC_LIBRARY), y)
LOCAL_LDFLAGS += -L$(PREBUILD_3RD_PARTY_DIR)/aac/lib -laacdec -laacenc
endif

LOCAL_LDFLAGS += -lpthread -ldl -lrt -lam_playback_new

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###############################################################################
LOCAL_TARGET := test_oryx_video_edit
LOCAL_SRCS   := $(PB_UT_DIR)/test_oryx_video_edit.cpp
LOCAL_LIBS   := libam_playback_new.so
LOCAL_CFLAGS := -I$(PB_UT_DIR)/../include -Werror

LOCAL_LDFLAGS :=

ifeq ($(BUILD_AMBARELLA_ORYX_PLAYBACK_NEW_ALSA_LIBRARY), y)
LOCAL_LDFLAGS += -L$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/usr/lib -lasound
endif

ifeq ($(BUILD_AMBARELLA_ORYX_PLAYBACK_NEW_FFMPEG_LIBRARY), y)
LOCAL_LDFLAGS += -L$(PREBUILD_3RD_PARTY_DIR)/ffmpeg/usr/lib -lavformat -lavcodec -lavutil
endif

ifeq ($(BUILD_AMBARELLA_ORYX_PLAYBACK_NEW_AUDIO_AMBA_AAC_LIBRARY), y)
LOCAL_LDFLAGS += -L$(PREBUILD_3RD_PARTY_DIR)/aac/lib -laacdec -laacenc
endif

LOCAL_LDFLAGS += -lpthread -ldl -lrt -lam_playback_new

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif