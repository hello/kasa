##
## kernel/private/drivers/vout/make.inc
##
## History:
##    2012/06/05 - [Cao Rongrong] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_VOUT), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

UGLY_ARCH	:= true
LOCAL_SRCS	:= $(wildcard $(LOCAL_PATH)/*.c) $(wildcard $(LOCAL_PATH)/*.h)
LOCAL_SRCS	+= $(wildcard $(LOCAL_PATH)/arch_$(AMBARELLA_ARCH)/*.c) \
			$(wildcard $(LOCAL_PATH)/arch_$(AMBARELLA_ARCH)/*.h)
LOCAL_SRCS	+= $(LOCAL_PATH)/arch_$(AMBARELLA_ARCH)/Kbuild

include $(BUILD_DRIVER)

###

include $(call all-makefiles-under, $(LOCAL_PATH)/*)
include $(call all-makefiles-under, $(LOCAL_PATH)/)

endif

