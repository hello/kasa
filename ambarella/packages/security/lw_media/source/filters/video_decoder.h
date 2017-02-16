/*
 * video_decoder.h
 *
 * History:
 *    2014/10/23 - [Zhi He] create file
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
    EECode flushInputData();

private:
    IVideoDecoder *mpDecoder;
    EVideoDecoderModuleID mCurDecoderID;
    EDecoderMode mCurDecoderMode;
    StreamFormat mCurrentFormat;
    TU32 mCustomizedContentFormat;

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

    TU8 mbNeedSendSyncPoint;
    TU8 mbNewSequence;
    TU8 mbWaitARPrecache;
    TU8 mReserved2;

private:
    TU32 mnCurrentInputPinNumber;
    CQueueInputPin *mpInputPins[EConstVideoDecoderMaxInputPinNumber];
    CQueueInputPin *mpCurInputPin;

    COutputPin *mpOutputPin;
    CBufferPool *mpBufferPool;

private:
    CIBuffer *mpBuffer;
    TU32 mCurVideoWidth, mCurVideoHeight;
    TU32 mVideoFramerateNum;
    TU32 mVideoFramerateDen;
    float mVideoFramerate;

private:
    CIBuffer mPersistEOSBuffer;
};

#endif

