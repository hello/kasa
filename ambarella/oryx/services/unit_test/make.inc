##
## oryx/services/unit_test/make.inc
##
## History:
##    2014/09/09 - [Louis Sun] Created file
##
## Copyright (c) 2016 Ambarella, Inc.
##
## This file and its contents ("Software") are protected by intellectual
## property rights including, without limitation, U.S. and/or foreign
## copyrights. This Software is also the confidential and proprietary
## information of Ambarella, Inc. and its licensors. You may not use, reproduce,
## disclose, distribute, modify, or otherwise prepare derivative works of this
## Software or any portion thereof except pursuant to a signed license agreement
## or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
## In the absence of such an agreement, you agree to promptly notify and return
## this Software to Ambarella, Inc.
##
## THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
## INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
## MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
## IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
## INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
## (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
## LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##

LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(BUILD_AMBARELLA_ORYX_SERVICE_APPS_LAUNCHER), y)

###############################################################################
ifeq ($(BUILD_AMBARELLA_ORYX_SERVICE_MANAGER), y)

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
LOCAL_LIBS    := libamservice-manager.so libamutil.so libamipc.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
###############################################################################
LOCAL_TARGET := test_api_helper
LOCAL_SRCS   := $(LOCAL_PATH)/test_api_helper.cpp
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include       \
                 -I$(ORYX_DIR)/include/utility \
                 -I$(ORYX_DIR)/include/ipc    \
                 -I$(ORYX_DIR)/include/services  \
                 -I$(ORYX_DIR)/services/include \
                 -I$(PREBUILD_3RD_PARTY_DIR)/util-linux/include/uuid \
                 -I$(LOCAL_PATH)             \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib \
                 -L$(PREBUILD_3RD_PARTY_DIR)/util-linux/usr/lib -luuid
LOCAL_LIBS    := libamutil.so libamosal.so libamipc.so libamapi.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###############################################################################
ifeq ($(BUILD_AMBARELLA_ORYX_SERVICE_VIDEO), y)
##Video
LOCAL_TARGET := test_video_service_cfg_air_api
LOCAL_SRCS   := $(LOCAL_PATH)/test_video_service_cfg_air_api.cpp
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include            \
                 -I$(ORYX_DIR)/include/ipc        \
                 -I$(ORYX_DIR)/include/video      \
                 -I$(ORYX_DIR)/include/utility    \
                 -I$(ORYX_DIR)/include/services   \
                 -I$(ORYX_DIR)/services/include   \
                 -I$(LOCAL_PATH)                  \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib
LOCAL_LIBS    := libamutil.so libamosal.so libamipc.so libamapi.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

##
LOCAL_TARGET := test_video_service_dyn_air_api
LOCAL_SRCS   := $(LOCAL_PATH)/test_video_service_dyn_air_api.cpp
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include            \
                 -I$(ORYX_DIR)/include/ipc        \
                 -I$(ORYX_DIR)/include/video      \
                 -I$(ORYX_DIR)/include/utility    \
                 -I$(ORYX_DIR)/include/services   \
                 -I$(ORYX_DIR)/services/include   \
                 -I$(PREBUILD_3RD_PARTY_DIR)/util-linux/include/uuid \
                 -I$(LOCAL_PATH)                  \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib \
                 -L$(PREBUILD_3RD_PARTY_DIR)/util-linux/usr/lib -luuid
LOCAL_LIBS    := libamutil.so libamosal.so libamipc.so libamapi.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

##
LOCAL_TARGET := video_service_operation_guide
LOCAL_SRCS   := $(LOCAL_PATH)/video_service_operation_guide.cpp
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include            \
                 -I$(ORYX_DIR)/include/ipc        \
                 -I$(ORYX_DIR)/include/video      \
                 -I$(ORYX_DIR)/include/utility    \
                 -I$(ORYX_DIR)/include/services   \
                 -I$(ORYX_DIR)/services/include   \
                 -I$(LOCAL_PATH)                  \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib
LOCAL_LIBS    := libamutil.so libamosal.so libamipc.so libamapi.so

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
LOCAL_LIBS    := libamutil.so libamosal.so libamipc.so libamapi.so

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
LOCAL_LIBS    := libamutil.so libamosal.so libamipc.so libamapi.so
IMAGE_SERVICE_TEST_SHELL := image_service_self_test.sh
include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
ifeq ($(BUILD_AMBARELLA_ORYX_SERVICE_IMAGE_SELF_TEST), y)
	@cp -dpRf $(ORYX_DIR)/services/unit_test/$(IMAGE_SERVICE_TEST_SHELL) $(ORYX_BIN_DIR)/
	@chmod +x $(ORYX_BIN_DIR)/$(IMAGE_SERVICE_TEST_SHELL)
endif
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

LOCAL_LIBS    := libamutil.so libamosal.so libamipc.so libamapi.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif
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

LOCAL_LIBS    := libamutil.so libamosal.so libamipc.so libamapi.so

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

LOCAL_LIBS    := libamutil.so libamosal.so libamipc.so libamapi.so libammedia-data-struct.so

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

LOCAL_TARGET := test_media_service_dyn_air_api
LOCAL_SRCS   := $(LOCAL_PATH)/test_media_service_dyn_air_api.cpp
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include            \
                 -I$(ORYX_DIR)/include/ipc        \
                 -I$(ORYX_DIR)/include/utility    \
                 -I$(ORYX_DIR)/include/services   \
                 -I$(ORYX_DIR)/services/include   \
                 -I$(LOCAL_PATH)                  \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib

LOCAL_LIBS    := libamutil.so libamosal.so libamipc.so libamapi.so libammedia-data-struct.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
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

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libeXosip2/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libosip2/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/openssl/usr/lib

LOCAL_LIBS    := libamsip-data-struct.so      \
                 libamipc.so                     \
                 libamapi.so                     \
                 libamosal.so                     \
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
ifeq ($(BUILD_AMBARELLA_ORYX_SERVICE_EVENT), y)
ifeq ($(BUILD_AMBARELLA_ORYX_JPEG_SNAPSHOT), y)
LOCAL_TARGET := test_api_helper_notify
LOCAL_SRCS   := $(LOCAL_PATH)/test_api_helper_notify.cpp
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include       \
                 -I$(ORYX_DIR)/include/utility \
                 -I$(ORYX_DIR)/include/ipc    \
                 -I$(ORYX_DIR)/include/services  \
                 -I$(ORYX_DIR)/services/include \
                 -I$(LOCAL_PATH)             \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib
LOCAL_LIBS    := libamutil.so libamosal.so libamipc.so libamapi.so \
                 libamjpeg-snapshot.so libamvideo.so libamvideo-reader.so \
                 libamvideo-address.so libammedia-data-struct.so \
                 libamjpeg-encoder.so

LOCAL_LDFLAGS += -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/freetype/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/bzip2/usr/lib    \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib     \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libjpeg-turbo/usr/lib

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif
endif

###############################################################################
ifeq ($(BUILD_AMBARELLA_ORYX_SERVICE_PLAYBACK), y)

LOCAL_TARGET := test_playback_service_air_api
LOCAL_SRCS   := $(LOCAL_PATH)/test_playback_service_air_api.cpp
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include               \
                 -I$(ORYX_DIR)/include/utility       \
                 -I$(ORYX_DIR)/include/ipc           \
                 -I$(ORYX_DIR)/include/services      \
                 -I$(ORYX_DIR)/services/include      \
                 -I$(LOCAL_PATH)                     \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib
LOCAL_LIBS    := libamutil.so libamosal.so libamipc.so libamapi.so
include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

###############################################################################
ifeq ($(BUILD_AMBARELLA_ORYX_SERVICE_VIDEO_EDIT), y)

LOCAL_TARGET := test_video_edit_service_air_api
LOCAL_SRCS   := $(LOCAL_PATH)/test_video_edit_service_air_api.cpp
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include               \
                 -I$(ORYX_DIR)/include/utility       \
                 -I$(ORYX_DIR)/include/ipc           \
                 -I$(ORYX_DIR)/include/services      \
                 -I$(ORYX_DIR)/services/include      \
                 -I$(LOCAL_PATH)                     \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib
LOCAL_LIBS    := libamutil.so libamosal.so libamipc.so libamapi.so
include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

###############################################################################
ifeq ($(BUILD_AMBARELLA_ORYX_SERVICE_EFM_SOURCE), y)

LOCAL_TARGET := test_efm_src_service_air_api
LOCAL_SRCS   := $(LOCAL_PATH)/test_efm_src_service_air_api.cpp
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include               \
                 -I$(ORYX_DIR)/include/utility       \
                 -I$(ORYX_DIR)/include/ipc           \
                 -I$(ORYX_DIR)/include/services      \
                 -I$(ORYX_DIR)/services/include      \
                 -I$(LOCAL_PATH)                     \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib
LOCAL_LIBS    := libamutil.so libamosal.so libamipc.so libamapi.so
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
