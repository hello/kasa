/**
 * audio_capturer.cpp
 *
 * History:
 *    2014/01/05 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __AUDIO_CAPTURER_H__
#define __AUDIO_CAPTURER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CAudioCapturer
//
//-----------------------------------------------------------------------
class CAudioCapturer: public CActiveFilter
{
    typedef CActiveFilter inherited;

public:
    static IFilter *Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

protected:
    CAudioCapturer(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

    EECode Construct();
    virtual ~CAudioCapturer();

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

    virtual EECode SwitchInput(TComponentIndex focus_input_index = 0);

    virtual EECode FlowControl(EBufferType flowcontrol_type);

    virtual EECode AddOutputPin(TUint &index, TUint &sub_index, TUint type);
    virtual EECode AddInputPin(TUint &index, TUint type);

    virtual IOutputPin *GetOutputPin(TUint index, TUint sub_index = 0);
    virtual IInputPin *GetInputPin(TUint index);

    virtual EECode FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param);
    virtual void PrintStatus();

    virtual void EventNotify(EEventType type, TU64 param1, TPointer param2);

    void GetInfo(INFO &info);

private:
    void processCmd(SCMD cmd);
    void sendFlowControlBuffer(EBufferType flowcontrol_type);

private:
    IAudioInput *mpAudioInput;
    EAudioInputModuleID mCurInputID;
    TUint mBytesPerFrame;
    TUint mChunkSize;

    TU8 mbInputContentSetup;
    TU8 mbInputStart;

private:
    COutputPin *mpOutputPin;
    CBufferPool *mpBufferPool;
    IMemPool *mpMemPool;
    TMemSize mPCMBlockSize;

private:
    CIBuffer mPersistEOSBuffer;
    CIBuffer *mpOutputBuffer;

    TU8 *mpReserveBuffer;
    TU8 mbUseReserveBuffer;

    TUint mSpecifiedFrameSize;

private:
    TU8 mbSendSyncPoint;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif
