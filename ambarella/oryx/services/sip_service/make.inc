##
## oryx/services/sip_service/make.inc
##
## History:
##    2015/01/29 - [Shiming Dong] Created file
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


ifeq ($(BUILD_AMBARELLA_ORYX_SIP_SERVICE), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

SIP_SVC_NAME = sip_svc
###############################################################################
LOCAL_TARGET := $(SIP_SVC_NAME)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/am_sip_ua_server_main.cpp  \
                $(LOCAL_PATH)/am_sip_ua_server_msg_action.cpp)
LOCAL_CFLAGS :=  -I$(LOCAL_PATH)                         \
                 -I$(ORYX_DIR)/include                   \
                 -I$(ORYX_DIR)/include/ipc               \
                 -I$(ORYX_DIR)/include/utility           \
                 -I$(ORYX_DIR)/include/protocol          \
                 -I$(ORYX_DIR)/include/services          \
                 -I$(ORYX_DIR)/include/services/commands \
                 -I$(ORYX_DIR)/services/include          \
                 -std=c++11 -Werror

LOCAL_LIBS    := libamutil.so          \
                 libamipc.so           \
                 libamosal.so          \
                 libamservice-frame.so \
                 libamprotocol-sip.so

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libeXosip2/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libosip2/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/openssl/usr/lib


include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@mkdir -p $(ORYX_CONF_DIR)/apps/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

### libamsip-data-struct.so
include $(CLEAR_VARS)

SIP_DATA_STRUCT = libamsip-data-struct.so

LOCAL_TARGET := $(SIP_DATA_STRUCT)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/am_sip_ua_server_data_structure.cpp)

LOCAL_CFLAGS := -I$(ORYX_DIR)/include                 \
                -I$(ORYX_DIR)/include/protocol        \
                -I$(ORYX_DIR)/include/utility         \
                -I$(ORYX_DIR)/include/services        \
                -I$(ORYX_DIR)/include/service/commands\
                -I$(ORYX_DIR)/include/ipc             \
                -std=c++11 -Werror

LOCAL_LIBS := libamprotocol-sip.so     \
              libamutil.so

SIP_DATA_STRUCT_NAME   = $(SIP_DATA_STRUCT)
SIP_DATA_STRUCT_TARGET = $(SIP_DATA_STRUCT)
LOCAL_SO_NAME := $(SIP_DATA_STRUCT)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(SIP_DATA_STRUCT_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
