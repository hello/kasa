/*
 * aac_audio_decoder.h
 *
 * History:
 *    2013/8/29 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */
#ifndef __AAC_AUDIO_DECODER_H__
#define __AAC_AUDIO_DECODER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

typedef struct _SAACData {
    TU8 *data;
    TInt size;
} SAACData;

class CAACAudioDecoder: public CObject, virtual public IAudioDecoder
{
    typedef CObject inherited;

public:
    static IAudioDecoder *Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    virtual void Destroy();

public:
    virtual void PrintStatus();

public:
    virtual EECode SetupContext(SAudioParams *param);
    virtual EECode DestroyContext();

    virtual EECode Start();
    virtual EECode Stop();

    virtual EECode Decode(CIBuffer *in_buffer, CIBuffer *out_buffer, TInt &consumed_bytes);
    virtual EECode Flush();

    virtual EECode Suspend();

    virtual EECode SetExtraData(TU8 *p, TUint size);

    virtual void Delete();

private:
    CAACAudioDecoder(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    EECode Construct();
    virtual ~CAACAudioDecoder();

private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;

private:
    EECode constructAACdec(SAudioParams *param);
    EECode initParameters(SAudioParams *param);
    EECode setupDecoder();
    EECode DecodeAudioAACdec(SAACData *pPacket, TS16 *out_ptr, TInt *out_size);
    EECode DecodeAudioAACdecRTP(SAACData *pPacket, TS16 *out_ptr, TInt *out_size);
    EECode WriteBitBuf(SAACData *pPacket, TInt offset);
    EECode AddAdtsHeader(SAACData *pPacket);
private:
    TInt mFrameNum;

    static TU32 mpDecMem[106000];
    static TU8 mpDecBackUp[252];
    static TU8 mpInputBuf[16384];
    static TU32 mpOutBuf[8192];
};
DCONFIG_COMPILE_OPTION_HEADERFILE_END
#endif //__AAC_AUDIO_DECODER_H__

