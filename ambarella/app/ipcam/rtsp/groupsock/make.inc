##
## app/ipcam/rtsp/groupsock/make.inc
##
## History:
##    2012/06/25 - [Jian Tang] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH	:= $(call my-dir)
C_CFLAGS		:= -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64
CPP_CFLAGS	:= -DBSD=1 -g -O2

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= libgroupsock.a
LOCAL_SRCS	:= $(LOCAL_PATH)/inet.c \
		$(LOCAL_PATH)/GroupsockHelper.cpp \
		$(LOCAL_PATH)/GroupEId.cpp \
		$(LOCAL_PATH)/Groupsock.cpp \
		$(LOCAL_PATH)/NetInterface.cpp \
		$(LOCAL_PATH)/NetAddress.cpp \
		$(LOCAL_PATH)/IOHandlers.cpp
LOCAL_CFLAGS	:= -I$(LOCAL_PATH)/include \
		-I$(LOCAL_PATH)/../UsageEnvironment/include
LOCAL_CFLAGS	+= $(C_CFLAGS) $(CPP_CFLAGS)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

