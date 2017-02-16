##
## prebuild/third-party/armv7-a-hf/util-linux/make.inc
##
## History:
##    2012/06/03 - [Cao Rongrong] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(CONFIG_UTIL_LINUX_SUPPORT), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET     := prebuild_util-linux
LOCAL_SRCS       := bin sbin usr

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): basic_fs
	$(call prebuild-links)
	$(call overwrite-bb)
ifeq ($(CONFIG_AMBARELLA_INIT_LINUXRC), y)
	@rm -f $(FAKEROOT_DIR)/bin/login
	@ln -sf /bin/busybox $(FAKEROOT_DIR)/bin/login
endif
ifeq ($(CONFIG_AMBARELLA_INIT_LINUXRC_FASTBOOT), y)
	@rm -f $(FAKEROOT_DIR)/bin/login
	@ln -sf /bin/busybox $(FAKEROOT_DIR)/bin/login
endif
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

endif