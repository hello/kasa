##
## /S2l/oryx/stream/unit_test/make.inc
##
## History:
##   2014-11-5 - [ypchang] created file
##
## Copyright (C) 2008-2014, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

LOCAL_PATH := $(call my-dir)
ifeq ($(BUILD_AMBARELLA_ORYX_STREAM_PLAYBACK_UNIT_TEST), y)
##test_oryx_playback
include $(CLEAR_VARS)

TEST_PLAYBACK_TARGET := test_oryx_playback
LOCAL_TARGET         := $(TEST_PLAYBACK_TARGET)
LOCAL_SRCS           := $(LOCAL_PATH)/test_oryx_playback.cpp

LOCAL_CFLAGS         := -I$(ORYX_DIR)/include               \
                        -I$(ORYX_DIR)/include/audio         \
                        -I$(ORYX_DIR)/include/video         \
                        -I$(ORYX_DIR)/include/stream        \
                        -I$(ORYX_DIR)/include/utility       \
                        -std=c++11 -Werror

LOCAL_LDFLAGS  := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib \
                  -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/lua/usr/lib

LOCAL_LIBS := libamstream-playback.so \
              libamstream-amf.so      \
              libamosal.so            \
              libamutil.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

##test_oryx_record
include $(CLAER_VARS)

TEST_RECORD_TARGET := test_oryx_record
LOCAL_TARGET       := $(TEST_RECORD_TARGET)
LOCAL_SRCS         := $(LOCAL_PATH)/test_oryx_record.cpp

LOCAL_CFLAGS       := -I$(ORYX_DIR)/include              \
                      -I$(ORYX_DIR)/include/stream       \
                      -I$(ORYX_DIR)/include/utility      \
                      -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/lua/usr/lib

LOCAL_LIBS := libamstream-record.so \
              libamstream-amf.so    \
              libamosal.so          \
              libamutil.so

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

## test_oryx_file_sink
include $(CLEAR_VARS)

TEST_FILE_SINK_TARGET := test_oryx_file_sink
LOCAL_TARGET         := $(TEST_FILE_SINK_TARGET)
LOCAL_SRCS           := $(LOCAL_PATH)/test_oryx_file_sink.cpp

LOCAL_CFLAGS         := -I$(ORYX_DIR)/include                  \
                        -I$(ORYX_DIR)/include/stream           \
                        -I$(ORYX_DIR)/stream/include/common/io \
                        -I$(ORYX_DIR)/include/utility          \
                        -std=c++11 -Werror

LOCAL_LDFLAGS  := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib

LOCAL_LIBS := libamfile-sink.so       \
              libamutil.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))


## test_mp4_split
include $(CLEAR_VARS)

TEST_MP4_SPLIT_TARGET := test_mp4_split
LOCAL_TARGET         := $(TEST_MP4_SPLIT_TARGET)
LOCAL_SRCS           := $(LOCAL_PATH)/test_mp4_split.cpp

LOCAL_CFLAGS         := -I$(ORYX_DIR)/include                  \
                        -I$(ORYX_DIR)/stream/include/common/mp4_processer\
                        -I$(ORYX_DIR)/stream/common/mp4_processer\
                        -I$(ORYX_DIR)/include/utility          \
                        -std=c++11 -Werror

LOCAL_LDFLAGS  := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib

LOCAL_LIBS :=libammp4-processer.so       \
              libamutil.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif

## test_oryx_data_export
include $(CLEAR_VARS)
TEST_DATA_EXPORT_TARGET := test_oryx_data_export
LOCAL_TARGET         := $(TEST_DATA_EXPORT_TARGET)
LOCAL_SRCS           := $(LOCAL_PATH)/test_oryx_data_export.cpp

LOCAL_CFLAGS         := -I$(ORYX_DIR)/include                  \
                        -I$(ORYX_DIR)/include/stream           \
                        -I$(ORYX_DIR)/stream/include/record    \
                        -I$(ORYX_DIR)/include/utility          \
                        -std=c++11 -Werror

LOCAL_LDFLAGS  := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib

LOCAL_LIBS := libamexport-client.so  \
              libamutil.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
