/*******************************************************************************
 * demuxer.h
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

#ifndef __DEMUXER_H__
#define __DEMUXER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CDemuxer
//
//-----------------------------------------------------------------------
class CDemuxer
    : public CObject
    , public IFilter
    , public IEventListener
{
    typedef CObject inherited;

public:
    static IFilter *Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index = 0);
    virtual CObject *GetObject0() const;

protected:
    CDemuxer(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index = 0);

    EECode Construct();
    virtual ~CDemuxer();

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

    virtual void EventNotify(EEventType type, TU64 param1, TPointer param2);

private:
    void finalize(TUint delete_demuxer = 1);
    EECode initialize(TChar *p_param, void *p_agent);
    EECode initialize_vod(TChar *p_param, void *p_agent);
    EECode copyString(TChar *p_param);

private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpEngineMsgSink;

private:
    SStreamCodecInfos *mpStreamCodecInfos;

    IDemuxer *mpDemuxer;
    SClockListener *mpClockListener;

    EDemuxerModuleID mCurDemexerID;
    EDemuxerModuleID mPresetDemuxerID;

private:
    TU8 mbDemuxerContentSetup;
    TU8 mbDemuxerOutputSetup;
    TU8 mbDemuxerRunning;
    TU8 mbDemuxerStarted;

private:
    TU8 mbDemuxerSuspended;
    TU8 mbDemuxerPaused;
    TU8 mbBackward;
    TU8 mScanMode;

private:
    TU8 mbEnableAudio;
    TU8 mbEnableVideo;
    TU8 mbEnableSubtitle;
    TU8 mbEnablePrivateData;

    TU8 mPriority;
    TU8 mbPreSetReceiveBufferSize;
    TU8 mbPreSetSendBufferSize;
    TU8 mbReconnectDone;

    TU32 mnRetryUrlMaxCount;
    TU32 mnCurrentRetryUrlCount;

    TU32 mReceiveBufferSize;
    TU32 mSendBufferSize;

private:
    TUint mnTotalOutputPinNumber;
    COutputPin *mpOutputPins[EDemuxerOutputPinNumber];
    CBufferPool *mpBufferPool[EDemuxerOutputPinNumber];
    IMemPool *mpMemPool[EDemuxerOutputPinNumber];

private:
    TChar *mpInputString;
    TU32 mInputStringBufferSize;

private:
    void *mpExternalVideoPostProcessingCallbackContext;
    void *mpExternalVideoPostProcessingCallback;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

