/*
 * ts_demuxer.h
 *
 * History:
 *    2014/04/21 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef _TS_DEMUXER_H_
#define _TS_DEMUXER_H_

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

class CTSDemuxer
    : public CActiveObject
    , public IDemuxer
    , public IEventListener
{
    typedef CActiveObject inherited;

public:
    static IDemuxer *Create(const char *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TUint index);
    virtual CObject *GetObject0() const;
    void Delete();

private:
    CTSDemuxer(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TUint index);
    EECode Construct();
    virtual ~CTSDemuxer();

public:
    virtual EECode SetupOutput(COutputPin *p_output_pins[], CBufferPool *p_bufferpools[],  IMemPool *p_mempools[], IMsgSink *p_msg_sink);
    virtual EECode SetupContext(TChar *url, void *p_agent = NULL, TU8 priority = 0, TU32 request_receive_buffer_size = 0, TU32 request_send_buffer_size = 0);
    virtual EECode DestroyContext();
    virtual EECode ReconnectServer();

    virtual EECode Seek(TTime &target_time, ENavigationPosition position = ENavigationPosition_Invalid);
    virtual EECode Start();
    virtual EECode Stop();

    virtual EECode Suspend();
    virtual EECode Pause();
    virtual EECode Resume();
    virtual EECode Flush();
    virtual EECode ResumeFlush();
    virtual EECode Purge();

    virtual EECode SetPbRule(TU8 direction, TU8 feeding_rule, TU16 speed);
    virtual EECode SetPbLoopMode(TU32 *p_loop_mode);

    virtual EECode EnableVideo(TU32 enable);
    virtual EECode EnableAudio(TU32 enable);

    virtual void OnRun();

public:
    virtual EECode SetVideoPostProcessingCallback(void *callback_context, void *callback);
    virtual EECode SetAudioPostProcessingCallback(void *callback_context, void *callback);

public:
    virtual void PrintStatus();

public:
    virtual void EventNotify(EEventType type, TU64 param1, TPointer param2);

public:
    virtual EECode QueryContentInfo(const SStreamCodecInfos *&pinfos) const;
    virtual EECode UpdateContext(SContextInfo *pContext);
    virtual EECode GetExtraData(SStreamingSessionContent *pContent);
    virtual EECode NavigationSeek(SContextInfo *info);
    virtual EECode ResumeChannel();

private:
    EECode processCmd(SCMD &cmd);
    EECode prepare();
    EECode readPacket();
    TU8 *syncPacket();
    EECode readESData(MPEG_TS_TP_HEADER *pTsHeader, SPESInfo *pPesInfo, TU8 *pPayload);
    EECode readStream(TU8 type, SPESInfo *&pPesInfo, TU8 flags);
    EECode readFrame(SPESInfo *&pPesInfo, TU8 flags);
    EECode freeFrame(SPESInfo *pPesInfo);
    EECode setupContext(TChar *url);
    EECode destroyContext();
    EECode getExtraData(SStreamingSessionContent *pContent);
    EECode sendExtraData();
    EECode processUpdateContext(SContextInfo *pContext);
    EECode parsePrivatePacket(TU8 type, TU8 *pData);
    EECode readPrivatePacket(TU8 type, SPESInfo *pPesInfo);
    EECode seekToRequestTime(const SDateTime *pStart, const SDateTime *pRequest);
    EECode processNavigation(SContextInfo *pContext);
    EECode getFirstTime(TTime &time);

private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;

    IMsgSink *mpMsgSink;
    COutputPin *mpOutputPins[EConstMaxDemuxerMuxerStreamNumber];
    CBufferPool *mpBufferPool[EConstMaxDemuxerMuxerStreamNumber];
    IMemPool *mpMemPool[EConstMaxDemuxerMuxerStreamNumber];

    COutputPin *mpCurOutputPin;
    IBufferPool *mpCurBufferPool;

    SStreamCodecInfos mStreamCodecInfos;

    TU8 *mpBufferBase;
    TU8 *mpBufferAligned;
    TUint mBufferSize;
    TUint mDataRemainSize;
    TUint mDataSize;
    TUint mTsPacketNum;
    TUint mTsPacketSize;

    IIO *mpIO;

    TU8 mbFileOpened;
    TU8 mbFileEOF;

    TS64 mFileTotalSize;

private:
    TUint mPmtPID;
    TUint mProgramCount;
    TUint mStreamCount;
    SStreamInfo msVideoStream;
    SStreamInfo msAudioStream;
    SStreamInfo msPrivatePES;

    SPESInfo msVideoPES;
    SPESInfo msAudioPES;

    TTime mStreamLastPTS[EConstMaxDemuxerMuxerStreamNumber];
    TTime mStreamFirstPTS[EConstMaxDemuxerMuxerStreamNumber];
    TTime mPTSDiff[EConstMaxDemuxerMuxerStreamNumber];

private:
    IStorageManagement *mpStorageManager;
    TChar *mpCurUrl;
    TChar *mpChannelName;
    TU32 mChannelNameSize;

    TU16 mCurDuration;
    TU8 mbDebugPTSGap;
    TU8 reserved_0;

    TU8 mbEnableVideo;
    TU8 mbEnableAudio;
    TU8 mbVodMode;
    TU8 mbExtraDataSent;

    TU8 *mpVideoExtraData;
    TU32 mVideoExtraDataSize;
    TU8 *mpAudioExtraData;
    TU32 mAudioExtraDataSize;

private:
    TTime mFakeVideoPTS;
    TTime mFakeAudioPTS;
    TS64 mVideoFrameCount;
    TS64 mAudioFrameCount;
    TTime mVideoFakePTSNum;
    TTime mVideoFakePTSDen;
    TTime mAudioFakePTSNum;
    TTime mAudioFakePTSDen;

private:
    STSPrivateDataStartTime msCurFileStartTime;
    TU8 *mpGOPIndex;
    TUint mnGOPCount;

    TUint mFramRate;
    TUint mGOPSize;
};
DCONFIG_COMPILE_OPTION_HEADERFILE_END
#endif
