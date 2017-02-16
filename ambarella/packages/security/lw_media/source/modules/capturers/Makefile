##
## $(MODULE_NAME_TAG)/source/modules/captures/Makefile
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

ifeq ($(BUILD_CONFIG_MODULE_ALSA), n)
MODULE_ALSA_CAPTURER_SRC := alsa_audio_capturer.cpp
MODULE_SRC := $(filter-out $(MODULE_ALSA_CAPTURER_SRC),$(MODULE_SRC))
endif

MODULE_OBJ := $(MODULE_SRC:.cpp=.o)

lw_media_modules_capturers_src		=	$(MODULE_SRC)
lw_media_modules_capturers_obj		=	$(lw_media_modules_capturers_src:.cpp=.o)

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk
