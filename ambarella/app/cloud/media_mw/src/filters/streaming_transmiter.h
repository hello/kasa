/*******************************************************************************
 * streaming_transmiter.h
 *
 * History:
 *    2013/01/24 - [Zhi He] create file
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

#ifndef __STREAMING_TRANSMITER_H__
#define __STREAMING_TRANSMITER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CStreamingTransmiter
//
//-----------------------------------------------------------------------

class CStreamingTransmiter: virtual public CActiveFilter
{
    typedef CActiveFilter inherited;

public:
    static IFilter *Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

protected:
    CStreamingTransmiter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

    EECode Construct();
    virtual ~CStreamingTransmiter();

public:
    virtual void Delete();

public:
    virtual EECode Initialize(TChar *p_param = NULL);
    virtual EECode Finalize();


    virtual EECode Run();
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

    virtual EECode AddOutputPin(TUint &index, TUint &sub_index, TUint type);
    virtual EECode AddInputPin(TUint &index, TUint type);

    virtual IOutputPin *GetOutputPin(TUint index, TUint sub_index = 0);
    virtual IInputPin *GetInputPin(TUint index);
    virtual EECode FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param);

    void GetInfo(INFO &info);
    void PrintStatus();

protected:
    virtual void OnRun();

private:
    bool processCmd(SCMD &cmd);
    EECode flushInputData();
    EECode getPinIndex(CQueueInputPin *pin, TComponentIndex &index);
    //inline TU8* findIDR(TU8* data_base, TInt data_size);
    EECode newSubSessionContent(TComponentIndex index, StreamType type, StreamFormat format);
    EECode processSyncBuffer(TComponentIndex pin_index);

private:
    CQueueInputPin *mpInputPins[EConstStreamingTransmiterMaxInputPinNumber];
    IStreamingTransmiter *mpDataTransmiter[EConstStreamingTransmiterMaxInputPinNumber];
    TU32 mnDataTransmiterClientNumber[EConstStreamingTransmiterMaxInputPinNumber];
    TU8 mbSetExtraData[EConstStreamingTransmiterMaxInputPinNumber];
    TU8 mDataTransmiterState[EConstStreamingTransmiterMaxInputPinNumber];

    SStreamingSubSessionContent *mpSubSessionContent[EConstStreamingTransmiterMaxInputPinNumber];

    TComponentIndex mnInputPinsNumber;
    //todo
    TU8 mbVod;
    TU8 mReserved1;

private:
    CIBuffer *mpBuffer;
    //CIMutex *mpMutex;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif


