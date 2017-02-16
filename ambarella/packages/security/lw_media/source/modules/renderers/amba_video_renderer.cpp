/*
 * amba_video_renderer.cpp
 *
 * History:
 *    2013/04/09 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "lwmd_if.h"
#include "lwmd_log.h"

#include "internal.h"

#include "osal.h"
#include "framework.h"
#include "modules_interface.h"

#include "amba_dsp.h"

#ifdef BUILD_MODULE_AMBA_DSP

#include "amba_video_renderer.h"

IVideoRenderer *gfCreateAmbaVideoRendererModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
{
    return CAmbaVideoRenderer::Create(pName, pPersistMediaConfig, pEngineMsgSink);
}

//-----------------------------------------------------------------------
//
// CAmbaVideoRenderer
//
//-----------------------------------------------------------------------
CAmbaVideoRenderer::CAmbaVideoRenderer(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
    : inherited(pname)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpMsgSink(pMsgSink)
    , mIavFd(-1)
{
}

IVideoRenderer *CAmbaVideoRenderer::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    CAmbaVideoRenderer *result = new CAmbaVideoRenderer(pname, pPersistMediaConfig, pMsgSink);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

void CAmbaVideoRenderer::Destroy()
{
    Delete();
}

EECode CAmbaVideoRenderer::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleAmbaVideoRenderer);

    gfSetupDSPAlContext(&mfDSPAL);

    return EECode_OK;
}

CAmbaVideoRenderer::~CAmbaVideoRenderer()
{
}

EECode CAmbaVideoRenderer::SetupContext(SVideoParams *param)
{
    DASSERT(mpPersistMediaConfig);
    mIavFd = mpPersistMediaConfig->dsp_config.device_fd;

    return EECode_OK;
}

EECode CAmbaVideoRenderer::DestroyContext()
{
    return EECode_OK;
}

EECode CAmbaVideoRenderer::Start(TUint index)
{
    return EECode_OK;
}

EECode CAmbaVideoRenderer::Stop(TUint index)
{
    return EECode_OK;
}

EECode CAmbaVideoRenderer::Flush(TUint index)
{
    return EECode_OK;
}

EECode CAmbaVideoRenderer::Render(CIBuffer *p_buffer, TUint index)
{
    return EECode_OK;
}

EECode CAmbaVideoRenderer::Pause(TUint index)
{
    if (mfDSPAL.f_trickplay) {
        mfDSPAL.f_trickplay(mIavFd, (TU8) index, EAMDSP_TRICK_PLAY_PAUSE);
    } else {
        LOGM_ERROR("NULL mfDSPAL.f_trickplay\n");
        return EECode_BadParam;
    }
    return EECode_OK;
}

EECode CAmbaVideoRenderer::Resume(TUint index)
{
    if (mfDSPAL.f_trickplay) {
        mfDSPAL.f_trickplay(mIavFd, (TU8) index, EAMDSP_TRICK_PLAY_RESUME);
    } else {
        LOGM_ERROR("NULL mfDSPAL.f_trickplay\n");
        return EECode_BadParam;
    }
    return EECode_OK;
}

EECode CAmbaVideoRenderer::StepPlay(TUint index)
{
    if (mfDSPAL.f_trickplay) {
        mfDSPAL.f_trickplay(mIavFd, (TU8) index, EAMDSP_TRICK_PLAY_STEP);
    } else {
        LOGM_ERROR("NULL mfDSPAL.f_trickplay\n");
        return EECode_BadParam;
    }
    return EECode_OK;
}

EECode CAmbaVideoRenderer::SyncTo(TTime pts, TUint index)
{
    return EECode_OK;
}

EECode CAmbaVideoRenderer::QueryLastShownTimeStamp(TTime &pts, TUint index)
{
    if (mfDSPAL.f_query_decode_status) {
        SAmbaDSPDecodeStatus dec_status;
        dec_status.decoder_id = (TU8) index;
        mfDSPAL.f_query_decode_status(mIavFd, &dec_status);
        pts = dec_status.last_pts;
    } else {
        LOGM_ERROR("NULL mfDSPAL.f_trickplay\n");
        return EECode_BadParam;
    }
    return EECode_OK;
}

#endif

