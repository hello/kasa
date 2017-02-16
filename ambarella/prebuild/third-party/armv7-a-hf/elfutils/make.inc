##
## prebuild/third-party/elfutils/make.inc
##
## History:
##    2012/05/30 - [Yupeng Chang] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(CONFIG_AMBARELLA_LIBELF_SUPPORT), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_libelf
LOCAL_SRCS	:= usr

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/*.a
ifneq ($(CONFIG_AMBARELLA_ELFUTILS_SUPPORT), y)
	@rm -f $(FAKEROOT_DIR)/bin/addr2line
	@rm -f $(FAKEROOT_DIR)/bin/ar
	@rm -f $(FAKEROOT_DIR)/bin/elfcmp
	@rm -f $(FAKEROOT_DIR)/bin/elflint
	@rm -f $(FAKEROOT_DIR)/bin/findtextrel
	@rm -f $(FAKEROOT_DIR)/bin/ld
	@rm -f $(FAKEROOT_DIR)/bin/make-debug-archive
	@rm -f $(FAKEROOT_DIR)/bin/nm
	@rm -f $(FAKEROOT_DIR)/bin/objdump
	@rm -f $(FAKEROOT_DIR)/bin/ranlib
	@rm -f $(FAKEROOT_DIR)/bin/readelf
	@rm -f $(FAKEROOT_DIR)/bin/size
	@rm -f $(FAKEROOT_DIR)/bin/strings
	@rm -f $(FAKEROOT_DIR)/bin/strip
	@rm -f $(FAKEROOT_DIR)/bin/unstrip
endif
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

endif

