/**
 * simple_capserver.cpp
 *
 * History:
 *    2015/01/05 - [Zhi He] create file
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

#include "media_simple_api.h"

#include "simple_capserver_if.h"

#include "simple_capserver.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

ISimpleCapServer *gfCreateSimpleCapServer()
{
    CISimpleCapServer *thiz = CISimpleCapServer::Create();
    return (ISimpleCapServer *) thiz;
}

CISimpleCapServer::CISimpleCapServer()
    : mpMediaSimpleAPI(NULL)
    , mCurrentSoundInputDeviceIndex(0)
    , mbSetupMediaContext(0)
    , mbRunning(0)
    , mbStarted(0)
{
    memset(&mMediaAPIContxt, 0x0, sizeof(mMediaAPIContxt));
    memset(&mSystemSoundInputDevices, 0x0, sizeof(mSystemSoundInputDevices));
}

CISimpleCapServer::~CISimpleCapServer()
{
}

CISimpleCapServer *CISimpleCapServer::Create()
{
    CISimpleCapServer *thiz = new CISimpleCapServer();
    return thiz;
}

void CISimpleCapServer::Destroy()
{
    storeCurrentConfig("wcs_cur.ini");

    destroyMediaAPI();
}

int CISimpleCapServer::Initialize()
{
    if (!isLogFileOpened()) {
        gfOpenLogFile("wcslog.txt");
    }

    EECode err = gfInitPlatform();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("gfInitPlatform() fail\n");
        return (-1);
    }

    gfMediaSimpleAPILoadDefaultCapServerConfig(&mMediaAPIContxt);
    gfMediaSimpleAPILoadMediaConfig(&mMediaAPIContxt, "wcs.ini");

    loadCurrentConfig("wcs_cur.ini");
    err = checkWithSystemSettings();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("checkWithSystemSettings() fail\n");
        return (-2);
    }

    gfMediaSimpleAPIPrintMediaConfig(&mMediaAPIContxt);

    err = loadAudioDevices();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("loadAudioDevices() fail\n");
        return (-3);
    }

    err = setupMediaAPI();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("setupMediaAPI() fail\n");
        return (-4);
    }

    err = mpMediaSimpleAPI->Run();
    mbRunning = 1;
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("Run() fail\n");
        return (-5);
    }

    return 0;
}

int CISimpleCapServer::Start()
{
    if (DUnlikely(!mpMediaSimpleAPI)) {
        LOG_ERROR("NULL mpMediaSimpleAPI\n");
        return (-10);
    }

    if (DUnlikely(mbStarted)) {
        LOG_ERROR("already started\n");
        return (-11);
    }

    EECode err = mpMediaSimpleAPI->Start();
    mbStarted = 1;
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("mpMediaSimpleAPI->Start() fail, err %d, %s\n", err, gfGetErrorCodeString(err));
        return (-12);
    }

    storeCurrentConfig("wcs_cur.ini");

    return 0;
}

int CISimpleCapServer::Stop()
{
    if (DUnlikely(!mpMediaSimpleAPI)) {
        LOG_ERROR("NULL mpMediaSimpleAPI\n");
        return (-20);
    }

    if (DUnlikely(!mbStarted)) {
        LOG_ERROR("not started\n");
        return (-21);
    }

    EECode err = mpMediaSimpleAPI->Stop();
    mbStarted = 0;
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("mpMediaSimpleAPI->Stop() fail, err %d, %s\n", err, gfGetErrorCodeString(err));
        return (-22);
    }

    return 0;
}

void CISimpleCapServer::Mute()
{
    if (DUnlikely(!mpMediaSimpleAPI)) {
        LOG_ERROR("NULL mpMediaSimpleAPI\n");
        return;
    }

    if (DUnlikely(!mbStarted)) {
        LOG_ERROR("not started\n");
        return;
    }

    IGenericEngineControlV4 *p_engine = mpMediaSimpleAPI->QueryEngineControl();
    SUserParamType_CaptureAudioMuteControl param;
    param.check_field = EGenericEngineConfigure_AudioCapture_Mute;
    param.id = mMediaAPIContxt.audio_capturer_id[0];
    p_engine->GenericControl(EGenericEngineConfigure_AudioCapture_Mute, &param);

    return;
}

void CISimpleCapServer::UnMute()
{
    if (DUnlikely(!mpMediaSimpleAPI)) {
        LOG_ERROR("NULL mpMediaSimpleAPI\n");
        return;
    }

    if (DUnlikely(!mbStarted)) {
        LOG_ERROR("not started\n");
        return;
    }

    IGenericEngineControlV4 *p_engine = mpMediaSimpleAPI->QueryEngineControl();
    SUserParamType_CaptureAudioMuteControl param;
    param.check_field = EGenericEngineConfigure_AudioCapture_UnMute;
    param.id = mMediaAPIContxt.audio_capturer_id[0];
    p_engine->GenericControl(EGenericEngineConfigure_AudioCapture_UnMute, &param);

    return;
}

void CISimpleCapServer::loadCurrentConfig(const TChar *configfile)
{
    IRunTimeConfigAPI *api = gfRunTimeConfigAPIFactory(ERunTimeConfigType_SimpleINI, 60);
    if (DUnlikely(!api)) {
        LOG_ERROR("gfRunTimeConfigAPIFactory() fail\n");
        return;
    }

    EECode err = api->OpenConfigFile((const TChar *)configfile, 0, 1, 1);
    if (DUnlikely(EECode_OK != err)) {
        LOG_NOTICE("OpenConfigFile(%s) fail, file not exist\n", configfile);
        api->Destroy();
        return;
    }

    TChar read_string[256] = {0};
    TChar read_string1[256] = {0};
    TU32 val = 0;

    err = api->GetContentValue("local_rtsp_port", (TChar *)read_string);
    if (EECode_OK == err) {
        sscanf(read_string, "%d", &val);
        mMediaAPIContxt.setting.local_rtsp_port = val;
        LOG_NOTICE("[user specify]: local_rtsp_port:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("url_cap_streaming", (TChar *)read_string);
    if (EECode_OK == err) {
        if (read_string[0]) {
            snprintf(mMediaAPIContxt.setting.cap_local_streaming_url[0], DMAX_URL_LENGTH, "%s", read_string);
        }
        LOG_NOTICE("[user specify]: url_cap_streaming: %s\n", mMediaAPIContxt.setting.cap_local_streaming_url[0]);
    }

    err = api->GetContentValue("url_cap_storage", (TChar *)read_string);
    if (EECode_OK == err) {
        if (read_string[0]) {
            snprintf(mMediaAPIContxt.setting.cap_sink_url[0], DMAX_URL_LENGTH, "%s", read_string);
        }
        LOG_NOTICE("[user specify]: url_cap_storage: %s\n", mMediaAPIContxt.setting.cap_sink_url[0]);
    }

    err = api->GetContentValue("audio_enc_bitrate", (TChar *)read_string);
    if (EECode_OK == err) {
        sscanf(read_string, "%d", &val);
        mMediaAPIContxt.audio_enc_setting.bitrate = val;
        LOG_PRINTF("[user specify]: audio_enc_bitrate:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("video_enc_fr_num", (TChar *)read_string);
    if (EECode_OK == err) {
        sscanf(read_string, "%d", &val);
        mMediaAPIContxt.video_enc_setting.framerate_num = val;
        LOG_NOTICE("[user specify]: video_enc_fr_num:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("video_enc_fr_den", (TChar *)read_string);
    if (EECode_OK == err) {
        sscanf(read_string, "%d", &val);
        mMediaAPIContxt.video_enc_setting.framerate_den = val;
        LOG_NOTICE("[user specify]: video_enc_fr_den:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("video_enc_bitrate", (TChar *)read_string);
    if (EECode_OK == err) {
        sscanf(read_string, "%d", &val);
        mMediaAPIContxt.video_enc_setting.bitrate = val;
        LOG_NOTICE("[user specify]: video_enc_bitrate:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("dump_debug_sound_input_pcm", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        mMediaAPIContxt.common_config.debug_dump.dump_debug_sound_input_pcm = val;
        LOG_PRINTF("dump_debug_sound_input_pcm:%s, %d\n", read_string, val);
    }

    api->CloseConfigFile();
    api->Destroy();

}

void CISimpleCapServer::storeCurrentConfig(const TChar *configfile)
{
    FILE *file = fopen(configfile, "wt+");
    if (!file) {
        LOG_ERROR("open [%s] fail in storeCurrentConfig()\n", configfile);
        return;
    }

    if (mMediaAPIContxt.setting.local_rtsp_port) {
        fprintf(file, "local_rtsp_port=%d\n", mMediaAPIContxt.setting.local_rtsp_port);
    }

    if (mMediaAPIContxt.setting.cap_local_streaming_url[0][0]) {
        fprintf(file, "url_cap_streaming=%s\n", mMediaAPIContxt.setting.cap_local_streaming_url[0]);
    }

    if (mMediaAPIContxt.setting.cap_sink_url[0][0]) {
        fprintf(file, "url_cap_storage=%s\n", mMediaAPIContxt.setting.cap_sink_url[0]);
    }

    if (mMediaAPIContxt.audio_enc_setting.bitrate) {
        fprintf(file, "audio_enc_bitrate=%d\n", mMediaAPIContxt.audio_enc_setting.bitrate);
    }

    if (mMediaAPIContxt.video_enc_setting.framerate_num) {
        fprintf(file, "video_enc_fr_num=%d\n", mMediaAPIContxt.video_enc_setting.framerate_num);
    }

    if (mMediaAPIContxt.video_enc_setting.framerate_den) {
        fprintf(file, "video_enc_fr_den=%d\n", mMediaAPIContxt.video_enc_setting.framerate_den);
    }

    if (mMediaAPIContxt.video_enc_setting.bitrate) {
        fprintf(file, "video_enc_bitrate=%d\n", mMediaAPIContxt.video_enc_setting.bitrate);
    }

    fprintf(file, "dump_debug_sound_input_pcm=%d\n", mMediaAPIContxt.common_config.debug_dump.dump_debug_sound_input_pcm);

    fclose(file);
    file = NULL;

}

EECode CISimpleCapServer::checkWithSystemSettings()
{
    SSystemSettings settings;
    EECode err = gfGetSystemSettings(&settings);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("gfGetSystemSettings fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    } else {
        TDimension system_default_width = settings.screens[0].screen_width;
        TDimension system_default_height = settings.screens[0].screen_height;

        if ((mMediaAPIContxt.video_enc_setting.width > system_default_width) \
                || (mMediaAPIContxt.video_enc_setting.height > system_default_height) \
                || (!mMediaAPIContxt.video_enc_setting.width) \
                || (!mMediaAPIContxt.video_enc_setting.height)) {
            mMediaAPIContxt.video_enc_setting.width = system_default_width;
            mMediaAPIContxt.video_enc_setting.height = system_default_height;
            LOG_NOTICE("not specify encode dimension, set to system default %d x %d\n", system_default_width, system_default_height);
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
            LOG_NOTICE("not specify capture dimension, set to system default %d x %d\n", system_default_width, system_default_height);
        }
    }

    return EECode_OK;
}

EECode CISimpleCapServer::loadAudioDevices()
{
    memset(&mSystemSoundInputDevices, 0x0, sizeof(mSystemSoundInputDevices));

    EECode err = gfGetSystemSoundInputDevices(&mSystemSoundInputDevices);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("gfGetSystemSoundInputDevices fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    mCurrentSoundInputDeviceIndex = mSystemSoundInputDevices.sound_input_devices_number;

    mMediaAPIContxt.p_sound_input_devices[0] = mSystemSoundInputDevices.devices[0].p_context;

    if (mCurrentSoundInputDeviceIndex < mSystemSoundInputDevices.sound_input_devices_number) {
        mMediaAPIContxt.p_sound_input_devices[0] = mSystemSoundInputDevices.devices[mCurrentSoundInputDeviceIndex].p_context;
        snprintf(&mMediaAPIContxt.setting.prefer_string_audio_capture[0], DMaxPreferStringLen, "%s", "DirectSound");
    } else {
        snprintf(&mMediaAPIContxt.setting.prefer_string_audio_capture[0], DMaxPreferStringLen, "%s", "MMD");
    }

    return EECode_OK;
}

EECode CISimpleCapServer::setupMediaAPI()
{
    if (DUnlikely(mbSetupMediaContext)) {
        LOG_ERROR("already setup\n");
        return EECode_BadState;
    }

    mpMediaSimpleAPI = CMediaSimpleAPI::Create(1, NULL);
    if (DUnlikely(!mpMediaSimpleAPI)) {
        LOG_ERROR("CMediaSimpleAPI::Create(1) fail\n");
        return EECode_NoMemory;
    }
    mbSetupMediaContext = 1;

    EECode err = mpMediaSimpleAPI->AssignContext(&mMediaAPIContxt);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("mpMediaSimpleAPI->AssignContext() fail, err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    return EECode_OK;
}

void CISimpleCapServer::destroyMediaAPI()
{
    if (mpMediaSimpleAPI) {
        if (mbStarted) {
            mpMediaSimpleAPI->Stop();
            mbStarted = 0;
        }
        if (mbRunning) {
            mpMediaSimpleAPI->Exit();
            mbRunning = 0;
        }

        mpMediaSimpleAPI->Destroy();
        mpMediaSimpleAPI = NULL;
        mbSetupMediaContext = 0;
    }
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

