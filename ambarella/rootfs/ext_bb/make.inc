##
## rootfs/extbb/make.inc
##
## History:
##    2012/06/03 - [Cao Rongrong] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
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
$(MAKE) ARCH=arm EXTRA_CFLAGS=$(BUSYBOX_EXTRA_CFLAGS) \
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
