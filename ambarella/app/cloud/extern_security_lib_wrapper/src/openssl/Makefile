##
## $(MODULE_NAME_TAG)/extern_security_lib_wrapper/src/openssl/Makefile
##
## History:
##    2015/04/17 - [Zhi He] Create File
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

PREBUILD_3RD_PARTY_DIR := $(DEVICE_DIR)/prebuild/third-party/armv7-a-hf/

MODULE_INC := -I$(PREBUILD_3RD_PARTY_DIR)/openssl/include

MODULE_SRC := $(wildcard *.cpp)
MODULE_OBJ := $(MODULE_SRC:%.cpp=%.o)

STATIC_LIB_NAMES = mwcg_extern_security_wrapper

mwcg_extern_security_wrapper_src     := $(MODULE_SRC)
mwcg_extern_security_wrapper_obj     := $(MODULE_OBJ)

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk



