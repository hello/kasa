##
## Makefile
##
## History:
##    2012/06/06 - [Cao Rongrong] Create
##
## Copyright (C) 2011-2015, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

.PHONY: all
all:

AMB_TOPDIR := $(shell pwd)/../..

include $(AMB_TOPDIR)/build/core/definition.mk
include $(AMB_TOPDIR)/build/core/configure.mk
include $(AMB_TOPDIR)/build/core/menuconfig.mk

# Find all of make.inc
MAKE.INC := $(shell $(AMB_TOPDIR)/build/bin/findleaves.py --prune=.git \
		--prune=out --prune=build --prune=scripts \
		$(AMB_TOPDIR) make.inc)

include $(MAKE.INC)
include $(AMB_TOPDIR)/build/core/dependency.mk

all: $(ALL_TARGETS)
	@$(MAKE) $(AMBA_MAKE_PARA) build_fsimage
	@$(MAKE) $(AMBA_MAKE_PARA) amboot
	@echo "Build Done"

.PHONY: clean distclean

clean:
	@rm -rf $(AMB_BOARD_OUT)

distclean: clean
	@rm -rf $(dir $(DOT_CONFIG))/.config* $(dir $(DOT_CONFIG))/config.h

