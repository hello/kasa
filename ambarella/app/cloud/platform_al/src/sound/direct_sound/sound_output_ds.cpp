/*******************************************************************************
 * sound_output_ds.cpp
 *
 * History:
 *    2014/11/20 - [Zhi He] create file
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
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"

#include "platform_al_if.h"

#include <DSound.h>

#include "sound_output_ds.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

ISoundOutput *gfCreateSoundOutputWinDS(SPersistCommonConfig *pPersistCommonConfig)
{
    return CSoundOutputDS::Create(pPersistCommonConfig);
}

//-----------------------------------------------------------------------
//
// CSoundOutputDS
//
//-----------------------------------------------------------------------
ISoundOutput *CSoundOutputDS::Create(SPersistCommonConfig *pPersistCommonConfig)
{
    CSoundOutputDS *result = new CSoundOutputDS(pPersistCommonConfig);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CSoundOutputDS::CSoundOutputDS(SPersistCommonConfig *pPersistCommonConfig)
    : inherited("CSoundOutputDS", 0)
    , mpPersistCommonConfig(pPersistCommonConfig)
    , mbStarted(0)
    , mbContextSetup(0)
    , mBlockSize(2048)
    , mBufferSize(0)
    , mCurOffset(0)
    , mpDS(NULL)
    , mpDSBuffer8(NULL)
    , mpDSBuffer(NULL)
    //    , mpNotify(NULL)
{
    //TU32 i = 0;
    //for (i = 0; i < DDS_DATA_BLOCK_NUM; i ++) {
    //    memset(&mPosNotify[i], 0x0, sizeof(DSBPOSITIONNOTIFY));
    //    mReadyEvents[i] = NULL;
    //}
}

EECode CSoundOutputDS::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogPlatformALSoundOutput);

    memset(&mDSBufferDesc, 0x0, sizeof(mDSBufferDesc));
    memset(&mWaveFormatEX, 0x0, sizeof(mWaveFormatEX));

    return EECode_OK;
}

CSoundOutputDS::~CSoundOutputDS()
{
    if (mpDS) {
        mpDS->Release();
        mpDS = NULL;
    }

    //if (mpDSBuffer8) {
    //    mpDSBuffer8->Release();
    //    mpDSBuffer8 = NULL;
    //}

    //if (mpDSBuffer) {
    //    mpDSBuffer->Release();
    //    mpDSBuffer = NULL;
    //}
}

void CSoundOutputDS::Destroy()
{
    delete this;
}

EECode CSoundOutputDS::SetupContext(SSoundConfigure *config, TULong handle)
{
    if (DUnlikely(mbContextSetup)) {
        LOGM_FATAL("already setup\n");
        return EECode_BadState;
    }

    if (DUnlikely(!config)) {
        LOGM_FATAL("NULL config\n");
        return EECode_BadParam;
    }

    mSoundConfig = *config;

    EECode err = setupContext(handle);

    if (DUnlikely(EECode_OK != err)) {
        LOGM_FATAL("setupContext return %d, %s\n", err, gfGetErrorCodeString(err));
        return EECode_BadParam;
    }

    mbContextSetup = 1;

    return EECode_OK;
}

void CSoundOutputDS::DestroyContext()
{
    if (DUnlikely(!mbContextSetup)) {
        LOGM_FATAL("DestroyContext, not setup yet\n");
        return;
    }

    return;
}

EECode CSoundOutputDS::Start()
{
    if (DUnlikely(!mbContextSetup)) {
        LOGM_FATAL("context not setup yet\n");
        return EECode_BadState;
    }

    if (DUnlikely(mbStarted)) {
        LOGM_FATAL("already started\n");
        return EECode_BadState;
    }

    if (DUnlikely(!mpDSBuffer8)) {
        LOGM_FATAL("NULL mpDSBuffer8\n");
        return EECode_BadState;
    }

    mbStarted = 1;
    //mCurOffset = 0;
    mpDSBuffer8->SetCurrentPosition(0);
    mpDSBuffer8->SetVolume(DSBVOLUME_MAX);
    mpDSBuffer8->Play(0, 0, DSBPLAY_LOOPING);

    return EECode_OK;
}

EECode CSoundOutputDS::Stop()
{
    if (DUnlikely(!mbContextSetup)) {
        LOGM_FATAL("context not setup yet\n");
        return EECode_BadState;
    }

    if (DUnlikely(!mbStarted)) {
        LOGM_FATAL("not started yet\n");
        return EECode_BadState;
    }

    if (DUnlikely(!mpDSBuffer8)) {
        LOGM_FATAL("NULL mpDSBuffer8\n");
        return EECode_BadState;
    }

    mbStarted = 0;
    mpDSBuffer8->Stop();
    mCurOffset = 0;

    return EECode_OK;
}

EECode CSoundOutputDS::WriteSoundData(TU8 *p, TU32 size)
{
    if (DUnlikely(!mbContextSetup)) {
        LOGM_FATAL("WriteSoundData, context not setup yet\n");
        return EECode_BadState;
    }

    if (DUnlikely(!mpDSBuffer8)) {
        LOGM_FATAL("WriteSoundData, NULL mpDSBuffer8\n");
        return EECode_BadState;
    }

    if (DUnlikely((!p) || (!size))) {
        LOGM_FATAL("WriteSoundData, NULL data %p or size %d\n", p, size);
        return EECode_BadParam;
    }

    do {
        if (size > mBlockSize) {
            writeSoundData(p, mBlockSize);
            p += mBlockSize;
            size -= mBlockSize;
        } else {
            writeSoundData(p, size);
            break;
        }
    } while (1);

    return EECode_OK;
}

EECode CSoundOutputDS::QueryBufferSize(TULong &mem_size) const
{
    mem_size = mBufferSize;
    return EECode_OK;
}

EECode CSoundOutputDS::QueryCurrentPosition(TULong &write_offset, TULong &play_offset)
{
    if (DLikely(mpDSBuffer8)) {
        HRESULT ret = mpDSBuffer8->GetCurrentPosition(&play_offset, NULL);
        if (DUnlikely(FAILED(ret))) {
            LOGM_FATAL("GetCurrentPosition() fail\n");
            return EECode_OSError;
        }
        write_offset = mCurOffset;
        return EECode_OK;
    }

    LOGM_FATAL("NULL mpDSBuffer8\n");
    return EECode_BadState;
}

EECode CSoundOutputDS::SetCurrentPlayPosition(TULong play_offset)
{
    if (DLikely(mpDSBuffer8)) {
        HRESULT ret = mpDSBuffer8->SetCurrentPosition(play_offset);
        if (DUnlikely(FAILED(ret))) {
            LOG_FATAL("SetCurrentPosition() fail\n");
            return EECode_OSError;
        }
        return EECode_OK;
    }

    LOGM_FATAL("NULL mpDSBuffer8\n");
    return EECode_BadState;
}

EECode CSoundOutputDS::setupContext(TULong handle)
{
    HRESULT ret = DirectSoundCreate8(NULL, &mpDS, NULL);
    if (DUnlikely(FAILED(ret))) {
        LOG_FATAL("DirectSoundCreate8() fail\n");
        return EECode_OSError;
    }

    mBlockSize = mSoundConfig.channels * mSoundConfig.frame_size * mSoundConfig.bytes_per_sample;
    if (mpPersistCommonConfig && mpPersistCommonConfig->sound_output.sound_output_buffer_count) {
        mBufferSize = mBlockSize * mpPersistCommonConfig->sound_output.sound_output_buffer_count;
    } else {
        mBufferSize = mBlockSize * DDS_DATA_BLOCK_NUM;
    }
    //LOGM_NOTICE("[config]: sound output buffer size %d\n", mBufferSize);

    memset(&mDSBufferDesc, 0, sizeof(mDSBufferDesc));
    mDSBufferDesc.dwSize = sizeof(mDSBufferDesc);
    mDSBufferDesc.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2;
    mDSBufferDesc.dwBufferBytes = mBufferSize;

    mDSBufferDesc.lpwfxFormat = &mWaveFormatEX;
    mDSBufferDesc.lpwfxFormat->wFormatTag = WAVE_FORMAT_PCM;
    mDSBufferDesc.lpwfxFormat->nChannels = mSoundConfig.channels;
    mDSBufferDesc.lpwfxFormat->nSamplesPerSec = mSoundConfig.sample_frequency;
    mDSBufferDesc.lpwfxFormat->nAvgBytesPerSec = mSoundConfig.sample_frequency * mSoundConfig.bytes_per_sample * mSoundConfig.channels;

    mDSBufferDesc.lpwfxFormat->nBlockAlign = mSoundConfig.bytes_per_sample * mSoundConfig.channels;
    mDSBufferDesc.lpwfxFormat->wBitsPerSample = mSoundConfig.bytes_per_sample * 8;
    mDSBufferDesc.lpwfxFormat->cbSize = 0;

    ret = mpDS->CreateSoundBuffer(&mDSBufferDesc, &mpDSBuffer, NULL);
    if (DUnlikely(FAILED(ret))) {
        LOG_FATAL("CreateSoundBuffer() fail\n");
        return EECode_OSError;
    }

    LOGM_NOTICE("mpDS->SetCooperativeLevel: handle 0x%08x\n", handle);
    ret = mpDS->SetCooperativeLevel((HWND)handle, DSSCL_NORMAL);
    if (DUnlikely(FAILED(ret))) {
        LOGM_FATAL("CreateSoundBuffer() fail\n");
        return EECode_OSError;
    }

    ret = mpDSBuffer->QueryInterface(IID_IDirectSoundBuffer8, (LPVOID *)&mpDSBuffer8);
    if (DUnlikely(FAILED(ret))) {
        LOGM_FATAL("QueryInterface(IID_IDirectSoundBuffer8) fail\n");
        return EECode_OSError;
    }

    return EECode_OK;

}


EECode CSoundOutputDS::writeSoundData(TU8 *p, TU32 size)
{
    LPVOID p_buf_1 = NULL;
    DWORD  buf_size_1 = 0;
    LPVOID p_buf_2 = NULL;
    DWORD  buf_size_2 = 0;

    TU8 *p_src = p;
    TU32 src_size = size;

    HRESULT hr = mpDSBuffer8->Lock(mCurOffset, src_size, &p_buf_1, &buf_size_1, &p_buf_2, &buf_size_2, 0);
    if (DUnlikely(FAILED(hr))) {
        LOGM_FATAL("WriteSoundData: mpDSBuffer8->Lock fail\n");
        return EECode_Error;
    }

    if (DUnlikely(DSERR_BUFFERLOST == hr)) {
        mpDSBuffer8->Restore();
        mpDSBuffer8->Lock(mCurOffset, src_size, &p_buf_1, &buf_size_1, &p_buf_2, &buf_size_2, 0);
    }

    if (p_buf_1) {
        if (src_size <= buf_size_1) {
            memcpy(p_buf_1, p_src, src_size);
        } else {
            memcpy(p_buf_1, p_src, buf_size_1);
            p_src += buf_size_1;
            src_size -= buf_size_1;

            if (p_buf_2 && src_size) {
                if (src_size <= buf_size_2) {
                    memcpy(p_buf_2, p_src, src_size);
                } else {
                    memcpy(p_buf_2, p_src, buf_size_2);
                    src_size -= buf_size_2;
                    LOGM_ERROR("WriteSoundData: error here\n");
                }
            }
        }
    }

    mpDSBuffer8->Unlock(p_buf_1, buf_size_1, p_buf_2, buf_size_2);

    mCurOffset += size;
    if (DUnlikely(mCurOffset >= mBufferSize)) {
        mCurOffset -= mBufferSize;
    }

    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

