##
## app/ipcam/fastboot_smart3a/make.inc
##
## History:
##    2015/01/21 - [Roy Su] Created file
##
## Copyright (C) 2014-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_APP_FASTBOOT_SMART3A), y)

LOCAL_PATH	:= $(call my-dir)

##
include $(CLEAR_VARS)

LOCAL_TARGET	:= smart3a_adc
LOCAL_SRCS	:= $(LOCAL_PATH)/smart3a_adc.c
LOCAL_LDFLAG 	:= -O3 -Wall

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/usr/local/bin
	@cp -dpRf $< $(FAKEROOT_DIR)/usr/local/bin
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
