##
## oryx/services/rtsp_server/make.inc
##
## History:
##    2015/01/19 - [Dong Shiming] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##


ifeq ($(BUILD_AMBARELLA_ORYX_RTSP_SERVICE), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

RTSP_SVC_NAME = rtsp_svc
###############################################################################
LOCAL_TARGET := $(RTSP_SVC_NAME)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS :=  -I$(LOCAL_PATH)                \
                 -I$(ORYX_DIR)/include          \
                 -I$(ORYX_DIR)/include/ipc      \
                 -I$(ORYX_DIR)/include/utility  \
                 -I$(ORYX_DIR)/include/protocol \
                 -I$(ORYX_DIR)/include/services \
                 -I$(ORYX_DIR)/services/include \
                 -std=c++11 -Werror

LOCAL_LIBS    := libamutil.so          \
                 libamipc.so           \
                 libamservice-frame.so \
                 libamprotocol-rtsp.so

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@mkdir -p $(ORYX_CONF_DIR)/apps/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
