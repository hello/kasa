##
## /S2L/oryx/upgrade/unit_test/make.inc
##
## History:
##   2015-1-12 - [longli] created file
##
## Copyright (C) 2008-2015, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

ifeq ($(BUILD_AMBARELLA_ORYX_UPGRADE_UNIT_TEST), y)

LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

###############################################################################
LOCAL_TARGET := test_upgrade
LOCAL_SRCS   := $(LOCAL_PATH)/test_upgrade.cpp
LOCAL_CFLAGS := -I$(LOCAL_PATH)\
                -I$(ORYX_DIR)/include \
                -I$(ORYX_DIR)/include/upgrade \
                -I$(ORYX_DIR)/include/utility \
                -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libcurl/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/openssl/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libssh2/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libidn/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/7zdec/lib

LOCAL_LIBS    := libamupgrade.so libamutil.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif