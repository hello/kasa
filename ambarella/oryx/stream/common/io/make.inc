##
## /S2l/oryx/stream/common/IO/make.inc
##
## History:
##   2014-12-12 - [ccJing] created file
##
## Copyright (C) 2008-2014, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

LOCAL_PATH   := $(call my-dir)
ifeq ($(BUILD_AMBARELLA_ORYX_STREAM), y)

### libamfile-sink.so
include $(CLEAR_VARS)

FILE_SINK_TARGET := libamfile-sink.so
FILE_SINK_VER_FILE = $(ORYX_DIR)/stream/include/common/io/version.h
FILE_SINK_VER_PREFIX = FILE_SINK

LOCAL_TARGET := $(FILE_SINK_TARGET)
LOCAL_SRCS   := $(LOCAL_PATH)/am_file_sink.cpp

LOCAL_CFLAGS := -I$(LOCAL_PATH)                        \
                -I$(ORYX_DIR)/include                  \
                -I$(ORYX_DIR)/include/utility          \
                -I$(ORYX_DIR)/stream/include/common/io \
                -std=c++11 -Werror

FILE_SINK_SO_VER_MAJ := $(shell awk '/define\ $(FILE_SINK_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(FILE_SINK_VER_FILE))
FILE_SINK_SO_VER_MIN := $(shell awk '/define\ $(FILE_SINK_VER_PREFIX)_LIB_MINOR/{print $$3}' $(FILE_SINK_VER_FILE))
FILE_SINK_SO_VER_PAT := $(shell awk '/define\ $(FILE_SINK_VER_PREFIX)_LIB_PATCH/{print $$3}' $(FILE_SINK_VER_FILE))
FILE_SINK_SO_VER_STR := $(FILE_SINK_SO_VER_MAJ).$(FILE_SINK_SO_VER_MIN).$(FILE_SINK_SO_VER_PAT)

FILE_SINK_SO_NAME   := $(patsubst %.so,%.so.$(FILE_SINK_SO_VER_MAJ), $(FILE_SINK_TARGET))
FILE_SINK_SO_TARGET := $(patsubst %.so,%.so.$(FILE_SINK_SO_VER_STR), $(FILE_SINK_TARGET))

LOCAL_LIBS := libamutil.so      \
              libamosal.so

LOCAL_SO_NAME := $(FILE_SINK_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(FILE_SINK_SO_TARGET)
	@ln -sf $(FILE_SINK_SO_TARGET) $(ORYX_LIB_DIR)/$(FILE_SINK_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif