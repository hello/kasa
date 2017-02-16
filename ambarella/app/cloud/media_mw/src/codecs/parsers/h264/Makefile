##
## $(MODULE_NAME_TAG)/media_mw/src/codecs/parsers/h264/Makefile
##
## History:
##    2014/02/24 - [Zhi He] Create File
##
## Copyright (C) 2014 - 2024, the Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of the Ambarella, Inc.
##

default: all

include ../../../../../build/core/linux/pre_requirement.mk

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/config_linux.mk

MODULE_INC = -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/media_mw/src/internal_include -I$(BUILDSYSTEM_DIR)/build/include
MODULE_SRC = $(wildcard *.cpp)
MODULE_OBJ = $(MODULE_SRC:.cpp=.o)

media_mw_codecs_parsers_h264_src		=	$(MODULE_SRC)
media_mw_codecs_parsers_h264_obj		=	$(media_mw_codecs_parsers_h264_src:.cpp=.o)

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk



