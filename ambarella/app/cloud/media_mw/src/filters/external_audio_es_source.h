/*
 * external_audio_es_source.h
 *
 * History:
 *    2014/12/10 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __EXTERNAL_AUDIO_ES_SOURCE_H__
#define __EXTERNAL_AUDIO_ES_SOURCE_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CExtAudioEsSource
//
//-----------------------------------------------------------------------
class CExtAudioEsSource
    : public CObject
    , public IFilter
    , public IExternalSourcePushModeES
{
    typedef CObject inherited;

public:
    static IFilter *Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
    virtual CObject *GetObject0() const;

protected:
    CExtAudioEsSource(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

    EECode Construct();
    virtual ~CExtAudioEsSource();

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

public:
    virtual EECode PushEs(TU8 *data, TU32 len, SEsInfo *info);
    virtual EECode AllocMemory(TU8 *&mem, TU32 len);
    virtual EECode ReturnMemory(TU8 *retm, TU32 len);

    virtual EECode SetMediaInfo(SMediaInfo *info);
    virtual void SendEOS();

private:
    EECode processExtraData();

private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpEngineMsgSink;

private:
    TU8 mbWaitKeyFrame;
    TU8 mbSendExtraData;
    TU8 mReserved1;
    TU8 mReserved2;

private:
    TU8 *mpExtraData;
    TU32 mExtraDataSize;

private:
    COutputPin *mpOutputPin;
    CBufferPool *mpBufferPool;
    IMemPool *mpMemPool;

private:
    CIBuffer mPersistEOSBuffer;

private:
    TU8 mAudioChannelNumber;
    TU8 mAudioSampleSize;
    TU8 mAudioProfile;
    TU8 mAudioFormat;

    TU32 mAudioSampleRate;
    TU32 mAudioFrameSize;
    TU32 mAudioBitrate;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

