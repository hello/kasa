/*******************************************************************************
 * audio_alsa.h
 *
 * History:
 * 2013/5/07 - [Zhi He] create file
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

#ifndef __AUDIO_ALSA_H__
#define __AUDIO_ALSA_H__
DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

#define VERBOSE     0

class CAudioALSA: public CObject, public IAudioHAL
{
    typedef CObject inherited;

public:
    static IAudioHAL *Create(const volatile SPersistMediaConfig *p_config);
private:
    CAudioALSA(const volatile SPersistMediaConfig *p_config);
    EECode Construct();
    virtual ~CAudioALSA();

public:
    virtual void Delete();

    // IAudioHAL
    virtual EECode OpenDevice(SAudioParams *pParam, TUint *pLatency, TUint *pAudiosize, TUint source_or_sink);
    virtual EECode CloseDevice();
    virtual EECode RenderAudio(TU8 *pData, TUint dataSize, TUint *pNumFrames);
    virtual EECode ReadStream(TU8 *pData, TUint dataSize, TUint *pNumFrames, TU64 *pTimeStamp);
    virtual EECode Start();
    virtual EECode Pause() {return EECode_NoImplement;}
    virtual EECode Resume() {return EECode_NoImplement;}
    virtual EECode Stop() {return EECode_NoImplement;}
    //virtual EECode Flush();
    virtual EAudioPlatform QueryAudioPlatform() const {return EAudioPlatform_Alsa;}

    virtual TUint GetChunkSize() {  return (TUint)mChunkSize;  }
    virtual TUint GetBitsPerFrame() {  return mBitsPerFrame;  }

private:
    EECode pcmInit(snd_pcm_stream_t stream);
    EECode pcmDeinit();
    EECode setParams(snd_pcm_format_t pcmFormat, TUint sampleRate, TUint numOfChannels, TUint frameSize);
    EECode xrun(snd_pcm_stream_t stream);
    EECode suspend(void);
    TInt pcmWrite(TU8 *pData, TUint rcount);
    TInt pcmRead(TU8 *pData, TUint count);
    //void gettimestamp(snd_pcm_t *handle, snd_timestamp_t *timestamp);

private:
    snd_pcm_t *mpAudioHandle;
    snd_output_t *mpLog;

    TU8 mOpened;
    TU8 mbNoWait;
    TU8 mReserved0;
    TU8 mReserved1;

    TUint mSampleRate;
    TUint mNumOfChannels;
    TUint  mChunkBytes;
    TUint mBitsPerSample;
    TUint mBitsPerFrame;

    TU32 mReserved2;

    snd_pcm_uframes_t mChunkSize;
    snd_pcm_stream_t mStream;
    snd_pcm_format_t mPcmFormat;
    snd_timestamp_t mStartTimeStamp;

    TU32 mReserved3;

private:
    TU64 mToTSample;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif


