##
## oryx/services/audio_service/make.inc
##
## History:
##    2014/10/21 - [Louis Sun] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

###############################################################################
LOCAL_TARGET := test_oryx_adevice

LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include              \
                 -I$(ORYX_DIR)/include/utility      \
                 -I$(ORYX_DIR)/include/audio/device \
                 -I$(LOCAL_PATH)                    \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib                        \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/xz/usr/lib           \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/json-c/usr/lib       \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libattr/usr/lib      \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libcap/usr/lib       \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libsndfile/usr/lib   \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libgcrypt/usr/lib    \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libgpg-error/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/systemd/usr/lib      \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/pulseaudio/usr/lib   \
                 -lpthread

LOCAL_LIBS    := libamutil.so libamaudio-device.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

