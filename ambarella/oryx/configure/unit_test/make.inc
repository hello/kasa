##
## /S2l/oryx/configure/unit_test/make.inc
##
## History:
##   2014-7-30 - [ypchang] created file
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

LOCAL_PATH := $(call my-dir)

ifeq ($(BUILD_AMBARELLA_ORYX_CONFIG_UNIT_TEST), y)

include $(CLEAR_VARS)

CONFIG_FILE := $(LOCAL_PATH)/test_config.lua
TEST_CONFIG := test_config
LOCAL_TARGET := $(TEST_CONFIG)
LOCAL_SRCS   := $(LOCAL_PATH)/test_config.cpp

LOCAL_CFLAGS := -I$(ORYX_DIR)/include           \
                -I$(ORYX_DIR)/include/configure \
                -I$(ORYX_DIR)/include/utility   \
                -std=c++11 -Werror
LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib               \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/lua/usr/lib

LOCAL_LIBS := libamconfig.so \
              libamutil.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_CONF_DIR)
	@mkdir -p $(ORYX_BIN_DIR)
	@cp -dpRf $< $(ORYX_BIN_DIR)/$(TEST_CONFIG)
	@cp $(CONFIG_FILE) $(ORYX_CONF_DIR)

$(call add-target-into-build, $(LOCAL_TARGET))

endif
