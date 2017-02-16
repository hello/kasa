##
## /S2l/oryx/stream/record/muxers/muxer-export/make.inc
##
## History:
##   2015-1-14 - [ypchang] created file
##
## Copyright (C) 2008-2015, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

LOCAL_PATH := $(call my-dir)

### muxer-export.so
include $(CLEAR_VARS)

MUXER_EXPORT = muxer-export.so

LOCAL_TARGET   := $(MUXER_EXPORT)
LOCAL_SRCS_UDS := $(LOCAL_PATH)/unixdomainsocket/am_muxer_export_uds.cpp

LOCAL_SRCS     := $(LOCAL_PATH)/am_muxer_export_interface.cpp
LOCAL_SRCS     += $(LOCAL_SRCS_UDS)

LOCAL_CFLAGS := -I$(ORYX_DIR)/include                     \
                -I$(ORYX_DIR)/include/stream              \
                -I$(ORYX_DIR)/include/utility             \
                -I$(ORYX_DIR)/include/audio               \
                -I$(ORYX_DIR)/stream/include/amf          \
                -I$(ORYX_DIR)/stream/include/common       \
                -I$(ORYX_DIR)/stream/include/stream       \
                -I$(ORYX_DIR)/stream/include/record       \
                -I$(ORYX_DIR)/stream/include/common/media \
                -std=c++11 -Werror

ifeq ($(BUILD_AMBARELLA_ORYX_VIDEO_NEW), y)
LOCAL_CFLAGS += -I$(ORYX_DIR)/include/video_new
else
LOCAL_CFLAGS += -I$(ORYX_DIR)/include/video
endif

LOCAL_LIBS := libamutil.so \
              libamosal.so

MUXER_EXPORT_NAME   = $(MUXER_EXPORT)
MUXER_EXPORT_TARGET = $(MUXER_EXPORT)
LOCAL_SO_NAME      := $(MUXER_EXPORT)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_MUXER_DIR)/
	@cp -dpRf $< $(ORYX_MUXER_DIR)/$(MUXER_EXPORT_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

### libamexport-client.so
include $(CLEAR_VARS)

EXPORT_CLIENT = libamexport-client.so

LOCAL_TARGET   := $(EXPORT_CLIENT)

ifeq ($(BUILD_AMBARELLA_ORYX_VIDEO_NEW), y)
LOCAL_SRCS_UDS := $(LOCAL_PATH)/unixdomainsocket/am_export_client_uds_new.cpp
else
LOCAL_SRCS_UDS := $(LOCAL_PATH)/unixdomainsocket/am_export_client_uds.cpp
endif

LOCAL_SRCS     := $(LOCAL_PATH)/am_export_client_interface.cpp
LOCAL_SRCS     += $(LOCAL_SRCS_UDS)

LOCAL_CFLAGS := -I$(ORYX_DIR)/include                     \
                -I$(ORYX_DIR)/include/stream              \
                -I$(ORYX_DIR)/include/audio               \
                -I$(ORYX_DIR)/include/utility             \
                -I$(ORYX_DIR)/stream/include/amf          \
                -I$(ORYX_DIR)/stream/include/common       \
                -I$(ORYX_DIR)/stream/include/stream       \
                -I$(ORYX_DIR)/stream/include/record       \
                -std=c++11 -Werror

ifeq ($(BUILD_AMBARELLA_ORYX_VIDEO_NEW), y)
LOCAL_CFLAGS += -I$(ORYX_DIR)/include/video_new
else
LOCAL_CFLAGS += -I$(ORYX_DIR)/include/video
endif

LOCAL_LIBS := libamutil.so libamosal.so

ifeq ($(BUILD_AMBARELLA_ORYX_VIDEO_NEW), y)
LOCAL_LIBS += libamvideo-address.so
endif

EXPORT_CLIENT_NAME   = $(EXPORT_CLIENT)
EXPORT_CLIENT_TARGET = $(EXPORT_CLIENT)
LOCAL_SO_NAME       := $(EXPORT_CLIENT)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(EXPORT_CLIENT_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
