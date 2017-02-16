##
## $(MODULE_NAME_TAG)/media_mw/src/modules/muxers/Makefile
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

include ../../../../build/core/linux/pre_requirement.mk

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/config_linux.mk

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/extern_lib.mk

MODULE_INC = -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/media_mw/src/internal_include 
MODULE_INC += $(EXTERN_LIB_FFMPEG_INC)

MODULE_SRC := $(wildcard *.cpp)

ifeq ($(BUILD_CONFIG_MODULE_FFMPEG), n)
MODULE_FFMPEG_MUXER_SRC := ffmpeg_muxer.cpp
MODULE_SRC := $(filter-out $(MODULE_FFMPEG_MUXER_SRC),$(MODULE_SRC))
endif

MODULE_OBJ := $(MODULE_SRC:.cpp=.o)

media_mw_modules_muxers_src		=	$(MODULE_SRC)
media_mw_modules_muxers_obj		=	$(media_mw_modules_muxers_src:.cpp=.o)

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk



