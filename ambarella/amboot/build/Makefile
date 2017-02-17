##
## amboot/build/Makefile
##
## History:
##    2005/01/26 - [Charles Chiou] created file
##    2007/10/23 - [Charles Chiou] migrated to the Linux tree
##    2008/02/19 - [Anthony Ginger] imported to AMBuild
##
## Copyright (c) 2015 Ambarella, Inc.
##
## This file and its contents ("Software") are protected by intellectual
## property rights including, without limitation, U.S. and/or foreign
## copyrights. This Software is also the confidential and proprietary
## information of Ambarella, Inc. and its licensors. You may not use, reproduce,
## disclose, distribute, modify, or otherwise prepare derivative works of this
## Software or any portion thereof except pursuant to a signed license agreement
## or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
## In the absence of such an agreement, you agree to promptly notify and return
## this Software to Ambarella, Inc.
##
## THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
## INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
## MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
## IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
## INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
## (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
## LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
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

