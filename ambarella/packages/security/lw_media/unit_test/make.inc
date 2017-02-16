##
## ambarella/packages/security/lw_media/unit_test/make.inc
##
## History:
##   2015-9-18 - [Zhi He] created file
##
## Copyright (C) 2015-2025, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

ifeq ($(BUILD_AMBARELLA_LW_MEDIA_UT), y)

LW_MEDIA_UT_DIR := $(call my-dir)
LOCAL_PATH := $(LW_MEDIA_UT_DIR)

LOCAL_TARGET := test_lwmedia
LOCAL_SRCS   := $(LW_MEDIA_UT_DIR)/test_media.cpp
LOCAL_CFLAGS := -I$(LW_MEDIA_UT_DIR)/../include -Werror

LOCAL_LDFLAGS :=

ifeq ($(BUILD_AMBARELLA_LW_MEDIA_ALSA_LIBRARY), y)
LOCAL_LDFLAGS += -L$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/usr/lib -lasound
endif

ifeq ($(BUILD_AMBARELLA_LW_MEDIA_FFMPEG_LIBRARY), y)
LOCAL_LDFLAGS += -L$(PREBUILD_3RD_PARTY_DIR)/ffmpeg/usr/lib -lavformat -lavcodec -lavutil
endif

ifeq ($(BUILD_AMBARELLA_LW_MEDIA_AMBA_AAC_LIBRARY), y)
LOCAL_LDFLAGS += -L$(PREBUILD_3RD_PARTY_DIR)/aac/lib -laacdec -laacenc
endif

LOCAL_LIBS	:= liblwmedia.a
LOCAL_LDFLAGS += -lpthread -ldl -lrt -llwmedia

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif