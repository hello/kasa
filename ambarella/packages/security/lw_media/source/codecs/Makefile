##
## $(MODULE_NAME_TAG)/source/codecs/Makefile
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

include ../../build/core/linux/pre_requirement.mk

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/config_linux.mk

SUBDIRS	:=
SUBDIRS	+= parsers

MODULE_SRC = $(wildcard *.cpp)
MODULE_OBJ = $(MODULE_SRC:.cpp=.o)
MODULE_DEF = $(V_COMPILE_CFLAGS)

lw_media_codecs_src		=	$(MODULE_SRC)
lw_media_codecs_obj		=	$(lw_media_codecs_src:.cpp=.o)

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk

$(SUBDIRS_ALL)	:	$(SHARED_LIB_NAMES)



