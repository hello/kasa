##
## $(MODULE_NAME_TAG)/storage_lib/src/Makefile
##
## History:
##    2014/05/07 - [Zhi He] Create File
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

SUBDIRS = simple

MODULE_INC = -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/storage_lib/include/

storage_lib_interface_src := $(wildcard *.cpp)
storage_lib_interface_obj := $(storage_lib_interface_src:%.cpp=%.o)

storage_lib_simple_src = $(wildcard simple/*.cpp)
storage_lib_simple_obj = $(storage_lib_simple_src:simple/%.cpp=simple/%.o)

MODULE_SRC := 	 $(storage_lib_interface_src)	\
			 $(storage_lib_simple_src)

MODULE_OBJ := 	 $(storage_lib_interface_obj)	\
			 $(storage_lib_simple_obj)

STATIC_LIB_NAMES = mwcg_storage_lib

mwcg_storage_lib_src     := $(MODULE_SRC)
mwcg_storage_lib_obj     := $(MODULE_OBJ)

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk



