##
## amboot/build/Makefile
##
## History:
##    2005/01/26 - [Charles Chiou] created file
##    2007/10/23 - [Charles Chiou] migrated to the Linux tree
##    2008/02/19 - [Anthony Ginger] imported to AMBuild
##
## Copyright (C) 2004-2007, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

AMBABUILD_TOPDIR	:= $(AMB_TOPDIR)

include $(DOT_CONFIG)

HOST_MACHINE		:= $(shell uname -m)

TOP			:= $(AMBABUILD_TOPDIR)/amboot
SEDTOP			:= $(subst /,\\/,$(TOP))
BUILD_DIR		:= $(dir $(DOT_CONFIG))

#CROSS			:= $(TOOLCHAIN_PATH)/$(CROSS_COMPILE)
CROSS			:= $(ARM_ELF_TOOLCHAIN_DIR)/bin/$(ARM_ELF_CROSS_COMPILE)
AR			:= ar
CC			:= gcc
CPP			:= gcc -E
LD			:= ld
NM			:= nm
OBJCOPY			:= objcopy
RANLIB			:= ranlib
STRIP			:= strip
SED			:= sed
GCCDEP			:= gcc -M -P

MAKE_VERBOSE		= yes

.PHONY: all clean dep
.SUFFIX: .elf .bin .fw .o

ifneq (.dep,$(wildcard .dep))
default: .dep fwlist
	$(MAKE)
else
ifneq (.fwlist,$(wildcard .fwlist))
default: .fwlist
else
default: all
include .dep
endif
endif

include $(TOP)/config/Makefile.archbsp
include $(TOP)/config/Makefile.amboot
include $(TOP)/config/Makefile.devfw

TARGETS = \
	$(BST_DEBUG_BINS)		\
	bld_debug.bin		\
	$(BST_RELEASE_BINS)		\
	bld_release.bin

ifeq ($(BUILD_DEVFW),y)
TARGETS += $(MEMFWPROG)
endif

all: $(TARGETS)

$(TARGETS): Makefile $(DOT_CONFIG)

clean::
	rm -f .dep *.o *.elf *.bin *.idb *.stf *.map *.lds *~ *.piggy *.fw header_*.S devfw_*.lds memfwprog_*.lds *.log *.stackdump

