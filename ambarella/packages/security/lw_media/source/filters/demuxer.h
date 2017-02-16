/*
 * demuxer.h
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

#ifndef __DEMUXER_H__
#define __DEMUXER_H__

//-----------------------------------------------------------------------
//
// CDemuxer
//
//-----------------------------------------------------------------------
class CDemuxer
    : public CObject
    , public IFilter
    , public IEventListener
{
    typedef CObject inherited;

public:
    static IFilter *Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index = 0);
    virtual CObject *GetObject0() const;

protected:
    CDemuxer(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index = 0);

    EECode Construct();
    virtual ~CDemuxer();

public:
    virtual void Delete();

public:
    virtual EECode Initialize(TChar *p_param = NULL);
    virtual EECode Finalize();

    virtual EECode Run();
    virtual EECode Exit();

    virtual EECode Start();
    virtual EECode Stop();

    virtual void Pause();
    virtual void Resume();
    virtual void Flush();
    virtual void ResumeFlush();

    virtual EECode Suspend(TUint release_context = 0);
    virtual EECode ResumeSuspend(TComponentIndex input_index = 0);

    virtual EECode SwitchInput(TComponentIndex focus_input_index = 0);

    virtual EECode FlowControl(EBufferType flowcontrol_type);

    virtual EECode SendCmd(TUint cmd);
    virtual void PostMsg(TUint cmd);

    virtual EECode AddOutputPin(TUint &index, TUint &sub_index, TUint type);
    virtual EECode AddInputPin(TUint &index, TUint type);

    virtual IOutputPin *GetOutputPin(TUint index, TUint sub_index = 0);
    virtual IInputPin *GetInputPin(TUint index);

    virtual EECode FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param);

    virtual void GetInfo(INFO &info);
    virtual void PrintStatus();

    virtual void EventNotify(EEventType type, TU64 param1, TPointer param2);

private:
    void finalize(TUint delete_demuxer = 1);
    EECode initialize(TChar *p_param, void *p_agent);
    EECode initialize_vod(TChar *p_param, void *p_agent);
    EECode copyString(TChar *p_param);

private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpEngineMsgSink;

private:
    SStreamCodecInfos *mpStreamCodecInfos;

    IDemuxer *mpDemuxer;
    SClockListener *mpClockListener;

    EDemuxerModuleID mCurDemexerID;
    EDemuxerModuleID mPresetDemuxerID;

private:
    TU8 mbDemuxerContentSetup;
    TU8 mbDemuxerOutputSetup;
    TU8 mbDemuxerRunning;
    TU8 mbDemuxerStarted;

private:
    TU8 mbDemuxerSuspended;
    TU8 mbDemuxerPaused;
    TU8 mbBackward;
    TU8 mScanMode;

private:
    TU8 mbEnableAudio;
    TU8 mbEnableVideo;
    TU8 mbEnableSubtitle;
    TU8 mbEnablePrivateData;

    TU8 mPriority;
    TU8 mbPreSetReceiveBufferSize;
    TU8 mbPreSetSendBufferSize;
    TU8 mbReconnectDone;

    TU32 mnRetryUrlMaxCount;
    TU32 mnCurrentRetryUrlCount;

    TU32 mReceiveBufferSize;
    TU32 mSendBufferSize;

private:
    TUint mnTotalOutputPinNumber;
    COutputPin *mpOutputPins[EDemuxerOutputPinNumber];
    CBufferPool *mpBufferPool[EDemuxerOutputPinNumber];
    IMemPool *mpMemPool[EDemuxerOutputPinNumber];

private:
    TChar *mpInputString;
    TU32 mInputStringBufferSize;

private:
    void *mpExternalVideoPostProcessingCallbackContext;
    void *mpExternalVideoPostProcessingCallback;
};

#endif

