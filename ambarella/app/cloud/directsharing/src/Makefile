##
## $(MODULE_NAME_TAG)/im_mw/src/Makefile
##
## History:
##    2015/03/11 - [Zhi He] Create File
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

directsharing_src = $(wildcard *.cpp)
directsharing_obj = $(directsharing_src:%.cpp=%.o)

lib_MODULE_SRC := 	 $(directsharing_src)

lib_MODULE_OBJ := 	 $(directsharing_obj)

STATIC_LIB_NAMES = mwcg_directsharing

mwcg_directsharing_src     := $(lib_MODULE_SRC)
mwcg_directsharing_obj     := $(lib_MODULE_OBJ)

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk



