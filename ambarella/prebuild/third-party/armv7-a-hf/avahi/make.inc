##
## prebuild/third-party/armv7-a-hf/avahi/make.inc
##
## History:
##    2012/05/30 - [Cao Rongrong] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
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

