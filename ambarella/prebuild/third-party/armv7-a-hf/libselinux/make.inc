##
## prebuild/third-party/libselinux/make.inc
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

ifeq ($(CONFIG_AMBARELLA_LIBSELINUX_SUPPORT), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_libselinux
LOCAL_SRCS	:= lib usr

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
ifneq ($(CONFIG_AMBARELLA_LIBSELINUX_UTILS_SUPPORT), y)
	@rm -f $(FAKEROOT_DIR)/usr/sbin/avcstat
	@rm -f $(FAKEROOT_DIR)/usr/sbin/compute_create
	@rm -f $(FAKEROOT_DIR)/usr/sbin/compute_relabel
	@rm -f $(FAKEROOT_DIR)/usr/sbin/getconlist
	@rm -f $(FAKEROOT_DIR)/usr/sbin/getenforce
	@rm -f $(FAKEROOT_DIR)/usr/sbin/getpidcon
	@rm -f $(FAKEROOT_DIR)/usr/sbin/getseuser
	@rm -f $(FAKEROOT_DIR)/usr/sbin/policyvers
	@rm -f $(FAKEROOT_DIR)/usr/sbin/selinux_check_securetty_context
	@rm -f $(FAKEROOT_DIR)/usr/sbin/selinuxexeccon
	@rm -f $(FAKEROOT_DIR)/usr/sbin/setfilecon
	@rm -f $(FAKEROOT_DIR)/usr/sbin/compute_av
	@rm -f $(FAKEROOT_DIR)/usr/sbin/compute_member
	@rm -f $(FAKEROOT_DIR)/usr/sbin/compute_user
	@rm -f $(FAKEROOT_DIR)/usr/sbin/getdefaultcon
	@rm -f $(FAKEROOT_DIR)/usr/sbin/getfilecon
	@rm -f $(FAKEROOT_DIR)/usr/sbin/getsebool
	@rm -f $(FAKEROOT_DIR)/usr/sbin/matchpathcon
	@rm -f $(FAKEROOT_DIR)/usr/sbin/sefcontext_compile
	@rm -f $(FAKEROOT_DIR)/usr/sbin/selinuxenabled
	@rm -f $(FAKEROOT_DIR)/usr/sbin/setenforce
	@rm -f $(FAKEROOT_DIR)/usr/sbin/togglesebool
endif
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

endif

