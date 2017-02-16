##
## $(MODULE_NAME_TAG)/common/base/runtime_config/Makefile
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

include ../../../build/core/linux/pre_requirement.mk

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/config_linux.mk

SUBDIRS	:= simple_ini
ifeq ($(BUILD_CONFIG_MODULE_LIBXML2), y)
SUBDIRS	+= xml
endif

MODULE_SRC = $(wildcard *.cpp)
MODULE_OBJ = $(MODULE_SRC:.cpp=.o)

common_base_runtime_config_src		=	$(MODULE_SRC)
common_base_runtime_config_obj		=	$(common_base_runtime_config_src:.cpp=.o)

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk



