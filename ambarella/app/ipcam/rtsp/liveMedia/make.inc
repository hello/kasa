##
## app/ipcam/rtsp/liveMedia/make.inc
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

MP3_SOURCE_OBJS = $(LOCAL_PATH)/MP3FileSource.cpp \
		$(LOCAL_PATH)/MP3HTTPSource.cpp \
		$(LOCAL_PATH)/MP3Transcoder.cpp \
		$(LOCAL_PATH)/MP3ADU.cpp \
		$(LOCAL_PATH)/MP3ADUdescriptor.cpp \
		$(LOCAL_PATH)/MP3ADUinterleaving.cpp \
		$(LOCAL_PATH)/MP3ADUTranscoder.cpp \
		$(LOCAL_PATH)/MP3StreamState.cpp \
		$(LOCAL_PATH)/MP3Internals.cpp \
		$(LOCAL_PATH)/MP3InternalsHuffman.cpp \
		$(LOCAL_PATH)/MP3InternalsHuffmanTable.cpp \
		$(LOCAL_PATH)/MP3ADURTPSource.cpp
MPEG_SOURCE_OBJS = $(LOCAL_PATH)/MPEG1or2Demux.cpp \
		$(LOCAL_PATH)/MPEG1or2DemuxedElementaryStream.cpp \
		$(LOCAL_PATH)/MPEGVideoStreamFramer.cpp \
		$(LOCAL_PATH)/MPEG1or2VideoStreamFramer.cpp \
		$(LOCAL_PATH)/MPEG1or2VideoStreamDiscreteFramer.cpp \
		$(LOCAL_PATH)/MPEG4VideoStreamFramer.cpp \
		$(LOCAL_PATH)/MPEG4VideoStreamDiscreteFramer.cpp \
		$(LOCAL_PATH)/H264VideoStreamFramer.cpp \
		$(LOCAL_PATH)/MPEGVideoStreamParser.cpp \
		$(LOCAL_PATH)/MPEG1or2AudioStreamFramer.cpp \
		$(LOCAL_PATH)/MPEG1or2AudioRTPSource.cpp \
		$(LOCAL_PATH)/MPEG4LATMAudioRTPSource.cpp \
		$(LOCAL_PATH)/MPEG4ESVideoRTPSource.cpp \
		$(LOCAL_PATH)/MPEG4GenericRTPSource.cpp \
		$(MP3_SOURCE_OBJS) \
		$(LOCAL_PATH)/MPEG1or2VideoRTPSource.cpp \
		$(LOCAL_PATH)/MPEG2TransportStreamMultiplexor.cpp \
		$(LOCAL_PATH)/MPEG2TransportStreamFromPESSource.cpp \
		$(LOCAL_PATH)/MPEG2TransportStreamFromESSource.cpp \
		$(LOCAL_PATH)/MPEG2TransportStreamFramer.cpp \
		$(LOCAL_PATH)/ADTSAudioFileSource.cpp
H263_SOURCE_OBJS = $(LOCAL_PATH)/H263plusVideoRTPSource.cpp \
		$(LOCAL_PATH)/H263plusVideoStreamFramer.cpp \
		$(LOCAL_PATH)/H263plusVideoStreamParser.cpp
AC3_SOURCE_OBJS = $(LOCAL_PATH)/AC3AudioStreamFramer.cpp \
		$(LOCAL_PATH)/AC3AudioRTPSource.cpp
DV_SOURCE_OBJS = $(LOCAL_PATH)/DVVideoStreamFramer.cpp \
		$(LOCAL_PATH)/DVVideoRTPSource.cpp
MP3_SINK_OBJS = $(LOCAL_PATH)/MP3ADURTPSink.cpp
MPEG_SINK_OBJS = $(LOCAL_PATH)/MPEG1or2AudioRTPSink.cpp \
		$(MP3_SINK_OBJS) $(LOCAL_PATH)/MPEG1or2VideoRTPSink.cpp \
		$(LOCAL_PATH)/MPEG1or2VideoHTTPSink.cpp \
		$(LOCAL_PATH)/MPEG4LATMAudioRTPSink.cpp \
		$(LOCAL_PATH)/MPEG4GenericRTPSink.cpp \
		$(LOCAL_PATH)/MPEG4ESVideoRTPSink.cpp
H263_SINK_OBJS = $(LOCAL_PATH)/H263plusVideoRTPSink.cpp
H264_SINK_OBJS = $(LOCAL_PATH)/H264VideoRTPSink.cpp
DV_SINK_OBJS = $(LOCAL_PATH)/DVVideoRTPSink.cpp
AC3_SINK_OBJS = $(LOCAL_PATH)/AC3AudioRTPSink.cpp

MISC_SOURCE_OBJS = $(LOCAL_PATH)/MediaSource.cpp \
		$(LOCAL_PATH)/FramedSource.cpp \
		$(LOCAL_PATH)/FramedFileSource.cpp \
		$(LOCAL_PATH)/FramedFilter.cpp \
		$(LOCAL_PATH)/ByteStreamFileSource.cpp \
		$(LOCAL_PATH)/ByteStreamMultiFileSource.cpp \
		$(LOCAL_PATH)/BasicUDPSource.cpp \
		$(LOCAL_PATH)/DeviceSource.cpp \
		$(LOCAL_PATH)/AudioInputDevice.cpp \
		$(LOCAL_PATH)/WAVAudioFileSource.cpp \
		$(MPEG_SOURCE_OBJS) $(H263_SOURCE_OBJS) \
		$(AC3_SOURCE_OBJS) $(DV_SOURCE_OBJS) \
		$(LOCAL_PATH)/JPEGVideoSource.cpp \
		$(LOCAL_PATH)/AMRAudioSource.cpp \
		$(LOCAL_PATH)/AMRAudioFileSource.cpp \
		$(LOCAL_PATH)/InputFile.cpp
MISC_SINK_OBJS = $(LOCAL_PATH)/MediaSink.cpp \
		$(LOCAL_PATH)/FileSink.cpp \
		$(LOCAL_PATH)/BasicUDPSink.cpp \
		$(LOCAL_PATH)/AMRAudioFileSink.cpp \
		$(LOCAL_PATH)/H264VideoFileSink.cpp \
		$(LOCAL_PATH)/HTTPSink.cpp \
		$(MPEG_SINK_OBJS) $(H263_SINK_OBJS) \
		$(H264_SINK_OBJS) $(DV_SINK_OBJS) $(AC3_SINK_OBJS) \
		$(LOCAL_PATH)/GSMAudioRTPSink.cpp \
		$(LOCAL_PATH)/JPEGVideoRTPSink.cpp \
		$(LOCAL_PATH)/SimpleRTPSink.cpp \
		$(LOCAL_PATH)/AMRAudioRTPSink.cpp \
		$(LOCAL_PATH)/OutputFile.cpp
MISC_FILTER_OBJS = $(LOCAL_PATH)/uLawAudioFilter.cpp
TRANSPORT_STREAM_TRICK_PLAY_OBJS = \
		$(LOCAL_PATH)/MPEG2IndexFromTransportStream.cpp \
		$(LOCAL_PATH)/MPEG2TransportStreamIndexFile.cpp \
		$(LOCAL_PATH)/MPEG2TransportStreamTrickModeFilter.cpp

RTP_SOURCE_OBJS = $(LOCAL_PATH)/RTPSource.cpp \
		$(LOCAL_PATH)/MultiFramedRTPSource.cpp \
		$(LOCAL_PATH)/SimpleRTPSource.cpp \
		$(LOCAL_PATH)/H261VideoRTPSource.cpp \
		$(LOCAL_PATH)/H264VideoRTPSource.cpp \
		$(LOCAL_PATH)/QCELPAudioRTPSource.cpp \
		$(LOCAL_PATH)/AMRAudioRTPSource.cpp \
		$(LOCAL_PATH)/JPEGVideoRTPSource.cpp
RTP_SINK_OBJS = $(LOCAL_PATH)/RTPSink.cpp \
		$(LOCAL_PATH)/MultiFramedRTPSink.cpp \
		$(LOCAL_PATH)/AudioRTPSink.cpp \
		$(LOCAL_PATH)/VideoRTPSink.cpp
RTP_INTERFACE_OBJS = $(LOCAL_PATH)/RTPInterface.cpp
RTP_OBJS = $(RTP_SOURCE_OBJS) $(RTP_SINK_OBJS) $(RTP_INTERFACE_OBJS)

RTCP_OBJS = $(LOCAL_PATH)/RTCP.cpp $(LOCAL_PATH)/rtcp_from_spec.cpp
RTSP_OBJS = $(LOCAL_PATH)/RTSPServer.cpp $(LOCAL_PATH)/RTSPOverHTTPServer.cpp \
		$(LOCAL_PATH)/RTSPClient.cpp $(LOCAL_PATH)/RTSPCommon.cpp
SIP_OBJS = $(LOCAL_PATH)/SIPClient.cpp

SESSION_OBJS = $(LOCAL_PATH)/MediaSession.cpp \
		$(LOCAL_PATH)/ServerMediaSession.cpp \
		$(LOCAL_PATH)/PassiveServerMediaSubsession.cpp \
		$(LOCAL_PATH)/OnDemandServerMediaSubsession.cpp \
		$(LOCAL_PATH)/FileServerMediaSubsession.cpp \
		$(LOCAL_PATH)/MPEG4VideoFileServerMediaSubsession.cpp \
		$(LOCAL_PATH)/H263plusVideoFileServerMediaSubsession.cpp \
		$(LOCAL_PATH)/WAVAudioFileServerMediaSubsession.cpp \
		$(LOCAL_PATH)/AMRAudioFileServerMediaSubsession.cpp \
		$(LOCAL_PATH)/MP3AudioFileServerMediaSubsession.cpp \
		$(LOCAL_PATH)/MPEG1or2VideoFileServerMediaSubsession.cpp \
		$(LOCAL_PATH)/MPEG1or2FileServerDemux.cpp \
		$(LOCAL_PATH)/MPEG1or2DemuxedServerMediaSubsession.cpp \
		$(LOCAL_PATH)/MPEG2TransportFileServerMediaSubsession.cpp \
		$(LOCAL_PATH)/ADTSAudioFileServerMediaSubsession.cpp \
		$(LOCAL_PATH)/DVVideoFileServerMediaSubsession.cpp

QUICKTIME_OBJS = $(LOCAL_PATH)/QuickTimeFileSink.cpp \
		$(LOCAL_PATH)/QuickTimeGenericRTPSource.cpp
AVI_OBJS = $(LOCAL_PATH)/AVIFileSink.cpp

MISC_OBJS = $(LOCAL_PATH)/DarwinInjector.cpp \
		$(LOCAL_PATH)/BitVector.cpp \
		$(LOCAL_PATH)/StreamParser.cpp \
		$(LOCAL_PATH)/DigestAuthentication.cpp \
		$(LOCAL_PATH)/our_md5.c $(LOCAL_PATH)/our_md5hl.c \
		$(LOCAL_PATH)/Base64.cpp $(LOCAL_PATH)/Locale.cpp

LIB_SRCS = $(LOCAL_PATH)/Media.cpp $(MISC_SOURCE_OBJS) $(MISC_SINK_OBJS) \
		$(MISC_FILTER_OBJS) $(RTP_OBJS) $(RTCP_OBJS) $(RTSP_OBJS) \
		$(SIP_OBJS) $(SESSION_OBJS) $(QUICKTIME_OBJS) $(AVI_OBJS) \
		$(TRANSPORT_STREAM_TRICK_PLAY_OBJS) $(MISC_OBJS)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= libliveMedia.a
LOCAL_SRCS	:= $(LIB_SRCS)
LOCAL_CFLAGS	:= -I$(LOCAL_PATH)/include \
		-I$(LOCAL_PATH)/../UsageEnvironment/include \
		-I$(LOCAL_PATH)/../groupsock/include
LOCAL_CFLAGS	+= $(C_CFLAGS) $(CPP_CFLAGS)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

