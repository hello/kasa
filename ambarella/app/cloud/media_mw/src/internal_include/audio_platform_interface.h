/**
 * audio_platform_interface.h
 *
 * History:
 * 2013/5/07 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

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

