/*
 * customized_audio_encoder.h
 *
 * History:
 *    2014/02/20 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */
#ifndef __CUSTOMIZED_AUDIO_ENCODER_H__
#define __CUSTOMIZED_AUDIO_ENCODER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

class CCustomizedAudioEncoder: public CObject, public IAudioEncoder
{
    typedef CObject inherited;

public:
    static IAudioEncoder *Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    virtual void Destroy();
    virtual void Delete();

private:
    CCustomizedAudioEncoder(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    EECode Construct();
    virtual ~CCustomizedAudioEncoder();

public:
    virtual EECode SetupContext(SAudioParams *format);
    virtual void DestroyContext();

    virtual EECode Encode(TU8 *p_in, TU8 *&p_out, TU32 &output_size);
    virtual EECode GetExtraData(TU8 *&p, TU32 &size);

public:
    virtual void PrintStatus();

private:
    ICustomizedCodec *mpCodec;
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

