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

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"

#include "media_mw_if.h"
#include "media_mw_utils.h"

#include "framework_interface.h"
#include "mw_internal.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

#include "amba_video_renderer.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN

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
    , mpDSP(NULL)
{
}

IVideoRenderer *CAmbaVideoRenderer::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    CAmbaVideoRenderer *result = new CAmbaVideoRenderer(pname, pPersistMediaConfig, pMsgSink);
    if (result && result->Construct() != EECode_OK) {
        //        LOGM_FATAL("CAmbaVideoRenderer->Construct() fail\n");
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

    return EECode_OK;
}

CAmbaVideoRenderer::~CAmbaVideoRenderer()
{
    //LOGM_RESOURCE("~CAmbaVideoRenderer.\n");

    //LOGM_RESOURCE("~CAmbaVideoRenderer done.\n");
}

EECode CAmbaVideoRenderer::SetupContext(SVideoParams *param)
{
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpPersistMediaConfig->dsp_config.p_dsp_handler);

    mpDSP = (IDSPAPI *)mpPersistMediaConfig->dsp_config.p_dsp_handler;

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
    SDSPTrickPlay trickplay;
    EECode err;

    DASSERT(mpDSP);

    if (mpDSP) {
        trickplay.udec_id = index;
        trickplay.trickplay_type = EDSPTrickPlay_Pause;
        err = mpDSP->DSPControl(EDSPControlType_UDEC_trickplay, (void *)&trickplay);
        DASSERT_OK(err);
    } else {
        LOGM_ERROR("NULL mpDSP\n");
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CAmbaVideoRenderer::Resume(TUint index)
{
    SDSPTrickPlay trickplay;
    EECode err;

    DASSERT(mpDSP);

    if (mpDSP) {
        trickplay.udec_id = index;
        trickplay.trickplay_type = EDSPTrickPlay_Resume;
        err = mpDSP->DSPControl(EDSPControlType_UDEC_trickplay, (void *)&trickplay);
        DASSERT_OK(err);
    } else {
        LOGM_ERROR("NULL mpDSP\n");
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CAmbaVideoRenderer::StepPlay(TUint index)
{
    SDSPTrickPlay trickplay;
    EECode err;

    DASSERT(mpDSP);

    if (mpDSP) {
        trickplay.udec_id = index;
        trickplay.trickplay_type = EDSPTrickPlay_Step;
        err = mpDSP->DSPControl(EDSPControlType_UDEC_trickplay, (void *)&trickplay);
        DASSERT_OK(err);
    } else {
        LOGM_ERROR("NULL mpDSP\n");
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CAmbaVideoRenderer::SyncTo(TTime pts, TUint index)
{
    LOGM_ERROR("TO DO\n");
    return EECode_OK;
}

EECode CAmbaVideoRenderer::SyncMultipleStream(TUint master_index)
{
    LOGM_ERROR("TO DO\n");
    return EECode_OK;
}

EECode CAmbaVideoRenderer::WaitVoutDormant(TUint index)
{
    EECode err;
    SDSPControlParams params;

    if (mpDSP) {
        params.u8_param[0] = index;

        err = mpDSP->DSPControl(EDSPControlType_UDEC_wait_vout_dormant, &params);
        return err;
    }

    LOGM_FATAL("NULL mpDSP\n");
    return EECode_InternalLogicalBug;
}

EECode CAmbaVideoRenderer::WakeVout(TUint index)
{
    EECode err;
    SDSPControlParams params;

    if (mpDSP) {
        params.u8_param[0] = index;

        err = mpDSP->DSPControl(EDSPControlType_UDEC_wake_vout, &params);
        return err;
    }

    LOGM_FATAL("NULL mpDSP\n");
    return EECode_InternalLogicalBug;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

