/*
 * ffmpeg_audio_encoder.h
 *
 * History:
 *    2014/10/27 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifdef BUILD_MODULE_FFMPEG

#ifndef __FFMPEG_AUDIO_ENCODER_H__
#define __FFMPEG_AUDIO_ENCODER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

class CFFMpegAudioEncoder: public CObject, virtual public IAudioEncoder
{
    typedef CObject inherited;

public:
    static IAudioEncoder *Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    virtual void Destroy();
    virtual void Delete();

public:
    virtual void PrintStatus();

public:
    virtual EECode SetupContext(SAudioParams *param);
    virtual void DestroyContext();

    virtual EECode Encode(TU8 *p_in, TU8 *&p_out, TU32 &output_size);
    virtual EECode GetExtraData(TU8 *&p, TU32 &size);

private:
    CFFMpegAudioEncoder(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    EECode Construct();
    virtual ~CFFMpegAudioEncoder();

private:
    EECode setupEncoder(enum AVCodecID codec_id);
    void destroyEncoder();

private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    AVCodecContext *mpCodec;
    AVCodec *mpEncoder;

    enum AVCodecID mCodecID;
    TU32 mSamplerate;
    TU32 mFrameSize;
    TU32 mBitrate;

private:
    TU8 *mpAudioExtraData;
    TU32 mAudioExtraDataSize;

private:
    TU8 mbEncoderSetup;
    TU8 mChannelNumber;
    TU8 mSampleFormat;
    TU8 mReserved2;

private:
    AVFrame mFrame;
    AVPacket mPacket;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END
#endif

#endif

