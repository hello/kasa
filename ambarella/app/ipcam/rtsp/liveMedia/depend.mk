###############################################
#
# app/ipcam/rtsp/livemedia/depend.mk
#
# 2010/02/03 - [Jian Tang] created file
#
# Copyright (C) 2010, Ambarella, Inc.
#
# All rights reserved. No Part of this file may be reproduced, stored
# in a retrieval system, or transmitted, in any form, or by any means,
# electronic, mechanical, photocopying, recording, or otherwise,
# without the prior consent of Ambarella, Inc.
#
###############################################



################ Local objects:

MP3_SOURCE_OBJS = MP3FileSource.$(OBJ) MP3HTTPSource.$(OBJ) MP3Transcoder.$(OBJ) MP3ADU.$(OBJ) MP3ADUdescriptor.$(OBJ) MP3ADUinterleaving.$(OBJ) MP3ADUTranscoder.$(OBJ) MP3StreamState.$(OBJ) MP3Internals.$(OBJ) MP3InternalsHuffman.$(OBJ) MP3InternalsHuffmanTable.$(OBJ) MP3ADURTPSource.$(OBJ)
MPEG_SOURCE_OBJS = MPEG1or2Demux.$(OBJ) MPEG1or2DemuxedElementaryStream.$(OBJ) MPEGVideoStreamFramer.$(OBJ) MPEG1or2VideoStreamFramer.$(OBJ) MPEG1or2VideoStreamDiscreteFramer.$(OBJ) MPEG4VideoStreamFramer.$(OBJ) MPEG4VideoStreamDiscreteFramer.$(OBJ) H264VideoStreamFramer.$(OBJ) MPEGVideoStreamParser.$(OBJ) MPEG1or2AudioStreamFramer.$(OBJ) MPEG1or2AudioRTPSource.$(OBJ) MPEG4LATMAudioRTPSource.$(OBJ) MPEG4ESVideoRTPSource.$(OBJ) MPEG4GenericRTPSource.$(OBJ) $(MP3_SOURCE_OBJS) MPEG1or2VideoRTPSource.$(OBJ) MPEG2TransportStreamMultiplexor.$(OBJ) MPEG2TransportStreamFromPESSource.$(OBJ) MPEG2TransportStreamFromESSource.$(OBJ) MPEG2TransportStreamFramer.$(OBJ) ADTSAudioFileSource.$(OBJ)
H263_SOURCE_OBJS = H263plusVideoRTPSource.$(OBJ) H263plusVideoStreamFramer.$(OBJ) H263plusVideoStreamParser.$(OBJ)
AC3_SOURCE_OBJS = AC3AudioStreamFramer.$(OBJ) AC3AudioRTPSource.$(OBJ)
DV_SOURCE_OBJS = DVVideoStreamFramer.$(OBJ) DVVideoRTPSource.$(OBJ)
MP3_SINK_OBJS = MP3ADURTPSink.$(OBJ)
MPEG_SINK_OBJS = MPEG1or2AudioRTPSink.$(OBJ) $(MP3_SINK_OBJS) MPEG1or2VideoRTPSink.$(OBJ) MPEG1or2VideoHTTPSink.$(OBJ) MPEG4LATMAudioRTPSink.$(OBJ) MPEG4GenericRTPSink.$(OBJ) MPEG4ESVideoRTPSink.$(OBJ)
H263_SINK_OBJS = H263plusVideoRTPSink.$(OBJ)
H264_SINK_OBJS = H264VideoRTPSink.$(OBJ)
DV_SINK_OBJS = DVVideoRTPSink.$(OBJ)
AC3_SINK_OBJS = AC3AudioRTPSink.$(OBJ)

MISC_SOURCE_OBJS = MediaSource.$(OBJ) FramedSource.$(OBJ) FramedFileSource.$(OBJ) FramedFilter.$(OBJ) ByteStreamFileSource.$(OBJ) ByteStreamMultiFileSource.$(OBJ) BasicUDPSource.$(OBJ) DeviceSource.$(OBJ) AudioInputDevice.$(OBJ) WAVAudioFileSource.$(OBJ) $(MPEG_SOURCE_OBJS) $(H263_SOURCE_OBJS) $(AC3_SOURCE_OBJS) $(DV_SOURCE_OBJS) JPEGVideoSource.$(OBJ) AMRAudioSource.$(OBJ) AMRAudioFileSource.$(OBJ) InputFile.$(OBJ)
MISC_SINK_OBJS = MediaSink.$(OBJ) FileSink.$(OBJ) BasicUDPSink.$(OBJ) AMRAudioFileSink.$(OBJ) H264VideoFileSink.$(OBJ) HTTPSink.$(OBJ) $(MPEG_SINK_OBJS) $(H263_SINK_OBJS) $(H264_SINK_OBJS) $(DV_SINK_OBJS) $(AC3_SINK_OBJS) GSMAudioRTPSink.$(OBJ) JPEGVideoRTPSink.$(OBJ) SimpleRTPSink.$(OBJ) AMRAudioRTPSink.$(OBJ) OutputFile.$(OBJ)
MISC_FILTER_OBJS = uLawAudioFilter.$(OBJ)
TRANSPORT_STREAM_TRICK_PLAY_OBJS = MPEG2IndexFromTransportStream.$(OBJ) MPEG2TransportStreamIndexFile.$(OBJ) MPEG2TransportStreamTrickModeFilter.$(OBJ)

RTP_SOURCE_OBJS = RTPSource.$(OBJ) MultiFramedRTPSource.$(OBJ) SimpleRTPSource.$(OBJ) H261VideoRTPSource.$(OBJ) H264VideoRTPSource.$(OBJ) QCELPAudioRTPSource.$(OBJ) AMRAudioRTPSource.$(OBJ) JPEGVideoRTPSource.$(OBJ)
RTP_SINK_OBJS = RTPSink.$(OBJ) MultiFramedRTPSink.$(OBJ) AudioRTPSink.$(OBJ) VideoRTPSink.$(OBJ)
RTP_INTERFACE_OBJS = RTPInterface.$(OBJ)
RTP_OBJS = $(RTP_SOURCE_OBJS) $(RTP_SINK_OBJS) $(RTP_INTERFACE_OBJS)

RTCP_OBJS = RTCP.$(OBJ) rtcp_from_spec.$(OBJ)
RTSP_OBJS = RTSPServer.$(OBJ) RTSPOverHTTPServer.$(OBJ) RTSPClient.$(OBJ) RTSPCommon.$(OBJ)
SIP_OBJS = SIPClient.$(OBJ)

SESSION_OBJS = MediaSession.$(OBJ) ServerMediaSession.$(OBJ) PassiveServerMediaSubsession.$(OBJ) OnDemandServerMediaSubsession.$(OBJ) FileServerMediaSubsession.$(OBJ) MPEG4VideoFileServerMediaSubsession.$(OBJ) H263plusVideoFileServerMediaSubsession.$(OBJ) WAVAudioFileServerMediaSubsession.$(OBJ) AMRAudioFileServerMediaSubsession.$(OBJ) MP3AudioFileServerMediaSubsession.$(OBJ) MPEG1or2VideoFileServerMediaSubsession.$(OBJ) MPEG1or2FileServerDemux.$(OBJ) MPEG1or2DemuxedServerMediaSubsession.$(OBJ) MPEG2TransportFileServerMediaSubsession.$(OBJ) ADTSAudioFileServerMediaSubsession.$(OBJ) DVVideoFileServerMediaSubsession.$(OBJ)

QUICKTIME_OBJS = QuickTimeFileSink.$(OBJ) QuickTimeGenericRTPSource.$(OBJ)
AVI_OBJS = AVIFileSink.$(OBJ)

MISC_OBJS = DarwinInjector.$(OBJ) BitVector.$(OBJ) StreamParser.$(OBJ) DigestAuthentication.$(OBJ) our_md5.$(OBJ) our_md5hl.$(OBJ) Base64.$(OBJ) Locale.$(OBJ)

LIB_OBJS = Media.$(OBJ) $(MISC_SOURCE_OBJS) $(MISC_SINK_OBJS) \
			$(MISC_FILTER_OBJS) $(RTP_OBJS) $(RTCP_OBJS) $(RTSP_OBJS) \
			$(SIP_OBJS) $(SESSION_OBJS) $(QUICKTIME_OBJS) $(AVI_OBJS) \
			$(TRANSPORT_STREAM_TRICK_PLAY_OBJS) $(MISC_OBJS)

################ End of local objects:

