##
## prebuild/third-party/gdbserver-eglibc/make.inc
##
## History:
##    2012/05/30 - [Cao Rongrong] Created file
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

ifeq ($(CONFIG_AMBARELLA_GDBSERVER_SUPPORT), y)

.PHONY: prebuild_gdbserver

prebuild_gdbserver:
	@mkdir -p $(FAKEROOT_DIR)/usr/bin
ifeq ($(CONFIG_CPU_ARM1136JS), y)
	@cp -dpRf $(GDBSERVER_DIR_ARMV6)/gdbserver $(FAKEROOT_DIR)/usr/bin
endif
ifeq ($(CONFIG_CPU_CORTEXA9), y)
	@cp -dpRf $(GDBSERVER_DIR_ARMV7_SF)/gdbserver $(FAKEROOT_DIR)/usr/bin
endif
ifeq ($(CONFIG_CPU_CORTEXA9_HF), y)
	@cp -dpRf $(GDBSERVER_DIR_ARMV7)/gdbserver $(FAKEROOT_DIR)/usr/bin
endif
ifeq ($(CONFIG_CPU_CORTEXA53), y)
	@cp -dpRf $(GDBSERVER_DIR_AARCH64)/gdbserver $(FAKEROOT_DIR)/usr/bin
endif
	@echo "Copy $@ Done"

$(call add-target-into-build, prebuild_gdbserver)

endif
