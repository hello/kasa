##
## $(MODULE_NAME_TAG)/media_mw/src/dsp_platform/Makefile
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

SUBDIRS	:= 

ifeq ($(BUILD_CONGIG_DSP_AMBA_I1), y)
SUBDIRS	+= amba_i1
endif

ifeq ($(BUILD_CONGIG_DSP_AMBA_S2), y)
SUBDIRS	+= amba_s2
endif

MODULE_INC = -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/media_mw/src/internal_include 
MODULE_SRC = $(wildcard *.cpp)
MODULE_OBJ = $(MODULE_SRC:.cpp=.o)
MODULE_DEF = $(V_COMPILE_CFLAGS)

media_mw_dsp_platform_src		=	$(MODULE_SRC)
media_mw_dsp_platform_obj		=	$(media_mw_dsp_platform_src:.cpp=.o)

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk

$(SUBDIRS_ALL)	:	$(SHARED_LIB_NAMES)



