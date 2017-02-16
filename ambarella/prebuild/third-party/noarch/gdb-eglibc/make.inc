##
## prebuild/third-party/gdb-eglibc/make.inc
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

ifeq ($(CONFIG_AMBARELLA_GDB_SUPPORT), y)

.PHONY: prebuild_gdb

__gdb_GLIBC_VERSION__ := $(shell find $(SYS_LIB_DIR) -name libc-*.so | \
				xargs -Iname basename name .so | awk -F- '{print $$2}' | \
				sed 's/\([0-9*]\)\.\([0-9]*\)\.*[0-9]*/\1\.\2/g')

__gdb_VERSION := $(if $(__gdb_GLIBC_VERSION__), $(__gdb_GLIBC_VERSION__), 2.10)
GDB_DIR := $(strip $(if $(CONFIG_CPU_ARM1136JS), $(GDB_DIR_ARMV6), $(if $(CONFIG_CPU_CORTEXA9), $(GDB_DIR_ARMV7_SF), $(if $(CONFIG_CPU_CORTEXA9_HF), $(GDB_DIR_ARMV7), ))))

prebuild_gdb: LOCAL_PATH:=$(call my-dir)
prebuild_gdb:
ifneq ($(GDB_DIR),)
	@mkdir -p $(FAKEROOT_DIR)/usr/bin
	@cp -dpRf $(GDB_DIR)/gdb $(FAKEROOT_DIR)/usr/bin
	@echo add-auto-load-safe-path / > $(FAKEROOT_DIR)/root/.gdbinit
else
	@mkdir -p $(FAKEROOT_DIR)/
	@cp -dpRf $(LOCAL_PATH)/$(strip $(__gdb_VERSION))/* $(FAKEROOT_DIR)/
endif
	@echo "Copy $@ Done"

$(call add-target-into-build, prebuild_gdb)

endif
