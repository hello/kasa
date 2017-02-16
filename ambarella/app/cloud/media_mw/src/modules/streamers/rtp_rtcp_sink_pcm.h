/*
 * rtp_rtcp_sink_pcm.h
 *
 * History:
 *    2013/11/07 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __RTP_RTCP_SINK_PCM__
#define __RTP_RTCP_SINK_PCM__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CPCMRTPRTCPSink
//
//-----------------------------------------------------------------------
class CPCMRTPRTCPSink: public CObject, public IStreamingTransmiter
{
    typedef CObject inherited;

public:
    static IStreamingTransmiter *Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    virtual void Destroy();
    virtual void Delete();

protected:
    CPCMRTPRTCPSink(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    EECode Construct();
    virtual ~CPCMRTPRTCPSink();

public:
    virtual EECode SetupContext(StreamTransportType type, ProtocolType protocol_type, const SStreamCodecInfos *infos, TTime start_time = 0);
    virtual EECode DestroyContext();
    virtual EECode UpdateStreamFormat(StreamType type, StreamFormat format);

    virtual EECode Start();
    virtual EECode Stop();

    virtual EECode SetExtraData(TU8 *pdata, TMemSize size);
    virtual EECode SendData(CIBuffer *pBuffer);

    virtual EECode AddSubSession(SSubSessionInfo *p_sub_session);
    virtual EECode RemoveSubSession(SSubSessionInfo *p_sub_session);
    virtual EECode SetSrcPort(TSocketPort port, TSocketPort port_ext);
    virtual EECode GetSrcPort(TSocketPort &port, TSocketPort &port_ext);

protected:
    void udpSendData(TU8 *p_data, TMemSize size, TTime pts);
    EECode setupUDPSocket();

private:
    StreamTransportType mStreamingType;
    ProtocolType mProtocolType;
    StreamFormat mFormat;
    StreamType mType;
    CIClockReference   *mpClockReference;
    IMutex *mpMutex;

private:
    const volatile SPersistMediaConfig *mpConfig;
    IMsgSink *mpMsgSink;

private:
    CIDoubleLinkedList mSubSessionList;
    TU8 mbRTPSocketSetup;
    TU8 mPayloadType;
    TU8 mReserved0[2];

private:
    TSocketHandler mRTPSocket;
    TSocketHandler mRTCPSocket;
    TSocketPort mRTPPort;
    TSocketPort mRTCPPort;

private:
    TU8 *mpRTPBuffer;
    TUint mRTPBufferLength;
    TUint mRTPBufferTotalLength;

private:
    TU8 *mpExtraData;
    TMemSize mExtraDataSize;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

