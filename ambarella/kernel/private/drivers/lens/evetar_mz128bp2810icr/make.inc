##
## kernel/private/drivers/lens/evetar_mz128bp2810icr/make.inc
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

ifeq ($(CONFIG_EVETAR_MZ128BP2810ICR), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_SRCS  := $(wildcard $(LOCAL_PATH)/*.c) $(wildcard $(LOCAL_PATH)/*.h)
LOCAL_SRCS  += $(LOCAL_PATH)/Kbuild

include $(BUILD_DRIVER)

endif
