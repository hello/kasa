##
## packages/security/openssl_wrapper/make.inc
##
## History:
##    2015/05/19 - [Zhi He] Created file
##
## Copyright (C) 2015-2025, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_OPENSSL_WRAPPER_LIBRARY), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_CFLAGS	:= -I$(PREBUILD_3RD_PARTY_DIR)/openssl/include
LOCAL_TARGET	:= libopensslwrapper.a
LOCAL_SRCS	:= $(LOCAL_PATH)/openssl_wrapper.cpp

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

