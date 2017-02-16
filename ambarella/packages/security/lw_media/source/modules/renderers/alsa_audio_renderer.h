/*
 * alsa_audio_renderer.h
 *
 * History:
 *    2013/05/17 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __ALSA_AUDIO_RENDERER_H__
#define __ALSA_AUDIO_RENDERER_H__

class CAudioRendererAlsa
    : public CObject
    , virtual public IAudioRenderer
{
    typedef CObject inherited;

protected:
    CAudioRendererAlsa(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    virtual ~CAudioRendererAlsa();

public:
    static IAudioRenderer *Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    virtual void Destroy();

public:
    virtual EECode SetupContext(SAudioParams *param);
    virtual EECode DestroyContext();

    virtual EECode Start(TUint index = 0);
    virtual EECode Stop(TUint index = 0);
    virtual EECode Flush(TUint index = 0);

    virtual EECode Render(CIBuffer *p_buffer, TUint index = 0);

    virtual EECode Pause(TUint index = 0);
    virtual EECode Resume(TUint index = 0);

    virtual EECode SyncTo(TTime pts, TUint index = 0);

    virtual EECode SyncMultipleStream(TUint master_index);

private:
    EECode Construct();
private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpMsgSink;

private:
    SFAudioAL mfAudioAL;
    void *mpAudioAL;

    TUint mLatency;
    TUint mBufferSize;
    TUint mChannels;
    TUint mBytePerFrame;

};
#endif

