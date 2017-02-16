/*
 * faac_audio_encoder.h
 *
 * History:
 *    2014/11/05 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */
#ifndef __FAAC_AUDIO_ENCODER_H__
#define __FAAC_AUDIO_ENCODER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

class CFAACAudioEncoder: public CObject, virtual public IAudioEncoder
{
    typedef CObject inherited;

public:
    static IAudioEncoder *Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    virtual void Destroy();

private:
    CFAACAudioEncoder(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    EECode Construct();
    virtual ~CFAACAudioEncoder();

public:
    virtual void Delete();
    virtual void PrintStatus();

public:
    virtual EECode SetupContext(SAudioParams *param);
    virtual void DestroyContext();

    virtual EECode Encode(TU8 *p_in, TU8 *&p_out, TU32 &output_size);
    virtual EECode GetExtraData(TU8 *&p, TU32 &size);

private:
    EECode setupEncoder();
    void destroyEncoder();

private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    faacEncHandle   mpEncoder;
    faacEncConfiguration *mpEncConfig;

    TU32 mSamplerate;
    TU32 mFrameSize;
    TU32 mSampleNumber;
    TU32 mBitrate;

private:
    TU8 *mpAudioExtraData;
    TULong mAudioExtraDataSize;

private:
    TU8 mbEncoderSetup;
    TU8 mChannelNumber;
    TU8 mSampleFormat;
    TU8 mReserved2;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END
#endif

