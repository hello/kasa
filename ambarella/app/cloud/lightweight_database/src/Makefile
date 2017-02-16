##
## $(MODULE_NAME_TAG)/lightweight_database/src/Makefile
##
## History:
##    2014/06/19 - [Zhi He] Create File
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

SUBDIRS = two_level_hash

two_level_hash_src = $(wildcard two_level_hash/*.cpp)
two_level_hash_obj = $(two_level_hash_src:two_level_hash/%.cpp=two_level_hash/%.o)

lightweight_database_src := $(wildcard *.cpp)
lightweight_database_obj := $(lightweight_database_src:%.cpp=%.o)

lib_MODULE_SRC := 	 $(two_level_hash_src)	\
			 $(lightweight_database_src)

lib_MODULE_OBJ := 	 $(two_level_hash_obj)	\
			 $(lightweight_database_obj)

STATIC_LIB_NAMES = mwcg_lwdb_lib
#SHARED_LIB_NAMES = mwcg_lwdb_lib

mwcg_lwdb_lib_src     := $(lib_MODULE_SRC)
mwcg_lwdb_lib_obj     := $(lib_MODULE_OBJ)

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk



