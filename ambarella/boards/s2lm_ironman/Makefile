##
## Makefile
##
## History:
##    2012/06/06 - [Cao Rongrong] Create
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

