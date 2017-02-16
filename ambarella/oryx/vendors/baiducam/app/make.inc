##
## oryx/vendors/baiducam/app/make.inc
##
## History:
##    2015/3/20 - [Shupeng Ren] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH   := $(call my-dir)

BAIDUCAM_DIR := $(ORYX_DIR)/vendors/baiducam

###############################################################################
## baiducam
include $(CLEAR_VARS)
DATA_EXPORT_TARGET := baiducam
LOCAL_TARGET       := $(DATA_EXPORT_TARGET)
LOCAL_SRCS         := $(LOCAL_PATH)/am_bls_rtmp.cpp \
                      $(LOCAL_PATH)/am_baiducam.cpp \
                      $(LOCAL_PATH)/main.cpp

LOCAL_CFLAGS       := -I$(ORYX_DIR)/include                                \
                      -I$(ORYX_DIR)/include/stream                         \
                      -I$(ORYX_DIR)/stream/include/record                  \
                      -I$(ORYX_DIR)/stream/include/common/media            \
                      -I$(ORYX_DIR)/include/utility                        \
                      -I$(BAIDUCAM_DIR)/include                            \
                      -I$(PREBUILD_3RD_PARTY_DIR)/json-c/include/json      \
                      -std=c++11 -Werror

LOCAL_LDFLAGS  := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib \
                  -L$(PREBUILD_3RD_PARTY_DIR)/json-c/usr/lib -ljson-c \
                  -lpthread

LOCAL_LIBS := libamexport-client.so \
              libamutil.so \
              libamosal.so \
              librtmp-bls.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###############################################################################
