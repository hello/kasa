##
## unit_test/linux/socketcrypto/make.inc
##
## History:
##    2012/05/31 - [Cao Rongrong] Created file
##
## Copyright (C) 2011-2015, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_CRYPTO_SOCKET), y)

LOCAL_PATH	:= $(call my-dir)


###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_crypto_socket
LOCAL_SRCS	:= $(LOCAL_PATH)/socket_crypto.c $(LOCAL_PATH)/md5_sha1.c \
		   $(LOCAL_PATH)/ecb_aes.c $(LOCAL_PATH)/ecb_des.c $(LOCAL_PATH)/cbc_aes.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

