##
## packages/security/cryptochip_library/make.inc
##
## History:
##    2015/09/22 - [Zhi He] Created file
##
## Copyright (C) 2015-2025, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_CRYPTOCHIP_LIBRARY), y)
ifeq ($(BUILD_AMBARELLA_CRYPTOCHIP_LIBRARY_FROM_SOURCE), y)
LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/include
LOCAL_TARGET	:= libcryptochip.a
LOCAL_SRCS	:= $(LOCAL_PATH)/library_atsha204.c $(LOCAL_PATH)/cryptochip_library_interface.c $(LOCAL_PATH)/sha256.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif
endif

