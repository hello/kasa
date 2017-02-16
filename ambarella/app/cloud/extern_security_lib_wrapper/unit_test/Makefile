##
## $(MODULE_NAME_TAG)/extern_security_lib_wrapper/unit_test/Makefile
##
## History:
##    2015/04/17 - [Zhi He] Create File
##
## Copyright (C) 2014 - 2024, the Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of the Ambarella, Inc.
##

default: all

include ../../build/core/linux/pre_requirement.mk

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/config_linux.mk
include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/extern_lib.mk

.PHONY: all clean

PREBUILD_3RD_PARTY_DIR := $(DEVICE_DIR)/prebuild/third-party/armv7-a-hf/

MODULE_INC += -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/extern_security_lib_wrapper/include

ifeq ($(BUILD_CONGIG_OS_LINUX), y)
MODULE_INC += -DBUILD_OS_LINUX=1
endif

MODULE_LIB += -L$(PREBUILD_3RD_PARTY_DIR)/openssl/usr/lib -lmwcg_extern_security_wrapper -lssl -lcrypto -ldl
MODULE_LIB += -L$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/lib

MODULE_SRC = test_cypher.cpp
MODULE_TARGET = $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/bin/test_cypher

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk

all:
	$(MAKE_PRINT)echo "build test_cypher:"
	$(MAKE_PRINT)mkdir -p $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/bin
	$(MAKE_PRINT)$(CXX) -o $(MODULE_TARGET) $(MODULE_INC) -g -O0 $(MODULE_SRC) $(MODULE_LIB)

clean:
	$(MAKE_PRINT)echo "clean test_cypher:"
	$(MAKE_PRINT)$(RM) $(MODULE_TARGET)






