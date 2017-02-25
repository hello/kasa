## History:
##    2012/09/02 - [Cao Rongrong] Create
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

KBUILD_DIR	:= $(shell cd $(dir $(filter %Kbuild, $(LOCAL_SRCS))) && pwd)
LOCAL_TARGET	:= $(KBUILD_DIR:$(EXT_DRIVERS_SRCS_DIR)/%=%)

LOCAL_OBJS	:= $(patsubst $(EXT_DRIVERS_SRCS_DIR)/%, $(EXT_DRIVERS_OUT_DIR)/%,$(LOCAL_SRCS))

$(EXT_DRIVERS_OUT_DIR)/%: $(EXT_DRIVERS_SRCS_DIR)/%
	@mkdir -p $(dir $@)
	@cp -dpRf $< $@

prepare_external_drivers: $(LOCAL_OBJS)

EXT_DRIVER_MODULES += $(LOCAL_TARGET)/

