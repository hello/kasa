##
## /S2l/oryx/stream/common/iid/make.inc
##
## History:
##   2014-11-5 - [ypchang] created file
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

ifeq ($(BUILD_AMBARELLA_ORYX_STREAM_PLAYBACK), y)
include $(CLEAR_VARS)

PLAYBACK_IID_TARGET := libamstream-iid-playback.so
PLAYBACK_IID_VER_FILE = $(ORYX_DIR)/include/stream/version.h
PLAYBACK_IID_VER_PREFIX = PLAYBACK

LOCAL_TARGET := $(PLAYBACK_IID_TARGET)
LOCAL_SRCS   := $(LOCAL_PATH)/am_filter_iids_playback.cpp

LOCAL_CFLAGS := -I$(ORYX_DIR)/stream/include/amf    \
                -I$(ORYX_DIR)/stream/include/common \
                -I$(ORYX_DIR)/include               \
                -std=c++11 -Werror

PLAYBACK_IID_SO_VER_MAJ := $(shell awk '/define\ $(PLAYBACK_IID_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(PLAYBACK_IID_VER_FILE))
PLAYBACK_IID_SO_VER_MIN := $(shell awk '/define\ $(PLAYBACK_IID_VER_PREFIX)_LIB_MINOR/{print $$3}' $(PLAYBACK_IID_VER_FILE))
PLAYBACK_IID_SO_VER_PAT := $(shell awk '/define\ $(PLAYBACK_IID_VER_PREFIX)_LIB_PATCH/{print $$3}' $(PLAYBACK_IID_VER_FILE))
PLAYBACK_IID_SO_VER_STR := $(PLAYBACK_IID_SO_VER_MAJ).$(PLAYBACK_IID_SO_VER_MIN).$(PLAYBACK_IID_SO_VER_PAT)

PLAYBACK_IID_SO_NAME   := $(patsubst %.so,%.so.$(PLAYBACK_IID_SO_VER_MAJ), $(PLAYBACK_IID_TARGET))
PLAYBACK_IID_SO_TARGET := $(patsubst %.so,%.so.$(PLAYBACK_IID_SO_VER_STR), $(PLAYBACK_IID_TARGET))

LOCAL_SO_NAME := $(PLAYBACK_IID_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(PLAYBACK_IID_SO_TARGET)
	@ln -sf $(PLAYBACK_IID_SO_TARGET) $(ORYX_LIB_DIR)/$(PLAYBACK_IID_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif

ifeq ($(BUILD_AMBARELLA_ORYX_STREAM_RECORD), y)
include $(CLEAR_VARS)

RECORD_IID_TARGET := libamstream-iid-record.so
RECORD_IID_VER_FILE = $(ORYX_DIR)/include/stream/version.h
RECORD_IID_VER_PREFIX = RECORD

LOCAL_TARGET := $(RECORD_IID_TARGET)
LOCAL_SRCS   := $(LOCAL_PATH)/am_filter_iids_record.cpp

LOCAL_CFLAGS := -I$(ORYX_DIR)/stream/include/amf    \
                -I$(ORYX_DIR)/stream/include/common \
                -I$(ORYX_DIR)/include               \
                -std=c++11 -Werror

RECORD_IID_SO_VER_MAJ := $(shell awk '/define\ $(RECORD_IID_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(RECORD_IID_VER_FILE))
RECORD_IID_SO_VER_MIN := $(shell awk '/define\ $(RECORD_IID_VER_PREFIX)_LIB_MINOR/{print $$3}' $(RECORD_IID_VER_FILE))
RECORD_IID_SO_VER_PAT := $(shell awk '/define\ $(RECORD_IID_VER_PREFIX)_LIB_PATCH/{print $$3}' $(RECORD_IID_VER_FILE))
RECORD_IID_SO_VER_STR := $(RECORD_IID_SO_VER_MAJ).$(RECORD_IID_SO_VER_MIN).$(RECORD_IID_SO_VER_PAT)

RECORD_IID_SO_NAME   := $(patsubst %.so,%.so.$(RECORD_IID_SO_VER_MAJ), $(RECORD_IID_TARGET))
RECORD_IID_SO_TARGET := $(patsubst %.so,%.so.$(RECORD_IID_SO_VER_STR), $(RECORD_IID_TARGET))

LOCAL_SO_NAME := $(RECORD_IID_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(RECORD_IID_SO_TARGET)
	@ln -sf $(RECORD_IID_SO_TARGET) $(ORYX_LIB_DIR)/$(RECORD_IID_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif
