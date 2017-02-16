##
## $(MODULE_NAME_TAG)/licence/encryption/Makefile
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

SUBDIRS = customized_v1
MODULE_INC = -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/common/include -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/licence/include 

licence_encryption_src := $(wildcard *.cpp)
licence_encryption_obj := $(licence_encryption_src:%.cpp=%.o)

licence_encryption_customized_v1_src = $(wildcard customized_v1/*.cpp)
licence_encryption_customized_v1_obj = $(licence_encryption_customized_v1_src:customized_v1/%.cpp=customized_v1/%.o)

MODULE_SRC := 	 $(licence_encryption_src)	\
			 $(licence_encryption_customized_v1_src)

MODULE_OBJ := 	 $(licence_encryption_obj)	\
			 $(licence_encryption_customized_v1_obj)

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk



