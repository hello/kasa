##
## app/utility/mptool/make.inc
##
## History:
##   2015-3-18 - [longli] created file
##
## Copyright (C) 2008-2015, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

ifeq ($(BUILD_AMBARELLA_ORYX_MPTOOL), y)
LOCAL_PATH	:= $(call my-dir)

###

include $(CLEAR_VARS)

LOCAL_BOARD	:= $(shell grep CONFIG_BSP_BOARD $(DOT_CONFIG) | \
        grep -v '^\# CONFIG_BSP_BOARD' | sed -e s/^CONFIG_BSP_BOARD_// | \
        sed -e s/=y//)
LOCAL_BSP_DIR	:= $(AMB_TOPDIR)/boards/$(shell echo $(LOCAL_BOARD) | \
        tr [:upper:] [:lower:])/bsp

LOCAL_TARGET    :=  mptool
LOCAL_SRCS      :=  $(wildcard $(LOCAL_PATH)/src/*.c)
LOCAL_CFLAGS :=  -I$(AMB_TOPDIR)/include \
                 -I$(AMB_TOPDIR)/include/arch_$(AMBARELLA_ARCH) \
                 -I$(LOCAL_PATH)/include \
                 -I$(LOCAL_BSP_DIR) \
                 -Werror

LOCAL_LDFLAGS   := -lpthread -lm
LOCAL_LIBS  := libamp.so libamdev.so
ifeq ($(BUILD_AMBARELLA_PACKAGES_PROPRIETARY_SOURCES), y)
ifeq ($(CONFIG_IMGPROC_PACKAGE_DYNAMIC_LIB), y)
LOCAL_LIBS	+= libimg_algo_s2l.so
else
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2l
endif
else
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2l
endif


include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/usr/local/bin
	@cp -dpRf $< $(FAKEROOT_DIR)/usr/local/bin
	@cp -dpRf $(PRIVATE_PATH)/data/* $(FAKEROOT_DIR)/usr/local/bin
	@echo "Build $@ Done."


$(call add-target-into-build, $(LOCAL_TARGET))

endif
