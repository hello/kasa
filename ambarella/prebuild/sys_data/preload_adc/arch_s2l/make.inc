##
## prebuild/sys_data/preload_adc/arch_s2l/make.inc
##
## History:
##    2015/01/21 - [Roy Su] Created file
##
## Copyright (c) 2016 Ambarella, Inc.
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

ifeq ($(CONFIG_BSP_BOARD_S2LM_ELEKTRA), y)

ifeq ($(CONFIG_S2LMELEKTRA_ENABLE_ADVANCED_ISO_MODE),y)
IDSPCFG_LIST	:= $(IDSPCFG_DIR)/imx322_idspcfg_mode4.list
else
IDSPCFG_LIST	:= $(IDSPCFG_DIR)/imx322_idspcfg.list
endif

else ifeq ($(CONFIG_BSP_BOARD_S2LM_KIWI), y)

ifeq ($(CONFIG_S2LMKIWI_ENABLE_ADVANCED_ISO_MODE),y)
ifeq ($(CONFIG_ISO_TYPE_MIDDLE),y)
IDSPCFG_LIST	:= $(IDSPCFG_DIR)/ov4689_idspcfg_mode4_mliso.list
else ifeq ($(CONFIG_ISO_TYPE_ADVANCED),y)
IDSPCFG_LIST	:= $(IDSPCFG_DIR)/ov4689_idspcfg_mode4_aliso.list
endif
else
IDSPCFG_LIST	:= $(IDSPCFG_DIR)/ov4689_idspcfg_mode0.list
endif

else

IDSPCFG_LIST	:= $(IDSPCFG_DIR)/ov4689_idspcfg_mode0.list

endif
