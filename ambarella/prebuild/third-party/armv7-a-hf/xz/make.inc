##
## prebuild/third-party/orc/make.inc
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

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_xz
LOCAL_SRCS	:= usr

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
ifneq ($(CONFIG_AMBARELLA_XZ_UTILS_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/bin/lzcat
	@rm -rf $(FAKEROOT_DIR)/usr/bin/lzcmp
	@rm -rf $(FAKEROOT_DIR)/usr/bin/lzdiff
	@rm -rf $(FAKEROOT_DIR)/usr/bin/lzegrep
	@rm -rf $(FAKEROOT_DIR)/usr/bin/lzfgrep
	@rm -rf $(FAKEROOT_DIR)/usr/bin/lzgrep
	@rm -rf $(FAKEROOT_DIR)/usr/bin/lzless
	@rm -rf $(FAKEROOT_DIR)/usr/bin/lzma
	@rm -rf $(FAKEROOT_DIR)/usr/bin/lzmadec
	@rm -rf $(FAKEROOT_DIR)/usr/bin/lzmainfo
	@rm -rf $(FAKEROOT_DIR)/usr/bin/lzmore
	@rm -rf $(FAKEROOT_DIR)/usr/bin/unlzma 
	@rm -rf $(FAKEROOT_DIR)/usr/bin/unxz 
	@rm -rf $(FAKEROOT_DIR)/usr/bin/xz 
	@rm -rf $(FAKEROOT_DIR)/usr/bin/xzcat
	@rm -rf $(FAKEROOT_DIR)/usr/bin/xzcmp
	@rm -rf $(FAKEROOT_DIR)/usr/bin/xzdec
	@rm -rf $(FAKEROOT_DIR)/usr/bin/xzdiff
	@rm -rf $(FAKEROOT_DIR)/usr/bin/xzegrep
	@rm -rf $(FAKEROOT_DIR)/usr/bin/xzfgrep
	@rm -rf $(FAKEROOT_DIR)/usr/bin/xzgrep
	@rm -rf $(FAKEROOT_DIR)/usr/bin/xzless
	@rm -rf $(FAKEROOT_DIR)/usr/bin/xzmore
endif
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

