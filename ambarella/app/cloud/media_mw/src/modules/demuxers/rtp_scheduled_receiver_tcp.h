/*
 * rtp_scheduled_receiver_tcp.h
 *
 * History:
 *    2014/12/13 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __RTP_SCHEDULED_RECEIVER_TCP_H__
#define __RTP_SCHEDULED_RECEIVER_TCP_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CRTPScheduledReceiverTCP
//
//-----------------------------------------------------------------------
class CRTPScheduledReceiverTCP
    : public CObject
    , public IScheduledClient
    , public IScheduledRTPReceiver
    , public IEventListener
{
    typedef CObject inherited;

public:
    CRTPScheduledReceiverTCP(TUint index, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    virtual ~CRTPScheduledReceiverTCP();

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
    virtual void PrintStatus();

public:
    virtual void EventNotify(EEventType type, TU64 param1, TPointer param2);

private:
    void postEngineMsg(EMSGType msg_type);
    void requestMemory();
    void releaseMemory();
    void sendVideoExtraDataBuffer();
    void sendVideoData(TU8 nal_type);
    void sendAudioExtraDataBuffer();
    void sendAudioData();
    EECode doSetVideoExtraData(TU8 *p_extradata, TMemSize extradata_size);
    EECode doSetAudioExtraData(TU8 *p_extradata, TMemSize extradata_size);
    void sendEOS();

private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpMsgSink;
    CIClockReference *mpSystemClockReference;
    SClockListener *mpWatchDog;
    IMutex *mpMutex;

    StreamFormat mVideoFormat;
    StreamFormat mAudioFormat;

    COutputPin *mpOutputPin[EConstMaxDemuxerMuxerStreamNumber];
    CBufferPool *mpBufferPool[EConstMaxDemuxerMuxerStreamNumber];
    IMemPool *mpMemPool[EConstMaxDemuxerMuxerStreamNumber];
    TInt mRequestMemorySize[EConstMaxDemuxerMuxerStreamNumber];

    TTime mFakePts[EConstMaxDemuxerMuxerStreamNumber];
    TU8 mPayloadType[EConstMaxDemuxerMuxerStreamNumber];
    TU8 mbSendExtraData[EConstMaxDemuxerMuxerStreamNumber];

    TSocketHandler mRTSPSocket;
    TU32 mReconnectTag;

private:
    TU32 msState;

    TU8 mCurrentTrack;
    TU8 mbInitialized;
    TU8 mbEnableRCTP;
    TU8 mbSetTimeStamp;

    TU8 mSentErrorMsg;
    TU8 mCurPayloadType;
    TU8 mMarkFlag;
    TU8 mHaveFU;

    TU8 mFilledStartCode;
    TU8 mbReadRemaingVideoData;
    TU8 mbReadRemaingAudioData;
    TU8 mbWaitFirstSpsPps;

    TInt mDataLength;

    TU32 mVideoWidth;
    TU32 mVideoHeight;
    TU32 mVideoFramerateNum;
    TU32 mVideoFramerateDen;
    float mVideoFramerate;

    TU8 mAudioChannelNumber;
    TU8 mAudioSampleFormat;
    TU8 mbAudioChannelInterlave;
    TU8 mbParseMultipleAccessUnit;

    TU32 mAudioSamplerate;
    TU32 mAudioBitrate;
    TU32 mAudioFrameSize;

private:
    TU8 *mpMemoryStart;
    //TU8* mpCurrentPointer;
    TInt mTotalWritenLength;
    TInt mMemorySize;

private:
    //from sdp
    TU8 *mpVideoExtraDataFromSDP;
    TU32 mVideoExtraDataSize;
    TU8 *mpH264spspps;
    TU32 mH264spsppsSize;

    TU8 *mpAudioExtraData;
    TU32 mAudioExtraDataSize;

private:
    volatile TTime mLastAliveTimeStamp;

private:
    TU8 mFrameHeader[4];
    TU8 mRTPHeader[DRTP_HEADER_FIXED_LENGTH];
    TU8 mRTCPPacket[256];
    TU8 mAACHeader[4];
    TInt mCurrentFrameHeaderLength;
    TInt mCurrentRTPHeaderLength;
    TInt mCurrentRTPDataLength;
    TInt mCurrentRTCPPacketLength;
    TInt mCurrentAudioCodecSpecificLength;

private:
    CIBuffer mPersistExtradataBuffer;
    CIBuffer mPersistEOSBuffer;

private:
    TEmbededDataProcessingCallBack mVideoPostProcessingCallback;
    void *mpVideoPostProcessingCallbackContext;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

