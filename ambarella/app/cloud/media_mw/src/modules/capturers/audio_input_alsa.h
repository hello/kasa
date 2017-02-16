/**
 * audio_input_alsa.h
 *
 * History:
 *    2014/01/5 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __AUDIO_CAPTURER_ALSA_H__
#define __AUDIO_CAPTURER_ALSA_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

class CAudioInputAlsa: public CObject, virtual public IAudioInput
{
    typedef CObject inherited;

protected:
    CAudioInputAlsa(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    EECode Construct();
    virtual ~CAudioInputAlsa();

public:
    static IAudioInput *Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    virtual void Destroy();

public:
    virtual EECode SetupContext(SAudioParams *param);
    virtual EECode DestroyContext();
    virtual TUint GetChunkSize();
    virtual TUint GetBitsPerFrame();

    virtual EECode Start(TUint index = 0);
    virtual EECode Stop(TUint index = 0);
    virtual EECode Flush(TUint index = 0);

    virtual EECode Read(TU8 *pData, TUint dataSize, TUint *pNumFrames, TU64 *pTimeStamp, TUint index = 0);

    virtual EECode Pause(TUint index = 0);
    virtual EECode Resume(TUint index = 0);

    virtual EECode Mute();
    virtual EECode UnMute();

private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpMsgSink;

private:
    IAudioHAL *mpAudio;
    TUint mLatency;
    TUint mBufferSize;
};
DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

