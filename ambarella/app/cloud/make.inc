###############################################################################
## app/cloud/make.inc
##
## History:
##    2015/04/09 - [Zhi He] Created file
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
##################################################################################

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

