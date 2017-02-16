##
## packages/security/digital_signature/make.inc
##
## History:
##    2015/04/03 - [Zhi He] Created file
##
## Copyright (C) 2015-2025, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_DIGITAL_SIGNATURE), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_CFLAGS	:= -I$(PREBUILD_3RD_PARTY_DIR)/openssl/include -I$(AMB_TOPDIR)/packages/security/lw_cryptography/include
LOCAL_TARGET	:= libdigisign.a
LOCAL_SRCS	:= 

ifeq ($(CONFIG_DIGITAL_SIGNATURE_LIBRARY_NATIVE), y)
LOCAL_SRCS	+= $(LOCAL_PATH)/digital_signature_native.cpp
endif

ifeq ($(CONFIG_DIGITAL_SIGNATURE_LIBRARY_OPENSSL), y)
LOCAL_SRCS	+= $(LOCAL_PATH)/digital_signature_openssl.cpp
endif

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

