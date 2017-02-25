##
## utility/jpeg_exif/make.inc
##
## History:
##    2016/07/27 - [smdong] Created file
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

LOCAL_PATH   := $(call my-dir)

##libamjpeg-exif.so
ifeq ($(BUILD_AMBARELLA_ORYX_JPEG_EXIF), y)

include $(CLEAR_VARS)

JPEG_EXIF_VER_FILE   = $(ORYX_DIR)/include/utility/jpeg_exif/version.h
JPEG_EXIF_VER_PREFIX = JPEG_EXIF

JPEG_EXIF_TARGET = libamjpeg-exif.so
LOCAL_TARGET := $(JPEG_EXIF_TARGET)
LOCAL_SRCS   := $(LOCAL_PATH)/am_jpeg_exif.cpp

LOCAL_CFLAGS := -I$(ORYX_DIR)/include         \
                -I$(ORYX_DIR)/include/utility \
                -I$(ORYX_DIR)/utility/include  \
                -I$(ORYX_DIR)/include/utility/jpeg_exif  \
                -I$(PREBUILD_3RD_PARTY_DIR)/libexif/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/libjpeg-turbo/include \
                -std=c++11 -Werror

LOCAL_LDFLAGS := -L$(PREBUILD_3RD_PARTY_DIR)/libexif/usr/lib -lexif \
                 -L$(PREBUILD_3RD_PARTY_DIR)/libjpeg-turbo/usr/lib -lturbojpeg
                 -Wl,-rpath=$(patsubst $(FAKEROOT_DIR)/%,%,$(ORYX_MUXER_DIR)) \
                 -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib

LOCAL_LIBS   := libamutil.so libamosal.so

JPEG_EXIF_SO_VER_MAJOR  := $(shell awk '/define\ $(JPEG_EXIF_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(JPEG_EXIF_VER_FILE))
JPEG_EXIF_SO_VER_MINOR  := $(shell awk '/define\ $(JPEG_EXIF_VER_PREFIX)_LIB_MINOR/{print $$3}' $(JPEG_EXIF_VER_FILE))
JPEG_EXIF_SO_VER_PATCH  := $(shell awk '/define\ $(JPEG_EXIF_VER_PREFIX)_LIB_PATCH/{print $$3}' $(JPEG_EXIF_VER_FILE))
JPEG_EXIF_SO_VER_STRING := $(JPEG_EXIF_SO_VER_MAJOR).$(JPEG_EXIF_SO_VER_MINOR).$(JPEG_EXIF_SO_VER_PATCH)

JPEG_EXIF_SO_NAME   = $(patsubst %.so,%.so.$(JPEG_EXIF_SO_VER_MAJOR), $(JPEG_EXIF_TARGET))
JPEG_EXIF_SO_TARGET = $(patsubst %.so,%.so.$(JPEG_EXIF_SO_VER_STRING), $(JPEG_EXIF_TARGET))

LOCAL_SO_NAME := $(JPEG_EXIF_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(JPEG_EXIF_SO_TARGET)
	@ln -sf $(JPEG_EXIF_SO_TARGET) $(ORYX_LIB_DIR)/$(JPEG_EXIF_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
