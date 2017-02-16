/*
 * video_capturer_active.h
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

#ifndef __VIDEO_CAPTURER_ACTIVE_H__
#define __VIDEO_CAPTURER_ACTIVE_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CVideoCapturerActive
//
//-----------------------------------------------------------------------
class CVideoCapturerActive
    : public CActiveFilter
{
    typedef CActiveFilter inherited;

public:
    static IFilter *Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

protected:
    CVideoCapturerActive(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

    EECode Construct();
    virtual ~CVideoCapturerActive();

public:
    virtual void Delete();

public:
    virtual EECode Initialize(TChar *p_param = NULL);
    virtual EECode Finalize();

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
    void flushData();
    void beginCapture(TU32 fr_num, TU32 fr_den);

private:
    IVideoInput *mpCapturer;

private:
    TU8 mbContentSetup;
    TU8 mbCaptureFirstFrame;
    TU8 mbRunning;
    TU8 mbStarted;

private:
    TU32 mnCurrentOutputPinNumber;
    COutputPin *mpOutputPin;
    CBufferPool *mpBufferPool;
    CIBuffer *mpBuffer;

private:
    TU32 mCapFramerateNum;
    TU32 mCapFramerateDen;

private:
    TTime mCaptureDuration;
    TU64 mSkipedFrameCount;
    TU64 mCapturedFrameCount;

    TTime mFirstFrameTime;
    TTime mCurTime;
    TTime mEstimatedNextFrameTime;
    TTime mSkipThreashold;
    TTime mWaitThreashold;

private:
    CIBuffer mPersistEOSBuffer;

private:
    EVideoInputModuleID mCurModuleID;
    SVideoCaptureParams mModuleParams;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

