##
## prebuild/third-party/armv7-a-hf/libnl/make.inc
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

ifeq ($(CONFIG_AMBARELLA_LIBNL_SUPPORT), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_libnl
LOCAL_SRCS	:= etc usr

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
ifneq ($(CONFIG_AMBARELLA_LIBNL_UTILS_SUPPORT), y)
	@rm -f $(FAKEROOT_DIR)/usr/sbin/enl-ctrl-list
	@rm -f $(FAKEROOT_DIR)/usr/sbin/nl-class-add
	@rm -f $(FAKEROOT_DIR)/usr/sbin/nl-class-delete
	@rm -f $(FAKEROOT_DIR)/usr/sbin/nl-classid-lookup
	@rm -f $(FAKEROOT_DIR)/usr/sbin/nl-class-list
	@rm -f $(FAKEROOT_DIR)/usr/sbin/nl-cls-add
	@rm -f $(FAKEROOT_DIR)/usr/sbin/nl-cls-delete
	@rm -f $(FAKEROOT_DIR)/usr/sbin/nl-cls-list
	@rm -f $(FAKEROOT_DIR)/usr/sbin/nl-link-list
	@rm -f $(FAKEROOT_DIR)/usr/sbin/nl-pktloc-lookup
	@rm -f $(FAKEROOT_DIR)/usr/sbin/nl-qdisc-add
	@rm -f $(FAKEROOT_DIR)/usr/sbin/nl-qdisc-delete
	@rm -f $(FAKEROOT_DIR)/usr/sbin/nl-qdisc-list
endif
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

endif

