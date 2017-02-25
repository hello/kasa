###############################################################################
## $(MODULE_NAME_TAG)/media_mw/src/Makefile
##
## History:
##    2013/04/15 - [Zhi He] Create File
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
##################################################################################

default: all

include ../../build/core/linux/pre_requirement.mk

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/config_linux.mk

ifeq ($(CONFIG_ARCH_I1), y)
include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/platform_i1_linux.mk
endif

ifeq ($(CONFIG_ARCH_S2), y)
include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/platform_s2_linux.mk
endif

MODULE_INC := -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/media_mw/src/internal_include

SUBDIRS =audio_platform utils framework engines filters modules dsp_platform codecs simple_api

utils_src = $(wildcard utils/*.cpp)
utils_obj = $(utils_src:utils/%.cpp=utils/%.o)

framework_src := $(wildcard framework/*.cpp)
framework_src += $(wildcard framework/streaming/*.cpp)
framework_src += $(wildcard framework/cloud/*.cpp)
framework_obj := $(framework_src:framework/%.cpp=framework/%.o)

engines_src = $(wildcard engines/*.cpp)
engines_obj = $(engines_src:engines/%.cpp=engines/%.o)

filters_src = $(wildcard filters/*.cpp)
filters_obj = $(filters_src:filters/%.cpp=filters/%.o)

modules_src = $(wildcard modules/*.cpp)
modules_obj = $(modules_src:modules/%.cpp=modules/%.o)

modules_demuxers_src := $(wildcard modules/demuxers/*.cpp)
ifeq ($(BUILD_CONFIG_MODULE_FFMPEG), n)
MODULE_FFMPEG_DEMUXER_SRC := modules/demuxers/ffmpeg_demuxer.cpp
modules_demuxers_src := $(filter-out $(MODULE_FFMPEG_DEMUXER_SRC),$(modules_demuxers_src))
endif
modules_demuxers_obj := $(modules_demuxers_src:modules/demuxers/%.cpp=modules/demuxers/%.o)

modules_decoders_src := $(wildcard modules/decoders/*.cpp)
ifeq ($(BUILD_CONFIG_MODULE_LIBAAC), n)
MODULE_LIBAAC_DECODER_SRC := modules/decoders/aac_audio_decoder.cpp
modules_decoders_src := $(filter-out $(MODULE_LIBAAC_DECODER_SRC),$(modules_decoders_src))
endif
ifeq ($(BUILD_CONFIG_MODULE_FFMPEG), n)
MODULE_AUDIO_FFMPEG_DECODER_SRC := modules/decoders/ffmpeg_audio_decoder.cpp modules/decoders/ffmpeg_video_decoder.cpp
modules_decoders_src := $(filter-out $(MODULE_AUDIO_FFMPEG_DECODER_SRC),$(modules_decoders_src))
endif
ifeq ($(BUILD_CONFIG_MODULE_AMBA_DSP), n)
MODULE_AMBA_DECODER_SRC := modules/decoders/amba_video_decoder.cpp
modules_decoders_src := $(filter-out $(MODULE_AMBA_DECODER_SRC),$(modules_decoders_src))
endif
modules_decoders_obj := $(modules_decoders_src:modules/decoders/%.cpp=modules/decoders/%.o)

modules_encoders_src := $(wildcard modules/encoders/*.cpp)
ifeq ($(BUILD_CONFIG_MODULE_LIBAAC), n)
MODULE_LIBAAC_ENCODER_SRC := modules/encoders/aac_audio_encoder.cpp
modules_encoders_src := $(filter-out $(MODULE_LIBAAC_ENCODER_SRC),$(modules_encoders_src))
endif
ifeq ($(BUILD_CONFIG_MODULE_FFMPEG), n)
MODULE_AUDIO_FFMPEG_ENCODER_SRC := modules/encoders/ffmpeg_audio_encoder.cpp
modules_encoders_src := $(filter-out $(MODULE_AUDIO_FFMPEG_ENCODER_SRC),$(modules_encoders_src))
endif
ifneq ($(BUILD_CONFIG_MODULE_X264), y)
MODULE_VIDEO_X264_ENCODER_SRC = modules/encoders/x264_video_encoder.cpp
modules_encoders_src := $(filter-out $(MODULE_VIDEO_X264_ENCODER_SRC),$(modules_encoders_src))
endif
ifeq ($(BUILD_CONFIG_MODULE_AMBA_DSP), n)
MODULE_AMBA_ENCODER_SRC := modules/encoders/amba_video_encoder.cpp
modules_encoders_src := $(filter-out $(MODULE_AMBA_ENCODER_SRC),$(modules_encoders_src))
endif
modules_encoders_obj := $(modules_encoders_src:modules/encoders/%.cpp=modules/encoders/%.o)

modules_muxers_src := $(wildcard modules/muxers/*.cpp)
ifeq ($(BUILD_CONFIG_MODULE_FFMPEG), n)
MODULE_FFMPEG_MUXER_SRC := modules/muxers/ffmpeg_muxer.cpp
modules_muxers_src := $(filter-out $(MODULE_FFMPEG_MUXER_SRC),$(modules_muxers_src))
endif
modules_muxers_obj := $(modules_muxers_src:modules/muxers/%.cpp=modules/muxers/%.o)

modules_postprocessors_src := $(wildcard modules/post_processors/*.cpp)
ifeq ($(BUILD_CONFIG_MODULE_AMBA_DSP), n)
MODULE_AMBA_POST_PROCESSOR_SRC = modules/post_processors/amba_video_post_processor.cpp
modules_postprocessors_src := $(filter-out $(MODULE_AMBA_POST_PROCESSOR_SRC),$(modules_postprocessors_src))
endif
modules_postprocessors_obj := $(modules_postprocessors_src:modules/post_processors/%.cpp=modules/post_processors/%.o)

modules_renderers_src := $(wildcard modules/renderers/*.cpp)
ifeq ($(BUILD_CONFIG_MODULE_AMBA_DSP), n)
MODULE_AMBA_RENDERER_SRC := modules/renderers/amba_video_renderer.cpp
modules_renderers_src := $(filter-out $(MODULE_AMBA_RENDERER_SRC),$(modules_renderers_src))
endif
modules_renderers_obj := $(modules_renderers_src:modules/renderers/%.cpp=modules/renderers/%.o)

modules_streamers_src := $(wildcard modules/streamers/*.cpp)
modules_streamers_obj := $(modules_streamers_src:modules/streamers/%.cpp=modules/streamers/%.o)

modules_streaming_servers_src := $(wildcard modules/streaming_servers/*.cpp)
modules_streaming_servers_obj := $(modules_streaming_servers_src:modules/streaming_servers/%.cpp=modules/streaming_servers/%.o)

modules_cloud_servers_src := $(wildcard modules/cloud_servers/*.cpp)
modules_cloud_servers_obj := $(modules_cloud_servers_src:modules/cloud_servers/%.cpp=modules/cloud_servers/%.o)

modules_parsers_src := $(wildcard modules/parsers/*.cpp)
modules_parsers_obj := $(modules_parsers_src:modules/parsers/%.cpp=modules/parsers/%.o)

modules_capturers_src := 
ifeq ($(BUILD_CONFIG_MODULE_ALSA), y)
modules_capturers_src += modules/captures/audio_input_alsa.cpp
endif
ifeq ($(BUILD_CONFIG_MODULE_WINDESKDUP), y)
modules_capturers_src += modules/captures/video_input_windeskdup.cpp
endif
modules_capturers_obj := $(modules_capturers_src:modules/capturers/%.cpp=modules/capturers/%.o)

codecs_src := $(wildcard codecs/*.cpp)
codecs_obj := $(codecs_src:codecs/%.cpp=codecs/%.o)

codecs_parsers_h264_src := $(wildcard codecs/parsers/h264/*.cpp)
codecs_parsers_h264_obj := $(codecs_parsers_h264_src:codecs/parsers/h264/%.cpp=codecs/parsers/h264/%.o)

codecs_parsers_h265_src := $(wildcard codecs/parsers/h265/*.cpp)
codecs_parsers_h265_obj := $(codecs_parsers_h265_src:codecs/parsers/h265/%.cpp=codecs/parsers/h265/%.o)

dsp_platform_src := $(wildcard dsp_platform/*.cpp)
dsp_platform_obj := $(dsp_platform_src:dsp_platform/%.cpp=dsp_platform/%.o)

dsp_platform_i1_src := $(wildcard dsp_platform/amba_i1/*.cpp)
dsp_platform_i1_obj := $(dsp_platform_i1_src:dsp_platform/amba_i1/%.cpp=dsp_platform/amba_i1/%.o)

dsp_platform_s2_src := $(wildcard dsp_platform/amba_s2/*.cpp)
dsp_platform_s2_obj := $(dsp_platform_s2_src:dsp_platform/amba_s2/%.cpp=dsp_platform/amba_s2/%.o)

audio_platform_src := $(wildcard audio_platform/*.cpp)
audio_platform_obj := $(audio_platform_src:audio_platform/%.cpp=audio_platform/%.o)

audio_platform_alsa_src := $(wildcard audio_platform/linux_alsa/*.cpp)
audio_platform_alsa_obj := $(audio_platform_alsa_src:audio_platform/linux_alsa/%.cpp=audio_platform/linux_alsa/%.o)

audio_platform_pulseaudio_src := $(wildcard audio_platform/pulse_audio/*.cpp)
audio_platform_pulseaudio_obj := $(audio_platform_pulseaudio_src:audio_platform/pulse_audio/%.cpp=audio_platform/pulse_audio/%.o)

simple_api_src = $(wildcard simple_api/*.cpp)
simple_api_obj = $(simple_api_src:simple_api/%.cpp=simple_api/%.o)

lib_MODULE_SRC := 	 $(audio_platform_src)	\
			 $(utils_src)   \
             $(framework_src)     \
			 $(engines_src)     \
			 $(simple_api_src)     \
			 $(filters_src)     \
			 $(modules_src)     \
			 $(modules_demuxers_src)     \
			 $(modules_decoders_src)     \
			 $(modules_encoders_src)     \
			 $(modules_muxers_src)     \
			 $(modules_postprocessors_src)     \
			 $(modules_renderers_src)	\
			 $(modules_streamers_src)	\
			 $(modules_streaming_servers_src) \
			 $(modules_cloud_servers_src) \
			 $(modules_parsers_src)	\
			 $(modules_capturers_src) \
			 $(dsp_platform_src) \
			 $(codecs_src) \
			 $(codecs_parsers_h264_src) \
			 $(codecs_parsers_h265_src)

ifeq ($(BUILD_CONFIG_MODULE_AMBA_DSP), y)
ifeq ($(BUILD_CONGIG_DSP_AMBA_I1), y)
lib_MODULE_SRC += $(dsp_platform_i1_src)
endif

ifeq ($(BUILD_CONGIG_DSP_AMBA_S2), y)
lib_MODULE_SRC += $(dsp_platform_s2_src)
endif
endif

ifeq ($(BUILD_CONFIG_MODULE_ALSA), y)
lib_MODULE_SRC += $(audio_platform_alsa_src)

ifeq ($(BUILD_CONFIG_MODULE_PULSEAUDIO), y)
lib_MODULE_SRC += $(audio_platform_pulseaudio_src)
endif
endif

lib_MODULE_OBJ := 	 $(audio_platform_obj)	\
			 $(utils_obj)   \
             $(framework_obj)     \
			 $(engines_obj)     \
			 $(simple_api_obj)     \
			 $(filters_obj)     \
			 $(modules_obj)     \
			 $(modules_demuxers_obj)     \
			 $(modules_decoders_obj)     \
			 $(modules_encoders_obj)     \
			 $(modules_muxers_obj)     \
			 $(modules_postprocessors_obj)     \
			 $(modules_renderers_obj)	\
			 $(modules_streamers_obj)	\
			 $(modules_streaming_servers_obj) \
			 $(modules_cloud_servers_obj) \
			 $(modules_parsers_obj) \
			 $(modules_capturers_obj) \
			 $(dsp_platform_obj) \
			 $(codecs_obj) \
			 $(codecs_parsers_h264_obj) \
			 $(codecs_parsers_h265_obj)

ifeq ($(BUILD_CONFIG_MODULE_AMBA_DSP), y)
ifeq ($(BUILD_CONGIG_DSP_AMBA_I1), y)
lib_MODULE_OBJ += $(dsp_platform_i1_obj)
endif

ifeq ($(BUILD_CONGIG_DSP_AMBA_S2), y)
lib_MODULE_OBJ += $(dsp_platform_s2_obj)
endif
endif

ifeq ($(BUILD_CONFIG_MODULE_ALSA), y)
lib_MODULE_OBJ += $(audio_platform_alsa_obj)

ifeq ($(BUILD_CONFIG_MODULE_PULSEAUDIO), y)
lib_MODULE_OBJ += $(audio_platform_pulseaudio_obj)
endif
endif

STATIC_LIB_NAMES = mwcg_media_mw
#SHARED_LIB_NAMES = mwcg_media_mw

mwcg_media_mw_src     := $(lib_MODULE_SRC)
mwcg_media_mw_obj     := $(lib_MODULE_OBJ)

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk



