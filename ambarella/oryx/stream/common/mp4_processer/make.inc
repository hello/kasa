##
## /S2l/oryx/stream/common/mp4_processer/make.inc
##
## History:
##   2015-9-16 - [ccjing] created file
##
## Copyright (C) 2008-2015, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

LOCAL_PATH := $(call my-dir)

### libammp4-processer.so
include $(CLEAR_VARS)

MP4_PROCESSER_TARGET:=libammp4-processer.so
MP4_PROCESSER_VER_FILE=$(ORYX_DIR)/stream/include/common/mp4_processer/version.h
MP4_PROCESSER_VER_PREFIX=MP4_PROCESSER

LOCAL_TARGET := $(MP4_PROCESSER_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)                           \
                -I$(ORYX_DIR)/include                     \
                -I$(ORYX_DIR)/include/utility             \
                -I$(ORYX_DIR)/stream/include/common/mp4_processer \
                -I$(ORYX_DIR)/stream/include/common/io    \
                -std=c++11 -Werror

MP4_PROCESSER_SO_VER_MAJ := $(shell awk '/define\ $(MP4_PROCESSER_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(MP4_PROCESSER_VER_FILE))
MP4_PROCESSER_SO_VER_MIN := $(shell awk '/define\ $(MP4_PROCESSER_VER_PREFIX)_LIB_MINOR/{print $$3}' $(MP4_PROCESSER_VER_FILE))
MP4_PROCESSER_SO_VER_PAT := $(shell awk '/define\ $(MP4_PROCESSER_VER_PREFIX)_LIB_PATCH/{print $$3}' $(MP4_PROCESSER_VER_FILE))
MP4_PROCESSER_SO_VER_STR := $(MP4_PROCESSER_SO_VER_MAJ).$(MP4_PROCESSER_SO_VER_MIN).$(MP4_PROCESSER_SO_VER_PAT)

MP4_PROCESSER_SO_NAME   := $(patsubst %.so,%.so.$(MP4_PROCESSER_SO_VER_MAJ), $(MP4_PROCESSER_TARGET))
MP4_PROCESSER_SO_TARGET := $(patsubst %.so,%.so.$(MP4_PROCESSER_SO_VER_STR), $(MP4_PROCESSER_TARGET))

LOCAL_LIBS := libamutil.so    \
              libamutil.so   \
              libamfile-sink.so

LOCAL_SO_NAME := $(MP4_PROCESSER_SO_NAME)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(MP4_PROCESSER_SO_TARGET)
	@ln -sf $(MP4_PROCESSER_SO_TARGET) $(ORYX_LIB_DIR)/$(MP4_PROCESSER_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
