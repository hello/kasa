##
## oryx/vendors/baiducam/app/make.inc
##
## History:
##    2015/3/20 - [Shupeng Ren] Created file
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

BAIDUCAM_DIR := $(ORYX_DIR)/vendors/baiducam

###############################################################################
## baiducam
include $(CLEAR_VARS)
DATA_EXPORT_TARGET := baiducam
LOCAL_TARGET       := $(DATA_EXPORT_TARGET)
LOCAL_SRCS         := $(LOCAL_PATH)/am_bls_rtmp.cpp \
                      $(LOCAL_PATH)/am_baiducam.cpp \
                      $(LOCAL_PATH)/main.cpp

LOCAL_CFLAGS       := -I$(ORYX_DIR)/include                                \
                      -I$(ORYX_DIR)/include/stream                         \
                      -I$(ORYX_DIR)/stream/include/record                  \
                      -I$(ORYX_DIR)/stream/include/common/media            \
                      -I$(ORYX_DIR)/include/utility                        \
                      -I$(BAIDUCAM_DIR)/include                            \
                      -I$(PREBUILD_3RD_PARTY_DIR)/json-c/include/json      \
                      -std=c++11 -Werror

LOCAL_LDFLAGS  := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib \
                  -L$(PREBUILD_3RD_PARTY_DIR)/json-c/usr/lib -ljson-c \
                  -lpthread

LOCAL_LIBS := libamexport-client.so \
              libamutil.so \
              libamosal.so \
              librtmp-bls.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###############################################################################
