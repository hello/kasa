##
## app/ipcam/rtsp/mediaServer/make.inc
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
RTSP_LIBS	:= libliveMedia.a libgroupsock.a \
		libBasicUsageEnvironment.a libUsageEnvironment.a
RTSP_INCLUDES	:= -I$(LOCAL_PATH)/../BasicUsageEnvironment/include \
		-I$(LOCAL_PATH)/../groupsock/include \
		-I$(LOCAL_PATH)/../liveMedia/include \
		-I$(LOCAL_PATH)/../UsageEnvironment/include
C_CFLAGS		:= -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64
CPP_CFLAGS	:= -DBSD=1 -g -O2

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= live555MediaServer
LOCAL_SRCS	:= $(LOCAL_PATH)/live555MediaServer.cpp \
		$(LOCAL_PATH)/DynamicRTSPServer.cpp
LOCAL_LDFLAGS	:= -lpthread
LOCAL_LIBS	:= $(RTSP_LIBS)
LOCAL_CFLAGS	:= $(C_CFLAGS) $(CPP_CFLAGS) $(RTSP_INCLUDES)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

