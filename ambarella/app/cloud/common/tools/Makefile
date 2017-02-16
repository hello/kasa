##
## $(MODULE_NAME_TAG)/common/tools/Makefile
##
## History:
##    2014/07/21 - [Zhi He] Create File
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

MODULE_INC = -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/common/include 
MODULE_LIB = -lrt -lpthread -L$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/lib -lmwcg_common 

MODULE_LIB += -lpthread -ldl -lrt
MODULE_SRC = guardian.cpp
MODULE_TARGET = $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/bin/guardian

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk

all:
	$(MAKE_PRINT)echo "build guardian:"
	$(MAKE_PRINT)mkdir -p $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/bin
	$(MAKE_PRINT)$(CXX) -o $(MODULE_TARGET) $(MODULE_INC) -g -O0 $(MODULE_SRC) $(MODULE_LIB)

clean:
	$(MAKE_PRINT)echo "clean guardian:"
	$(MAKE_PRINT)$(RM) $(MODULE_TARGET)






