/*
 * connecter_pinmuxer.h
 *
 * History:
 *    2013/11/26 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __CONNECTER_PINMUXER_H__
#define __CONNECTER_PINMUXER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CConnecterPinmuxer
//
//-----------------------------------------------------------------------
class CConnecterPinmuxer
    : public CObject
    , public IFilter
{
    typedef CObject inherited;

public:
    static IFilter *Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
    virtual CObject *GetObject0() const;

protected:
    CConnecterPinmuxer(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

    EECode Construct();
    virtual ~CConnecterPinmuxer();

public:
    virtual void Delete();

public:
    virtual EECode Initialize(TChar *p_param = NULL);
    virtual EECode Finalize();

    virtual EECode Run();
    virtual EECode Exit();

    virtual void Pause();
    virtual void Resume();
    virtual void Flush();
    virtual void ResumeFlush();

    virtual EECode Suspend(TUint release_flag = EReleaseFlag_None);
    virtual EECode ResumeSuspend(TComponentIndex input_index = 0);

    virtual EECode Start();
    virtual EECode Stop();

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

    //virtual void EventNotify(EEventType type, TU64 param1, TPointer param2);

private:
    void saveExtraData(TU8 *p, TMemSize size);

private:
    IMutex *mpMutex;

private:
    TComponentIndex mnCurrentInputPinIndex;
    TComponentIndex mnCurrentInputPinNumber;
    //TComponentIndex mnCurrentSubOutputPinNumber;
    //TComponentIndex mReserved0;

    CBypassInputPin *mpInputPins[EConstPinmuxerMaxInputPinNumber];
    CBypassInputPin *mpCurInputPin;
    CBypassInputPin *mpLastCurInputPin;

    COutputPin *mpOutputPin;
    CBufferPool *mpBufferPool;

private:
    CIBuffer mPersistEOSBuffer;

    StreamType mType;

private:
    TU8 *mpExtraData;
    TMemSize mExtraDataSize;
    TMemSize mExtraDataBufferSize;

private:
    TU8 mbStopped;
    TU8 mReserved0;
    TU8 mReserved1;
    TU8 mReserved2;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

