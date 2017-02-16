/**
 * audio_alsa.h
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


