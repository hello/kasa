##
## $(MODULE_NAME_TAG)/common/Makefile
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

SUBDIRS = base utils osal io date_time

base_src = $(wildcard base/*.cpp)
base_obj = $(base_src:base/%.cpp=base/%.o) 

base_runtime_config_src = $(wildcard base/runtime_config/*.cpp)
base_runtime_config_obj = $(base_runtime_config_src:base/runtime_config/%.cpp=base/runtime_config/%.o) 

base_runtime_config_xml_src = $(wildcard base/runtime_config/xml/*.cpp)
base_runtime_config_xml_obj = $(base_runtime_config_xml_src:base/runtime_config/xml/%.cpp=base/runtime_config/xml/%.o) 

base_runtime_config_simple_ini_src = $(wildcard base/runtime_config/simple_ini/*.cpp)
base_runtime_config_simple_ini_obj = $(base_runtime_config_simple_ini_src:base/runtime_config/simple_ini/%.cpp=base/runtime_config/simple_ini/%.o) 

utils_src = $(wildcard utils/*.cpp)
utils_obj = $(utils_src:utils/%.cpp=utils/%.o) 

osal_generic_src = $(wildcard osal/generic/*.cpp)
osal_generic_obj = $(osal_generic_src:osal/generic/%.cpp=osal/generic/%.o) 

osal_linux_src = $(wildcard osal/linux/*.cpp)
osal_linux_obj = $(osal_linux_src:osal/linux/%.cpp=osal/linux/%.o) 

osal_src = $(wildcard osal/*.cpp)
osal_obj = $(osal_src:osal/%.cpp=osal/%.o) 

io_src = $(wildcard io/*.cpp)
io_obj = $(io_src:io/%.cpp=io/%.o) 

datetime_src = $(wildcard date_time/*.cpp)
datetime_obj = $(datetime_src:date_time/%.cpp=date_time/%.o) 

MODULE_SRC = $(base_src)   \
             $(utils_src)     \
             $(osal_generic_src) \
			 $(osal_linux_src) \
			 $(osal_src) \
			 $(base_runtime_config_src) \
			 $(base_runtime_config_simple_ini_src) \
			 $(io_src) \
			 $(datetime_src)

ifeq ($(BUILD_CONFIG_MODULE_LIBXML2), y)
MODULE_SRC += $(base_runtime_config_xml_src)
endif

MODULE_OBJ = $(base_obj)   \
             $(utils_obj)     \
			 $(base_runtime_config_obj) \
			 $(base_runtime_config_simple_ini_obj) \
             $(osal_generic_obj) \
			 $(osal_linux_obj) \
			 $(osal_obj) \
			 $(io_obj) \
			 $(datetime_obj)

ifeq ($(BUILD_CONFIG_MODULE_LIBXML2), y)
MODULE_OBJ += $(base_runtime_config_xml_obj)
endif

VER_FILE   = $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/common/include/common_version.h
VER_PREFIX = COMMON

STATIC_LIB_NAMES = mwcg_common
#SHARED_LIB_NAMES = mwcg_common

mwcg_common_src     = $(MODULE_SRC)
mwcg_common_obj     = $(MODULE_OBJ)
mwcg_common_major   = $(shell awk '/define\ $(VER_PREFIX)_LIB_MAJOR/{print $$3}' $(VER_FILE))
mwcg_common_minor   = $(shell awk '/define\ $(VER_PREFIX)_LIB_MINOR/{print $$3}' $(VER_FILE))
mwcg_common_patch   = $(shell awk '/define\ $(VER_PREFIX)_LIB_PATCH/{print $$3}' $(VER_FILE))
mwcg_common_version = $(mwcg_common_major).$(mwcg_common_minor).$(mwcg_common_patch)

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk



