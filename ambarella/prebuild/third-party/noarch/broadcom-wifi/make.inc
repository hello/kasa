##
## prebuild/third-party/broadcom-wifi/make.inc
##
## History:
##    2012/05/30 - [Cao Rongrong] Created file
##
## Copyright (c) 2015 Ambarella, Inc.
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

ifeq ($(CONFIG_AMBARELLA_BROADCOM_WIFI_SUPPORT), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_broadcom_wifi
LOCAL_SRCS	:= lib

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
	@echo "Copy $@ Done"

#### WiFi SDIO Interface
ifneq ($(BUILD_BROADCOM_WIFI_SDIO_CHIP_BCM43362), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/broadcom/bcm43362
endif
ifneq ($(BUILD_BROADCOM_WIFI_SDIO_MODULE_AP6441), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/broadcom/ap6441
endif
ifneq ($(BUILD_BROADCOM_WIFI_SDIO_MODULE_AP6234), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/broadcom/ap6234
endif
ifneq ($(BUILD_BROADCOM_WIFI_SDIO_MODULE_AP6234_PS), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/broadcom/ap6234_ps
endif
ifneq ($(BUILD_BROADCOM_WIFI_SDIO_MODULE_AP6212), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/broadcom/ap6212
endif
ifneq ($(BUILD_BROADCOM_WIFI_SDIO_MODULE_AP6452), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/broadcom/ap6452
endif
ifneq ($(BUILD_BROADCOM_WIFI_SDIO_MODULE_AP6212A), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/broadcom/ap6212a
endif
ifneq ($(BUILD_BROADCOM_WIFI_SDIO_MODULE_AP6255), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/broadcom/ap6255
endif
ifneq ($(BUILD_BROADCOM_WIFI_SDIO_MODULE_AP62X2), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/broadcom/ap62x2
endif

#### WiFi USB Interface
ifneq ($(BUILD_BROADCOM_WIFI_USB_BCM43143), y)
	@rm -rf $(FAKEROOT_DIR)/lib/firmware/broadcom/bcm43143
endif

$(call add-target-into-build, $(LOCAL_TARGET))
endif

