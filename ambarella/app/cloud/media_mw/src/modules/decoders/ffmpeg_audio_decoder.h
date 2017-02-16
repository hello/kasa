/*
 * ffmpeg_audio_decoder.h
 *
 * History:
 *    2013/5/21 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifdef BUILD_MODULE_FFMPEG

#ifndef __FFMPEG_AUDIO_DECODER_H__
#define __FFMPEG_AUDIO_DECODER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

class CFFMpegAudioDecoder: public CObject, virtual public IAudioDecoder
{
    typedef CObject inherited;

public:
    static IAudioDecoder *Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    virtual void Destroy();

public:
    virtual void PrintStatus();

public:
    virtual EECode SetupContext(SAudioParams *param);
    virtual void DestroyContext();

    virtual EECode Start();
    virtual EECode Stop();

    virtual EECode Decode(CIBuffer *in_buffer, CIBuffer *out_buffer, TInt &consumed_bytes);
    virtual EECode Flush();

    virtual EECode Suspend();

    virtual EECode SetExtraData(TU8 *p, TUint size);

    virtual void Delete();

private:
    CFFMpegAudioDecoder(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    EECode Construct();
    virtual ~CFFMpegAudioDecoder();

private:
    EECode setupDecoder(enum AVCodecID codec_id);
    void destroyDecoder();

private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    AVCodecContext *mpCodec;
    AVCodec *mpDecoder;

    enum AVCodecID mCodecID;
    TU32 mSamplerate;
    TU32 mBitrate;

    AVFrame mAVFrame;

private:
    TU8 *mpAudioExtraData;
    TU32 mAudioExtraDataSize;

private:
    TU8 mbDecoderSetup;
    TU8 mChannelNumber;
    TU8 mSampleFormat;
    TU8 mReserved2;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END
#endif

#endif

