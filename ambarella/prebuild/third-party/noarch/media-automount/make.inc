##
## /S2l/prebuild/third-party/noarch/media-automount/make.inc
##
## History:
##   2015年8月26日 - [ypchang] created file
##
## Copyright (C) 2008-2015, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

LOCAL_PATH := $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET := prebuild_media_automount
LOCAL_SRCS := etc usr

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

ifeq ($(CONFIG_AMBARELLA_INIT_SYSTEMD), y)
$(LOCAL_TARGET): prebuild_systemd
endif
ifeq ($(CONFIG_AMBARELLA_INIT_LINUXRC), y)
$(LOCAL_TARGET): prebuild_udev
endif
$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
ifeq ($(CONFIG_AMBARELLA_INIT_SYSTEMD), y)
	@rm -rf $(FAKEROOT_DIR)/etc/udev/rules.d/11-sd-cards-auto-mount-linuxrc.rules
	@rm -rf $(FAKEROOT_DIR)/etc/udev/rules.d/11-usb-storage-auto-mount-linuxrc.rules
endif
ifeq ($(CONFIG_AMBARELLA_INIT_LINUXRC), y)
	@rm -rf $(FAKEROOT_DIR)/etc/udev/rules.d/11-sd-cards-auto-mount-systemd.rules
	@rm -rf $(FAKEROOT_DIR)/etc/udev/rules.d/11-usb-storage-auto-mount-systemd.rules
endif
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))
