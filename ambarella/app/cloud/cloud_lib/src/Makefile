##
## $(MODULE_NAME_TAG)/cloud_lib/src/Makefile
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

include ../../build/core/linux/pre_requirement.mk

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/config_linux.mk

SUBDIRS = protocols user_protocols

cloud_lib_interface_src := $(wildcard *.cpp)
cloud_lib_interface_obj := $(cloud_lib_interface_src:%.cpp=%.o)

cloud_lib_protocols_sacp_src = $(wildcard protocols/sacp/*.cpp)
cloud_lib_protocols_sacp_obj = $(cloud_lib_protocols_sacp_src:protocols/sacp/%.cpp=protocols/sacp/%.o)

cloud_lib_user_protocols_default_src = $(wildcard user_protocols/default/*.cpp)
cloud_lib_user_protocols_default_obj = $(cloud_lib_user_protocols_default_src:user_protocols/default/%.cpp=user_protocols/default/%.o)

MODULE_SRC := 	 $(cloud_lib_interface_src)	\
			 $(cloud_lib_protocols_sacp_src) \
			 $(cloud_lib_user_protocols_default_src)

MODULE_OBJ := 	 $(cloud_lib_interface_obj)	\
			 $(cloud_lib_protocols_sacp_obj) \
			 $(cloud_lib_user_protocols_default_obj)

STATIC_LIB_NAMES = mwcg_cloud_lib

mwcg_cloud_lib_src     := $(MODULE_SRC)
mwcg_cloud_lib_obj     := $(MODULE_OBJ)

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk



