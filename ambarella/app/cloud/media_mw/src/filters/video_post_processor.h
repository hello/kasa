/*******************************************************************************
 * video_post_processor.h
 *
 * History:
 *    2013/04/05 - [Zhi He] create file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

#ifndef __VIDEO_POST_PROCESSOR_H__
#define __VIDEO_POST_PROCESSOR_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CVideoPostProcessorInput
//
//-----------------------------------------------------------------------
class CVideoPostProcessorInput: public CQueueInputPin
{
    typedef CQueueInputPin inherited;

public:
    static CVideoPostProcessorInput *Create(const TChar *name, IFilter *pFilter, CIQueue *queue, TUint index);

public:
    TU8 GetEOS() { return mbEOS;}
    void SetEOS(TU8 eos) {mbEOS = eos;}

protected:
    CVideoPostProcessorInput(const TChar *name, IFilter *pFilter, TUint index)
        : inherited(name, pFilter, StreamType_Video)
        , mIndex(index)
        , mbEOS(0) {
    }

protected:
    virtual ~CVideoPostProcessorInput();
    EECode Construct(CIQueue *queue);

private:
    TUint mIndex;
    TU8 mbEOS;
    TU8 mReserved0;
    TU8 mReserved1;
    TU8 mReserved2;
};

//-----------------------------------------------------------------------
//
// CVideoPostProcessor
//
//-----------------------------------------------------------------------
class CVideoPostProcessor: public CActiveFilter
{
    typedef CActiveFilter inherited;

public:
    static IFilter *Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

protected:
    CVideoPostProcessor(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

    EECode Construct();
    virtual ~CVideoPostProcessor();

public:
    virtual void Delete();
    virtual void Pause();
    virtual void Resume();
    virtual void Flush();
    virtual void ResumeFlush();

    virtual EECode Suspend(TUint release_context = 0);
    virtual EECode ResumeSuspend(TComponentIndex input_index = 0);

    virtual EECode SwitchInput(TComponentIndex focus_input_index = 0);

    virtual EECode Run() {return EECode_OK;};
    virtual EECode Start();
    virtual EECode Stop();

    virtual EECode FlowControl(EBufferType type);

    virtual IInputPin *GetInputPin(TUint index);
    virtual IOutputPin *GetOutputPin(TUint index, TUint sub_index);
    virtual EECode AddOutputPin(TUint &index, TUint &sub_index, TUint type);
    virtual EECode AddInputPin(TUint &index, TUint type);

    virtual void EventNotify(EEventType type, TU64 param1, TPointer param2);
    virtual EECode FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param);

    virtual void PrintStatus();
    virtual void GetInfo(INFO &info);

public:
    virtual EECode Initialize(TChar *p_param = NULL);
    virtual EECode Finalize();

protected:
    virtual void OnRun() {};

private:
    void postMsg(TUint msg_code);
    bool readInputData(CVideoPostProcessorInput *inputpin, EBufferType &type);
    EECode flushInputData();

private:
    IVideoPostProcessor *mpVideoPostProcessor;
    CVideoPostProcessorInput *mpInputPins[EConstVideoPostProcessorMaxInputPinNumber];
    CVideoPostProcessorInput *mpCurrentInputPin;
    TUint mnInputPinsNumber;

private:
    COutputPin *mpOutputPin;
    CBufferPool *mpBufferPool;

private:
    EVideoPostPModuleID mCurVideoPostProcessorID;

private:
    CIBuffer *mpBuffer;

private:
    TU8 mbVideoPostPContextSetup;
    TU8 mbRecievedEosSignal;
    TU8 mbPaused;
    TU8 mbSetGlobalSetting;

    SVideoPostPGlobalSetting mPostPGlobalSetting;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif


