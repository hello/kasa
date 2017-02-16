##
## $(MODULE_NAME_TAG)/im_mw/src/Makefile
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

SUBDIRS = base server account_management

base_src = $(wildcard base/*.cpp)
base_obj = $(base_src:base/%.cpp=base/%.o)

server_src := $(wildcard server/*.cpp)
server_obj := $(server_src:server/%.cpp=server/%.o)

account_management_src = $(wildcard account_management/*.cpp)
account_management_obj = $(account_management_src:account_management/%.cpp=account_management/%.o)

lib_MODULE_SRC := 	 $(base_src)	\
			 $(server_src)   \
             $(account_management_src)

lib_MODULE_OBJ := 	 $(base_obj)	\
			 $(server_obj)   \
             $(account_management_obj)

STATIC_LIB_NAMES = mwcg_im_mw
#SHARED_LIB_NAMES = mwcg_im_mw

mwcg_im_mw_src     := $(lib_MODULE_SRC)
mwcg_im_mw_obj     := $(lib_MODULE_OBJ)

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk



