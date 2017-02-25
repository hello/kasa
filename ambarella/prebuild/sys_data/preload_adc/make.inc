################################################################################
 #  prebuild/sys_data/preload_adc/make.inc
 #
 # History:
 #    2016/01/15 - [Tao Wu] Created file
 #
 # Copyright (c) 2016 Ambarella, Inc.
 #
 # This file and its contents ( "Software" ) are protected by intellectual
 # property rights including, without limitation, U.S. and/or foreign
 # copyrights. This Software is also the confidential and proprietary
 # information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 # disclose, distribute, modify, or otherwise prepare derivative works of this
 # Software or any portion thereof except pursuant to a signed license agreement
 # or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 # In the absence of such an agreement, you agree to promptly notify and return
 # this Software to Ambarella, Inc.
 #
 # THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 # INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 # MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 # IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 # INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 # (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 # LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 # INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 # CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 # ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 # POSSIBILITY OF SUCH DAMAGE.
 #
################################################################################

ifeq ($(CONFIG_AMBARELLA_PRELOAD_ADC), y)

LOCAL_PATH	:= $(call my-dir)
PRELOAD_ADC_PATH:=$(LOCAL_PATH)

include $(CLEAR_VARS)
############################

HOST_CC	:= gcc
HOST_ADCFW	:= host_adcfw
HOST_SRC1	:= $(PRELOAD_ADC_PATH)/arch_$(AMBARELLA_ARCH)/$(HOST_ADCFW).c
HOST_CFLAGS	:= -I$(AMB_TOPDIR)/amboot/include -I$(AMB_BOARD_DIR) \
				-I$(PRELOAD_ADC_PATH)/arch_$(AMBARELLA_ARCH)

ADCFW_OUT_DIR	:= $(AMB_BOARD_OUT)/prebuild/sys_data
ADCFW_OUT	:= $(ADCFW_OUT_DIR)/adc.bin
IDSPCFG_DIR	:= $(PRELOAD_ADC_PATH)/arch_$(AMBARELLA_ARCH)

include $(IDSPCFG_DIR)/make.inc

ifndef IDSPCFG_LIST
$(error IDSPCFG_LIST is not defined. Please specify your AMBARELLA_ARCH.)
endif

LOCAL_TARGET	:= preload_adc

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET):
	@mkdir -p $(ADCFW_OUT_DIR)
	@$(HOST_CC) $(HOST_SRC1) $(HOST_CFLAGS) -o $(ADCFW_OUT_DIR)/$(HOST_ADCFW)
	@$(ADCFW_OUT_DIR)/$(HOST_ADCFW) $(ADCFW_OUT) $(IDSPCFG_DIR) $(IDSPCFG_LIST)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif