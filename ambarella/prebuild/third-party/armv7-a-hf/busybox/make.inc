##
## rootfs/busybox/make.inc
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

ifneq ($(CONFIG_AMBARELLA_BUSYBOX_BUILD), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_bb
LOCAL_SRCS	:= bin debug dev etc home lib linuxrc \
			mnt opt proc root run sbin sys tmp usr var

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
	@mkdir -p $(FAKEROOT_DIR)/etc/ld.so.conf.d
	@mkdir -p $(FAKEROOT_DIR)/etc/network/if-up.d
	@mkdir -p $(FAKEROOT_DIR)/etc/network/if-down.d
	@mkdir -p $(FAKEROOT_DIR)/etc/network/if-pre-up.d
	@mkdir -p $(FAKEROOT_DIR)/etc/network/if-post-down.d
	@mkdir -p $(FAKEROOT_DIR)/debug
	@mkdir -p $(FAKEROOT_DIR)/dev
	@mkdir -p $(FAKEROOT_DIR)/home/default
	@mkdir -p $(FAKEROOT_DIR)/lib
	@mkdir -p $(FAKEROOT_DIR)/mnt
	@mkdir -p $(FAKEROOT_DIR)/opt
	@mkdir -p $(FAKEROOT_DIR)/proc
	@mkdir -p $(FAKEROOT_DIR)/root
	@mkdir -p $(FAKEROOT_DIR)/run
	@mkdir -p $(FAKEROOT_DIR)/sys
	@mkdir -p $(FAKEROOT_DIR)/tmp
	@mkdir -p $(FAKEROOT_DIR)/usr/lib

$(call add-target-into-build, $(LOCAL_TARGET))

endif
