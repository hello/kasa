##
## prebuild/third-party/gnutls/make.inc
##
## History:
##    2012/10/17 - [Yupeng Chang] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(CONFIG_AMBARELLA_DHCP_SUPPORT), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_dhcp
LOCAL_SRCS	:= usr var

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
ifneq ($(CONFIG_AMBARELLA_DHCP_TOOL_OMSHELL), y)
	@rm -rf $(FAKEROOT_DIR)/usr/bin/omshell
endif
ifneq ($(CONFIG_AMBARELLA_DHCP_TOOL_DHCPD), y)
	@rm -rf $(FAKEROOT_DIR)/usr/sbin/dhcpd
endif
ifneq ($(CONFIG_AMBARELLA_DHCP_TOOL_DHCLIENT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/sbin/dhclient
endif
ifneq ($(CONFIG_AMBARELLA_DHCP_TOOL_DHCRELAY), y)
	@rm -rf $(FAKEROOT_DIR)/usr/sbin/dhcrelay
endif
	@mkdir -p $(FAKEROOT_DIR)/var/lib/dhclient
	@mkdir -p $(FAKEROOT_DIR)/var/lib/dhcp
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

endif

