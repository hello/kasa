##
## /S2l/oryx/stream/common/codecs/make.inc
##
## History:
##   2014-11-3 - [ypchang] created file
##
## Copyright (C) 2008-2014, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

LOCAL_PATH := $(call my-dir)

### libamcodec-base.so
include $(CLEAR_VARS)

CODEC_BASE = libamcodec-base.so

LOCAL_TARGET := $(CODEC_BASE)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/base/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)/base                \
                -I$(ORYX_DIR)/include               \
                -I$(ORYX_DIR)/include/audio         \
                -I$(ORYX_DIR)/include/utility       \
                -I$(ORYX_DIR)/stream/include/amf    \
                -I$(ORYX_DIR)/stream/include/common \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so

CODEC_BASE_NAME   = $(CODEC_BASE)
CODEC_BASE_TARGET = $(CODEC_BASE)

LOCAL_SO_NAME := $(CODEC_BASE)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(CODEC_BASE_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

### codec-aac.so
include $(CLEAR_VARS)

CODEC_AAC  = codec-aac.so
AAC_CONFIG = codec-aac.acs
CODEC_AAC_LOW  = codec-aac-low.so
AAC_CONFIG_LOW = codec-aac-low.acs

LOCAL_TARGET := $(CODEC_AAC)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/codec-aac/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)/base                    \
                -I$(LOCAL_PATH)/codec-aac               \
                -I$(ORYX_DIR)/include                   \
                -I$(ORYX_DIR)/include/audio             \
                -I$(ORYX_DIR)/include/utility           \
                -I$(ORYX_DIR)/include/configure         \
                -I$(ORYX_DIR)/stream/include/amf        \
                -I$(ORYX_DIR)/stream/include/common     \
                -I$(PREBUILD_3RD_PARTY_DIR)/aac/include \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so   \
              libamconfig.so \
              libamcodec-base.so

LOCAL_LDFLAGS = -L$(PREBUILD_3RD_PARTY_DIR)/aac/lib -laacenc -laacdec

CODEC_AAC_NAME   = $(CODEC_AAC)
CODEC_AAC_TARGET = $(CODEC_AAC)

LOCAL_SO_NAME := $(CODEC_AAC)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): libamcodec-base.so
$(LOCAL_TARGET): AAC_SRC_PATH := $(LOCAL_PATH)/codec-aac
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_CODEC_DIR)
	@mkdir -p $(ORYX_CONF_DIR)/stream/codec
	@cp -dpRf $(AAC_SRC_PATH)/$(AAC_CONFIG) $(ORYX_CONF_DIR)/stream/codec/$(AAC_CONFIG)
	@cp -dpRf $(AAC_SRC_PATH)/$(AAC_CONFIG_LOW) $(ORYX_CONF_DIR)/stream/codec/$(AAC_CONFIG_LOW)
	@cp -dpRf $< $(ORYX_CODEC_DIR)/$(CODEC_AAC_TARGET)
	@ln -sf $(CODEC_AAC_TARGET) $(ORYX_CODEC_DIR)/$(CODEC_AAC_LOW)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

### codec-g711.so
include $(CLEAR_VARS)

CODEC_G711  = codec-g711.so
G711_CONFIG = codec-g711.acs
CODEC_G711_LOW  = codec-g711-low.so
G711_CONFIG_LOW = codec-g711-low.acs

LOCAL_TARGET := $(CODEC_G711)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/codec-g711/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)/base                        \
                -I$(LOCAL_PATH)/codec-g711                  \
                -I$(ORYX_DIR)/include                       \
                -I$(ORYX_DIR)/include/audio                 \
                -I$(ORYX_DIR)/include/utility               \
                -I$(ORYX_DIR)/include/configure             \
                -I$(ORYX_DIR)/stream/include/amf            \
                -I$(ORYX_DIR)/stream/include/common         \
                -I$(PREBUILD_3RD_PARTY_DIR)/libg7xx/include \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so   \
              libamconfig.so \
              libamcodec-base.so

LOCAL_LDFLAGS = -L$(PREBUILD_3RD_PARTY_DIR)/libg7xx/usr/lib -lg7xx

CODEC_G711_NAME   = $(CODEC_G711)
CODEC_G711_TARGET = $(CODEC_G711)

LOCAL_SO_NAME := $(CODEC_G711)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): libamcodec-base.so
$(LOCAL_TARGET): G711_SRC_PATH := $(LOCAL_PATH)/codec-g711
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_CODEC_DIR)
	@mkdir -p $(ORYX_CONF_DIR)/stream/codec
	@cp -dpRf $(G711_SRC_PATH)/$(G711_CONFIG) $(ORYX_CONF_DIR)/stream/codec/$(G711_CONFIG)
	@cp -dpRf $(G711_SRC_PATH)/$(G711_CONFIG_LOW) $(ORYX_CONF_DIR)/stream/codec/$(G711_CONFIG_LOW)
	@cp -dpRf $< $(ORYX_CODEC_DIR)/$(CODEC_G711_TARGET)
	@ln -sf $(CODEC_G711_TARGET) $(ORYX_CODEC_DIR)/$(CODEC_G711_LOW)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

### codec-g726.so
include $(CLEAR_VARS)

CODEC_G726  = codec-g726.so
G726_CONFIG = codec-g726.acs

LOCAL_TARGET := $(CODEC_G726)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/codec-g726/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)/base                        \
                -I$(LOCAL_PATH)/codec-g726                  \
                -I$(ORYX_DIR)/include                       \
                -I$(ORYX_DIR)/include/audio                 \
                -I$(ORYX_DIR)/include/utility               \
                -I$(ORYX_DIR)/include/configure             \
                -I$(ORYX_DIR)/stream/include/amf            \
                -I$(ORYX_DIR)/stream/include/common         \
                -I$(PREBUILD_3RD_PARTY_DIR)/libg7xx/include \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so   \
              libamconfig.so \
              libamcodec-base.so

LOCAL_LDFLAGS = -L$(PREBUILD_3RD_PARTY_DIR)/libg7xx/usr/lib -lg7xx

CODEC_G726_NAME   = $(CODEC_G726)
CODEC_G726_TARGET = $(CODEC_G726)

LOCAL_SO_NAME := $(CODEC_G726)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): libamcodec-base.so
$(LOCAL_TARGET): G726_SRC_PATH := $(LOCAL_PATH)/codec-g726
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_CODEC_DIR)
	@mkdir -p $(ORYX_CONF_DIR)/stream/codec
	@cp -dpRf $(G726_SRC_PATH)/$(G726_CONFIG) $(ORYX_CONF_DIR)/stream/codec/$(G726_CONFIG)
	@cp -dpRf $< $(ORYX_CODEC_DIR)/$(CODEC_G726_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

### codec-opus.so
include $(CLEAR_VARS)

CODEC_OPUS  = codec-opus.so
OPUS_CONFIG = codec-opus.acs
CODEC_OPUS_LOW  = codec-opus-low.so
OPUS_CONFIG_LOW = codec-opus-low.acs

LOCAL_TARGET := $(CODEC_OPUS)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/codec-opus/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)/base                        \
                -I$(LOCAL_PATH)/codec-opus                  \
                -I$(ORYX_DIR)/include                       \
                -I$(ORYX_DIR)/include/audio                 \
                -I$(ORYX_DIR)/include/utility               \
                -I$(ORYX_DIR)/include/configure             \
                -I$(ORYX_DIR)/stream/include/amf            \
                -I$(ORYX_DIR)/stream/include/common         \
                -I$(PREBUILD_3RD_PARTY_DIR)/opus/include \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so   \
              libamconfig.so \
              libamcodec-base.so

LOCAL_LDFLAGS = -L$(PREBUILD_3RD_PARTY_DIR)/opus/usr/lib -lopus

CODEC_OPUS_NAME   = $(CODEC_OPUS)
CODEC_OPUS_TARGET = $(CODEC_OPUS)

LOCAL_SO_NAME := $(CODEC_OPUS)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): OPUS_SRC_PATH := $(LOCAL_PATH)/codec-opus
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_CODEC_DIR)
	@mkdir -p $(ORYX_CONF_DIR)/stream/codec
	@cp -dpRf $(OPUS_SRC_PATH)/$(OPUS_CONFIG) $(ORYX_CONF_DIR)/stream/codec/$(OPUS_CONFIG)
	@cp -dpRf $(OPUS_SRC_PATH)/$(OPUS_CONFIG_LOW) $(ORYX_CONF_DIR)/stream/codec/$(OPUS_CONFIG_LOW)
	@cp -dpRf $< $(ORYX_CODEC_DIR)/$(CODEC_OPUS_TARGET)
	@ln -sf $(CODEC_OPUS_TARGET) $(ORYX_CODEC_DIR)/$(CODEC_OPUS_LOW)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

### codec-speex.so
include $(CLEAR_VARS)

CODEC_SPEEX  = codec-speex.so
SPEEX_CONFIG = codec-speex.acs
CODEC_SPEEX_LOW  = codec-speex-low.so
SPEEX_CONFIG_LOW = codec-speex-low.acs

LOCAL_TARGET := $(CODEC_SPEEX)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/codec-speex/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)/base                            \
                -I$(LOCAL_PATH)/codec-speex                     \
                -I$(ORYX_DIR)/include                           \
                -I$(ORYX_DIR)/include/audio                     \
                -I$(ORYX_DIR)/include/utility                   \
                -I$(ORYX_DIR)/include/configure                 \
                -I$(ORYX_DIR)/stream/include/amf                \
                -I$(ORYX_DIR)/stream/include/common             \
                -I$(PREBUILD_3RD_PARTY_DIR)/speex/include/speex \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so   \
              libamconfig.so \
              libamcodec-base.so

LOCAL_LDFLAGS = -L$(PREBUILD_3RD_PARTY_DIR)/speex/usr/lib -lspeex

CODEC_SPEEX_NAME   = $(CODEC_SPEEX)
CODEC_SPEEX_TARGET = $(CODEC_SPEEX)

LOCAL_SO_NAME := $(CODEC_SPEEX)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): SPEEX_SRC_PATH := $(LOCAL_PATH)/codec-speex
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_CODEC_DIR)
	@mkdir -p $(ORYX_CONF_DIR)/stream/codec
	@cp -dpRf $(SPEEX_SRC_PATH)/$(SPEEX_CONFIG) $(ORYX_CONF_DIR)/stream/codec/$(SPEEX_CONFIG)
	@cp -dpRf $(SPEEX_SRC_PATH)/$(SPEEX_CONFIG_LOW) $(ORYX_CONF_DIR)/stream/codec/$(SPEEX_CONFIG_LOW)
	@cp -dpRf $< $(ORYX_CODEC_DIR)/$(CODEC_SPEEX_TARGET)
	@ln -sf $(CODEC_SPEEX_TARGET) $(ORYX_CODEC_DIR)/$(CODEC_SPEEX_LOW)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
