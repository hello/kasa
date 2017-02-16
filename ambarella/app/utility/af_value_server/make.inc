##
## app/mptool/make.inc
##
## History:
##    2014/03/06 - [HuaiShun Hu] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_APP_UTILITY_AF_VALUE_SERVER), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET    :=  af_value_server
LOCAL_SRCS      :=  $(wildcard $(LOCAL_PATH)/*.cpp)
MODULE_INCLUDES :=  -I$(AMB_TOPDIR) \
                    -I$(AMB_TOPDIR)/build/include \
                    -I$(AMB_TOPDIR)/build/include/arch \
                    -I$(AMB_TOPDIR)/include

LOCAL_LDFLAGS   :=

LOCAL_CFLAGS    :=	$(MODULE_INCLUDES) -g -O3 -Wall

INSTALL_BIN_DIR  = $(FAKEROOT_DIR)/usr/bin

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH := $(LOCAL_PATH)
$(LOCAL_TARGET): LENS_FOCUS_SCRIPT := lens_focus.sh
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(INSTALL_BIN_DIR)/
	@chmod +x $(LENS_FOCUS_SCRIPT:%=$(PRIVATE_PATH)/%)
	@cp $(LENS_FOCUS_SCRIPT:%=$(PRIVATE_PATH)/%) $(INSTALL_BIN_DIR)/
	@cp -dpRf $< $(INSTALL_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
