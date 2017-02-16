##
## prebuild/imgproc/img_data/arch_s2l/preload_idsp/make.inc
##
## History:
##    2015/01/21 - [Roy Su] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(CONFIG_AMBARELLA_PRELOAD_ADC), y)

LOCAL_PATH	:= $(call my-dir)
PRELOAD_ADC_PATH:=$(LOCAL_PATH)

include $(CLEAR_VARS)
############################

HOST_CC	:= gcc
HOST_ADCFW	:= host_adcfw
HOST_SRC1 	:= $(PRELOAD_ADC_PATH)/$(HOST_ADCFW).c
HOST_CFLAGS 	:= -I$(AMB_TOPDIR)/amboot/include -I$(AMB_BOARD_DIR)

ADCFW_OUT_DIR	:= $(AMB_BOARD_OUT)/prebuild/sys_data
ADCFW_OUT	:= $(ADCFW_OUT_DIR)/adc.bin
IDSPCFG_DIR	:= $(PRELOAD_ADC_PATH)
IDSPCFG_LIST 	:= $(PRELOAD_ADC_PATH)/idspcfg.list
############################

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

