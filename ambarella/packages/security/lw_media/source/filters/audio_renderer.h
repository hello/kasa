/*
 * audio_renderer.h
 *
 * History:
 *    2013/01/24 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __AUDIO_RENDERER_H__
#define __AUDIO_RENDERER_H__

//-----------------------------------------------------------------------
//
// CAudioRenderer
//
//-----------------------------------------------------------------------
class CAudioRenderer: public CActiveFilter
{
    typedef CActiveFilter inherited;

public:
    static IFilter *Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

protected:
    CAudioRenderer(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

    EECode Construct();
    virtual ~CAudioRenderer();

public:
    virtual void Delete();

public:
    virtual EECode Initialize(TChar *p_param = NULL);
    virtual EECode Finalize();

    virtual void OnRun();

    virtual EECode Run();

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

    virtual EECode AddOutputPin(TUint &index, TUint &sub_index, TUint type);
    virtual EECode AddInputPin(TUint &index, TUint type);

    virtual IOutputPin *GetOutputPin(TUint index, TUint sub_index = 0);
    virtual IInputPin *GetInputPin(TUint index);

    virtual EECode FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param);
    virtual void PrintStatus();

    void GetInfo(INFO &info);

    //protected:
    //virtual EECode OnCmd(SCMD& cmd);

private:
    EECode stopAudioRenderer();
    EECode startAudioRenderer();
    EECode flushAudioRenderer();
    EECode pauseAudioRenderer();
    EECode resumeAudioRenderer();
    EECode flushInputData();

private:
    void processCmd(SCMD cmd);

private:
    IAudioRenderer *mpRenderer;
    CQueueInputPin *mpInputPins[EConstAudioRendererMaxInputPinNumber];
    CQueueInputPin *mpCurrentInputPin;
    TUint mnInputPinsNumber;

private:
    EAudioRendererModuleID mCurRendererID;

private:
    TU8 mbUsePresetMode;
    TU8 mReserved0;
    TU8 mReserved1;
    TU8 mReserved2;

private:
    CIBuffer *mpBuffer;

private:
    TTime mRenderingPTS;
    TTime mPrebufferingLength;
    TTime mEstimatedPrebufferingLength;

private:
    TU8 mbRendererContentSetup;
    TU8 mbRendererPaused;
    TU8 mbRendererMuted;
    TU8 mbRendererStarted;

    TU8 mbBlendingMode;//blend audio data from multiple channel before renderering
    TU8 mbRendererPrebufferReady;
    TU8 mbRecievedEosSignal;
    TU8 mbEos;
};

#endif


