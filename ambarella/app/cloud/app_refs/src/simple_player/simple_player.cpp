/**
 * simple_player.cpp
 *
 * History:
 *    2015/01/09 - [Zhi He] create file
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

#include "platform_al_if.h"

#include "app_framework_if.h"

#include "media_mw_if.h"
#include "media_simple_api.h"

#include "simple_player_if.h"
#include "simple_player.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

ISimplePlayer *gfCreateSimplePlayer()
{
    CISimplePlayer *thiz = CISimplePlayer::Create();
    return (ISimplePlayer *) thiz;
}

EECode __simple_player_thread_proc(void *p)
{
    CISimplePlayer *thiz = (CISimplePlayer *) p;
    if (thiz) {
        thiz->MainLoop();
    }

    return EECode_OK;
}

CISimplePlayer::CISimplePlayer()
    : mpMainThread(NULL)
    , mpMsgQueue(NULL)
    , mpExternalMsgQueue(NULL)
    , mpMediaAPI(NULL)
    , mpMediaEngineAPI(NULL)
    , mpClockSource(NULL)
    , mpClockManager(NULL)
    , mpClockReference(NULL)
    , mpDirector(NULL)
    , mpScene(NULL)
    , mpLayer(NULL)
    , mpVisualDirectRendering(NULL)
    , mpTexture(NULL)
    , mpSoundComposer(NULL)
    , mpSoundTrack(NULL)
    , mpSoundDirectRendering(NULL)
    , mbSetupContext(0)
    , mbStartContext(0)
    , mbThreadStarted(0)
    , mbReconnecting(0)
    , mbExit(0)
    , mPbMainWindowWidth(0)
    , mPbMainWindowHeight(0)
{
    mpOwner = NULL;
    memset(&mMediaAPIContxt, 0x0, sizeof(mMediaAPIContxt));
    memset(mPreferDisplay, 0x0, sizeof(mPreferDisplay));
    memset(mPreferPlatform, 0x0, sizeof(mPreferPlatform));
}

CISimplePlayer::~CISimplePlayer()
{

}

CISimplePlayer *CISimplePlayer::Create()
{
    CISimplePlayer *thiz = new CISimplePlayer();
    return thiz;
}

void CISimplePlayer::Destroy()
{
    if (mbThreadStarted) {
        if (DLikely(mpMsgQueue)) {
            SMSG msg;
            msg.code = ECMDType_ExitProcessing;
            mpMsgQueue->PostMsg(&msg, sizeof(msg));
        }

        if (mpMainThread) {
            mpMainThread->Delete();
            mpMainThread = NULL;
        }

        if (mpMsgQueue) {
            mpMsgQueue->Delete();
            mpMsgQueue = NULL;
        }
    }
}

int CISimplePlayer::Initialize(void *owner, void *p_external_msg_queue)
{
    if (!isLogFileOpened()) {
        gfOpenLogFile("wplog.txt");
    }

    EECode err = gfInitPlatform();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("gfInitPlatform() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return (-1);
    }

    memset(&mMediaAPIContxt, 0x0, sizeof(mMediaAPIContxt));

    mpOwner = owner;

    mpMediaAPI = NULL;
    mpMediaEngineAPI = NULL;
    mpClockSource = NULL;
    mpClockManager = NULL;
    mpClockReference = NULL;
    mpDirector = NULL;
    mpScene = NULL;
    mpLayer = NULL;
    mpVisualDirectRendering = NULL;
    mpTexture = NULL;

    mpSoundComposer = NULL;
    mpSoundTrack = NULL;
    mpSoundDirectRendering = NULL;

    mbSetupContext = 0;
    mbStartContext = 0;
    mbThreadStarted = 0;
    mbReconnecting = 0;
    mbExit = 0;

    memset(mPreferDisplay, 0x0, DMaxPreferStringLen);
    memset(mPreferPlatform, 0x0, DMaxPreferStringLen);

    mPbMainWindowWidth = 0;
    mPbMainWindowHeight = 0;

    gfMediaSimpleAPILoadDefaultPlaybackConfig(&mMediaAPIContxt);
    gfMediaSimpleAPILoadMediaConfig(&mMediaAPIContxt, "wp.ini");

    loadCurrentConfig("wp_cur.ini");
    checkWithSystemSettings();
    mMediaAPIContxt.setting.b_constrain_latency = 1;

    gfMediaSimpleAPIPrintMediaConfig(&mMediaAPIContxt);

    mpMsgQueue = CIQueue::Create(NULL, NULL, sizeof(SMSG), 64);

    if (DUnlikely(!mpMsgQueue)) {
        LOG_ERROR("CIQueue::Create() fail\n");
        return EECode_NoMemory;
    }

    mpExternalMsgQueue = (CIQueue *) p_external_msg_queue;

    mpMainThread = gfCreateThread("simple_player_main_thread", __simple_player_thread_proc, (void *) this);
    mbThreadStarted = 1;

    return 0;
}

int CISimplePlayer::Play(char *url, char *title_name)
{
    if (DUnlikely((!mbThreadStarted) || (!mpMsgQueue))) {
        LOG_FATAL("not initialized!\n");
        return (-2);
    }

    SMSG msg;

    if (title_name) {
        snprintf(mWindowTitleName, sizeof(mWindowTitleName), "%s", title_name);
    } else {
        strcpy(mWindowTitleName, "liveplayer");
    }

    if (DUnlikely(!url)) {
        LOG_WARN("url is NULL, use last used url [%s]\n", mMediaAPIContxt.setting.pb_source_url[0]);
        msg.code = ECMDType_PlayUrl;
    } else if (DMAX_URL_LENGTH <= strlen(url)) {
        LOG_FATAL("url is too long\n");
        return (-3);
    } else {
        if (!strcmp(mMediaAPIContxt.setting.pb_source_url[0], url)) {
            msg.code = ECMDType_PlayUrl;
        } else {
            strcpy(mMediaAPIContxt.setting.pb_source_url[0], url);
            msg.code = ECMDType_PlayNewUrl;
        }
    }

    mpMsgQueue->PostMsg(&msg, sizeof(msg));

    return 0;
}

void CISimplePlayer::MainLoop()
{
    LOG_NOTICE("CISimplePlayer::MainLoop() begin\n");

    do {
        LOG_NOTICE("CISimplePlayer::MainLoop(), before waitPlay()\n");

        EECode err = waitPlay();
        if (DUnlikely(EECode_OK != err)) {
            LOG_WARN("waitPlay() ret %d, %s\n", err, gfGetErrorCodeString(err));
            break;
        }

        if (!mbSetupContext) {
            LOG_NOTICE("CISimplePlayer::MainLoop(), before setupContext()\n");
            err = setupContext();
            if (EECode_OK != err) {
                LOG_ERROR("setupContext() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                destroyContext();
                break;
            }
            mbSetupContext = 1;
        }

        if (!mbStartContext) {
            LOG_NOTICE("CISimplePlayer::MainLoop(), before startContext()\n");
            err = startContext();
            if (EECode_OK != err) {
                LOG_ERROR("startContext() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                destroyContext();
                mbSetupContext = 0;
                break;
            }
            mbStartContext = 1;
        }

        err = processMsgLoop();

        if (mbStartContext) {
            stopContext();
            mbStartContext = 0;
        }

        if (mbSetupContext) {
            destroyContext();
            mbSetupContext = 0;
        }

        storeCurrentConfig("wp_cur.ini");

    } while (!mbExit);

    LOG_NOTICE("CISimplePlayer::MainLoop() end\n");
    if (mpExternalMsgQueue) {
        SMSG msg;
        msg.code = EMSGType_PlaybackEndNotify;
        msg.p2 = (TULong)mpOwner;
        LOG_NOTICE("post playback end notify\n");
        mpExternalMsgQueue->PostMsg(&msg, sizeof(msg));
    }

    return;
}

EECode CISimplePlayer::setupContext()
{
    EECode err = EECode_OK;
    SSharedComponent shared_component;

    mpClockSource = gfCreateClockSource(EClockSourceType_generic);
    mpClockManager = CIClockManager::Create();
    mpClockReference = NULL;

    if (DUnlikely((!mpClockSource) || (!mpClockManager))) {
        LOG_ERROR("create clock source or clock manager fail\n");
        return EECode_NoMemory;
    }

    err = mpClockManager->SetClockSource(mpClockSource);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("mpClockManager->SetClockSource() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = mpClockManager->Start();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("mpClockManager->Start() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    mpClockReference = CIClockReference::Create();
    if (DUnlikely(!mpClockReference)) {
        LOG_ERROR("create clock reference fail\n");
        return EECode_NoMemory;
    }

    err = mpClockManager->RegisterClockReference(mpClockReference);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("mpClockManager->RegisterClockReference fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    shared_component.p_clock_manager = mpClockManager;
    shared_component.p_clock_source = mpClockSource;
    shared_component.p_msg_queue = mpMsgQueue;

    mpDirector = gfCreateSceneDirector(ESceneDirectorType_basic, EVisualSettingPrefer_native, &mMediaAPIContxt.common_config, &shared_component);
    mpScene = CIScene::Create();
    mpLayer = CIGUILayer::Create();
    mpVisualDirectRendering = NULL;
    mpTexture = gfCreateGUIArea(EGUIAreaType_Texture2D, mpVisualDirectRendering);

    mpSoundComposer = gfCreateSoundComposer(ESoundComposerType_basic, &mMediaAPIContxt.common_config, &shared_component);
    mpSoundTrack = CISoundTrack::Create(4096 * 32);
    mpSoundDirectRendering = (ISoundDirectRendering *)mpSoundTrack;

    if (DUnlikely(!mpDirector)) {
        LOG_ERROR("gfCreateSceneDirector fail\n");
        return EECode_NoMemory;
    }

    if (DUnlikely((!mpScene) || (!mpLayer) || (!mpTexture))) {
        LOG_ERROR("no memory?\n");
        return EECode_NoMemory;
    }

    if (DUnlikely(!mpSoundComposer)) {
        LOG_ERROR("gfCreateSoundComposer fail\n");
        return EECode_NoMemory;
    }

    if (DUnlikely(!mpSoundTrack)) {
        LOG_ERROR("CISoundTrack::Create(4096 * 8) fail, no memory?\n");
        return EECode_NoMemory;
    }

    err = mpLayer->AddArea(mpTexture);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("mpLayer->AddArea() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = mpScene->AddLayer((CIGUILayer *) mpLayer);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("mpScene->AddLayer() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = mpDirector->AddScene((CIScene *) mpScene);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("mpDirector->AddScene() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    if (mPbMainWindowWidth && mPbMainWindowWidth) {
        err = mpDirector->CreateMainWindow(mWindowTitleName, 0, 0, mPbMainWindowWidth, mPbMainWindowHeight, EWindowState_Nomal);
    } else {
        err = mpDirector->CreateMainWindow(mWindowTitleName, 0, 0, 1280, 720, EWindowState_Nomal);
    }
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("mpDirector->CreateMainWindow() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }
    LOG_NOTICE("CreateMainWindow done!\n");

    TULong handle = mpDirector->GetWindowHandle();
    mpSoundComposer->SetHandle(handle);

    err = mpSoundComposer->RegisterTrack(mpSoundTrack);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("mpSoundComposer->RegisterTrack() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    mpMediaAPI = CMediaSimpleAPI::Create(1, &shared_component);
    if (DUnlikely(!mpMediaAPI)) {
        LOG_ERROR("CMediaSimpleAPI::Create() fail\n");
        return EECode_NoMemory;
    }

    mpMediaEngineAPI = mpMediaAPI->QueryEngineControl();

    mMediaAPIContxt.p_pb_textures[0] = (void *)(mpVisualDirectRendering);
    mMediaAPIContxt.p_pb_sound_tracks[0] = (void *)(mpSoundDirectRendering);

    err = mpMediaAPI->AssignContext(&mMediaAPIContxt);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("mpMediaAPI->AssignContext() fail, err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    return EECode_OK;
}

EECode CISimplePlayer::startContext()
{
    EECode err = mpDirector->StartRunning();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("mpDirector->StartRunning() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = mpSoundComposer->StartRunning();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("mpSoundComposer->StartRunning() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = mpMediaAPI->Run();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("mpMediaAPI->Run() fail, err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = mpMediaAPI->Start();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("mpMediaAPI->Start() fail, err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    return EECode_OK;
}

EECode CISimplePlayer::stopContext()
{
    EECode err;

    mpClockManager->UnRegisterClockReference(mpClockReference);
    mpClockReference->ClearAllTimers();

    if (mpClockManager) {
        mpClockManager->Stop();
    }

    LOG_NOTICE("start exit...\n");

    err = mpMediaAPI->Stop();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("mpMediaAPI->Stop() fail, err %d, %s\n", err, gfGetErrorCodeString(err));
    }
    LOG_NOTICE("after mpMediaAPI->Stop()\n");

    err = mpMediaAPI->Exit();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("mpMediaAPI->Exit() fail, err %d, %s\n", err, gfGetErrorCodeString(err));
    }
    LOG_NOTICE("after mpMediaAPI->Exit()\n");

    err = mpDirector->ExitRunning();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("mpDirector->ExitRunning() fail, err %d, %s\n", err, gfGetErrorCodeString(err));
    }
    LOG_NOTICE("after mpDirector->ExitRunning()\n");

    err = mpSoundComposer->ExitRunning();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("mpSoundComposer->ExitRunning() fail, err %d, %s\n", err, gfGetErrorCodeString(err));
    }
    LOG_NOTICE("after mpSoundComposer->ExitRunning()\n");

    return EECode_OK;
}

void CISimplePlayer::destroyContext()
{
    if (mpMediaAPI) {
        mpMediaAPI->Destroy();
        mpMediaAPI = NULL;
    }

    if (mpDirector) {
        mpDirector->Destroy();
        mpDirector = NULL;
    }

    if (mpSoundComposer) {
        mpSoundComposer->Destroy();
        mpSoundComposer = NULL;
    }

    if (mpScene) {
        mpScene->Destroy();
        mpScene = NULL;
    }

    if (mpLayer) {
        mpLayer->Destroy();
        mpLayer = NULL;
    }

    if (mpTexture) {
        mpTexture->Destroy();
        mpTexture = NULL;
    }

    if (mpClockReference) {
        mpClockReference->Delete();
        mpClockReference = NULL;
    }

    if (mpClockManager) {
        mpClockManager->Delete();
        mpClockManager = NULL;
    }

    if (mpClockSource) {
        mpClockSource->GetObject0()->Delete();
        mpClockSource = NULL;
    }

    if (mpSoundTrack) {
        mpSoundTrack->Destroy();
        mpSoundTrack = NULL;
    }

}

EECode CISimplePlayer::processMsgLoop()
{
    SMSG msg;
    TU32 running = 1;

    while (running) {

        mpMsgQueue->GetMsg(&msg, sizeof(msg));

        switch (msg.code) {

            case ECMDType_ExitProcessing:
                LOG_NOTICE("[MSG]: ECMDType_ExitProcessing\n");
                running = 0;
                mbExit = 1;
                break;

            case EMSGType_WindowClose:
                LOG_NOTICE("[MSG]: EMSGType_WindowClose\n");
                running = 0;
                mbExit = 1;
                break;

            case EMSGType_UnknownError:
            case EMSGType_NetworkError:
                LOG_NOTICE("[MSG]: EMSGType_NetworkError, mbReconnecting %d\n", mbReconnecting);
                break;

            case EMSGType_MissionComplete:
                LOG_NOTICE("[MSG]: EMSGType_MissionComplete, mbReconnecting %d\n", mbReconnecting);
                if (!mbReconnecting) {
                    LOG_NOTICE("[MSG]: server quit, play complete\n");
                    running = 0;
                }
                break;

            case EMSGType_ClientReconnectDone:
                LOG_NOTICE("[MSG]: EMSGType_ClientReconnectDone\n");
                mbReconnecting = 0;
                break;

            case EMSGType_RePlay: {
                    SGenericReconnectStreamingServer reconnect;
                    LOG_NOTICE("[MSG]: EMSGType_RePlay\n");
                    memset(&reconnect, 0x0, sizeof(SGenericReconnectStreamingServer));
                    reconnect.check_field = EGenericEngineConfigure_ReConnectServer;
                    reconnect.index = 0;
                    reconnect.all_demuxer = 1;
                    mbReconnecting = 1;
                    mpMediaEngineAPI->GenericControl(EGenericEngineConfigure_ReConnectServer, (void *)&reconnect);
                }
                break;

            case ECMDType_PlayUrl: {
                    SGenericReconnectStreamingServer reconnect;
                    LOG_NOTICE("[MSG]: ECMDType_PlayUrl\n");
                    memset(&reconnect, 0x0, sizeof(SGenericReconnectStreamingServer));
                    reconnect.check_field = EGenericEngineConfigure_ReConnectServer;
                    reconnect.index = 0;
                    reconnect.all_demuxer = 1;
                    mbReconnecting = 1;
                    mpMediaEngineAPI->GenericControl(EGenericEngineConfigure_ReConnectServer, (void *)&reconnect);
                }
                break;

            case ECMDType_PlayNewUrl:
                LOG_NOTICE("[MSG]: ECMDType_PlayNewUrl\n");
                running = 0;
                break;

            case EMSGType_DebugDump_Flow:
                mpMediaAPI->PrintRuntimeStatus();
                break;

            case EMSGType_WindowActive:
                LOG_NOTICE("[MSG]: EMSGType_WindowActive: %d\n", msg.flag);
                break;

            case EMSGType_WindowSize:
                LOG_NOTICE("[MSG]: EMSGType_WindowSize: %d, %d\n", (TDimension)msg.p1, (TDimension)msg.p2);
                mpDirector->ReSizeMainWindow((TDimension)msg.p1, (TDimension)msg.p2);
                break;

            case EMSGType_WindowMove:
                LOG_NOTICE("[MSG]: EMSGType_WindowSize: %d, %d\n", (TDimension)msg.p1, (TDimension)msg.p2);
                mpDirector->MoveMainWindow((TDimension)msg.p1, (TDimension)msg.p2);
                break;

            case EMSGType_OpenSourceFail:
                LOG_ERROR("EMSGType_OpenSourceFail\n");
                break;

            case EMSGType_OpenSourceDone:
                LOG_NOTICE("EMSGType_OpenSourceDone\n");
                break;

            default:
                LOG_ERROR("Not processed msg %d\n", msg.code);
                break;
        }

    }

    return EECode_OK;
}

EECode CISimplePlayer::waitPlay()
{
    SMSG msg;
    TU32 wait = 1;

    while (wait) {

        mpMsgQueue->GetMsg(&msg, sizeof(msg));

        switch (msg.code) {

            case ECMDType_PlayNewUrl:
                LOG_NOTICE("[CMD]: ECMDType_PlayNewUrl\n");
                wait = 0;
                break;

            case ECMDType_PlayUrl:
                LOG_NOTICE("[CMD]: ECMDType_PlayUrl\n");
                wait = 0;
                break;

            case ECMDType_ExitProcessing:
                LOG_NOTICE("[CMD]: ECMDType_ExitProcessing\n");
                wait = 0;
                return EECode_ExitIndicator;
                break;

            default:
                LOG_NOTICE("Not processed cmd 0x%08x\n", msg.code);
                break;
        }

    }

    return EECode_OK;
}

int CISimplePlayer::loadCurrentConfig(const char *configfile)
{
    IRunTimeConfigAPI *api = gfRunTimeConfigAPIFactory(ERunTimeConfigType_SimpleINI, 60);
    if (DUnlikely(!api)) {
        LOG_ERROR("gfRunTimeConfigAPIFactory fail\n");
        return (-2);
    }

    EECode err = api->OpenConfigFile((const TChar *)configfile, 0, 1, 1);
    if (DUnlikely(EECode_OK != err)) {
        api->Destroy();
        LOG_WARN("loadCurrentConfig: OpenConfigFile(%s) fail\n", configfile);
        return (-3);
    }

    TChar read_string[256] = {0};
    TInt val = 0;

    err = api->GetContentValue("pb_rtsp_tcp_mode", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        mMediaAPIContxt.setting.rtsp_client_try_tcp_mode_first = val;
        LOG_PRINTF("pb_rtsp_tcp_mode:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("play_url", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        if (read_string) {
            snprintf(mMediaAPIContxt.setting.pb_source_url[0], DMaxUrlLen, "%s", read_string);
        }
        LOG_PRINTF("play_url: %s\n", mMediaAPIContxt.setting.pb_source_url[0]);
    }

    err = api->GetContentValue("sound_output_buffer_count", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        mMediaAPIContxt.common_config.sound_output.sound_output_buffer_count = val;
        LOG_PRINTF("sound_output_buffer_count:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("sound_output_precache_count", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        mMediaAPIContxt.common_config.sound_output.sound_output_precache_count = val;
        LOG_PRINTF("sound_output_precache_count:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("pb_precached_video_frames", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        mMediaAPIContxt.setting.pb_precached_video_frames = val;
        LOG_PRINTF("pb_precached_video_frames:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("pb_max_video_frames", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        mMediaAPIContxt.setting.pb_max_video_frames = val;
        LOG_PRINTF("pb_max_video_frames:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("dump_debug_sound_output_pcm", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        mMediaAPIContxt.common_config.debug_dump.dump_debug_sound_output_pcm = val;
        LOG_PRINTF("dump_debug_sound_output_pcm:%s, %d\n", read_string, val);
    }

    err = api->CloseConfigFile();
    DASSERT_OK(err);

    api->Destroy();

    return 0;
}

int CISimplePlayer::storeCurrentConfig(const char *configfile)
{
    FILE *file = fopen(configfile, "wt+");
    if (!file) {
        LOG_ERROR("open [%s] fail in storeCurrentConfig()\n", configfile);
        return (-1);
    }

    fprintf(file, "pb_rtsp_tcp_mode=%d\n", mMediaAPIContxt.setting.rtsp_client_try_tcp_mode_first);

    //if (mMediaAPIContxt.setting.pb_source_url[0][0]) {
    fprintf(file, "play_url=%s\n", mMediaAPIContxt.setting.pb_source_url[0]);
    //}

    fprintf(file, "sound_output_buffer_count=%d\n", mMediaAPIContxt.common_config.sound_output.sound_output_buffer_count);
    fprintf(file, "sound_output_precache_count=%d\n", mMediaAPIContxt.common_config.sound_output.sound_output_precache_count);
    fprintf(file, "pb_precached_video_frames=%d\n", mMediaAPIContxt.setting.pb_precached_video_frames);
    fprintf(file, "pb_max_video_frames=%d\n", mMediaAPIContxt.setting.pb_max_video_frames);
    fprintf(file, "dump_debug_sound_input_pcm=%d\n", mMediaAPIContxt.common_config.debug_dump.dump_debug_sound_input_pcm);

    fclose(file);
    file = NULL;

    return 0;
}

int CISimplePlayer::checkWithSystemSettings()
{
    SSystemSettings settings;
    EECode err = gfGetSystemSettings(&settings);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("gfGetSystemSettings fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return (-1);
    } else {
        TDimension system_default_width = settings.screens[0].screen_width;
        TDimension system_default_height = settings.screens[0].screen_height;

        if ((mMediaAPIContxt.video_enc_setting.width > system_default_width) \
                || (mMediaAPIContxt.video_enc_setting.height > system_default_height) \
                || (!mMediaAPIContxt.video_enc_setting.width) \
                || (!mMediaAPIContxt.video_enc_setting.height)) {
            mMediaAPIContxt.video_enc_setting.width = system_default_width;
            mMediaAPIContxt.video_enc_setting.height = system_default_height;
            LOG_NOTICE("enc dimension not set or not proper, set to system default %d x %d\n", system_default_width, system_default_height);
        }

        if (mMediaAPIContxt.video_enc_setting.framerate_num && mMediaAPIContxt.video_enc_setting.framerate_den) {
            mMediaAPIContxt.video_cap_setting.framerate_num = mMediaAPIContxt.video_enc_setting.framerate_num;
            mMediaAPIContxt.video_cap_setting.framerate_den = mMediaAPIContxt.video_enc_setting.framerate_den;
        } else {
            mMediaAPIContxt.video_cap_setting.framerate_num = mMediaAPIContxt.video_enc_setting.framerate_num = DDefaultVideoFramerateNum;
            mMediaAPIContxt.video_cap_setting.framerate_den = mMediaAPIContxt.video_enc_setting.framerate_den = DDefaultVideoFramerateDen;
            LOG_NOTICE("not specify framerate, set to system default 30 fps\n");
        }

        if (((mMediaAPIContxt.video_cap_setting.width + mMediaAPIContxt.video_cap_setting.offset_x) > system_default_width) \
                || ((mMediaAPIContxt.video_cap_setting.height + mMediaAPIContxt.video_cap_setting.offset_y) > system_default_height) \
                || (!mMediaAPIContxt.video_cap_setting.width) \
                || (!mMediaAPIContxt.video_cap_setting.height)) {
            mMediaAPIContxt.video_cap_setting.width = system_default_width;
            mMediaAPIContxt.video_cap_setting.height = system_default_height;
            mMediaAPIContxt.video_cap_setting.offset_x = 0;
            mMediaAPIContxt.video_cap_setting.offset_y = 0;
            LOG_NOTICE("capture dimension not set or not proper, set to system default %d x %d\n", system_default_width, system_default_height);
        }

        if ((mPbMainWindowWidth > system_default_width) \
                || (mPbMainWindowHeight > system_default_height) \
                || (mPbMainWindowWidth < 16) \
                || (mPbMainWindowHeight < 16)) {
            mPbMainWindowWidth = (TDimension)((((TULong)system_default_width * 3 / 4) + 15) & (~ 15));
            mPbMainWindowHeight = (TDimension)((((TULong)system_default_height * 3 / 4) + 7) & (~ 7));
            LOG_NOTICE("pb main window dimension not set or not proper, set to default (3/4 of screen resolution) %d x %d\n", mPbMainWindowWidth, mPbMainWindowHeight);
        }
    }

    return 0;
}


DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

