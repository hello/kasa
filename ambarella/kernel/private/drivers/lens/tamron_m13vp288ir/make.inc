##
## kernel/private/drivers/lens/tamron_m13vp288ir/make.inc
##
## History:
##    2014/10/10 - [Peter Jiao] Created file
##
## Copyright (C) 2014-2018, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(CONFIG_TAMRON_M13VP288IR), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_SRCS  := $(wildcard $(LOCAL_PATH)/*.c) $(wildcard $(LOCAL_PATH)/*.h)
LOCAL_SRCS  += $(LOCAL_PATH)/Kbuild

include $(BUILD_DRIVER)

endif
