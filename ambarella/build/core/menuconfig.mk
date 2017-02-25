##
## History:
##    2011/12/13 - [Cao Rongrong] Create
##    2013/03/26 - [Jian Tang] Modified
##
## Copyright (C) 2015 Ambarella, Inc.
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

$(DOT_CONFIG):
	@echo "System not configured!"
	@echo "Please run 'make menuconfig'"
	@exit 1

$(AMB_TOPDIR)/build/kconfig/conf: $(AMB_TOPDIR)/build/kconfig/Makefile
	@$(MAKE) $(AMBA_MAKE_PARA) -C $(AMB_TOPDIR)/build/kconfig conf

$(AMB_TOPDIR)/build/kconfig/mconf: $(AMB_TOPDIR)/build/kconfig/Makefile
	@$(MAKE) $(AMBA_MAKE_PARA) -C $(AMB_TOPDIR)/build/kconfig ncurses conf mconf

.PHONY: menuconfig show_configs

show_configs:
	@ls $(AMB_BOARD_DIR)/config/

menuconfig: $(AMB_TOPDIR)/build/kconfig/mconf
	@AMBABUILD_SRCTREE=$(AMB_TOPDIR) $(AMB_TOPDIR)/build/kconfig/mconf $(AMB_TOPDIR)/AmbaConfig

%_config: $(AMB_BOARD_DIR)/config/%_config prepare_public_linux $(AMB_TOPDIR)/build/kconfig/conf
	@cp -f $< $(DOT_CONFIG)
	@AMBABUILD_SRCTREE=$(AMB_TOPDIR) $(AMB_TOPDIR)/build/kconfig/conf --defconfig=$< $(AMB_TOPDIR)/AmbaConfig

.PHONY: clean_kconfig
clean_kconfig:
	@$(MAKE) $(AMBA_MAKE_PARA) -C $(AMB_TOPDIR)/build/kconfig clean

.PHONY: create_boards
create_boards: $(AMB_TOPDIR)/build/bin/create_board_mkcfg
ifeq ($(shell [ -d $(AMB_TOPDIR)/boards ] && echo y), y)
	@$(AMB_TOPDIR)/build/bin/create_board_mkcfg $(AMB_TOPDIR)/boards;
endif

.PHONY: sync_build_mkcfg
sync_build_mkcfg: create_boards $(AMB_TOPDIR)/build/bin/create_root_mkcfg
	@$(AMB_TOPDIR)/build/bin/create_mkcfg $(AMB_TOPDIR)/build/cfg $(AMB_TOPDIR)
	@$(AMB_TOPDIR)/build/bin/create_root_mkcfg $(AMB_TOPDIR)



.PHONY: prepare_public_linux
prepare_public_linux:
ifneq ($(shell [ -L ../../kernel/linux ] && echo y), y)
	$(AMBA_MAKEFILE_V)echo "start to extract linux"
	tar xjvf ../../kernel/linux-3.10.tar.bz2 -C ../../kernel/.
	ln -s linux-3.10 ../../kernel/linux
	patch -d ../../kernel/linux -p1 < ../../kernel/linux-3.10-ambarella.patch
	$(AMBA_MAKEFILE_V)echo "linux patched"
endif