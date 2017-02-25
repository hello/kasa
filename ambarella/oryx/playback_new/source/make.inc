##
## /S2l/oryx/playback_new/source/make.inc
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

ifeq ($(BUILD_AMBARELLA_ORYX_PLAYBACK_NEW), y)

ENGINE_DIR := $(call my-dir)
LOCAL_PATH := $(ENGINE_DIR)
LOCAL_LIBS :=

## Engine, filters and framework
ENGINE_SRCS := $(wildcard $(ENGINE_DIR)/*.cpp)
BASE_SRCS := $(wildcard $(ENGINE_DIR)/base/*.cpp)
CODECS_SRCS := $(wildcard $(ENGINE_DIR)/codecs/*.cpp)
CODECS_SRCS += $(wildcard $(ENGINE_DIR)/codecs/parsers/h264/*.cpp)
CODECS_SRCS += $(wildcard $(ENGINE_DIR)/codecs/parsers/h265/*.cpp)
FILTERS_SRCS := $(wildcard $(ENGINE_DIR)/filters/*.cpp)
FRAMEWORK_SRCS := $(wildcard $(ENGINE_DIR)/framework/*.cpp)
PLATFORM_AL_SRCS := $(wildcard $(ENGINE_DIR)/platform_al/*.cpp)

## Modules
MODULES_SRCS := $(wildcard $(ENGINE_DIR)/modules/*.cpp)
MODULES_SRCS += $(wildcard $(ENGINE_DIR)/modules/demuxers/*.cpp)
MODULES_SRCS += $(wildcard $(ENGINE_DIR)/modules/demuxers/rtsp_demuxer/*.cpp)
MODULES_SRCS += $(wildcard $(ENGINE_DIR)/modules/decoders/*.cpp)
MODULES_SRCS += $(wildcard $(ENGINE_DIR)/modules/fileparsers/*.cpp)
MODULES_SRCS += $(wildcard $(ENGINE_DIR)/modules/renderers/*.cpp)
MODULES_SRCS += $(wildcard $(ENGINE_DIR)/modules/probes/*.cpp)
MODULES_SRCS += $(wildcard $(ENGINE_DIR)/modules/capturers/*.cpp)
MODULES_SRCS += $(wildcard $(ENGINE_DIR)/modules/encoders/*.cpp)
MODULES_SRCS += $(wildcard $(ENGINE_DIR)/modules/muxers/*.cpp)
MODULES_SRCS += $(wildcard $(ENGINE_DIR)/modules/video_injecters/*.cpp)

PLAYBACK_NEW_TARGET = libam_playback_new.so
LOCAL_TARGET := $(PLAYBACK_NEW_TARGET)

LOCAL_SRCS := $(ENGINE_SRCS) $(BASE_SRCS) $(CODECS_SRCS) $(FILTERS_SRCS) $(FRAMEWORK_SRCS) $(PLATFORM_AL_SRCS) $(MODULES_SRCS)

LOCAL_CFLAGS := -std=c++11 -I$(ENGINE_DIR)/../include \
                -I$(ENGINE_DIR)/internal_include \
                -Werror

ifeq ($(AMBARELLA_CPU_ARCH), armv7-a-hf)
CROSS_COMPILE_TARGRT_CPU_ARCH_NAME := armv7-a
else
CROSS_COMPILE_TARGRT_CPU_ARCH_NAME := $(AMBARELLA_CPU_ARCH)
endif

ifeq ($(CROSS_COMPILE_TARGRT_CPU_ARCH_NAME), armv7-a)
BUILD_CONFIG_CPU_OPT_NEON := y
else
BUILD_CONFIG_CPU_OPT_NEON := n
endif

ifeq ($(BUILD_CONFIG_CPU_OPT_NEON), y)
ASM_SRCS := $(wildcard $(ENGINE_DIR)/asm/$(CROSS_COMPILE_TARGRT_CPU_ARCH_NAME)/*.S)
LOCAL_SRCS += $(ASM_SRCS)
LOCAL_CFLAGS += -DBUILD_ASM_NEON=1
endif

LOCAL_SRCS += $(ASM_SRCS)

LOCAL_LDFLAGS:=

ifeq ($(BUILD_AMBARELLA_ORYX_PLAYBACK_NEW_DEMUXER_RTSP), y)
LOCAL_CFLAGS += -DBUILD_MODULE_DEMUXER_RTSP=1
endif

ifeq ($(BUILD_AMBARELLA_ORYX_PLAYBACK_NEW_DEMUXER_MP4), y)
LOCAL_CFLAGS += -DBUILD_MODULE_DEMUXER_MP4=1
endif

ifeq ($(BUILD_AMBARELLA_ORYX_PLAYBACK_NEW_DEMUXER_TS), y)
LOCAL_CFLAGS += -DBUILD_MODULE_DEMUXER_TS=1
endif

ifeq ($(BUILD_AMBARELLA_ORYX_PLAYBACK_NEW_MUXER_MP4), y)
LOCAL_CFLAGS += -DBUILD_MODULE_MUXER_MP4=1
endif

ifeq ($(BUILD_AMBARELLA_ORYX_PLAYBACK_NEW_LINUXFB), y)
LOCAL_CFLAGS += -DBUILD_MODULE_LINUX_FB=1
endif

ifeq ($(BUILD_AMBARELLA_ORYX_PLAYBACK_NEW_LINUX_UVC), y)
LOCAL_CFLAGS += -DBUILD_MODULE_LINUX_UVC=1
endif

LOCAL_CFLAGS += -DBUILD_MODULE_DEMUXER_VIDEO_ES_FILE=1

ifeq ($(BUILD_AMBARELLA_ORYX_PLAYBACK_NEW_ALSA_LIBRARY), y)
LOCAL_CFLAGS += -DBUILD_MODULE_ALSA=1 -I$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/include
LOCAL_LDFLAGS += -L$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/usr/lib
endif

ifeq ($(BUILD_AMBARELLA_ORYX_PLAYBACK_NEW_FFMPEG_LIBRARY), y)
LOCAL_CFLAGS += -DBUILD_MODULE_FFMPEG=1 -I$(PREBUILD_3RD_PARTY_DIR)/ffmpeg/include
LOCAL_LDFLAGS += -L$(PREBUILD_3RD_PARTY_DIR)/ffmpeg/usr/lib -lavformat -lavcodec -lavutil
endif

ifeq ($(BUILD_AMBARELLA_ORYX_PLAYBACK_NEW_AUDIO_AMBA_AAC_LIBRARY), y)
LOCAL_CFLAGS += -DBUILD_MODULE_LIBAAC=1 -I$(PREBUILD_3RD_PARTY_DIR)/aac/include
LOCAL_LDFLAGS += -L$(PREBUILD_3RD_PARTY_DIR)/aac/lib
endif

ifeq ($(BUILD_AMBARELLA_ORYX_PLAYBACK_NEW_AMBA_DSP), y)
LOCAL_CFLAGS += -DBUILD_MODULE_AMBA_DSP=1
ifeq ($(AMBARELLA_ARCH), s2l)
LOCAL_CFLAGS += -DBUILD_DSP_AMBA_S2L=1
endif
ifeq ($(AMBARELLA_ARCH), s2)
LOCAL_CFLAGS += -DBUILD_DSP_AMBA_S2=1
endif
ifeq ($(AMBARELLA_ARCH), s2e)
LOCAL_CFLAGS += -DBUILD_DSP_AMBA_S2E=1
endif
ifeq ($(AMBARELLA_ARCH), s3l)
LOCAL_CFLAGS += -DBUILD_DSP_AMBA_S3L=1
endif
ifeq ($(AMBARELLA_ARCH), s5)
LOCAL_CFLAGS += -DBUILD_DSP_AMBA_S5=1
endif
ifeq ($(AMBARELLA_ARCH), s5l)
LOCAL_CFLAGS += -DBUILD_DSP_AMBA_S5L=1
endif
endif

LOCAL_SO_NAME := $(PLAYBACK_NEW_TARGET)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(PLAYBACK_NEW_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
