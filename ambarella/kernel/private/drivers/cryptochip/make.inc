##
## kernel/private/drivers/cryptochip/make.inc
##
## History:
##    2015/06/10 - [Zhi He] Created file
##
## Copyright (C) 2015 - 2025, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(CONFIG_CRYPTOCHIP_ATSHA204), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

ifeq ($(BUILD_PRIVATE_ATSHA204_DRIVER_FROM_SOURCE), y)
LOCAL_SRCS	+= $(LOCAL_PATH)/atmel_atsha204/atmel_atsha204.c $(LOCAL_PATH)/atmel_atsha204/atmel_atsha204.h
else
LOCAL_SRCS	+= $(LOCAL_PATH)/atmel_atsha204/atmel_atsha204.o_shipped
endif

LOCAL_SRCS	+= $(LOCAL_PATH)/atmel_atsha204/Kbuild

include $(BUILD_DRIVER)

endif

