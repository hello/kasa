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

ifeq ($(BUILD_AMBARELLA_EXTERNAL_MODULES), y)

EXTERNAL_DRIVER_DIR		:= $(call my-dir)
OUT_EXTERNAL_DRIVER_DIR		:= $(patsubst $(AMB_TOPDIR)/%,$(AMB_BOARD_OUT)/%,$(EXTERNAL_DRIVER_DIR))
ECHO := $(shell which echo) -e

# EXT_DRIVER_MODULES will be filled in each subdir make.inc
EXT_DRIVER_MODULES		:=

include $(call all-subdir-makefiles)

###

.PHONY: prepare_external_drivers

prepare_external_drivers: linux
	@mkdir -p $(OUT_EXTERNAL_DRIVER_DIR)
	@cat $(EXTERNAL_DRIVER_DIR)/Kbuild | sed -e "s/@MODULES@/$(subst /,\\/,$(EXT_DRIVER_MODULES))"/ \
		> $(OUT_EXTERNAL_DRIVER_DIR)/Kbuild

$(call add-target-into-build, prepare_external_drivers)

###

.PHONY: build_external_drivers

build_external_drivers: prepare_external_drivers
	@echo "Build External Drivers:"
ifneq ($(EXT_DRIVER_MODULES),)
	@$(ECHO) "  $(patsubst %/,%\n ,$(EXT_DRIVER_MODULES))"
	$(AMBA_MAKEFILE_V)$(MAKE) $(AMBA_MAKE_PARA) ARCH=$(CPU_ARCH) -C $(LINUX_SRC_DIR) O=$(LINUX_OUT_DIR) \
		M=$(OUT_EXTERNAL_DRIVER_DIR) modules
	$(AMBA_MAKEFILE_V)$(MAKE) $(AMBA_MAKE_PARA) ARCH=$(CPU_ARCH) -C $(LINUX_SRC_DIR) O=$(LINUX_OUT_DIR) \
		M=$(OUT_EXTERNAL_DRIVER_DIR) $(LINUX_INSTALL_FLAG) INSTALL_MOD_DIR=extra modules_install;
else
	@echo "  Nothing"
endif
	@echo "Build External Drivers Done"

$(call add-target-into-build, build_external_drivers)

###
endif

