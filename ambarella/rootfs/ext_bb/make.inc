##
## rootfs/extbb/make.inc
##
## History:
##    2012/06/03 - [Cao Rongrong] Created file
##
## Copyright (c) 2016 Ambarella, Inc.
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

LOCAL_PATH		:= $(call my-dir)

.PHONY: menuconfig_extbb build_extbb clean_extbb

BUSYBOX_SRC_DIR		:= $(shell $(if $(wildcard $(EXT_BUSYBOX_DIR)),cd $(EXT_BUSYBOX_DIR) && pwd))
BUSYBOX_OUT_DIR		:= $(patsubst $(ALL_TOPDIR)/%, $(AMB_BOARD_OUT)/%,$(BUSYBOX_SRC_DIR))
BUSYBOX_EXTRA_CFLAGS  := "$(AMBARELLA_APP_CFLAGS) -Wno-error=format-security"
BUSYBOX_EXTRA_LDFLAGS := "$(AMBARELLA_APP_CFLAGS)"

ifeq ($(CONFIG_AMBARELLA_BUSYBOX_BUILD_STATIC),y)
BB_CONFIG := bbconfig_static
else
BB_CONFIG := bbconfig
endif

define build_ext_busybox
$(MAKE) ARCH=$(CPU_ARCH) EXTRA_CFLAGS=$(BUSYBOX_EXTRA_CFLAGS) \
	EXTRA_LDFLAGS=$(BUSYBOX_EXTRA_LDFLAGS) \
	CONFIG_PREFIX=$(FAKEROOT_DIR) \
	KBUILD_SRC=$(BUSYBOX_SRC_DIR) -f $(BUSYBOX_SRC_DIR)/Makefile $(1)
endef

menuconfig_extbb: PRIVATE_PATH:=$(LOCAL_PATH)
menuconfig_extbb:
	@mkdir -p $(BUSYBOX_OUT_DIR)
	@if [ -f $(BUSYBOX_OUT_DIR)/.config ]; then \
		rm -rf $(BUSYBOX_OUT_DIR)/.config; \
	fi
	@cp -dpRf $(PRIVATE_PATH)/$(BB_CONFIG) $(BUSYBOX_OUT_DIR)/.config
	@cd $(BUSYBOX_OUT_DIR) && $(call build_ext_busybox, menuconfig)

build_extbb: PRIVATE_PATH:=$(LOCAL_PATH)
build_extbb:
	@echo "  Build BusyBox ..."
	@mkdir -p $(FAKEROOT_DIR)
	@mkdir -p $(BUSYBOX_OUT_DIR)
	@tar -xjf $(PRIVATE_PATH)/skeleton.tar.bz2 -C $(FAKEROOT_DIR)/..
	@if [ ! -f $(BUSYBOX_OUT_DIR)/.config ]; then \
		cp -dpRf $(PRIVATE_PATH)/$(BB_CONFIG) $(BUSYBOX_OUT_DIR)/.config; \
	fi
	@cd $(BUSYBOX_OUT_DIR) && $(call build_ext_busybox)
	@cd $(BUSYBOX_OUT_DIR) && $(call build_ext_busybox, install)

clean_extbb:
	@echo "  Clean BusyBox ..."
	@cd $(BUSYBOX_OUT_DIR) && $(call build_ext_busybox, uninstall)
	@cd $(BUSYBOX_OUT_DIR) && $(call build_ext_busybox, distclean)
	@rm -rf $(BUSYBOX_OUT_DIR)
