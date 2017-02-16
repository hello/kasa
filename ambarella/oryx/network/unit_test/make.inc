##
## camera/test/network/make.inc
##
## History:
##    2015-1-12 - [Tao Wu] created file
##
## Copyright (C) 2012-2015, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_TARGET := test_network_manager
LOCAL_SRCS   := $(LOCAL_PATH)/test_network_manager.cpp
LOCAL_LIBS   := libamutil.so libamnetwork.so libamnetwork-dev.so
LOCAL_CFLAGS := -I$(ORYX_DIR)/include\
                -I$(ORYX_DIR)/include/utility \
                -I$(ORYX_DIR)/include/network

LOCAL_LDFLAGS:= -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/networkmanager/usr/lib \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/expat/usr/lib        \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/dbus/usr/lib         \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib         \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/ncurses/usr/lib      \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/glib2/usr/lib        \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libffi/usr/lib       \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/util-linux/usr/lib   \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/gnutls/usr/lib       \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libgcrypt/usr/lib    \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libgpg-error/usr/lib \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/nettle/usr/lib       \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/gmp/usr/lib          \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libtasn1/usr/lib     \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/udev/usr/lib         \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/bzip2/usr/lib        \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libselinux/lib       \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libsepol/lib         \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/pam/usr/lib          \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libcap/usr/lib       \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libattr/usr/lib      \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/pcre/usr/lib         \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/readline/usr/lib     \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/dbus-glib/usr/lib    \
                -lpthread -lstdc++ -ldl -lrt -lresolv

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

