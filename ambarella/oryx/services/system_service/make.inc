##
## oryx/services/system_service/make.inc
##
## History:
##    2014/09/09 - [Louis Sun] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##


ifeq ($(BUILD_AMBARELLA_ORYX_SERVICE_SYSTEM), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

###############################################################################
LOCAL_TARGET := sys_svc
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include \
                 -I$(ORYX_DIR)/include/utility \
                 -I$(ORYX_DIR)/include/ipc \
                 -I$(ORYX_DIR)/include/services \
                 -I$(ORYX_DIR)/services/include \
                 -I$(ORYX_DIR)/include/upgrade \
                 -I$(ORYX_DIR)/include/led \
                 -I$(LOCAL_PATH) \
                 -std=c++11 -pthread -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/lua/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libcurl/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/openssl/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libssh2/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libidn/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib \
                 -lrt

LOCAL_LIBS    := libamutil.so \
                 libamipc.so \
                 libamconfig.so \
                 libamupgrade.so \
                 libamservice-frame.so \
                 libamled.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))



endif
