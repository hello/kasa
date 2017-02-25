###############################################################################
## $(MODULE_NAME_TAG)/build/core/linux/extern_lib.mk
##
## History:
##    2013/03/16 - [Zhi He] Create
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

EXTERN_LIB_FFMPEG_DIR = $(BUILDSYSTEM_DIR)/mw_ce/ffmpeg_v0.7
EXTERN_LIB_LIBXML2_DIR = $(BUILDSYSTEM_DIR)/prebuild/third-party/armv6k/libxml2/
EXTERN_LIB_LIBAAC_DIR = $(BUILDSYSTEM_DIR)/prebuild/third-party/armv6k/aac/
EXTERN_LIB_ALSA_DIR = $(BUILDSYSTEM_DIR)/prebuild/third-party/armv6k/alsa-lib/
EXTERN_LIB_PULSEAUDIO_DIR = $(BUILDSYSTEM_DIR)/prebuild/third-party/armv6k/pulseaudio/
EXTERN_BUILD_DIR = $(BUILDSYSTEM_DIR)/build/

EXTERN_LIB_FFMPEG_INC = -I$(EXTERN_LIB_FFMPEG_DIR)/libavutil -I$(EXTERN_LIB_FFMPEG_DIR) \
	-I$(EXTERN_LIB_FFMPEG_DIR)/libavcodec -I$(EXTERN_LIB_FFMPEG_DIR)/libavformat 
EXTERN_LIB_LIBXML2_INC = -I$(EXTERN_LIB_LIBXML2_DIR)/include/
EXTERN_LIB_LIBAAC_INC = -I$(EXTERN_LIB_LIBAAC_DIR)/include/ -I$(BUILDSYSTEM_DIR)/include/
EXTERN_LIB_ALSA_INC = -I$(EXTERN_LIB_ALSA_DIR)/include
EXTERN_LIB_PULSEAUDIO_INC = -I$(EXTERN_LIB_PULSEAUDIO_DIR)/include
EXTERN_BUILD_INC = -I$(EXTERN_BUILD_DIR)/include/
	
EXTERN_LIB_FFMPEG_LIB = -L$(EXTERN_LIB_FFMPEG_DIR)/build/linux
EXTERN_LIB_LIBXML2_LIB = -L$(EXTERN_LIB_LIBXML2_DIR)/lib/
EXTERN_LIB_LIBAAC_LIB = -L$(EXTERN_LIB_LIBAAC_DIR)/lib/
EXTERN_LIB_ALSA_LIB = -L$(EXTERN_LIB_ALSA_DIR)/usr/lib/
EXTERN_LIB_PULSEAUDIO_LIB = -L$(EXTERN_LIB_PULSEAUDIO_DIR)/usr/lib/

#third party lib for pulse audio
EXTERN_LIB_LIBFFI_LIB = -L$(BUILDSYSTEM_DIR)/prebuild/third-party/armv6k/libffi/usr/lib
EXTERN_LIB_EXPAT_LIB = -L$(BUILDSYSTEM_DIR)/prebuild/third-party/armv6k/expat/usr/lib
EXTERN_LIB_LIBSNDFILE_LIB = -L$(BUILDSYSTEM_DIR)/prebuild/third-party/armv6k/libsndfile/usr/lib
EXTERN_LIB_DBUS_LIB = -L$(BUILDSYSTEM_DIR)/prebuild/third-party/armv6k/dbus/usr/lib
EXTERN_LIB_LIBTOOL_LIB = -L$(BUILDSYSTEM_DIR)/prebuild/third-party/armv6k/libtool/usr/lib
EXTERN_LIB_LIBSAMPLERATE_LIB = -L$(BUILDSYSTEM_DIR)/prebuild/third-party/armv6k/libsamplerate/usr/lib
EXTERN_LIB_LIBSPEEX_LIB = -L$(BUILDSYSTEM_DIR)/prebuild/third-party/armv6k/speex/usr/lib
EXTERN_LIB_JSON-C_LIB = -L$(BUILDSYSTEM_DIR)/prebuild/third-party/armv6k/json-c/usr/lib
EXTERN_LIB_ORC_LIB = -L$(BUILDSYSTEM_DIR)/prebuild/third-party/armv6k/orc/usr/lib
EXTERN_LIB_GLIB2_LIB = -L$(BUILDSYSTEM_DIR)/prebuild/third-party/armv6k/glib2/usr/lib
EXTERN_LIB_ZLIB_LIB = -L$(BUILDSYSTEM_DIR)/prebuild/third-party/armv6k/zlib/usr/lib
