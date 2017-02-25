/*******************************************************************************
 * audio_platform_interface.h
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

#ifndef __AUDIO_PLATFORM_INTERFACE_H__
#define __AUDIO_PLATFORM_INTERFACE_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

typedef enum {
    EAudioPlatform_Invalid = 0,

    EAudioPlatform_Alsa = 0x100,
    EAudioPlatform_Android,
    EAudioPlatform_PulseAudio,
} EAudioPlatform;

/*
enum STREAM {
    STREAM_PLAYBACK = 0,
    STREAM_CAPTURE,
};

enum PCM_FORMAT {
    FORMAT_S16_LE = 0,
    FORMAT_U8_LE,
    FORMAT_S16_BE,//noable
};

typedef struct {
    TUint sampleRate;
    TUint numChannels;
    STREAM stream;
    PCM_FORMAT sampleFormat;
} SAudioParam;
*/

class IAudioDecAPI
{
public:
    virtual EECode InitDecoder() = 0;
    virtual EECode ReleaseDecoder(TUint dec_id) = 0;

    virtual EECode Decode(TUint dec_id, TU8 *pstart, TU8 *pend) = 0;
    virtual EECode Stop(TUint dec_id, TUint stop_flag) = 0;
    virtual EECode TrickPlay() = 0;

    virtual EECode QueryStatus() = 0;

    virtual ~IAudioDecAPI() {}
};

class IAudioEncAPI
{
public:
    virtual EECode InitEncoder() = 0;
    virtual EECode ReleaseEncoder(TUint enc_id) = 0;

    virtual EECode GetAudioBuffer() = 0;
    virtual EECode Start(TUint enc_id) = 0;
    virtual EECode Stop(TUint enc_id, TUint stop_flag) = 0;

    virtual ~IAudioEncAPI() {}
};

class IAudioHAL
{
public:
    virtual EECode OpenDevice(SAudioParams *pParam, TUint *pLatency, TUint *pAudiosize, TUint source_or_sink) = 0;
    virtual EECode CloseDevice() = 0;

    virtual EECode RenderAudio(TU8 *pData, TUint dataSize, TUint *pNumFrames) = 0;
    virtual EECode ReadStream(TU8 *pData, TUint dataSize, TUint *pNumFrames, TU64 *pTimeStamp) = 0;
    virtual EECode Start() = 0;
    virtual EECode Pause() = 0;
    virtual EECode Resume() = 0;
    virtual EECode Stop() = 0;

    virtual EAudioPlatform QueryAudioPlatform() const = 0;
    virtual ~IAudioHAL() {}

    virtual TUint GetChunkSize() = 0;
    virtual TUint GetBitsPerFrame() = 0;
};
extern IAudioHAL *gfCreateAlsaAuidoHAL(const volatile SPersistMediaConfig *p_config);

extern IAudioHAL *gfAudioHALFactory(EAudioPlatform audio_platform, const volatile SPersistMediaConfig *p_config);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

