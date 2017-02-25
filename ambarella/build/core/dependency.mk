## History:
##    2012/06/09 - [Cao Rongrong] Create
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

$(ALL_TARGETS): $(DOT_CONFIG)

# Add clean target
.PHONY: clean_fakeroot clean_ut clean_linux clean_private_drivers
.PHONY: clean_dtc clean_amboot

clean_fakeroot:
	@rm -rf $(FAKEROOT_DIR)
	@rm -rf $(ROOTFS_OUT_DIR)

clean_ut:
	@rm -rf $(AMB_BOARD_OUT)/unit_test

clean_app:
	@rm -rf $(AMB_BOARD_OUT)/app

clean_mw:
	@rm -rf $(AMB_BOARD_OUT)/mw

clean_linux:
	@rm -rf $(LINUX_OUT_DIR)

clean_private_drivers:
	@rm -rf $(AMB_BOARD_OUT)/kernel/private

clean_external_drivers:
	@rm -rf $(AMB_BOARD_OUT)/kernel/external

clean_dtc:
	@$(MAKE) $(AMBA_MAKE_PARA) -C $(AMB_TOPDIR)/build/dtc clean

clean_amboot: clean_dtc
	@rm -rf $(AMB_BOARD_OUT)/amboot

