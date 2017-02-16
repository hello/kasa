##
## packages/textinsert/make.inc
##
## History:
##    2012/06/01 - [Cao Rongrong] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_TEXTINSERT_PACKAGE), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= libtextinsert.a
LOCAL_SRCS	:= $(wildcard $(LOCAL_PATH)/*.c)
LOCAL_CFLAGS	:= -Wall -O3 -I. \
			-I$(PREBUILD_3RD_PARTY_DIR)/freetype/include \
			-I$(PREBUILD_3RD_PARTY_DIR)/freetype/include/freetype2

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

