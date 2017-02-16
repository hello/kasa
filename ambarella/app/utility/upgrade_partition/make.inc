##
## app/utility/upgrade_partition/make.inc
##
## History:
##    2014/06/03 - [Ken He] Created file
##
## Copyright (C) 2014-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBAREALLA_APP_UTILITY_UPGRADE), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= upgrade_partition
ifeq ($(BUILD_UPGRADE_FOR_3_10), y)
LOCAL_SRCS		:= $(wildcard $(LOCAL_PATH)/libfdt/*.c)
LOCAL_SRCS		+= $(LOCAL_PATH)/upgrade_partition.c
endif
ifeq ($(BUILD_UPGRADE_FOR_3_8), y)
LOCAL_SRCS		:= $(LOCAL_PATH)/upgrade_partition_v38.c
endif
LOCAL_LDFLAG 	:= -O3 -Wall

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/usr/local/bin
	@cp -dpRf $< $(FAKEROOT_DIR)/usr/local/bin
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
