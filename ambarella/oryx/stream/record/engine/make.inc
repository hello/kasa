##
## /S2l/oryx/stream/record/engine/make.inc
##
## History:
##   2014-12-31 - [ypchang] created file
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

RECORD_ENGINE_TARGET    := libamstream-record-engine.so
ifeq ($(CONFIG_AMBARELLA_ORYX_STREAM_RECORD_MUXER_ACTIVE), y)
RECORD_ENGINE_CONF       = record-engine-activemuxer.acs
endif
ifeq ($(CONFIG_AMBARELLA_ORYX_STREAM_RECORD_MUXER_ORIG), y)
RECORD_ENGINE_CONF       = record-engine-orig.acs
endif
RECORD_ENGINE_VER_FILE   = $(ORYX_DIR)/include/stream/version.h
RECORD_ENGINE_VER_PREFIX = PLAYBACK

LOCAL_TARGET := $(RECORD_ENGINE_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)                     \
                -I$(ORYX_DIR)/include               \
                -I$(ORYX_DIR)/include/audio         \
                -I$(ORYX_DIR)/include/video         \
                -I$(ORYX_DIR)/include/configure     \
                -I$(ORYX_DIR)/include/utility       \
                -I$(ORYX_DIR)/include/stream        \
                -I$(ORYX_DIR)/stream/include/amf    \
                -I$(ORYX_DIR)/stream/include/common \
                -I$(ORYX_DIR)/stream/include/record \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so       \
              libamosal.so       \
              libamconfig.so     \
              libamstream-iid-record.so

RECORD_ENGINE_SO_VER_MAJ := $(shell awk '/define\ $(RECORD_ENGINE_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(RECORD_ENGINE_VER_FILE))
RECORD_ENGINE_SO_VER_MIN := $(shell awk '/define\ $(RECORD_ENGINE_VER_PREFIX)_LIB_MINOR/{print $$3}' $(RECORD_ENGINE_VER_FILE))
RECORD_ENGINE_SO_VER_PAT := $(shell awk '/define\ $(RECORD_ENGINE_VER_PREFIX)_LIB_PATCH/{print $$3}' $(RECORD_ENGINE_VER_FILE))
RECORD_ENGINE_SO_VER_STR := $(RECORD_ENGINE_SO_VER_MAJ).$(RECORD_ENGINE_SO_VER_MIN).$(RECORD_ENGINE_SO_VER_PAT)

RECORD_ENGINE_SO_NAME   := $(patsubst %.so,%.so.$(RECORD_ENGINE_SO_VER_MAJ), $(RECORD_ENGINE_TARGET))
RECORD_ENGINE_SO_TARGET := $(patsubst %.so,%.so.$(RECORD_ENGINE_SO_VER_STR), $(RECORD_ENGINE_TARGET))

LOCAL_SO_NAME := $(RECORD_ENGINE_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)
ifeq ($(CONFIG_AMBARELLA_ORYX_STREAM_RECORD_MUXER_ACTIVE), y)
$(LOCAL_TARGET): filter-activemuxer.so
endif
ifeq ($(CONFIG_AMBARELLA_ORYX_STREAM_RECORD_MUXER_ORIG), y)
$(LOCAL_TARGET): filter-muxer.so
endif
$(LOCAL_TARGET): filter-audio-source.so filter-video-source.so filter-avqueue.so
$(LOCAL_TARGET): RECORD_ENGINE_SRC_PATH := $(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@mkdir -p $(ORYX_CONF_DIR)/stream/engine/
	@cp -dpRf $(RECORD_ENGINE_SRC_PATH)/$(RECORD_ENGINE_CONF) $(ORYX_CONF_DIR)/stream/engine/record-engine.acs
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(RECORD_ENGINE_SO_TARGET)
	@ln -sf $(RECORD_ENGINE_SO_TARGET) $(ORYX_LIB_DIR)/$(RECORD_ENGINE_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
