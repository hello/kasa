/*******************************************************************************
 * vod_cloud_simple_api.cpp
 *
 * History:
 *    2014/06/11 - [Zhi He] create file
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

#include "cloud_lib_if.h"
#include "sacp_types.h"

#include "storage_lib_if.h"
#include "media_mw_if.h"

#include "vod_cloud_simple_api.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

CSimpleAPIInVodMode::CSimpleAPIInVodMode(TUint direct_access)
    : mpEngineControl(NULL)
    , mpStorageManager(NULL)
    , mChannelTotalCount(0)
    , mpContext(NULL)
    , mpMediaConfig(NULL)
    , mbStartRunning(0)
    , mbBuildGraghDone(0)
    , mbAssignContext(0)
    , mbDirectAccess(direct_access)
    , mpAdminServerPort(NULL)
    , mnCurPortSourceNum(0)
    , mnMaxPortSourceNum(DMaxCommunicationPortSourceNumber)
    , mChannelIndex(0)
{
}

CSimpleAPIInVodMode::~CSimpleAPIInVodMode()
{
}

EECode CSimpleAPIInVodMode::Construct()
{
    mpStorageManager = gfCreateStorageManagement(EStorageMnagementType_Simple);
    if (DUnlikely(!mpStorageManager)) {
        LOG_FATAL("gfCreateStorageManagement fail\n");
        return EECode_NoMemory;
    }
    mpEngineControl = gfMediaEngineFactoryV3(EFactory_MediaEngineType_Generic, mpStorageManager);
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

CSimpleAPIInVodMode *CSimpleAPIInVodMode::Create(TUint direct_access)
{
    CSimpleAPIInVodMode *thiz = new CSimpleAPIInVodMode(direct_access);
    if (DLikely(thiz)) {
        EECode err = thiz->Construct();
        if (DLikely(EECode_OK == err)) {
            return thiz;
        }
        delete thiz;
    }

    return NULL;
}

void CSimpleAPIInVodMode::Destroy()
{
    if (mpStorageManager) {
        TU32 channel_number;
        EECode err = mpStorageManager->SaveDataBase(channel_number);
        if (DUnlikely(err != EECode_OK)) {
            LOG_ERROR("storage manager, SaveDataBase fail!\n");
        }
        mpStorageManager->Delete();
        mpStorageManager = NULL;
    }
    if (mpEngineControl) {
        mpEngineControl->Destroy();
        mpEngineControl = NULL;
    }

    if (mpAdminServerPort) {
        mpAdminServerPort->Stop();
        mpAdminServerPort->Destroy();
        mpAdminServerPort = NULL;
    }

    this->~CSimpleAPIInVodMode();
}

IGenericEngineControlV3 *CSimpleAPIInVodMode::QueryEngineControl() const
{
    return mpEngineControl;
}

EECode CSimpleAPIInVodMode::AssignContext(SSimpleAPIContxtInVodMode *p_context)
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
        mpContext = (SSimpleAPIContxtInVodMode *) malloc(sizeof(SSimpleAPIContxtInVodMode));
        if (!mpContext) {
            LOG_FATAL("No memory, request size %lu\n", (TULong)sizeof(SSimpleAPIContxtInVodMode));
            return EECode_NoMemory;
        }
        memset(mpContext, 0x0, sizeof(SSimpleAPIContxtInVodMode));
        *mpContext = *p_context;
    }
    mbAssignContext = 1;

    return EECode_OK;
}

EECode CSimpleAPIInVodMode::StartRunning()
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

    if (mpContext->setting.enable_admin_server) {
        err = setupCommunicationServerPort();
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("setupCommunicationServerPort fail\n");
            return err;
        }
    }

    return EECode_OK;
}

EECode CSimpleAPIInVodMode::ExitRunning()
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

EECode CSimpleAPIInVodMode::Control(EUserParamType type, void *param)
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

void CSimpleAPIInVodMode::MsgCallback(void *context, SMSG &msg)
{
    if (DUnlikely(!context)) {
        LOG_FATAL("NULL context\n");
        return;
    }

    CSimpleAPIInVodMode *thiz = (CSimpleAPIInVodMode *) context;
    EECode err = thiz->ProcessMsg(msg);

    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("ProcessMsg(msg) return %d, %s\n", err, gfGetErrorCodeString(err));
    }

    return;
}

EECode CSimpleAPIInVodMode::ProcessMsg(SMSG &msg)
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

        case ECMDType_CloudSourceClient_UpdateAudioParams:
            break;

        default:
            LOG_ERROR("unknown msg code %d, 0x%x\n", msg.code, msg.code);
            return EECode_NotSupported;
            break;
    }

    return EECode_OK;
}

EECode CSimpleAPIInVodMode::AddPortSource(TChar *source, TChar *password, void *p_source, TChar *p_identification_string, void *p_context)
{
    DASSERT(mpContext->setting.enable_admin_server == 1);
    EECode err = mpAdminServerPort->AddSource(source, password, p_source, p_identification_string, p_context, mpPortSource[mnCurPortSourceNum]);
    if (DUnlikely(EECode_OK != err)) {
        LOG_FATAL("CSimpleAPIInVodMode::AddPortSource fail\n");
        return err;
    }

    mnCurPortSourceNum++;
    return EECode_OK;
}

EECode CSimpleAPIInVodMode::RemovePortSource(TU32 index)
{
    DASSERT(mpContext->setting.enable_admin_server == 1);
    if (DUnlikely((index >= mnCurPortSourceNum) || (mpPortSource[index] == NULL))) {
        LOG_FATAL("Bad parameters, index: %u, current port source num: %u\n", index, mnCurPortSourceNum);
        return EECode_BadParam;
    }

    EECode err = mpAdminServerPort->RemoveSource(mpPortSource[index]);
    if (DUnlikely(EECode_OK != err)) {
        LOG_FATAL("CSimpleAPIInVodMode::RemovePortSource fail\n");
        return err;
    }

    mpPortSource[index] = mpPortSource[mnCurPortSourceNum - 1];
    mpPortSource[mnCurPortSourceNum - 1] = NULL;
    mnCurPortSourceNum--;
    return EECode_OK;
}

EECode CSimpleAPIInVodMode::ProcessReadBuffer(TU8 *p_data, TU32 datasize, TU32 subtype)
{
    EECode err = EECode_OK;
    DASSERT(p_data);
    switch (subtype) {

        case ESACPAdminSubType_AssignDataServerSourceDevice: {
                TChar *p_name = NULL;
                ETLV16Type type = ETLV16Type_Invalid;
                TU16 length = 0;
                gfParseTLV16Struct(p_data, &type, &length, (void **)&p_name);
                DASSERT(type == ETLV16Type_String);

                err = updateUrls(mChannelIndex, p_name);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_FATAL("ProcessReadBuffer, AssignDataServerSourceDevice updateUrls fail, err %d, %s\n", err, gfGetErrorCodeString(err));
                    break;
                }
                //reply to im server, todo
            }
            break;

        case ESACPAdminSubType_RemoveDataServerSourceDevice: {
                TChar *p_name = NULL;
                ETLV16Type type = ETLV16Type_Invalid;
                TU16 length = 0;
                gfParseTLV16Struct(p_data, &type, &length, (void **)&p_name);
                DASSERT(type == ETLV16Type_String);

                err = removeChannel(p_name);

                //remove one channel
                //stop pipeline
                //clear content
                //remove storage
            }
            break;

        case ESACPAdminSubType_UpdateDeviceDynamicCode: {
                //name + dynamic_input
                TChar *p_name = NULL;
                ETLV16Type type[2] = {ETLV16Type_Invalid};
                TU16 length[2] = {0};
                TU32 dynamic_code = 0;
                void *value[2] = {NULL};
                value[1] = (void *)&dynamic_code;
                gfParseTLV16Struct(p_data, type, length, value, 2);
                DASSERT(type[0] == ETLV16Type_DeviceName);
                DASSERT(type[1] == ETLV16Type_DynamicCode);
                p_name = (TChar *)value[0];

                LOG_PRINTF("ESACPAdminSubType_UpdateDeviceDynamicCode, device %s, dynamic_code %u\n", p_name, dynamic_code);

                TU32 index = 0;
                if (DUnlikely(checkIsExist(p_name, index) != true)) {
                    err = EECode_NotExist;
                    LOG_FATAL("Data node no this channel: %s\n", p_name);
                    break;
                }

                err = updateDeviceDynamicCode(index, dynamic_code);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_FATAL("ESACPAdminSubType_UpdateDeviceDynamicCode: updateDeviceDynamicCode fail\n");
                    break;
                }
            }
            break;

        case ESACPAdminSubType_UpdateUserDynamicCode: {
                //name + dynamic_input
                void *value[3] = {NULL};
                ETLV16Type type[3] = {ETLV16Type_Invalid};
                TU16 length[3] = {0};
                TU32 dynamic_code = 0;
                value[2] = (void *)&dynamic_code;
                gfParseTLV16Struct(p_data, type, length, value, 3);
                DASSERT(type[0] == ETLV16Type_AccountName);
                DASSERT(type[1] == ETLV16Type_DeviceName);
                DASSERT(type[2] == ETLV16Type_DynamicCode);

                LOG_PRINTF("ESACPAdminSubType_UpdateUserDynamicCode, user %s, device %s, dynamic_code %u\n", (TChar *)value[0], (TChar *)value[1], dynamic_code);

                //add into one user list
                err = updateUserDynamicCode((const TChar *)value[0], (const TChar *)value[1], dynamic_code);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_FATAL("ESACPAdminSubType_UpdateUserDynamicCode: updateUserDynamicCode fail\n");
                    break;
                }
            }
            break;

        default:
            break;
    }

    return err;
}

EECode CSimpleAPIInVodMode::CommunicationClientReadCallBack(void *owner, TChar *source, void *p_source, void *p_port, TU8 *p_data, TU32 datasize, TU32 type, TU32 cat, TU32 subtype)
{
    CSimpleAPIInVodMode *thiz = (CSimpleAPIInVodMode *)owner;
    if (!thiz) {
        return EECode_BadParam;
    }

    thiz->ProcessReadBuffer(p_data, datasize, subtype);
    return EECode_OK;
}

EECode CSimpleAPIInVodMode::checkSetting()
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

    if (DUnlikely(DMaxStreamingTransmiterNumberInVodMode < mpContext->setting.max_streaming_transmiter_number)) {
        LOG_ERROR("mpContext->setting.max_streaming_transmiter_number %d excced max %d\n", mpContext->setting.max_streaming_transmiter_number, DMaxStreamingTransmiterNumberInVodMode);
        return EECode_BadParam;
    }
    DASSERT(mpContext->setting.max_streaming_transmiter_number);

    if (DUnlikely(DMinChannelNumberPerStreamingTransmiterInVodMode > mpContext->setting.channel_number_per_transmiter)) {
        LOG_ERROR("mpContext->setting.channel_number_per_transmiter %d less than min %d\n", mpContext->setting.channel_number_per_transmiter, DMinChannelNumberPerStreamingTransmiterInVodMode);
        return EECode_BadParam;
    }

    if (DUnlikely(mpContext->setting.max_push_channel_number > (mpContext->setting.channel_number_per_transmiter * mpContext->setting.max_streaming_transmiter_number))) {
        LOG_ERROR("streaming transmiter number not enough %dx%d < %d\n", mpContext->setting.channel_number_per_transmiter, mpContext->setting.max_streaming_transmiter_number, mpContext->setting.max_push_channel_number);
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CSimpleAPIInVodMode::initialSetting()
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

        if (mpContext->setting.filter_version) {
            mpMediaConfig->engine_config.filter_version = mpContext->setting.filter_version;
            LOG_NOTICE("specify filter version %d\n", mpMediaConfig->engine_config.filter_version);
        }
    } else {
        LOG_FATAL("NULL mpMediaConfig\n");
    }

    if (mpStorageManager && (mpContext->setting.enable_recording || mpContext->setting.enable_vod)) {
        TChar *url = NULL;
        err = mpStorageManager->LoadDataBase(mpContext->setting.root_dir, mChannelTotalCount);
        if (EECode_OK != err) {
            LOG_ERROR("mpStorageManager->LoadDataBase(root=%s) fail, err %d, %s\n", mpContext->setting.root_dir, err, gfGetErrorCodeString(err));
            return err;
        }
        for (TU32 i = 0; i < mpContext->setting.max_push_channel_number; i++) {
            url = mpContext->group_url.channel_name[i];
            if (strlen(url)) {
                err = mpStorageManager->AddChannel(mpContext->group_url.channel_name[i], mpContext->setting.max_storage_time, mpContext->setting.redudant_storage_time, mpContext->setting.single_file_duration_minutes);
            } else {
                snprintf(url, DMaxUrlLen, "xman_%d", i);
                err = mpStorageManager->AddChannel(mpContext->group_url.channel_name[i], mpContext->setting.max_storage_time, mpContext->setting.redudant_storage_time, mpContext->setting.single_file_duration_minutes);
            }
            if (DUnlikely((EECode_AlreadyExist != err) && (EECode_OK != err))) {
                LOG_ERROR("setupVodContent[%d], storage manager AddChannel fail, err %d, %s\n", i, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    if (mpContext->setting.enable_rtsp_authentication) {
        pre_config.check_field = EGenericEnginePreConfigure_RTSPStreamingServerSetAuthentication;
        pre_config.number = mpContext->setting.enable_rtsp_authentication;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_RTSPStreamingServerSetAuthentication, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_RTSPStreamingServerSetAuthentication), err %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }

        pre_config.check_field = EGenericEnginePreConfigure_RTSPStreamingServerAuthenticationMode;
        pre_config.number = mpContext->setting.rtsp_authentication_mode;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_RTSPStreamingServerAuthenticationMode, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_RTSPStreamingServerAuthenticationMode), err %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    }

    return EECode_OK;
}

EECode CSimpleAPIInVodMode::createGlobalComponents()
{
    EECode err = EECode_OK;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_PRINTF("[CSimpleAPIInVodMode]: createGlobalComponents begin\n");

    //create streaming server and data transmiter
    //DASSERT(mpContext->setting.enable_streaming_server);
    //mpContext->setting.enable_streaming_server = 1;
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

    LOG_PRINTF("[CSimpleAPIInVodMode]: createGlobalComponents end, mpContext->cloud_server_id %08x\n", mpContext->cloud_server_id);

    return EECode_OK;
}

EECode CSimpleAPIInVodMode::createGroupComponents()
{
    EECode err = EECode_OK;
    TUint i = 0;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_PRINTF("[CSimpleAPIInVodMode]: createGroupComponents() begin\n");

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

    //create vod
    if (mpContext->setting.enable_vod) {
        //create demuxers
        for (i = 0; i < mpContext->setting.max_push_channel_number; i ++) {
            err = mpEngineControl->NewComponent(EGenericComponentType_Demuxer, mpContext->gragh_component.demuxer_id[i]);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_Demuxer(%d)) fail, ret %d, %s.\n", i, err, gfGetErrorCodeString(err));
                return err;
            }
        }

        //create flow controllers
        for (i = 0; i < mpContext->setting.max_push_channel_number; i ++) {
            err = mpEngineControl->NewComponent(EGenericComponentType_FlowController, mpContext->gragh_component.flow_controller_id[i]);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_FlowController(%d)) fail, ret %d, %s.\n", i, err, gfGetErrorCodeString(err));
                return err;
            }
        }

        //create vod transmiters
        for (i = 0; i < mpContext->setting.max_push_channel_number; i ++) {
            err = mpEngineControl->NewComponent(EGenericComponentType_StreamingTransmiter, mpContext->gragh_component.vod_transmiter_id[i]);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_StreamingTransmiter(%d)) fail, ret %d, %s.\n", i, err, gfGetErrorCodeString(err));
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

    LOG_PRINTF("[CSimpleAPIInVodMode]: createGroupComponents() end\n");

    return EECode_OK;
}

EECode CSimpleAPIInVodMode::connectRecordingComponents()
{
    EECode err = EECode_OK;
    TUint channel_index = 0;
    TGenericID connection_id;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_PRINTF("[CSimpleAPIInVodMode]: connectRecordingComponents() begin\n");

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

    LOG_PRINTF("[CSimpleAPIInVodMode]: connectRecordingComponents() end\n");

    return EECode_OK;
}


EECode CSimpleAPIInVodMode::connectStreamingComponents()
{
    EECode err = EECode_OK;
    TUint i = 0;
    TUint round = mpContext->setting.max_streaming_transmiter_number;
    TUint transmiter_index = 0;
    TGenericID connection_id;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);
    DASSERT(round);

    LOG_PRINTF("[CSimpleAPIInVodMode]: connectStreamingComponents() begin\n");

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

    LOG_PRINTF("[CSimpleAPIInVodMode]: connectStreamingComponents() end\n");

    return EECode_OK;
}

EECode CSimpleAPIInVodMode::connectCloudComponents()
{
    if (!mpContext->setting.enable_push_path) {
        return EECode_OK;
    }

    EECode err = EECode_OK;
    TUint channel_index = 0;
    TGenericID connection_id;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_PRINTF("[CSimpleAPIInVodMode]: connectCloudComponents() begin\n");

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

    LOG_PRINTF("[CSimpleAPIInVodMode]: connectCloudComponents() end\n");

    return EECode_OK;
}

EECode CSimpleAPIInVodMode::connectVodComponents()
{
    if (!mpContext->setting.enable_vod) {
        return EECode_OK;
    }

    EECode err = EECode_OK;
    TUint channel_index = 0;
    TGenericID connection_id;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_PRINTF("[CSimpleAPIInVodMode]: connectVodComponents() begin\n");
    for (channel_index = 0; channel_index < mpContext->setting.max_push_channel_number; channel_index ++) {
        //demuxer to flow controller
        if (!mpContext->setting.disable_video_path) {
            err = mpEngineControl->ConnectComponent(connection_id, \
                                                    mpContext->gragh_component.demuxer_id[channel_index], \
                                                    mpContext->gragh_component.flow_controller_id[channel_index], \
                                                    StreamType_Video);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(demuxer[%d] to flow controller[%d]), video stream, err %d, %s\n", channel_index, channel_index, err, gfGetErrorCodeString(err));
                return err;
            }

            //flow controller to vod transmiter
            err = mpEngineControl->ConnectComponent(connection_id, \
                                                    mpContext->gragh_component.flow_controller_id[channel_index], \
                                                    mpContext->gragh_component.vod_transmiter_id[channel_index], \
                                                    StreamType_Video);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(flow controller[%d] to transmiter[%d]), video stream, err %d, %s\n", channel_index, channel_index, err, gfGetErrorCodeString(err));
                return err;
            }
        }

        if (!mpContext->setting.disable_audio_path) {
            err = mpEngineControl->ConnectComponent(connection_id, \
                                                    mpContext->gragh_component.demuxer_id[channel_index], \
                                                    mpContext->gragh_component.flow_controller_id[channel_index], \
                                                    StreamType_Audio);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(demuxer[%d] to flow controller[%d]), audio stream, err %d, %s\n", channel_index, channel_index, err, gfGetErrorCodeString(err));
                return err;
            }

            err = mpEngineControl->ConnectComponent(connection_id, \
                                                    mpContext->gragh_component.flow_controller_id[channel_index], \
                                                    mpContext->gragh_component.vod_transmiter_id[channel_index], \
                                                    StreamType_Audio);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(flow controller[%d] to transmiter[%d]), audio stream, err %d, %s\n", channel_index, channel_index, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    LOG_PRINTF("[CSimpleAPIInVodMode]: connectVodComponents() end\n");

    return EECode_OK;
}

EECode CSimpleAPIInVodMode::setupRecordingPipelines()
{
    EECode err = EECode_OK;
    TUint channel_index = 0;
    TUint max = mpContext->setting.max_push_channel_number;
    TGenericID video_source_id, audio_source_id;


    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_NOTICE("[CSimpleAPIInVodMode]: setupRecordingPipelines begin, enable_recording %d\n", mpContext->setting.enable_recording);

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
                    mpContext->gragh_component.muxer_id[channel_index], \
                    1, \
                    mpContext->group_url.channel_name[channel_index]);

            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("SetupRecordingPipeline[%d], err %d, %s\n", channel_index, err, gfGetErrorCodeString(err));
                return err;
            }

        }

    } else {
        LOG_NOTICE("[CSimpleAPIInVodMode]: recording not enabled\n");
    }

    return EECode_OK;
}


EECode CSimpleAPIInVodMode::setupStreamingPipelines()
{
    EECode err = EECode_OK;
    TUint channel_index = 0;
    TUint max = mpContext->setting.max_push_channel_number;
    TGenericID video_source_id, audio_source_id;
    TUint round = mpContext->setting.max_streaming_transmiter_number;
    TUint transmiter_index = 0;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_NOTICE("[CSimpleAPIInVodMode]: setupStreamingPipelines begin, enable_streaming_server %d\n", mpContext->setting.enable_streaming_server);

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
        LOG_NOTICE("[CSimpleAPIInVodMode]: streaming not enabled\n");
    }

    return EECode_OK;
}

EECode CSimpleAPIInVodMode::setupCloudPipelines()
{
    if (!mpContext->setting.enable_push_path) {
        return EECode_OK;
    }

    EECode err = EECode_OK;
    TUint channel_index = 0;
    TUint max = mpContext->setting.max_push_channel_number;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_NOTICE("[CSimpleAPIInVodMode]: setupCloudPipelines begin, enable_cloud_server %d\n", mpContext->setting.enable_cloud_server);

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
        LOG_NOTICE("[CSimpleAPIInVodMode]: cloud server not enabled\n");
    }

    return EECode_OK;
}

EECode CSimpleAPIInVodMode::setupVodPipelines()
{
    EECode err = EECode_OK;
    TUint channel_index = 0;
    TUint max = mpContext->setting.max_push_channel_number;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    TGenericID video_source_id, audio_source_id;

    if (DLikely(mpContext->setting.enable_vod)) {
        for (channel_index = 0; channel_index < max; channel_index ++) {

            if (DUnlikely(mpContext->setting.disable_video_path)) {
                video_source_id = 0;
            } else {
                video_source_id = mpContext->gragh_component.demuxer_id[channel_index];
            }

            if (DUnlikely(mpContext->setting.disable_audio_path)) {
                audio_source_id = 0;
            } else {
                audio_source_id = mpContext->gragh_component.demuxer_id[channel_index];
            }

            err = mpEngineControl->SetupVODPipeline(mpContext->gragh_component.vod_pipeline_id[channel_index], \
                                                    video_source_id, audio_source_id, \
                                                    mpContext->gragh_component.flow_controller_id[channel_index], \
                                                    mpContext->gragh_component.vod_transmiter_id[channel_index], \
                                                    mpContext->streaming_server_id, 1);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("SetupVODPipeline[%d], err %d, %s\n", channel_index, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    return EECode_OK;
}

EECode CSimpleAPIInVodMode::setupVodContent()
{
    EECode err = EECode_OK;
    TUint channel_index = 0;
    TUint max = mpContext->setting.max_push_channel_number;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    if (DLikely(mpContext->setting.enable_vod)) {
        //todo
        for (channel_index = 0; channel_index < max; channel_index ++) {
            err = mpEngineControl->SetupVodContent(channel_index, mpContext->group_url.channel_name[channel_index], \
                                                   0, !(mpContext->setting.disable_video_path), !(mpContext->setting.disable_audio_path));
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("setupVodContent[%d], err %d, %s\n", channel_index, err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    return EECode_OK;
}

EECode CSimpleAPIInVodMode::buildDataProcessingGragh()
{
    EECode err = EECode_OK;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_PRINTF("[CSimpleAPIInVodMode]: buildDataProcessingGragh begin\n");

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

    if (DLikely(mpContext->setting.enable_streaming_server)) {
        err = connectStreamingComponents();
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("connectStreamingComponents(), err %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    }

    err = connectCloudComponents();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("connectCloudComponents(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = connectVodComponents();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("connectVodComponents(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = setupRecordingPipelines();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("setupRecordingPipelines(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    if (DLikely(mpContext->setting.enable_streaming_server)) {
        err = setupStreamingPipelines();
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("setupStreamingPipelines(), err %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    }

    err = setupCloudPipelines();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("setupCloudPipelines(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = setupVodPipelines();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("setupVodPipelines(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = setupVodContent();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("setupVodContent(), err %d, %s\n", err, gfGetErrorCodeString(err));
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

    LOG_PRINTF("[CSimpleAPIInVodMode]: buildDataProcessingGragh end\n");

    return EECode_OK;
}

EECode CSimpleAPIInVodMode::preSetUrls()
{
    EECode err = EECode_OK;
    TUint i = 0;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    for (i = 0; i < mpContext->setting.max_push_channel_number; i ++) {
        TChar *url = mpContext->group_url.channel_name[i];
        if (strlen(url) == 0) {
            snprintf(url, DMaxUrlLen, "xman_%04d", i);
        }

        if (mpContext->setting.enable_recording && mpContext->gragh_component.muxer_id[i]) {
            err = mpEngineControl->SetSinkUrl(mpContext->gragh_component.muxer_id[i], url);
            LOG_PRINTF("SetSinkUrl 0x%08x, %s, ret %d, %s\n", mpContext->gragh_component.muxer_id[i], url, err, gfGetErrorCodeString(err));
        }

        if (mpContext->setting.enable_streaming_server && mpContext->gragh_component.streaming_pipeline_id[i]) {
            err = mpEngineControl->SetStreamingUrl(mpContext->gragh_component.streaming_pipeline_id[i], url);
            LOG_PRINTF("SetStreamingUrl 0x%08x, %s, ret %d, %s\n", mpContext->gragh_component.streaming_pipeline_id[i], url, err, gfGetErrorCodeString(err));
        }

        if (mpContext->setting.enable_cloud_server) {
            if (mpContext->gragh_component.cloud_agent_id[i]) {
                err = mpEngineControl->SetCloudAgentTag(mpContext->gragh_component.cloud_agent_id[i], url, mpContext->cloud_server_id);
                LOG_PRINTF("SetCloudAgentTag 0x%08x, %s, ret %d, %s\n", mpContext->gragh_component.cloud_agent_id[i], url, err, gfGetErrorCodeString(err));
            }

            if (mpContext->gragh_component.cloud_agent_cmd_id[i]) {
                err = mpEngineControl->SetCloudAgentTag(mpContext->gragh_component.cloud_agent_cmd_id[i], url, mpContext->cloud_server_id);
                LOG_PRINTF("SetCloudAgentCmdTag 0x%08x, %s, ret %d, %s\n", mpContext->gragh_component.cloud_agent_cmd_id[i], url, err, gfGetErrorCodeString(err));
            }

            if (mpContext->gragh_component.remote_control_agent_id[i]) {
                err = mpEngineControl->SetCloudAgentTag(mpContext->gragh_component.remote_control_agent_id[i], (TChar *)DRemoteControlString, mpContext->cloud_server_id);
                LOG_PRINTF("SetCloudAgentTag 0x%08x, %s, ret %d, %s\n", mpContext->gragh_component.remote_control_agent_id[i], DRemoteControlString, err, gfGetErrorCodeString(err));
            }
        }
    }
#if 0
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
                    snprintf(url, DMaxUrlLen, "xman_%d", i);
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
#endif
    return err;
}

EECode CSimpleAPIInVodMode::setupCommunicationServerPort()
{
    if (DUnlikely(mpAdminServerPort)) {
        mpAdminServerPort->Stop();
        mpAdminServerPort->Destroy();
        mpAdminServerPort = NULL;
    }

    mpAdminServerPort = CICommunicationServerPort::Create(this, CommunicationClientReadCallBack, mpContext->setting.admin_server_port);
    if (DUnlikely(!mpAdminServerPort)) {
        LOG_FATAL("CICommunicationServerPort::Create fail\n");
        return EECode_Error;
    }

    EECode err = mpAdminServerPort->Start(mpContext->setting.admin_server_port);
    if (DUnlikely(EECode_OK != err)) {
        LOG_FATAL("Start admin server port fail\n");
        mpAdminServerPort->Destroy();
        mpAdminServerPort = NULL;
        return err;
    }

    memset(mpPortSource, 0x0, sizeof(SCommunicationPortSource *)*mnMaxPortSourceNum);
    mnCurPortSourceNum = 0;

    return EECode_OK;
}

EECode CSimpleAPIInVodMode::updateUrls(TU32 index, TChar *p_url)
{
    DASSERT(p_url);

    if (DUnlikely(index >= mpContext->setting.max_push_channel_number)) {
        LOG_FATAL("No idle channel? index: %u, max number: %u\n", index, mpContext->setting.max_push_channel_number);
        return EECode_Busy;
    }

    TU32 target_index = 0;
    if (checkIsExist(p_url, target_index) == true) {
        LOG_FATAL("p_url is already added into channle list\n");
        return EECode_AlreadyExist;
    }

    EECode err = EECode_OK;
    TChar t_url[DMaxUrlLen] = {0};

    if (mpContext->setting.enable_recording) {
        err = mpEngineControl->SetSinkUrl(mpContext->gragh_component.muxer_id[index], p_url);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("updateUrls: SetSinkUrl fail, index: %u, url %s\n", index, p_url);
            return err;
        }
    }

    if (mpContext->setting.enable_streaming_server) {
        err = mpEngineControl->SetStreamingUrl(mpContext->gragh_component.streaming_pipeline_id[index], p_url);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("updateUrls: SetStreamingUrl fail, index: %u, url %s\n", index, p_url);
            return err;
        }
    }

    if (mpContext->setting.enable_cloud_server) {
        snprintf(t_url, DMaxUrlLen, "cloud_agent_%s", p_url);
        err = mpEngineControl->SetCloudAgentTag(mpContext->gragh_component.cloud_agent_id[index], t_url, mpContext->cloud_server_id);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("updateUrls: SetCloudAgentTag fail, index: %u, url: %s\n", index, p_url);
            return err;
        }
        snprintf(t_url, DMaxUrlLen, "cloud_agent_cmd_%s", p_url);
        err = mpEngineControl->SetCloudAgentTag(mpContext->gragh_component.cloud_agent_cmd_id[index], t_url, mpContext->cloud_server_id);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("updateUrls: SetCloudAgentCmdTag fail, index: %u, url: %s\n", index, p_url);
            return err;
        }
    }

    if (DLikely(mpContext->setting.enable_vod)) {
        err = mpEngineControl->SetupVodContent(index, p_url, \
                                               0, !(mpContext->setting.disable_video_path), !(mpContext->setting.disable_audio_path));
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("updateUrls: SetupVodContent fail, index: %u, url: %s\n", index, p_url);
            return err;
        }
    }

    snprintf(mpContext->group_url.channel_name[index], DMaxUrlLen, "%s", p_url);

    err = updateIdleChannelIndex();
    if (DUnlikely(EECode_OK != err)) {
        LOG_WARN("updateIdleChannelIndex fail, no idle channel?\n");
    }

    return EECode_OK;
}

EECode CSimpleAPIInVodMode::updateIdleChannelIndex()
{
    //update channel index
    TU32 current_index = mChannelIndex + 1;
    TU32 max_channel_num = mpContext->setting.max_push_channel_number;
    TU32 index = 0;

    while (current_index < (max_channel_num + mChannelIndex)) {
        if (current_index < max_channel_num) {
            index = current_index;
        } else {
            index = current_index - max_channel_num;
        }

        if (strlen(mpContext->group_url.channel_name[index]) == 0) {
            mChannelIndex = index;
            return EECode_OK;
        }
        current_index++;
    }

    return EECode_Busy;
}

bool CSimpleAPIInVodMode::checkIsExist(TChar *target_url, TU32 &index)
{
    if (DUnlikely(!target_url)) {
        LOG_FATAL("Invaild Parameter, target_url is NULL\n");
        return false;
    }

    TU32 max_channel_num = mpContext->setting.max_push_channel_number;
    TU32 target_index = 0;
    TU32 target_length = strlen(target_url);
    while (target_index < max_channel_num) {
        if (target_length == strlen(mpContext->group_url.channel_name[target_index])) {
            if (strcmp(target_url, mpContext->group_url.channel_name[target_index]) == 0) {
                index = target_index;
                return true;
            }
        }
        target_index++;
    }

    return false;
}

EECode CSimpleAPIInVodMode::removeChannel(TChar *p_url)
{
    TU32 index = 0;
    if (checkIsExist(p_url, index) == false) {
        LOG_FATAL("target channel is not in list\n");
        return EECode_Error;
    }

    mpContext->group_url.channel_name[index][0] = '\0';
    mChannelIndex = index;

    return EECode_OK;
}

EECode CSimpleAPIInVodMode::updateDeviceDynamicCode(TU32 index, TU32 dynamic_code)
{
    DASSERT(mpEngineControl);
    EECode err = mpEngineControl->UpdateDeviceDynamicCode(mpContext->gragh_component.cloud_agent_id[index], dynamic_code);
    if (DUnlikely(EECode_OK != err)) {
        LOG_FATAL("updateDeviceDynamicCode fail, err: %d, %s\n", err, gfGetErrorCodeString(err));
    }
    return err;
}

EECode CSimpleAPIInVodMode::updateUserDynamicCode(const TChar *p_user_name, const TChar *p_device_name, TU32 dynamic_code)
{
    DASSERT(mpEngineControl);
    EECode err = mpEngineControl->UpdateUserDynamicCode(p_user_name, p_device_name, dynamic_code);
    if (DUnlikely(EECode_OK != err)) {
        LOG_FATAL("updateDeviceDynamicCode fail, err: %d, %s\n", err, gfGetErrorCodeString(err));
    }
    return err;
}
#if 0
int CSimpleAPIInVodMode::ProcessReadBuffer(TU8 *p_data, TU32 datasize, TU32 type, TU32 cat, TU32 subtype)
{
    TU32 cur_size = 0;
    TU8 *p_cur = NULL;
    TUniqueID id = 0;
    STLV16HeaderBigEndian *p_tlv = NULL;
    TU16 tlv_type = 0;
    TU16 tlv_len = 0;

    LOG_PRINTF("ProcessReadBuffer start, datasize %d\n", datasize);
    LOG_PRINTF("                   subtype %d\n", subtype);

    switch (subtype) {

        case ESACPAdminSubType_QueryDataServerStatus:
            LOG_PRINTF("ESACPAdminSubType_QueryDataServerStatus start\n");

            break;

        case ESACPAdminSubType_AssignDataServerSourceDevice:
            LOG_PRINTF("ESACPAdminSubType_AssignDataServerSourceDevice start\n");

            p_cur = p_data;
            p_tlv = (STLV16HeaderBigEndian *) p_cur;
            tlv_type = (p_tlv->type_high << 8) | (p_tlv->type_low);
            tlv_len = (p_tlv->length_high << 8) | (p_tlv->length_low);
            if (ETLV16Type_AccountID != tlv_type) {
                if (ETLV16Type_ErrorDescription == tlv_type) {
                    TChar return_error_string[128] = {0};
                    if (tlv_len < 128) {
                        memcpy(return_error_string, p_cur + sizeof(STLV16HeaderBigEndian), tlv_len);
                    } else {
                        LOG_ERROR("Query source device account via name fail, return error string too long");
                        return (-6);
                    }
                    LOG_ERROR("Query source device account via name fail\n");
                    return (-5);
                }
                LOG_ERROR("bad responce from server, ESACPAdminSubType_QuerySourceViaAccountName");
                return (-4);
            }

            p_cur += sizeof(mw_cg::STLV16HeaderBigEndian);
            cur_size += sizeof(STLV16HeaderBigEndian);
            id = ((((mw_cg::TUniqueID)p_cur[0]) << 56) | (((mw_cg::TUniqueID)p_cur[1]) << 48) | (((mw_cg::TUniqueID)p_cur[2]) << 40) | (((mw_cg::TUniqueID)p_cur[3]) << 32) | (((mw_cg::TUniqueID)p_cur[4]) << 24) | (((mw_cg::TUniqueID)p_cur[5]) << 16) | (((mw_cg::TUniqueID)p_cur[6]) << 8) | ((mw_cg::TUniqueID)p_cur[7]));
            p_cur += sizeof(mw_cg::TUniqueID);
            cur_size += sizeof(mw_cg::TUniqueID);

            {
                SAccountInfoSourceDevice *p_source = getDeviceAccount(id);
                int new_source = 0;
                if (!p_source) {
                    p_source = newDeviceAccount();
                    if (!p_source) {
                        break;
                    }
                    new_source = 1;
                }

                p_source->root.header.id = id;

                p_tlv = (mw_cg::STLV16HeaderBigEndian *) p_cur;
                tlv_type = (p_tlv->type_high << 8) | (p_tlv->type_low);
                tlv_len = (p_tlv->length_high << 8) | (p_tlv->length_low);
                p_cur += sizeof(mw_cg::STLV16HeaderBigEndian);
                cur_size += sizeof(STLV16HeaderBigEndian);
                if ((ETLV16Type_AccountName == tlv_type) || (tlv_len <= DMAX_ACCOUNT_NAME_LENGTH_EX)) {
                    strncpy(p_source->root.base.name, (char *)p_cur, tlv_len - 1);
                } else {
                    AfxMessageBox("bad ETLV16Type_AccountName field");
                    return (-8);
                }
                p_cur += tlv_len;
                cur_size += tlv_len;


                p_tlv = (mw_cg::STLV16HeaderBigEndian *) p_cur;
                tlv_type = (p_tlv->type_high << 8) | (p_tlv->type_low);
                tlv_len = (p_tlv->length_high << 8) | (p_tlv->length_low);
                p_cur += sizeof(mw_cg::STLV16HeaderBigEndian);
                cur_size += sizeof(STLV16HeaderBigEndian);
                if ((ETLV16Type_AccountPassword == tlv_type) || (tlv_len <= DMAX_ACCOUNT_NAME_LENGTH)) {
                    strncpy(p_source->root.base.password, (char *)p_cur, tlv_len - 1);
                } else {
                    AfxMessageBox("bad ETLV16Type_AccountPassword field");
                    return (-8);
                }
                p_cur += tlv_len;
                cur_size += tlv_len;


                p_tlv = (mw_cg::STLV16HeaderBigEndian *) p_cur;
                tlv_type = (p_tlv->type_high << 8) | (p_tlv->type_low);
                tlv_len = (p_tlv->length_high << 8) | (p_tlv->length_low);
                p_cur += sizeof(mw_cg::STLV16HeaderBigEndian);
                cur_size += sizeof(STLV16HeaderBigEndian);
                if ((ETLV16Type_AccountIDString == tlv_type) || (tlv_len <= DMAX_ID_AUTHENTICATION_STRING_LENGTH)) {
                    strncpy(p_source->root.base.idauthentication, (char *)p_cur, tlv_len - 1);
                } else {
                    AfxMessageBox("bad ETLV16Type_AccountIDString field");
                    return (-8);
                }
                p_cur += tlv_len;
                cur_size += tlv_len;


                p_tlv = (mw_cg::STLV16HeaderBigEndian *) p_cur;
                tlv_type = (p_tlv->type_high << 8) | (p_tlv->type_low);
                tlv_len = (p_tlv->length_high << 8) | (p_tlv->length_low);
                p_cur += sizeof(mw_cg::STLV16HeaderBigEndian);
                cur_size += sizeof(STLV16HeaderBigEndian);
                if ((ETLV16Type_SourceDeviceProductCode == tlv_type) || (tlv_len <= DMAX_PRODUCTION_CODE_LENGTH)) {
                    strncpy(p_source->ext.production_code, (char *)p_cur, tlv_len - 1);
                } else {
                    AfxMessageBox("bad ETLV16Type_SourceDeviceProductCode field");
                    return (-8);
                }
                p_cur += tlv_len;
                cur_size += tlv_len;


                p_tlv = (mw_cg::STLV16HeaderBigEndian *) p_cur;
                tlv_type = (p_tlv->type_high << 8) | (p_tlv->type_low);
                tlv_len = (p_tlv->length_high << 8) | (p_tlv->length_low);
                p_cur += sizeof(mw_cg::STLV16HeaderBigEndian);
                cur_size += sizeof(STLV16HeaderBigEndian);
                if ((ETLV16Type_SourceDeviceOwner == tlv_type) || (tlv_len == (sizeof(mw_cg::TUniqueID)))) {
                    p_source->ext.ownerid = ((((mw_cg::TUniqueID)p_cur[0]) << 56) | (((mw_cg::TUniqueID)p_cur[1]) << 48) | (((mw_cg::TUniqueID)p_cur[2]) << 40) | (((mw_cg::TUniqueID)p_cur[3]) << 32) | (((mw_cg::TUniqueID)p_cur[4]) << 24) | (((mw_cg::TUniqueID)p_cur[5]) << 16) | (((mw_cg::TUniqueID)p_cur[6]) << 8) | ((mw_cg::TUniqueID)p_cur[7]));
                } else {
                    AfxMessageBox("bad owner id field");
                    return (-8);
                }
                p_cur += tlv_len;
                cur_size += tlv_len;


                p_tlv = (mw_cg::STLV16HeaderBigEndian *) p_cur;
                tlv_type = (p_tlv->type_high << 8) | (p_tlv->type_low);
                tlv_len = (p_tlv->length_high << 8) | (p_tlv->length_low);
                p_cur += sizeof(mw_cg::STLV16HeaderBigEndian);
                cur_size += sizeof(STLV16HeaderBigEndian);
                if ((ETLV16Type_SourceDeviceShareList == tlv_type) || (tlv_len >= (sizeof(mw_cg::TUniqueID)))) {
                    unsigned int max_unit = tlv_len / (sizeof(mw_cg::TUniqueID));
                    unsigned int i = 0;
                    if (!p_source->p_sharedfriendsid || (p_source->friendsnum < max_unit)) {
                        if (p_source->p_sharedfriendsid) {
                            free(p_source->p_sharedfriendsid);
                        }
                        p_source->p_sharedfriendsid = (mw_cg::TUniqueID *) malloc(max_unit * sizeof(mw_cg::TUniqueID));
                        if (!p_source->p_sharedfriendsid) {
                            AfxMessageBox("no memory");
                            return (-10);
                        }
                    }

                    for (i = 0; i < max_unit; i ++) {
                        p_source->p_sharedfriendsid[i] = ((((mw_cg::TUniqueID)p_cur[0]) << 56) | (((mw_cg::TUniqueID)p_cur[1]) << 48) | (((mw_cg::TUniqueID)p_cur[2]) << 40) | (((mw_cg::TUniqueID)p_cur[3]) << 32) | (((mw_cg::TUniqueID)p_cur[4]) << 24) | (((mw_cg::TUniqueID)p_cur[5]) << 16) | (((mw_cg::TUniqueID)p_cur[6]) << 8) | ((mw_cg::TUniqueID)p_cur[7]));
                        p_cur += sizeof(mw_cg::TUniqueID);
                        cur_size += sizeof(mw_cg::TUniqueID);
                    }
                } else if (!tlv_len) {
                    //skip
                } else {
                    AfxMessageBox("bad share list id field");
                    return (-8);
                }

                p_tlv = (mw_cg::STLV16HeaderBigEndian *) p_cur;
                tlv_type = (p_tlv->type_high << 8) | (p_tlv->type_low);
                tlv_len = (p_tlv->length_high << 8) | (p_tlv->length_low);
                p_cur += sizeof(mw_cg::STLV16HeaderBigEndian);
                cur_size += sizeof(STLV16HeaderBigEndian);
                if ((ETLV16Type_SourceDeviceSharePrivilegeMaskList == tlv_type) || (tlv_len >= (sizeof(mw_cg::TU32)))) {
                    unsigned int max_unit = tlv_len / (sizeof(mw_cg::TU32));
                    unsigned int i = 0;
                    if (!p_source->p_privilege_mask || (p_source->friendsnum < max_unit)) {
                        if (p_source->p_privilege_mask) {
                            free(p_source->p_privilege_mask);
                        }
                        p_source->p_privilege_mask = (mw_cg::TU32 *) malloc(max_unit * sizeof(mw_cg::TU32));
                        if (!p_source->p_privilege_mask) {
                            AfxMessageBox("no memory");
                            return (-10);
                        }
                    }

                    for (i = 0; i < max_unit; i ++) {
                        p_source->p_privilege_mask[i] = ((((mw_cg::TU32)p_cur[0]) << 24) | (((mw_cg::TU32)p_cur[1]) << 16) | (((mw_cg::TU32)p_cur[2]) << 8) | (((mw_cg::TU32)p_cur[3])));
                        p_cur += sizeof(mw_cg::TU32);
                        cur_size += sizeof(mw_cg::TU32);
                    }
                } else if (!tlv_len) {
                    //skip
                } else {
                    AfxMessageBox("bad share privilege list id field");
                    return (-8);
                }

                if (new_source) {
                    if (FALSE == insertDeviceAccount(p_source)) {
                        free(p_source);
                    } else {
                        CString str;
                        str += p_source->root.base.name;
                        mSourceDeviceList.AddString(str);
                    }
                }
            }
            break;

        case ESACPAdminSubType_NewSourceAccount:
            gfWriteLogString("ESACPAdminSubType_NewSourceAccount start\n");
            if (mpWaitResponceSourceDevice) {
                p_cur = p_data;
                p_tlv = (mw_cg::STLV16HeaderBigEndian *) p_cur;
                tlv_type = (p_tlv->type_high << 8) | (p_tlv->type_low);
                tlv_len = (p_tlv->length_high << 8) | (p_tlv->length_low);
                mpWaitResponceSourceDevice->root.header.id = ((((mw_cg::TUniqueID)p_cur[0]) << 56) | (((mw_cg::TUniqueID)p_cur[1]) << 48) | (((mw_cg::TUniqueID)p_cur[2]) << 40) | (((mw_cg::TUniqueID)p_cur[3]) << 32) | (((mw_cg::TUniqueID)p_cur[4]) << 24) | (((mw_cg::TUniqueID)p_cur[5]) << 16) | (((mw_cg::TUniqueID)p_cur[6]) << 8) | ((mw_cg::TUniqueID)p_cur[7]));
                gfWriteLogStringInterger("tlv_type %d\n", tlv_type);
                gfWriteLogStringInterger("tlv_len %d\n", tlv_len);
                if (FALSE == insertDeviceAccount(mpWaitResponceSourceDevice)) {
                    gfWriteLogString("insertDeviceAccount fail\n");
                    free(mpWaitResponceSourceDevice);
                } else {
                    CString str;
                    str += mpWaitResponceSourceDevice->root.base.name;
                    mSourceDeviceList.AddString(str);
                    gfWriteLogString("insertDeviceAccount success: \n");
                    gfWriteLogString(mpWaitResponceSourceDevice->root.base.name);
                }
                mpWaitResponceSourceDevice = NULL;
            } else {
                AfxMessageBox("no mpWaitResponceSourceDevice?");
                return (-8);
            }
            mbWaitResponceFromServer = FALSE;
            break;

        case mw_cg::ESACPAdminSubType_QuerySinkViaAccountName:
            p_cur = p_data;

            p_tlv = (mw_cg::STLV16HeaderBigEndian *) p_cur;
            tlv_type = (p_tlv->type_high << 8) | (p_tlv->type_low);
            tlv_len = (p_tlv->length_high << 8) | (p_tlv->length_low);
            if (ETLV16Type_AccountID != tlv_type) {
                if (ETLV16Type_ErrorDescription == tlv_type) {
                    char return_error_string[128] = {0};
                    if (tlv_len < 128) {
                        memcpy(return_error_string, p_cur + sizeof(mw_cg::STLV16HeaderBigEndian), tlv_len);
                    } else {
                        AfxMessageBox(_T("Query source device account via name fail, return error string too long"));
                        return (-6);
                    }
                    CString error_string;
                    error_string += "Query source device account via name fail: ";
                    error_string += return_error_string;
                    AfxMessageBox(error_string);
                    return (-5);
                }
                AfxMessageBox("bad responce from server, ESACPAdminSubType_QuerySourceViaAccountName");
                return (-4);
            }
            p_cur += sizeof(mw_cg::STLV16HeaderBigEndian);
            cur_size += sizeof(STLV16HeaderBigEndian);
            id = ((((mw_cg::TUniqueID)p_cur[0]) << 56) | (((mw_cg::TUniqueID)p_cur[1]) << 48) | (((mw_cg::TUniqueID)p_cur[2]) << 40) | (((mw_cg::TUniqueID)p_cur[3]) << 32) | (((mw_cg::TUniqueID)p_cur[4]) << 24) | (((mw_cg::TUniqueID)p_cur[5]) << 16) | (((mw_cg::TUniqueID)p_cur[6]) << 8) | ((mw_cg::TUniqueID)p_cur[7]));
            p_cur += sizeof(mw_cg::TUniqueID);
            cur_size += sizeof(mw_cg::TUniqueID);

            {
                SAccountInfoSourceDevice *p_source = getDeviceAccount(id);
                int new_source = 0;
                if (!p_source) {
                    p_source = newDeviceAccount();
                    if (!p_source) {
                        break;
                    }
                    new_source = 1;
                }

                p_source->root.header.id = id;

                p_tlv = (mw_cg::STLV16HeaderBigEndian *) p_cur;
                tlv_type = (p_tlv->type_high << 8) | (p_tlv->type_low);
                tlv_len = (p_tlv->length_high << 8) | (p_tlv->length_low);
                p_cur += sizeof(mw_cg::STLV16HeaderBigEndian);
                cur_size += sizeof(STLV16HeaderBigEndian);
                if ((ETLV16Type_AccountName == tlv_type) || (tlv_len <= DMAX_ACCOUNT_NAME_LENGTH_EX)) {
                    strncpy(p_source->root.base.name, (char *)p_cur, tlv_len - 1);
                } else {
                    AfxMessageBox("bad ETLV16Type_AccountName field");
                    return (-8);
                }
                p_cur += tlv_len;
                cur_size += tlv_len;


                p_tlv = (mw_cg::STLV16HeaderBigEndian *) p_cur;
                tlv_type = (p_tlv->type_high << 8) | (p_tlv->type_low);
                tlv_len = (p_tlv->length_high << 8) | (p_tlv->length_low);
                p_cur += sizeof(mw_cg::STLV16HeaderBigEndian);
                cur_size += sizeof(STLV16HeaderBigEndian);
                if ((ETLV16Type_AccountPassword == tlv_type) || (tlv_len <= DMAX_ACCOUNT_NAME_LENGTH)) {
                    strncpy(p_source->root.base.password, (char *)p_cur, tlv_len - 1);
                } else {
                    AfxMessageBox("bad ETLV16Type_AccountPassword field");
                    return (-8);
                }
                p_cur += tlv_len;
                cur_size += tlv_len;


                p_tlv = (mw_cg::STLV16HeaderBigEndian *) p_cur;
                tlv_type = (p_tlv->type_high << 8) | (p_tlv->type_low);
                tlv_len = (p_tlv->length_high << 8) | (p_tlv->length_low);
                p_cur += sizeof(mw_cg::STLV16HeaderBigEndian);
                cur_size += sizeof(STLV16HeaderBigEndian);
                if ((ETLV16Type_AccountIDString == tlv_type) || (tlv_len <= DMAX_ID_AUTHENTICATION_STRING_LENGTH)) {
                    strncpy(p_source->root.base.idauthentication, (char *)p_cur, tlv_len - 1);
                } else {
                    AfxMessageBox("bad ETLV16Type_AccountIDString field");
                    return (-8);
                }
                p_cur += tlv_len;
                cur_size += tlv_len;


                p_tlv = (mw_cg::STLV16HeaderBigEndian *) p_cur;
                tlv_type = (p_tlv->type_high << 8) | (p_tlv->type_low);
                tlv_len = (p_tlv->length_high << 8) | (p_tlv->length_low);
                p_cur += sizeof(mw_cg::STLV16HeaderBigEndian);
                cur_size += sizeof(STLV16HeaderBigEndian);
                if ((ETLV16Type_SourceDeviceProductCode == tlv_type) || (tlv_len <= DMAX_PRODUCTION_CODE_LENGTH)) {
                    strncpy(p_source->ext.production_code, (char *)p_cur, tlv_len - 1);
                } else {
                    AfxMessageBox("bad ETLV16Type_SourceDeviceProductCode field");
                    return (-8);
                }
                p_cur += tlv_len;
                cur_size += tlv_len;


                p_tlv = (mw_cg::STLV16HeaderBigEndian *) p_cur;
                tlv_type = (p_tlv->type_high << 8) | (p_tlv->type_low);
                tlv_len = (p_tlv->length_high << 8) | (p_tlv->length_low);
                p_cur += sizeof(mw_cg::STLV16HeaderBigEndian);
                cur_size += sizeof(STLV16HeaderBigEndian);
                if ((ETLV16Type_SourceDeviceOwner == tlv_type) || (tlv_len == (sizeof(mw_cg::TUniqueID)))) {
                    p_source->ext.ownerid = ((((mw_cg::TUniqueID)p_cur[0]) << 56) | (((mw_cg::TUniqueID)p_cur[1]) << 48) | (((mw_cg::TUniqueID)p_cur[2]) << 40) | (((mw_cg::TUniqueID)p_cur[3]) << 32) | (((mw_cg::TUniqueID)p_cur[4]) << 24) | (((mw_cg::TUniqueID)p_cur[5]) << 16) | (((mw_cg::TUniqueID)p_cur[6]) << 8) | ((mw_cg::TUniqueID)p_cur[7]));
                } else {
                    AfxMessageBox("bad owner id field");
                    return (-8);
                }
                p_cur += tlv_len;
                cur_size += tlv_len;


                p_tlv = (mw_cg::STLV16HeaderBigEndian *) p_cur;
                tlv_type = (p_tlv->type_high << 8) | (p_tlv->type_low);
                tlv_len = (p_tlv->length_high << 8) | (p_tlv->length_low);
                p_cur += sizeof(mw_cg::STLV16HeaderBigEndian);
                cur_size += sizeof(STLV16HeaderBigEndian);
                if ((ETLV16Type_SourceDeviceShareList == tlv_type) || (tlv_len >= (sizeof(mw_cg::TUniqueID)))) {
                    unsigned int max_unit = tlv_len / (sizeof(mw_cg::TUniqueID));
                    unsigned int i = 0;
                    if (!p_source->p_sharedfriendsid || (p_source->friendsnum < max_unit)) {
                        if (p_source->p_sharedfriendsid) {
                            free(p_source->p_sharedfriendsid);
                        }
                        p_source->p_sharedfriendsid = (mw_cg::TUniqueID *) malloc(max_unit * sizeof(mw_cg::TUniqueID));
                        if (!p_source->p_sharedfriendsid) {
                            AfxMessageBox("no memory");
                            return (-10);
                        }
                    }

                    for (i = 0; i < max_unit; i ++) {
                        p_source->p_sharedfriendsid[i] = ((((mw_cg::TUniqueID)p_cur[0]) << 56) | (((mw_cg::TUniqueID)p_cur[1]) << 48) | (((mw_cg::TUniqueID)p_cur[2]) << 40) | (((mw_cg::TUniqueID)p_cur[3]) << 32) | (((mw_cg::TUniqueID)p_cur[4]) << 24) | (((mw_cg::TUniqueID)p_cur[5]) << 16) | (((mw_cg::TUniqueID)p_cur[6]) << 8) | ((mw_cg::TUniqueID)p_cur[7]));
                        p_cur += sizeof(mw_cg::TUniqueID);
                        cur_size += sizeof(mw_cg::TUniqueID);
                    }
                } else if (!tlv_len) {
                    //skip
                } else {
                    AfxMessageBox("bad share list id field");
                    return (-8);
                }

                p_tlv = (mw_cg::STLV16HeaderBigEndian *) p_cur;
                tlv_type = (p_tlv->type_high << 8) | (p_tlv->type_low);
                tlv_len = (p_tlv->length_high << 8) | (p_tlv->length_low);
                p_cur += sizeof(mw_cg::STLV16HeaderBigEndian);
                cur_size += sizeof(STLV16HeaderBigEndian);
                if ((ETLV16Type_SourceDeviceSharePrivilegeMaskList == tlv_type) || (tlv_len >= (sizeof(mw_cg::TU32)))) {
                    unsigned int max_unit = tlv_len / (sizeof(mw_cg::TU32));
                    unsigned int i = 0;
                    if (!p_source->p_privilege_mask || (p_source->friendsnum < max_unit)) {
                        if (p_source->p_privilege_mask) {
                            free(p_source->p_privilege_mask);
                        }
                        p_source->p_privilege_mask = (mw_cg::TU32 *) malloc(max_unit * sizeof(mw_cg::TU32));
                        if (!p_source->p_privilege_mask) {
                            AfxMessageBox("no memory");
                            return (-10);
                        }
                    }

                    for (i = 0; i < max_unit; i ++) {
                        p_source->p_privilege_mask[i] = ((((mw_cg::TU32)p_cur[0]) << 24) | (((mw_cg::TU32)p_cur[1]) << 16) | (((mw_cg::TU32)p_cur[2]) << 8) | (((mw_cg::TU32)p_cur[3])));
                        p_cur += sizeof(mw_cg::TU32);
                        cur_size += sizeof(mw_cg::TU32);
                    }
                } else if (!tlv_len) {
                    //skip
                } else {
                    AfxMessageBox("bad share privilege list id field");
                    return (-8);
                }

                if (new_source) {
                    if (FALSE == insertDeviceAccount(p_source)) {
                        free(p_source);
                    } else {
                        CString str;
                        str += p_source->root.base.name;
                        mSourceDeviceList.AddString(str);
                    }
                }
            }

            break;

        default:
            LOG_ERROR("Bad subtype %d\n", subtype);
            break;

    }

    return 0;
}
#endif

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END




