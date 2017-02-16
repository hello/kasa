##
## /S2l/oryx/stream/playback/filters/player/make.inc
##
## History:
##   2014-9-22 - [ypchang] created file
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

PLAYER_TARGET = filter-player.so
PLAYER_CONF   = filter-player.acs

LOCAL_TARGET := $(PLAYER_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)                                \
                -I$(ORYX_DIR)/include                          \
                -I$(ORYX_DIR)/include/audio                    \
                -I$(ORYX_DIR)/include/stream                   \
                -I$(ORYX_DIR)/include/utility                  \
                -I$(ORYX_DIR)/include/configure                \
                -I$(ORYX_DIR)/stream/include/amf               \
                -I$(ORYX_DIR)/stream/include/common            \
                -I$(ORYX_DIR)/stream/include/playback          \
                -I$(PREBUILD_3RD_PARTY_DIR)/pulseaudio/include \
                -std=c++11 -Werror

LOCAL_LDFLAGS := -L$(PREBUILD_3RD_PARTY_DIR)/pulseaudio/usr/lib -lpulse       \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/systemd/usr/lib    \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/pulseaudio/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libsndfile/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/dbus/usr/lib       \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/json-c/usr/lib     \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libcap/usr/lib

LOCAL_LIBS := libamutil.so                \
              libamosal.so                \
              libamconfig.so              \
              libamstream-iid-playback.so \
              libamstream-amf.so

PLAYER_SO_NAME   = $(PLAYER_TARGET)
PLAYER_SO_TARGET = $(PLAYER_TARGET)

LOCAL_SO_NAME := $(PLAYER_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PLAYER_SRC_PATH := $(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_FILTER_DIR)/
	@mkdir -p $(ORYX_CONF_DIR)/stream/filter/
	@cp -dpRf $(PLAYER_SRC_PATH)/$(PLAYER_CONF) $(ORYX_CONF_DIR)/stream/filter/$(PLAYER_CONF)
	@cp -dpRf $< $(ORYX_FILTER_DIR)/$(PLAYER_SO_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
