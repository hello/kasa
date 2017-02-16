##
## prebuild/third-party/armv7-a-hf/libndp/make.inc
##
## History:
##    2012/10/17 - [Yupeng Chang] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(CONFIG_AMBARELLA_LIBNDP_SUPPORT), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_libndp
LOCAL_SRCS	:= usr

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
ifneq ($(CONFIG_AMBARELLA_LIBNDP_TOOL_SUPPORT), y)
	@rm -f $(FAKEROOT_DIR)/usr/bin/ndptool
endif
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

endif

