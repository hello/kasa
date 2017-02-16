##
## $(MODULE_NAME_TAG)/media_mw/src/framework/Makefile
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

SUBDIRS = streaming cloud

MODULE_INC = -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/media_mw/src/internal_include 
MODULE_SRC = $(wildcard *.cpp)
MODULE_OBJ = $(MODULE_SRC:.cpp=.o)

media_mw_framework_src		=	$(MODULE_SRC)
media_mw_framework_obj		=	$(media_mw_framework_src:.cpp=.o)

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk



