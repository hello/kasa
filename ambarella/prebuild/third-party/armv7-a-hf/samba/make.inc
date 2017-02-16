##
## prebuild/third-party/samba/make.inc
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

ifeq ($(CONFIG_AMBARELLA_SAMBA_SERVICE_SUPPORT), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_samba
LOCAL_SRCS	:= etc sbin usr

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "$@ is currently not available for hard-float."
#	$(call prebuild-links)
#ifeq ($(CONFIG_AMBARELLA_BASIC_SAMBA_SERVICE), y)
#	@rm -rf $(FAKEROOT_DIR)/usr/sbin/winbindd
#	@rm -rf $(FAKEROOT_DIR)/usr/sbin/swat
#	@rm -rf $(FAKEROOT_DIR)/usr/bin/wbinfo
#	@rm -rf $(FAKEROOT_DIR)/usr/bin/testparm
#	@rm -rf $(FAKEROOT_DIR)/usr/bin/smbget
#	@rm -rf $(FAKEROOT_DIR)/usr/bin/ntlm_auth
#	@rm -rf $(FAKEROOT_DIR)/usr/bin/smbpasswd
#	@rm -rf $(FAKEROOT_DIR)/usr/bin/tdbtool
#	@rm -rf $(FAKEROOT_DIR)/usr/bin/eventlogadm
#	@rm -rf $(FAKEROOT_DIR)/usr/bin/smbspool
#	@rm -rf $(FAKEROOT_DIR)/usr/bin/pdbedit
#	@rm -rf $(FAKEROOT_DIR)/usr/bin/smbtree
#	@rm -rf $(FAKEROOT_DIR)/usr/bin/smbclient
#	@rm -rf $(FAKEROOT_DIR)/usr/bin/tdbbackup
#	@rm -rf $(FAKEROOT_DIR)/usr/bin/nmblookup
#	@rm -rf $(FAKEROOT_DIR)/usr/bin/net
#	@rm -rf $(FAKEROOT_DIR)/usr/bin/smbcontrol
#	@rm -rf $(FAKEROOT_DIR)/usr/bin/smbstatus
#	@rm -rf $(FAKEROOT_DIR)/usr/bin/tdbdump
#	@rm -rf $(FAKEROOT_DIR)/usr/bin/smbcquotas
#	@rm -rf $(FAKEROOT_DIR)/usr/bin/smbtar
#	@rm -rf $(FAKEROOT_DIR)/usr/bin/smbcacls
#	@rm -rf $(FAKEROOT_DIR)/usr/bin/rpcclient
#	@rm -rf $(FAKEROOT_DIR)/usr/bin/profiles
#ifeq ($(CONFIG_AMBARELLA_SAMBA_CLIENT_ONLY), y)
#	@rm -rf $(FAKEROOT_DIR)/usr/sbin/nmbd
#	@rm -rf $(FAKEROOT_DIR)/usr/sbin/smbd
#	@rm -rf $(FAKEROOT_DIR)/sbin/portmap
#	@rm -rf $(FAKEROOT_DIR)/etc/init.d/S91smb
#	@rm -rf $(FAKEROOT_DIR)/etc/init.d/S13portmap
#	@rm -rf $(FAKEROOT_DIR)/etc/samba/smb.conf
#endif
#endif
#	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

endif

