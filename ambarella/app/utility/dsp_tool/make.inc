##
## app/utility/dsp_tool/make.inc
##
## History:
##    2014/08/04 - [Jian Tang] Created file
##
## Copyright (C) 2014-2018, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_APP_UTILITY_DSP), y)

LOCAL_PATH	:= $(call my-dir)

#################################

ifeq ($(BUILD_AMBARELLA_APP_UTILITY_LOAD_UCODE), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= load_ucode
LOCAL_SRCS	:= $(LOCAL_PATH)/load_ucode.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/usr/sbin
	@cp -dpRf $< $(FAKEROOT_DIR)/usr/sbin
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif	## BUILD_AMBARELLA_APP_UTILITY_LOAD_UCODE

#################################

ifeq ($(BUILD_AMBARELLA_APP_UTILITY_DSPLOG), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= dsplog_cap
LOCAL_SRCS	:= $(LOCAL_PATH)/dsplog_cap.c  $(LOCAL_PATH)/arch_$(AMBARELLA_ARCH)/dsplog_utils.c
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/data_transfer
LOCAL_LIBS	:= libdatatx.a
LOCAL_LDFLAGS	:= -lpthread

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(APP_INSTALL_PATH)/
	@cp -dpRf $< $(APP_INSTALL_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif	## BUILD_AMBARELLA_APP_UTILITY_DSPLOG

#################################

include $(LOCAL_PATH)/arch_$(AMBARELLA_ARCH)/make.inc

endif	## BUILD_AMBARELLA_APP_UTILITY_DSP

