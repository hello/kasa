##
## app/cloud/make.inc
##
## History:
##    2015/04/09 - [Zhi He] Created file
##
## Copyright (C) 2015-2025, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ARCH := arm
export ARCH

export AMB_TOPDIR
export AMB_BOARD
export FAKEROOT_DIR
export AMB_BOARD_OUT
export PREBUILD_3RD_PARTY_DIR
export TOOLCHAIN_PATH

ifeq ($(BUILD_AMBAREALLA_CLOUD_LIB), y)

LOCAL_PATH:=$(call my-dir)
AMBA_CLOUD_SRC_DIR	:= $(AMB_TOPDIR)/app/cloud
AMBA_CLOUD_AGENT_DIR	:= $(AMB_TOPDIR)/app/cloud/platform_specific/amba_s2l/cloud_agent
#####

.PHONY: amba_cloud amba_cloud_agent

amba_cloud:
	$(AMBA_MAKEFILE_V)$(MAKE) $(AMBA_MAKE_PARA) -C $(AMBA_CLOUD_SRC_DIR) for_cloud_agent
##	@echo "Build amba cloud Done."
$(call add-target-into-build, amba_cloud)

ifeq ($(BUILD_AMBAREALLA_CLOUD_AGENT), y)
ifeq ($(BUILD_AMBARELLA_ORYX_MW), y)
ifeq ($(BUILD_AMBARELLA_ORYX_STREAM), y)
amba_cloud_agent: amba_cloud libamexport-client.so libamutil.so
	$(AMBA_MAKEFILE_V)$(MAKE) $(AMBA_MAKE_PARA) -C $(AMBA_CLOUD_AGENT_DIR) all
##	@echo "Build amba cloud agent Done."

$(call add-target-into-build, amba_cloud_agent)
endif
endif
endif

endif

###

