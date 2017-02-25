##
## app/utility/mptool/make.inc
##
## History:
##   2016-12-19 - [Yupeng Chang] created file
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

LOCAL_PATH      := $(call my-dir)

ifeq ($(CONFIG_SONY_SD_LIBRARY), y)
SONY_LIB_DIR    := $(LOCAL_PATH)

include $(CLEAR_VARS)
SONY_SD_LIBRARY := libsonysd.so
SONY_SD_VERSION := 20150925
LOCAL_TARGET    :=  $(SONY_SD_LIBRARY)
LOCAL_SRCS      :=  $(SONY_LIB_DIR)/src/sonysd_kernel.c \
                    $(SONY_LIB_DIR)/src/sonysd_lib.c
LOCAL_CFLAGS    := -I$(AMB_TOPDIR)/kernel/linux/include \
                   -I$(SONY_LIB_DIR)/include            \
                   -Werror

SONY_SD_LIB_TARGET := $(SONY_SD_LIBRARY).$(SONY_SD_VERSION)
LOCAL_SO_NAME      := $(SONY_SD_LIB_TARGET)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/usr/lib
	@cp -dpRf $< $(FAKEROOT_DIR)/usr/lib/$(SONY_SD_LIB_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

ifeq ($(CONFIG_SONY_SD_LIBRARY_TEST), y)
SONY_TEST_DIR := $(LOCAL_PATH)

include $(CLEAR_VARS)

LOCAL_TARGET :=  sonysd_test
LOCAL_SRCS   :=  $(SONY_TEST_DIR)/src/sonysd_test.c
LOCAL_CFLAGS := -I$(AMB_TOPDIR)/kernel/linux/include \
                -I$(SONY_TEST_DIR)/include           \
                -I$(SONY_TEST_DIR)/src               \
                -Werror

LOCAL_LIBS := libsonysd.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/usr/bin/
	@cp -dpRf $< $(FAKEROOT_DIR)/usr/bin/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
