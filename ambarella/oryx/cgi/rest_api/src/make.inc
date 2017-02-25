##
## oryx/cgi/rest_api/make.inc
##
## History:
##    2015/08/10 - [Huaiqing Wang] Created file
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
ORYX_WEB_PATH	:= $(call my-dir)
ORYX_WEB_APPS_PATH  := $(call my-dir)/../../web_apps/
include $(CLEAR_VARS)

ifeq ($(BUILD_AMBARELLA_ORYX_CGI), y)
###############################################################################
LOCAL_TARGET := oryx.cgi
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include            \
                 -I$(ORYX_DIR)/include/ipc        \
                 -I$(ORYX_DIR)/include/video      \
                 -I$(ORYX_DIR)/include/utility    \
                 -I$(ORYX_DIR)/include/services   \
                 -I$(ORYX_DIR)/services/include   \
                 -I$(ORYX_DIR)/cgi/rest_api/include   \
                 -I$(LOCAL_PATH)                  \
                 -I$(PREBUILD_3RD_PARTY_DIR)/json-c/include/json      \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib  \
                 -L$(PREBUILD_3RD_PARTY_DIR)/json-c/usr/lib -ljson-c
LOCAL_LIBS    := libamutil.so libamipc.so libamapi.so libammedia-data-struct.so

ifeq ($(BUILD_AMBARELLA_ORYX_WEB_SONY_SD), y)
LOCAL_CFLAGS  +=  -I$(PREBUILD_3RD_PARTY_NOARCH)/sonysd/include
LOCAL_LIBS    += libsonysd.so
endif

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/webSvr/web/cgi-bin
	@cp -dpRf $< $(FAKEROOT_DIR)/webSvr/web/cgi-bin/
	@echo "Build $@ Done."
ifeq ($(BUILD_AMBARELLA_ORYX_WEB_APPS), y)
ifeq ($(BUILD_AMBARELLA_ORYX_WEB_DOCUMENT), y)
	@mkdir -p $(FAKEROOT_DIR)/webSvr/web/media/getting_started
	@cp -dpRf $(ORYX_DIR)/document/getting_started/* $(FAKEROOT_DIR)/webSvr/web/media/getting_started
endif
	@cp -dpRf $(ORYX_WEB_APPS_PATH)/web_page/* $(FAKEROOT_DIR)/webSvr/web
	@echo "Copy $@ Done."
endif

$(call add-target-into-build, $(LOCAL_TARGET))

###############################################################################
endif
