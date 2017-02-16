##
## prebuild/third-party/armv7-a-hf/lib/make.inc
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

ifeq ($(CONFIG_AMBARELLA_LIBJPEG_TURBO_SUPPORT), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_libjpeg_turbo
LOCAL_SRCS	:= usr

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
ifneq ($(CONFIG_AMBARELLA_LIBJPEG_TURBO_UTIL_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/bin/cjpeg
	@rm -rf $(FAKEROOT_DIR)/usr/bin/djpeg
	@rm -rf $(FAKEROOT_DIR)/usr/bin/jpegtran
	@rm -rf $(FAKEROOT_DIR)/usr/bin/rdjpgcom
	@rm -rf $(FAKEROOT_DIR)/usr/bin/wrjpgcom
endif
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

endif
