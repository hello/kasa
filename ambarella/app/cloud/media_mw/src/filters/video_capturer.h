/*
 * video_capturer.h
 *
 * History:
 *    2014/10/17 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __VIDEO_CAPTURER_H__
#define __VIDEO_CAPTURER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CVideoCapturer
//
//-----------------------------------------------------------------------
class CVideoCapturer
    : public CObject
    , public IFilter
    , public IEventListener
{
    typedef CObject inherited;

public:
    static IFilter *Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
    virtual CObject *GetObject0() const;

protected:
    CVideoCapturer(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

    EECode Construct();
    virtual ~CVideoCapturer();

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

    virtual EECode Suspend(TUint release_flag = EReleaseFlag_None);
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

    //protected:
    //    virtual void OnRun();

private:
    void processCmd(SCMD &cmd);

private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpEngineMsgSink;
    CIClockReference *mpSystemClockReference;
    IMutex *mpMutex;
    IVideoInput *mpCapturer;
    SClockListener *mpWatchDog;

private:
    TU8 mbContentSetup;
    TU8 mbCaptureFirstFrame;
    TU8 mbRunning;
    TU8 mbStarted;

private:
    TU32 mnCurrentOutputPinNumber;
    COutputPin *mpOutputPin;
    CBufferPool *mpBufferPool;

private:
    TTime mCaptureBeginTime;
    TTime mCaptureDuration;
    TU64 mCaptureCurrentCount;
    TU64 mCapturedFrameCount;

    TTime mFirstFrameTime;
    TTime mLastCapTime;

private:
    CIBuffer mPersistEOSBuffer;

private:
    EVideoInputModuleID mCurModuleID;
    SVideoCaptureParams mModuleParams;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

