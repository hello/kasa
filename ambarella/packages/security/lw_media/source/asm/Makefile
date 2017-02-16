##
## $(MODULE_NAME_TAG)/source/asm/Makefile
##
## History:
##    2015/09/16 - [Zhi He] Create File
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
##include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/extern_lib.mk

MODULE_SRC = $(wildcard *.S)
MODULE_OBJ = $(MODULE_SRC:.S=.o)

lw_media_asm_src		=	$(MODULE_SRC)
lw_media_asm_obj		=	$(lw_media_asm_src:.S=.o)

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk



