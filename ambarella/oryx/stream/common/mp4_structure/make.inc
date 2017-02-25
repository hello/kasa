##
## /S2l/oryx/stream/common/mp4_structure/make.inc
##
## History:
##   2016-01-12 - [ccjing] created file
##
## Copyright (c) 2015 Ambarella, Inc.
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

### libammp4-structure.so
include $(CLEAR_VARS)

MP4_STRUCTURE_TARGET:=libammp4-structure.so
MP4_STRUCTURE_VER_FILE=$(ORYX_DIR)/stream/include/common/mp4_structure/version.h
MP4_STRUCTURE_VER_PREFIX=MP4_STRUCTURE

LOCAL_TARGET := $(MP4_STRUCTURE_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)                                   \
                -I$(ORYX_DIR)/include                             \
                -I$(ORYX_DIR)/include/utility                     \
                -I$(ORYX_DIR)/stream/include/common/io            \
                -I$(ORYX_DIR)/stream/include/common/mp4_structure \
                -std=c++11 -Werror

MP4_STRUCTURE_SO_VER_MAJ := $(shell awk '/define\ $(MP4_STRUCTURE_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(MP4_STRUCTURE_VER_FILE))
MP4_STRUCTURE_SO_VER_MIN := $(shell awk '/define\ $(MP4_STRUCTURE_VER_PREFIX)_LIB_MINOR/{print $$3}' $(MP4_STRUCTURE_VER_FILE))
MP4_STRUCTURE_SO_VER_PAT := $(shell awk '/define\ $(MP4_STRUCTURE_VER_PREFIX)_LIB_PATCH/{print $$3}' $(MP4_STRUCTURE_VER_FILE))
MP4_STRUCTURE_SO_VER_STR := $(MP4_STRUCTURE_SO_VER_MAJ).$(MP4_STRUCTURE_SO_VER_MIN).$(MP4_STRUCTURE_SO_VER_PAT)

MP4_STRUCTURE_SO_NAME   := $(patsubst %.so,%.so.$(MP4_STRUCTURE_SO_VER_MAJ), $(MP4_STRUCTURE_TARGET))
MP4_STRUCTURE_SO_TARGET := $(patsubst %.so,%.so.$(MP4_STRUCTURE_SO_VER_STR), $(MP4_STRUCTURE_TARGET))

LOCAL_LIBS := libamutil.so \
              libamfile-sink.so


LOCAL_SO_NAME := $(MP4_STRUCTURE_SO_NAME)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(MP4_STRUCTURE_SO_TARGET)
	@ln -sf $(MP4_STRUCTURE_SO_TARGET) $(ORYX_LIB_DIR)/$(MP4_STRUCTURE_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
