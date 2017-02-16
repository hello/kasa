##
## prebuild/third-party/armv7-a-hf/audiofile/make.inc
##
## History:
##    2012/05/30 - [Cao Rongrong] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(CONFIG_AMBARELLA_AUDIOFILE_SUPPORT), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_audiofile
LOCAL_SRCS	:= usr

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
ifneq ($(CONFIG_AMBARELLA_AUDIOFILE_TOOLS_SUPPORT), y)
	@rm -f $(FAKEROOT_DIR)/usr/bin/sfconvert
	@rm -f $(FAKEROOT_DIR)/usr/bin/sfinfo
endif
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

endif

