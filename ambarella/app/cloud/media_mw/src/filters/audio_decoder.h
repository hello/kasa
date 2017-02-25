/*******************************************************************************
 * audio_decoder.h
 *
 * History:
 *    2012/06/08 - [Zhi He] create file
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

#ifndef __AUDIO_DECODER_H__
#define __AUDIO_DECODER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CAudioDecoder
//
//-----------------------------------------------------------------------
class CAudioDecoder
    : public CActiveFilter
{
    typedef CActiveFilter inherited;

public:
    static IFilter *Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

protected:
    CAudioDecoder(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

    EECode Construct();
    virtual ~CAudioDecoder();

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
    virtual void OnRun(void);

private:
    EECode initialize(EAudioDecoderModuleID id);
    void processCmd(SCMD &cmd);
    EECode setupDecoderContext();
    void destroyDecoderContext();
    TU32 isFormatChanges(CIBuffer *p_buffer);
    EECode flushInputData();

private:
    IAudioDecoder *mpDecoder;
    EAudioDecoderModuleID mCurDecoderID;

private:
    TU8 mbUsePresetMode;
    TU8 mbNeedSendSyncPointBuffer;
    TU8 mbMuted;
    TU8 mbNewSequence;

private:
    TU8 mAudioCurrentCodecID;
    TU8 mAudioConfigSampleFormat;
    TU8 mAudioConfigChannelNumber;
    TU8 mAudioConfigInterlave;

    TU32 mAudioConfigCustomizedCodecID;

    TU32 mAudioConfigSampleRate;
    TU32 mAudioConfigFramesize;
    TU32 mAudioConfigBitRate;

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

private:
    TUint mnCurrentInputPinNumber;
    CQueueInputPin *mpInputPins[EConstAudioDecoderMaxInputPinNumber];
    CQueueInputPin *mpCurInputPin;

    TUint mnCurrentOutputPinNumber;
    COutputPin *mpOutputPin;
    CBufferPool *mpBufferPool;
    IMemPool *mpMemPool;

private:
    CIBuffer *mpBuffer;
    CIBuffer *mpOutputBuffer;

private:
    TMemSize mPCMBlockSize;

private:
    CIBuffer mPersistEOSBuffer;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

