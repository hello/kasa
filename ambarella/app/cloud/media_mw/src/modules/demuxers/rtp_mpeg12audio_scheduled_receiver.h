/*
 * rtp_mpeg12audio_scheduled_receiver.h
 *
 * History:
 *    2013/11/24 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __RTP_MPEG12AUDIO_SCHEDULED_RECEIVER_H__
#define __RTP_MPEG12AUDIO_SCHEDULED_RECEIVER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CRTPMpeg12AudioScheduledReceiver
//
//-----------------------------------------------------------------------
class CRTPMpeg12AudioScheduledReceiver
    : public CObject
    , virtual public IScheduledClient
    , virtual public IScheduledRTPReceiver
{
    typedef CObject inherited;

public:
    CRTPMpeg12AudioScheduledReceiver(TUint index, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    virtual ~CRTPMpeg12AudioScheduledReceiver();

public:
    virtual EECode Scheduling(TUint times = 1, TU32 inout_mask = 0);
    virtual TInt IsPassiveMode() const;
    virtual TSchedulingHandle GetWaitHandle() const;
    virtual TSchedulingUnit HungryScore() const;
    virtual EECode EventHandling(EECode err);
    virtual TU8 GetPriority() const;

public:
    virtual CObject *GetObject0() const;

public:
    virtual EECode Initialize(SRTPContext *context, SStreamCodecInfo *p_stream_info, TUint number_of_content = 1);
    virtual EECode ReInitialize(SRTPContext *context, SStreamCodecInfo *p_stream_info, TUint number_of_content = 1);
    virtual EECode Flush();

    virtual EECode Purge();
    virtual EECode SetExtraData(TU8 *p_extradata, TU32 extradata_size, TU32 index);

    virtual void SetVideoDataPostProcessingCallback(void *callback_context, void *callback);
    virtual void SetAudioDataPostProcessingCallback(void *callback_context, void *callback);

public:
    void PrintStatus();

public:
    void writeRR();
    void updateRR();
    void sendRR();
    void receiveSR();
    void checkSRsendRR();

private:
    TU8 mTrackID;
    TU8 mbRun;
    TU8 mbInitialized;
    TU8 mbEnableRCTP;

    StreamType mType;
    StreamFormat mFormat;

    IMsgSink *mpMsgSink;
    const volatile SPersistMediaConfig *mpPersistMediaConfig;

    COutputPin *mpOutputPin;
    CBufferPool *mpBufferPool;
    IMemPool *mpMemPool;

    TInt mRTSPSocket;
    TInt mRTPSocket;
    TInt mRTCPSocket;

    ISimpleQueue *mpCmdSimpleQueue;

private:
    struct sockaddr mSrcAddr;
    struct sockaddr mSrcRTCPAddr;
    socklen_t mFromLen;

private:
    TUint msState;

private:
    TInt mRTPHeaderLength;

    TU8 *mpMemoryStart;
    TInt mReadLen;

private:
    TTime mRTPTimeStamp;
    TU16 mRTPCurrentSeqNumber;
    TU16 mRTPLastSeqNumber;

    CIBuffer *mpBuffer;

    TInt mRequestMemorySize;

    TUint mCmd;

private:
    TU8 mDebugWaitMempoolFlag;
    TU8 mDebugWaitBufferpoolFlag;
    TU8 mDebugWaitReadSocketFlag;
    TU8 mbGetSSRC;

private:
    TU8 mAudioChannelNumber;
    TU8 mAudioSampleFormat;
    TU8 mbAudioChannelInterlave;
    TU8 mReserved5;

    TU32 mAudioSamplerate;
    TU32 mAudioBitrate;
    TU32 mAudioFrameSize;

private:
    TU8 mbSendSyncPointBuffer;
    TU8 mPriority;
    TU16 mBufferSessionNumber;

private:
    TU8 mReservedData[32];

private:
    TU8 *mpExtraData;
    TUint mnExtraDataSize;

private:
    TTime mLastRtcpNtpTime;
    TTime mFirstRtcpNtpTime;
    TTime mLastRtcpTimeStamp;
    TTime mRtcpTimeStampOffset;

    TUint mPacketCount;
    TUint mOctetCount;
    TUint mLastOctetCount;

    TU32 mServerSSRC;
    TU32 mSSRC;
    TU32 mPacketSSRC;

    TU32 mRTCPCurrentTick;
    TU32 mRTCPCoolDown;

    TChar mCName[128];

    TU8 mRTCPBuffer[256];
    TUint mRTCPDataLen;

    TU8 mRTCPReadBuffer[256];
    TUint mRTCPReadDataLen;

    SRTPStatistics mRTPStatistics;

private:
    TTime mFakePts;
    TU32 mLastPrintLostCount;
};


DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

