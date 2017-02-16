/*
 * video_decoder.h
 *
 * History:
 *    2012/06/08 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __VIDEO_DECODER_H__
#define __VIDEO_DECODER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CVideoDecoder
//
//-----------------------------------------------------------------------
class CVideoDecoder
    : public CActiveFilter
{
    typedef CActiveFilter inherited;

public:
    static IFilter *Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

protected:
    CVideoDecoder(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

    EECode Construct();
    virtual ~CVideoDecoder();

public:
    virtual void Delete();

public:
    virtual EECode Initialize(TChar *p_param = NULL);
    virtual EECode Finalize();

    virtual EECode Run();

    virtual EECode Start();
    virtual EECode Stop();

    virtual EECode SwitchInput(TComponentIndex focus_input_index = 0);

    virtual EECode FlowControl(EBufferType flowcontrol_type);

    virtual EECode AddOutputPin(TUint &index, TUint &sub_index, TUint type);
    virtual EECode AddInputPin(TUint &index, TUint type);

    virtual IOutputPin *GetOutputPin(TUint index, TUint sub_index = 0);
    virtual IInputPin *GetInputPin(TUint index);

    virtual EECode FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param);

    virtual void GetInfo(INFO &info);
    virtual void PrintStatus();

    virtual void EventNotify(EEventType type, TU64 param1, TPointer param2);

protected:
    virtual void OnRun();

private:
    void processCmd(SCMD &cmd);
    void checkBufferFullness();
    void checkDelayFromPTS();
    EECode flushInputData();
    void bufferSyncPointCheck();

private:
    IVideoDecoder *mpDecoder;
    CIClockReference *mpSystemClockReference;
    SClockListener *mpWatchDog;
    EVideoDecoderModuleID mCurDecoderID;

private:
    TU8 mbDecoderContentSetup;
    TU8 mbWaitKeyFrame;
    TU8 mbDecoderRunning;
    TU8 mbDecoderStarted;

private:
    TU8 mbDecoderSuspended;
    TU8 mbDecoderPaused;
    TU8 mbBackward;
    TU8 mScanMode;

    TU8 mPrefetchCount;
    TU8 mCurPrefetchCount;
    TU8 mbEnablePrefetch;
    TU8 mbPrefetchDone;

    TU8 mbLiveMode;
    TU8 mbTuningPlayback;
    TU8 mIdentifyerCount;
    TU8 mReserved1;

    TU16 mTuningPlaybackFrameTime;
    TU16 mTuningCooldown;
    TU16 mTuningCurrentTick;
    TU16 mTuningCooldownRefVaule;

    TU32 mBufferFullnessThreashold;
    TU32 mBufferFullnessThreasholdRefValue;
    TU32 mBufferPrefetchDataSize;
    TU32 mLastSendDataSize;

    TTime mLastBufferCtlTime;

    TTime mTuningMaxTimeGap;
    TTime mLastSendFrameTime;
    TTime mLastDisplayedFrameTime;

    TTime mLastCheckedSendPTS;
    TTime mLastCheckedDisplayedPTS;

private:
    TUint mnCurrentInputPinNumber;
    CQueueInputPin *mpInputPins[EConstVideoDecoderMaxInputPinNumber];
    CQueueInputPin *mpCurInputPin;

    TUint mnCurrentOutputPinNumber;
    COutputPin *mpOutputPin;
    CBufferPool *mpBufferPool;

private:
    CIBuffer *mpBuffer;
    TUint mCurVideoWidth, mCurVideoHeight;

private:
    CIBuffer mPersistEOSBuffer;

};

//-----------------------------------------------------------------------
//
// CScheduledVideoDecoder
//
//-----------------------------------------------------------------------
class CScheduledVideoDecoder
    : public CScheduledFilter
{
    typedef CScheduledFilter inherited;

public:
    static IFilter *Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

protected:
    CScheduledVideoDecoder(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

    EECode Construct();
    virtual ~CScheduledVideoDecoder();

public:
    virtual void Delete();

public:
    virtual EECode Initialize(TChar *p_param = NULL);
    virtual EECode Finalize();

    virtual EECode Start();
    virtual EECode Stop();

    virtual EECode SwitchInput(TComponentIndex focus_input_index = 0);

    virtual EECode FlowControl(EBufferType flowcontrol_type);

    virtual EECode AddOutputPin(TUint &index, TUint &sub_index, TUint type);
    virtual EECode AddInputPin(TUint &index, TUint type);

    virtual IOutputPin *GetOutputPin(TUint index, TUint sub_index = 0);
    virtual IInputPin *GetInputPin(TUint index);

    virtual EECode FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param);

    virtual void GetInfo(INFO &info);
    virtual void PrintStatus();

public:
    virtual EECode Scheduling(TUint times = 1, TU32 inout_mask = 0);
    virtual TInt IsPassiveMode() const;
    virtual TSchedulingHandle GetWaitHandle() const;
    virtual TSchedulingUnit HungryScore() const;
    virtual TU8 GetPriority() const;
    virtual void Destroy();


public:
    virtual void EventNotify(EEventType type, TU64 param1, TPointer param2);

private:
    void processCmd(SCMD &cmd);

private:
    IVideoDecoder *mpDecoder;
    IScheduler *mpScheduler;
    EVideoDecoderModuleID mCurDecoderID;

private:
    TU8 mbDecoderContentSetup;
    TU8 mbWaitKeyFrame;
    TU8 mbDecoderRunning;
    TU8 mbDecoderStarted;

private:
    TU8 mbDecoderSuspended;
    TU8 mbDecoderPaused;
    TU8 mbBackward;
    TU8 mScanMode;

private:
    TU8 mPrefetchCount;
    TU8 mCurPrefetchCount;
    TU8 mbEnablePrefetch;
    TU8 mPriority;

private:
    TUint mnCurrentInputPinNumber;
    CQueueInputPin *mpInputPins[EConstVideoDecoderMaxInputPinNumber];
    CQueueInputPin *mpCurInputPin;

    TUint mnCurrentOutputPinNumber;
    COutputPin *mpOutputPin;
    CBufferPool *mpBufferPool;

private:
    CIBuffer *mpBuffer;

private:
    CIBuffer mPersistEOSBuffer;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

