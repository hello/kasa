##
## /S2l/oryx/stream/playback/codecs/demuxer/make.inc
##
## History:
##   2014-11-3 - [ypchang] created file
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

### libamdemuxer-base.so
include $(CLEAR_VARS)

DEMUXER_CODEC_BASE = libamdemuxer-base.so

LOCAL_TARGET := $(DEMUXER_CODEC_BASE)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/base/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)/base                  \
                -I$(ORYX_DIR)/include                 \
                -I$(ORYX_DIR)/include/audio           \
                -I$(ORYX_DIR)/include/utility         \
                -I$(ORYX_DIR)/include/stream          \
                -I$(ORYX_DIR)/stream/include/amf      \
                -I$(ORYX_DIR)/stream/include/common   \
                -I$(ORYX_DIR)/stream/include/playback \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so     \
              libamstream-amf.so

DEMUXER_CODEC_BASE_NAME   = $(DEMUXER_CODEC_BASE)
DEMUXER_CODEC_BASE_TARGET = $(DEMUXER_CODEC_BASE)
LOCAL_SO_NAME := $(DEMUXER_CODEC_BASE)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(DEMUXER_CODEC_BASE_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

### demuxer-aac.so
include $(CLEAR_VARS)

DEMUXER_CODEC_AAC = demuxer-aac.so

LOCAL_TARGET := $(DEMUXER_CODEC_AAC)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/demuxer-aac/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)/base                      \
                -I$(LOCAL_PATH)/demuxer-aac               \
                -I$(ORYX_DIR)/include                     \
                -I$(ORYX_DIR)/include/audio               \
                -I$(ORYX_DIR)/include/utility             \
                -I$(ORYX_DIR)/include/stream              \
                -I$(ORYX_DIR)/stream/include/amf          \
                -I$(ORYX_DIR)/stream/include/common       \
                -I$(ORYX_DIR)/stream/include/playback     \
                -I$(ORYX_DIR)/stream/include/common/media \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so     \
              libamdemuxer-base.so

DEMUXER_CODEC_AAC_NAME   = $(DEMUXER_CODEC_AAC)
DEMUXER_CODEC_AAC_TARGET = $(DEMUXER_CODEC_AAC)
LOCAL_SO_NAME := $(DEMUXER_CODEC_AAC)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_DEMUXER_DIR)/
	@cp -dpRf $< $(ORYX_DEMUXER_DIR)/$(DEMUXER_CODEC_AAC_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

### demuxer-wav.so
include $(CLEAR_VARS)

DEMUXER_CODEC_WAV = demuxer-wav.so

LOCAL_TARGET := $(DEMUXER_CODEC_WAV)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/demuxer-wav/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)/base                      \
                -I$(LOCAL_PATH)/demuxer-wav               \
                -I$(ORYX_DIR)/include                     \
                -I$(ORYX_DIR)/include/audio               \
                -I$(ORYX_DIR)/include/utility             \
                -I$(ORYX_DIR)/include/stream              \
                -I$(ORYX_DIR)/stream/include/amf          \
                -I$(ORYX_DIR)/stream/include/common       \
                -I$(ORYX_DIR)/stream/include/playback     \
                -I$(ORYX_DIR)/stream/include/common/media \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so     \
              libamdemuxer-base.so

DEMUXER_CODEC_WAV_NAME   = $(DEMUXER_CODEC_WAV)
DEMUXER_CODEC_WAV_TARGET = $(DEMUXER_CODEC_WAV)
LOCAL_SO_NAME := $(DEMUXER_CODEC_WAV)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_DEMUXER_DIR)/
	@cp -dpRf $< $(ORYX_DEMUXER_DIR)/$(DEMUXER_CODEC_WAV_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

### demuxer-ogg.so
include $(CLEAR_VARS)

DEMUXER_CODEC_OGG = demuxer-ogg.so

LOCAL_TARGET := $(DEMUXER_CODEC_OGG)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/demuxer-ogg/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)/base                            \
                -I$(LOCAL_PATH)/demuxer-ogg                     \
                -I$(ORYX_DIR)/include                           \
                -I$(ORYX_DIR)/include/audio                     \
                -I$(ORYX_DIR)/include/utility                   \
                -I$(ORYX_DIR)/include/stream                    \
                -I$(ORYX_DIR)/stream/include/amf                \
                -I$(ORYX_DIR)/stream/include/common             \
                -I$(ORYX_DIR)/stream/include/playback           \
                -I$(ORYX_DIR)/stream/include/common/media       \
                -I$(PREBUILD_3RD_PARTY_DIR)/libogg/include      \
                -I$(PREBUILD_3RD_PARTY_DIR)/speex/include/speex \
                -std=c++11 -Werror

LOCAL_LDFLAGS := -L$(PREBUILD_3RD_PARTY_DIR)/libogg/usr/lib -logg \
                 -L$(PREBUILD_3RD_PARTY_DIR)/speex/usr/lib -lspeex

LOCAL_LIBS := libamutil.so     \
              libamdemuxer-base.so

DEMUXER_CODEC_OGG_NAME   = $(DEMUXER_CODEC_OGG)
DEMUXER_CODEC_OGG_TARGET = $(DEMUXER_CODEC_OGG)
LOCAL_SO_NAME := $(DEMUXER_CODEC_OGG)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_DEMUXER_DIR)/
	@cp -dpRf $< $(ORYX_DEMUXER_DIR)/$(DEMUXER_CODEC_OGG_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

### demuxer-rtp.so
include $(CLEAR_VARS)

DEMUXER_CODEC_RTP = demuxer-rtp.so

LOCAL_TARGET := $(DEMUXER_CODEC_RTP)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/demuxer-rtp/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)/base                      \
                -I$(LOCAL_PATH)/demuxer-rtp               \
                -I$(ORYX_DIR)/include                     \
                -I$(ORYX_DIR)/include/audio               \
                -I$(ORYX_DIR)/include/stream              \
                -I$(ORYX_DIR)/include/utility             \
                -I$(ORYX_DIR)/stream/include/amf          \
                -I$(ORYX_DIR)/stream/include/common       \
                -I$(ORYX_DIR)/stream/include/playback     \
                -I$(ORYX_DIR)/stream/include/common/media \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so     \
              libamdemuxer-base.so

DEMUXER_CODEC_RTP_NAME   = $(DEMUXER_CODEC_RTP)
DEMUXER_CODEC_RTP_TARGET = $(DEMUXER_CODEC_RTP)
LOCAL_SO_NAME := $(DEMUXER_CODEC_RTP)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_DEMUXER_DIR)/
	@cp -dpRf $< $(ORYX_DEMUXER_DIR)/$(DEMUXER_CODEC_RTP_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

### demuxer-unix-domain.so
include $(CLEAR_VARS)

DEMUXER_CODEC_UNIX_DOMAIN = demuxer-unix-domain.so

LOCAL_TARGET := $(DEMUXER_CODEC_UNIX_DOMAIN)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/demuxer-unix-domain/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)/base                      \
                -I$(LOCAL_PATH)/demuxer-unix-domain               \
                -I$(ORYX_DIR)/include                     \
                -I$(ORYX_DIR)/include/audio               \
                -I$(ORYX_DIR)/include/stream              \
                -I$(ORYX_DIR)/include/utility             \
                -I$(ORYX_DIR)/stream/include/amf          \
                -I$(ORYX_DIR)/stream/include/common       \
                -I$(ORYX_DIR)/stream/include/playback     \
                -I$(ORYX_DIR)/stream/include/common/media \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so     \
              libamdemuxer-base.so

DEMUXER_CODEC_UNIX_DOMAIN_NAME   = $(DEMUXER_CODEC_UNIX_DOMAIN)
DEMUXER_CODEC_UNIX_DOMAIN_TARGET = $(DEMUXER_CODEC_UNIX_DOMAIN)
LOCAL_SO_NAME := $(DEMUXER_CODEC_UNIX_DOMAIN)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_DEMUXER_DIR)/
	@cp -dpRf $< $(ORYX_DEMUXER_DIR)/$(DEMUXER_CODEC_DOMAIN_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
