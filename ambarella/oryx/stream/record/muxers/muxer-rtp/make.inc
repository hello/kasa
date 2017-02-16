##
## /S2l/oryx/stream/record/muxers/muxer-rtp/make.inc
##
## History:
##   2015-1-14 - [ypchang] created file
##
## Copyright (C) 2008-2015, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

MUXER_RTP       = muxer-rtp.so
MUXER_RTP_CONF  = muxer-rtp.acs
RTP_CLIENT_CONF = rtp-client.acs

LOCAL_TARGET   := $(MUXER_RTP)
LOCAL_SRCS     := $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_CFLAGS   := -I$(LOCAL_PATH)                           \
                  -I$(ORYX_DIR)/include                     \
                  -I$(ORYX_DIR)/include/video               \
                  -I$(ORYX_DIR)/include/audio               \
                  -I$(ORYX_DIR)/include/stream              \
                  -I$(ORYX_DIR)/include/utility             \
                  -I$(ORYX_DIR)/include/configure           \
                  -I$(ORYX_DIR)/stream/include/amf          \
                  -I$(ORYX_DIR)/stream/include/record       \
                  -I$(ORYX_DIR)/stream/include/common       \
                  -I$(ORYX_DIR)/stream/include/common/media \
                  -std=c++11 -Werror

LOCAL_LIBS := libamutil.so     \
              libamosal.so     \
              libamconfig.so   \
              libamstream-amf.so

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
