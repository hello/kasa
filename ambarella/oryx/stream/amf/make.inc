##
## oryx/amf/make.inc
##
## History:
##    2014/07/22 - [Yupeng Chang] Created file
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

ifeq ($(BUILD_AMBARELLA_ORYX_STREAM), y)

LOCAL_PATH   := $(call my-dir)

## libamf.so
include $(CLEAR_VARS)

AMF_VER_FILE   = $(ORYX_DIR)/stream/include/amf/version.h
AMF_VER_PREFIX = AMF

AMF_TARGET  = libamstream-amf.so
LOCAL_TARGET := $(AMF_TARGET)
LOCAL_SRCS   := $(LOCAL_PATH)/am_amf_base.cpp          \
                $(LOCAL_PATH)/am_amf_queue.cpp         \
                $(LOCAL_PATH)/am_amf_packet.cpp        \
                $(LOCAL_PATH)/am_amf_msgsys.cpp        \
                $(LOCAL_PATH)/am_amf_interface.cpp     \
                $(LOCAL_PATH)/am_amf_packet_pin.cpp    \
                $(LOCAL_PATH)/am_amf_packet_pool.cpp   \
                $(LOCAL_PATH)/am_amf_engine_frame.cpp  \
                $(LOCAL_PATH)/am_amf_packet_filter.cpp

LOCAL_CFLAGS := -I$(ORYX_DIR)/stream/include/amf    \
                -I$(ORYX_DIR)/stream/include/common \
                -I$(ORYX_DIR)/include/utility       \
                -I$(ORYX_DIR)/include               \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so \
              libamosal.so

AMF_SO_VER_MAJOR  := $(shell awk '/define\ $(AMF_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(AMF_VER_FILE))
AMF_SO_VER_MINOR  := $(shell awk '/define\ $(AMF_VER_PREFIX)_LIB_MINOR/{print $$3}' $(AMF_VER_FILE))
AMF_SO_VER_PATCH  := $(shell awk '/define\ $(AMF_VER_PREFIX)_LIB_PATCH/{print $$3}' $(AMF_VER_FILE))
AMF_SO_VER_STRING := $(AMF_SO_VER_MAJOR).$(AMF_SO_VER_MINOR).$(AMF_SO_VER_PATCH)

AMF_SO_NAME   = $(patsubst %.so,%.so.$(AMF_SO_VER_MAJOR), $(AMF_TARGET))
AMF_SO_TARGET = $(patsubst %.so,%.so.$(AMF_SO_VER_STRING), $(AMF_TARGET))

LOCAL_SO_NAME := $(AMF_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(AMF_SO_TARGET)
	@ln -sf $(AMF_SO_TARGET) $(ORYX_LIB_DIR)/$(AMF_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
