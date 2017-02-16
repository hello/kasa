##
## $(MODULE_NAME_TAG)/platform_specific/amba_a5s/cloud_agent/Makefile
##
## History:
##    2014/09/12 - [Zhi He] Create File
##
## Copyright (C) 2014 - 2024, the Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of the Ambarella, Inc.
##

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




