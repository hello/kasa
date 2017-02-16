/**
 * audio_encoder.h
 *
 * History:
 *    2014/01/07 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __AUDIO_ENCODER_H__
#define __AUDIO_ENCODER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CAudioEncoder
//
//-----------------------------------------------------------------------
class CAudioEncoder: public CActiveFilter
{
    typedef CActiveFilter inherited;

public:
    static IFilter *Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

protected:
    CAudioEncoder(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

    EECode Construct();
    virtual ~CAudioEncoder();

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
    void processCmd(SCMD &cmd);
    void sendFlowControlBuffer(EBufferType flowcontrol_type);

private:
    IAudioEncoder *mpEncoder;
    EAudioEncoderModuleID mCurEncoderID;

    TU8 mbEncoderContentSetup;
    TU8 mbEncoderStarted;
    TU8 mbSendExtradata;
    TU8 mReserved0;

private:
    TUint mnCurrentInputPinNumber;
    CQueueInputPin *mpInputPin;

    TUint mnCurrentOutputPinNumber;
    COutputPin *mpOutputPin;
    CBufferPool *mpBufferPool;
    IMemPool *mpMemPool;

private:
    CIBuffer *mpBuffer;
    CIBuffer *mpOutputBuffer;

private:
    StreamFormat mAudioCodecFormat;
    AudioSampleFMT mAudioSampleFormat;
    TU32 mAudioSampleRate;
    TU32 mAudioChannelNumber;
    TU32 mAudioFrameSize;
    TU32 mAudioBitrate;

private:
    CIBuffer mPersistEOSBuffer;

    TU8 mpPreSendAudioExtradata[8];
};
DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif
