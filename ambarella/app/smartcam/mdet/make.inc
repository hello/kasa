##
## unit_test/common.mk
##
## History:
##    2014/03/28 - [Zhenwu Xue] Create
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_SMARTCAM_APP_MDET), y)
LOCAL_PATH	:= $(call my-dir)
UTILS_PATH	:= $(LOCAL_PATH)/../utils

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_mdet
LOCAL_SRCS	:= $(LOCAL_PATH)/test_mdet.c $(UTILS_PATH)/arch_$(AMBARELLA_ARCH)/iav.c $(UTILS_PATH)/arch_$(AMBARELLA_ARCH)/fb.c
LOCAL_LDFLAGS	:= -lm
ifeq ($(BUILD_AMBARELLA_PACKAGES_PROPRIETARY_SOURCES), y)
LOCAL_LIBS	:= libmdet.so
else
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/packages/smartcam/mdet_lib/lib -lmdet
endif
LOCAL_CFLAGS	+= -I$(AMB_TOPDIR)/packages/smartcam/mdet_lib/include -I$(UTILS_PATH)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(APP_INSTALL_PATH)/
ifneq ($(BUILD_AMBARELLA_PACKAGES_PROPRIETARY_SOURCES), y)
	@cp -dpRf $(AMB_TOPDIR)/packages/smartcam/mdet_lib/lib/* $(FAKEROOT_DIR)/usr/lib/
endif
	@cp -dpRf $< $(APP_INSTALL_PATH)/
	@chmod +x $(APP_INSTALL_PATH)/$@
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif


