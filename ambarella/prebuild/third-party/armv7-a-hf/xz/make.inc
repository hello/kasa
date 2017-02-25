##
## prebuild/third-party/armv7-a-hf/xz/make.inc
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

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_xz
LOCAL_SRCS	:= usr

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): basic_fs
$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
	$(call overwrite-bb)
ifneq ($(CONFIG_AMBARELLA_XZ_UTILS_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/bin/lzcat
	@rm -rf $(FAKEROOT_DIR)/usr/bin/lzcmp
	@rm -rf $(FAKEROOT_DIR)/usr/bin/lzdiff
	@rm -rf $(FAKEROOT_DIR)/usr/bin/lzegrep
	@rm -rf $(FAKEROOT_DIR)/usr/bin/lzfgrep
	@rm -rf $(FAKEROOT_DIR)/usr/bin/lzgrep
	@rm -rf $(FAKEROOT_DIR)/usr/bin/lzless
	@rm -rf $(FAKEROOT_DIR)/usr/bin/lzma
	@rm -rf $(FAKEROOT_DIR)/usr/bin/lzmadec
	@rm -rf $(FAKEROOT_DIR)/usr/bin/lzmainfo
	@rm -rf $(FAKEROOT_DIR)/usr/bin/lzmore
	@rm -rf $(FAKEROOT_DIR)/usr/bin/unlzma
	@rm -rf $(FAKEROOT_DIR)/usr/bin/unxz
	@rm -rf $(FAKEROOT_DIR)/usr/bin/xz
	@rm -rf $(FAKEROOT_DIR)/usr/bin/xzcat
	@rm -rf $(FAKEROOT_DIR)/usr/bin/xzcmp
	@rm -rf $(FAKEROOT_DIR)/usr/bin/xzdec
	@rm -rf $(FAKEROOT_DIR)/usr/bin/xzdiff
	@rm -rf $(FAKEROOT_DIR)/usr/bin/xzegrep
	@rm -rf $(FAKEROOT_DIR)/usr/bin/xzfgrep
	@rm -rf $(FAKEROOT_DIR)/usr/bin/xzgrep
	@rm -rf $(FAKEROOT_DIR)/usr/bin/xzless
	@rm -rf $(FAKEROOT_DIR)/usr/bin/xzmore
endif
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

