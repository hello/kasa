##
## /S2l/oryx/stream/record/filters/activemuxer/make.inc
##
## History:
##   2016-10-26 - [ypchang] created file
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

ifeq ($(CONFIG_AMBARELLA_ORYX_STREAM_RECORD_MUXER_ACTIVE), y)

MUXER_SRC_TARGET  = filter-activemuxer.so
MUXER_FILE_CONF0   = filter-file-muxer-0.acs
MUXER_FILE_CONF1   = filter-file-muxer-1.acs
MUXER_DIRECT_CONF = filter-direct-muxer.acs

LOCAL_TARGET := $(MUXER_SRC_TARGET)
LOCAL_SRCS   := $(LOCAL_PATH)/am_activemuxer_codec_obj.cpp \
                $(LOCAL_PATH)/am_activemuxer_config.cpp    \
                $(LOCAL_PATH)/am_activemuxer.cpp

LOCAL_CFLAGS := -I$(LOCAL_PATH)                     \
                -I$(LOCAL_PATH)/libavqueue          \
                -I$(ORYX_DIR)/include               \
                -I$(ORYX_DIR)/include/configure     \
                -I$(ORYX_DIR)/include/utility       \
                -I$(ORYX_DIR)/include/stream        \
                -I$(ORYX_DIR)/include/audio         \
                -I$(ORYX_DIR)/include/image_quality \
                -I$(ORYX_DIR)/stream/include/amf    \
                -I$(ORYX_DIR)/stream/include/common \
                -I$(ORYX_DIR)/stream/include/record \
                -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath=$(patsubst $(FAKEROOT_DIR)/%,%,$(ORYX_MUXER_DIR))

LOCAL_LIBS := libamutil.so                  \
              libamosal.so                  \
              libamconfig.so                \
              libamactivemuxer-codec-iid.so \
              libamstream-iid-record.so     \
              libamstream-amf.so            \
              libavqueue.so

MUXER_SRC_SO_NAME   = $(MUXER_SRC_TARGET)
MUXER_SRC_SO_TARGET = $(MUXER_SRC_TARGET)

LOCAL_SO_NAME := $(MUXER_SRC_SO_NAME)

include $(BUILD_APP)

.PHONY: $(MUXER_SRC_TARGET)

$(LOCAL_TARGET): MUXER_SRC_PATH := $(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_FILTER_DIR)/
	@mkdir -p $(ORYX_CONF_DIR)/stream/filter/
	@cp -dpRf $(MUXER_SRC_PATH)/$(MUXER_FILE_CONF0) $(ORYX_CONF_DIR)/stream/filter/$(MUXER_FILE_CONF0)
	@cp -dpRf $(MUXER_SRC_PATH)/$(MUXER_FILE_CONF1) $(ORYX_CONF_DIR)/stream/filter/$(MUXER_FILE_CONF1)
	@cp -dpRf $(MUXER_SRC_PATH)/$(MUXER_DIRECT_CONF) $(ORYX_CONF_DIR)/stream/filter/$(MUXER_DIRECT_CONF)
	@cp -dpRf $< $(ORYX_FILTER_DIR)/$(MUXER_SRC_SO_TARGET)
	@ln -sf $(MUXER_SRC_SO_TARGET) $(ORYX_FILTER_DIR)/filter-file-muxer-0.so
	@ln -sf $(MUXER_SRC_SO_TARGET) $(ORYX_FILTER_DIR)/filter-file-muxer-1.so
	@ln -sf $(MUXER_SRC_SO_TARGET) $(ORYX_FILTER_DIR)/filter-direct-muxer.so
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

include $(CLEAR_VARS)

CODEC_IID_TARGET = libamactivemuxer-codec-iid.so
LOCAL_TARGET = $(CODEC_IID_TARGET)
LOCAL_SRCS := $(LOCAL_PATH)/am_activemuxer_codec_iid.cpp
LOCAL_CFLAGS := -I$(ORYX_DIR)/stream/include/common \
                -I$(ORYX_DIR)/include               \
                -std=c++11 -Werror
LOCAL_SO_NAME := $(CODEC_IID_TARGET)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_MUXER_DIR)
	@cp -dpRf $< $(ORYX_MUXER_DIR)/$(CODEC_IID_TARGET)
	@echo "Build $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

include $(ORYX_DIR)/stream/record/filters/activemuxer/libavqueue/make.inc

endif
