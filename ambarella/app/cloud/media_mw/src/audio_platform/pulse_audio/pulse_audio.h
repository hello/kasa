/*******************************************************************************
 * pulse_audio.h
 *
 * History:
 * 2014/02/19 - [Zhi He] create file
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

#ifndef __PULSE_AUDIO_H__
#define __PULSE_AUDIO_H__
DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

typedef struct {
    pa_threaded_mainloop *mainloop;
    pa_context *context;
    pa_stream *stream;
    pa_channel_map *channelmap;
    pa_sample_spec *samplespec;
    pa_buffer_attr *bufferattr;
    pa_stream_direction_t *direction;
    TUint *latency;
    TUint underflow_count;
    TUint overllow_count;
} SPulseAudioData;

class CPulseAudio: public CObject, public IAudioHAL
{
    typedef CObject inherited;

public:
    static IAudioHAL *Create(const volatile SPersistMediaConfig *p_config);
private:
    CPulseAudio(const volatile SPersistMediaConfig *p_config);
    EECode Construct();
    virtual ~CPulseAudio();

public:
    virtual void Delete();

    // IAudioHAL
    virtual EECode OpenDevice(SAudioParams *pParam, TUint *pLatency, TUint *pAudiosize, TUint source_or_sink);
    virtual EECode CloseDevice();
    virtual EECode RenderAudio(TU8 *pData, TUint dataSize, TUint *pNumFrames);
    virtual EECode ReadStream(TU8 *pData, TUint dataSize, TUint *pNumFrames, TU64 *pTimeStamp);
    virtual EECode Start();
    virtual EECode Pause();
    virtual EECode Resume();
    virtual EECode Stop();
    //virtual EECode Flush();
    virtual EAudioPlatform QueryAudioPlatform() const {return EAudioPlatform_PulseAudio;}

    virtual TUint GetChunkSize() {  return 0;  }
    virtual TUint GetBitsPerFrame() {  return 0;  }

private:
    EECode paInitialize();
    void paFinalize();
    EECode paWrite(TUint size);
    EECode paDrain();
    EECode paCork(TInt isPause);

    //
private:
    pa_threaded_mainloop *mpPaMainLoop;
    pa_context *mpPaContext;
    pa_stream *mpPaStream;
    pa_stream_direction_t mPaDirection;
    pa_channel_map mPaChannelMap;
    pa_sample_spec mPaSampleSpec;
    pa_buffer_attr mPaBufferAttr;
    TUint mPaLatency;

    SPulseAudioData mPaData;

private:
    TU8 *mpDataBuffer;
    TUint mDataSize;

    bool mbOpened;
    bool mbPaused;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif