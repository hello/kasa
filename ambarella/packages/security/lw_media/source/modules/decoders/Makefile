##
## $(MODULE_NAME_TAG)/source/modules/decoders/Makefile
##
## History:
##    2015/07/28 - [Zhi He] Create File
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

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/extern_lib.mk

MODULE_INC += $(EXTERN_LIB_LIBAAC_INC)
MODULE_INC += $(EXTERN_LIB_FFMPEG_INC)

MODULE_SRC := $(wildcard *.cpp)

ifeq ($(BUILD_CONFIG_MODULE_LIBAAC), n)
MODULE_LIBAAC_DECODER_SRC = aac_audio_decoder.cpp
MODULE_SRC := $(filter-out $(MODULE_LIBAAC_DECODER_SRC),$(MODULE_SRC))
endif

ifeq ($(BUILD_CONFIG_MODULE_FFMPEG), n)
MODULE_FFMPEG_DECODER_SRC := modules/decoders/ffmpeg_video_decoder.cpp
MODULE_SRC := $(filter-out $(MODULE_FFMPEG_DECODER_SRC),$(MODULE_SRC))
endif

MODULE_OBJ := $(MODULE_SRC:.cpp=.o)
MODULE_DEF = $(V_COMPILE_CFLAGS)

lw_media_modules_decoders_src		=	$(MODULE_SRC)
lw_media_modules_decoders_obj		=	$(lw_media_modules_decoders_src:.cpp=.o)

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk



