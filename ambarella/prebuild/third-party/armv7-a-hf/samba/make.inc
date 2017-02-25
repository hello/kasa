##
## prebuild/third-party/samba/make.inc
##
## History:
##    2012/05/30 - [Cao Rongrong] Created file
##
## Copyright (c) 2015 Ambarella, Inc.
##
## This file and its contents ("Software") are protected by intellectual
## property rights including, without limitation, U.S. and/or foreign
## copyrights. This Software is also the confidential and proprietary
## information of Ambarella, Inc. and its licensors. You may not use, reproduce,
## disclose, distribute, modify, or otherwise prepare derivative works of this
## Software or any portion thereof except pursuant to a signed license agreement
## or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
## In the absence of such an agreement, you agree to promptly notify and return
## this Software to Ambarella, Inc.
##
## THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
## INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
## MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
## IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
## INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
## (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
## LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
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

