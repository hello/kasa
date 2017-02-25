##
## prebuild/third-party/armv7-a-hf/avahi/make.inc
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

ifeq ($(CONFIG_AMBARELLA_AVAHI_SUPPORT), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_avahi
LOCAL_SRCS	:= etc usr

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
	@mkdir -p $(FAKEROOT_DIR)/etc/avahi/services
	@mkdir -p $(FAKEROOT_DIR)/etc/avahi/etc
ifneq ($(CONFIG_AMBARELLA_AVAHI_TOOLS_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/bin/avahi-browse
	@rm -rf $(FAKEROOT_DIR)/usr/bin/avahi-browse-domains
	@rm -rf $(FAKEROOT_DIR)/usr/bin/avahi-publish
	@rm -rf $(FAKEROOT_DIR)/usr/bin/avahi-publish-address
	@rm -rf $(FAKEROOT_DIR)/usr/bin/avahi-publish-service
	@rm -rf $(FAKEROOT_DIR)/usr/bin/avahi-resolve
	@rm -rf $(FAKEROOT_DIR)/usr/bin/avahi-resolve-address
	@rm -rf $(FAKEROOT_DIR)/usr/bin/avahi-resolve-host-name
	@rm -rf $(FAKEROOT_DIR)/usr/bin/avahi-set-host-name
endif
ifneq ($(CONFIG_AMBARELLA_AVAHI_AUTOIPD_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/etc/avahi/avahi-autoipd.action
	@rm -rf $(FAKEROOT_DIR)/usr/sbin/avahi-autoipd
endif
ifneq ($(CONFIG_AMBARELLA_AVAHI_DNSCONFD_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/etc/avahi/avahi-dnsconfd.action
	@rm -rf $(FAKEROOT_DIR)/usr/lib/systemd/system/avahi-dnsconfd.service
	@rm -rf $(FAKEROOT_DIR)/usr/sbin/avahi-dnsconfd
endif
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

endif

