##
## prebuild/third-party/armv7-a-hf/systemd/make.inc
##
## History:
##    2014/02/20 - [Yupeng Chang] Created file
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

ifeq ($(CONFIG_AMBARELLA_INIT_SYSTEMD), y)
LOCAL_TARGET := prebuild_systemd
LOCAL_SRCS   := etc sbin usr var

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): basic_fs
$(LOCAL_TARGET): SYSTEMD_PATH := $(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
	$(call overwrite-bb)
	@mkdir -p $(FAKEROOT_dir)/lib
	@rm -f $(FAKEROOT_DIR)/etc/init.d/S*
	@rm -f $(FAKEROOT_DIR)/etc/init.d/rcS
	@rm -f $(FAKEROOT_DIR)/etc/ambrwfs
	@ln -sf ../usr/lib/udev $(FAKEROOT_DIR)/lib/udev
	@ln -sf /usr/lib/systemd/systemd $(FAKEROOT_DIR)/linuxrc
	@cp -af $(SYSTEMD_PATH)/etc/inittab $(FAKEROOT_DIR)/etc/inittab
ifneq ($(CONFIG_AMBARELLA_SYSTEMD_JOURNAL_GATEWAY), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/systemd/systemd-journal-gatewayd
	@rm -rf $(FAKEROOT_DIR)/usr/lib/systemd/systemd-journal-remote
	@rm -rf $(FAKEROOT_DIR)/usr/lib/systemd/systemd/systemd-journal-gatewayd.*
	@rm -rf $(FAKEROOT_DIR)/usr/share/systemd/gatewayd
else
	@ln -sf ../systemd-journal-gatewayd.socket $(FAKEROOT_DIR)/usr/lib/systemd/system/sockets.target.wants/systemd-journal-gatewayd.socket
endif
ifneq ($(CONFIG_AMBARELLA_SYSTEMD_NETWORKD), y)
	@rm -rf $(FAKEROOT_DIR)/etc/systemd/system/multi-user.target.wants/systemd-networkd.service
	@rm -rf $(FAKEROOT_DIR)/etc/systemd/system/network-online.target.wants/systemd-networkd-wait-online.service
	@rm -rf $(FAKEROOT_DIR)/usr/lib/systemd/systemd-networkd
	@rm -rf $(FAKEROOT_DIR)/usr/lib/systemd/systemd-networkd-wait-online
	@rm -rf $(FAKEROOT_DIR)/usr/lib/systemd/system/systemd-networkd.service
	@rm -rf $(FAKEROOT_DIR)/usr/lib/systemd/system/systemd-networkd-wait-online.service
endif
ifneq ($(CONFIG_AMBARELLA_SYSTEMD_RESOLVD), y)
	@rm -rf $(FAKEROOT_DIR)/etc/systemd/system/multi-user.target.wants/systemd-resolved.service
	@rm -rf $(FAKEROOT_DIR)/etc/systemd/resolved.conf
	@rm -rf $(FAKEROOT_DIR)/usr/lib/systemd/system/systemd-resolved.service
	@rm -rf $(FAKEROOT_DIR)/usr/lib/systemd/systemd-resolved
	@sed -i -e 's|/run/systemd/resolve/resolv.conf|/tmp/resolv.conf|g' -e '/^.*resolv.conf/if /tmp/resolv.conf 0644 root root - -' $(FAKEROOT_DIR)/usr/lib/tmpfiles.d/etc.conf
endif
ifneq ($(CONFIG_AMBARELLA_SYSTEMD_TIMESYNCD), y)
	@rm -rf $(FAKEROOT_DIR)/etc/systemd/timesyncd.conf
	@rm -rf $(FAKEROOT_DIR)/etc/systemd/system/multi-user.target.wants/systemd-timesyncd.service
	@rm -rf $(FAKEROOT_DIR)/usr/lib/systemd/systemd-timesyncd
	@rm -rf $(FAKEROOT_DIR)/usr/lib/systemd/system/time-sync.target
	@rm -rf $(FAKEROOT_DIR)/usr/lib/systemd/system/systemd-timesyncd.service
endif
	@echo "Copy $@ Done"
endif

ifeq ($(CONFIG_AMBARELLA_INIT_LINUXRC), y)
LOCAL_TARGET := prebuild_udev
LOCAL_SRCS   := lib
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): basic_fs
$(LOCAL_TARGET): UDEV_PATH := $(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
	@mkdir -p $(FAKEROOT_DIR)/etc/init.d
	@mkdir -p $(FAKEROOT_DIR)/etc/udev/hwdb.d
	@mkdir -p $(FAKEROOT_DIR)/usr/bin
	@mkdir -p $(FAKEROOT_DIR)/usr/lib/udev
	@mkdir -p $(FAKEROOT_DIR)/lib
	@ln -sf ../usr/lib/udev $(FAKEROOT_DIR)/lib/udev
	@cp -r $(UDEV_PATH)/etc/udev/rules.d $(FAKEROOT_DIR)/etc/udev/
	@cp -r $(UDEV_PATH)/etc/udev/udev.conf $(FAKEROOT_DIR)/etc/udev/
	@cp -r $(UDEV_PATH)/etc/init.d/S12udev $(FAKEROOT_DIR)/etc/init.d/
	@cp -r $(UDEV_PATH)/usr/bin/udevadm $(FAKEROOT_DIR)/usr/bin/
	@cp -r $(UDEV_PATH)/usr/lib/libgudev* $(FAKEROOT_DIR)/usr/lib/
	@cp -r $(UDEV_PATH)/usr/lib/libudev*  $(FAKEROOT_DIR)/usr/lib/
	@cp -r $(UDEV_PATH)/usr/lib/libsystemd* $(FAKEROOT_DIR)/usr/lib/
	@cp -r $(UDEV_PATH)/usr/lib/udev      $(FAKEROOT_DIR)/usr/lib/
	@cp -r $(UDEV_PATH)/usr/lib/systemd/systemd-udevd $(FAKEROOT_DIR)/usr/lib/udev/udevd
	@echo "Copy $@ Done"
endif

$(call add-target-into-build, $(LOCAL_TARGET))

