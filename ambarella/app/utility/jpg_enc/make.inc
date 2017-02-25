##
## app/jpg_enc/make.inc
##
## History:
##    2014/11/17 - [Shaohua Fan] Create
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

LOCAL_PATH	:= $(call my-dir)

ifeq ($(BUILD_AMBARELLA_APP_UTILITY_JPG), y)
ifeq ($(BUILD_AMBARELLA_DATATX_PACKAGE), y)
ifeq ($(CONFIG_AMBARELLA_LIBJPEG_TURBO_SUPPORT), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= jpg_enc
LOCAL_SRCS	:= $(LOCAL_PATH)/jpg_enc.c $(LOCAL_PATH)/chrome_convert.S
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/data_transfer \
		-I$(PREBUILD_3RD_PARTY_DIR)/libjpeg-turbo/include

LOCAL_LDFLAGS	:= -L$(PREBUILD_3RD_PARTY_DIR)/libjpeg-turbo/usr/lib -ljpeg
LOCAL_LIBS	:= libdatatx.a

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif
endif
endif

