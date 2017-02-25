###############################################################################
## $(MODULE_NAME_TAG)/Makefile
##
## History:
##    2013/04/16 - [Zhi He] Create File
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

default: all

include build/core/linux/pre_requirement.mk

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/config_linux.mk

.PHONY: a5s i1 s2 s2l a7l ubuntu centos
.PHONY: device_common a5s_specific s2l_specific a7l_specific
.PHONY: sync_build_a5s sync_build_i1 sync_build_s2 sync_build_s2l sync_build_a7l sync_build_ubuntu sync_build_centos
.PHONY: all common security_utils extern_security_lib_wrappers cloud_lib licence storage_lib lw_db im_mw media_mw common_ut cloud_lib_ut storage_lib_ut media_mw_ut im_mw_ut 
.PHONY: clean clean_common clean_security_utils clean_cloud_lib clean_storage_lib clean_ws_discovery clean_media_mw clean_common_ut clean_cloud_lib_ut clean_storage_lib_ut clean_media_mw_ut clean_lw_db clean_im_mw clean_im_mw_ut cleandir

.PHONY: for_cloud_agent

common:
	$(MAKE_PRINT)echo "Build libmwcg_common.a Done."
	$(MAKE_PRINT)$(MAKE) -C common default --no-print-directory

security_utils:
	$(MAKE_PRINT)echo "Build libmwcg_security_lib.a Done."
	$(MAKE_PRINT)$(MAKE) -C security_utils default --no-print-directory

extern_security_lib_wrappers:
	$(MAKE_PRINT)echo "Build libmwcg_extern_security_wrapper.a Done."
	$(MAKE_PRINT)$(MAKE) -C extern_security_lib_wrapper default --no-print-directory
	
cloud_lib: common security_utils
	$(MAKE_PRINT)echo "Build libmwcg_cloud_lib.a Done."
	$(MAKE_PRINT)$(MAKE) -C cloud_lib default --no-print-directory

storage_lib: common
	$(MAKE_PRINT)echo "Build libmwcg_storage_lib.a Done."
	$(MAKE_PRINT)$(MAKE) -C storage_lib default --no-print-directory

licence: common
	$(MAKE_PRINT)echo "Build libmwcg_licence.a Done."
	$(MAKE_PRINT)$(MAKE) -C licence default --no-print-directory

lw_db: common
	$(MAKE_PRINT)echo "Build libmwcg_lw_db.a Done."
	$(MAKE_PRINT)$(MAKE) -C lightweight_database default --no-print-directory

im_mw: common security_utils cloud_lib lw_db  
	$(MAKE_PRINT)echo "Build libmwcg_im_mw.a Done."
	$(MAKE_PRINT)$(MAKE) -C im_mw default --no-print-directory

media_mw: common security_utils cloud_lib storage_lib licence 
	$(MAKE_PRINT)echo "Build libmwcg_media_mw.a Done."
	$(MAKE_PRINT)$(MAKE) -C media_mw default --no-print-directory

common_ut: common
	$(MAKE_PRINT)echo "Build common_ut Done."
	$(MAKE_PRINT)$(MAKE) -C common/tools default --no-print-directory

cloud_lib_ut: common security_utils cloud_lib 
	$(MAKE_PRINT)echo "Build cloud_lib_ut Done."
	$(MAKE_PRINT)$(MAKE) -C cloud_lib/unit_test default --no-print-directory

im_mw_ut: common security_utils cloud_lib lw_db im_mw   
	$(MAKE_PRINT)echo "Build im_mw_ut Done."
	$(MAKE_PRINT)$(MAKE) -C im_mw/unit_test default --no-print-directory

storage_lib_ut: common storage_lib 
	$(MAKE_PRINT)echo "Build storage_lib_ut Done."
	$(MAKE_PRINT)$(MAKE) -C storage_lib/unit_test default --no-print-directory

media_mw_ut: common security_utils cloud_lib storage_lib licence media_mw
	$(MAKE_PRINT)echo "Build media_mw_ut Done."
	$(MAKE_PRINT)$(MAKE) -C media_mw/unit_test default --no-print-directory

all: common security_utils cloud_lib storage_lib licence lw_db im_mw media_mw common_ut cloud_lib_ut storage_lib_ut media_mw_ut im_mw_ut
##	$(MAKE_PRINT)echo "Build mwcg all Done."

for_cloud_agent: common security_utils cloud_lib extern_security_lib_wrappers
	$(MAKE_PRINT)echo "Build ambarella cloud library Done."

sync_build_a5s:
	$(MAKE_PRINT)cp -a build/core/linux/targets/amba_a5s_arm_11/config_linux.mk build/core/linux/config_linux.mk
	$(MAKE_PRINT)cp -a build/core/linux/targets/amba_a5s_arm_11/extern_lib.mk build/core/linux/extern_lib.mk

sync_build_a7l:
	$(MAKE_PRINT)cp -a build/core/linux/targets/amba_a7l_arm_11/config_linux.mk build/core/linux/config_linux.mk
	$(MAKE_PRINT)cp -a build/core/linux/targets/amba_a7l_arm_11/extern_lib.mk build/core/linux/extern_lib.mk

sync_build_i1:
	$(MAKE_PRINT)cp -a build/core/linux/targets/amba_i1_arm_cortexa9/config_linux.mk build/core/linux/config_linux.mk
	$(MAKE_PRINT)cp -a build/core/linux/targets/amba_i1_arm_cortexa9/extern_lib.mk build/core/linux/extern_lib.mk

sync_build_s2:
	$(MAKE_PRINT)cp -a build/core/linux/targets/amba_s2_arm_cortexa9/config_linux.mk build/core/linux/config_linux.mk
	$(MAKE_PRINT)cp -a build/core/linux/targets/amba_s2_arm_cortexa9/extern_lib.mk build/core/linux/extern_lib.mk

sync_build_s2l:
	$(MAKE_PRINT)cp -a build/core/linux/targets/amba_s2l_arm_cortexa9/config_linux.mk build/core/linux/config_linux.mk
	$(MAKE_PRINT)cp -a build/core/linux/targets/amba_s2l_arm_cortexa9/extern_lib.mk build/core/linux/extern_lib.mk

sync_build_ubuntu:
	$(MAKE_PRINT)cp -a build/core/linux/targets/generic_ubuntu_x86/config_linux.mk build/core/linux/config_linux.mk

sync_build_centos:
	$(MAKE_PRINT)cp -a build/core/linux/targets/generic_ubuntu_x86/config_linux.mk build/core/linux/config_linux.mk

a5s_specific: device_base
	$(MAKE_PRINT)echo "Build a5s specific Done."
	$(MAKE_PRINT)$(MAKE) -C platform_specific/amba_a5s/cloud_agent default --no-print-directory

a7l_specific: device_base
	$(MAKE_PRINT)echo "Build a7l specific Done."

s2l_specific: device_base
	$(MAKE_PRINT)echo "Build s2l specific Done."

device_base: common security_utils cloud_lib
	$(MAKE_PRINT)echo "Build device common Done."

a5s: device_base a5s_specific
	$(MAKE_PRINT)echo "Build a5s Done."

a7l: device_base a7l_specific
	$(MAKE_PRINT)echo "Build a7l Done."

s2l: device_base s2l_specific
	$(MAKE_PRINT)echo "Build s2l Done."

clean_common:
##	$(MAKE_PRINT)echo "Clean common Done."
	$(MAKE_PRINT)$(MAKE) -C common clean --no-print-directory

clean_security_utils:
##	$(MAKE_PRINT)echo "Clean security_utils Done."
	$(MAKE_PRINT)$(MAKE) -C security_utils clean --no-print-directory

clean_cloud_lib:
##	$(MAKE_PRINT)echo "Clean cloud_lib Done."
	$(MAKE_PRINT)$(MAKE) -C cloud_lib clean --no-print-directory

clean_storage_lib:
##	$(MAKE_PRINT)echo "Clean storage_lib Done."
	$(MAKE_PRINT)$(MAKE) -C storage_lib clean --no-print-directory

clean_lw_db:
##	$(MAKE_PRINT)echo "Clean lw_db Done."
	$(MAKE_PRINT)$(MAKE) -C lightweight_database clean --no-print-directory

clean_im_mw:
##	$(MAKE_PRINT)echo "Clean im_mw Done."
	$(MAKE_PRINT)$(MAKE) -C im_mw clean --no-print-directory

clean_common_ut:
##	$(MAKE_PRINT)echo "Clean common_ut Done."
	$(MAKE_PRINT)$(MAKE) -C common/tools clean --no-print-directory

clean_im_mw_ut:
##	$(MAKE_PRINT)echo "Clean im_mw_ut Done."
	$(MAKE_PRINT)$(MAKE) -C im_mw/unit_test clean --no-print-directory

clean_media_mw:
##	$(MAKE_PRINT)echo "Clean media_mw Done."
	$(MAKE_PRINT)$(MAKE) -C media_mw clean --no-print-directory

clean_cloud_lib_ut:
##	$(MAKE_PRINT)echo "Clean cloud_lib_ut Done."
	$(MAKE_PRINT)$(MAKE) -C cloud_lib/unit_test clean --no-print-directory

clean_storage_lib_ut:
##	$(MAKE_PRINT)echo "Clean storage_lib_ut Done."
	$(MAKE_PRINT)$(MAKE) -C storage_lib/unit_test clean --no-print-directory

clean_media_mw_ut:
##	$(MAKE_PRINT)echo "Clean media_mw_ut Done."
	$(MAKE_PRINT)$(MAKE) -C media_mw/unit_test clean --no-print-directory

clean: clean_common clean_security_utils clean_cloud_lib clean_storage_lib clean_media_mw clean_common_ut clean_cloud_lib_ut clean_storage_lib_ut clean_media_mw_ut clean_lw_db clean_im_mw clean_im_mw_ut cleandir
##	$(MAKE_PRINT)echo "Clean mwcg Done."

cleandir:
	-@rm -rf out


