##
## /S2L/oryx/app/doorbell/make.inc
##
## History:
##   April 8, 2016 - [smdong] created file
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
include $(CLEAR_VARS)

ifeq ($(BUILD_AMBARELLA_ORYX_APP_APPS_DOORBELL), y)

LOCAL_TARGET := oryx_doorbell_demo
DOORBELL_USER_GUIDE := oryx_doorbell_demo.txt
LOCAL_SRCS   := $(LOCAL_PATH)/oryx_doorbell_demo.cpp
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include               \
                 -I$(ORYX_DIR)/include/utility       \
                 -I$(ORYX_DIR)/include/event         \
                 -I$(ORYX_DIR)/include/audio         \
                 -I$(ORYX_DIR)/include/ipc           \
                 -I$(ORYX_DIR)/include/stream        \
                 -I$(ORYX_DIR)/include/services      \
                 -I$(ORYX_DIR)/services/include      \
                 -I$(ORYX_DIR)/include/audio/device  \
                 -I$(LOCAL_PATH)                     \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib \
                 -lpthread

LOCAL_LIBS    := libamipc.so              \
                 libamapi.so              \
                 libamutil.so             \
                 libamosal.so             \
                 libamstream-playback.so  \
                 libamstream-amf.so       \
                 libamevent-monitor.so    \
                 libamservice-manager.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): DOORBELL_SRC_PATH := $(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/usr/share/oryx/doc
	@cp -dpRf $(DOORBELL_SRC_PATH)/$(DOORBELL_USER_GUIDE) $(FAKEROOT_DIR)/usr/share/oryx/doc
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif