##
## /S2l/oryx/stream/record/engine/make.inc
##
## History:
##   2014-12-31 - [ypchang] created file
##
## Copyright (C) 2008-2014, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

RECORD_ENGINE_TARGET    := libamstream-record-engine.so
RECORD_ENGINE_CONF       = record-engine.acs
RECORD_ENGINE_VER_FILE   = $(ORYX_DIR)/include/stream/version.h
RECORD_ENGINE_VER_PREFIX = PLAYBACK

LOCAL_TARGET := $(RECORD_ENGINE_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)                     \
                -I$(ORYX_DIR)/include               \
                -I$(ORYX_DIR)/include/audio         \
                -I$(ORYX_DIR)/include/video         \
                -I$(ORYX_DIR)/include/configure     \
                -I$(ORYX_DIR)/include/utility       \
                -I$(ORYX_DIR)/include/stream        \
                -I$(ORYX_DIR)/stream/include/amf    \
                -I$(ORYX_DIR)/stream/include/common \
                -I$(ORYX_DIR)/stream/include/record \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so       \
              libamosal.so       \
              libamconfig.so     \
              libamstream-iid-record.so

RECORD_ENGINE_SO_VER_MAJ := $(shell awk '/define\ $(RECORD_ENGINE_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(RECORD_ENGINE_VER_FILE))
RECORD_ENGINE_SO_VER_MIN := $(shell awk '/define\ $(RECORD_ENGINE_VER_PREFIX)_LIB_MINOR/{print $$3}' $(RECORD_ENGINE_VER_FILE))
RECORD_ENGINE_SO_VER_PAT := $(shell awk '/define\ $(RECORD_ENGINE_VER_PREFIX)_LIB_PATCH/{print $$3}' $(RECORD_ENGINE_VER_FILE))
RECORD_ENGINE_SO_VER_STR := $(RECORD_ENGINE_SO_VER_MAJ).$(RECORD_ENGINE_SO_VER_MIN).$(RECORD_ENGINE_SO_VER_PAT)

RECORD_ENGINE_SO_NAME   := $(patsubst %.so,%.so.$(RECORD_ENGINE_SO_VER_MAJ), $(RECORD_ENGINE_TARGET))
RECORD_ENGINE_SO_TARGET := $(patsubst %.so,%.so.$(RECORD_ENGINE_SO_VER_STR), $(RECORD_ENGINE_TARGET))

LOCAL_SO_NAME := $(RECORD_ENGINE_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): filter-audio-source.so filter-video-source.so filter-avqueue.so filter-muxer.so
$(LOCAL_TARGET): RECORD_ENGINE_SRC_PATH := $(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@mkdir -p $(ORYX_CONF_DIR)/stream/engine/
	@cp -dpRf $(RECORD_ENGINE_SRC_PATH)/$(RECORD_ENGINE_CONF) $(ORYX_CONF_DIR)/stream/engine/$(RECORD_ENGINE_CONF)
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(RECORD_ENGINE_SO_TARGET)
	@ln -sf $(RECORD_ENGINE_SO_TARGET) $(ORYX_LIB_DIR)/$(RECORD_ENGINE_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
