##
## /S2l/oryx/stream/record/muxers/muxer-rtp/make.inc
##
## History:
##   2015-1-14 - [ypchang] created file
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

MUXER_RTP       = muxer-rtp.so
MUXER_RTP_CONF  = muxer-rtp.acs
RTP_CLIENT_CONF = rtp-client.acs

LOCAL_TARGET   := $(MUXER_RTP)
LOCAL_SRCS     := $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_CFLAGS   := -std=c++11 -Werror                        \
                  -I$(LOCAL_PATH)                           \
                  -I$(LOCAL_PATH)/rtp-session-base          \
                  -I$(ORYX_DIR)/include                     \
                  -I$(ORYX_DIR)/include/audio               \
                  -I$(ORYX_DIR)/include/video               \
                  -I$(ORYX_DIR)/include/stream              \
                  -I$(ORYX_DIR)/include/utility             \
                  -I$(ORYX_DIR)/include/configure           \
                  -I$(ORYX_DIR)/stream/include/amf          \
                  -I$(ORYX_DIR)/stream/include/record       \
                  -I$(ORYX_DIR)/stream/include/common       \
                  -I$(ORYX_DIR)/stream/include/common/media

LOCAL_LDFLAGS := -Wl,-rpath=$(patsubst $(FAKEROOT_DIR)/%,%,$(ORYX_MUXER_DIR))/rtp-session \
                 -Wl,-rpath=$(patsubst $(FAKEROOT_DIR)/%,%,$(ORYX_MUXER_DIR))

LOCAL_LIBS := libamutil.so                   \
              libamosal.so                   \
              libamconfig.so                 \
              libamstream-amf.so             \
              libammuxer-rtp-session-base.so

ifeq ($(CONFIG_AMBARELLA_ORYX_STREAM_RECORD_MUXER_ORIG), y)
LOCAL_LIBS += libammuxer-codec-iid.so
endif

ifeq ($(CONFIG_AMBARELLA_ORYX_STREAM_RECORD_MUXER_ACTIVE), y)
LOCAL_LIBS += libamactivemuxer-codec-iid.so
endif

MUXER_RTP_NAME    = $(MUXER_RTP)
MUXER_RTP_TARGET  = $(MUXER_RTP)
LOCAL_SO_NAME    := $(MUXER_RTP)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): MUXER_RTP_SRC_PATH := $(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_MUXER_DIR)/
	@mkdir -p $(ORYX_CONF_DIR)/stream/muxer/
	@cp -dpRf $(MUXER_RTP_SRC_PATH)/$(MUXER_RTP_CONF) $(ORYX_CONF_DIR)/stream/muxer/$(MUXER_RTP_CONF)
	@cp -dpRf $(MUXER_RTP_SRC_PATH)/$(RTP_CLIENT_CONF) $(ORYX_CONF_DIR)/stream/muxer/$(RTP_CLIENT_CONF)
	@cp -dpRf $< $(ORYX_MUXER_DIR)/$(MUXER_RTP_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

include $(MUXER_DIR)/muxer-rtp/rtp-session-base/make.inc
include $(MUXER_DIR)/muxer-rtp/rtp-session-aac/make.inc
include $(MUXER_DIR)/muxer-rtp/rtp-session-g711/make.inc
include $(MUXER_DIR)/muxer-rtp/rtp-session-g726/make.inc
include $(MUXER_DIR)/muxer-rtp/rtp-session-opus/make.inc
include $(MUXER_DIR)/muxer-rtp/rtp-session-speex/make.inc
include $(MUXER_DIR)/muxer-rtp/rtp-session-h264/make.inc
include $(MUXER_DIR)/muxer-rtp/rtp-session-h265/make.inc
include $(MUXER_DIR)/muxer-rtp/rtp-session-mjpeg/make.inc
