/*
 * video_encoder_v2.h
 *
 * History:
 *    2014/10/29 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __VIDEO_ENCODER_V2_H__
#define __VIDEO_ENCODER_V2_H__

//-----------------------------------------------------------------------
//
// CVideoEncoderV2
//
//-----------------------------------------------------------------------
class CVideoEncoderV2
    : public CActiveFilter
{
    typedef CActiveFilter inherited;

public:
    static IFilter *Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

protected:
    CVideoEncoderV2(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

    EECode Construct();
    virtual ~CVideoEncoderV2();

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

    virtual void EventNotify(EEventType type, TU64 param1, TPointer param2);

protected:
    virtual void OnRun();

private:
    void processCmd(SCMD &cmd);
    void flushData();

private:
    IVideoEncoder *mpEncoder;
    CIClockReference *mpSystemClockReference;
    EVideoEncoderModuleID mCurEncoderID;

private:
    TU8 mbEncoderContentSetup;
    TU8 mbWaitKeyFrame;
    TU8 mReserved0;
    TU8 mbEncoderStarted;

private:
    TU32 mnCurrentInputPinNumber;
    CQueueInputPin *mpInputPins[EConstVideoDecoderMaxInputPinNumber];
    CQueueInputPin *mpCurInputPin;

    TU32 mnCurrentOutputPinNumber;
    COutputPin *mpOutputPin;
    CBufferPool *mpBufferPool;

private:
    CIBuffer *mpInputBuffer;
    CIBuffer *mpBuffer;
    CIBuffer *mpCachedInputBuffer;

private:
    CIBuffer mPersistEOSBuffer;

};

#endif

