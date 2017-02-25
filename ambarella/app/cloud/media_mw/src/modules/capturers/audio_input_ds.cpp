/*******************************************************************************
 * audio_input_ds.cpp
 *
 * History:
 *    2014/11/03 - [Zhi He] create file
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

#include "common_config.h"

#ifdef BUILD_MODULE_WDS

#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"

#include "media_mw_if.h"
#include "media_mw_utils.h"

#include "framework_interface.h"
#include "mw_internal.h"
#include "dsp_platform_interface.h"
#include "audio_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

#include <DSound.h>

#include "audio_input_ds.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

//#define DDUMP_PCM_DS

IAudioInput *gfCreateAudioInputWinDS(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, void *pDev)
{
    return CAudioInputDS::Create(pName, pPersistMediaConfig, pEngineMsgSink, pDev);
}

static BOOL CALLBACK DSEnumProc(LPGUID lpGUID, LPCTSTR lpszDesc, LPCTSTR lpszDrvName, LPVOID lpContext)
{
    CAudioInputDS *thiz = (CAudioInputDS *) lpContext;
    if (DLikely(thiz)) {
        thiz->InsertSoundDevice(lpGUID, lpszDesc);
        LOG_NOTICE("DSEnumProc [%s]\n", lpszDesc);
    } else {
        LOG_FATAL("NULL thiz in DSEnumProc\n");
    }

    return(TRUE);
}

static EECode _alloc_audio_pcm_frame(CIBuffer *p_buffer, TU32 buffer_size)
{
    TU8 *p = (TU8 *) DDBG_MALLOC(buffer_size, "AIPM");
    if (DUnlikely(!p)) {
        LOG_FATAL("NULL p\n");
        return EECode_NoMemory;
    }

    p_buffer->SetDataPtrBase(p, 0);
    p_buffer->SetDataMemorySize(buffer_size, 0);

    return EECode_OK;
}

//-----------------------------------------------------------------------
//
// CAudioInputDS
//
//-----------------------------------------------------------------------
IAudioInput *CAudioInputDS::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, void *pDev)
{
    CAudioInputDS *result = new CAudioInputDS(pName, pPersistMediaConfig, pEngineMsgSink, pDev);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CObject *CAudioInputDS::GetObject() const
{
    return (CObject *) this;
}

void CAudioInputDS::Destroy()
{
    Delete();
}

CAudioInputDS::CAudioInputDS(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, void *pDev)
    : inherited(pName)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpMsgSink(pEngineMsgSink)
    , mpWorkQueue(NULL)
    , mpOutputPin(NULL)
    , mpBufferPool(NULL)
    , msState(EModuleState_WaitCmd)
    , mbRunning(1)
    , mFrameCount(0)
    , mPTS(0)
    , mDuration(0)
    , mpCapture(NULL)
    , mpCaptureBuffer(NULL)
    , mCaptureNumber(0)
    , mpNotify(NULL)
    , mBlockSize(0)
    , mBlockCount(0)
    , mOffset(0)
{
    mbSpecifiedDevice = 1;
    mpSpecifiedGUID = (GUID *) pDev;
}

EECode CAudioInputDS::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleAudioInput);

    memset(&mCaptureGUID[0], 0x0, sizeof(mCaptureGUID));

    TU32 i = 0;
    for (i = 0; i < DDS_MAX_DEVICE_NUM; i ++) {
        mCaptureDescription[i] = NULL;
    }

    memset(&mWaveFormatEX, 0x0, sizeof(mWaveFormatEX));

    for (i = 0; i < DDS_READY_EVENTS_NUM; i ++) {
        mReadyEvents[i] = 0;
    }

    memset(&mPosNotify[0], 0x0, sizeof(mPosNotify));
    memset(&mBufferDescription, 0x0, sizeof(mBufferDescription));

    mWaveFormatEX.wFormatTag = WAVE_FORMAT_PCM;
    mWaveFormatEX.nChannels = mpPersistMediaConfig->audio_prefer_setting.channel_number;
    mWaveFormatEX.nSamplesPerSec = mpPersistMediaConfig->audio_prefer_setting.sample_rate;
    mWaveFormatEX.wBitsPerSample = 16;
    mWaveFormatEX.nBlockAlign = (short)(mWaveFormatEX.nChannels * (mWaveFormatEX.wBitsPerSample / 8));
    mWaveFormatEX.nAvgBytesPerSec = mWaveFormatEX.nSamplesPerSec * mWaveFormatEX.nBlockAlign;
    mWaveFormatEX.cbSize = 0;

    mBufferDescription.dwSize = sizeof(mBufferDescription);
    mBufferDescription.dwFlags = DSCBCAPS_WAVEMAPPED;
    mBlockSize = (mpPersistMediaConfig->audio_prefer_setting.framesize * mWaveFormatEX.nChannels * mWaveFormatEX.wBitsPerSample) >> 3;
    mBlockCount = 32;

    mBufferDescription.dwBufferBytes = mBlockSize * mBlockCount;
    mBufferDescription.lpwfxFormat = &mWaveFormatEX;

    mpWorkQueue = CIWorkQueue::Create((IActiveObject *)this);
    if (DUnlikely(!mpWorkQueue)) {
        LOGM_ERROR("Create CIWorkQueue fail.\n");
        return EECode_NoMemory;
    }

    LOGM_NOTICE("CAudioInputDS: before mpWorkQueue->Run().\n");
    DASSERT(mpWorkQueue);
    mpWorkQueue->Run();
    LOGM_NOTICE("CAudioInputDS: after mpWorkQueue->Run().\n");

    return EECode_OK;
}

CAudioInputDS::~CAudioInputDS()
{

}

EECode CAudioInputDS::SetupContext(SAudioParams *param)
{
    //hard code here
    return openDevice();
}

EECode CAudioInputDS::DestroyContext()
{
    return EECode_OK;
}

EECode CAudioInputDS::SetupOutput(COutputPin *p_output_pin, CBufferPool *p_bufferpool, IMsgSink *p_msg_sink)
{
    mpOutputPin = p_output_pin;
    mpBufferPool = p_bufferpool;

    return EECode_OK;
}

TUint CAudioInputDS::GetChunkSize()
{
    return mBufferDescription.dwBufferBytes;
}

TUint CAudioInputDS::GetBitsPerFrame()
{
    return mWaveFormatEX.wBitsPerSample;
}

EECode CAudioInputDS::Start(TUint index)
{
    EECode err;
    DASSERT(mpWorkQueue);

    LOGM_INFO("CAudioInputDS::Start() begin.\n");
    err = mpWorkQueue->SendCmd(ECMDType_Start, NULL);
    LOGM_INFO("CAudioInputDS::Start() end.\n");
    return err;
}

EECode CAudioInputDS::Stop(TUint index)
{
    EECode err;
    DASSERT(mpWorkQueue);

    LOGM_INFO("CAudioInputDS::Start() begin.\n");
    err = mpWorkQueue->SendCmd(ECMDType_Stop, NULL);
    LOGM_INFO("CAudioInputDS::Start() end.\n");
    return err;
}

EECode CAudioInputDS::Flush(TUint index)
{
    return EECode_OK;
}

EECode CAudioInputDS::Read(TU8 *pData, TUint dataSize, TUint *pNumFrames, TU64 *pTimeStamp, TUint index)
{
    LOGM_ERROR("should not invoke this API\n");
    return EECode_NoImplement;
}

EECode CAudioInputDS::Pause(TUint index)
{
    return EECode_OK;
}

EECode CAudioInputDS::Resume(TUint index)
{
    return EECode_OK;
}

EECode CAudioInputDS::Mute()
{
    return EECode_OK;
}

EECode CAudioInputDS::UnMute()
{
    return EECode_OK;
}

void CAudioInputDS::OnRun()
{
    LONG capture_length = 0;
    DWORD ptr0, ptr1, len0, len1, ptr2, len2;
    HRESULT hr;
    CIBuffer *p_buffer = NULL;
    TU8 *pdst = NULL;
    TU32 second_part = 0;
    SCMD cmd;

    mpWorkQueue->CmdAck(EECode_OK);

    msState = EModuleState_WaitCmd;

    while (mbRunning) {

        LOGM_STATE("CAudioInputDS OnRun: start switch, msState=%d, %s\n", msState, gfGetModuleStateString((EModuleState)msState));

        switch (msState) {
            case EModuleState_WaitCmd:
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EModuleState_Running:
                while (mpWorkQueue->PeekCmd(cmd)) {
                    processCmd(cmd);
                }
                if (DUnlikely(EModuleState_Running != msState)) {
                    break;
                }
                do {
                    DWORD obj_id = WaitForMultipleObjects(DDS_READY_EVENTS_NUM, &mReadyEvents[0], FALSE, INFINITE);
                    if (DUnlikely(obj_id >= DDS_READY_EVENTS_NUM)) {
                        LOGM_FATAL("WaitForMultipleObjects return unexpected %d\n", obj_id);
                        return;
                        //return EECode_ExternalUnexpected;
                    }

                    hr = mpCaptureBuffer->GetCurrentPosition(&ptr0, &ptr1);
                    if (FAILED(hr)) {
                        LOGM_ERROR("GetCurrentPosition return 0x%08x\n", hr);
                        return;
                        //return EECode_ExternalUnexpected;
                    }

                    if (DUnlikely(DLikely(ptr1 > mOffset))) {
                        capture_length = ptr1 - mOffset;
                    } else {
                        capture_length = mBufferDescription.dwBufferBytes + ptr1 - mOffset;
                    }
                } while (capture_length < mBlockSize);

                hr = mpCaptureBuffer->Lock(mOffset, capture_length, (void **)(&ptr0), &len0, (void **)&ptr1, &len1, 0);
                if (DUnlikely(FAILED(hr))) {
                    LOGM_ERROR("mpCaptureBuffer->Lock() fail, return 0x%08x\n", hr);
                    return;
                    //return EECode_ExternalUnexpected;
                }

                ptr2 = ptr0;
                len2 = len0;
                second_part = 0;
                capture_length = 0;
                do {
                    mpBufferPool->AllocBuffer(p_buffer, (TUint)sizeof(CIBuffer));
                    pdst = p_buffer->GetDataPtrBase();
                    if (DUnlikely(!pdst)) {
                        if (DUnlikely(p_buffer->mbAllocated)) {
                            LOG_FATAL("[CAudioInputDS]: why buffer is allocated, but the data pointer is NULL?\n");
                        }
                        EECode err = _alloc_audio_pcm_frame(p_buffer, mBlockSize);
                        if (DUnlikely(EECode_OK != err)) {
                            LOGM_ERROR("no memory in CAudioInputDS::OnRun()\n");
                            return;
                        }
                        p_buffer->mbAllocated = 1;
                        pdst = p_buffer->GetDataPtrBase();
                    } else {
                        p_buffer->SetDataPtr(pdst);
                        p_buffer->SetDataSize(mBlockSize);
                    }
                    addBufferFlags(p_buffer);

                    if (DLikely(len2 >= mBlockSize)) {
                        memcpy(pdst, (void *)ptr2, mBlockSize);
                        len2 -= mBlockSize;
                        ptr2 += mBlockSize;
                        p_buffer->SetDataPtr(pdst);
                        p_buffer->SetDataSize(mBlockSize);
#ifdef DDUMP_PCM_DS
                        FILE *p_dump_file = fopen("dump/cap.pcm", "ab");
                        if (p_dump_file) {
                            fwrite(pdst, 1, mBlockSize, p_dump_file);
                            fclose(p_dump_file);
                            p_dump_file = NULL;
                        }
#endif
                        mpOutputPin->SendBuffer(p_buffer);
                        capture_length += mBlockSize;
                    } else {
                        if (second_part || (len2 < 1)) {
                            break;
                        }
                        second_part = 1;
                        memcpy(pdst, (void *)ptr2, len2);
                        memcpy(pdst + len2, (void *)ptr2, mBlockSize - len2);
#ifdef DDUMP_PCM_DS
                        FILE *p_dump_file = fopen("dump/cap.pcm", "ab");
                        if (p_dump_file) {
                            fwrite(pdst, 1, mBlockSize, p_dump_file);
                            fclose(p_dump_file);
                            p_dump_file = NULL;
                        }
#endif
                        p_buffer->SetDataPtr(pdst);
                        p_buffer->SetDataSize(mBlockSize);
                        mpOutputPin->SendBuffer(p_buffer);
                        ptr2 = ptr1 + (mBlockSize - len2);
                        len2 = len1 - (mBlockSize - len2);
                        capture_length += mBlockSize;
                    }
                } while (len2 >= mBlockSize);
                mOffset += capture_length;
                if (DUnlikely(mOffset >= mBufferDescription.dwBufferBytes)) {
                    mOffset -= mBufferDescription.dwBufferBytes;
                    if (DUnlikely(mOffset >= mBufferDescription.dwBufferBytes)) {
                        LOGM_FATAL("why comes here, mOffset %ld, mBufferDescription.dwBufferBytes %ld\n", mOffset, mBufferDescription.dwBufferBytes);
                        DASSERT(mBufferDescription.dwBufferBytes);
                        mOffset %= mBufferDescription.dwBufferBytes;
                    }
                }
                hr = mpCaptureBuffer->Unlock((void *)ptr0, len0, (void *)ptr1, len1);
                if (FAILED(hr)) {
                    LOGM_ERROR("mpCaptureBuffer->Unlock() fail, return 0x%08x\n", hr);
                    return;
                    //return EECode_ExternalUnexpected;
                }
                break;

            case EModuleState_Stopped:
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            default:
                LOGM_FATAL("BAD state %d\n", msState);
                break;
        }

    }

}

EECode CAudioInputDS::InsertSoundDevice(GUID *id, TCHAR const *p_desc)
{
    if (DLikely(mCaptureNumber < DDS_MAX_DEVICE_NUM)) {
        if (id) {
            mCaptureGUID[mCaptureNumber] = *id;
        }
        mCaptureDescription[mCaptureNumber] = p_desc;
        mCaptureNumber ++;
    } else {
        LOGM_ERROR("InsertSoundDevice() fail, rmCaptureNumber %d exceed\n", mCaptureNumber);
        return EECode_OK;
    }
    return EECode_OK;
}

EECode CAudioInputDS::openDevice()
{
    HRESULT hr;

    if (!mbSpecifiedDevice) {
        HRESULT hr = DirectSoundCaptureEnumerate((LPDSENUMCALLBACK)DSEnumProc, (VOID *)this);
        if (FAILED(hr)) {
            LOGM_ERROR("DirectSoundCaptureEnumerate() fail, ret 0x%08x\n", hr);
            return EECode_OSError;
        }

        LOGM_NOTICE("mCaptureNumber %d\n", mCaptureNumber);

        hr = DirectSoundCaptureCreate8(&mCaptureGUID[0], &mpCapture, 0);
    } else {
        hr = DirectSoundCaptureCreate8(mpSpecifiedGUID, &mpCapture, 0);
    }

    if (FAILED(hr)) {
        LOGM_ERROR("DirectSoundCaptureCreate8() fail, ret 0x%08x\n", hr);
        return EECode_OSError;
    }

    TU32 i = 0;
    for (i = 0; i < DDS_READY_EVENTS_NUM; i++) {
        mReadyEvents[i] = CreateEvent(0, FALSE, FALSE, 0);
    }

    TU32 size = mBufferDescription.dwBufferBytes / DDS_READY_EVENTS_NUM;

    hr = mpCapture->CreateCaptureBuffer(&mBufferDescription, &mpCaptureBuffer, 0);
    if (FAILED(hr)) {
        LOGM_ERROR("CreateCaptureBuffer() fail, ret 0x%08x\n", hr);
        return EECode_OSError;
    }

    hr = mpCaptureBuffer->QueryInterface(IID_IDirectSoundNotify8, reinterpret_cast<void **>(&mpNotify));
    if (FAILED(hr)) {
        LOGM_ERROR("QueryInterface() fail, ret 0x%08x\n", hr);
        return EECode_OSError;
    }

    for (i = 0; i < DDS_READY_EVENTS_NUM; ++ i) {
        mPosNotify[i].dwOffset = size * i;
        mPosNotify[i].hEventNotify = mReadyEvents[i];
    }

    hr = mpNotify->SetNotificationPositions(DWORD(DDS_READY_EVENTS_NUM), &mPosNotify[0]);
    if (FAILED(hr)) {
        LOGM_ERROR("SetNotificationPositions() fail, ret 0x%08x\n", hr);
        return EECode_OSError;
    }

    mpNotify->Release();
    mpNotify = NULL;

    return EECode_OK;
}

void CAudioInputDS::processCmd(SCMD &cmd)
{
    LOGM_FLOW("cmd.code %d.\n", cmd.code);
    DASSERT(mpWorkQueue);

    switch (cmd.code) {
        case ECMDType_Start:
            msState = EModuleState_Running;
            mpCaptureBuffer->Start(DSCBSTART_LOOPING);
            mpWorkQueue->CmdAck(EECode_OK);
            break;

        case ECMDType_Stop:
            msState = EModuleState_Stopped;
            mpCaptureBuffer->Stop();
            mpWorkQueue->CmdAck(EECode_OK);
            break;

        default:
            LOGM_ERROR("wrong cmd.code: %d", cmd.code);
            break;
    }

    LOGM_FLOW("CAudioInputDS::ProcessCmd, cmd.code %d end.\n", cmd.code);
}

void CAudioInputDS::addBufferFlags(CIBuffer *p_buffer)
{
    p_buffer->SetBufferFlags(CIBuffer::KEY_FRAME);
    p_buffer->SetBufferType(EBufferType_AudioPCMBuffer);

    if (DUnlikely(!mFrameCount)) {
        mPTS = 0;
        p_buffer->SetBufferFlagBits(CIBuffer::SYNC_POINT);
        p_buffer->mAudioChannelNumber = mWaveFormatEX.nChannels;
        p_buffer->mAudioSampleRate = mWaveFormatEX.nSamplesPerSec;
        p_buffer->mAudioSampleFormat = AudioSampleFMT_S16;
    } else {
        mPTS = mFrameCount * ((mpPersistMediaConfig->audio_prefer_setting.framesize * DDefaultTimeScale) / mWaveFormatEX.nSamplesPerSec);
    }

    p_buffer->SetBufferNativePTS(mPTS);
    p_buffer->SetBufferNativeDTS(mPTS);

    p_buffer->SetBufferLinearPTS(mPTS);
    p_buffer->SetBufferLinearDTS(mPTS);
    mFrameCount ++;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

#endif

