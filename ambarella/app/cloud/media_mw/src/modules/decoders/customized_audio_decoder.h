/*
 * customized_audio_decoder.h
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
#ifndef __CUSTOMIZED_AUDIO_DECODER_H__
#define __CUSTOMIZED_AUDIO_DECODER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

class CCustomizedAudioDecoder: public CObject, public IAudioDecoder
{
    typedef CObject inherited;

public:
    static IAudioDecoder *Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    virtual void Destroy();
    virtual void Delete();

private:
    CCustomizedAudioDecoder(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    EECode Construct();
    virtual ~CCustomizedAudioDecoder();

public:
    virtual EECode SetupContext(SAudioParams *param);
    virtual void DestroyContext();

    virtual EECode Start();
    virtual EECode Stop();

    virtual EECode Decode(CIBuffer *in_buffer, CIBuffer *out_buffer, TInt &consumed_bytes);
    virtual EECode Flush();

    virtual EECode Suspend();

    virtual EECode SetExtraData(TU8 *p, TUint size);

public:
    virtual void PrintStatus();

private:
    ICustomizedCodec *mpCodec;
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

