##
## /S2l/oryx/stream/unit_test/make.inc
##
## History:
##   2014-11-5 - [ypchang] created file
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
ifeq ($(BUILD_AMBARELLA_ORYX_STREAM_UNIT_TEST), y)
##test_oryx_playback
ifeq ($(BUILD_AMBARELLA_ORYX_STREAM_PLAYBACK), y)
include $(CLEAR_VARS)

TEST_PLAYBACK_TARGET := test_oryx_playback
LOCAL_TARGET         := $(TEST_PLAYBACK_TARGET)
LOCAL_SRCS           := $(LOCAL_PATH)/test_oryx_playback.cpp

LOCAL_CFLAGS         := -I$(ORYX_DIR)/include               \
                        -I$(ORYX_DIR)/include/audio         \
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
endif
##test_oryx_record
ifeq ($(BUILD_AMBARELLA_ORYX_STREAM_RECORD), y)
include $(CLEAR_VARS)

TEST_RECORD_TARGET := test_oryx_record
LOCAL_TARGET       := $(TEST_RECORD_TARGET)
LOCAL_SRCS         := $(LOCAL_PATH)/test_oryx_record.cpp

LOCAL_CFLAGS       := -I$(ORYX_DIR)/include               \
                      -I$(ORYX_DIR)/include/audio         \
                      -I$(ORYX_DIR)/include/stream        \
                      -I$(ORYX_DIR)/include/utility       \
                      -I$(ORYX_DIR)/include/image_quality \
                      -I$(ORYX_DIR)/stream/include/record \
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
endif
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

## test_oryx_signature_and_verify
include $(CLEAR_VARS)

TEST_ORYX_SIGNATURE_AND_VERIFY_TARGET := test_oryx_signature_and_verify
LOCAL_TARGET         := $(TEST_ORYX_SIGNATURE_AND_VERIFY_TARGET)
LOCAL_SRCS           := $(LOCAL_PATH)/test_oryx_signature_and_verify.cpp

LOCAL_CFLAGS         := -I$(ORYX_DIR)/include                  \
                        -I$(ORYX_DIR)/stream/include/common/security_openssl\
                        -I$(ORYX_DIR)/include/utility          \
                        -std=c++11 -Werror

LOCAL_LDFLAGS  := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib

LOCAL_LIBS :=libamsecurity-openssl.so      \
              libamutil.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

## test_AV3_hash_verify
ifeq ($(BUILD_AMBARELLA_ORYX_MUXER_AV3), y)
include $(CLEAR_VARS)

TEST_AV3_HASH_VERIFY_TARGET := test_AV3_hash_verify
LOCAL_TARGET         := $(TEST_AV3_HASH_VERIFY_TARGET)
LOCAL_SRCS           := $(LOCAL_PATH)/test_AV3_hash_verify.cpp

LOCAL_CFLAGS         := -I$(ORYX_DIR)/include                                \
                        -I$(ORYX_DIR)/stream/include/common/security_openssl \
                        -I$(ORYX_DIR)/stream/include/common/io/              \
                        -I$(ORYX_DIR)/include/utility                        \
                        -std=c++11 -Werror

LOCAL_LDFLAGS  := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib

LOCAL_LIBS :=libamsecurity-openssl.so  \
             libamfile-sink.so         \
              libamutil.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif


## test_mp4_combine
include $(CLEAR_VARS)

TEST_MP4_COMBINE_TARGET := test_mp4_combine
LOCAL_TARGET         := $(TEST_MP4_COMBINE_TARGET)
LOCAL_SRCS           := $(LOCAL_PATH)/test_mp4_combine.cpp

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

## test_mp4_fix
include $(CLEAR_VARS)

TEST_MP4_FIXER_TARGET := test_mp4_fixer
LOCAL_TARGET          := $(TEST_MP4_FIXER_TARGET)
LOCAL_SRCS            := $(LOCAL_PATH)/test_mp4_fixer.cpp

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

## test_oryx_data_export
ifeq ($(BUILD_AMBARELLA_ORYX_STREAM_RECORD), y)
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
              libamutil.so \
              libamosal.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif
endif
