/**
 * pulse_audio.h
 *
 * History:
 * 2014/02/19 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

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