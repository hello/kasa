/*******************************************************************************
 * basic_scene_director.cpp
 *
 * History:
 *    2014/11/11 - [Zhi He] create file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

#include "common_base.h"

#include "platform_al_if.h"
#include "app_framework_if.h"

#include "basic_scene_director.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

#ifdef DCONFIG_TEST_END2END_DELAY
CIClockReference *gpSystemClockReference = NULL;
#endif

ISceneDirector *gfCreateBasicSceneDirector(EVisualSettingPrefer visual_platform_prefer, SPersistCommonConfig *p_common_config, SSharedComponent *p_shared_component)
{
    return CBasicSceneDirector::Create(visual_platform_prefer, p_common_config, p_shared_component);
}

//-----------------------------------------------------------------------
//
// CBasicSceneDirector
//
//-----------------------------------------------------------------------
CBasicSceneDirector *CBasicSceneDirector::Create(EVisualSettingPrefer visual_platform_prefer, SPersistCommonConfig *p_common_config, SSharedComponent *p_shared_component)
{
    CBasicSceneDirector *result = new CBasicSceneDirector(visual_platform_prefer, p_common_config, p_shared_component);
    if (DUnlikely((result) && (EECode_OK != result->Construct()))) {
        result->Destroy();
        result = NULL;
    }
    return result;
}

CObject *CBasicSceneDirector::GetObject() const
{
    return (CObject *) this;
}

void CBasicSceneDirector::Delete()
{
    if (mpWorkQueue) {
        mpWorkQueue->Delete();
        mpWorkQueue = NULL;
    }

    if (mpPlatformWindow) {
        mpPlatformWindow->DeInitialize();
        mpPlatformWindow->Destroy();
        mpPlatformWindow = NULL;
    }

    if (mpSceneList) {
        delete mpSceneList;
        mpSceneList = NULL;
    }

#ifndef DCONFIG_TEST_END2END_DELAY
    if (mpClockReference) {
        mpClockReference->Delete();
        mpClockReference = NULL;
    }
#endif

    CObject::Delete();
}

CBasicSceneDirector::CBasicSceneDirector(EVisualSettingPrefer visual_platform_prefer, SPersistCommonConfig *p_common_config, SSharedComponent *p_shared_component)
    : inherited("CBasicSceneDirector", 0)
    , mpWorkQueue(NULL)
    , mpCommonConfig(p_common_config)
    , mpSceneList(NULL)
    , mpPlatformWindow(NULL)
    , mpMainWindowName(NULL)
    , mRequestMainWindowPosX(0)
    , mRequestMainWindowPosY(0)
    , mRequestMainWindowWdith(0)
    , mRequestMainWindowHeight(0)
    , mRequestMainWindowState(EWindowState_Nomal)
    , mbLoop(1)
    , mbDrawFirstFrame(0)
    , mVisualSetting(EVisualSetting_opengl)
    , mVisualSettingPrefer(visual_platform_prefer)
    , msState(ESceneDirectorState_idle)
{
    if (p_shared_component) {
        mpClockManager = p_shared_component->p_clock_manager;
        mpMsgQueue = p_shared_component->p_msg_queue;
#ifdef DCONFIG_TEST_END2END_DELAY
        mpClockReference = p_shared_component->p_clock_reference;
#else
        mpClockReference = NULL;
#endif
    } else {
        mpClockManager = NULL;
        mpMsgQueue = NULL;
        mpClockReference = NULL;
    }
}

EECode CBasicSceneDirector::Construct()
{
    if (DLikely(mpClockManager && (!mpClockReference))) {
        mpClockReference = CIClockReference::Create();
        if (DUnlikely(!mpClockReference)) {
            LOGM_FATAL("CIClockReference::Create() fail\n");
            return EECode_NoMemory;
        }

        EECode ret = mpClockManager->RegisterClockReference(mpClockReference);
        DASSERT_OK(ret);
    } else {
#ifndef DCONFIG_TEST_END2END_DELAY
        LOGM_FATAL("NULL mpClockManager\n");
        return EECode_BadState;
#endif
    }

#ifdef DCONFIG_TEST_END2END_DELAY
    gpSystemClockReference = mpClockReference;
#endif

    mpSceneList = new CIDoubleLinkedList();
    if (DUnlikely(!mpSceneList)) {
        LOG_FATAL("alloc CIDoubleLinkedList fail\n");
        return EECode_NoMemory;
    }

    DSET_MODULE_LOG_CONFIG(ELogAPPFrameworkSceneDirector);
    //mConfigLogLevel = ELogLevel_Debug;
    //mConfigLogOption = 0xffffffff;

    mpWorkQueue = CIWorkQueue::Create((IActiveObject *)this);
    if (DUnlikely(!mpWorkQueue)) {
        LOGM_ERROR("Create CIWorkQueue fail.\n");
        return EECode_NoMemory;
    }

    //LOGM_INFO("before mpWorkQueue->Run().\n");
    DASSERT(mpWorkQueue);
    mbLoop = 1;
    mpWorkQueue->Run();
    //LOGM_INFO("after mpWorkQueue->Run().\n");

    return EECode_OK;
}

CBasicSceneDirector::~CBasicSceneDirector()
{
    if (mpWorkQueue) {
        mpWorkQueue->Delete();
        mpWorkQueue = NULL;
    }

    if (mpPlatformWindow) {
        mpPlatformWindow->DeInitialize();
        mpPlatformWindow->Destroy();
        mpPlatformWindow = NULL;
    }

    if (mpSceneList) {
        delete mpSceneList;
        mpSceneList = NULL;
    }
}

EECode CBasicSceneDirector::AddScene(CIScene *scene)
{
    if (DLikely(mpSceneList)) {
        mpSceneList->InsertContent(NULL, (void *) scene, 0);
    } else {
        LOG_FATAL("mpSceneList is NULL\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

void CBasicSceneDirector::RemoveScene(CIScene *scene)
{
    if (DLikely(mpSceneList)) {
        mpSceneList->RemoveContent((void *) scene);
    } else {
        LOG_FATAL("mpSceneList is NULL\n");
    }
}

EECode CBasicSceneDirector::SwitchScene(CIScene *scene)
{
    LOG_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CBasicSceneDirector::StartRunning()
{
    LOGM_NOTICE("StartRunning start.\n");
    EECode err = mpWorkQueue->SendCmd(ECMDType_Start, NULL);
    LOGM_NOTICE("StartRunning end, ret %d, %s.\n", err, gfGetErrorCodeString(err));

    return err;
}

EECode CBasicSceneDirector::ExitRunning()
{
    LOGM_NOTICE("ExitRunning start.\n");
    EECode err = mpWorkQueue->SendCmd(ECMDType_Stop, NULL);
    LOGM_NOTICE("ExitRunning end, ret %d, %s.\n", err, gfGetErrorCodeString(err));

    return err;
}

void CBasicSceneDirector::OnRun()
{
    SCMD cmd;
    CIDoubleLinkedList::SNode *pnode = NULL;
    mpWorkQueue->CmdAck(EECode_OK);
    EECode err = EECode_OK;
    TTime target_time = 0;
    TTime time_cur = 0;
    DASSERT(mpClockReference);

    LOGM_NOTICE("OnRun() loop, start\n");

    while (mbLoop) {

        //LOGM_STATE("msState(%d)\n", msState);
        switch (msState) {

            case ESceneDirectorState_idle:
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case ESceneDirectorState_running:
                if (mpWorkQueue->PeekCmd(cmd)) {
                    processCmd(cmd);
                    if (DUnlikely(ESceneDirectorState_running != msState)) {
                        break;
                    }
                }

                time_cur = mpClockReference->GetCurrentTime();
                if (target_time) {
                    if (time_cur > target_time) {
                        target_time += 16666;
                    } else {
                        if (mpPlatformWindow) {
                            mpPlatformWindow->ProcessEvent();
                        }
                        continue;
                    }
                } else {
                    target_time = time_cur + 16666;
                }

                if (mpPlatformWindow) {
                    mpPlatformWindow->ProcessEvent();
                }

                if (DLikely(mbDrawFirstFrame)) {
                    if (0 == needReDraw()) {
                        //gfOSmsleep(1);
                        break;
                    }
                }

                err = prepareRendering();
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("prepareRendering() fail, enter error state\n");
                    msState = ESceneDirectorState_error;
                    break ;
                }

                mpPlatformWindow->PreDrawing();
                //LOG_PRINTF("[DEBUG]: before renderScenes\n");
                err = renderScenes();
                if (DLikely(EECode_OK == err)) {
                    mpPlatformWindow->SwapBuffers();
                } else if (EECode_OK_NoActionNeeded == err) {
                    if (DLikely(mbDrawFirstFrame)) {
                    } else {
                        mpPlatformWindow->SwapBuffers();
                        mbDrawFirstFrame = 1;
                    }
                    //LOG_PRINTF("[DEBUG]: nothing updated, not update frame buffer\n");
                } else {
                    LOG_ERROR("renderScenes() ret %d, %s\n", err, gfGetErrorCodeString(err));
                }
                break;

            case ESceneDirectorState_pending:
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case ESceneDirectorState_error:
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case ESceneDirectorState_stepmode:
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            default:
                LOG_FATAL("Unexpected state(%d) in CBasicSceneDirector\n", msState);
                break;
        }
    }

    LOGM_NOTICE("OnRun() loop, exit\n");
}

EECode CBasicSceneDirector::PreferMainWindowSetting()
{
    LOG_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CBasicSceneDirector::PreferVisualSetting(EVisualSetting setting)
{
    mVisualSetting = setting;
    return EECode_OK;
}

EECode CBasicSceneDirector::PreferAudioSetting()
{
    LOG_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CBasicSceneDirector::CreateMainWindow(const TChar *name, TDimension posx, TDimension posy, TDimension width, TDimension height, EWindowState state)
{
    mpMainWindowName = name;
    mRequestMainWindowPosX = posx;
    mRequestMainWindowPosY = posy;
    mRequestMainWindowWdith = width;
    mRequestMainWindowHeight = height;
    mRequestMainWindowState = state;

    LOGM_NOTICE("CreateMainWindow start.\n");
    EECode err = mpWorkQueue->SendCmd(ECMDType_CreateMainWindow, NULL);
    LOGM_NOTICE("CreateMainWindow end, ret %d, %s.\n", err, gfGetErrorCodeString(err));

    return err;
}

EECode CBasicSceneDirector::MoveMainWindow(TDimension posx, TDimension posy)
{
    SCMD cmd;
    cmd.code = ECMDType_MoveMainWindow;
    cmd.res64_1 = posx;
    cmd.res64_2 = posy;

    EECode err = mpWorkQueue->SendCmd(cmd);

    return err;
}

EECode CBasicSceneDirector::ReSizeMainWindow(TDimension width, TDimension height)
{
    SCMD cmd;
    cmd.code = ECMDType_ReSizeMainWindow;
    cmd.res64_1 = width;
    cmd.res64_2 = height;

    EECode err = mpWorkQueue->SendCmd(cmd);

    return err;
}

EECode CBasicSceneDirector::ReSizeClientWindow(TDimension width, TDimension height)
{
    SCMD cmd;
    cmd.code = ECMDType_ReSizeClientWindow;
    cmd.res64_1 = width;
    cmd.res64_2 = height;

    EECode err = mpWorkQueue->SendCmd(cmd);

    return err;
}

TULong CBasicSceneDirector::GetWindowHandle() const
{
    if (mpPlatformWindow) {
        return mpPlatformWindow->GetWindowHandle();
    }

    return 0;
}

void CBasicSceneDirector::Destroy()
{
    Delete();
}

EECode CBasicSceneDirector::renderScenes()
{
    CIScene *p_scene = NULL;
    EECode err = EECode_OK;
    TU32 updated = 0;

    CIDoubleLinkedList::SNode *p_node = mpSceneList->FirstNode();
    while (p_node) {
        p_scene = (CIScene *) p_node->p_context;
        if (DLikely(p_scene)) {
            err = p_scene->Draw(updated);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("p_scene(%p)->Draw(), ret %d, %s\n", p_scene, err, gfGetErrorCodeString(err));
                return err;
            }
        }
        p_node = mpSceneList->NextNode(p_node);
    }

    if (updated) {
        return EECode_OK;
    } else {
        return EECode_OK_NoActionNeeded;
    }

}

TU32 CBasicSceneDirector::needReDraw()
{
    CIScene *p_scene = NULL;
    TU32 needed = 0;

    CIDoubleLinkedList::SNode *p_node = mpSceneList->FirstNode();
    while (p_node) {
        p_scene = (CIScene *) p_node->p_context;
        if (DLikely(p_scene)) {
            p_scene->NeedReDraw(needed);
            if (needed) {
                return 1;
            }
        }
        p_node = mpSceneList->NextNode(p_node);
    }

    return 0;
}

void CBasicSceneDirector::processCmd(SCMD &cmd)
{
    EECode err = EECode_OK;
    //debug check
    //LOGM_FLOW("processCmd(cmd.code %d).\n", cmd.code);
    //DASSERT(mpWorkQueue);

    switch (cmd.code) {

        case ECMDType_Start:
            msState = ESceneDirectorState_running;
            mpWorkQueue->CmdAck(EECode_OK);
            break;

        case ECMDType_Stop:
            mbLoop = 0;
            mpPlatformWindow->DestroyMainWindow();
            msState = ESceneDirectorState_pending;
            mpWorkQueue->CmdAck(EECode_OK);
            break;

        case ECMDType_CreateMainWindow:
            err = createMainWindow();
            mpWorkQueue->CmdAck(err);
            break;

        case ECMDType_MoveMainWindow:
            err = moveMainWindow((TDimension)cmd.res64_1, (TDimension)cmd.res64_2);
            mpWorkQueue->CmdAck(err);
            break;

        case ECMDType_ReSizeMainWindow:
            err = resizeMainWindow((TDimension)cmd.res64_1, (TDimension)cmd.res64_2);
            mpWorkQueue->CmdAck(err);
            break;

        case ECMDType_ReSizeClientWindow:
            err = resizeClientWindow((TDimension)cmd.res64_1, (TDimension)cmd.res64_2);
            mpWorkQueue->CmdAck(err);
            break;

        default:
            LOGM_ERROR("wrong cmd.code: %d\n", cmd.code);
            break;
    }

    //LOGM_FLOW("processCmd(cmd.code %d) end.\n", cmd.code);
    return;
}

EECode CBasicSceneDirector::setupComponentContext()
{
    CIScene *p_scene = NULL;
    EECode err = EECode_OK;

    CIDoubleLinkedList::SNode *p_node = mpSceneList->FirstNode();
    while (p_node) {
        p_scene = (CIScene *) p_node->p_context;
        if (DLikely(p_scene)) {
            err = p_scene->SetupContexts();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("p_scene(%p)->setupComponentContext(), ret %d, %s\n", p_scene, err, gfGetErrorCodeString(err));
                return err;
            }
        }
        p_node = mpSceneList->NextNode(p_node);
    }

    return EECode_OK;
}

EECode CBasicSceneDirector::prepareRendering()
{
    CIScene *p_scene = NULL;
    EECode err = EECode_OK;

    CIDoubleLinkedList::SNode *p_node = mpSceneList->FirstNode();
    while (p_node) {
        p_scene = (CIScene *) p_node->p_context;
        if (DLikely(p_scene)) {
            err = p_scene->PrepareDrawing();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("p_scene(%p)->PrepareDrawing(), ret %d, %s\n", p_scene, err, gfGetErrorCodeString(err));
                return err;
            }
        }
        p_node = mpSceneList->NextNode(p_node);
    }

    return EECode_OK;
}

EECode CBasicSceneDirector::createMainWindow()
{
    if (!mpPlatformWindow) {
        mpPlatformWindow = gfCreatePlatformWindow(mpMsgQueue, mVisualSetting, mVisualSettingPrefer);
        if (!mpPlatformWindow) {
            LOG_FATAL("gfCreatePlatformWindow() fail\n");
            return EECode_OSError;
        }
    }

    EECode err = mpPlatformWindow->Initialize();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("mpPlatformWindow->Initialize() fail, ret %d %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = mpPlatformWindow->CreateMainWindow(mpMainWindowName, mRequestMainWindowPosX, mRequestMainWindowPosY, mRequestMainWindowWdith, mRequestMainWindowHeight, mRequestMainWindowState);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("mpPlatformWindow->CreateMainWindow() fail, ret %d %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    /*
        err = mpPlatformWindow->ActivateDrawingContext();
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("mpPlatformWindow->ActivateDrawingContext() fail, ret %d %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    */
    err = mpPlatformWindow->LoadCursor(1, EMouseShape_Standard);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("mpPlatformWindow->LoadCursor() fail, ret %d %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    SRect client_rect;
    err = mpPlatformWindow->GetClientRect(client_rect);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("mpPlatformWindow->GetClientRect() fail, ret %d %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    LOGM_NOTICE("[client rect window]: width %d, height %d, pos_x %d, pos_y %d\n", client_rect.width, client_rect.height, client_rect.pos_x, client_rect.pos_y);
    err = mpPlatformWindow->InitOrthoDrawing(client_rect.width, client_rect.height);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("mpPlatformWindow->InitOrthoDrawing(%d, %d) fail, ret %d %s\n", client_rect.width, client_rect.height, err, gfGetErrorCodeString(err));
        return err;
    }

    err = setupComponentContext();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("setupComponentContext() fail, ret %d %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    return EECode_OK;
}

EECode CBasicSceneDirector::moveMainWindow(TDimension posx, TDimension posy)
{
    if (DUnlikely(!mpPlatformWindow)) {
        LOG_FATAL("NULL mpPlatformWindow in CBasicSceneDirector::moveMainWindow()\n");
        return EECode_InternalLogicalBug;
    }

    EECode err = mpPlatformWindow->MoveMainWindow(posx, posy);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("mpPlatformWindow->MoveMainWindow() fail, ret %d %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    return EECode_OK;
}

EECode CBasicSceneDirector::resizeMainWindow(TDimension width, TDimension height)
{
    if (DUnlikely(!mpPlatformWindow)) {
        LOG_FATAL("NULL mpPlatformWindow in CBasicSceneDirector::moveMainWindow()\n");
        return EECode_InternalLogicalBug;
    }

    EECode err = mpPlatformWindow->ReSizeClientWindow(width, height);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("mpPlatformWindow->ReSizeMainWindow() fail, ret %d %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    return EECode_OK;
}

EECode CBasicSceneDirector::resizeClientWindow(TDimension width, TDimension height)
{
    if (DUnlikely(!mpPlatformWindow)) {
        LOG_FATAL("NULL mpPlatformWindow in CBasicSceneDirector::moveMainWindow()\n");
        return EECode_InternalLogicalBug;
    }

    EECode err = mpPlatformWindow->SetClientSize(width, height);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("mpPlatformWindow->SetClientSize() fail, ret %d %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

