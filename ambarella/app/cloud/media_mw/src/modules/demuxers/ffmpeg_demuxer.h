/*
 * ffmpeg_demuxer.h
 *
 * History:
 *    2013/4/27 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __FFMPEG_DEMUXER_H__
#define __FFMPEG_DEMUXER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

#define DEMUXER_BQUEUE_NUM_V 96
#define DEMUXER_BQUEUE_NUM_A 256

class CFFMpegDemuxer
    : public CActiveObject
    , public IDemuxer
    , public IEventListener
{
    typedef CActiveObject inherited;

public:
    static IDemuxer *Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TUint index);
    virtual CObject *GetObject0() const;

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
    virtual void Delete();

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
    CFFMpegDemuxer(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TUint index);
    EECode Construct();
    virtual ~CFFMpegDemuxer();

private:
    EECode openUrl(TChar *url);
    void closeUrl();

    EECode processCmd(SCMD &cmd);
    void sendFlowControlBuffer(EBufferType flowcontrol_type);
    void updatePTSConvertor();
    TTime convertVideoPts(TTime pts);
    TTime convertAudioPts(TTime pts);
    void postVideoSizeMsg(TU32 width, TU32 height);
    EECode DoSeek(TTime target);

private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    SStreamCodecInfos mStreamCodecInfos;
    AVFormatContext *mpAVFormat;
    AVPacket mPkt;
    TInt mVideo;
    TInt mAudio;

    TU64 mPTSVideo_Num;
    TU64 mPTSVideo_Den;
    TU64 mPTSAudio_Num;
    TU64 mPTSAudio_Den;
    TU64 mVideoFramerateNum;
    TU64 mVideoFramerateDen;
    float mVideoFramerate;
    TTime mFakePts;
private:
    IMsgSink *mpMsgSink;
    COutputPin *mpOutputPins[EConstMaxDemuxerMuxerStreamNumber];
    CBufferPool *mpBufferPool[EConstMaxDemuxerMuxerStreamNumber];
    //IMemPool* mpMemPool[EConstMaxDemuxerMuxerStreamNumber];

    COutputPin *mpCurOutputPin;
    IBufferPool *mpCurBufferPool;

    //flow control related
private:
    CIBuffer mPersistEOSBuffer;
    CIBuffer mPersistPauseBuffer;
    CIBuffer mPersistResumeBuffer;
    CIBuffer mPersistFinalizeFileBuffer;

private:
    TU8 *mpVideoExtraData;
    TUint mVideoExtraDataSize;
    TU8 *mpAudioExtraData;
    TUint mAudioExtraDataSize;

private:
    TU8 mbSendVideoExtraData;
    TU8 mbSendAudioExtraData;
    TU8 mbIsBackward;
    TU8 mfwbwFeedingRule;
    TU16 mCurrentSpeed;
    TU16 mCurrentSpeedFrac;

    TTime mEstimatedKeyFrameInterval;
    TTime mBWLastPTS;
    TUint mbBWEOS;

private:
    TU32 mPlaybackLoopMode;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END
#endif

