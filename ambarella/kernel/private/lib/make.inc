##
## kernel/private/lib/make.inc
##
## History:
##    2012/06/01 - [Cao Rongrong] Created file
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

ifeq ($(BUILD_AMBARELLA_PRIVATE_MODULES), y)
.PHONY: prebuild_firmware

prebuild_firmware: LOCAL_PATH:=$(call my-dir)
prebuild_firmware: basic_fs
	@mkdir -p $(FIRMWARE_INSTALL_PATH)/
ifeq ($(CONFIG_ARCH_S2L), y)
	@-cp -dpRf $(LOCAL_PATH)/firmware_$(AMBARELLA_ARCH)/*.bin $(FIRMWARE_INSTALL_PATH)/
endif
ifeq ($(CONFIG_DSP_FIRMWARE_ENCODE_ONLY), y)
	@-cp -dpRf $(LOCAL_PATH)/firmware_$(AMBARELLA_ARCH)/encode_only/*.bin $(FIRMWARE_INSTALL_PATH)/
else
	@-cp -dpRf $(LOCAL_PATH)/firmware_$(AMBARELLA_ARCH)/generic/*.bin $(FIRMWARE_INSTALL_PATH)/
endif
	@echo "Copy $@ Done."

$(call add-target-into-build, prebuild_firmware)

###
endif

