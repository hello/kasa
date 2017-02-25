##
## prebuild/ambarella/library/smartcam/lens_mask_det/make.inc
##
## History:
##    2016/11/08 - [Ming Wen] Created file
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

###

include $(CLEAR_VARS)
ifeq ($(CONFIG_AMBARELLA_LENS_MASK_DET_LIBRARY), y)
ifneq ($(BUILD_AMBARELLA_SMARTCAM_PACKAGE_LENS_MASK_DET),y)
.PHONY: pre_lens_mask_det_lib
pre_lens_mask_det_lib: PRIVATE_PATH:=$(LOCAL_PATH)
pre_lens_mask_det_lib:
	@mkdir -p $(FAKEROOT_DIR)/usr/lib/
	@cp -dpRf $(PRIVATE_PATH)/lib/arch_$(AMBARELLA_ARCH)/*.so* $(FAKEROOT_DIR)/usr/lib/
	@echo "Copy $@ Done."

$(call add-target-into-build, pre_lens_mask_det_lib)
endif
endif

