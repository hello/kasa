##
## $(MODULE_NAME_TAG)/licence/client/Makefile
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

SUBDIRS = standalone
MODULE_INC = -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/common/include -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/licence/include 

licence_client_src := $(wildcard *.cpp)
licence_client_obj := $(licence_client_src:%.cpp=%.o)

licence_client_standalone_src = $(wildcard standalone/*.cpp)
licence_client_standalone_obj = $(licence_client_standalone_src:standalone/%.cpp=standalone/%.o)

MODULE_SRC := 	 $(licence_client_src)	\
			 $(licence_client_standalone_src)

MODULE_OBJ := 	 $(licence_client_obj)	\
			 $(licence_client_standalone_obj)

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk



