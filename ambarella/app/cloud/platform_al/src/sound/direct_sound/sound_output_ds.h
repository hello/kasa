/*******************************************************************************
 * sound_output_ds.h
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

#ifndef __SOUND_OUTPUT_DS_H__
#define __SOUND_OUTPUT_DS_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

#define DDS_DATA_BLOCK_NUM 40

class CSoundOutputDS: public CObject, public ISoundOutput
{
    typedef CObject inherited;

protected:
    CSoundOutputDS(SPersistCommonConfig *pPersistCommonConfig);
    EECode Construct();
    virtual ~CSoundOutputDS();

public:
    static ISoundOutput *Create(SPersistCommonConfig *pPersistCommonConfig);
    virtual void Destroy();

public:
    virtual EECode SetupContext(SSoundConfigure *config, TULong handle);
    virtual void DestroyContext();

public:
    virtual EECode Start();
    virtual EECode Stop();

public:
    virtual EECode WriteSoundData(TU8 *p, TU32 size);
    virtual EECode QueryBufferSize(TULong &mem_size) const;
    virtual EECode QueryCurrentPosition(TULong &write_offset, TULong &play_offset);
    virtual EECode SetCurrentPlayPosition(TULong play_offset);

private:
    EECode setupContext(TULong handle);
    EECode writeSoundData(TU8 *p, TU32 size);

private:
    TU8 mbStarted;
    TU8 mbContextSetup;
    TU8 mReserved1;
    TU8 mReserved2;

private:
    DWORD mBlockSize;
    DWORD mBufferSize;
    DWORD mCurOffset;

private:
    SPersistCommonConfig *mpPersistCommonConfig;
    SSoundConfigure mSoundConfig;

private:
    IDirectSound8 *mpDS;
    IDirectSoundBuffer8 *mpDSBuffer8;
    IDirectSoundBuffer *mpDSBuffer;
    //DSBPOSITIONNOTIFY mPosNotify[DDS_READY_EVENTS_NUM];
    //IDirectSoundNotify8* mpNotify;
    //HANDLE      mReadyEvents[DDS_READY_EVENTS_NUM];

    DSBUFFERDESC mDSBufferDesc;
    WAVEFORMATEX mWaveFormatEX;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

