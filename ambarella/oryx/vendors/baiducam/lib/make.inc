##
## oryx/audio/device/make.inc
##
## History:
##    2014/12/15 - [Shupeng Ren] Created file
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

include $(CLEAR_VARS)

RTMP_BLS_VER_FILE   = $(LOCAL_PATH)/version.h
RTMP_BLS_VER_PREFIX = RTMP_BLS

RTMP_BLS_TARGET  = librtmp-bls.so
LOCAL_TARGET    := $(RTMP_BLS_TARGET)
LOCAL_SRCS      :=
LOCAL_CFLAGS    := -Werror
LOCAL_LIBS      :=

LOCAL_LDFLAGS := -Wl,--whole-archive -L$(LOCAL_PATH) -lrtmp -Wl,--no-whole-archive

RTMP_BLS_SO_VER_MAJOR  := $(shell awk '/define\ $(RTMP_BLS_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(RTMP_BLS_VER_FILE))
RTMP_BLS_SO_VER_MINOR  := $(shell awk '/define\ $(RTMP_BLS_VER_PREFIX)_LIB_MINOR/{print $$3}' $(RTMP_BLS_VER_FILE))
RTMP_BLS_SO_VER_PATCH  := $(shell awk '/define\ $(RTMP_BLS_VER_PREFIX)_LIB_PATCH/{print $$3}' $(RTMP_BLS_VER_FILE))
RTMP_BLS_SO_VER_STRING := $(RTMP_BLS_SO_VER_MAJOR).$(RTMP_BLS_SO_VER_MINOR).$(RTMP_BLS_SO_VER_PATCH)

RTMP_BLS_SO_NAME   = $(patsubst %.so,%.so.$(RTMP_BLS_SO_VER_MAJOR), $(RTMP_BLS_TARGET))
RTMP_BLS_SO_TARGET = $(patsubst %.so,%.so.$(RTMP_BLS_SO_VER_STRING), $(RTMP_BLS_TARGET))

LOCAL_SO_NAME := $(RTMP_BLS_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(RTMP_BLS_SO_TARGET)
	@ln -sf $(RTMP_BLS_SO_TARGET) $(ORYX_LIB_DIR)/$(RTMP_BLS_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
