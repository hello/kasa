##
## oryx/services/media_service/make.inc
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


ifeq ($(BUILD_AMBARELLA_ORYX_SERVICE_MEDIA), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

###############################################################################
MEDIA_SVC_NAME =media_svc
LOCAL_TARGET := $(MEDIA_SVC_NAME)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/am_media_service_main.cpp     \
                           $(LOCAL_PATH)/am_media_service_instance.cpp \
                           $(LOCAL_PATH)/am_media_service_msg_action.cpp)
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include                    \
                 -I$(ORYX_DIR)/include/audio              \
                 -I$(ORYX_DIR)/include/utility            \
                 -I$(ORYX_DIR)/include/ipc                \
                 -I$(ORYX_DIR)/include/services           \
                 -I$(ORYX_DIR)/include/services/commands  \
                 -I$(ORYX_DIR)/services/include           \
                 -I$(ORYX_DIR)/include/stream             \
                 -I$(LOCAL_PATH)                          \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib
LOCAL_LIBS    := libamutil.so            \
                 libamosal.so            \
                 libamipc.so             \
                 libamstream-amf.so      \
                 libamstream-record.so   \
                 libamstream-playback.so \
                 libamservice-frame.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

### libammedia-data-struct.so
include $(CLEAR_VARS)

MEDIA_DATA_STRUCT = libammedia-data-struct.so

LOCAL_TARGET := $(MEDIA_DATA_STRUCT)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/am_media_service_data_structure.cpp)

LOCAL_CFLAGS := -I$(ORYX_DIR)/include                 \
                -I$(ORYX_DIR)/include/utility         \
                -I$(ORYX_DIR)/include/services        \
                -I$(ORYX_DIR)/include/service/commands\
                -I$(ORYX_DIR)/include/ipc             \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so

MEDIA_DATA_STRUCT_NAME   = $(MEDIA_DATA_STRUCT)
MEDIA_DATA_STRUCT_TARGET = $(MEDIA_DATA_STRUCT)
LOCAL_SO_NAME := $(MEDIA_DATA_STRUCT)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(MEDIA_DATA_STRUCT_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
