##
## oryx/services/unit_test/make.inc
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

LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(BUILD_AMBARELLA_ORYX_SERVICE_APPS_LAUNCHER), y)
###############################################################################
LOCAL_TARGET := test_service_manager
LOCAL_SRCS   := $(LOCAL_PATH)/test_service_manager.cpp
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include       \
                 -I$(ORYX_DIR)/include/utility \
                 -I$(ORYX_DIR)/include/ipc    \
                 -I$(ORYX_DIR)/include/services  \
                 -I$(ORYX_DIR)/services/include \
                 -I$(LOCAL_PATH)             \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib
LOCAL_LIBS    := libamutil.so libamipc.so libamservice-manager.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###############################################################################
LOCAL_TARGET := test_api_helper
LOCAL_SRCS   := $(LOCAL_PATH)/test_api_helper.cpp
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include       \
                 -I$(ORYX_DIR)/include/utility \
                 -I$(ORYX_DIR)/include/ipc    \
                 -I$(ORYX_DIR)/include/services  \
                 -I$(ORYX_DIR)/services/include \
                 -I$(LOCAL_PATH)             \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib
LOCAL_LIBS    := libamutil.so libamipc.so libamapi.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###############################################################################
ifeq ($(BUILD_AMBARELLA_ORYX_SERVICE_VIDEO), y)

LOCAL_TARGET := test_video_service_air_api
LOCAL_SRCS   := $(LOCAL_PATH)/test_video_service_air_api.cpp
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include            \
                 -I$(ORYX_DIR)/include/ipc        \
                 -I$(ORYX_DIR)/include/video      \
                 -I$(ORYX_DIR)/include/utility    \
                 -I$(ORYX_DIR)/include/services   \
                 -I$(ORYX_DIR)/services/include   \
                 -I$(LOCAL_PATH)                  \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib
LOCAL_LIBS    := libamutil.so libamipc.so libamapi.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

###############################################################################
ifeq ($(BUILD_AMBARELLA_ORYX_SERVICE_AUDIO), y)

LOCAL_TARGET := test_audio_service_air_api
LOCAL_SRCS   := $(LOCAL_PATH)/test_audio_service_air_api.cpp
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include              \
                 -I$(ORYX_DIR)/include/utility      \
                 -I$(ORYX_DIR)/include/ipc          \
                 -I$(ORYX_DIR)/include/audio/device \
                 -I$(ORYX_DIR)/include/services     \
                 -I$(ORYX_DIR)/services/include     \
                 -I$(LOCAL_PATH)                    \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib
LOCAL_LIBS    := libamutil.so libamipc.so libamapi.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

###############################################################################
ifeq ($(BUILD_AMBARELLA_ORYX_SERVICE_IMAGE), y)

LOCAL_TARGET := test_image_service_air_api
LOCAL_SRCS   := $(LOCAL_PATH)/test_image_service_air_api.cpp
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include               \
                 -I$(ORYX_DIR)/include/utility       \
                 -I$(ORYX_DIR)/include/ipc           \
                 -I$(ORYX_DIR)/include/image_quality \
                 -I$(ORYX_DIR)/image_quality/include \
                 -I$(ORYX_DIR)/include/services      \
                 -I$(ORYX_DIR)/services/include      \
                 -I$(LOCAL_PATH)                     \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib
LOCAL_LIBS    := libamutil.so libamipc.so libamapi.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

###############################################################################
ifeq ($(BUILD_AMBARELLA_ORYX_SERVICE_EVENT), y)

LOCAL_TARGET := test_event_service_air_api
LOCAL_SRCS   := $(LOCAL_PATH)/test_event_service_air_api.cpp
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include            \
                 -I$(ORYX_DIR)/include/ipc        \
                 -I$(ORYX_DIR)/include/event      \
                 -I$(ORYX_DIR)/include/utility    \
                 -I$(ORYX_DIR)/include/services   \
                 -I$(ORYX_DIR)/services/include   \
                 -I$(LOCAL_PATH)                  \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib

LOCAL_LIBS    := libamutil.so libamipc.so libamapi.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
###############################################################################

###############################################################################
ifeq ($(BUILD_AMBARELLA_ORYX_SERVICE_SYSTEM), y)

LOCAL_TARGET := test_system_service_air_api
LOCAL_SRCS   := $(LOCAL_PATH)/test_system_service_air_api.cpp
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include            \
                 -I$(ORYX_DIR)/include/ipc        \
                 -I$(ORYX_DIR)/include/utility    \
                 -I$(ORYX_DIR)/include/services   \
                 -I$(ORYX_DIR)/services/include   \
                 -I$(LOCAL_PATH)                  \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib

LOCAL_LIBS    := libamutil.so libamipc.so libamapi.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
###############################################################################
ifeq ($(BUILD_AMBARELLA_ORYX_SERVICE_MEDIA), y)

LOCAL_TARGET := test_media_service_air_api
LOCAL_SRCS   := $(LOCAL_PATH)/test_media_service_air_api.cpp
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include            \
                 -I$(ORYX_DIR)/include/ipc        \
                 -I$(ORYX_DIR)/include/utility    \
                 -I$(ORYX_DIR)/include/services   \
                 -I$(ORYX_DIR)/services/include   \
                 -I$(LOCAL_PATH)                  \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib

LOCAL_LIBS    := libamutil.so libamipc.so libamapi.so libammedia-data-struct.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
###############################################################################
###############################################################################
ifeq ($(BUILD_AMBARELLA_ORYX_SIP_SERVICE), y)

LOCAL_TARGET := test_sip_service_air_api
LOCAL_SRCS   := $(LOCAL_PATH)/test_sip_service_air_api.cpp
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include                  \
                 -I$(ORYX_DIR)/include/ipc              \
                 -I$(ORYX_DIR)/include/utility          \
                 -I$(ORYX_DIR)/include/services         \
                 -I$(ORYX_DIR)/services/include         \
                 -I$(ORYX_DIR)/include/protocol         \
                 -I$(ORYX_DIR)/include/service/commands \
                 -I$(LOCAL_PATH)                        \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib

LOCAL_LIBS    := libamsip-data-struct.so      \
                 libamipc.so                     \
                 libamapi.so                     \
                 libamutil.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
###############################################################################
endif
