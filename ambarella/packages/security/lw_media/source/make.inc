##
## ambarella/packages/security/lw_media/source/make.inc
##
## History:
##   2015-9-18 - [Zhi He] created file
##
## Copyright (C) 2015-2025, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

ifeq ($(BUILD_AMBARELLA_LW_MEDIA), y)

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
STREAMING_SRCS := $(wildcard $(ENGINE_DIR)/streaming/*.cpp)
ASM_SRCS := $(wildcard $(ENGINE_DIR)/asm/*.S)

## Modules
MODULES_SRCS := $(wildcard $(ENGINE_DIR)/modules/*.cpp)
MODULES_SRCS += $(wildcard $(ENGINE_DIR)/modules/capturers/*.cpp)
MODULES_SRCS += $(wildcard $(ENGINE_DIR)/modules/demuxers/*.cpp)
MODULES_SRCS += $(wildcard $(ENGINE_DIR)/modules/demuxers/rtsp_demuxer/*.cpp)
MODULES_SRCS += $(wildcard $(ENGINE_DIR)/modules/decoders/*.cpp)
MODULES_SRCS += $(wildcard $(ENGINE_DIR)/modules/encoders/*.cpp)
MODULES_SRCS += $(wildcard $(ENGINE_DIR)/modules/fileparsers/*.cpp)
MODULES_SRCS += $(wildcard $(ENGINE_DIR)/modules/muxers/*.cpp)
MODULES_SRCS += $(wildcard $(ENGINE_DIR)/modules/renderers/*.cpp)
MODULES_SRCS += $(wildcard $(ENGINE_DIR)/modules/streamers/*.cpp)

LOCAL_TARGET := liblwmedia.a

LOCAL_SRCS := $(ENGINE_SRCS) $(BASE_SRCS) $(CODECS_SRCS) $(FILTERS_SRCS) $(FRAMEWORK_SRCS) $(PLATFORM_AL_SRCS) $(MODULES_SRCS) $(STREAMING_SRCS) $(ASM_SRCS)

LOCAL_CFLAGS := -I$(ENGINE_DIR)/../include                          \
                -I$(ENGINE_DIR)/internal_include                    \
                -Werror

LOCAL_LDFLAGS:=

ifeq ($(BUILD_AMBARELLA_LW_MEDIA_DEMUXER_RTSP), y)
LOCAL_CFLAGS += -DBUILD_MODULE_DEMUXER_RTSP=1
endif

ifeq ($(BUILD_AMBARELLA_LW_MEDIA_DEMUXER_MP4), y)
LOCAL_CFLAGS += -DBUILD_MODULE_DEMUXER_MP4=1
endif

ifeq ($(BUILD_AMBARELLA_LW_MEDIA_DEMUXER_TS), y)
LOCAL_CFLAGS += -DBUILD_MODULE_DEMUXER_TS=1
endif

ifeq ($(BUILD_AMBARELLA_LW_MEDIA_MUXER_MP4), y)
LOCAL_CFLAGS += -DBUILD_MODULE_MUXER_MP4=1
endif

ifeq ($(BUILD_AMBARELLA_LW_MEDIA_MUXER_TS), y)
LOCAL_CFLAGS += -DBUILD_MODULE_MUXER_TS=1
endif

ifeq ($(BUILD_AMBARELLA_LW_MEDIA_LINUXFB), y)
LOCAL_CFLAGS += -DBUILD_MODULE_LINUX_FB=1
endif

ifeq ($(BUILD_AMBARELLA_LW_MEDIA_ALSA_LIBRARY), y)
LOCAL_CFLAGS += -DBUILD_MODULE_ALSA=1 -I$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/include
LOCAL_LDFLAGS += -L$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/usr/lib
endif

ifeq ($(BUILD_AMBARELLA_LW_MEDIA_FFMPEG_LIBRARY), y)
LOCAL_CFLAGS += -DBUILD_MODULE_FFMPEG=1 -I$(PREBUILD_3RD_PARTY_DIR)/ffmpeg/include
LOCAL_LDFLAGS += -L$(PREBUILD_3RD_PARTY_DIR)/ffmpeg/usr/lib -lavformat -lavcodec -lavutil
endif

ifeq ($(BUILD_AMBARELLA_LW_MEDIA_AMBA_AAC_LIBRARY), y)
LOCAL_CFLAGS += -DBUILD_MODULE_LIBAAC=1 -I$(PREBUILD_3RD_PARTY_DIR)/aac/include
LOCAL_LDFLAGS += -L$(PREBUILD_3RD_PARTY_DIR)/aac/lib
endif

ifeq ($(BUILD_AMBARELLA_LW_MEDIA_AMBA_DSP), y)
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
endif

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
