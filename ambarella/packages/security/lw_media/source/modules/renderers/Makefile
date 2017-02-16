##
## $(MODULE_NAME_TAG)/source/modules/renderers/Makefile
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

MODULE_SRC := $(wildcard *.cpp)
ifeq ($(BUILD_CONFIG_MODULE_AMBA_DSP), n)
MODULE_AMBA_RENDERER_SRC = amba_video_renderer.cpp
MODULE_SRC := $(filter-out $(MODULE_AMBA_RENDERER_SRC),$(MODULE_SRC))
endif

ifeq ($(BUILD_CONFIG_MODULE_ALSA), n)
MODULE_ALSA_RENDERER_SRC := alsa_audio_renderer.cpp
MODULE_SRC := $(filter-out $(MODULE_ALSA_RENDERER_SRC),$(MODULE_SRC))
endif

ifeq ($(BUILD_CONFIG_MODULE_LINUX_FB), n)
MODULE_LINUX_FB_RENDERER_SRC := linuxfb_video_renderer.cpp
MODULE_SRC := $(filter-out $(MODULE_LINUX_FB_RENDERER_SRC),$(MODULE_SRC))
endif

MODULE_OBJ := $(MODULE_SRC:.cpp=.o)

lw_media_modules_renderers_src		=	$(MODULE_SRC)
lw_media_modules_renderers_obj		=	$(MODULE_OBJ)

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk



