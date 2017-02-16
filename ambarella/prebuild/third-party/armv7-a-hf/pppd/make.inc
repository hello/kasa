##
## prebuild/third-party/pppd/make.inc
##
## History:
##    2013/06/19 - [Diao Chengdong] Created file
##
## Copyright (C) 2013-2017, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(CONFIG_AMBARELLA_PPPD_SUPPORT), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_pppd
LOCAL_SRCS	:= usr etc

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

endif

