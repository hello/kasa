##
## app/utility/dsp_tool/arch_s2l/make.inc
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


LOCAL_PATH	:= $(call my-dir)

#################################

ifeq ($(BUILD_AMBARELLA_APP_UTILITY_GEN_FASTBOOT_UCODE), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= gen_fastboot_ucode
LOCAL_SRCS	:= $(LOCAL_PATH)/gen_fastboot_ucode.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/usr/sbin
	@cp -dpRf $< $(FAKEROOT_DIR)/usr/sbin
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

