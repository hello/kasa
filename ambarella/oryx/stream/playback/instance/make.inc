##
## /S2l/oryx/stream/playback/instance/make.inc
##
## History:
##   2014-12-9 - [ypchang] created file
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

PLAYBACK_TARGET := libamstream-playback.so
PLAYBACK_VER_FILE = $(ORYX_DIR)/include/stream/version.h
PLAYBACK_VER_PREFIX = PLAYBACK

LOCAL_TARGET := $(PLAYBACK_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)                       \
                -I$(ORYX_DIR)/include                 \
                -I$(ORYX_DIR)/include/audio           \
                -I$(ORYX_DIR)/include/utility         \
                -I$(ORYX_DIR)/include/stream          \
                -I$(ORYX_DIR)/stream/include/amf      \
                -I$(ORYX_DIR)/stream/include/playback \
                -std=c++11 -Werror

LOCAL_LIBS := libamstream-playback-engine.so

PLAYBACK_SO_VER_MAJ := $(shell awk '/define\ $(PLAYBACK_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(PLAYBACK_VER_FILE))
PLAYBACK_SO_VER_MIN := $(shell awk '/define\ $(PLAYBACK_VER_PREFIX)_LIB_MINOR/{print $$3}' $(PLAYBACK_VER_FILE))
PLAYBACK_SO_VER_PAT := $(shell awk '/define\ $(PLAYBACK_VER_PREFIX)_LIB_PATCH/{print $$3}' $(PLAYBACK_VER_FILE))
PLAYBACK_SO_VER_STR := $(PLAYBACK_SO_VER_MAJ).$(PLAYBACK_SO_VER_MIN).$(PLAYBACK_SO_VER_PAT)

PLAYBACK_SO_NAME   := $(patsubst %.so,%.so.$(PLAYBACK_SO_VER_MAJ), $(PLAYBACK_TARGET))
PLAYBACK_SO_TARGET := $(patsubst %.so,%.so.$(PLAYBACK_SO_VER_STR), $(PLAYBACK_TARGET))

LOCAL_SO_NAME := $(PLAYBACK_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(PLAYBACK_SO_TARGET)
	@ln -sf $(PLAYBACK_SO_TARGET) $(ORYX_LIB_DIR)/$(PLAYBACK_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
