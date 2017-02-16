##
## $(MODULE_NAME_TAG)/unit_test/Makefile
##
## History:
##    2013/04/15 - [Zhi He] Create File
##
## Copyright (C) 2014 - 2024, the Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of the Ambarella, Inc.
##

default: all

include ../build/core/linux/pre_requirement.mk

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/config_linux.mk
include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/extern_lib.mk

.PHONY: all clean

MODULE_INC = -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/include
MODULE_LIB = -lrt -lpthread -L$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/lib -llw_media

ifeq ($(BUILD_CONFIG_MODULE_LIBAAC), y)
MODULE_LIB += $(EXTERN_LIB_LIBAAC_LIB) -laacdec -laacenc
endif
ifeq ($(BUILD_CONFIG_MODULE_ALSA), y)
MODULE_LIB += $(EXTERN_LIB_ALSA_LIB) -lasound
endif
ifeq ($(BUILD_CONFIG_MODULE_FFMPEG), y)
MODULE_LIB += $(EXTERN_LIB_FFMPEG_LIB) -lavformat -lavcodec -lavutil
endif
MODULE_LIB += -lpthread -ldl -lrt

UT_MODULE_SRC = test_media.cpp
UT_MODULE_TARGET = $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/bin/test_media

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk

all:
	$(MAKE_PRINT)echo "build test_media:"
	$(MAKE_PRINT)$(MKDIR) $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/bin/
	$(MAKE_PRINT)$(CXX) -o $(UT_MODULE_TARGET) $(MODULE_INC) -g -O0 $(UT_MODULE_SRC) $(MODULE_LIB)
##	$(MAKE_PRINT)$(STRIP) $(UT_MODULE_TARGET)

clean:
	$(MAKE_PRINT)echo "clean test_media:"
	$(MAKE_PRINT)$(RM) $(UT_MODULE_TARGET)





