/*******************************************************************************
 * simple_api.cpp
 *
 * History:
 *    2014/03/14 - [Zhi He] create file
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

#include "common_base.h"

#include "cloud_lib_if.h"
#include "sacp_types.h"

#include "media_mw_if.h"

#include "push_cloud_simple_api.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

CSimpleAPIInPushMode::CSimpleAPIInPushMode(TUint direct_access)
    : mpEngineControl(NULL)
    , mpContext(NULL)
    , mpMediaConfig(NULL)
    , mbStartRunning(0)
    , mbBuildGraghDone(0)
    , mbAssignContext(0)
    , mbDirectAccess(direct_access)
{
}

CSimpleAPIInPushMode::~CSimpleAPIInPushMode()
{
}

EECode CSimpleAPIInPushMode::Construct()
{
    mpEngineControl = gfMediaEngineFactoryV2(EFactory_MediaEngineType_Generic);
    if (DUnlikely(!mpEngineControl)) {
        LOG_FATAL("gfMediaEngineFactoryV2 fail\n");
        return EECode_NoMemory;
    }

    EECode err = mpEngineControl->SetAppMsgCallback(MsgCallback, (void *) this);
    if (DUnlikely(EECode_OK != err)) {
        LOG_FATAL("SetAppMsgCallback fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    return EECode_OK;
}

CSimpleAPIInPushMode *CSimpleAPIInPushMode::Create(TUint direct_access)
{
    CSimpleAPIInPushMode *thiz = new CSimpleAPIInPushMode(direct_access);
    if (DLikely(thiz)) {
        EECode err = thiz->Construct();
        if (DLikely(EECode_OK == err)) {
            return thiz;
        }
        delete thiz;
    }

    return NULL;
}

void CSimpleAPIInPushMode::Destroy()
{
    if (mpEngineControl) {
        mpEngineControl->Destroy();
        mpEngineControl = NULL;
    }

    this->~CSimpleAPIInPushMode();
}

IGenericEngineControlV2 *CSimpleAPIInPushMode::QueryEngineControl() const
{
    return mpEngineControl;
}

EECode CSimpleAPIInPushMode::AssignContext(SSimpleAPIContxtInPushMode *p_context)
{
    if (DUnlikely(!p_context)) {
        LOG_FATAL("NULL p_context\n");
        return EECode_BadParam;
    }

    if (DUnlikely(mbStartRunning || mbBuildGraghDone)) {
        LOG_FATAL("start running, or build gragh done, cannot invoke this API at this time\n");
        return EECode_BadState;
    }

    if (DUnlikely(mpContext || mbAssignContext)) {
        LOG_FATAL("mpContext %p, mbAssignContext %d, already set?\n", mpContext, mbAssignContext);
        return EECode_BadState;
    }

    if (mbDirectAccess) {
        mpContext = p_context;
    } else {
        mpContext = (SSimpleAPIContxtInPushMode *) malloc(sizeof(SSimpleAPIContxtInPushMode));
        if (!mpContext) {
            LOG_FATAL("No memory, request size %lu\n", (TULong)sizeof(SSimpleAPIContxtInPushMode));
            return EECode_NoMemory;
        }
        memset(mpContext, 0x0, sizeof(SSimpleAPIContxtInPushMode));
        *mpContext = *p_context;
    }
    mbAssignContext = 1;

    return EECode_OK;
}

EECode CSimpleAPIInPushMode::StartRunning()
{
    DASSERT(mpEngineControl);
    DASSERT(mpContext);
    EECode err = EECode_OK;

    if (!mbBuildGraghDone) {
        if (DUnlikely((!mbAssignContext) || (!mpContext))) {
            LOG_ERROR("need invoke AssignContext first\n");
            return EECode_BadState;
        }

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

    } else {
        DASSERT(mbAssignContext);
        DASSERT(mpContext);
    }

    if (DLikely(mpEngineControl)) {
        err = mpEngineControl->StartProcess();
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("StartProcess return %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
        mbStartRunning = 1;

    } else {
        LOG_ERROR("NULL mpEngineControl\n");
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CSimpleAPIInPushMode::ExitRunning()
{
    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    if (DLikely(mpEngineControl && mbStartRunning)) {
        mbStartRunning = 0;
        EECode err = mpEngineControl->StopProcess();
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("StopProcess return %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }

    } else {
        LOG_ERROR("NULL mpEngineControl %p, or not started %d yet\n", mpEngineControl, mbStartRunning);
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CSimpleAPIInPushMode::Control(EUserParamType type, void *param)
{
    EECode err = EECode_OK;

    switch (type) {

        default:
            LOG_FATAL("unknown user cmd type %d\n", type);
            return EECode_NotSupported;
            break;
    }

    return err;
}

void CSimpleAPIInPushMode::MsgCallback(void *context, SMSG &msg)
{
    if (DUnlikely(!context)) {
        LOG_FATAL("NULL context\n");
        return;
    }

    CSimpleAPIInPushMode *thiz = (CSimpleAPIInPushMode *) context;
    EECode err = thiz->ProcessMsg(msg);

    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("ProcessMsg(msg) return %d, %s\n", err, gfGetErrorCodeString(err));
    }

    return;
}

EECode CSimpleAPIInPushMode::ProcessMsg(SMSG &msg)
{
    switch (msg.code) {

            //internal msg
        case EMSGType_StorageError:
            LOG_ERROR("EMSGType_StorageError TO DO\n");
            break;

        case EMSGType_SystemError:
            LOG_ERROR("EMSGType_SystemError TO DO\n");
            break;

        case EMSGType_IOError:
            LOG_ERROR("EMSGType_IOError TO DO\n");
            break;

        case EMSGType_DeviceError:
            LOG_ERROR("EMSGType_DeviceError TO DO\n");
            break;

        case EMSGType_StreamingError_TCPSocketConnectionClose:
            LOG_ERROR("EMSGType_StreamingError_TCPSocketConnectionClose TO DO\n");
            break;

        case EMSGType_StreamingError_UDPSocketInvalidArgument:
            LOG_ERROR("EMSGType_StreamingError_UDPSocketInvalidArgument TO DO\n");
            break;

        case EMSGType_NotifyNewFileGenerated:
            LOG_ERROR("EMSGType_NotifyNewFileGenerated TO DO\n");
            break;

        case ECMDType_CloudSinkClient_UpdateFrameRate:
            break;

        case ECMDType_CloudClient_PeerClose:
            break;

        default:
            LOG_ERROR("unknown msg code %d, 0x%x\n", msg.code, msg.code);
            return EECode_NotSupported;
            break;
    }

    return EECode_OK;
}

EECode CSimpleAPIInPushMode::checkSetting()
{
    if (DUnlikely(DSYSTEM_MAX_CHANNEL_NUM < mpContext->setting.max_push_channel_number)) {
        LOG_ERROR("mpContext->setting.max_push_channel_number %d excced max %d\n", mpContext->setting.max_push_channel_number, DSYSTEM_MAX_CHANNEL_NUM);
        return EECode_BadParam;
    }
    DASSERT(mpContext->setting.max_push_channel_number);

    if (DUnlikely(DSYSTEM_MAX_CHANNEL_NUM < mpContext->setting.max_remote_control_channel_number)) {
        LOG_ERROR("mpContext->setting.max_remote_control_channel_number %d excced max %d\n", mpContext->setting.max_remote_control_channel_number, DSYSTEM_MAX_CHANNEL_NUM);
        return EECode_BadParam;
    }
    //DASSERT(mpContext->setting.max_remote_control_channel_number);

    if (DUnlikely(DMaxStreamingTransmiterNumberInPushMode < mpContext->setting.max_streaming_transmiter_number)) {
        LOG_ERROR("mpContext->setting.max_streaming_transmiter_number %d excced max %d\n", mpContext->setting.max_streaming_transmiter_number, DMaxStreamingTransmiterNumberInPushMode);
        return EECode_BadParam;
    }
    DASSERT(mpContext->setting.max_streaming_transmiter_number);

    if (DUnlikely(DMinChannelNumberPerStreamingTransmiterInPushMode > mpContext->setting.channel_number_per_transmiter)) {
        LOG_ERROR("mpContext->setting.channel_number_per_transmiter %d less than min %d\n", mpContext->setting.channel_number_per_transmiter, DMinChannelNumberPerStreamingTransmiterInPushMode);
        return EECode_BadParam;
    }

    if (DUnlikely(mpContext->setting.max_push_channel_number > (mpContext->setting.channel_number_per_transmiter * mpContext->setting.max_streaming_transmiter_number))) {
        LOG_ERROR("streaming transmiter number not enough %dx%d < %d\n", mpContext->setting.channel_number_per_transmiter, mpContext->setting.max_streaming_transmiter_number, mpContext->setting.max_push_channel_number);
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CSimpleAPIInPushMode::initialSetting()
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

    if (mpContext->setting.net_receiver_tcp_scheduler_number) {
        pre_config.check_field = EGenericEnginePreConfigure_NetworkTCPReceiverSchedulerNumber;
        pre_config.number = mpContext->setting.net_receiver_tcp_scheduler_number;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_NetworkTCPReceiverSchedulerNumber, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_NetworkTCPReceiverSchedulerNumber, %d), err %d, %s\n", mpContext->setting.net_receiver_tcp_scheduler_number, err, gfGetErrorCodeString(err));
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

    if (mpContext->setting.rtsp_client_try_tcp_mode_first) {
        pre_config.check_field = EGenericEnginePreConfigure_RTSPClientTryTCPModeFirst;
        pre_config.number = mpContext->setting.rtsp_client_try_tcp_mode_first;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_RTSPClientTryTCPModeFirst, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_RTSPClientTryTCPModeFirst, %d), err %d, %s\n", mpContext->setting.net_receiver_scheduler_number, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    pre_config.check_field = EGenericEnginePreConfigure_RTSPClientParseMultipleAccessUnit;
    pre_config.number = mpContext->setting.parse_multiple_access_unit;
    err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_RTSPClientParseMultipleAccessUnit, &pre_config);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("EGenericEnginePreConfigure_RTSPClientParseMultipleAccessUnit(%d), err %d, %s\n", mpContext->setting.parse_multiple_access_unit, err, gfGetErrorCodeString(err));
        return err;
    }

    if (mpContext->setting.rtsp_server_port) {
        pre_config.check_field = EGenericEnginePreConfigure_RTSPStreamingServerPort;
        pre_config.number = mpContext->setting.rtsp_server_port;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_RTSPStreamingServerPort, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_RTSPStreamingServerPort, %d), err %d, %s\n", mpContext->setting.rtsp_server_port, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.cloud_server_port) {
        pre_config.check_field = EGenericEnginePreConfigure_SACPCloudServerPort;
        pre_config.number = mpContext->setting.cloud_server_port;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_SACPCloudServerPort, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_SACPCloudServerPort, %d), err %d, %s\n", mpContext->setting.cloud_server_port, err, gfGetErrorCodeString(err));
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

    if (mpContext->setting.debug_prefer_ffmpeg_muxer) {
        pre_config.check_field = EGenericEnginePreConfigure_PreferModule_Muxer_FFMpeg;
        pre_config.number = 0;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_Muxer_FFMpeg, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_Muxer_FFMpeg), err %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.debug_prefer_private_ts_muxer) {
        pre_config.check_field = EGenericEnginePreConfigure_PreferModule_Muxer_TS;
        pre_config.number = 0;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_Muxer_TS, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_Muxer_TS), err %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.debug_prefer_ffmpeg_audio_decoder) {
        pre_config.check_field = EGenericEnginePreConfigure_PreferModule_AudioDecoder_FFMpeg;
        pre_config.number = 0;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_AudioDecoder_FFMpeg, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_AudioDecoder_FFMpeg), err %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.debug_prefer_aac_audio_decoder) {
        pre_config.check_field = EGenericEnginePreConfigure_PreferModule_AudioDecoder_AAC;
        pre_config.number = 0;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_AudioDecoder_AAC, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_AudioDecoder_AAC), err %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.debug_prefer_aac_audio_encoder) {
        pre_config.check_field = EGenericEnginePreConfigure_PreferModule_AudioEncoder_AAC;
        pre_config.number = 0;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_AudioEncoder_AAC, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_AudioEncoder_AAC), err %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.debug_prefer_customized_adpcm_encoder) {
        pre_config.check_field = EGenericEnginePreConfigure_PreferModule_AudioEncoder_Customized_ADPCM;
        pre_config.number = 0;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_AudioEncoder_Customized_ADPCM, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_AudioEncoder_Customized_ADPCM), err %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.debug_prefer_customized_adpcm_decoder) {
        pre_config.check_field = EGenericEnginePreConfigure_PreferModule_AudioDecoder_Customized_ADPCM;
        pre_config.number = 0;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_AudioDecoder_Customized_ADPCM, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_AudioDecoder_Customized_ADPCM), err %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    }

    //direct setting
    mpEngineControl->GetMediaConfig(mpMediaConfig);
    if (DLikely(mpMediaConfig)) {
        mpMediaConfig->network_config.mbUseTCPPulseSender = mpContext->setting.pulse_sender_number;
        mpMediaConfig->number_tcp_pulse_sender = mpContext->setting.pulse_sender_number;
        mpMediaConfig->network_config.mPulseTransferMaxFramecount = mpContext->setting.pulse_sender_framecount;
        mpMediaConfig->network_config.mPulseTransferMemSize = mpContext->setting.pulse_sender_memsize;
    } else {
        LOG_FATAL("NULL mpMediaConfig\n");
    }

    return EECode_OK;
}

EECode CSimpleAPIInPushMode::createGlobalComponents()
{
    EECode err = EECode_OK;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_PRINTF("[CSimpleAPIInPushMode]: createGlobalComponents begin\n");

    //create streaming server and data transmiter
    DASSERT(mpContext->setting.enable_streaming_server);
    mpContext->setting.enable_streaming_server = 1;
    if (mpContext->setting.enable_streaming_server) {
        err = mpEngineControl->NewComponent(EGenericComponentType_StreamingServer, mpContext->streaming_server_id);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("NewComponent(EGenericComponentType_StreamingServer) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
            return err;
        }
    }

    //create cloud server and remote control connecter
    DASSERT(mpContext->setting.enable_cloud_server);
    mpContext->setting.enable_cloud_server = 1;
    if (mpContext->setting.enable_cloud_server) {
        //cloud server
        err = mpEngineControl->NewComponent(EGenericComponentType_CloudServer, mpContext->cloud_server_id);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("NewComponent(EGenericComponentType_CloudServer) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
            return err;
        }
    }

    LOG_PRINTF("[CSimpleAPIInPushMode]: createGlobalComponents end, mpContext->cloud_server_id %08x\n", mpContext->cloud_server_id);

    return EECode_OK;
}

EECode CSimpleAPIInPushMode::createGroupComponents()
{
    EECode err = EECode_OK;
    TUint i = 0;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_PRINTF("[CSimpleAPIInPushMode]: createGroupComponents() begin\n");

    //create cloud agent
    if (mpContext->setting.enable_cloud_server) {
        for (i = 0; i < mpContext->setting.max_push_channel_number; i ++) {
            err = mpEngineControl->NewComponent(EGenericComponentType_CloudConnecterServerAgent, mpContext->gragh_component.cloud_agent_id[i]);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_CloudConnecterServerAgent(%d)) fail, ret %d, %s.\n", i, err, gfGetErrorCodeString(err));
                return err;
            }

#if 0
            err = mpEngineControl->SetCloudAgentTag(mpContext->gragh_component.cloud_agent_id[i], NULL, mpContext->cloud_server_id);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("SetCloudAgentTag(%d) fail, ret %d, %s.\n", i, err, gfGetErrorCodeString(err));
                return err;
            }
#endif

            if (mpContext->setting.enable_push_path) {
                err = mpEngineControl->NewComponent(EGenericComponentType_CloudConnecterCmdAgent, mpContext->gragh_component.cloud_agent_cmd_id[i]);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("NewComponent(EGenericComponentType_CloudConnecterCmdAgent(%d)) fail, ret %d, %s.\n", i, err, gfGetErrorCodeString(err));
                    return err;
                }
            }
#if 0
            err = mpEngineControl->SetCloudAgentTag(mpContext->gragh_component.cloud_agent_cmd_id[i], NULL, mpContext->cloud_server_id);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("SetCloudAgentTag(%d) fail, ret %d, %s.\n", i, err, gfGetErrorCodeString(err));
                return err;
            }
#endif

        }
    }

    //create muxers
    if (mpContext->setting.enable_recording) {
        for (i = 0; i < mpContext->setting.max_push_channel_number; i ++) {
            err = mpEngineControl->NewComponent(EGenericComponentType_Muxer, mpContext->gragh_component.muxer_id[i]);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_Muxer(%d)) fail, ret %d, %s.\n", i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    //create streaming transmiters
    if (mpContext->setting.enable_streaming_server) {
        for (i = 0; i < mpContext->setting.max_streaming_transmiter_number; i ++) {
            err = mpEngineControl->NewComponent(EGenericComponentType_StreamingTransmiter, mpContext->gragh_component.streaming_transmiter_id[i]);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_StreamingTransmiter(%d)) fail, ret %d, %s.\n", i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    //create cloud agent
    if (mpContext->setting.enable_cloud_server && mpContext->setting.enable_push_path) {
        for (i = 0; i < mpContext->setting.max_remote_control_channel_number; i ++) {
            err = mpEngineControl->NewComponent(EGenericComponentType_CloudConnecterServerAgent, mpContext->gragh_component.remote_control_agent_id[i]);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_CloudConnecterServerAgent(%d)) fail, ret %d, %s.\n", i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    err = mpEngineControl->NewComponent(EGenericComponentType_CloudConnecterServerAgent, mpContext->remote_control_agent_id);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("NewComponent(EGenericComponentType_CloudConnecterServerAgent) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = mpEngineControl->SetCloudAgentTag(mpContext->remote_control_agent_id, (TChar *)"remote_debug", mpContext->cloud_server_id);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("SetCloudAgentTag() fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
        return err;
    }

    LOG_PRINTF("[CSimpleAPIInPushMode]: createGroupComponents() end\n");

    return EECode_OK;
}

EECode CSimpleAPIInPushMode::connectRecordingComponents()
{
    EECode err = EECode_OK;
    TUint channel_index = 0;
    TGenericID connection_id;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_PRINTF("[CSimpleAPIInPushMode]: connectRecordingComponents() begin\n");

    if (DLikely(mpContext->setting.enable_recording)) {

        for (channel_index = 0; channel_index < mpContext->setting.max_push_channel_number; channel_index ++) {

            if (!mpContext->setting.disable_video_path) {
                err = mpEngineControl->ConnectComponent(connection_id, \
                                                        mpContext->gragh_component.cloud_agent_id[channel_index], \
                                                        mpContext->gragh_component.muxer_id[channel_index], \
                                                        StreamType_Video);

                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("ConnectComponent(cloud[%d] to muxer[%d]), video path, err %d, %s\n", channel_index, channel_index, err, gfGetErrorCodeString(err));
                    return err;
                }
            }

            if (!mpContext->setting.disable_audio_path) {
                err = mpEngineControl->ConnectComponent(connection_id, \
                                                        mpContext->gragh_component.cloud_agent_id[channel_index], \
                                                        mpContext->gragh_component.muxer_id[channel_index], \
                                                        StreamType_Audio);

                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("ConnectComponent(cloud[%d] to muxer[%d]), audio path, err %d, %s\n", channel_index, channel_index, err, gfGetErrorCodeString(err));
                    return err;
                }
            }
        }

    }

    LOG_PRINTF("[CSimpleAPIInPushMode]: connectRecordingComponents() end\n");

    return EECode_OK;
}


EECode CSimpleAPIInPushMode::connectStreamingComponents()
{
    EECode err = EECode_OK;
    TUint i = 0;
    TUint round = mpContext->setting.max_streaming_transmiter_number;
    TUint transmiter_index = 0;
    TGenericID connection_id;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);
    DASSERT(round);

    LOG_PRINTF("[CSimpleAPIInPushMode]: connectStreamingComponents() begin\n");

    if (DLikely(mpContext->setting.enable_streaming_server)) {

        for (i = 0; i < mpContext->setting.max_push_channel_number; i ++) {
            transmiter_index = i % round;

            if (!mpContext->setting.disable_video_path) {
                err = mpEngineControl->ConnectComponent(connection_id, \
                                                        mpContext->gragh_component.cloud_agent_id[i], \
                                                        mpContext->gragh_component.streaming_transmiter_id[transmiter_index], \
                                                        StreamType_Video);

                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("ConnectComponent(cloud[%d] to streaming_transmiter[%d]), video path, err %d, %s\n", i, transmiter_index, err, gfGetErrorCodeString(err));
                    return err;
                }
            }

            if (!mpContext->setting.disable_audio_path) {
                err = mpEngineControl->ConnectComponent(connection_id, \
                                                        mpContext->gragh_component.cloud_agent_id[i], \
                                                        mpContext->gragh_component.streaming_transmiter_id[transmiter_index], \
                                                        StreamType_Audio);

                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("ConnectComponent(cloud[%d] to streaming_transmiter[%d]), audio path, err %d, %s\n", i, transmiter_index, err, gfGetErrorCodeString(err));
                    return err;
                }
            }

        }

    } else {
        LOG_ERROR("mpContext->setting.enable_streaming_server is 0\n");
        return EECode_BadParam;
    }

    LOG_PRINTF("[CSimpleAPIInPushMode]: connectStreamingComponents() end\n");

    return EECode_OK;
}

EECode CSimpleAPIInPushMode::connectCloudComponents()
{
    if (!mpContext->setting.enable_push_path) {
        return EECode_OK;
    }

    EECode err = EECode_OK;
    TUint channel_index = 0;
    TGenericID connection_id;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_PRINTF("[CSimpleAPIInPushMode]: connectCloudComponents() begin\n");

    if (DLikely(mpContext->setting.enable_cloud_server)) {

        for (channel_index = 0; channel_index < mpContext->setting.max_remote_control_channel_number; channel_index ++) {

            // cloud channel, cmd channel
            err = mpEngineControl->ConnectComponent(connection_id, \
                                                    mpContext->gragh_component.remote_control_agent_id[channel_index], \
                                                    mpContext->gragh_component.cloud_agent_id[channel_index], \
                                                    StreamType_Cmd);

            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(remote control agent[%d] to cloud agent[%d]), cmd channel, err %d, %s\n", channel_index, channel_index, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    LOG_PRINTF("[CSimpleAPIInPushMode]: connectCloudComponents() end\n");

    return EECode_OK;
}

EECode CSimpleAPIInPushMode::setupRecordingPipelines()
{
    EECode err = EECode_OK;
    TUint channel_index = 0;
    TUint max = mpContext->setting.max_push_channel_number;
    TGenericID video_source_id, audio_source_id;


    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_NOTICE("[CSimpleAPIInPushMode]: setupRecordingPipelines begin, enable_recording %d\n", mpContext->setting.enable_recording);

    if (DLikely(mpContext->setting.enable_recording)) {

        for (channel_index = 0; channel_index < max; channel_index ++) {

            //LOG_NOTICE("[DEBUG]::enable_recording %d, sink_2rd_channel_number %d, sink_channel_number %d\n", mpContext->group[group_index].enable_recording, mpContext->group[group_index].sink_2rd_channel_number, mpContext->group[group_index].sink_channel_number);

            if (DUnlikely(mpContext->setting.disable_video_path)) {
                video_source_id = 0;
            } else {
                video_source_id = mpContext->gragh_component.cloud_agent_id[channel_index];
            }

            if (DUnlikely(mpContext->setting.disable_audio_path)) {
                audio_source_id = 0;
            } else {
                audio_source_id = mpContext->gragh_component.cloud_agent_id[channel_index];
            }

            err = mpEngineControl->SetupRecordingPipeline(mpContext->gragh_component.recording_pipeline_id[channel_index], \
                    video_source_id, \
                    audio_source_id, \
                    mpContext->gragh_component.muxer_id[channel_index]);

            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("SetupRecordingPipeline[%d], err %d, %s\n", channel_index, err, gfGetErrorCodeString(err));
                return err;
            }

        }

    } else {
        LOG_NOTICE("[CSimpleAPIInPushMode]: recording not enabled\n");
    }

    return EECode_OK;
}


EECode CSimpleAPIInPushMode::setupStreamingPipelines()
{
    EECode err = EECode_OK;
    TUint channel_index = 0;
    TUint max = mpContext->setting.max_push_channel_number;
    TGenericID video_source_id, audio_source_id;
    TUint round = mpContext->setting.max_streaming_transmiter_number;
    TUint transmiter_index = 0;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_NOTICE("[CSimpleAPIInPushMode]: setupStreamingPipelines begin, enable_streaming_server %d\n", mpContext->setting.enable_streaming_server);

    if (DLikely(mpContext->setting.enable_streaming_server)) {

        DASSERT(mpContext->streaming_server_id);

        for (channel_index = 0; channel_index < max; channel_index ++) {

            transmiter_index = channel_index % round;

            if (DUnlikely(mpContext->setting.disable_video_path)) {
                video_source_id = 0;
            } else {
                video_source_id = mpContext->gragh_component.cloud_agent_id[channel_index];
            }

            if (DUnlikely(mpContext->setting.disable_audio_path)) {
                audio_source_id = 0;
            } else {
                audio_source_id = mpContext->gragh_component.cloud_agent_id[channel_index];
            }

            err = mpEngineControl->SetupStreamingPipeline(mpContext->gragh_component.streaming_pipeline_id[channel_index], \
                    mpContext->gragh_component.streaming_transmiter_id[transmiter_index], \
                    mpContext->streaming_server_id, \
                    video_source_id, \
                    audio_source_id, \
                    0);

            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("SetupStreamingPipeline[%d], err %d, %s\n", channel_index, err, gfGetErrorCodeString(err));
                return err;
            }

        }

    } else {
        LOG_NOTICE("[CSimpleAPIInPushMode]: streaming not enabled\n");
    }

    return EECode_OK;
}

EECode CSimpleAPIInPushMode::setupCloudPipelines()
{
    if (!mpContext->setting.enable_push_path) {
        return EECode_OK;
    }

    EECode err = EECode_OK;
    TUint channel_index = 0;
    TUint max = mpContext->setting.max_remote_control_channel_number;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_NOTICE("[CSimpleAPIInPushMode]: setupCloudPipelines begin, enable_cloud_server %d\n", mpContext->setting.enable_cloud_server);

    if (DLikely(mpContext->setting.enable_cloud_server)) {

        for (channel_index = 0; channel_index < max; channel_index ++) {

            err = mpEngineControl->SetupCloudPipeline(mpContext->gragh_component.cloud_pipeline_id[channel_index], \
                    mpContext->gragh_component.remote_control_agent_id[channel_index], \
                    mpContext->cloud_server_id, \
                    0, \
                    0, \
                    mpContext->gragh_component.cloud_agent_id[channel_index], \
                    1);

            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("SetupCloudPipeline[%d], err %d, %s\n", channel_index, err, gfGetErrorCodeString(err));
                return err;
            }
        }

        err = mpEngineControl->SetupCloudPipeline(mpContext->remote_control_cloud_pipeline_id, \
                mpContext->remote_control_agent_id, \
                mpContext->cloud_server_id, \
                0, \
                0, \
                0, \
                1);

        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("SetupCloudPipeline[%d], err %d, %s\n", channel_index, err, gfGetErrorCodeString(err));
            return err;
        }

    } else {
        LOG_NOTICE("[CSimpleAPIInPushMode]: cloud server not enabled\n");
    }

    return EECode_OK;
}

EECode CSimpleAPIInPushMode::buildDataProcessingGragh()
{
    EECode err = EECode_OK;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_PRINTF("[CSimpleAPIInPushMode]: buildDataProcessingGragh begin\n");

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

    err = createGroupComponents();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("createGroupComponents(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = connectRecordingComponents();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("connectRecordingComponents(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = connectStreamingComponents();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("connectStreamingComponents(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = connectCloudComponents();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("connectCloudComponents(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = setupRecordingPipelines();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("setupRecordingPipelines(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = setupStreamingPipelines();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("setupStreamingPipelines(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = setupCloudPipelines();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("setupCloudPipelines(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
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

    LOG_PRINTF("[CSimpleAPIInPushMode]: buildDataProcessingGragh end\n");

    return EECode_OK;
}

EECode CSimpleAPIInPushMode::preSetUrls()
{
    EECode err = EECode_OK;
    TUint i = 0;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    if (mpContext->setting.enable_recording) {
        for (i = 0; i < mpContext->setting.max_push_channel_number; i ++) {
            if (mpContext->gragh_component.muxer_id[i]) {
                TChar *url = mpContext->group_url.sink_url[i];
                if (strlen(url)) {
                    err = mpEngineControl->SetSinkUrl(mpContext->gragh_component.muxer_id[i], url);
                    LOG_PRINTF("SetSinkUrl 0x%08x, %s, ret %d, %s\n", mpContext->gragh_component.muxer_id[i], url, err, gfGetErrorCodeString(err));
                } else {
                    //default url
                    snprintf(url, DMaxUrlLen, "muxing_%04d.ts", i);
                    err = mpEngineControl->SetSinkUrl(mpContext->gragh_component.muxer_id[i], url);
                    LOG_PRINTF("SetSinkUrl 0x%08x, %s, ret %d, %s\n", mpContext->gragh_component.muxer_id[i], url, err, gfGetErrorCodeString(err));
                }
            }
        }
    }

    //set streaming urls
    if (mpContext->setting.enable_streaming_server) {
        for (i = 0; i < mpContext->setting.max_push_channel_number; i ++) {
            if (mpContext->gragh_component.streaming_pipeline_id[i]) {
                TChar *url = mpContext->group_url.streaming_url[i];
                if (strlen(url)) {
                    err = mpEngineControl->SetStreamingUrl(mpContext->gragh_component.streaming_pipeline_id[i], url);
                    LOG_PRINTF("SetStreamingUrl 0x%08x, %s, ret %d, %s\n", mpContext->gragh_component.streaming_pipeline_id[i], url, err, gfGetErrorCodeString(err));
                } else {
                    //default tag
                    snprintf(url, DMaxUrlLen, "l%02d", i);
                    err = mpEngineControl->SetStreamingUrl(mpContext->gragh_component.streaming_pipeline_id[i], url);
                    LOG_PRINTF("SetStreamingUrl 0x%08x, %s, ret %d, %s\n", mpContext->gragh_component.streaming_pipeline_id[i], url, err, gfGetErrorCodeString(err));
                }
            }
        }
    }

    //cloud agent tag
    if (mpContext->setting.enable_cloud_server) {
        TChar *url = NULL;
        for (i = 0; i < mpContext->setting.max_push_channel_number; i ++) {
            if (mpContext->gragh_component.cloud_agent_id[i]) {
                url = mpContext->group_url.cloud_agent_tag[i];
                if (strlen(url)) {
                    err = mpEngineControl->SetCloudAgentTag(mpContext->gragh_component.cloud_agent_id[i], url, mpContext->cloud_server_id);
                    LOG_PRINTF("SetCloudAgentTag 0x%08x, %s, ret %d, %s\n", mpContext->gragh_component.cloud_agent_id[i], url, err, gfGetErrorCodeString(err));
                } else {
                    //default agent tag
                    snprintf(url, DMaxUrlLen, "n%d", i);
                    err = mpEngineControl->SetCloudAgentTag(mpContext->gragh_component.cloud_agent_id[i], url, mpContext->cloud_server_id);
                    LOG_PRINTF("SetCloudAgentTag 0x%08x, %s, ret %d, %s\n", mpContext->gragh_component.cloud_agent_id[i], url, err, gfGetErrorCodeString(err));
                }
            }

            if (mpContext->gragh_component.cloud_agent_cmd_id[i]) {
                TChar cmd_url[DMaxUrlLen] = {0};
                url = mpContext->group_url.cloud_agent_tag[i];
                if (strlen(url)) {
                    snprintf(cmd_url, DMaxUrlLen, "%s:cmd", url);
                    err = mpEngineControl->SetCloudAgentTag(mpContext->gragh_component.cloud_agent_cmd_id[i], cmd_url, mpContext->cloud_server_id);
                    LOG_PRINTF("SetCloudAgentTag 0x%08x, %s, ret %d, %s\n", mpContext->gragh_component.cloud_agent_cmd_id[i], cmd_url, err, gfGetErrorCodeString(err));
                } else {
                    //default agent tag
                    snprintf(cmd_url, DMaxUrlLen, "n%d:cmd", i);
                    err = mpEngineControl->SetCloudAgentTag(mpContext->gragh_component.cloud_agent_cmd_id[i], cmd_url, mpContext->cloud_server_id);
                    LOG_PRINTF("SetCloudAgentTag 0x%08x, %s, ret %d, %s\n", mpContext->gragh_component.cloud_agent_cmd_id[i], cmd_url, err, gfGetErrorCodeString(err));
                }
            }
        }
    }

    //cloud agent tag
    if (mpContext->setting.enable_cloud_server) {
        for (i = 0; i < mpContext->setting.max_remote_control_channel_number; i ++) {
            if (mpContext->gragh_component.remote_control_agent_id[i]) {
                err = mpEngineControl->SetCloudAgentTag(mpContext->gragh_component.remote_control_agent_id[i], (TChar *)DRemoteControlString, mpContext->cloud_server_id);
                LOG_PRINTF("SetCloudAgentTag 0x%08x, %s, ret %d, %s\n", mpContext->gragh_component.remote_control_agent_id[i], DRemoteControlString, err, gfGetErrorCodeString(err));
            }
        }
    }

    return err;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END



