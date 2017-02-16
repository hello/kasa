##
## prebuild/third-party/armv7-a-hf/bluetooth/make.inc
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

ifeq ($(CONFIG_AMBARELLA_BLUETOOTH5_TOOL_SUPPORT), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET := prebuild_bluetooth
LOCAL_SRCS   := etc usr

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
ifneq ($(CONFIG_AMBARELLA_BLUETOOTH_USB_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/udev/hid2hci
	@rm -rf $(FAKEROOT_DIR)/usr/lib/udev/rules.d/97-hid2hci.rules
endif
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

endif

