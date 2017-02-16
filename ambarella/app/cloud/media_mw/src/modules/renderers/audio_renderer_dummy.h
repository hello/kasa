/*
 * audio_renderer_dummy.h
 *
 * History:
 *    2014/01/09 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __AUDIO_RENDERER_DUMMY_H__
#define __AUDIO_RENDERER_DUMMY_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

class CAudioRendererDummy
    : public CObject
    , virtual public IAudioRenderer
{
    typedef CObject inherited;

protected:
    CAudioRendererDummy(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    virtual ~CAudioRendererDummy();

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
    IAudioHAL *mpAudio;
    TUint mLatency;
    TUint mBufferSize;
    TUint mChannels;
    TUint mBytePerFrame;

};
DCONFIG_COMPILE_OPTION_HEADERFILE_END
#endif

