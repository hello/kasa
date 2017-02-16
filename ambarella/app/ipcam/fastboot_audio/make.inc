##
## app/ipcam/fastboot_audio/make.inc
##
## History:
##    2014/12/12 - [Jian Liu] Created file
##
## Copyright (C) 2014, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_APP_FASTBOOT_AUDIO), y)

LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

IPCAM_APP_BIN_DIR	= $(FAKEROOT_DIR)/usr/bin

LOCAL_TARGET	:= fastboot_audio
LOCAL_SRCS	:= $(LOCAL_PATH)/fastboot_audio.c $(LOCAL_PATH)/getopt_s.c
LOCAL_CFLAGS	:= -I$(PREBUILD_3RD_PARTY_DIR)/speex/include
LOCAL_LDFLAGS	:= -L$(PREBUILD_3RD_PARTY_DIR)/speex/usr/lib -lspeexdsp

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(IPCAM_APP_BIN_DIR)/
	@cp -dpRf $< $(IPCAM_APP_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
endif

