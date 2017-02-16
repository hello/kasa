##
## prebuild/third-party/armv7-a-hf/openssh/make.inc
##
## History:
##    2014/02/27 - [Yupeng Chang] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(CONFIG_AMBARELLA_SSH_SUPPORT), y)

LOCAL_PATH := $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET := prebuild_openssh
LOCAL_SRCS   := etc usr var

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
ifneq ($(CONFIG_AMBARELLA_SSH_CLIENT_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/bin/ssh
	@rm -rf $(FAKEROOT_DIR)/usr/bin/scp
	@rm -rf $(FAKEROOT_DIR)/usr/bin/slogin
	@rm -rf $(FAKEROOT_DIR)/usr/bin/ssh-agent
	@rm -rf $(FAKEROOT_DIR)/usr/bin/ssh-add
	@rm -rf $(FAKEROOT_DIR)/usr/bin/ssh-keyscan
	@rm -rf $(FAKEROOT_DIR)/usr/bin/sftp
	@rm -rf $(FAKEROOT_DIR)/usr/bin/ssh-copy-id
	@rm -rf $(FAKEROOT_DIR)/usr/libexec/openssh/ssh-pkcs11-helper
	@rm -rf $(FAKEROOT_DIR)/etc/ssh/ssh_config
endif
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

endif
