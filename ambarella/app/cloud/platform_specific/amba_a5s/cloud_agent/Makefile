###############################################################################
## $(MODULE_NAME_TAG)/platform_specific/amba_a5s/cloud_agent/Makefile
##
## History:
##    2014/09/12 - [Zhi He] Create File
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
##################################################################################

default: all

include ../../../build/core/linux/pre_requirement.mk

##DEVICE_APP_DIR := $(BUILDSYSTEM_DIR)
DEVICE_APP_DIR := $(DEVICE_DIR)

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/config_linux.mk

agent_src = $(wildcard *.cpp)

.PHONY: all clean

MODULE_INC = -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/common/include -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/cloud_lib/include -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/security_utils/include 
MODULE_LIB = -lrt -lpthread -L$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/lib -lmwcg_cloud_lib -lmwcg_security_lib -lmwcg_common 

MODULE_INC += -I$(DEVICE_DIR)/camera/include -I$(DEVICE_DIR)/boards/a5sipcam -I$(DEVICE_DIR)/include -I$(DEVICE_DIR)/include/arch_a5s
MODULE_INC += -L$(DEVICE_DIR)/out/xman/camera/modules/utilities -L$(DEVICE_DIR)/out/xman/camera/modules/transfer
MODULE_LIB += -lpthread -ldl -lrt -lamutils -lamdatatransfer
MODULE_SRC = $(agent_src)
MODULE_TARGET = $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/bin/a5s_cloud_agent

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk


all:
	$(MAKE_PRINT)echo "build a5s_cloud_agent:"
	$(MAKE_PRINT)mkdir -p $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/bin
	$(MAKE_PRINT)$(CXX) -o $(MODULE_TARGET) $(MODULE_INC) -g -O0 $(MODULE_SRC) $(MODULE_LIB)
##	$(MAKE_PRINT)$(STRIP) $(MODULE_TARGET)

clean:
	$(MAKE_PRINT)echo "clean a5s_cloud_agent:"
	$(MAKE_PRINT)$(RM) $(MODULE_TARGET)




