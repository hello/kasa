###############################################################################
## $(MODULE_NAME_TAG)/media_mw/unit_test/Makefile
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
include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/extern_lib.mk

.PHONY: all clean

MODULE_INC = -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/common/include -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/extern_lib/include -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/security_utils/include -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/storage_lib/include -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/cloud_lib/include -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/media_mw/include
MODULE_LIB = -lrt -lpthread -L$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/lib -lmwcg_media_mw -lmwcg_cloud_lib -lmwcg_storage_lib -lmwcg_security_lib
ifeq ($(BUILD_CONFIG_WITH_LICENCE), y)
MODULE_LIB += -lmwcg_licence
endif
MODULE_LIB += -lmwcg_common
##MODULE_INC += -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/ws_discovery/include
##MODULE_LIB += -L$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/ws_discovery/lib -lwsd
ifeq ($(BUILD_CONFIG_MODULE_FFMPEG), y)
MODULE_LIB += $(EXTERN_LIB_FFMPEG_LIB) -lavcodec -lavformat -lavutil
endif
ifeq ($(BUILD_CONFIG_MODULE_LIBXML2), y)
MODULE_LIB += $(EXTERN_LIB_LIBXML2_LIB) -lxml2
endif
ifeq ($(BUILD_CONFIG_MODULE_LIBAAC), y)
MODULE_LIB += $(EXTERN_LIB_LIBAAC_LIB) -laacdec -laacenc
endif
ifeq ($(BUILD_CONFIG_MODULE_ALSA), y)
ifeq ($(BUILD_CONFIG_MODULE_PULSEAUDIO), y)
MODULE_LIB += $(EXTERN_LIB_PULSEAUDIO_LIB) -lpulsecommon-4.0 -lpulsecore-4.0 -lpulse-mainloop-glib -lpulse-simple -lpulse
MODULE_LIB += $(EXTERN_LIB_LIBFFI_LIB) -lffi
MODULE_LIB += $(EXTERN_LIB_EXPAT_LIB) -lexpat
MODULE_LIB += $(EXTERN_LIB_LIBSNDFILE_LIB) -lsndfile
MODULE_LIB += $(EXTERN_LIB_DBUS_LIB) -ldbus-1
MODULE_LIB += $(EXTERN_LIB_LIBTOOL_LIB) -lltdl
MODULE_LIB += $(EXTERN_LIB_LIBSAMPLERATE_LIB) -lsamplerate
MODULE_LIB += $(EXTERN_LIB_LIBSPEEX_LIB) -lspeexdsp
MODULE_LIB += $(EXTERN_LIB_JSON-C_LIB) -ljson-c
MODULE_LIB += $(EXTERN_LIB_ORC_LIB) -lorc-0.4
MODULE_LIB += $(EXTERN_LIB_GLIB2_LIB) -lglib-2.0 -lgobject-2.0 -lgmodule-2.0 -lgio-2.0 -lgthread-2.0
MODULE_LIB += $(EXTERN_LIB_ZLIB_LIB) -lz
endif
MODULE_LIB += $(EXTERN_LIB_ALSA_LIB) -lasound
endif
MODULE_LIB += -lpthread -ldl -lrt
##MODULE_SRC = mtest.cpp
##MODULE_TARGET = $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/bin/mtest

MTEST2_MODULE_SRC = mtest2.cpp
MTEST2_MODULE_TARGET = $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/bin/mtest2

MTEST2_PUSHMODE_MODULE_SRC = mtest2_push.cpp
MTEST2_PUSHMODE_MODULE_TARGET = $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/bin/cloud_relay_server

REMOTE_DEBUG_MODULE_SRC = remote_debug.cpp
REMOTE_DEBUG_MODULE_TARGET = $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/bin/remote_debug

VOD_TEST_MODULE_SRC = vod_test.cpp
VOD_TEST_MODULE_TARGET = $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/bin/vod_test

CLOUD_TEST_MODULE_SRC = cloud_test.cpp
CLOUD_TEST_MODULE_TARGET = $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/bin/cloud_test


include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk

all:
##	$(MAKE_PRINT)echo "build mtest:"
##	$(MAKE_PRINT)mkdir -p $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/bin
##	$(MAKE_PRINT)$(CXX) -o $(MODULE_TARGET) $(MODULE_INC) -g -O0 $(MODULE_SRC) $(MODULE_LIB)
##	-$(MAKE_PRINT)$(CP) $(MODULE_TARGET) $(MWCG_BINDIR)/

	$(MAKE_PRINT)echo "build mtest2:"
	$(MAKE_PRINT)$(CXX) -o $(MTEST2_MODULE_TARGET) $(MODULE_INC) -g -O0 $(MTEST2_MODULE_SRC) $(MODULE_LIB)
ifneq ($(BUILD_AMBARELLA_MWCG_DEBUG), y)
	$(MAKE_PRINT)$(STRIP) $(MTEST2_MODULE_TARGET)
endif
##	-$(MAKE_PRINT)$(CP) $(MTEST2_MODULE_TARGET) $(MWCG_BINDIR)/

	$(MAKE_PRINT)echo "build cloud_relay_server:"
	$(MAKE_PRINT)$(CXX) -o $(MTEST2_PUSHMODE_MODULE_TARGET) $(MODULE_INC) -g -O0 $(MTEST2_PUSHMODE_MODULE_SRC) $(MODULE_LIB)
ifneq ($(BUILD_AMBARELLA_MWCG_DEBUG), y)
	$(MAKE_PRINT)$(STRIP) $(MTEST2_PUSHMODE_MODULE_TARGET)
endif
##	-$(MAKE_PRINT)$(CP) $(MTEST2_PUSHMODE_MODULE_TARGET) $(MWCG_BINDIR)/

	$(MAKE_PRINT)echo "build remote_debug:"
	$(MAKE_PRINT)$(CXX) -o $(REMOTE_DEBUG_MODULE_TARGET) $(MODULE_INC) -g -O0 $(REMOTE_DEBUG_MODULE_SRC) $(MODULE_LIB)
##	-$(MAKE_PRINT)$(CP) $(REMOTE_DEBUG_MODULE_TARGET) $(MWCG_BINDIR)/

##	$(MAKE_PRINT)echo "build camera_test:"
##	$(MAKE_PRINT)$(CXX) -o $(CAMERA_TEST_MODULE_TARGET) $(MODULE_INC) -g -O0 $(CAMERA_TEST_MODULE_SRC) $(MODULE_LIB)
##	-$(MAKE_PRINT)$(CP) $(CAMERA_TEST_MODULE_TARGET) $(MWCG_BINDIR)/

##	$(MAKE_PRINT)echo "build audio_test:"
##	$(MAKE_PRINT)$(CXX) -o $(AUDIO_TEST_MODULE_TARGET) $(MODULE_INC) -g -O0 $(AUDIO_TEST_MODULE_SRC) $(MODULE_LIB)
##	-$(MAKE_PRINT)$(CP) $(AUDIO_TEST_MODULE_TARGET) $(MWCG_BINDIR)/

##	$(MAKE_PRINT)echo "build audio_adpcm_test:"
##	$(MAKE_PRINT)$(CXX) -o $(AUDIO_ADPCM_TEST_MODULE_TARGET) $(MODULE_INC) -g -O0 $(AUDIO_ADPCM_TEST_MODULE_SRC) $(MODULE_LIB)
##	-$(MAKE_PRINT)$(CP) $(AUDIO_ADPCM_TEST_MODULE_TARGET) $(MWCG_BINDIR)/

	$(MAKE_PRINT)echo "build vod_test:"
	$(MAKE_PRINT)$(CXX) -o $(VOD_TEST_MODULE_TARGET) $(MODULE_INC) -g -O0 $(VOD_TEST_MODULE_SRC) $(MODULE_LIB)

	$(MAKE_PRINT)echo "build cloud_test:"
	$(MAKE_PRINT)$(CXX) -o $(CLOUD_TEST_MODULE_TARGET) $(MODULE_INC) -g -O0 $(CLOUD_TEST_MODULE_SRC) $(MODULE_LIB)

clean:
##	$(MAKE_PRINT)echo "clean mtest:"
##	$(MAKE_PRINT)$(RM) $(MODULE_TARGET)
	$(MAKE_PRINT)echo "clean mtest2:"
	$(MAKE_PRINT)$(RM) $(MTEST2_MODULE_TARGET)
	$(MAKE_PRINT)echo "clean cloud_relay_server:"
	$(MAKE_PRINT)$(RM) $(MTEST2_PUSHMODE_MODULE_TARGET)
	$(MAKE_PRINT)echo "clean remote_debug:"
	$(MAKE_PRINT)$(RM) $(REMOTE_DEBUG_MODULE_TARGET)
##	$(MAKE_PRINT)echo "clean camera_test:"
##	$(MAKE_PRINT)$(RM) $(CAMERA_TEST_MODULE_TARGET)
##	$(MAKE_PRINT)echo "clean audio_test:"
##	$(MAKE_PRINT)$(RM) $(AUDIO_TEST_MODULE_TARGET)
	$(MAKE_PRINT)echo "clean vod_test:"
	$(MAKE_PRINT)$(RM) $(VOD_TEST_MODULE_TARGET)




