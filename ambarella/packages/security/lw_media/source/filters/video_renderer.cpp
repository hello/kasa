/**
 * video_renderer.cpp
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

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "lwmd_if.h"
#include "lwmd_log.h"

#include "internal.h"

#include "osal.h"
#include "framework.h"
#include "modules_interface.h"

#include "video_renderer.h"

IFilter *gfCreateVideoRendererFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    return CVideoRenderer::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

static EVideoRendererModuleID _guess_video_renderer_type_from_string(TChar *string)
{
    if (!string) {
        LOG_WARN("NULL input in _guess_video_renderer_type_from_string, choose default\n");
        return EVideoRendererModuleID_AMBADSP;
    }

    if (!strncmp(string, DNameTag_AMBA, strlen(DNameTag_AMBA))) {
        return EVideoRendererModuleID_AMBADSP;
    } else if (!strncmp(string, DNameTag_LinuxFB, strlen(DNameTag_LinuxFB))) {
        return EVideoRendererModuleID_LinuxFrameBuffer;
    }

    LOG_WARN("unknown string tag(%s) in _guess_video_renderer_type_from_string, choose default\n", string);
    return EVideoRendererModuleID_AMBADSP;
}

CVideoRendererInput *CVideoRendererInput::Create(const TChar *name, IFilter *pFilter, CIQueue *queue, TUint index)
{
    CVideoRendererInput *result = new CVideoRendererInput(name, pFilter, index);
    if (result && result->Construct(queue) != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

EECode CVideoRendererInput::Construct(CIQueue *queue)
{
    LOGM_DEBUG("CVideoRendererInput::Construct.\n");
    EECode err = inherited::Construct(queue);
    return err;
}

CVideoRendererInput::~CVideoRendererInput()
{
    LOGM_RESOURCE("CVideoRendererInput::~CVideoRendererInput.\n");
}

//-----------------------------------------------------------------------
//
// CVideoRenderer
//
//-----------------------------------------------------------------------
IFilter *CVideoRenderer::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    CVideoRenderer *result = new CVideoRenderer(pName, pPersistMediaConfig, pEngineMsgSink, index);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CVideoRenderer::CVideoRenderer(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
    : inherited(pName, index, pPersistMediaConfig, pEngineMsgSink)
    , mpVideoRenderer(NULL)
    , mpCurrentInputPin(NULL)
    , mnInputPinsNumber(0)
    , mCurVideoRendererID(EVideoRendererModuleID_Invalid)
    , mpBuffer(NULL)
    , mbVideoRendererSetup(0)
    , mbRecievedEosSignal(0)
    , mbPaused(0)
    , mbEOS(0)
{
    TUint i = 0;

    for (i = 0; i < EConstVideoRendererMaxInputPinNumber; i ++) {
        mpInputPins[i] = 0;
    }

    mCurVideoWidth = 0;
    mCurVideoHeight = 0;
    mVideoFramerateNum = 0;
    mVideoFramerateDen = 0;
    mVideoFramerate = 0;
}

EECode CVideoRenderer::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleVideoRendererFilter);

    //mConfigLogLevel = ELogLevel_Debug;
    //mConfigLogOption = DCAL_BITMASK(ELogOption_State) | DCAL_BITMASK(ELogOption_Resource) | DCAL_BITMASK(ELogOption_Flow);

    return inherited::Construct();
}

CVideoRenderer::~CVideoRenderer()
{
}

void CVideoRenderer::Delete()
{
    TUint i = 0;

    LOGM_RESOURCE("CVideoRenderer::~CVideoRenderer(), before delete inputpins.\n");
    for (i = 0; i < EConstVideoRendererMaxInputPinNumber; i ++) {
        if (mpInputPins[i]) {
            mpInputPins[i]->Delete();
            mpInputPins[i] = NULL;
        }
    }

    LOGM_RESOURCE("CVideoRenderer::~CVideoRenderer(), before mpVideoRenderer->Delete().\n");
    if (mpVideoRenderer) {
        mpVideoRenderer->Destroy();
        mpVideoRenderer = NULL;
    }

    LOGM_RESOURCE("CVideoRenderer::~CVideoRenderer(), end.\n");

    inherited::Delete();
}

EECode CVideoRenderer::Initialize(TChar *p_param)
{
    EVideoRendererModuleID id;
    id = _guess_video_renderer_type_from_string(p_param);

    LOGM_INFO("EVideoRendererModuleID %d\n", id);

    if (mbVideoRendererSetup) {

        DASSERT(mpVideoRenderer);
        if (!mpVideoRenderer) {
            LOGM_FATAL("[Internal bug], why the mpVideoRenderer is NULL here?\n");
            return EECode_InternalLogicalBug;
        }

        LOGM_INFO("before mpVideoRenderer->DestroyContext()\n");
        mpVideoRenderer->DestroyContext();
        mbVideoRendererSetup = 0;

        if (id != mCurVideoRendererID) {
            LOGM_INFO("before mpVideoRenderer->Delete(), cur id %d, request id %d\n", mCurVideoRendererID, id);
            mpVideoRenderer->Destroy();
            mpVideoRenderer = NULL;
        }

    }

    if (!mpVideoRenderer) {
        LOGM_INFO("before gfModuleFactoryVideoRenderer(%d)\n", id);
        mpVideoRenderer = gfModuleFactoryVideoRenderer(id, mpPersistMediaConfig, mpEngineMsgSink);
        if (mpVideoRenderer) {
            mCurVideoRendererID = id;
        } else {
            mCurVideoRendererID = EVideoRendererModuleID_Auto;
            LOGM_FATAL("[Internal bug], request gfModuleFactoryVideoRenderer(%d) fail\n", id);
            return EECode_InternalLogicalBug;
        }
    }

    LOGM_INFO("[VideoRenderer flow], CVideoRenderer::Initialize() done\n");

    return EECode_OK;
}

EECode CVideoRenderer::Finalize()
{
    if (mpVideoRenderer) {

        LOGM_INFO("before mpVideoRenderer->DestroyContext()\n");
        DASSERT(mbVideoRendererSetup);
        if (mbVideoRendererSetup) {
            mpVideoRenderer->DestroyContext();
            mbVideoRendererSetup = 0;
        }

        LOGM_INFO("before mpVideoRenderer->Delete(), cur id %d\n", mCurVideoRendererID);
        mpVideoRenderer->Destroy();
        mpVideoRenderer = NULL;
    }

    return EECode_OK;
}

void CVideoRenderer::PrintState()
{
    LOGM_FATAL("TO DO\n");
}

EECode CVideoRenderer::Run()
{
    //debug assert
    DASSERT(mpVideoRenderer);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    //DASSERT(mbVideoRendererSetup);

    inherited::Run();

    return EECode_OK;
}

EECode CVideoRenderer::Start()
{
    //debug assert
    DASSERT(mpVideoRenderer);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    //DASSERT(mbVideoRendererSetup);

    inherited::Start();

    return EECode_OK;
}

EECode CVideoRenderer::Stop()
{
    //debug assert
    DASSERT(mpVideoRenderer);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    //DASSERT(mbVideoRendererSetup);

    inherited::Stop();

    return EECode_OK;
}

void CVideoRenderer::Pause()
{
    //debug assert
    DASSERT(mpVideoRenderer);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    DASSERT(mbVideoRendererSetup);

    inherited::Pause();

    return;
}

void CVideoRenderer::Resume()
{
    //debug assert
    DASSERT(mpVideoRenderer);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    DASSERT(mbVideoRendererSetup);

    inherited::Resume();

    return;
}

void CVideoRenderer::Flush()
{
    //debug assert
    DASSERT(mpVideoRenderer);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    DASSERT(mbVideoRendererSetup);

    inherited::Flush();

    return;
}

void CVideoRenderer::ResumeFlush()
{
    inherited::ResumeFlush();
    return;
}

EECode CVideoRenderer::Suspend(TUint release_context)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CVideoRenderer::ResumeSuspend(TComponentIndex input_index)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CVideoRenderer::SwitchInput(TComponentIndex focus_input_index)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CVideoRenderer::FlowControl(EBufferType flowcontrol_type)
{
    LOGM_FATAL("TO DO\n");

    return EECode_NoImplement;
}

EECode CVideoRenderer::AddInputPin(TUint &index, TUint type)
{
    if (StreamType_Video == type) {
        if (mnInputPinsNumber >= EConstVideoRendererMaxInputPinNumber) {
            LOGM_ERROR("Max input pin number reached.\n");
            return EECode_InternalLogicalBug;
        }

        index = mnInputPinsNumber;
        DASSERT(!mpInputPins[mnInputPinsNumber]);
        if (mpInputPins[mnInputPinsNumber]) {
            LOGM_FATAL("mpInputPins[mnInputPinsNumber] here, must have problems!!! please check it\n");
        }

        mpInputPins[mnInputPinsNumber] = CVideoRendererInput::Create("[Audio input pin for CVideoRenderer]", (IFilter *) this, mpWorkQ->MsgQ(), index);
        DASSERT(mpInputPins[mnInputPinsNumber]);

        mnInputPinsNumber ++;
        return EECode_OK;
    } else {
        LOGM_FATAL("BAD input pin type %d\n", type);
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

IInputPin *CVideoRenderer::GetInputPin(TUint index)
{
    if (EConstVideoRendererMaxInputPinNumber > index) {
        return mpInputPins[index];
    } else {
        LOGM_ERROR("BAD index %d, max value %d, in CVideoRenderer::GetInputPin()\n", index, EConstVideoRendererMaxInputPinNumber);
    }

    return NULL;
}

void CVideoRenderer::EventNotify(EEventType type, TU64 param1, TPointer param2)
{
    LOGM_ERROR("TO DO\n");
}

EECode CVideoRenderer::FilterProperty(EFilterPropertyType property, TUint set_or_get, void *p_param)
{
    EECode err = EECode_OK;

    switch (property) {

        case EFilterPropertyType_vrenderer_trickplay_step:
            if (DLikely(mpVideoRenderer)) {
                mpVideoRenderer->StepPlay();
            } else {
                LOGM_ERROR("NULL mpVideoRenderer\n");
            }
            break;

        case EFilterPropertyType_vrenderer_trickplay_pause:
            if (DLikely(mpVideoRenderer)) {
                mpVideoRenderer->Pause();
            } else {
                LOGM_ERROR("NULL mpVideoRenderer\n");
            }
            break;

        case EFilterPropertyType_vrenderer_trickplay_resume:
            if (DLikely(mpVideoRenderer)) {
                mpVideoRenderer->Resume();
            } else {
                LOGM_ERROR("NULL mpVideoRenderer\n");
            }
            break;

        case EFilterPropertyType_vrenderer_sync_to_audio:
            LOGM_FATAL("TO DO\n");
            break;

        case EFilterPropertyType_vrenderer_align_all_video_streams:
            LOGM_FATAL("TO DO\n");
            break;

        case EFilterPropertyType_vrenderer_get_last_shown_timestamp: {
                SQueryLastShownTimeStamp *ptime = (SQueryLastShownTimeStamp *) p_param;
                if (ptime) {
                    mpVideoRenderer->QueryLastShownTimeStamp(ptime->time);
                }
            }
            break;

        default:
            LOGM_FATAL("BAD property 0x%08x\n", property);
            err = EECode_InternalLogicalBug;
            break;
    }

    return err;
}

void CVideoRenderer::GetInfo(INFO &info)
{
    info.mPriority = 0;
    info.mFlags = 0;
    info.nInput = 1;
    info.nOutput = 0;

    info.pName = mpModuleName;
}

void CVideoRenderer::PrintStatus()
{
    TUint i = 0;

    LOGM_ALWAYS("msState=%d, %s, mnCurrentInputPinNumber\n", msState, gfGetModuleStateString(msState));

    for (i = 0; i < mnInputPinsNumber; i ++) {
        LOGM_ALWAYS("       inputpin[%p, %d]'s status:\n", mpInputPins[i], i);
        if (mpInputPins[i]) {
            mpInputPins[i]->PrintStatus();
        }
    }

    return;
}

void CVideoRenderer::postMsg(TUint msg_code)
{
    DASSERT(mpEngineMsgSink);
    if (mpEngineMsgSink) {
        SMSG msg;
        msg.code = msg_code;
        LOGM_FLOW("Post msg_code %d to engine.\n", msg_code);
        mpEngineMsgSink->MsgProc(msg);
        LOGM_FLOW("Post msg_code %d to engine done.\n", msg_code);
    }
}

bool CVideoRenderer::readInputData(CVideoRendererInput *inputpin, EBufferType &type)
{
    DASSERT(!mpBuffer);
    DASSERT(inputpin);

    if (!inputpin->PeekBuffer(mpBuffer)) {
        LOGM_FATAL("No buffer?\n");
        return false;
    }

    type = mpBuffer->GetBufferType();

    if (EBufferType_FlowControl_EOS == type) {
        LOGM_FLOW("CVideoRenderer %p get EOS.\n", this);
        DASSERT(!mbEOS);
        mbEOS = 1;
        return true;
    } else {
        LOGM_ERROR("mpBuffer->GetBufferType() %d, mpBuffer->mFlags() %d.\n", mpBuffer->GetBufferType(), mpBuffer->mFlags);
        return true;
    }

    return true;
}

EECode CVideoRenderer::flushInputData()
{
    if (mpBuffer) {
        mpBuffer->Release();
        mpBuffer = NULL;
    }

    LOGM_FLOW("before purge input pins\n");

    if (mpCurrentInputPin) {
        mpCurrentInputPin->Purge();
    }

    LOGM_FLOW("after purge input pins\n");

    return EECode_OK;
}

EECode CVideoRenderer::processCmd(SCMD &cmd)
{
    EECode err = EECode_OK;

    switch (cmd.code) {

        case ECMDType_ExitRunning:
            mbRun = 0;
            flushInputData();
            msState = EModuleState_Halt;
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_Start:
            msState = EModuleState_Idle;
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_Stop:
            flushInputData();
            msState = EModuleState_WaitCmd;
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_Pause:
            mbPaused = 1;
            break;

        case ECMDType_Resume:
            if (EModuleState_Pending == msState) {
                msState = EModuleState_Idle;
            } else if (EModuleState_Completed == msState) {
                msState = EModuleState_Idle;
            } else if (EModuleState_Stopped == msState) {
                msState = EModuleState_Idle;
            }
            mbPaused = 0;
            break;

        case ECMDType_Flush:
            if (mpBuffer) {
                mpBuffer->Release();
                mpBuffer = NULL;
            }
            if (mpCurrentInputPin) {
                mpCurrentInputPin->Purge(0);
            }
            msState = EModuleState_Stopped;
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_ResumeFlush:
            msState = EModuleState_Idle;
            mpWorkQ->CmdAck(EECode_OK);
            break;

        default:
            LOGM_ERROR("processCmd, wrong cmd %d, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code));
            break;
    }

    return err;
}

void CVideoRenderer::OnRun()
{
    SCMD cmd;
    CIQueue::QType type;
    CIQueue::WaitResult result;

    msState = EModuleState_WaitCmd;
    mbRun = 1;

    mpWorkQ->CmdAck(EECode_OK);
    LOGM_INFO("OnRun loop, start\n");

    mbRun = true;

    while (mbRun) {

        LOGM_STATE("OnRun: start switch, msState=%d, %s\n", msState, gfGetModuleStateString(msState));

        switch (msState) {

            case EModuleState_WaitCmd:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EModuleState_Halt:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EModuleState_Idle:
                if (mbPaused) {
                    msState = EModuleState_Pending;
                    break;
                }

                type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd), &result);
                if (type == CIQueue::Q_MSG) {
                    processCmd(cmd);
                } else {
                    mpCurrentInputPin = (CVideoRendererInput *)result.pOwner;
                    DASSERT(!mpBuffer);
                    if (mpCurrentInputPin->PeekBuffer(mpBuffer)) {
                        msState = EModuleState_Running;
                    } else {
                        LOGM_FATAL("mpCurrentInputPin->PeekBuffer(mpBuffer) fail?\n");
                        msState = EModuleState_Error;
                    }
                }
                break;

            case EModuleState_Completed:
            case EModuleState_Stopped:
            case EModuleState_Pending:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EModuleState_Error:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EModuleState_Running:
                DASSERT(mpBuffer && mpVideoRenderer);

                if (EBufferType_FlowControl_EOS == mpBuffer->GetBufferType()) {
                    DASSERT(mpEngineMsgSink);
                    if (mpEngineMsgSink) {
                        SMSG msg;
                        msg.code = EMSGType_PlaybackEOS;
                        msg.p_owner = (TULong)((IFilter *)this);
                        mpEngineMsgSink->MsgProc(msg);
                    }
                    mpBuffer->Release();
                    mpBuffer = NULL;
                    msState = EModuleState_Completed;
                    break;
                }

                if (DUnlikely(mpBuffer->GetBufferFlags() & CIBuffer::SYNC_POINT)) {
                    mCurVideoWidth = mpBuffer->mVideoWidth;
                    mCurVideoHeight = mpBuffer->mVideoHeight;
                    mVideoFramerateNum = mpBuffer->mVideoFrameRateNum;
                    mVideoFramerateDen = mpBuffer->mVideoFrameRateDen;
                    mVideoFramerate = mpBuffer->mVideoRoughFrameRate;
                }

                if (DUnlikely(!mbVideoRendererSetup)) {
                    SVideoParams param;
                    EECode err;

                    memset(&param, 0x0, sizeof(param));
                    param.pic_width = mCurVideoWidth;
                    param.pic_height = mCurVideoHeight;
                    param.pic_offset_x = 0;
                    param.pic_offset_y = 0;
                    param.framerate_num = mVideoFramerateNum;
                    param.framerate_den = mVideoFramerateDen;

                    LOGM_NOTICE("renderer: video resolution %dx%d\n", mCurVideoWidth, mCurVideoHeight);

                    err = mpVideoRenderer->SetupContext(&param);
                    if (DUnlikely(EECode_OK != err)) {
                        LOGM_ERROR("SetupContext(), %d, %s fail\n", err, gfGetErrorCodeString(err));
                        msState = EModuleState_Error;
                        break;
                    }

                    mbVideoRendererSetup = 1;
                }

                mpVideoRenderer->Render(mpBuffer);

                mpBuffer->Release();
                mpBuffer = NULL;
                msState = EModuleState_Idle;
                break;

            default:
                LOGM_ERROR("CVideoRenderer: OnRun: state invalid: 0x%x\n", (TUint)msState);
                msState = EModuleState_Error;
                break;
        }
    }
}

