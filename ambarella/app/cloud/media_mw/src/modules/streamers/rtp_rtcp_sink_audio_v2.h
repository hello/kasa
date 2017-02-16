/*
 * rtp_rtcp_sink_audio_v2.h
 *
 * History:
 *    2014/08/25 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __RTP_RTCP_SINK_AUDIO_V2__
#define __RTP_RTCP_SINK_AUDIO_V2__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CAudioRTPRTCPSinkV2
//
//-----------------------------------------------------------------------
class CAudioRTPRTCPSinkV2: public CObject, public IStreamingTransmiter
{
    typedef CObject inherited;

public:
    static IStreamingTransmiter *Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    virtual void Destroy();
    virtual void Delete();

protected:
    CAudioRTPRTCPSinkV2(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    EECode Construct();
    virtual ~CAudioRTPRTCPSinkV2();

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
    EECode sendDataUDP(CIBuffer *pBuffer);
    EECode sendDataTCP(CIBuffer *pBuffer);
    EECode setupUDPSocket();
    void udpSendData(TU8 *p_data, TMemSize size);
    void tcpSendData(TU8 *p_data, TMemSize size);

    EECode sendDataAAC(CIBuffer *pBuffer);
    EECode sendDataAACTCP(CIBuffer *pBuffer);
    EECode sendDataPCM(CIBuffer *pBuffer);
    EECode sendDataPCMTCP(CIBuffer *pBuffer);
    EECode sendDataMpeg12Audio(CIBuffer *pBuffer);
    EECode sendDataMpeg12AudioTCP(CIBuffer *pBuffer);

    void processRTCP(SSubSessionInfo *sub_session, TTime ntp_time, TU32 timestamp);
    void udpSendSr(SSubSessionInfo *sub_session);
    void tcpSendSr(SSubSessionInfo *sub_session);

    void buildSr(TUint ssrc, TS64 ntp_time, TUint timestamp, TUint packet_count, TUint octet_count);
    void updateSr(TUint ssrc, TS64 ntp_time, TUint timestamp, TUint packet_count, TUint octet_count);
    void buildBye(TUint ssrc);
    void processBYE(SSubSessionInfo *sub_session);
    void updateTimeStamp();

private:
    StreamFormat mFormat;
    CIClockReference   *mpClockReference;

private:
    const volatile SPersistMediaConfig *mpConfig;
    IMsgSink *mpMsgSink;

private:
    CIDoubleLinkedList *mpSubSessionList;
    CIDoubleLinkedList *mpTCPSubSessionList;

    TU8 mbRTPSocketSetup;
    TU8 mPayloadType;
    TU8 mbSrBuilt;
    TU8 mReserved0;

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
    TU8 *mpRTCPBuffer;
    TMemSize mRTCPBufferLength;
    TMemSize mRTCPBufferTotalLength;

private:
    TTime mCurBufferLinearTime;
    TTime mCurBufferNativeTime;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

