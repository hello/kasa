##
## $(MODULE_NAME_TAG)/licence/Makefile
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

include ../build/core/linux/pre_requirement.mk

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/config_linux.mk

SUBDIRS = encryption misc client
MODULE_INC = -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/common/include -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/licence/include 

encryption_src = $(wildcard encryption/*.cpp)
encryption_obj = $(encryption_src:encryption/%.cpp=encryption/%.o) 

encryption_customized_v1_src = $(wildcard encryption/customized_v1/*.cpp)
encryption_customized_v1_obj = $(encryption_customized_v1_src:encryption/customized_v1/%.cpp=encryption/customized_v1/%.o) 

client_src = $(wildcard client/*.cpp)
client_obj = $(client_src:client/%.cpp=client/%.o) 

client_standalone_src := $(wildcard client/standalone/*.cpp)
client_standalone_obj := $(client_standalone_src:client/standalone/%.cpp=client/standalone/%.o) 

misc_src = $(wildcard misc/*.cpp)
misc_obj = $(misc_src:misc/%.cpp=misc/%.o) 

MODULE_SRC = $(encryption_src)   \
             $(encryption_customized_v1_src)     \
             $(client_src) \
			 $(client_standalone_src) \
			 $(misc_src)

MODULE_OBJ = $(encryption_obj)   \
             $(encryption_customized_v1_obj)     \
			 $(client_obj) \
             $(client_standalone_obj) \
			 $(misc_obj)

STATIC_LIB_NAMES = mwcg_licence
#SHARED_LIB_NAMES = mwcg_licence

mwcg_licence_src     = $(MODULE_SRC)
mwcg_licence_obj     = $(MODULE_OBJ)

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk



