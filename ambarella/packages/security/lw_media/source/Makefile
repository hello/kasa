##
## $(MODULE_NAME_TAG)/source/Makefile
##
## History:
##    2015/07/28 - [Zhi He] Create File
##
## Copyright (C) 2014 - 2024, the Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of the Ambarella, Inc.
##

default: all

include ../build/core/linux/pre_requirement.mk

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/config_linux.mk

SUBDIRS = asm filters modules streaming codecs base framework platform_al

base_src := $(wildcard base/*.cpp)
base_obj := $(base_src:base/%.cpp=base/%.o)

framework_src := $(wildcard framework/*.cpp)
framework_obj := $(framework_src:framework/%.cpp=framework/%.o)

engines_src := engine.cpp
engines_obj := $(engines_src:%.cpp=%.o)

filters_src := $(wildcard filters/*.cpp)
filters_obj := $(filters_src:filters/%.cpp=filters/%.o)

modules_src := $(wildcard modules/*.cpp)
modules_obj := $(modules_src:modules/%.cpp=modules/%.o)

modules_demuxers_src := $(wildcard modules/demuxers/*.cpp)
modules_demuxers_obj := $(modules_demuxers_src:modules/demuxers/%.cpp=modules/demuxers/%.o)

modules_demuxer_rtsp_src := $(wildcard modules/demuxers/rtsp_demuxer/*.cpp)
modules_demuxer_rtsp_obj := $(modules_demuxer_rtsp_src:modules/demuxers/rtsp_demuxer/%.cpp=modules/demuxers/rtsp_demuxer/%.o)

modules_decoders_src := $(wildcard modules/decoders/*.cpp)
ifeq ($(BUILD_CONFIG_MODULE_LIBAAC), n)
MODULE_LIBAAC_DECODER_SRC := modules/decoders/aac_audio_decoder.cpp
modules_decoders_src := $(filter-out $(MODULE_LIBAAC_DECODER_SRC),$(modules_decoders_src))
endif
ifeq ($(BUILD_CONFIG_MODULE_FFMPEG), n)
MODULE_FFMPEG_DECODER_SRC := modules/decoders/ffmpeg_video_decoder.cpp
modules_decoders_src := $(filter-out $(MODULE_FFMPEG_DECODER_SRC),$(modules_decoders_src))
endif
modules_decoders_obj := $(modules_decoders_src:modules/decoders/%.cpp=modules/decoders/%.o)

modules_encoders_src := $(wildcard modules/encoders/*.cpp)
ifeq ($(BUILD_CONFIG_MODULE_LIBAAC), n)
MODULE_LIBAAC_ENCODER_SRC := modules/encoders/aac_audio_encoder.cpp
modules_encoders_src := $(filter-out $(MODULE_LIBAAC_ENCODER_SRC),$(modules_encoders_src))
endif
ifeq ($(BUILD_CONFIG_MODULE_AMBA_DSP), n)
MODULE_AMBA_ENCODER_SRC := modules/encoders/amba_video_encoder.cpp
modules_encoders_src := $(filter-out $(MODULE_AMBA_ENCODER_SRC),$(modules_encoders_src))
endif
modules_encoders_obj := $(modules_encoders_src:modules/encoders/%.cpp=modules/encoders/%.o)

modules_muxers_src := $(wildcard modules/muxers/*.cpp)
modules_muxers_obj := $(modules_muxers_src:modules/muxers/%.cpp=modules/muxers/%.o)

modules_renderers_src := $(wildcard modules/renderers/*.cpp)
ifeq ($(BUILD_CONFIG_MODULE_AMBA_DSP), n)
MODULE_AMBA_RENDERER_SRC := modules/renderers/amba_video_renderer.cpp
modules_renderers_src := $(filter-out $(MODULE_AMBA_RENDERER_SRC),$(modules_renderers_src))
endif
ifeq ($(BUILD_CONFIG_MODULE_LINUX_FB), n)
MODULE_LINUXFB_RENDERER_SRC := modules/renderers/linuxfb_video_renderer.cpp
modules_renderers_src := $(filter-out $(MODULE_LINUXFB_RENDERER_SRC),$(modules_renderers_src))
endif
ifeq ($(BUILD_CONFIG_MODULE_ALSA), n)
MODULE_ALSA_RENDERER_SRC := modules/renderers/alsa_audio_renderer.cpp
modules_renderers_src := $(filter-out $(MODULE_ALSA_RENDERER_SRC),$(modules_renderers_src))
endif
modules_renderers_obj := $(modules_renderers_src:modules/renderers/%.cpp=modules/renderers/%.o)

asm_src += asm/neon_ycbcr.S
asm_obj += asm/neon_ycbcr.o

modules_streamers_src := $(wildcard modules/streamers/*.cpp)
modules_streamers_obj := $(modules_streamers_src:modules/streamers/%.cpp=modules/streamers/%.o)

modules_streaming_servers_src := $(wildcard streaming/*.cpp)
modules_streaming_servers_obj := $(modules_streaming_servers_src:streaming/%.cpp=streaming/%.o)

modules_parsers_src := $(wildcard modules/parsers/*.cpp)
modules_parsers_obj := $(modules_parsers_src:modules/parsers/%.cpp=modules/parsers/%.o)

modules_capturers_src := $(wildcard modules/capturers/*.cpp)
ifeq ($(BUILD_CONFIG_MODULE_ALSA), n)
MODULE_ALSA_CAPTURER_SRC := modules/capturers/alsa_audio_capturer.cpp
modules_capturers_src := $(filter-out $(MODULE_ALSA_CAPTURER_SRC),$(modules_capturers_src))
endif
modules_capturers_obj := $(modules_capturers_src:modules/capturers/%.cpp=modules/capturers/%.o)

codecs_src := $(wildcard codecs/*.cpp)
codecs_obj := $(codecs_src:codecs/%.cpp=codecs/%.o)

codecs_parsers_h264_src := $(wildcard codecs/parsers/h264/*.cpp)
codecs_parsers_h264_obj := $(codecs_parsers_h264_src:codecs/parsers/h264/%.cpp=codecs/parsers/h264/%.o)

codecs_parsers_h265_src := $(wildcard codecs/parsers/h265/*.cpp)
codecs_parsers_h265_obj := $(codecs_parsers_h265_src:codecs/parsers/h265/%.cpp=codecs/parsers/h265/%.o)

platform_al_src := $(wildcard platform_al/*.cpp)
platform_al_obj := $(platform_al_src:platform_al/%.cpp=platform_al/%.o)

lib_MODULE_SRC := 	 $(engines_src)	\
             $(framework_src)     \
			 $(base_src)   \
			 $(filters_src)     \
			 $(modules_src)     \
			 $(modules_demuxers_src)     \
			 $(modules_demuxer_rtsp_src)     \
			 $(modules_decoders_src)     \
			 $(modules_encoders_src)     \
			 $(modules_muxers_src)     \
			 $(modules_renderers_src)	\
			 $(modules_streamers_src)	\
			 $(modules_streaming_servers_src) \
			 $(modules_parsers_src)	\
			 $(modules_capturers_src) \
			 $(codecs_src) \
			 $(codecs_parsers_h264_src) \
			 $(codecs_parsers_h265_src) \
			 $(platform_al_src) \
			 $(asm_src)

lib_MODULE_OBJ := 	 $(engines_obj)	\
             $(framework_obj)     \
			 $(base_obj)     \
			 $(simple_api_obj)     \
			 $(filters_obj)     \
			 $(modules_obj)     \
			 $(modules_demuxers_obj)     \
			 $(modules_demuxer_rtsp_obj)     \
			 $(modules_decoders_obj)     \
			 $(modules_encoders_obj)     \
			 $(modules_muxers_obj)     \
			 $(modules_renderers_obj)	\
			 $(modules_streamers_obj)	\
			 $(modules_streaming_servers_obj) \
			 $(modules_parsers_obj) \
			 $(modules_capturers_obj) \
			 $(codecs_obj) \
			 $(codecs_parsers_h264_obj) \
			 $(codecs_parsers_h265_obj) \
			 $(platform_al_obj) \
			 $(asm_obj)

STATIC_LIB_NAMES = lw_media

lw_media_src     := $(lib_MODULE_SRC)
lw_media_obj     := $(lib_MODULE_OBJ)

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk



