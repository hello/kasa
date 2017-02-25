##
## /S2L/oryx/led/led_handler/make.inc
##
## History:
##   2015-2-6 - [longli] created file
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

ifeq ($(BUILD_AMBARELLA_ORYX_LED), y)

LOCAL_PATH   := $(call my-dir)

#####
include $(CLEAR_VARS)

GPIO_LED_VER_FILE   = $(ORYX_DIR)/include/led/version.h
GPIO_LED_VER_PREFIX = GPIO_LED
GPIO_LED_TARGET = libamled.so

LOCAL_TARGET  := $(GPIO_LED_TARGET)
LOCAL_SRCS    := $(wildcard $(LOCAL_PATH)/am_led_handler.cpp)

LOCAL_CFLAGS := -I$(ORYX_DIR)/include/utility \
                -I$(ORYX_DIR)/include \
                -I$(ORYX_DIR)/include/led \
                -I$(ORYX_DIR)/led/include \
                -std=c++11 -lpthread -Werror

LOCAL_LDFLAGS := -pthread

LOCAL_LIBS := libamutil.so

ifeq ($(BUILD_AMBARELLA_ORYX_DEBUG), y)
LOCAL_CFLAGS += -O0 -g
endif

GPIO_LED_SO_VER_MAJOR  := $(shell awk '/define\ $(GPIO_LED_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(GPIO_LED_VER_FILE))
GPIO_LED_SO_VER_MINOR  := $(shell awk '/define\ $(GPIO_LED_VER_PREFIX)_LIB_MINOR/{print $$3}' $(GPIO_LED_VER_FILE))
GPIO_LED_SO_VER_PATCH  := $(shell awk '/define\ $(GPIO_LED_VER_PREFIX)_LIB_PATCH/{print $$3}' $(GPIO_LED_VER_FILE))
GPIO_LED_SO_VER_STRING := $(GPIO_LED_SO_VER_MAJOR).$(GPIO_LED_SO_VER_MINOR).$(GPIO_LED_SO_VER_PATCH)

GPIO_LED_SO_NAME   = $(patsubst %.so,%.so.$(GPIO_LED_SO_VER_MAJOR), $(GPIO_LED_TARGET))
GPIO_LED_SO_TARGET = $(patsubst %.so,%.so.$(GPIO_LED_SO_VER_STRING), $(GPIO_LED_TARGET))

LOCAL_SO_NAME := $(GPIO_LED_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(GPIO_LED_SO_TARGET)
	@ln -sf $(GPIO_LED_SO_TARGET) $(ORYX_LIB_DIR)/$(GPIO_LED_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif