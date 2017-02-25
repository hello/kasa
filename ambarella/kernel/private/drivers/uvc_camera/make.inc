##
## kernel/private/drivers/uvc_camera/make.inc
##
## History:
##    2015/10/26 - [Jorney] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_UVC_CAMERA), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_SRCS	+=  $(wildcard $(LOCAL_PATH)/uvc/*.c) \
		     	$(wildcard $(LOCAL_PATH)/uvc/*.h)


LOCAL_SRCS	+= $(wildcard $(LOCAL_PATH)/*.h)

LOCAL_SRCS	+= $(LOCAL_PATH)/uvc/Kbuild

include $(BUILD_DRIVER)

endif

