/*
 * audio_capturer_v2.h
 *
 * History:
 *    2014/11/03 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __AUDIO_CAPTURER_V2_H__
#define __AUDIO_CAPTURER_V2_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CAudioCapturerV2
//
//-----------------------------------------------------------------------
class CAudioCapturerV2
    : public CObject
    , public IFilter
    , public IEventListener
{
    typedef CObject inherited;

public:
    static IFilter *Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index = 0);
    virtual CObject *GetObject0() const;

protected:
    CAudioCapturerV2(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index = 0);

    EECode Construct();
    virtual ~CAudioCapturerV2();

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
    void finalize();
    EECode initialize(TChar *p_param);

private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpEngineMsgSink;

private:
    IAudioInput *mpAudioInput;
    EAudioInputModuleID mCurCapturerID;

private:
    TU8 mbContentSetup;
    TU8 mbOutputSetup;
    TU8 mbRunning;
    TU8 mbStarted;

private:
    TU8 mbSuspended;
    TU8 mbPaused;
    TU8 mReserved0;
    TU8 mReserved1;

private:
    COutputPin *mpOutputPin;
    CBufferPool *mpBufferPool;
    IMemPool *mpMemPool;

private:
    void *mpDevExternal;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

