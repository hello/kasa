##
## /S2l/oryx/stream/record/muxers/muxer-rtp/rtp-session-opus/make.inc
##
## History:
##   Nov 4, 2015 - [ypchang] created file
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
##
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

RTP_SESSION_OPUS = rtp-session-opus.so

LOCAL_TARGET   := $(RTP_SESSION_OPUS)
LOCAL_SRCS     := $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_CFLAGS   := -std=c++11 -Werror                        \
                  -I$(LOCAL_PATH)                           \
                  -I$(LOCAL_PATH)/..                        \
                  -I$(LOCAL_PATH)/../rtp-session-base       \
                  -I$(ORYX_DIR)/include                     \
                  -I$(ORYX_DIR)/include/audio               \
                  -I$(ORYX_DIR)/include/stream              \
                  -I$(ORYX_DIR)/include/utility             \
                  -I$(ORYX_DIR)/stream/include/amf          \
                  -I$(ORYX_DIR)/stream/include/common       \
                  -I$(ORYX_DIR)/stream/include/common/media

LOCAL_LDFLAGS := -Wl,-rpath=$(patsubst $(FAKEROOT_DIR)/%,%,$(ORYX_MUXER_DIR))/rtp-session

LOCAL_LIBS := libamutil.so                   \
              libammuxer-rtp-session-base.so

LOCAL_SO_NAME := $(RTP_SESSION_OPUS)
RTP_OPUS_48K   := $(patsubst %.so,%-48k.so,$(RTP_SESSION_OPUS))
RTP_OPUS_16K   := $(patsubst %.so,%-16k.so,$(RTP_SESSION_OPUS))
RTP_OPUS_8K    := $(patsubst %.so,%-8k.so,$(RTP_SESSION_OPUS))

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_MUXER_DIR)/rtp-session
	@cp -dpRf $< $(ORYX_MUXER_DIR)/rtp-session/$(RTP_SESSION_OPUS)
	@ln -sf $(RTP_SESSION_OPUS) $(ORYX_MUXER_DIR)/rtp-session/$(RTP_OPUS_48K)
	@ln -sf $(RTP_SESSION_OPUS) $(ORYX_MUXER_DIR)/rtp-session/$(RTP_OPUS_16K)
	@ln -sf $(RTP_SESSION_OPUS) $(ORYX_MUXER_DIR)/rtp-session/$(RTP_OPUS_8K)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
