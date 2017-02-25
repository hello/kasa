##
## /S2l/oryx/stream/common/IO/make.inc
##
## History:
##   2014-12-12 - [ccJing] created file
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