/*******************************************************************************
 * basic_sound_composer.h
 *
 * History:
 *  2014/11/20 - [Zhi He] create file
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

#ifndef __BASIC_SOUND_COMPOSER_H__
#define __BASIC_SOUND_COMPOSER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CBasicSoundComposer
//
//-----------------------------------------------------------------------

class CBasicSoundComposer: public CObject, public ISoundComposer, public IActiveObject
{
    typedef CObject inherited;
public:
    static CBasicSoundComposer *Create(SPersistCommonConfig *p_common_config, SSharedComponent *p_shared_component);
    virtual CObject *GetObject() const;
    virtual void Delete();

protected:
    CBasicSoundComposer(SPersistCommonConfig *p_common_config, SSharedComponent *p_shared_component);

    EECode Construct();
    virtual ~CBasicSoundComposer();

public:
    virtual EECode SetHandle(TULong handle);
    virtual EECode SetConfig(TU32 samplerate, TU32 framesize, TU32 channel_number, TU32 sample_format);

public:
    virtual EECode RegisterTrack(CISoundTrack *track);
    virtual void UnRegisterTrack(CISoundTrack *track);

public:
    virtual void DataNotify();
    virtual void ControlNotify(TU32 control_type);

public:
    virtual EECode StartRunning();
    virtual EECode ExitRunning();

public:
    virtual void OnRun();

public:
    virtual void Destroy();

private:
    void processCmd(SCMD &cmd);
    EECode createSoundOutput(TU32 smplerate, TU32 channel_number);
    TU32 composeSound();

#ifdef DAUTO_RESAMPLE
    TU32 composeSoundResample1(TU32 clean_remain);
    TU32 composeSoundResample2(TU32 clean_remain);
#endif

    void startOutput();
    void stopOutput();
    void writeData(CISoundTrack *p_track, TU8 *p, TU32 size);
#ifdef DAUTO_RESAMPLE
    void writeDataResample(CISoundTrack *p_track, TU8 *p, TU32 size, void *p_resampler, TU32 clean_remain);
#endif
    void adjustPlayPoint();
    void adjustResample();

private:
    SPersistCommonConfig *mpPersistCommonConfig;
    CIWorkQueue *mpWorkQueue;
    CIDoubleLinkedList *mpTrackList;
    IMutex *mpMutex;

    CIClockManager *mpClockManager;
    CIClockReference *mpClockReference;
    CIQueue *mpMsgQueue;

    ISoundOutput *mpSoundOutput;
    TULong mHandle;

    TU8 *mpComposeBuffer;
    TU32 mComposeBufferSize;

    TU8 *mpComposeSilentBuffer;
    TU32 mComposeSilentBufferSize;

    TU8 *mpComposeResamplerBuffer;
    TU32 mComposeResamplerBufferSize;

    TU32 mPrepareSize;
    TU32 mCurrentPrepareSize;

    TULong mOutputBufferSize;
    TULong mPlayWriteMinimumGap;
    TULong mPlayWriteAdjustGap;

    TULong mNormalGap;
    TULong mResample1Gap;
    TULong mResample2Gap;

private:
    TU8 mbLoop;
    TU8 mbStartOutput;
    TU8 mbCompensateUnderflow;
    TU8 mbStartCompensate;

private:
    SSoundConfigure mSoundConfig;

    TU32 mUnderflowSamplerate1;
    TU32 mUnderflowSamplerate2;
    TU32 mOverflowSamplerate1;
    TU32 mOverflowSamplerate2;

    void *mpUnderflowResampler1;
    void *mpUnderflowResampler2;
    void *mpOverflowResampler1;
    void *mpOverflowResampler2;

    TU32 mResampleMaxPacketNumber;
    TU32 mResampleCurPacketCount;

    ESoundComposerState msState;

#ifdef DDUMP_PCM_PB
private:
    FILE *mpDumpFile;
#endif
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

