##
## /S2l/oryx/stream/record/muxers/muxer-export/make.inc
##
## History:
##   2015-1-14 - [ypchang] created file
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

LOCAL_PATH := $(call my-dir)

### muxer-export.so
include $(CLEAR_VARS)

MUXER_EXPORT = muxer-export.so

LOCAL_TARGET   := $(MUXER_EXPORT)
LOCAL_SRCS_UDS := $(LOCAL_PATH)/unixdomainsocket/am_muxer_export_uds.cpp

LOCAL_SRCS     := $(LOCAL_PATH)/am_muxer_export_interface.cpp
LOCAL_SRCS     += $(LOCAL_SRCS_UDS)

LOCAL_CFLAGS := -std=c++11 -Werror                        \
                -I$(ORYX_DIR)/include                     \
                -I$(ORYX_DIR)/include/stream              \
                -I$(ORYX_DIR)/include/utility             \
                -I$(ORYX_DIR)/include/audio               \
                -I$(ORYX_DIR)/include/video               \
                -I$(ORYX_DIR)/stream/include/amf          \
                -I$(ORYX_DIR)/stream/include/common       \
                -I$(ORYX_DIR)/stream/include/stream       \
                -I$(ORYX_DIR)/stream/include/record       \
                -I$(ORYX_DIR)/stream/include/common/media

LOCAL_LDFLAGS := -Wl,-rpath=$(patsubst $(FAKEROOT_DIR)/%,%,$(ORYX_MUXER_DIR))

LOCAL_LIBS := libamutil.so \
              libamosal.so \
              libamvideo-reader.so

ifeq ($(CONFIG_AMBARELLA_ORYX_STREAM_RECORD_MUXER_ORIG), y)
LOCAL_LIBS += libammuxer-codec-iid.so
endif

ifeq ($(CONFIG_AMBARELLA_ORYX_STREAM_RECORD_MUXER_ACTIVE), y)
LOCAL_LIBS += libamactivemuxer-codec-iid.so
endif

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

LOCAL_SRCS_UDS := $(LOCAL_PATH)/unixdomainsocket/am_export_client_uds.cpp

LOCAL_SRCS     := $(LOCAL_PATH)/am_export_client_interface.cpp
LOCAL_SRCS     += $(LOCAL_SRCS_UDS)

LOCAL_CFLAGS := -std=c++11 -Werror                  \
                -I$(ORYX_DIR)/include               \
                -I$(ORYX_DIR)/include/stream        \
                -I$(ORYX_DIR)/include/audio         \
                -I$(ORYX_DIR)/include/video         \
                -I$(ORYX_DIR)/include/utility       \
                -I$(ORYX_DIR)/stream/include/amf    \
                -I$(ORYX_DIR)/stream/include/common \
                -I$(ORYX_DIR)/stream/include/stream \
                -I$(ORYX_DIR)/stream/include/record

LOCAL_LIBS := libamutil.so libamosal.so libamvideo-address.so

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
