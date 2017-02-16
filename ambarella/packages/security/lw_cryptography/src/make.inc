##
## packages/security/lw_cryptography/src/make.inc
##
## History:
##    2015/08/03 - [Zhi He] Created file
##
## Copyright (C) 2015-2025, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_LW_CRYPTOGRAPHY), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_CFLAGS    := -I$(LOCAL_PATH)/../include
LOCAL_TARGET	:= liblwcryptography.a
LOCAL_SRCS	:= $(LOCAL_PATH)/big_number.c $(LOCAL_PATH)/crypt_utils.c $(LOCAL_PATH)/digest_md5.c $(LOCAL_PATH)/digest_sha256.c $(LOCAL_PATH)/pk_rsa.c $(LOCAL_PATH)/tiny_asn1.c $(LOCAL_PATH)/tiny_oid.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

