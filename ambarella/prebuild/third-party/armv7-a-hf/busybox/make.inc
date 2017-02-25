##
## rootfs/busybox/make.inc
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

ifneq ($(CONFIG_AMBARELLA_BUSYBOX_BUILD), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_bb
LOCAL_SRCS	:= bin debug dev etc home lib linuxrc \
			mnt opt proc root run sbin sys tmp usr var

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
	@mkdir -p $(FAKEROOT_DIR)/etc/ld.so.conf.d
	@mkdir -p $(FAKEROOT_DIR)/etc/network/if-up.d
	@mkdir -p $(FAKEROOT_DIR)/etc/network/if-down.d
	@mkdir -p $(FAKEROOT_DIR)/etc/network/if-pre-up.d
	@mkdir -p $(FAKEROOT_DIR)/etc/network/if-post-down.d
	@mkdir -p $(FAKEROOT_DIR)/debug
	@mkdir -p $(FAKEROOT_DIR)/dev
	@mkdir -p $(FAKEROOT_DIR)/home/default
	@mkdir -p $(FAKEROOT_DIR)/lib
	@mkdir -p $(FAKEROOT_DIR)/mnt
	@mkdir -p $(FAKEROOT_DIR)/opt
	@mkdir -p $(FAKEROOT_DIR)/proc
	@mkdir -p $(FAKEROOT_DIR)/root
	@mkdir -p $(FAKEROOT_DIR)/run
	@mkdir -p $(FAKEROOT_DIR)/sys
	@mkdir -p $(FAKEROOT_DIR)/tmp
	@mkdir -p $(FAKEROOT_DIR)/usr/lib

$(call add-target-into-build, $(LOCAL_TARGET))

endif
