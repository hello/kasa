/*
 * amba_video_renderer.h
 *
 * History:
 *    2013/04/05 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __AMBA_VIDEO_RENDERER_H__
#define __AMBA_VIDEO_RENDERER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

class CAmbaVideoRenderer
    : public CObject
    , virtual public IVideoRenderer
{
    typedef CObject inherited;

protected:
    CAmbaVideoRenderer(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    virtual ~CAmbaVideoRenderer();

public:
    static IVideoRenderer *Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    virtual void Destroy();

public:
    virtual EECode SetupContext(SVideoParams *param = NULL);
    virtual EECode DestroyContext();

    virtual EECode Start(TUint index = 0);
    virtual EECode Stop(TUint index = 0);
    virtual EECode Flush(TUint index = 0);

    virtual EECode Render(CIBuffer *p_buffer, TUint index = 0);

    virtual EECode Pause(TUint index = 0);
    virtual EECode Resume(TUint index = 0);
    virtual EECode StepPlay(TUint index = 0);

    virtual EECode SyncTo(TTime pts, TUint index = 0);

    virtual EECode SyncMultipleStream(TUint master_index);

    virtual EECode WaitVoutDormant(TUint index);
    virtual EECode WakeVout(TUint index);

private:
    EECode Construct();
private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpMsgSink;

private:
    IDSPAPI *mpDSP;

};
DCONFIG_COMPILE_OPTION_HEADERFILE_END
#endif

