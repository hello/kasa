##
## packages/main_preproc/make.inc
##
## History:
##    2013/06/05 - [qianshen] Created file
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

ifeq ($(BUILD_AMBARELLA_MAINPREPROC_PACKAGE), y)

LOCAL_PATH := $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET  := libmainpp.so

LOCAL_SRCS    := $(wildcard $(LOCAL_PATH)/*.c)
LOCAL_CFLAGS  := -I$(AMB_TOPDIR)/build/include \
                 -I$(AMB_TOPDIR)/packages/utils

MAINPP_MODULE	= libmainpp.so
MAINPP_VER_PREFIX = MAINPP
MAINPP_VERSION_FILE = $(LOCAL_PATH)/version.c
MAINPP_SO_VER_MAJOR  := $(shell awk '/define\ $(MAINPP_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(MAINPP_VERSION_FILE))
MAINPP_SO_VER_MINOR  := $(shell awk '/define\ $(MAINPP_VER_PREFIX)_LIB_MINOR/{print $$3}' $(MAINPP_VERSION_FILE))
MAINPP_SO_VER_PATCH  := $(shell awk '/define\ $(MAINPP_VER_PREFIX)_LIB_PATCH/{print $$3}' $(MAINPP_VERSION_FILE))
MAINPP_SO_VER_STRING := $(MAINPP_SO_VER_MAJOR).$(MAINPP_SO_VER_MINOR).$(MAINPP_SO_VER_PATCH)
MAINPP_SO_NAME   = $(patsubst %.so,%.so.$(MAINPP_SO_VER_MAJOR), $(MAINPP_MODULE))
MAINPP_SO_TARGET = $(patsubst %.so,%.so.$(MAINPP_SO_VER_STRING), $(MAINPP_MODULE))

LOCAL_SO_NAME := $(MAINPP_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/usr/lib/
	@cp -dpRf $< $(FAKEROOT_DIR)/usr/lib/$(MAINPP_SO_TARGET)
	@ln -sf $(MAINPP_SO_TARGET) $(FAKEROOT_DIR)/usr/lib/$(MAINPP_SO_NAME)
	@ln -sf $(MAINPP_SO_NAME) $(FAKEROOT_DIR)/usr/lib/$(MAINPP_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
