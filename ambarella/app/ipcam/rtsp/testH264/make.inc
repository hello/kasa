##
## app/ipcam/rtsp/testH264/make.inc
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
		-I$(LOCAL_PATH)/../UsageEnvironment/include \
		-I$(AMB_TOPDIR)/packages/bsreader
C_CFLAGS		:= -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64
CPP_CFLAGS	:= -g -O2

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= rtsp_server
LOCAL_SRCS	:= $(LOCAL_PATH)/rtsp_server.cpp \
		$(LOCAL_PATH)/H264VideoFileServerMediaSubsession.cpp \
		$(LOCAL_PATH)/MyH264VideoStreamFramer.cpp \
		$(LOCAL_PATH)/MyH264VideoRTPSink.cpp \
		$(LOCAL_PATH)/H264StreamParser.cpp \
		$(LOCAL_PATH)/MyJPEGVideoSource.cpp \
		$(LOCAL_PATH)/MyJPEGVideoRTPSink.cpp
LOCAL_LDFLAGS	:= -lpthread

ifeq ($(CONFIG_ARCH_A5S), y)
LOCAL_LIBS	:= libbsreader.a $(RTSP_LIBS) libcavlc.a
endif
ifeq ($(CONFIG_ARCH_S2), y)
LOCAL_LIBS	:= libbsreader.a $(RTSP_LIBS)
endif
ifeq ($(CONFIG_ARCH_S2L), y)
LOCAL_LIBS	:= libbsreader.a $(RTSP_LIBS)
endif
LOCAL_CFLAGS	:= $(C_CFLAGS) $(CPP_CFLAGS) $(RTSP_INCLUDES)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(APP_INSTALL_PATH)/
	@cp -dpRf $< $(APP_INSTALL_PATH)/
	@chmod +x $(APP_INSTALL_PATH)/$@
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))


###
include $(CLEAR_VARS)

LOCAL_TARGET	:= testH264VideoStreamer
LOCAL_SRCS	:= $(LOCAL_PATH)/testH264VideoStreamer.cpp \
		$(LOCAL_PATH)/MyH264VideoStreamFramer.cpp \
		$(LOCAL_PATH)/MyH264VideoRTPSink.cpp \
		$(LOCAL_PATH)/H264StreamParser.cpp \
		$(LOCAL_PATH)/MyJPEGVideoSource.cpp \
		$(LOCAL_PATH)/MyJPEGVideoRTPSink.cpp
LOCAL_LDFLAGS	:= -lpthread
ifeq ($(CONFIG_ARCH_A5S), y)
LOCAL_LIBS	:= libbsreader.a $(RTSP_LIBS) libcavlc.a
endif
ifeq ($(CONFIG_ARCH_S2), y)
LOCAL_LIBS	:= libbsreader.a $(RTSP_LIBS)
endif
ifeq ($(CONFIG_ARCH_S2L), y)
LOCAL_LIBS	:= libbsreader.a $(RTSP_LIBS)
endif
LOCAL_CFLAGS	:= $(C_CFLAGS) $(CPP_CFLAGS) $(RTSP_INCLUDES)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

