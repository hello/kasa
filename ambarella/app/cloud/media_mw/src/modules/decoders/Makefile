##
## $(MODULE_NAME_TAG)/media_mw/src/modules/decoders/Makefile
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

ifeq ($(CONFIG_ARCH_I1), y)
include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/platform_i1_linux.mk
endif

ifeq ($(CONFIG_ARCH_S2), y)
include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/platform_s2_linux.mk
endif

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/extern_lib.mk

MODULE_INC = -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/media_mw/src/internal_include 
MODULE_INC += $(EXTERN_LIB_FFMPEG_INC)
MODULE_INC += $(EXTERN_LIB_LIBAAC_INC)
MODULE_INC += $(EXTERN_BUILD_INC)

MODULE_SRC := $(wildcard *.cpp)

ifeq ($(BUILD_CONFIG_MODULE_LIBAAC), n)
MODULE_LIBAAC_DECODER_SRC = aac_audio_decoder.cpp
MODULE_SRC := $(filter-out $(MODULE_LIBAAC_DECODER_SRC),$(MODULE_SRC))
endif

ifeq ($(BUILD_CONFIG_MODULE_FFMPEG), n)
MODULE_AUDIO_FFMPEG_DECODER_SRC = ffmpeg_audio_decoder.cpp ffmpeg_video_decoder.cpp
MODULE_SRC := $(filter-out $(MODULE_AUDIO_FFMPEG_DECODER_SRC),$(MODULE_SRC))
endif

ifeq ($(BUILD_CONFIG_MODULE_AMBA_DSP), n)
MODULE_AMBA_DECODER_SRC = amba_video_decoder.cpp
MODULE_SRC := $(filter-out $(MODULE_AMBA_DECODER_SRC),$(MODULE_SRC))
endif

MODULE_OBJ := $(MODULE_SRC:.cpp=.o)
MODULE_DEF = $(V_COMPILE_CFLAGS)

media_mw_modules_decoders_src		=	$(MODULE_SRC)
media_mw_modules_decoders_obj		=	$(media_mw_modules_decoders_src:.cpp=.o)

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk



