/*******************************************************************************
 * media_simple_api.cpp
 *
 * History:
 *    2014/10/13 - [Zhi He] create file
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

#include "media_mw_if.h"

#include "media_simple_api.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

CMediaSimpleAPI::CMediaSimpleAPI(TU32 direct_access)
    : mpEngineControl(NULL)
    , mpContext(NULL)
    , mpMediaConfig(NULL)
    , mbBuildGraghDone(0)
    , mbAssignContext(0)
    , mbDirectAccess(direct_access)
    , mbStarted(0)
    , mbRunning(0)
{
}

CMediaSimpleAPI::~CMediaSimpleAPI()
{
}

EECode CMediaSimpleAPI::Construct(SSharedComponent *p_shared_component)
{
    mpEngineControl = gfMediaEngineFactoryV4(EFactory_MediaEngineType_Generic, NULL, p_shared_component);
    if (DUnlikely(!mpEngineControl)) {
        LOG_FATAL("gfMediaEngineFactoryV3 fail\n");
        return EECode_NoMemory;
    }

    EECode err = mpEngineControl->SetAppMsgCallback(MsgCallback, (void *) this);
    if (DUnlikely(EECode_OK != err)) {
        LOG_FATAL("SetAppMsgCallback fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = mpEngineControl->GetMediaConfig(mpMediaConfig);
    DASSERT_OK(err);

    return EECode_OK;
}

CMediaSimpleAPI *CMediaSimpleAPI::Create(TU32 direct_access, SSharedComponent *p_shared_component)
{
    CMediaSimpleAPI *thiz = new CMediaSimpleAPI(direct_access);
    if (DLikely(thiz)) {
        EECode err = thiz->Construct(p_shared_component);
        if (DLikely(EECode_OK == err)) {
            return thiz;
        }
        delete thiz;
    }

    return NULL;
}

void CMediaSimpleAPI::Destroy()
{
    if (mpEngineControl) {
        if (DUnlikely(mbStarted)) {
            mpEngineControl->Stop();
            mbStarted = 0;
        }

        if (DUnlikely(mbRunning)) {
            mpEngineControl->ExitProcessing();
            mbRunning = 0;
        }

        mpEngineControl->Destroy();
        mpEngineControl = NULL;
    }

    delete this;
}

IGenericEngineControlV4 *CMediaSimpleAPI::QueryEngineControl() const
{
    return mpEngineControl;
}

EECode CMediaSimpleAPI::AssignContext(SMediaSimpleAPIContext *p_context)
{
    if (DUnlikely(!p_context)) {
        LOG_FATAL("NULL p_context\n");
        return EECode_BadParam;
    }

    if (DUnlikely(mbBuildGraghDone)) {
        LOG_FATAL("build gragh done, cannot invoke this API at this time\n");
        return EECode_BadState;
    }

    if (DUnlikely(mpContext || mbAssignContext)) {
        LOG_FATAL("mpContext %p, mbAssignContext %d, already set?\n", mpContext, mbAssignContext);
        return EECode_BadState;
    }

    if (mbDirectAccess) {
        mpContext = p_context;
    } else {
        mpContext = (SMediaSimpleAPIContext *) DDBG_MALLOC(sizeof(SMediaSimpleAPIContext), "MSCXs");
        if (!mpContext) {
            LOG_FATAL("No memory, request size %lu\n", (TULong)sizeof(SMediaSimpleAPIContext));
            return EECode_NoMemory;
        }
        memset(mpContext, 0x0, sizeof(SMediaSimpleAPIContext));
        *mpContext = *p_context;
    }
    mbAssignContext = 1;

    return EECode_OK;
}

EECode CMediaSimpleAPI::Run()
{
    EECode err = EECode_OK;

    if (DUnlikely((!mpEngineControl) || (!mpContext) || (!mbAssignContext))) {
        LOG_FATAL("NULL mpEngineControl %p or NULL mpContext %p, or zero mbAssignContext %d\n", mpEngineControl, mpContext, mbAssignContext);
        return EECode_BadState;
    }

    if (DLikely(!mbBuildGraghDone)) {
        err = checkSetting();
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("checkSetting return %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }

        err = initialSetting();
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("initialSetting return %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }

        mpEngineControl->SetMediaConfig();
        err = buildDataProcessingGragh();
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("buildDataProcessingGragh return %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }

        if (mpContext->setting.pb_use_external_source) {
            getHandlerForExternalSource();
        }

        if (mpContext->setting.b_pb_video_source_use_external_data_processing) {
            setPBVideoSourceExternalDataProcessingCallback();
        }

        mbBuildGraghDone = 1;
    } else {
        DASSERT(mbAssignContext);
        DASSERT(mpContext);
    }

    err = mpEngineControl->RunProcessing();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("StartProcess: RunProcessing return %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }
    mbRunning = 1;

    return EECode_OK;
}

EECode CMediaSimpleAPI::Exit()
{
    if (DLikely(mpEngineControl)) {
        EECode err = EECode_OK;
        if (DUnlikely(mbStarted)) {
            err = mpEngineControl->Stop();
            mbStarted = 0;
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("Stop() return %d, %s\n", err, gfGetErrorCodeString(err));
            }
        }

        if (DLikely(mbRunning)) {
            err = mpEngineControl->ExitProcessing();
            mbRunning = 0;
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ExitProcessing() return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        } else {
            LOG_WARN("already exit?\n");
            return EECode_BadState;
        }
    } else {
        LOG_ERROR("NULL mpEngineControl\n");
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CMediaSimpleAPI::Start()
{
    EECode err = EECode_OK;

    if (DUnlikely((!mpEngineControl) || (!mpContext))) {
        LOG_FATAL("NULL params\n");
        return EECode_BadState;
    }

    if (DUnlikely(!mbBuildGraghDone)) {
        LOG_ERROR("should invoke Run first!\n");
        return EECode_BadState;
    }

    if (DUnlikely(mbStarted)) {
        LOG_ERROR("already started\n");
        return EECode_BadState;
    }

    err = mpEngineControl->Start();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("Start() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    mbStarted = 1;

    err = setUrls();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("setUrls() fail, return %d, %s\n", err, gfGetErrorCodeString(err));
    }

    return EECode_OK;
}

EECode CMediaSimpleAPI::Stop()
{
    if (DLikely(mpEngineControl)) {
        if (DLikely(mbStarted)) {
            EECode err = mpEngineControl->Stop();
            mbStarted = 0;
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("StopProcess: Stop return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        } else {
            LOG_ERROR("not started\n");
            return EECode_BadState;
        }
    } else {
        LOG_ERROR("NULL mpEngineControl\n");
        return EECode_BadState;
    }

    return EECode_OK;
}

void CMediaSimpleAPI::PrintRuntimeStatus()
{
    if (mpEngineControl) {
        mpEngineControl->PrintCurrentStatus(0, DPrintFlagBit_dataPipeline);
    }
}

void CMediaSimpleAPI::MsgCallback(void *context, SMSG &msg)
{
    if (DUnlikely(!context)) {
        LOG_FATAL("NULL context\n");
        return;
    }

    LOG_ERROR("CMediaSimpleAPI::MsgCallback here: TODO, msg.code 0x%x\n", msg.code);
    return;
}

EECode CMediaSimpleAPI::checkSetting()
{
    //to do
    return EECode_OK;
}

EECode CMediaSimpleAPI::initialSetting()
{
    EECode err = EECode_OK;
    SGenericPreConfigure pre_config;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    if (mpContext->setting.enable_force_log_level) {
        err = mpEngineControl->SetEngineLogLevel((ELogLevel)mpContext->setting.force_log_level);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("SetEngineLogLevel(%d), err %d, %s\n", mpContext->setting.force_log_level, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.net_receiver_scheduler_number) {
        pre_config.check_field = EGenericEnginePreConfigure_NetworkReceiverSchedulerNumber;
        pre_config.number = mpContext->setting.net_receiver_scheduler_number;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_NetworkReceiverSchedulerNumber, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_NetworkReceiverSchedulerNumber, %d), err %d, %s\n", mpContext->setting.net_receiver_scheduler_number, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.net_sender_scheduler_number) {
        pre_config.check_field = EGenericEnginePreConfigure_NetworkTransmiterSchedulerNumber;
        pre_config.number = mpContext->setting.net_sender_scheduler_number;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_NetworkTransmiterSchedulerNumber, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_NetworkTransmiterSchedulerNumber, %d), err %d, %s\n", mpContext->setting.net_sender_scheduler_number, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.fileio_reader_scheduler_number) {
        pre_config.check_field = EGenericEnginePreConfigure_IOReaderSchedulerNumber;
        pre_config.number = mpContext->setting.fileio_reader_scheduler_number;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_IOReaderSchedulerNumber, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_IOReaderSchedulerNumber, %d), err %d, %s\n", mpContext->setting.fileio_reader_scheduler_number, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.fileio_writer_scheduler_number) {
        pre_config.check_field = EGenericEnginePreConfigure_IOWriterSchedulerNumber;
        pre_config.number = mpContext->setting.fileio_writer_scheduler_number;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_IOWriterSchedulerNumber, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_IOWriterSchedulerNumber, %d), err %d, %s\n", mpContext->setting.fileio_writer_scheduler_number, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.video_decoder_scheduler_number) {
        pre_config.check_field = EGenericEnginePreConfigure_VideoDecoderSchedulerNumber;
        pre_config.number = mpContext->setting.video_decoder_scheduler_number;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_VideoDecoderSchedulerNumber, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_VideoDecoderSchedulerNumber, %d), err %d, %s\n", mpContext->setting.video_decoder_scheduler_number, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.rtsp_client_try_tcp_mode_first) {
        pre_config.check_field = EGenericEnginePreConfigure_RTSPClientTryTCPModeFirst;
        pre_config.number = mpContext->setting.rtsp_client_try_tcp_mode_first;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_RTSPClientTryTCPModeFirst, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_RTSPClientTryTCPModeFirst, %d), err %d, %s\n", mpContext->setting.net_receiver_scheduler_number, err, gfGetErrorCodeString(err));
            return err;
        }
    }

#if 0
    pre_config.check_field = EGenericEnginePreConfigure_RTSPClientParseMultipleAccessUnit;
    pre_config.number = mpContext->setting.parse_multiple_access_unit;
    err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_RTSPClientParseMultipleAccessUnit, &pre_config);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("EGenericEnginePreConfigure_RTSPClientParseMultipleAccessUnit(%d), err %d, %s\n", mpContext->setting.parse_multiple_access_unit, err, gfGetErrorCodeString(err));
        return err;
    }
#endif

    if (mpContext->setting.decoder_prefetch_count) {
        pre_config.check_field = EGenericEnginePreConfigure_PlaybackSetPreFetch;
        pre_config.number = mpContext->setting.decoder_prefetch_count;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_PlaybackSetPreFetch, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_PlaybackSetPreFetch, %d), err %d, %s\n", mpContext->setting.decoder_prefetch_count, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.local_rtsp_port) {
        pre_config.check_field = EGenericEnginePreConfigure_RTSPStreamingServerPort;
        pre_config.number = mpContext->setting.local_rtsp_port;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_RTSPStreamingServerPort, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_RTSPStreamingServerPort, %d), err %d, %s\n", mpContext->setting.local_rtsp_port, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.compensate_delay_from_jitter) {
        err = mpEngineControl->GenericControl(EGenericEngineConfigure_VideoPlayback_CompensateDelayFromJitter, NULL);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEngineConfigure_VideoPlayback_CompensateDelayFromJitter, %d), err %d, %s\n", mpContext->setting.compensate_delay_from_jitter, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    //direct settings
    if (DLikely(mpMediaConfig)) {
        if (mpContext->video_cap_setting.width && mpContext->video_cap_setting.height) {
            mpMediaConfig->video_capture_config.offset_x = mpContext->video_cap_setting.offset_x;
            mpMediaConfig->video_capture_config.offset_y = mpContext->video_cap_setting.offset_y;
            mpMediaConfig->video_capture_config.width = mpContext->video_cap_setting.width;
            mpMediaConfig->video_capture_config.height = mpContext->video_cap_setting.height;
        } else {
            mpMediaConfig->video_capture_config.offset_x = 0;
            mpMediaConfig->video_capture_config.offset_y = 0;
            mpMediaConfig->video_capture_config.width = mpContext->video_enc_setting.width;
            mpMediaConfig->video_capture_config.height = mpContext->video_enc_setting.height;
            LOG_NOTICE("[config]: not specify capture resolution, use encoding one: width %d, height %d\n", mpContext->video_enc_setting.width, mpContext->video_enc_setting.height);
        }

        if (mpContext->video_cap_setting.framerate_num && mpContext->video_cap_setting.framerate_den) {
            mpMediaConfig->video_capture_config.framerate_num = mpContext->video_cap_setting.framerate_num;
            mpMediaConfig->video_capture_config.framerate_den = mpContext->video_cap_setting.framerate_den;
        } else {
            mpMediaConfig->video_capture_config.framerate_num = mpContext->video_enc_setting.framerate_num;
            mpMediaConfig->video_capture_config.framerate_den = mpContext->video_enc_setting.framerate_den;
            LOG_NOTICE("[config]: not specify capture framerate, use encoding one: num %d, den %d\n", mpContext->video_enc_setting.framerate_num, mpContext->video_enc_setting.framerate_den);
        }

        mpMediaConfig->video_encoding_config.width = mpContext->video_enc_setting.width;
        mpMediaConfig->video_encoding_config.height = mpContext->video_enc_setting.height;
        mpMediaConfig->video_encoding_config.framerate_num = mpContext->video_enc_setting.framerate_num;
        mpMediaConfig->video_encoding_config.framerate_den = mpContext->video_enc_setting.framerate_den;
        mpMediaConfig->video_encoding_config.bitrate = mpContext->video_enc_setting.bitrate;

        mpMediaConfig->audio_encoding_config.bitrate = mpContext->audio_enc_setting.bitrate;

        mpMediaConfig->rtsp_streaming_video_enable = mpContext->setting.enable_video;
        mpMediaConfig->rtsp_streaming_audio_enable = mpContext->setting.enable_audio;

        mpMediaConfig->auto_cut.record_strategy = mpContext->setting.record_strategy;
        mpMediaConfig->auto_cut.record_autocut_condition = mpContext->setting.record_autocut_condition;
        mpMediaConfig->auto_cut.record_autocut_naming = mpContext->setting.record_autocut_naming;
        mpMediaConfig->auto_cut.record_autocut_framecount = mpContext->setting.record_autocut_framecount;
        mpMediaConfig->auto_cut.record_autocut_duration = mpContext->setting.record_autocut_duration;

        if (mpContext->pb_decode_setting.prealloc_buffer_number) {
            mpMediaConfig->pb_decode.prealloc_buffer_number = mpContext->pb_decode_setting.prealloc_buffer_number;
        }

        if (mpContext->pb_decode_setting.prefer_thread_number) {
            mpMediaConfig->pb_decode.prefer_thread_number = mpContext->pb_decode_setting.prefer_thread_number;
            mpMediaConfig->pb_decode.prefer_parallel_frame = mpContext->pb_decode_setting.prefer_parallel_frame;
            mpMediaConfig->pb_decode.prefer_parallel_slice = mpContext->pb_decode_setting.prefer_parallel_slice;
        }
        mpMediaConfig->pb_decode.prefer_official_reference_model_decoder = mpContext->setting.b_use_official_test_model_decoder;

        mpMediaConfig->pb_cache.b_constrain_latency = mpContext->setting.b_constrain_latency;
        mpMediaConfig->pb_cache.precache_video_frames = mpContext->setting.pb_precached_video_frames;
        mpMediaConfig->pb_cache.max_video_frames = mpContext->setting.pb_max_video_frames;
        mpMediaConfig->pb_cache.b_render_video_nodelay = mpContext->setting.b_render_video_nodelay;

        mpMediaConfig->try_requested_video_framerate = mpContext->setting.pb_try_requested_framerate;
        mpMediaConfig->requested_video_framerate_num  = mpContext->setting.requested_framerate_num;
        mpMediaConfig->requested_video_framerate_den = mpContext->setting.requested_framerate_den;

        memcpy((void *)&mpMediaConfig->common_config, &mpContext->common_config, sizeof(SPersistCommonConfig));
    }

    return EECode_OK;
}

EECode CMediaSimpleAPI::createGlobalComponents()
{
    EECode err = EECode_OK;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_PRINTF("[CMediaSimpleAPI]: createGlobalComponents begin\n");

    if (mpContext->setting.enable_capture_streaming_out) {
        err = mpEngineControl->NewComponent(EGenericComponentType_StreamingServer, mpContext->streaming_server_id);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("NewComponent(EGenericComponentType_StreamingServer) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
            return err;
        }
    }

    LOG_PRINTF("[CMediaSimpleAPI]: createGlobalComponents end\n");

    return EECode_OK;
}

EECode CMediaSimpleAPI::createPlaybackComponents()
{
    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    EECode err = EECode_OK;
    TU32 i = 0, max = mpContext->setting.total_num_playback_instance;

    LOG_PRINTF("[CMediaSimpleAPI]: createPlaybackComponents() begin, max %d\n", max);

    for (i = 0; i < max; i ++) {
        err = mpEngineControl->NewComponent(EGenericComponentType_Demuxer, mpContext->demuxer_id[i], mpContext->setting.prefer_string_demuxer);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("NewComponent(EGenericComponentType_Demuxer, %d) fail, ret %d, %s.\n", i, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.enable_video) {
        for (i = 0; i < max; i ++) {
            err = mpEngineControl->NewComponent(EGenericComponentType_VideoDecoder, mpContext->video_decoder_id[i], mpContext->setting.prefer_string_video_decoder);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_VideoDecoder, %d) fail, ret %d, %s.\n", i, err, gfGetErrorCodeString(err));
                return err;
            }

            err = mpEngineControl->NewComponent(EGenericComponentType_VideoOutput, mpContext->video_output_id[i], mpContext->setting.prefer_string_video_output, mpContext->p_pb_textures[i]);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_VideoOutput, %d) fail, ret %d, %s.\n", i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    if (mpContext->setting.enable_audio) {
        for (i = 0; i < max; i ++) {
            err = mpEngineControl->NewComponent(EGenericComponentType_AudioDecoder, mpContext->audio_decoder_id[i], mpContext->setting.prefer_string_audio_decoder);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_AudioDecoder, %d) fail, ret %d, %s.\n", i, err, gfGetErrorCodeString(err));
                return err;
            }

            err = mpEngineControl->NewComponent(EGenericComponentType_AudioOutput, mpContext->audio_output_id[i], mpContext->setting.prefer_string_audio_output, mpContext->p_pb_sound_tracks[i]);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_AudioOutput, %d) fail, ret %d, %s.\n", i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    LOG_PRINTF("[CMediaSimpleAPI]: createPlaybackComponents(%d) end\n", max);

    return EECode_OK;
}

EECode CMediaSimpleAPI::createPlaybackExtSourceComponents()
{
    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    EECode err = EECode_OK;
    TU32 i = 0, max = mpContext->setting.total_num_playback_instance;

    LOG_PRINTF("[CMediaSimpleAPI]: createPlaybackExtSourceComponents() begin, max %d\n", max);

    if (mpContext->setting.enable_video) {
        for (i = 0; i < max; i ++) {
            err = mpEngineControl->NewComponent(EGenericComponentType_ExtVideoESSource, mpContext->ext_video_source_id[i], mpContext->setting.prefer_string_ext_video_source);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_ExtVideoESSource, %d) fail, ret %d, %s.\n", i, err, gfGetErrorCodeString(err));
                return err;
            }

            err = mpEngineControl->NewComponent(EGenericComponentType_VideoDecoder, mpContext->video_decoder_id[i], mpContext->setting.prefer_string_video_decoder);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_VideoDecoder, %d) fail, ret %d, %s.\n", i, err, gfGetErrorCodeString(err));
                return err;
            }

            err = mpEngineControl->NewComponent(EGenericComponentType_VideoOutput, mpContext->video_output_id[i], mpContext->setting.prefer_string_video_output, mpContext->p_pb_textures[i]);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_VideoOutput, %d) fail, ret %d, %s.\n", i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    if (mpContext->setting.enable_audio) {
        for (i = 0; i < max; i ++) {
            err = mpEngineControl->NewComponent(EGenericComponentType_ExtAudioESSource, mpContext->ext_audio_source_id[i], mpContext->setting.prefer_string_ext_audio_source);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_ExtAudioESSource, %d) fail, ret %d, %s.\n", i, err, gfGetErrorCodeString(err));
                return err;
            }

            err = mpEngineControl->NewComponent(EGenericComponentType_AudioDecoder, mpContext->audio_decoder_id[i], mpContext->setting.prefer_string_audio_decoder);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_AudioDecoder, %d) fail, ret %d, %s.\n", i, err, gfGetErrorCodeString(err));
                return err;
            }

            err = mpEngineControl->NewComponent(EGenericComponentType_AudioOutput, mpContext->audio_output_id[i], mpContext->setting.prefer_string_audio_output, mpContext->p_pb_sound_tracks[i]);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_AudioOutput, %d) fail, ret %d, %s.\n", i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    LOG_PRINTF("[CMediaSimpleAPI]: createPlaybackExtSourceComponents(%d) end\n", max);

    return EECode_OK;
}

EECode CMediaSimpleAPI::createPlaybackLocalStorageComponents()
{
    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    EECode err = EECode_OK;
    TU32 i = 0, max = mpContext->setting.total_num_playback_instance;

    LOG_PRINTF("[CMediaSimpleAPI]: createPlaybackLocalStorageComponents() begin, max %d\n", max);

    for (i = 0; i < max; i ++) {
        err = mpEngineControl->NewComponent(EGenericComponentType_Muxer, mpContext->pb_muxer_id[i], mpContext->setting.prefer_string_muxer);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("NewComponent(EGenericComponentType_Muxer, %d) fail, ret %d, %s.\n", i, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    LOG_PRINTF("[CMediaSimpleAPI]: createPlaybackLocalStorageComponents(%d) end\n", max);

    return EECode_OK;
}

EECode CMediaSimpleAPI::createCaptureComponents()
{
    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    EECode err = EECode_OK;
    TU32 i = 0, max = mpContext->setting.total_num_capture_instance;

    LOG_PRINTF("[CMediaSimpleAPI]: createCaptureComponents() begin, max %d\n", max);

    if (mpContext->setting.enable_video) {
        for (i = 0; i < max; i ++) {
            err = mpEngineControl->NewComponent(EGenericComponentType_VideoCapture, mpContext->video_capturer_id[i], mpContext->setting.prefer_string_video_capture);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_VideoCapture, %d) fail, ret %d, %s.\n", i, err, gfGetErrorCodeString(err));
                return err;
            }

            err = mpEngineControl->NewComponent(EGenericComponentType_VideoEncoder, mpContext->video_encoder_id[i], mpContext->setting.prefer_string_video_encoder);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_VideoEncoder, %d) fail, ret %d, %s.\n", i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    if (mpContext->setting.enable_audio) {
        for (i = 0; i < max; i ++) {
            err = mpEngineControl->NewComponent(EGenericComponentType_AudioCapture, mpContext->audio_capturer_id[i], mpContext->setting.prefer_string_audio_capture, mpContext->p_sound_input_devices[i]);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_AudioCapture, %d) fail, ret %d, %s.\n", i, err, gfGetErrorCodeString(err));
                return err;
            }

            err = mpEngineControl->NewComponent(EGenericComponentType_AudioEncoder, mpContext->audio_encoder_id[i], mpContext->setting.prefer_string_audio_encoder);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_AudioEncoder, %d) fail, ret %d, %s.\n", i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    LOG_PRINTF("[CMediaSimpleAPI]: createPlaybackComponents(%d) end\n", max);

    return EECode_OK;
}

EECode CMediaSimpleAPI::createCaptureLocalStorageComponents()
{
    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    EECode err = EECode_OK;
    TU32 i = 0, max = mpContext->setting.total_num_capture_instance;

    LOG_PRINTF("[CMediaSimpleAPI]: createCaptureLocalStorageComponents() begin, max %d\n", max);

    for (i = 0; i < max; i ++) {
        err = mpEngineControl->NewComponent(EGenericComponentType_Muxer, mpContext->cap_muxer_id[i], mpContext->setting.prefer_string_muxer);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("NewComponent(EGenericComponentType_Muxer, %d) fail, ret %d, %s.\n", i, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    LOG_PRINTF("[CMediaSimpleAPI]: createCaptureLocalStorageComponents(%d) end\n", max);

    return EECode_OK;
}

EECode CMediaSimpleAPI::createCaptureLocalStreamingComponents()
{
    EECode err = EECode_OK;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_PRINTF("[CMediaSimpleAPI]: createCaptureLocalStreamingComponents begin\n");

    err = mpEngineControl->NewComponent(EGenericComponentType_StreamingTransmiter, mpContext->streaming_transmiter_id);
    DASSERT_OK(err);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("NewComponent(EGenericComponentType_StreamingTransmiter) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
        return err;
    }

    LOG_PRINTF("[CMediaSimpleAPI]: createCaptureLocalStreamingComponents end\n");

    return EECode_OK;
}

EECode CMediaSimpleAPI::connectPlaybackComponents()
{
    EECode err = EECode_OK;
    TU32 i = 0, max = mpContext->setting.total_num_playback_instance;
    TGenericID connection_id;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_PRINTF("[CMediaSimpleAPI]: connectPlaybackComponents() begin\n");

    if (DLikely(mpContext->setting.enable_video)) {
        for (i = 0; i < max; i ++) {
            err = mpEngineControl->ConnectComponent(connection_id, mpContext->demuxer_id[i], mpContext->video_decoder_id[i], StreamType_Video);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(demuxer[%d] to video decoder[%d]), err %d, %s\n", i, i, err, gfGetErrorCodeString(err));
                return err;
            }

            err = mpEngineControl->ConnectComponent(connection_id, mpContext->video_decoder_id[i], mpContext->video_output_id[i], StreamType_Video);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(video decoder [%d] to video output, err %d, %s\n", i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    if (mpContext->setting.enable_audio) {
        for (i = 0; i < max; i ++) {
            err = mpEngineControl->ConnectComponent(connection_id, mpContext->demuxer_id[i], mpContext->audio_decoder_id[i], StreamType_Audio);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(demuxer[%d] to audio decoder[%d]), err %d, %s\n", i, i, err, gfGetErrorCodeString(err));
                return err;
            }

            err = mpEngineControl->ConnectComponent(connection_id, mpContext->audio_decoder_id[i], mpContext->audio_output_id[i], StreamType_Audio);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(audio decoder [%d] to audio output, err %d, %s\n", i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    LOG_PRINTF("[CMediaSimpleAPI]: connectPlaybackComponents() end\n");

    return EECode_OK;
}

EECode CMediaSimpleAPI::connectPlaybackExtSourceComponents()
{
    EECode err = EECode_OK;
    TU32 i = 0, max = mpContext->setting.total_num_playback_instance;
    TGenericID connection_id;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_PRINTF("[CMediaSimpleAPI]: connectPlaybackExtSourceComponents() begin\n");

    if (DLikely(mpContext->setting.enable_video)) {
        for (i = 0; i < max; i ++) {
            err = mpEngineControl->ConnectComponent(connection_id, mpContext->ext_video_source_id[i], mpContext->video_decoder_id[i], StreamType_Video);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(ext_video_source_id[%d] to video decoder[%d]), err %d, %s\n", i, i, err, gfGetErrorCodeString(err));
                return err;
            }

            err = mpEngineControl->ConnectComponent(connection_id, mpContext->video_decoder_id[i], mpContext->video_output_id[i], StreamType_Video);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(video decoder [%d] to video output, err %d, %s\n", i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    if (mpContext->setting.enable_audio) {
        for (i = 0; i < max; i ++) {
            err = mpEngineControl->ConnectComponent(connection_id, mpContext->ext_audio_source_id[i], mpContext->audio_decoder_id[i], StreamType_Audio);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(ext_audio_source_id[%d] to audio decoder[%d]), err %d, %s\n", i, i, err, gfGetErrorCodeString(err));
                return err;
            }

            err = mpEngineControl->ConnectComponent(connection_id, mpContext->audio_decoder_id[i], mpContext->audio_output_id[i], StreamType_Audio);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(audio decoder [%d] to audio output, err %d, %s\n", i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    LOG_PRINTF("[CMediaSimpleAPI]: connectPlaybackExtSourceComponents() end\n");

    return EECode_OK;
}

EECode CMediaSimpleAPI::connectPlaybackLocalStorageComponents()
{
    EECode err = EECode_OK;
    TU32 i = 0, max = mpContext->setting.total_num_playback_instance;
    TGenericID connection_id;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_PRINTF("[CMediaSimpleAPI]: connectPlaybackLocalStorageComponents() begin\n");

    if ((mpContext->setting.enable_video) && (mpContext->setting.enable_audio)) {
        for (i = 0; i < max; i ++) {
            err = mpEngineControl->ConnectComponent(connection_id, mpContext->demuxer_id[i], mpContext->pb_muxer_id[i], StreamType_Video);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(demuxer[%d] to muxer[%d]), playback local storage, video, err %d, %s\n", i, i, err, gfGetErrorCodeString(err));
                return err;
            }

            err = mpEngineControl->ConnectComponent(connection_id, mpContext->demuxer_id[i], mpContext->pb_muxer_id[i], StreamType_Audio);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(demuxer[%d] to muxer[%d]), playback local storage, audio, err %d, %s\n", i, i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    } else if (mpContext->setting.enable_video) {
        for (i = 0; i < max; i ++) {
            err = mpEngineControl->ConnectComponent(connection_id, mpContext->demuxer_id[i], mpContext->pb_muxer_id[i], StreamType_Video);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(demuxer[%d] to muxer[%d]), playback local storage, video, err %d, %s\n", i, i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    } else if (mpContext->setting.enable_audio) {
        for (i = 0; i < max; i ++) {
            err = mpEngineControl->ConnectComponent(connection_id, mpContext->demuxer_id[i], mpContext->pb_muxer_id[i], StreamType_Audio);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(demuxer[%d] to muxer[%d]), playback local storage, audio, err %d, %s\n", i, i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    } else {
        LOG_FATAL("why comes here?\n");
    }

    LOG_PRINTF("[CMediaSimpleAPI]: connectPlaybackLocalStorageComponents() end\n");

    return EECode_OK;
}

EECode CMediaSimpleAPI::connectPlaybackExtSourceLocalStorageComponents()
{
    EECode err = EECode_OK;
    TU32 i = 0, max = mpContext->setting.total_num_playback_instance;
    TGenericID connection_id;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_PRINTF("[CMediaSimpleAPI]: connectPlaybackExtSourceLocalStorageComponents() begin\n");

    if ((mpContext->setting.enable_video) && (mpContext->setting.enable_audio)) {
        for (i = 0; i < max; i ++) {
            err = mpEngineControl->ConnectComponent(connection_id, mpContext->ext_video_source_id[i], mpContext->pb_muxer_id[i], StreamType_Video);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(ext_video_source_id[%d] to muxer[%d]), playback local storage, video, err %d, %s\n", i, i, err, gfGetErrorCodeString(err));
                return err;
            }

            err = mpEngineControl->ConnectComponent(connection_id, mpContext->ext_audio_source_id[i], mpContext->pb_muxer_id[i], StreamType_Audio);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(ext_audio_source_id[%d] to muxer[%d]), playback local storage, audio, err %d, %s\n", i, i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    } else if (mpContext->setting.enable_video) {
        for (i = 0; i < max; i ++) {
            err = mpEngineControl->ConnectComponent(connection_id, mpContext->ext_video_source_id[i], mpContext->pb_muxer_id[i], StreamType_Video);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(ext_video_source_id[%d] to muxer[%d]), playback local storage, video, err %d, %s\n", i, i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    } else if (mpContext->setting.enable_audio) {
        for (i = 0; i < max; i ++) {
            err = mpEngineControl->ConnectComponent(connection_id, mpContext->ext_audio_source_id[i], mpContext->pb_muxer_id[i], StreamType_Audio);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(ext_audio_source_id[%d] to muxer[%d]), playback local storage, audio, err %d, %s\n", i, i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    } else {
        LOG_FATAL("why comes here?\n");
    }

    LOG_PRINTF("[CMediaSimpleAPI]: connectPlaybackExtSourceLocalStorageComponents() end\n");

    return EECode_OK;
}

EECode CMediaSimpleAPI::connectCaptureComponents()
{
    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    EECode err = EECode_OK;
    TU32 i = 0, max = mpContext->setting.total_num_capture_instance;
    TGenericID connection_id;

    LOG_PRINTF("[CMediaSimpleAPI]: connectCaptureComponents() begin, max %d\n", max);

    if (mpContext->setting.enable_video) {
        for (i = 0; i < max; i ++) {
            err = mpEngineControl->ConnectComponent(connection_id, mpContext->video_capturer_id[i], mpContext->video_encoder_id[i], StreamType_Video);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(video capturer[%d] to video encoder[%d]), err %d, %s\n", i, i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    if (mpContext->setting.enable_audio) {
        for (i = 0; i < max; i ++) {
            err = mpEngineControl->ConnectComponent(connection_id, mpContext->audio_capturer_id[i], mpContext->audio_encoder_id[i], StreamType_Audio);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(audio capturer[%d] to audio encoder[%d]), err %d, %s\n", i, i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    LOG_PRINTF("[CMediaSimpleAPI]: createPlaybackComponents(%d) end\n", max);

    return EECode_OK;
}

EECode CMediaSimpleAPI::connectCaptureLocalStorageComponents()
{
    EECode err = EECode_OK;
    TU32 i = 0, max = mpContext->setting.total_num_capture_instance;
    TGenericID connection_id;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_PRINTF("[CMediaSimpleAPI]: connectCaptureLocalStorageComponents() begin\n");

    if ((mpContext->setting.enable_video) && (mpContext->setting.enable_audio)) {
        for (i = 0; i < max; i ++) {
            err = mpEngineControl->ConnectComponent(connection_id, mpContext->video_encoder_id[i], mpContext->cap_muxer_id[i], StreamType_Video);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(video encoder[%d] to muxer[%d]), capture local storage, video, err %d, %s\n", i, i, err, gfGetErrorCodeString(err));
                return err;
            }

            err = mpEngineControl->ConnectComponent(connection_id, mpContext->audio_encoder_id[i], mpContext->cap_muxer_id[i], StreamType_Audio);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(audio encoder[%d] to muxer[%d]), capture local storage, audio, err %d, %s\n", i, i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    } else if (mpContext->setting.enable_video) {
        for (i = 0; i < max; i ++) {
            err = mpEngineControl->ConnectComponent(connection_id, mpContext->video_encoder_id[i], mpContext->cap_muxer_id[i], StreamType_Video);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(video encoder[%d] to muxer[%d]), capture local storage, video, err %d, %s\n", i, i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    } else if (mpContext->setting.enable_audio) {
        for (i = 0; i < max; i ++) {
            err = mpEngineControl->ConnectComponent(connection_id, mpContext->audio_encoder_id[i], mpContext->cap_muxer_id[i], StreamType_Audio);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(audio encoder[%d] to muxer[%d]), capture local storage, audio, err %d, %s\n", i, i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    } else {
        LOG_FATAL("why comes here?\n");
    }

    LOG_PRINTF("[CMediaSimpleAPI]: connectCaptureLocalStorageComponents() end\n");

    return EECode_OK;
}

EECode CMediaSimpleAPI::connectCaptureLocalStreamingComponents()
{
    EECode err = EECode_OK;
    TU32 i = 0, max = mpContext->setting.total_num_capture_instance;
    TGenericID connection_id;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_PRINTF("[CMediaSimpleAPI]: connectCaptureLocalStreamingComponents() begin\n");

    if ((mpContext->setting.enable_video) && (mpContext->setting.enable_audio)) {
        for (i = 0; i < max; i ++) {
            err = mpEngineControl->ConnectComponent(connection_id, mpContext->video_encoder_id[i], mpContext->streaming_transmiter_id, StreamType_Video);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(video encoder[%d] to streaming transmiter[%d]), capture local streaming, video, err %d, %s\n", i, i, err, gfGetErrorCodeString(err));
                return err;
            }

            err = mpEngineControl->ConnectComponent(connection_id, mpContext->audio_encoder_id[i], mpContext->streaming_transmiter_id, StreamType_Audio);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(audio encoder[%d] to streaming transmiter[%d]), capture local streaming, audio, err %d, %s\n", i, i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    } else if (mpContext->setting.enable_video) {
        for (i = 0; i < max; i ++) {
            err = mpEngineControl->ConnectComponent(connection_id, mpContext->video_encoder_id[i], mpContext->streaming_transmiter_id, StreamType_Video);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(video encoder[%d] to streaming transmiter[%d]), capture local streaming, video, err %d, %s\n", i, i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    } else if (mpContext->setting.enable_audio) {
        for (i = 0; i < max; i ++) {
            err = mpEngineControl->ConnectComponent(connection_id, mpContext->audio_encoder_id[i], mpContext->streaming_transmiter_id, StreamType_Audio);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(audio encoder[%d] to streaming transmiter[%d]), capture local streaming, audio, err %d, %s\n", i, i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    } else {
        LOG_FATAL("why comes here?\n");
    }

    LOG_PRINTF("[CMediaSimpleAPI]: connectCaptureLocalStreamingComponents() end\n");

    return EECode_OK;
}

EECode CMediaSimpleAPI::setupPlaybackPipelines()
{
    EECode err = EECode_OK;
    TU32 i = 0, max = mpContext->setting.total_num_playback_instance;
    TGenericID video_source_id = 0, audio_source_id = 0;

    LOG_NOTICE("[CMediaSimpleAPI]::setupPlaybackPipelines begin, max %d\n", max);
    for (i = 0; i < max; i ++) {
        if (mpContext->setting.enable_video) {
            video_source_id = mpContext->demuxer_id[i];
        } else {
            video_source_id = 0;
        }
        if (mpContext->setting.enable_audio) {
            audio_source_id = mpContext->demuxer_id[i];
        } else {
            audio_source_id = 0;
        }

        err = mpEngineControl->SetupPlaybackPipeline(mpContext->pb_pipelines_id[i], \
                video_source_id, \
                audio_source_id, \
                mpContext->video_decoder_id[i], \
                mpContext->audio_decoder_id[i], \
                mpContext->video_output_id[i], \
                mpContext->audio_output_id[i], \
                0, \
                1);

        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("SetupPlaybackPipeline[%d], err %d, %s\n", i, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    return EECode_OK;
}

EECode CMediaSimpleAPI::setupPlaybackExtSourcePipelines()
{
    EECode err = EECode_OK;
    TU32 i = 0, max = mpContext->setting.total_num_playback_instance;
    TGenericID video_source_id = 0, audio_source_id = 0;

    LOG_NOTICE("[CMediaSimpleAPI]::setupPlaybackExtSourcePipelines begin, max %d\n", max);
    for (i = 0; i < max; i ++) {
        if (mpContext->setting.enable_video) {
            video_source_id = mpContext->ext_video_source_id[i];
        } else {
            video_source_id = 0;
        }
        if (mpContext->setting.enable_audio) {
            audio_source_id = mpContext->ext_audio_source_id[i];
        } else {
            audio_source_id = 0;
        }

        err = mpEngineControl->SetupPlaybackPipeline(mpContext->pb_pipelines_id[i], \
                video_source_id, \
                audio_source_id, \
                mpContext->video_decoder_id[i], \
                mpContext->audio_decoder_id[i], \
                mpContext->video_output_id[i], \
                mpContext->audio_output_id[i], \
                0, \
                1);

        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("setupPlaybackExtSourcePipelines[%d], err %d, %s\n", i, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    return EECode_OK;
}

EECode CMediaSimpleAPI::setupPlaybackLocalStoragePipelines()
{
    EECode err = EECode_OK;
    TU32 i = 0, max = mpContext->setting.total_num_playback_instance;

    LOG_NOTICE("[CMediaSimpleAPI]: setupPlaybackLocalStoragePipelines begin, max %d\n", max);

    if (mpContext->setting.enable_video && mpContext->setting.enable_audio) {
        for (i = 0; i < max; i ++) {
            err = mpEngineControl->SetupRecordingPipeline(mpContext->pb_recording_pipelines_id[i], \
                    mpContext->demuxer_id[i], \
                    mpContext->demuxer_id[i], \
                    mpContext->pb_muxer_id[i]);

            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("SetupRecordingPipeline[%d], err %d, %s\n", i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    } else if (mpContext->setting.enable_video) {
        for (i = 0; i < max; i ++) {
            err = mpEngineControl->SetupRecordingPipeline(mpContext->pb_recording_pipelines_id[i], \
                    mpContext->demuxer_id[i], \
                    0, \
                    mpContext->pb_muxer_id[i]);

            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("SetupRecordingPipeline[%d], err %d, %s\n", i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    } else if (mpContext->setting.enable_audio) {
        for (i = 0; i < max; i ++) {
            err = mpEngineControl->SetupRecordingPipeline(mpContext->pb_recording_pipelines_id[i], \
                    0, \
                    mpContext->demuxer_id[i], \
                    mpContext->pb_muxer_id[i]);

            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("SetupRecordingPipeline[%d], err %d, %s\n", i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    } else {
        LOG_ERROR("audio video are both disabled?\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CMediaSimpleAPI::setupPlaybackExtSourceLocalStoragePipelines()
{
    EECode err = EECode_OK;
    TU32 i = 0, max = mpContext->setting.total_num_playback_instance;

    LOG_NOTICE("[CMediaSimpleAPI]: setupPlaybackExtSourceLocalStoragePipelines begin, max %d\n", max);

    for (i = 0; i < max; i ++) {
        err = mpEngineControl->SetupRecordingPipeline(mpContext->pb_recording_pipelines_id[i], \
                mpContext->ext_video_source_id[i], \
                mpContext->ext_audio_source_id[i], \
                mpContext->pb_muxer_id[i]);

        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("SetupRecordingPipeline[%d], err %d, %s\n", i, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    return EECode_OK;
}

EECode CMediaSimpleAPI::setupCapturePipelines()
{
    EECode err = EECode_OK;
    TU32 i = 0, max = mpContext->setting.total_num_capture_instance;

    LOG_NOTICE("[CMediaSimpleAPI]: setupCapturePipelines begin, max %d\n", max);

    for (i = 0; i < max; i ++) {
        err = mpEngineControl->SetupCapturePipeline(mpContext->cap_pipelines_id[i], \
                mpContext->video_capturer_id[i], \
                mpContext->audio_capturer_id[i], \
                mpContext->video_encoder_id[i], \
                mpContext->audio_encoder_id[i], \
                1);

        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("SetupCapturePipeline[%d], err %d, %s\n", i, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    return EECode_OK;
}

EECode CMediaSimpleAPI::setupCaptureLocalStoragePipelines()
{
    EECode err = EECode_OK;
    TU32 i = 0, max = mpContext->setting.total_num_capture_instance;

    LOG_NOTICE("[CMediaSimpleAPI]: setupCaptureLocalStoragePipelines begin, max %d\n", max);

    for (i = 0; i < max; i ++) {
        err = mpEngineControl->SetupRecordingPipeline(mpContext->cap_recording_pipelines_id[i], \
                mpContext->video_encoder_id[i], \
                mpContext->audio_encoder_id[i], \
                mpContext->cap_muxer_id[i]);

        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("SetupRecordingPipeline[%d], err %d, %s\n", i, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    return EECode_OK;
}

EECode CMediaSimpleAPI::setupCaptureLocalStreamingPipelines()
{
    EECode err = EECode_OK;
    TU32 i = 0, max = mpContext->setting.total_num_capture_instance;

    LOG_NOTICE("[CMediaSimpleAPI]: setupCaptureLocalStreamingPipelines begin, max %d\n", max);

    for (i = 0; i < max; i ++) {
        err = mpEngineControl->SetupStreamingPipeline(mpContext->cap_streaming_pipelines_id[i], \
                mpContext->streaming_transmiter_id, \
                mpContext->streaming_server_id, \
                mpContext->video_encoder_id[i], \
                mpContext->audio_encoder_id[i], \
                0);

        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("SetupStreamingPipeline[%d], err %d, %s\n", i, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    return EECode_OK;
}

EECode CMediaSimpleAPI::buildDataProcessingGragh()
{
    EECode err = EECode_OK;
    TUint index = 0;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_PRINTF("[CMediaSimpleAPI]: buildDataProcessingGragh begin\n");

    err = mpEngineControl->BeginConfigProcessPipeline(1);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("BeginConfigProcessPipeline(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = createGlobalComponents();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("createGlobalComponents(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    if (mpContext->setting.enable_playback) {
        if (!mpContext->setting.pb_use_external_source) {
            err = createPlaybackComponents();

            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("createPlaybackComponents(), err %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }

            if (mpContext->setting.enable_playback_local_storage) {
                err = createPlaybackLocalStorageComponents();
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("createPlaybackLocalStorageComponents(), err %d, %s\n", err, gfGetErrorCodeString(err));
                    return err;
                }
            }
        } else {
            err = createPlaybackExtSourceComponents();

            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("createPlaybackExtSourceComponents(), err %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }

            if (mpContext->setting.enable_playback_local_storage) {
                err = createPlaybackLocalStorageComponents();
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("createPlaybackLocalStorageComponents(), err %d, %s\n", err, gfGetErrorCodeString(err));
                    return err;
                }
            }
        }
    }

    if (mpContext->setting.enable_capture) {
        err = createCaptureComponents();
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("createCaptureComponents(), err %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }

        if (mpContext->setting.enable_capture_local_storage) {
            err = createCaptureLocalStorageComponents();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("createCaptureLocalStorageComponents(), err %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        }

        if (mpContext->setting.enable_capture_streaming_out) {
            err = createCaptureLocalStreamingComponents();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("createCaptureLocalStreamingComponents(), err %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    if (mpContext->setting.enable_playback) {
        if (!mpContext->setting.pb_use_external_source) {
            err = connectPlaybackComponents();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("connectPlaybackComponents(), err %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }

            if (mpContext->setting.enable_playback_local_storage) {
                err = connectPlaybackLocalStorageComponents();
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("connectPlaybackLocalStorageComponents(), err %d, %s\n", err, gfGetErrorCodeString(err));
                    return err;
                }
            }
        } else {
            err = connectPlaybackExtSourceComponents();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("connectPlaybackExtSourceComponents(), err %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }

            if (mpContext->setting.enable_playback_local_storage) {
                err = connectPlaybackExtSourceLocalStorageComponents();
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("connectPlaybackExtSourceLocalStorageComponents(), err %d, %s\n", err, gfGetErrorCodeString(err));
                    return err;
                }
            }
        }
    }

    if (mpContext->setting.enable_capture) {
        err = connectCaptureComponents();
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("connectCaptureComponents(), err %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }

        if (mpContext->setting.enable_capture_local_storage) {
            err = connectCaptureLocalStorageComponents();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("connectCaptureLocalStorageComponents(), err %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        }

        if (mpContext->setting.enable_capture_streaming_out) {
            err = connectCaptureLocalStreamingComponents();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("connectCaptureLocalStreamingComponents(), err %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    if (mpContext->setting.enable_playback) {
        if (!mpContext->setting.pb_use_external_source) {
            err = setupPlaybackPipelines();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("setupPlaybackPipelines(), err %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }

            if (mpContext->setting.enable_playback_local_storage) {
                err = setupPlaybackLocalStoragePipelines();
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("setupPlaybackLocalStoragePipelines(), err %d, %s\n", err, gfGetErrorCodeString(err));
                    return err;
                }
            }
        } else {
            err = setupPlaybackExtSourcePipelines();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("setupPlaybackExtSourcePipelines(), err %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }

            if (mpContext->setting.enable_playback_local_storage) {
                err = setupPlaybackExtSourceLocalStoragePipelines();
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("setupPlaybackExtSourceLocalStoragePipelines(), err %d, %s\n", err, gfGetErrorCodeString(err));
                    return err;
                }
            }
        }
    }

    if (mpContext->setting.enable_capture) {
        err = setupCapturePipelines();
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("setupCapturePipelines(), err %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }

        if (mpContext->setting.enable_capture_local_storage) {
            err = setupCaptureLocalStoragePipelines();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("setupCaptureLocalStoragePipelines(), err %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        }

        if (mpContext->setting.enable_capture_streaming_out) {
            err = setupCaptureLocalStreamingPipelines();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("setupCaptureLocalStreamingPipelines(), err %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    err = preSetUrls();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("preSetUrls(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = mpEngineControl->FinalizeConfigProcessPipeline();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("FinalizeConfigProcessPipeline(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    LOG_PRINTF("[CMediaSimpleAPI]: buildDataProcessingGragh end\n");

    return EECode_OK;
}

EECode CMediaSimpleAPI::preSetUrls()
{
    EECode err = EECode_OK;
    TUint i = 0, max = 0;

    if (mpContext->setting.enable_playback) {
        if (mpContext->setting.enable_playback_local_storage) {
            max = mpContext->setting.total_num_playback_instance;
            for (i = 0; i < max; i ++) {
                if (0x0 != mpContext->setting.pb_sink_url[i][0]) {
                    err = mpEngineControl->SetSinkUrl(mpContext->pb_muxer_id[i], mpContext->setting.pb_sink_url[i]);
                    DASSERT_OK(err);
                }
            }
        }
    }

    if (mpContext->setting.enable_capture_streaming_out) {
        max = mpContext->setting.total_num_capture_instance;
        for (i = 0; i < max; i ++) {
            if (0x0 != mpContext->setting.cap_local_streaming_url[i][0]) {
                err = mpEngineControl->SetStreamingUrl(mpContext->cap_streaming_pipelines_id[i], mpContext->setting.cap_local_streaming_url[i]);
                DASSERT_OK(err);
            }
        }
    }

    if (mpContext->setting.enable_capture_local_storage) {
        max = mpContext->setting.total_num_capture_instance;
        for (i = 0; i < max; i ++) {
            if (0x0 != mpContext->setting.cap_sink_url[i][0]) {
                err = mpEngineControl->SetSinkUrl(mpContext->cap_muxer_id[i], mpContext->setting.cap_sink_url[i]);
                DASSERT_OK(err);
            }
        }
    }

    return EECode_OK;
}

EECode CMediaSimpleAPI::setUrls()
{
    EECode err = EECode_OK;
    TUint i = 0, max = 0;

    if (mpContext->setting.enable_playback) {
        max = mpContext->setting.total_num_playback_instance;
        for (i = 0; i < max; i ++) {
            if ((!mpContext->setting.pb_use_external_source) && (0x0 != mpContext->setting.pb_source_url[i][0])) {
                err = mpEngineControl->SetSourceUrl(mpContext->demuxer_id[i], mpContext->setting.pb_source_url[i]);
                DASSERT_OK(err);
            }
        }
    }

    return EECode_OK;
}

EECode CMediaSimpleAPI::getHandlerForExternalSource()
{
    TU32 i = 0, max = mpContext->setting.total_num_playback_instance;
    SQueryGetHandler query;

    if (mpContext->setting.enable_playback) {
        if (mpContext->setting.pb_use_external_source) {

            if (mpContext->setting.enable_video) {
                for (i = 0; i < max; i ++) {
                    if (mpContext->ext_video_source_id[i]) {
                        query.check_field = EGenericEngineQuery_GetHandler;
                        query.id = mpContext->ext_video_source_id[i];
                        mpEngineControl->GenericControl(EGenericEngineQuery_GetHandler, (void *) &query);
                        mpContext->p_ext_video_source_handler[i] = (void *) query.handler;
                    }
                }
            }

            if (mpContext->setting.enable_audio) {
                for (i = 0; i < max; i ++) {
                    if (mpContext->ext_audio_source_id[i]) {
                        query.check_field = EGenericEngineQuery_GetHandler;
                        query.id = mpContext->ext_audio_source_id[i];
                        mpEngineControl->GenericControl(EGenericEngineQuery_GetHandler, (void *) &query);
                        mpContext->p_ext_audio_source_handler[i] = (void *) query.handler;
                    }
                }
            }
        }
    }
    return EECode_OK;
}

void CMediaSimpleAPI::setPBVideoSourceExternalDataProcessingCallback()
{
    TU32 i = 0, max = mpContext->setting.total_num_playback_instance;
    SExternalDataProcessingCallback set_callback;

    if (mpContext->setting.enable_playback) {
        if (mpContext->setting.enable_video) {
            for (i = 0; i < max; i ++) {
                if (mpContext->demuxer_id[i]) {
                    set_callback.check_field = EGenericEngineConfigure_SetVideoExternalDataPostProcessingCallback;
                    set_callback.id = mpContext->demuxer_id[i];
                    if (mpContext->p_pb_videosource_ext_post_processing_callback[i] && mpContext->p_pb_videosource_ext_post_processing_callback_context[i]) {
                        set_callback.callback = mpContext->p_pb_videosource_ext_post_processing_callback[i];
                        set_callback.callback_context = mpContext->p_pb_videosource_ext_post_processing_callback_context[i];
                        mpEngineControl->GenericControl(EGenericEngineConfigure_SetVideoExternalDataPostProcessingCallback, (void *) &set_callback);
                    }
                }
            }
        }
    }
}

EECode gfMediaSimpleAPILoadDefaultCapServerConfig(SMediaSimpleAPIContext *p_content)
{
    if (DUnlikely(!p_content)) {
        LOG_ERROR("gfMediaSimpleAPILoadDefaultCapServerConfig: NULL params\n");
        return EECode_BadParam;
    }

    memset(p_content, 0x0, sizeof(SMediaSimpleAPIContext));
    p_content->setting.enable_force_log_level = 0;
    p_content->setting.force_log_level = ELogLevel_None;
    p_content->setting.enable_video = 1;
    p_content->setting.enable_audio = 1;
    p_content->setting.enable_capture = 1;
    p_content->setting.enable_capture_streaming_out = 1;
    p_content->setting.enable_capture_local_storage = 0;
    p_content->setting.enable_capture_cloud_uploading = 0;
    p_content->setting.total_num_capture_instance = 1;
    p_content->setting.net_receiver_scheduler_number = 0;
    p_content->setting.net_sender_scheduler_number = 1;
    p_content->setting.fileio_reader_scheduler_number = 0;
    p_content->setting.fileio_writer_scheduler_number = 0;
    p_content->setting.record_strategy = MuxerSavingFileStrategy_ToTalFile;
    p_content->setting.record_autocut_condition = MuxerSavingCondition_InputPTS;
    p_content->setting.record_autocut_naming = MuxerAutoFileNaming_ByNumber;
    p_content->setting.record_autocut_framecount = 5400;
    p_content->setting.record_autocut_duration = 180 * DDefaultTimeScale;
    p_content->setting.rtsp_client_try_tcp_mode_first = 1;
    p_content->setting.rtsp_enable_rtcp = 1;
    p_content->setting.local_rtsp_port = DDefaultRTSPServerPort;
    p_content->setting.im_server_port = DDefaultSACPIMServerPort;
    p_content->setting.data_server_port = DDefaultSACPServerPort;
    p_content->setting.data_server_rtsp_port = DDefaultRTSPServerPort;

    p_content->setting.pb_ext_video_source_format = StreamFormat_H264;
    p_content->setting.pb_ext_audio_source_format = StreamFormat_AAC;

    snprintf(p_content->setting.cloud_server_url, DMaxUrlLen, "%s", (TChar *) "127.0.0.1");
    snprintf(p_content->setting.prefer_string_demuxer, DMaxPreferStringLen, "%s", DDefaultDemuxerModule);
    snprintf(p_content->setting.prefer_string_muxer, DMaxPreferStringLen, "%s", DDefaultMuxerModule);
    snprintf(p_content->setting.prefer_string_video_decoder, DMaxPreferStringLen, "%s", DDefaultVDModule);
    snprintf(p_content->setting.prefer_string_audio_decoder, DMaxPreferStringLen, "%s", DDefaultADModule);
    snprintf(p_content->setting.prefer_string_video_encoder, DMaxPreferStringLen, "%s", DDefaultVEModule);
    snprintf(p_content->setting.prefer_string_audio_encoder, DMaxPreferStringLen, "%s", DDefaultAEModule);
    snprintf(p_content->setting.prefer_string_video_capture, DMaxPreferStringLen, "%s", DDefaultVCModule);
    snprintf(p_content->setting.prefer_string_audio_capture, DMaxPreferStringLen, "%s", DDefaultACModule);
    snprintf(p_content->setting.cap_local_streaming_url[0], DMAX_URL_LENGTH, "%s", "local");
    snprintf(p_content->setting.cap_sink_url[0], DMAX_URL_LENGTH, "%s", "cap.mp4");
    p_content->audio_enc_setting.bitrate = 128000;
    p_content->video_cap_setting.width = 0;
    p_content->video_cap_setting.height = 0;
    p_content->video_cap_setting.offset_x = 0;
    p_content->video_cap_setting.offset_y = 0;
    p_content->video_enc_setting.width = 0;
    p_content->video_enc_setting.height = 0;
    p_content->video_enc_setting.framerate_num = DDefaultTimeScale;
    p_content->video_enc_setting.framerate_den = DDefaultTimeScale / 20;
    p_content->video_enc_setting.bitrate = 1000000;

    p_content->setting.pb_precached_video_frames = DDefaultAVSYNCVideoLatencyFrameCount;
    p_content->setting.pb_max_video_frames = DDefaultAVSYNCVideoMaxAccumulatedFrameCount;
    p_content->setting.b_render_video_nodelay = 0;

    p_content->common_config.sound_output.sound_output_buffer_count = DDefaultAudioOutputBufferFrameCount;
    p_content->common_config.sound_output.sound_output_precache_count = DDefaultAVSYNCAudioLatencyFrameCount;

    return EECode_OK;
}

EECode gfMediaSimpleAPILoadDefaultPlaybackConfig(SMediaSimpleAPIContext *p_content)
{
    if (DUnlikely(!p_content)) {
        LOG_ERROR("gfMediaSimpleAPILoadDefaultPlaybackConfig: NULL params\n");
        return EECode_BadParam;
    }

    memset(p_content, 0x0, sizeof(SMediaSimpleAPIContext));
    p_content->setting.enable_force_log_level = 0;
    p_content->setting.force_log_level = ELogLevel_None;
    p_content->setting.enable_video = 1;
    p_content->setting.enable_audio = 1;
    p_content->setting.enable_playback = 1;
    p_content->setting.enable_playback_local_storage = 0;
    p_content->setting.total_num_playback_instance = 1;
    p_content->setting.net_receiver_scheduler_number = 1;
    p_content->setting.net_sender_scheduler_number = 0;
    p_content->setting.fileio_reader_scheduler_number = 0;
    p_content->setting.fileio_writer_scheduler_number = 0;
    p_content->setting.record_strategy = MuxerSavingFileStrategy_ToTalFile;
    p_content->setting.record_autocut_condition = MuxerSavingCondition_InputPTS;
    p_content->setting.record_autocut_naming = MuxerAutoFileNaming_ByNumber;
    p_content->setting.record_autocut_framecount = 5400;
    p_content->setting.record_autocut_duration = 180 * DDefaultTimeScale;
    p_content->setting.rtsp_client_try_tcp_mode_first = 1;
    p_content->setting.rtsp_enable_rtcp = 1;
    p_content->setting.local_rtsp_port = DDefaultRTSPServerPort;
    p_content->setting.im_server_port = DDefaultSACPIMServerPort;
    p_content->setting.data_server_port = DDefaultSACPServerPort;
    p_content->setting.data_server_rtsp_port = DDefaultRTSPServerPort;

    p_content->setting.pb_ext_video_source_format = StreamFormat_H264;
    p_content->setting.pb_ext_audio_source_format = StreamFormat_AAC;

    snprintf(p_content->setting.cloud_server_url, DMaxUrlLen, "%s", "127.0.0.1");
    snprintf(p_content->setting.prefer_string_demuxer, DMaxPreferStringLen, "%s", DDefaultDemuxerModule);
    snprintf(p_content->setting.prefer_string_muxer, DMaxPreferStringLen, "%s", DDefaultMuxerModule);
    snprintf(p_content->setting.prefer_string_video_decoder, DMaxPreferStringLen, "%s", DDefaultVDModule);
    snprintf(p_content->setting.prefer_string_audio_decoder, DMaxPreferStringLen, "%s", DDefaultADModule);
    snprintf(p_content->setting.prefer_string_video_encoder, DMaxPreferStringLen, "%s", DDefaultVEModule);
    snprintf(p_content->setting.prefer_string_audio_encoder, DMaxPreferStringLen, "%s", DDefaultAEModule);
    snprintf(p_content->setting.prefer_string_video_capture, DMaxPreferStringLen, "%s", DDefaultVCModule);
    snprintf(p_content->setting.prefer_string_audio_capture, DMaxPreferStringLen, "%s", DDefaultACModule);
    snprintf(p_content->setting.cap_local_streaming_url[0], DMAX_URL_LENGTH, "%s", "local");
    snprintf(p_content->setting.cap_sink_url[0], DMAX_URL_LENGTH, "%s", "cap.mp4");
    p_content->audio_enc_setting.bitrate = 128000;
    p_content->video_cap_setting.width = 0;
    p_content->video_cap_setting.height = 0;
    p_content->video_cap_setting.offset_x = 0;
    p_content->video_cap_setting.offset_y = 0;
    p_content->video_enc_setting.width = 0;
    p_content->video_enc_setting.height = 0;
    p_content->video_enc_setting.framerate_num = DDefaultTimeScale;
    p_content->video_enc_setting.framerate_den = DDefaultTimeScale / 20;
    p_content->video_enc_setting.bitrate = 1000000;

    p_content->setting.pb_precached_video_frames = DDefaultAVSYNCVideoLatencyFrameCount;
    p_content->setting.pb_max_video_frames = DDefaultAVSYNCVideoMaxAccumulatedFrameCount;
    p_content->setting.b_render_video_nodelay = 0;

    p_content->common_config.sound_output.sound_output_buffer_count = DDefaultAudioOutputBufferFrameCount;
    p_content->common_config.sound_output.sound_output_precache_count = DDefaultAVSYNCAudioLatencyFrameCount;

    return EECode_OK;
}

EECode gfMediaSimpleAPILoadDefaultDebugConfig(SMediaSimpleAPIContext *p_content)
{
    if (DUnlikely(!p_content)) {
        LOG_ERROR("gfMediaSimpleAPILoadDefaultDebugConfig: NULL params\n");
        return EECode_BadParam;
    }

    memset(p_content, 0x0, sizeof(SMediaSimpleAPIContext));
    p_content->setting.enable_force_log_level = 0;
    p_content->setting.force_log_level = ELogLevel_None;
    p_content->setting.enable_video = 1;
    p_content->setting.enable_audio = 0;
    p_content->setting.enable_playback = 1;
    p_content->setting.enable_playback_local_storage = 0;
    p_content->setting.enable_capture = 0;
    p_content->setting.enable_capture_streaming_out = 0;
    p_content->setting.enable_capture_local_storage = 0;
    p_content->setting.enable_capture_cloud_uploading = 0;
    p_content->setting.total_num_playback_instance = 1;
    p_content->setting.total_num_capture_instance = 0;
    p_content->setting.net_receiver_scheduler_number = 0;
    p_content->setting.net_sender_scheduler_number = 1;
    p_content->setting.fileio_reader_scheduler_number = 0;
    p_content->setting.fileio_writer_scheduler_number = 0;
    p_content->setting.record_strategy = MuxerSavingFileStrategy_ToTalFile;
    p_content->setting.record_autocut_condition = MuxerSavingCondition_InputPTS;
    p_content->setting.record_autocut_naming = MuxerAutoFileNaming_ByNumber;
    p_content->setting.record_autocut_framecount = 5400;
    p_content->setting.record_autocut_duration = 180 * DDefaultTimeScale;
    p_content->setting.rtsp_client_try_tcp_mode_first = 1;
    p_content->setting.rtsp_enable_rtcp = 1;
    p_content->setting.local_rtsp_port = DDefaultRTSPServerPort;
    p_content->setting.im_server_port = DDefaultSACPIMServerPort;
    p_content->setting.data_server_port = DDefaultSACPServerPort;
    p_content->setting.data_server_rtsp_port = DDefaultRTSPServerPort;

    p_content->setting.pb_ext_video_source_format = StreamFormat_H264;
    p_content->setting.pb_ext_audio_source_format = StreamFormat_AAC;

    snprintf(p_content->setting.cloud_server_url, DMaxUrlLen, "%s", "127.0.0.1");
    snprintf(p_content->setting.prefer_string_demuxer, DMaxPreferStringLen, "%s", DDefaultDemuxerModule);
    snprintf(p_content->setting.prefer_string_muxer, DMaxPreferStringLen, "%s", DDefaultMuxerModule);
    snprintf(p_content->setting.prefer_string_video_decoder, DMaxPreferStringLen, "%s", DDefaultVDModule);
    snprintf(p_content->setting.prefer_string_audio_decoder, DMaxPreferStringLen, "%s", DDefaultADModule);
    snprintf(p_content->setting.prefer_string_video_encoder, DMaxPreferStringLen, "%s", DDefaultVEModule);
    snprintf(p_content->setting.prefer_string_audio_encoder, DMaxPreferStringLen, "%s", DDefaultAEModule);
    snprintf(p_content->setting.prefer_string_video_capture, DMaxPreferStringLen, "%s", DDefaultVCModule);
    snprintf(p_content->setting.prefer_string_audio_capture, DMaxPreferStringLen, "%s", DDefaultACModule);
    snprintf(p_content->setting.cap_local_streaming_url[0], DMAX_URL_LENGTH, "%s", "local");
    snprintf(p_content->setting.cap_sink_url[0], DMAX_URL_LENGTH, "%s", "cap.mp4");
    p_content->audio_enc_setting.bitrate = 128000;
    p_content->video_cap_setting.width = 0;
    p_content->video_cap_setting.height = 0;
    p_content->video_cap_setting.offset_x = 0;
    p_content->video_cap_setting.offset_y = 0;
    p_content->video_enc_setting.width = 0;
    p_content->video_enc_setting.height = 0;
    p_content->video_enc_setting.framerate_num = DDefaultTimeScale;
    p_content->video_enc_setting.framerate_den = DDefaultTimeScale / 20;
    p_content->video_enc_setting.bitrate = 1000000;

    p_content->setting.pb_precached_video_frames = DDefaultAVSYNCVideoLatencyFrameCount;
    p_content->setting.pb_max_video_frames = DDefaultAVSYNCVideoMaxAccumulatedFrameCount;
    p_content->setting.b_render_video_nodelay = 0;

    p_content->common_config.sound_output.sound_output_buffer_count = DDefaultAudioOutputBufferFrameCount;
    p_content->common_config.sound_output.sound_output_precache_count = DDefaultAVSYNCAudioLatencyFrameCount;

    return EECode_OK;
}

EECode gfMediaSimpleAPILoadMediaConfig(SMediaSimpleAPIContext *p_content, TChar *filename)
{
    if (DUnlikely(!filename)) {
        LOG_ERROR("gfMediaSimpleAPILoadMediaConfig: NULL filename\n");
        return EECode_BadParam;
    }

    if (DUnlikely(!p_content)) {
        LOG_ERROR("gfMediaSimpleAPILoadMediaConfig: NULL params\n");
        return EECode_BadParam;
    }

    IRunTimeConfigAPI *api = gfRunTimeConfigAPIFactory(ERunTimeConfigType_SimpleINI, 120);
    if (DUnlikely(!api)) {
        LOG_ERROR("gfRunTimeConfigAPIFactory(ERunTimeConfigType_SimpleINI) fail.\n");
        return EECode_NoMemory;
    }

    EECode err = api->OpenConfigFile((const TChar *) filename, 0, 1, 1);
    if (DUnlikely(EECode_OK != err)) {
        api->Destroy();
        LOG_NOTICE("OpenConfigFile(%s) fail, file not exist\n", filename);
        return EECode_OK;
    }

    TChar read_string[256] = {0};
    TChar read_string1[256] = {0};
    TU32 val = 0;

    err = api->GetContentValue("loglevel", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        if (0 == val) {
            p_content->setting.enable_force_log_level = 0;
        } else {
            p_content->setting.enable_force_log_level = 1;
            p_content->setting.force_log_level = val;
        }
        LOG_PRINTF("loglevel:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("enable_video", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        if (0 == val) {
            p_content->setting.enable_video = 0;
        } else {
            p_content->setting.enable_video = 1;
        }
        LOG_PRINTF("enable_video:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("enable_audio", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        if (0 == val) {
            p_content->setting.enable_audio = 0;
        } else {
            p_content->setting.enable_audio = 1;
        }
        LOG_PRINTF("enable_audio:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("enable_playback", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        if (0 == val) {
            p_content->setting.enable_playback = 0;
        } else {
            p_content->setting.enable_playback = 1;
        }
        LOG_PRINTF("enable_playback:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("enable_pb_storage", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        if (0 == val) {
            p_content->setting.enable_playback_local_storage = 0;
        } else {
            p_content->setting.enable_playback_local_storage = 1;
        }
        LOG_PRINTF("enable_pb_storage:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("enable_capture", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        if (0 == val) {
            p_content->setting.enable_capture = 0;
        } else {
            p_content->setting.enable_capture = 1;
        }
        LOG_PRINTF("enable_capture:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("enable_cap_streaming", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        if (0 == val) {
            p_content->setting.enable_capture_streaming_out = 0;
        } else {
            p_content->setting.enable_capture_streaming_out = 1;
        }
        LOG_PRINTF("enable_cap_streaming:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("enable_cap_storage", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        if (0 == val) {
            p_content->setting.enable_capture_local_storage = 0;
        } else {
            p_content->setting.enable_capture_local_storage = 1;
        }
        LOG_PRINTF("enable_cap_storage:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("enable_cap_uploading", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        if (0 == val) {
            p_content->setting.enable_capture_cloud_uploading = 0;
        } else {
            p_content->setting.enable_capture_cloud_uploading = 1;
        }
        LOG_PRINTF("enable_cap_uploading:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("total_pb_instance", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->setting.total_num_playback_instance = val;
        LOG_PRINTF("total_pb_instance:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("total_cap_instance", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->setting.total_num_capture_instance = val;
        LOG_PRINTF("total_cap_instance:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("net_receiver", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->setting.net_receiver_scheduler_number = val;
        LOG_PRINTF("net_receiver:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("net_sender", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->setting.net_sender_scheduler_number = val;
        LOG_PRINTF("net_sender:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("io_reader", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->setting.fileio_reader_scheduler_number = val;
        LOG_PRINTF("io_reader:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("io_writer", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->setting.fileio_writer_scheduler_number = val;
        LOG_PRINTF("io_writer:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("record_strategy", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->setting.record_strategy = val;
        LOG_PRINTF("record_strategy:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("record_autocut_condition", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->setting.record_autocut_condition = val;
        LOG_PRINTF("record_autocut_condition:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("record_autocut_naming", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->setting.record_autocut_naming = val;
        LOG_PRINTF("record_autocut_naming:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("record_autocut_framecount", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->setting.record_autocut_framecount = val;
        LOG_PRINTF("record_autocut_framecount:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("record_autocut_duration", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->setting.record_autocut_duration = val;
        LOG_PRINTF("record_autocut_duration:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("pb_rtsp_tcp_mode", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->setting.rtsp_client_try_tcp_mode_first = val;
        LOG_PRINTF("pb_rtsp_tcp_mode:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("rtsp_enable_rtcp", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->setting.rtsp_enable_rtcp = val;
        LOG_PRINTF("rtsp_enable_rtcp:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("local_rtsp_port", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->setting.local_rtsp_port = val;
        LOG_PRINTF("local_rtsp_port:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("cloud_im_port", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->setting.im_server_port = val;
        LOG_PRINTF("cloud_im_port:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("cloud_upload_port", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->setting.data_server_port = val;
        LOG_PRINTF("cloud_upload_port:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("cloud_rtsp_port", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->setting.data_server_rtsp_port = val;
        LOG_PRINTF("cloud_rtsp_port:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("cloud_server_url", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        if (read_string[0]) {
            snprintf(p_content->setting.cloud_server_url, DMaxUrlLen, "%s", read_string);
        } else {
            snprintf(p_content->setting.cloud_server_url, DMaxUrlLen, "%s", "127.0.0.1");
        }
        LOG_PRINTF("cloud_server_url: %s\n", p_content->setting.cloud_server_url);
    }

    err = api->GetContentValue("prefer_demuxer", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        if (read_string[0]) {
            snprintf(p_content->setting.prefer_string_demuxer, DMaxPreferStringLen, "%s", read_string);
        } else {
            snprintf(p_content->setting.prefer_string_demuxer, DMaxPreferStringLen, "%s", DNonePerferString);
        }
        LOG_PRINTF("prefer_demuxer: %s\n", p_content->setting.prefer_string_demuxer);
    }

    err = api->GetContentValue("prefer_muxer", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        if (read_string[0]) {
            snprintf(p_content->setting.prefer_string_muxer, DMaxPreferStringLen, "%s", read_string);
        } else {
            snprintf(p_content->setting.prefer_string_muxer, DMaxPreferStringLen, "%s", DNonePerferString);
        }
        LOG_PRINTF("prefer_muxer: %s\n", p_content->setting.prefer_string_muxer);
    }

    err = api->GetContentValue("prefer_vd", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        if (read_string[0]) {
            snprintf(p_content->setting.prefer_string_video_decoder, DMaxPreferStringLen, "%s", read_string);
        } else {
            snprintf(p_content->setting.prefer_string_video_decoder, DMaxPreferStringLen, "%s", DNonePerferString);
        }
        LOG_PRINTF("prefer_vd: %s\n", p_content->setting.prefer_string_video_decoder);
    }

    err = api->GetContentValue("prefer_ad", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        if (read_string[0]) {
            snprintf(p_content->setting.prefer_string_audio_decoder, DMaxPreferStringLen, "%s", read_string);
        } else {
            snprintf(p_content->setting.prefer_string_audio_decoder, DMaxPreferStringLen, "%s", DNonePerferString);
        }
        LOG_PRINTF("prefer_ad: %s\n", p_content->setting.prefer_string_audio_decoder);
    }

    err = api->GetContentValue("prefer_ve", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        if (read_string[0]) {
            snprintf(p_content->setting.prefer_string_video_encoder, DMaxPreferStringLen, "%s", read_string);
        } else {
            snprintf(p_content->setting.prefer_string_video_encoder, DMaxPreferStringLen, "%s", DNonePerferString);
        }
        LOG_PRINTF("prefer_ve: %s\n", p_content->setting.prefer_string_video_encoder);
    }

    err = api->GetContentValue("prefer_ae", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        if (read_string[0]) {
            snprintf(p_content->setting.prefer_string_audio_encoder, DMaxPreferStringLen, "%s", read_string);
        } else {
            snprintf(p_content->setting.prefer_string_audio_encoder, DMaxPreferStringLen, "%s", DNonePerferString);
        }
        LOG_PRINTF("prefer_ae: %s\n", p_content->setting.prefer_string_audio_encoder);
    }

    err = api->GetContentValue("prefer_vc", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        if (read_string[0]) {
            snprintf(p_content->setting.prefer_string_video_capture, DMaxPreferStringLen, "%s", read_string);
        } else {
            snprintf(p_content->setting.prefer_string_video_capture, DMaxPreferStringLen, "%s", DNonePerferString);
        }
        LOG_PRINTF("prefer_vc: %s\n", p_content->setting.prefer_string_video_capture);
    }

    err = api->GetContentValue("prefer_ac", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        if (read_string[0]) {
            snprintf(p_content->setting.prefer_string_audio_capture, DMaxPreferStringLen, "%s", read_string);
        } else {
            snprintf(p_content->setting.prefer_string_audio_capture, DMaxPreferStringLen, "%s", DNonePerferString);
        }
        LOG_PRINTF("prefer_ac: %s\n", p_content->setting.prefer_string_audio_capture);
    }

    err = api->GetContentValue("b_constrain_latency", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->setting.b_constrain_latency = val;
        LOG_PRINTF("b_constrain_latency:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("pb_precached_video_frames", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->setting.pb_precached_video_frames = val;
        LOG_PRINTF("pb_precached_video_frames:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("pb_max_video_frames", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->setting.pb_max_video_frames = val;
        LOG_PRINTF("pb_max_video_frames:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("b_render_video_nodelay", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->setting.b_render_video_nodelay = val;
        LOG_PRINTF("b_render_video_nodelay:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("url_cap_streaming", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        if (read_string[0]) {
            snprintf(p_content->setting.cap_local_streaming_url[0], DMAX_URL_LENGTH, "%s", read_string);
        } else {
            snprintf(p_content->setting.cap_local_streaming_url[0], DMAX_URL_LENGTH, "%s", "local");
        }
        LOG_PRINTF("url_cap_streaming: %s\n", p_content->setting.cap_local_streaming_url[0]);
    }

    err = api->GetContentValue("url_cap_storage", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        if (read_string[0]) {
            snprintf(p_content->setting.cap_sink_url[0], DMAX_URL_LENGTH, "%s", read_string);
        } else {
            snprintf(p_content->setting.cap_sink_url[0], DMAX_URL_LENGTH, "%s", "local");
        }
        LOG_PRINTF("url_cap_storage: %s\n", p_content->setting.cap_sink_url[0]);
    }

    err = api->GetContentValue("audio_enc_bitrate", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->audio_enc_setting.bitrate = val;
        LOG_PRINTF("audio_enc_bitrate:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("video_cap_width", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->video_cap_setting.width = val;
        LOG_PRINTF("video_cap_width:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("video_cap_height", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->video_cap_setting.height = val;
        LOG_PRINTF("video_cap_height:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("video_cap_offsetx", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->video_cap_setting.offset_x = val;
        LOG_PRINTF("video_cap_offsetx:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("video_cap_offsety", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->video_cap_setting.offset_y = val;
        LOG_PRINTF("video_cap_offsety:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("video_enc_width", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->video_enc_setting.width = val;
        LOG_PRINTF("video_enc_width:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("video_enc_height", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->video_enc_setting.height = val;
        LOG_PRINTF("video_enc_height:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("video_enc_fr_num", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->video_enc_setting.framerate_num = val;
        LOG_PRINTF("video_enc_fr_num:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("video_enc_fr_den", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->video_enc_setting.framerate_den = val;
        LOG_PRINTF("video_enc_fr_den:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("video_enc_bitrate", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->video_enc_setting.bitrate = val;
        LOG_PRINTF("video_enc_bitrate:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("sound_output_buffer_count", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->common_config.sound_output.sound_output_buffer_count = val;
        LOG_PRINTF("sound_output_buffer_count:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("sound_output_precache_count", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->common_config.sound_output.sound_output_precache_count = val;
        LOG_PRINTF("sound_output_precache_count:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("dump_debug_sound_input_pcm", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->common_config.debug_dump.dump_debug_sound_input_pcm = val;
        LOG_PRINTF("dump_debug_sound_input_pcm:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("dump_debug_sound_output_pcm", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->common_config.debug_dump.dump_debug_sound_output_pcm = val;
        LOG_PRINTF("dump_debug_sound_output_pcm:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("debug_print_video_realtime", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->common_config.debug_dump.debug_print_video_realtime = val;
        LOG_PRINTF("debug_print_video_realtime:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("debug_print_audio_realtime", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        sscanf(read_string, "%d", &val);
        p_content->common_config.debug_dump.debug_print_audio_realtime = val;
        LOG_PRINTF("debug_print_audio_realtime:%s, %d\n", read_string, val);
    }

    err = api->GetContentValue("play_url", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        if (read_string[0]) {
            snprintf(p_content->setting.pb_source_url[0], DMaxUrlLen, "%s", read_string);
        } else {
            snprintf(p_content->setting.pb_source_url[0], DMaxUrlLen, "%s", "rtsp://127.0.0.1/local");
        }
        LOG_PRINTF("play_url: %s\n", p_content->setting.pb_source_url[0]);
    }

    err = api->GetContentValue("url_pb_storage", (TChar *)read_string);
    if (DLikely(EECode_OK == err)) {
        if (read_string[0]) {
            snprintf(p_content->setting.pb_sink_url[0], DMaxUrlLen, "%s", read_string);
        } else {
            snprintf(p_content->setting.pb_sink_url[0], DMaxUrlLen, "%s", "testpb.mp4");
        }
        LOG_PRINTF("url_pb_storage: %s\n", p_content->setting.pb_sink_url[0]);
    }

    err = api->CloseConfigFile();
    DASSERT_OK(err);

    api->Destroy();

    return EECode_OK;
}

EECode gfMediaSimpleAPIPrintMediaConfig(SMediaSimpleAPIContext *p_content)
{
    if (DUnlikely(!p_content)) {
        LOG_ERROR("gfMediaSimpleAPIPrintMediaConfig: NULL params\n");
        return EECode_BadParam;
    }

    LOG_PRINTF("print current media config:\n");
    LOG_PRINTF("\t[config]: loglevel: %d, force %d\n", p_content->setting.force_log_level, p_content->setting.enable_force_log_level);
    LOG_PRINTF("\t[config]: enable_video: %d\n", p_content->setting.enable_video);
    LOG_PRINTF("\t[config]: enable_audio: %d\n", p_content->setting.enable_audio);
    LOG_PRINTF("\t[config]: enable_playback: %d\n", p_content->setting.enable_playback);
    LOG_PRINTF("\t[config]: enable_pb_storage: %d\n", p_content->setting.enable_playback_local_storage);
    LOG_PRINTF("\t[config]: enable_capture: %d\n", p_content->setting.enable_capture);
    LOG_PRINTF("\t[config]: enable_cap_streaming: %d\n", p_content->setting.enable_capture_streaming_out);
    LOG_PRINTF("\t[config]: enable_cap_storage: %d\n", p_content->setting.enable_capture_local_storage);
    LOG_PRINTF("\t[config]: enable_cap_uploading: %d\n", p_content->setting.enable_capture_cloud_uploading);
    LOG_PRINTF("\t[config]: total_pb_instance: %d\n", p_content->setting.total_num_playback_instance);
    LOG_PRINTF("\t[config]: total_cap_instance: %d\n", p_content->setting.total_num_capture_instance);
    LOG_PRINTF("\t[config]: net_receiver: %d\n", p_content->setting.net_receiver_scheduler_number);
    LOG_PRINTF("\t[config]: net_sender: %d\n", p_content->setting.net_sender_scheduler_number);
    LOG_PRINTF("\t[config]: io_reader: %d\n", p_content->setting.fileio_reader_scheduler_number);
    LOG_PRINTF("\t[config]: io_writer: %d\n", p_content->setting.fileio_writer_scheduler_number);
    LOG_PRINTF("\t[config]: record_strategy: %d\n", p_content->setting.record_strategy);
    LOG_PRINTF("\t[config]: record_autocut_condition: %d\n", p_content->setting.record_autocut_condition);
    LOG_PRINTF("\t[config]: record_autocut_naming: %d\n", p_content->setting.record_autocut_naming);
    LOG_PRINTF("\t[config]: record_autocut_framecount: %d\n", p_content->setting.record_autocut_framecount);
    LOG_PRINTF("\t[config]: record_autocut_duration: %d\n", p_content->setting.record_autocut_duration);
    LOG_PRINTF("\t[config]: pb_rtsp_tcp_mode: %d\n", p_content->setting.rtsp_client_try_tcp_mode_first);
    LOG_PRINTF("\t[config]: rtsp_enable_rtcp: %d\n", p_content->setting.rtsp_enable_rtcp);
    LOG_PRINTF("\t[config]: local_rtsp_port: %d\n", p_content->setting.local_rtsp_port);
    LOG_PRINTF("\t[config]: cloud_im_port: %d\n", p_content->setting.im_server_port);
    LOG_PRINTF("\t[config]: data_server_port: %d\n", p_content->setting.data_server_port);
    LOG_PRINTF("\t[config]: cloud_rtsp_port: %d\n", p_content->setting.data_server_rtsp_port);
    LOG_PRINTF("\t[config]: cloud_server_url: %s\n", p_content->setting.cloud_server_url);
    LOG_PRINTF("\t[config]: prefer_demuxer: %s\n", p_content->setting.prefer_string_demuxer);
    LOG_PRINTF("\t[config]: prefer_muxer: %s\n", p_content->setting.prefer_string_muxer);
    LOG_PRINTF("\t[config]: prefer_vd: %s\n", p_content->setting.prefer_string_video_decoder);
    LOG_PRINTF("\t[config]: prefer_ad: %s\n", p_content->setting.prefer_string_audio_decoder);
    LOG_PRINTF("\t[config]: prefer_ve: %s\n", p_content->setting.prefer_string_video_encoder);
    LOG_PRINTF("\t[config]: prefer_ae: %s\n", p_content->setting.prefer_string_audio_encoder);
    LOG_PRINTF("\t[config]: prefer_vc: %s\n", p_content->setting.prefer_string_video_capture);
    LOG_PRINTF("\t[config]: prefer_ac: %s\n", p_content->setting.prefer_string_audio_capture);
    LOG_PRINTF("\t[config]: b_constrain_latency: %d\n", p_content->setting.b_constrain_latency);
    LOG_PRINTF("\t[config]: pb_precached_video_frames: %d\n", p_content->setting.pb_precached_video_frames);
    LOG_PRINTF("\t[config]: pb_max_video_frames: %d\n", p_content->setting.pb_max_video_frames);
    LOG_PRINTF("\t[config]: b_render_video_nodelay: %d\n", p_content->setting.b_render_video_nodelay);
    LOG_PRINTF("\t[config]: url_cap_streaming: %s\n", p_content->setting.cap_local_streaming_url[0]);
    LOG_PRINTF("\t[config]: url_cap_storage: %s\n", p_content->setting.cap_sink_url[0]);
    LOG_PRINTF("\t[config]: audio_enc_bitrate: %d\n", p_content->audio_enc_setting.bitrate);
    LOG_PRINTF("\t[config]: video_cap_width: %d\n", p_content->video_cap_setting.width);
    LOG_PRINTF("\t[config]: video_cap_height: %d\n", p_content->video_cap_setting.height);
    LOG_PRINTF("\t[config]: video_cap_offsetx: %d\n", p_content->video_cap_setting.offset_x);
    LOG_PRINTF("\t[config]: video_cap_offsety: %d\n", p_content->video_cap_setting.offset_y);
    LOG_PRINTF("\t[config]: video_enc_width: %d\n", p_content->video_enc_setting.width);
    LOG_PRINTF("\t[config]: video_enc_height: %d\n", p_content->video_enc_setting.height);
    LOG_PRINTF("\t[config]: video_enc_fr_num: %d\n", p_content->video_enc_setting.framerate_num);
    LOG_PRINTF("\t[config]: video_enc_fr_den: %d\n", p_content->video_enc_setting.framerate_den);
    LOG_PRINTF("\t[config]: video_enc_bitrate: %d\n", p_content->video_enc_setting.bitrate);
    LOG_PRINTF("\t[config]: sound_output_buffer_count: %d\n", p_content->common_config.sound_output.sound_output_buffer_count);
    LOG_PRINTF("\t[config]: sound_output_precache_count: %d\n", p_content->common_config.sound_output.sound_output_precache_count);
    LOG_PRINTF("\t[config]: dump_debug_sound_input_pcm: %d\n", p_content->common_config.debug_dump.dump_debug_sound_input_pcm);
    LOG_PRINTF("\t[config]: dump_debug_sound_output_pcm: %d\n", p_content->common_config.debug_dump.dump_debug_sound_output_pcm);
    LOG_PRINTF("\t[config]: debug_print_video_realtime: %d\n", p_content->common_config.debug_dump.debug_print_video_realtime);
    LOG_PRINTF("\t[config]: debug_print_audio_realtime: %d\n", p_content->common_config.debug_dump.debug_print_audio_realtime);
    LOG_PRINTF("\t[config]: play_url: %s\n", p_content->setting.pb_source_url[0]);
    LOG_PRINTF("\t[config]: url_pb_storage: %s\n", p_content->setting.pb_sink_url[0]);

    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END


