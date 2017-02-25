##
## $(MODULE_NAME_TAG)/source/Makefile
##
## History:
##    2015/07/28 - [Zhi He] Create File
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

default: all

include ../build/core/linux/pre_requirement.mk

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/config_linux.mk

SUBDIRS = asm filters modules codecs base framework platform_al

base_src := $(wildcard base/*.cpp)
base_obj := $(base_src:base/%.cpp=base/%.o)

framework_src := $(wildcard framework/*.cpp)
framework_obj := $(framework_src:framework/%.cpp=framework/%.o)

engines_src := am_media_engine.cpp
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
MODULE_LIBAAC_DECODER_SRC := modules/decoders/am_aac_audio_decoder.cpp
modules_decoders_src := $(filter-out $(MODULE_LIBAAC_DECODER_SRC),$(modules_decoders_src))
endif
ifeq ($(BUILD_CONFIG_MODULE_FFMPEG), n)
MODULE_FFMPEG_DECODER_SRC := modules/decoders/am_ffmpeg_video_decoder.cpp
modules_decoders_src := $(filter-out $(MODULE_FFMPEG_DECODER_SRC),$(modules_decoders_src))
endif
modules_decoders_obj := $(modules_decoders_src:modules/decoders/%.cpp=modules/decoders/%.o)

modules_renderers_src := $(wildcard modules/renderers/*.cpp)
ifeq ($(BUILD_CONFIG_MODULE_AMBA_DSP), n)
MODULE_AMBA_RENDERER_SRC := modules/renderers/am_amba_video_renderer.cpp
modules_renderers_src := $(filter-out $(MODULE_AMBA_RENDERER_SRC),$(modules_renderers_src))
endif
ifeq ($(BUILD_CONFIG_MODULE_LINUX_FB), n)
MODULE_LINUXFB_RENDERER_SRC := modules/renderers/am_linuxfb_video_renderer.cpp
modules_renderers_src := $(filter-out $(MODULE_LINUXFB_RENDERER_SRC),$(modules_renderers_src))
endif
ifeq ($(BUILD_CONFIG_MODULE_ALSA), n)
MODULE_ALSA_RENDERER_SRC := modules/renderers/am_alsa_audio_renderer.cpp
modules_renderers_src := $(filter-out $(MODULE_ALSA_RENDERER_SRC),$(modules_renderers_src))
endif
modules_renderers_obj := $(modules_renderers_src:modules/renderers/%.cpp=modules/renderers/%.o)

asm_src += asm/neon_ycbcr.S
asm_obj += asm/neon_ycbcr.o

modules_parsers_src := $(wildcard modules/parsers/*.cpp)
modules_parsers_obj := $(modules_parsers_src:modules/parsers/%.cpp=modules/parsers/%.o)

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
			 $(modules_renderers_src)	\
			 $(modules_parsers_src)	\
			 $(codecs_src) \
			 $(codecs_parsers_h264_src) \
			 $(codecs_parsers_h265_src) \
			 $(platform_al_src)\
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
			 $(modules_renderers_obj)	\
			 $(modules_parsers_obj) \
			 $(codecs_obj) \
			 $(codecs_parsers_h264_obj) \
			 $(codecs_parsers_h265_obj) \
			 $(platform_al_obj)\
			 $(asm_obj)

STATIC_LIB_NAMES = oryx_playback_new

oryx_playback_new_src     := $(lib_MODULE_SRC)
oryx_playback_new_obj     := $(lib_MODULE_OBJ)

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk



