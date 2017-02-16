##
## prebuild/third-party/armv7-a-hf/networkmanager/make.inc
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

ifeq ($(CONFIG_AMBARELLA_NETWORKMANAGER_SUPPORT), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_networkmanager
LOCAL_SRCS	:= etc usr

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
ifneq ($(CONFIG_AMBARELLA_BLUETOOTH5_TOOL_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/NetworkManager/libnm-device-plugin-bluetooth.so
endif
	@mkdir -p $(FAKEROOT_DIR)/etc/NetworkManager/dispatcher.d
	@mkdir -p $(FAKEROOT_DIR)/etc/NetworkManager/system-connections
	@mkdir -p $(FAKEROOT_DIR)/etc/NetworkManager/dnsmasq.d
	@mkdir -p $(FAKEROOT_DIR)/var/lib/NetworkManager
	@chmod 755 $(FAKEROOT_DIR)/etc/NetworkManager/dispatcher.d/*
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

endif

