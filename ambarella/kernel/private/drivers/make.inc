##
## kernel/private/drivers/ambarella/make.inc
##
## History:
##    2012/06/04 - [Cao Rongrong] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_PRIVATE_MODULES), y)

AMB_DRIVER_DIR		:= $(call my-dir)
OUT_DRIVER_DIR		:= $(patsubst $(AMB_TOPDIR)/%,$(AMB_BOARD_OUT)/%,$(AMB_DRIVER_DIR))
ECHO              := $(shell which echo) -e

# AMB_DRIVER_MODULES will be filled in each subdir make.inc
AMB_DRIVER_MODULES	:=

include $(call all-subdir-makefiles)

###

.PHONY: prepare_private_drivers

prepare_private_drivers: linux
	@mkdir -p $(OUT_DRIVER_DIR)
	@cat $(AMB_DRIVER_DIR)/Kbuild | sed -e "s/@MODULES@/$(subst /,\\/,$(AMB_DRIVER_MODULES))"/ \
		> $(OUT_DRIVER_DIR)/Kbuild

$(call add-target-into-build, prepare_private_drivers)

###

.PHONY: build_private_drivers

build_private_drivers: AMB_DRIVER_MODULES:=$(AMB_DRIVER_MODULES)
build_private_drivers: prepare_private_drivers
	@echo "Build Private Drivers:"
ifneq ($(AMB_DRIVER_MODULES),)
	@$(ECHO) "  $(patsubst %/,%\n ,$(AMB_DRIVER_MODULES))"
	$(AMBA_MAKEFILE_V)$(MAKE) $(AMBA_MAKE_PARA) -C $(LINUX_SRC_DIR) O=$(LINUX_OUT_DIR) \
		M=$(OUT_DRIVER_DIR) modules
	$(AMBA_MAKEFILE_V)$(MAKE) $(AMBA_MAKE_PARA) -C $(LINUX_SRC_DIR) O=$(LINUX_OUT_DIR) \
		M=$(OUT_DRIVER_DIR) $(LINUX_INSTALL_FLAG) INSTALL_MOD_DIR=extra modules_install;
	@kernel_version=`cat $(LINUX_OUT_DIR)/include/config/kernel.release`; \
	extra_path=$(KERNEL_INSTALL_PATH)/lib/modules/$$kernel_version/extra; \
		find $$extra_path/* -mindepth 1 -type f -exec mv {} $$extra_path/ \; && \
		find $$extra_path/* -maxdepth 0 -type d -exec rm -rf {} \; && \
		depmod -ae -F $(LINUX_OUT_DIR)/System.map -b $(KERNEL_INSTALL_PATH) $$kernel_version
else
	@echo "  Nothing"
endif
	@echo "Build Private Drivers Done"

# build_external_drivers is defined in ambarella/kernel/external/make.inc,
# in order to avoid race, here we make build_private_drivers depend on
# build_external_drivers.
ifeq ($(BUILD_WIFI_BLUETOOTH_MODULES), y)
build_private_drivers:build_external_drivers
endif

$(call add-target-into-build, build_private_drivers)

###
endif

