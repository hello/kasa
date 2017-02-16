##
## prebuild/third-party/dosfstools/make.inc
##
## History:
##    2014/03/11 - [Yupeng Chang] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(CONFIG_AMBARELLA_DOSFSTOOS_SUPPORT), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET     := prebuild_dosfstools
LOCAL_SRCS       := sbin

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): basic_fs
	$(call prebuild-links)
	$(call overwrite-bb)
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

endif

