##
## IntelliVision CORPORATION
##
## COPYRIGHT STATEMENT.
## This software has been provided pursuant to a License Agreement
## containing restrictions on its use.  The software contains trade
## secrets and proprietary information of IntelliVision Corporation
## and is protected by Federal copyright law.  It should not
## be copied or distributed in any form or medium, disclosed to
## any third party, or taken outside IntelliVision offices,
## or used in any manner, except with prior written
## agreement with IntelliVision Corporation. All rights reserved.
## www.Intelli-Vision.com
##
##

ifeq ($(CONFIG_AMBARELLA_IVA_SUPPORT), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_iva
LOCAL_SRCS	:= usr

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "$@ currently is not available for hard-float."
#	$(call prebuild-links)
#	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

endif
