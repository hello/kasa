##
## prebuild/third-party/armv7-a-hf/alsa-utils/make.inc
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

ifeq ($(CONFIG_AMBARELLA_ALSA_UTILS_SUPPORT), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_alsa-utils
LOCAL_SRCS	:= usr etc var

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
ifneq ($(CONFIG_AMBARELLA_ALSA_SPEEKER_TEST_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/bin/speaker-test
	@rm -rf $(FAKEROOT_DIR)/usr/share/alsa/speaker-test
	@rm -rf $(FAKEROOT_DIR)/usr/share/sounds/alsa
endif
	@[ -e $(FAKEROOT_DIR)/usr/share/alsa/init ] && chmod 644 $(FAKEROOT_DIR)/usr/share/alsa/init/*
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

endif

