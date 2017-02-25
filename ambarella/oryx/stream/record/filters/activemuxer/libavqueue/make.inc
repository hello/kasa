##
## /S2l/oryx/stream/record/filters/activemuxer/libavqueue/make.inc
##
## History:
##   2016-9-26 - [Shupeng Ren] created file
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

include $(CLEAR_VARS)

LIBAVQ_TARGET := libavqueue.so
LIBAVQ_VER_FILE := $(LOCAL_PATH)/am_avqueue_version.h
LIBAVQ_VER_PREFIX := AV_QUEUE

LOCAL_TARGET := $(LIBAVQ_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)                           \
                -I$(ORYX_DIR)/include                     \
                -I$(ORYX_DIR)/include/video               \
                -I$(ORYX_DIR)/include/audio               \
                -I$(ORYX_DIR)/include/configure           \
                -I$(ORYX_DIR)/include/utility             \
                -I$(ORYX_DIR)/include/stream              \
                -I$(ORYX_DIR)/stream/include/amf          \
                -I$(ORYX_DIR)/stream/include/common       \
                -I$(ORYX_DIR)/stream/include/common/media \
                -I$(ORYX_DIR)/stream/include/record       \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so              \
              libamosal.so              \
              libamconfig.so            \
              libamstream-iid-record.so \
              libamstream-amf.so

LIBAVQ_VER_MAJ := $(shell awk '/define\ $(LIBAVQ_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(LIBAVQ_VER_FILE))
LIBAVQ_VER_MIN := $(shell awk '/define\ $(LIBAVQ_VER_PREFIX)_LIB_MINOR/{print $$3}' $(LIBAVQ_VER_FILE))
LIBAVQ_VER_PAT := $(shell awk '/define\ $(LIBAVQ_VER_PREFIX)_LIB_PATCH/{print $$3}' $(LIBAVQ_VER_FILE))
LIBAVQ_VER_STR := $(LIBAVQ_VER_MAJ).$(LIBAVQ_VER_MIN).$(LIBAVQ_VER_PAT)

LIBAVQ_SO_NAME   := $(patsubst %.so,%.so.$(LIBAVQ_VER_MAJ), $(LOCAL_TARGET))
LIBAVQ_SO_TARGET := $(patsubst %.so,%.so.$(LIBAVQ_VER_STR), $(LOCAL_TARGET))

LOCAL_SO_NAME := $(LIBAVQ_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(LIBAVQ_SO_TARGET)
	@ln -sf $(LIBAVQ_SO_TARGET) $(ORYX_LIB_DIR)/$(LIBAVQ_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
