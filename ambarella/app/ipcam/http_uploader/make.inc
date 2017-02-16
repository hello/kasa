##
## app/ipcam/http_uploader/make.inc
##
## History:
##    2014/12/03 - [Chu Chen] Created file
##
## Copyright (C) 2014, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_APP_HTTP_UPLOADER), y)

LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

IPCAM_APP_BIN_DIR	= $(FAKEROOT_DIR)/usr/bin

LOCAL_TARGET	:= http_uploader
LOCAL_SRCS	:= $(LOCAL_PATH)/http_uploader.c
LOCAL_CFLAGS	:= -I$(PREBUILD_3RD_PARTY_DIR)/libcurl/include
LOCAL_LDFLAGS	:= -L$(PREBUILD_3RD_PARTY_DIR)/libcurl/usr/lib -lcurl \
                -L$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib -lz \
                -L$(PREBUILD_3RD_PARTY_DIR)/openssl/usr/lib -lssl -lcrypto \
                -L$(PREBUILD_3RD_PARTY_DIR)/libssh2/usr/lib -lssh2 \
                -L$(PREBUILD_3RD_PARTY_DIR)/libidn/usr/lib -lidn

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(IPCAM_APP_BIN_DIR)/
	@cp -dpRf $< $(IPCAM_APP_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
endif

