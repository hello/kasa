##
## ambarella/oryx/app/auto_intelligent/make.inc
##
## History:
##    2015/11/11 - [Huaiqing Wang] Created file
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

ifeq ($(BUILD_AMBARELLA_ORYX_APP_AUTO_INTELLIGENT), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_TARGET := auto_intelligent

LOCAL_SRCS := $(LOCAL_PATH)/auto_intelligent.cpp \
              $(LOCAL_PATH)/air_api_handle.cpp \
              $(LOCAL_PATH)/frame_buffer.cpp

LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include               \
                 -I$(ORYX_DIR)/include/utility       \
                 -I$(ORYX_DIR)/include/ipc    \
                 -I$(ORYX_DIR)/include/services  \
                 -I$(ORYX_DIR)/services/include \
                 -I$(LOCAL_PATH)              \
                 -I$(LOCAL_PATH)/fb           \
                 -std=c++11 -Werror

ifeq ($(BUILD_AMBARELLA_ORYX_VIDEO_NEW), y)
LOCAL_CFLAGS += -I$(ORYX_DIR)/include/video_new
else
LOCAL_CFLAGS += -I$(ORYX_DIR)/include/video
endif

LOCAL_LDFLAGS := -L$(PREBUILD_3RD_PARTY_DIR)/freetype/usr/lib -lfreetype \
                 -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/bzip2/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib

LOCAL_LIBS    :=  libamutil.so libamipc.so libamapi.so


ifeq ($(BUILD_AMBARELLA_ORYX_VIDEO_NEW), y)
LOCAL_LIBS += libamvideo.so
LOCAL_LDFLAGS += -L$(PREBUILD_3RD_PARTY_DIR)/freetype/usr/lib -lfreetype
else
LOCAL_LIBS += libamvideodsp.so
endif

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."


$(call add-target-into-build, $(LOCAL_TARGET))

endif
