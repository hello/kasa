##
## prebuild/third-party/openssh/make.inc
##
## History:
##    2014/02/27 - [Yupeng Chang] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(CONFIG_AMBARELLA_INIT_SYSTEMD), y)
ifeq ($(CONFIG_AMBARELLA_AUTO_TELNETD), y)

LOCAL_PATH := $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET := prebuild_telnetd
LOCAL_SRCS   := usr

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): basic_fs
$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
	$(call overwrite-bb)
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

endif
endif
