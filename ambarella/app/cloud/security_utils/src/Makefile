##
## $(MODULE_NAME_TAG)/security_utils/src/Makefile
##
## History:
##    2014/07/03 - [Zhi He] Create File
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

SUBDIRS = hash encryption helper

hash_src = $(wildcard hash/*.cpp)
hash_obj = $(hash_src:hash/%.cpp=hash/%.o)

encryption_src := $(wildcard encryption/*.cpp)
encryption_obj := $(encryption_src:encryption/%.cpp=encryption/%.o)

helper_src := $(wildcard helper/*.cpp)
helper_obj := $(helper_src:helper/%.cpp=helper/%.o)

lib_MODULE_SRC := 	 $(hash_src) \
	$(encryption_src) \
	$(helper_src)

lib_MODULE_OBJ := 	 $(hash_obj) \
	$(encryption_obj) \
	$(helper_obj)

STATIC_LIB_NAMES = mwcg_security_lib

mwcg_security_lib_src     := $(lib_MODULE_SRC)
mwcg_security_lib_obj     := $(lib_MODULE_OBJ)

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk



