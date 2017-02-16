/**
 * aac_audio_encoder.h
 *
 * History:
 *    2014/01/08 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __AAC_AUDIO_ENCODER_H__
#define __AAC_AUDIO_ENCODER_H__

class CAacAudioEncoder
    : public CObject
    , virtual public IAudioEncoder
{
    typedef CObject inherited;

protected:
    CAacAudioEncoder(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    EECode Construct();
    virtual ~CAacAudioEncoder();

public:
    static IAudioEncoder *Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    virtual void Destroy();

public:
    virtual EECode SetupContext(SAudioParams *param);
    virtual void DestroyContext();

    virtual EECode Encode(TU8 *p_in, TU8 *&p_out, TU32 &output_size);
    virtual EECode GetExtraData(TU8 *&p, TU32 &size);

private:
    EECode openAACEncoder();
    TUint doEncode(TU8 *pIn, TU8 *pOut);
    EECode closeAACEncoder();

private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpMsgSink;

private:
    TU8 *mpTmpEncBuf;

    TUint mSampleRate;
    TUint mNumOfChannels;
    TUint mBitRate;
    TUint mBitsPerSample;

private:
    TU8 mbEncoderStarted;

private:
    au_aacenc_config_t mAACEncConfig;
};

#endif
