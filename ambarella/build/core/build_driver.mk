## History:
##    2011/12/15 - [Cao Rongrong] Create
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

$(if $(filter %Kbuild, $(LOCAL_SRCS)),,$(error $(LOCAL_PATH): No Kbuild))

ifeq ($(BUILD_DRIVERS_FROM_BINARY),y)
# If we build drivers from .o files, get .o_shipped files from prebuild dir.
__BIN_SRCS__	:= $(wildcard $(AMB_DRIVERS_BIN_DIR)/$(lastword $(subst /, ,$(LOCAL_PATH)))/arch_$(AMBARELLA_ARCH)/*)
LOCAL_SRCS	:= $(if $(__BIN_SRCS__),$(__BIN_SRCS__),$(LOCAL_SRCS))
endif

KBUILD_DIR	:= $(shell cd $(dir $(filter %Kbuild, $(LOCAL_SRCS))) && pwd)
LOCAL_TARGET	:= $(KBUILD_DIR:$(AMB_DRIVERS_SRCS_DIR)/%=%)
LOCAL_TARGET	:= $(LOCAL_TARGET:$(AMB_DRIVERS_BIN_DIR)/%=%)

LOCAL_OBJS	:= $(patsubst $(AMB_DRIVERS_SRCS_DIR)/%, $(AMB_DRIVERS_OUT_DIR)/%,$(LOCAL_SRCS))
LOCAL_OBJS	:= $(patsubst $(AMB_DRIVERS_BIN_DIR)/%, $(AMB_DRIVERS_OUT_DIR)/%,$(LOCAL_OBJS))

$(AMB_DRIVERS_OUT_DIR)/%: $(AMB_DRIVERS_SRCS_DIR)/%
	@mkdir -p $(dir $@)
	@cp -dpRf $< $@

$(AMB_DRIVERS_OUT_DIR)/%: $(AMB_DRIVERS_BIN_DIR)/%
	@mkdir -p $(dir $@)
	@cp -dpRf $< $@

# This is just a workaround for ugly relationships of some
# private drivers, eg. sensors drivers.
# It should be removed finally when the ugly relationships are fixed.
.PHONY: $(LOCAL_PATH).create_ugly_arch

$(LOCAL_PATH).create_ugly_arch: PRIVATE_UGLY_ARCH:=$(UGLY_ARCH)
$(LOCAL_PATH).create_ugly_arch: PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_PATH).create_ugly_arch:
	@if [ -n "$(PRIVATE_UGLY_ARCH)" ]; then \
		dst_dir=$(subst $(AMB_DRIVERS_SRCS_DIR)/,$(AMB_DRIVERS_OUT_DIR)/,$(PRIVATE_PATH)); \
		mkdir -p $$dst_dir; rm -rf $$dst_dir/arch; \
		ln -fsn $$dst_dir/arch_$(AMBARELLA_ARCH) $$dst_dir/arch; \
	fi

prepare_private_drivers: $(LOCAL_OBJS) $(LOCAL_PATH).create_ugly_arch

AMB_DRIVER_MODULES += $(LOCAL_TARGET)/

