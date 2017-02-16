##
## $(MODULE_NAME_TAG)/licence/unit_test/Makefile
##
## History:
##    2014/03/10 - [Zhi He] Create File
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

MODULE_INC = -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/common/include -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/licence/include 
MODULE_LIB = -lrt -lpthread -L$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/lib -lmwcg_common -lmwcg_licence
MODULE_LIB += -lpthread -ldl -lrt

LICENCE_GENERATOR_MODULE_TARGET := $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/bin/licence_generator
LICENCE_GENERATOR_MODULE_SRC := licence_generator.cpp

DEVICEID_GENERATOR_MODULE_TARGET := $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/bin/deviceid_generator
DEVICEID_GENERATOR_MODULE_SRC := deviceid_generator.cpp

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk

all:
	$(MAKE_PRINT)echo "build licence_generator:"
	$(MAKE_PRINT)$(CXX) -o $(LICENCE_GENERATOR_MODULE_TARGET) $(MODULE_INC) -g -O0 $(LICENCE_GENERATOR_MODULE_SRC) $(MODULE_LIB)
	$(MAKE_PRINT)echo "build deviceid_generator:"
	$(MAKE_PRINT)$(CXX) -o $(DEVICEID_GENERATOR_MODULE_TARGET) $(MODULE_INC) -g -O0 $(DEVICEID_GENERATOR_MODULE_SRC) $(MODULE_LIB)

clean:
	$(MAKE_PRINT)echo "clean licence_generator:"
	$(MAKE_PRINT)$(RM) $(LICENCE_GENERATOR_MODULE_TARGET)
	$(MAKE_PRINT)echo "clean deviceid_generator:"
	$(MAKE_PRINT)$(RM) $(DEVICEID_GENERATOR_MODULE_TARGET)





