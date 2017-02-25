##
## app/utility/dsp_tool/make.inc
##
## History:
##    2014/08/04 - [Jian Tang] Created file
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

ifeq ($(BUILD_AMBARELLA_APP_UTILITY_DSP), y)

LOCAL_PATH	:= $(call my-dir)

#################################

ifeq ($(BUILD_AMBARELLA_APP_UTILITY_LOAD_UCODE), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= load_ucode
ifeq ($(CONFIG_ARCH_H2), y)
LOCAL_SRCS	:= $(LOCAL_PATH)/arch_$(AMBARELLA_ARCH)/load_ucode.c
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/kernel/private/lib/firmware_$(AMBARELLA_ARCH)
else
LOCAL_SRCS	:= $(LOCAL_PATH)/load_ucode.c
endif

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/usr/sbin
	@cp -dpRf $< $(FAKEROOT_DIR)/usr/sbin
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif	## BUILD_AMBARELLA_APP_UTILITY_LOAD_UCODE

#################################

ifeq ($(BUILD_AMBARELLA_APP_UTILITY_DSPLOG), y)
include $(CLEAR_VARS)

ifeq ($(CONFIG_ARCH_H2), y)

LOCAL_SRCS	:= $(LOCAL_PATH)/arch_$(AMBARELLA_ARCH)/dsplog_cap.c  $(LOCAL_PATH)/arch_$(AMBARELLA_ARCH)/dsplog_utils.c
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/kernel/private/lib/firmware_$(AMBARELLA_ARCH)
else

LOCAL_SRCS	:= $(LOCAL_PATH)/dsplog_cap.c  $(LOCAL_PATH)/arch_$(AMBARELLA_ARCH)/dsplog_utils.c
LOCAL_CFLAGS    :=
endif ####### CONFIG_ARCH_H2


LOCAL_TARGET	:= dsplog_cap
LOCAL_CFLAGS	+= -I$(AMB_TOPDIR)/packages/data_transfer
LOCAL_LIBS	:= libdatatx.a
LOCAL_LDFLAGS	:= -lpthread
include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(APP_INSTALL_PATH)/
	@cp -dpRf $< $(APP_INSTALL_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif	## BUILD_AMBARELLA_APP_UTILITY_DSPLOG

#################################

include $(LOCAL_PATH)/arch_$(AMBARELLA_ARCH)/make.inc

endif	## BUILD_AMBARELLA_APP_UTILITY_DSP

