##
## /S2L/oryx/config_set/make.inc
##
## History:
##   Apr 7, 2016 - [longli] created file
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
##
LOCAL_PATH   := $(call my-dir)

## libamcfgset.so
ifeq ($(BUILD_AMBARELLA_ORYX_CFG_SET), y)

include $(CLEAR_VARS)

CFG_SET_VER_FILE   = $(ORYX_DIR)/include/config_set/version.h
CFG_SET_VER_PREFIX = CFG_SET

CFG_SET_TARGET = libamcfgset.so

CONFIG_SAMPLES_PATH := $(ORYX_DIR)/config_set/config_samples

LOCAL_TARGET := $(CFG_SET_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/am_config_set.cpp)

LOCAL_CFLAGS := -I$(ORYX_DIR)/include/config_set \
                -I$(ORYX_DIR)/include/utility \
                -I$(ORYX_DIR)/include/utility/libarchive \
                -I$(ORYX_DIR)/include \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so \
              libamarchive.so

CFG_SET_SO_VER_MAJOR  := $(shell awk '/define\ $(CFG_SET_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(CFG_SET_VER_FILE))
CFG_SET_SO_VER_MINOR  := $(shell awk '/define\ $(CFG_SET_VER_PREFIX)_LIB_MINOR/{print $$3}' $(CFG_SET_VER_FILE))
CFG_SET_SO_VER_PATCH  := $(shell awk '/define\ $(CFG_SET_VER_PREFIX)_LIB_PATCH/{print $$3}' $(CFG_SET_VER_FILE))
CFG_SET_SO_VER_STRING := $(CFG_SET_SO_VER_MAJOR).$(CFG_SET_SO_VER_MINOR).$(CFG_SET_SO_VER_PATCH)

CFG_SET_SO_NAME   = $(patsubst %.so,%.so.$(CFG_SET_SO_VER_MAJOR), $(CFG_SET_TARGET))
CFG_SET_SO_TARGET = $(patsubst %.so,%.so.$(CFG_SET_SO_VER_STRING), $(CFG_SET_TARGET))

LOCAL_SO_NAME := $(CFG_SET_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(CFG_SET_SO_TARGET)
	@ln -sf $(CFG_SET_SO_TARGET) $(ORYX_LIB_DIR)/$(CFG_SET_SO_NAME)
	@echo "Build $@ Done."
	@cp -dpRf $(CONFIG_SAMPLES_PATH) $(FAKEROOT_DIR)/etc/
	@echo "Copy config set Done."

$(call add-target-into-build, $(LOCAL_TARGET))

include $(LOCAL_PATH)/unit_test/make.inc

endif
