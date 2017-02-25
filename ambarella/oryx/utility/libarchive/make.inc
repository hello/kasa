##
## oryx/utility/libarchive/make.inc
##
## History:
##    2016/04/18 - [Long Li] Created file
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

## libamarchive.so
ifeq ($(BUILD_AMBARELLA_ORYX_LIBARCHIVE), y)

include $(CLEAR_VARS)

LIBARCHIVE_VER_FILE   = $(ORYX_DIR)/include/utility/libarchive/version.h
LIBARCHIVE_VER_PREFIX = LIBARCHIVE

LIBARCHIVE_TARGET   = libamarchive.so
LOCAL_TARGET := $(LIBARCHIVE_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/am_libarchive.cpp)

LOCAL_CFLAGS := -I$(ORYX_DIR)/include \
                -I$(ORYX_DIR)/include/utility \
                -I$(ORYX_DIR)/include/utility/libarchive \
                -I$(ORYX_DIR)/include/utility/dec7z \
                -I$(ORYX_DIR)/utility/include\
                -I$(PREBUILD_3RD_PARTY_DIR)/libarchive/include \
                -std=c++11 -Werror

LOCAL_LDFLAGS := -L$(PREBUILD_3RD_PARTY_DIR)/libarchive/usr/lib -larchive

LOCAL_LIBS := libamutil.so \
              libamdec7z.so

LIBARCHIVE_SO_VER_MAJOR  := $(shell awk '/define\ $(LIBARCHIVE_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(LIBARCHIVE_VER_FILE))
LIBARCHIVE_SO_VER_MINOR  := $(shell awk '/define\ $(LIBARCHIVE_VER_PREFIX)_LIB_MINOR/{print $$3}' $(LIBARCHIVE_VER_FILE))
LIBARCHIVE_SO_VER_PATCH  := $(shell awk '/define\ $(LIBARCHIVE_VER_PREFIX)_LIB_PATCH/{print $$3}' $(LIBARCHIVE_VER_FILE))
LIBARCHIVE_SO_VER_STRING := $(LIBARCHIVE_SO_VER_MAJOR).$(LIBARCHIVE_SO_VER_MINOR).$(LIBARCHIVE_SO_VER_PATCH)

LIBARCHIVE_SO_NAME   = $(patsubst %.so,%.so.$(LIBARCHIVE_SO_VER_MAJOR), $(LIBARCHIVE_TARGET))
LIBARCHIVE_SO_TARGET = $(patsubst %.so,%.so.$(LIBARCHIVE_SO_VER_STRING), $(LIBARCHIVE_TARGET))

LOCAL_SO_NAME := $(LIBARCHIVE_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(LIBARCHIVE_SO_TARGET)
	@ln -sf $(LIBARCHIVE_SO_TARGET) $(ORYX_LIB_DIR)/$(LIBARCHIVE_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

include $(LOCAL_PATH)/unit_test/make.inc

endif
