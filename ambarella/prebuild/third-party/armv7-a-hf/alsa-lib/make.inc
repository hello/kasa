##
## prebuild/third-party/armv7-a-hf/alsa/make.inc
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

ifeq ($(CONFIG_AMBARELLA_ALSA_SUPPORT), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_alsa
LOCAL_SRCS	:= etc usr

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/*.a
	@mkdir -p $(FAKEROOT_DIR)/usr/share/alsa/ucm
	@cp $(PRIVATE_PATH)/etc/asound.conf $(FAKEROOT_DIR)/etc/asound.conf
	@if [ -f $(BOARD_INITD_DIR)/asoundrc ]; then \
		cat $(BOARD_INITD_DIR)/asoundrc >> $(FAKEROOT_DIR)/etc/asound.conf; \
	fi
	@[ -e $(FAKEROOT_DIR)/usr/share/alsa ] && find $(FAKEROOT_DIR)/usr/share/alsa -name "*.conf" | xargs chmod 644
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

endif

