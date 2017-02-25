##
## kernel/private/drivers/ambarella/make.inc
##
## History:
##    2012/06/04 - [Cao Rongrong] Created file
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

