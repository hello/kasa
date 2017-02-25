###############################################################################
## $(MODULE_NAME_TAG)/platform_specific/amba_s2l/cloud_agent/Makefile
##
## History:
##    2015/02/27 - [Zhi He] Create File
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

include ../../../build/core/linux/pre_requirement.mk

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/config_linux.mk

agent_src = $(wildcard *.cpp)

.PHONY: all clean

PREBUILD_3RD_PARTY_DIR := $(DEVICE_DIR)/prebuild/third-party/armv7-a-hf/
FAKEROOT_DIR ?= $(DEVICE_DIR)/out/s2lm_ironman/fakeroot
AMB_BOARD_OUT ?= $(DEVICE_DIR)/out/s2lm_ironman

MODULE_INC = -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/common/include -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/cloud_lib/include -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/security_utils/include 
MODULE_LIB = -lrt -lpthread -L$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/lib -lmwcg_cloud_lib -lmwcg_security_lib -lmwcg_common 

##MODULE_INC += -I$(DEVICE_DIR)/oryx/stream/include/record -I$(DEVICE_DIR)/include -I$(DEVICE_DIR)/oryx/utility/include -L$(DEVICE_DIR)/out/s2lm_ironman/fakeroot/usr/lib/ -L$(DEVICE_DIR)/out/s2lm_ironman/oryx/utility
MODULE_INC += -I$(DEVICE_DIR)/oryx/include/stream/ -I$(DEVICE_DIR)/oryx/stream/include/record -I$(DEVICE_DIR)/include -I$(DEVICE_DIR)/oryx/utility/include -I$(DEVICE_DIR)/oryx/include/utility/
MODULE_INC += -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/extern_security_lib_wrapper/include
MODULE_INC += -L$(FAKEROOT_DIR)/usr/lib/ -L$(AMB_BOARD_OUT)/oryx/utility/util -L$(AMB_BOARD_OUT)/oryx/utility/osal
MODULE_INC += -I$(DEVICE_DIR)/include/ -I$(DEVICE_DIR)/include/arch_s2l/
MODULE_LIB += -lpthread -ldl -lrt -lamexport-client -lamutil -lamosal -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib
MODULE_LIB += -L$(PREBUILD_3RD_PARTY_DIR)/openssl/usr/lib -lmwcg_extern_security_wrapper -lssl -lcrypto -ldl

MODULE_SRC = $(agent_src)
MODULE_TARGET = $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/bin/amba_cloud_agent
MODULE_TARGET_ROOTFS = $(FAKEROOT_DIR)/usr/local/bin/amba_cloud_agent
TARGET_TEST_ACCOUNT = $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/platform_specific/amba_s2l/test_account/iman_0/cloud_account.conf
TARGET_TEST_ACCOUNT_PATH = $(FAKEROOT_DIR)/etc/cloud

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk


all:
	$(MAKE_PRINT)mkdir -p $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/bin
	$(MAKE_PRINT)mkdir -p $(FAKEROOT_DIR)/usr/local/bin/
	$(MAKE_PRINT)$(CXX) -o $(MODULE_TARGET) $(MODULE_INC) -g -O0 $(MODULE_SRC) $(MODULE_LIB)
	$(MAKE_PRINT)$(CP) $(MODULE_TARGET) $(MODULE_TARGET_ROOTFS)
	$(MAKE_PRINT)mkdir -p $(TARGET_TEST_ACCOUNT_PATH)
	$(MAKE_PRINT)$(CP) $(TARGET_TEST_ACCOUNT) $(TARGET_TEST_ACCOUNT_PATH)/
	$(MAKE_PRINT)echo "Build amba_cloud_agent Done."
##	$(MAKE_PRINT)$(STRIP) $(MODULE_TARGET)

clean:
	$(MAKE_PRINT)echo "Clean amba_cloud_agent Done."
	$(MAKE_PRINT)$(RM) $(MODULE_TARGET)




