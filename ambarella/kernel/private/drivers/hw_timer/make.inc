##
## kernel/private/drivers/hw_timer/make.inc
##
## History:
##    2012/10/19 - [Zhaoyang Chen] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_HW_TIMER), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_SRCS	+= $(wildcard $(LOCAL_PATH)/arch_$(AMBARELLA_ARCH)/*.c) \
			$(wildcard $(LOCAL_PATH)/arch_$(AMBARELLA_ARCH)/*.h)
LOCAL_SRCS	+= $(LOCAL_PATH)/arch_$(AMBARELLA_ARCH)/Kbuild

include $(BUILD_DRIVER)

endif

