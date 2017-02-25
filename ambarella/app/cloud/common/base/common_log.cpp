/*******************************************************************************
 * common_log.cpp
 *
 * History:
 *  2013/04/25 - [Zhi He] create file
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
#include "common_log.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

SLogConfig gmModuleLogConfig[ELogModuleListEnd] = {
    //global settings
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogGlobal = 0,

    //engines
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleGenericEngine
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleStreamingServerManager
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleCloudServerManager
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleIMServerManager

    //filters
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleDemuxerFilter,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleVideoCapturerFilter,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleAudioCapturerFilter,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleVideoDecoderFilter,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleAudioDecoderFilter,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleVideoRendererFilter,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleAudioRendererFilter,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleVideoPostPFilter,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleAudioPostPFilter,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleVideoOutputFilter,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleAudioOutputFilter,

    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleMuxerFilter,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleStreamingTransmiterFilter,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleVideoEncoderFilter,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleAudioEncoderFilter,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleConnecterPinmuxer,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleCloudConnecterServerAgent,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleCloudConnecterClientAgent,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleFlowController,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleExternalSourceVideoEs,

    //modules
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModulePrivateRTSPDemuxer,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModulePrivateRTSPScheduledDemuxer,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleUploadingReceiver,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleAmbaDecoder,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleAmbaVideoPostProcessor,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleAmbaVideoRenderer,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleAmbaEncoder,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleStreamingTransmiter,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleFFMpegDemuxer,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleFFMpegMuxer,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleFFMpegAudioDecoder,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleFFMpegVideoDecoder,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleAudioRenderer,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleVideoInput,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleAudioInput,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleAACAudioDecoder,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleAACAudioEncoder,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleCustomizedAudioDecoder,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleCustomizedAudioEncoder,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleNativeTSMuxer,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleNativeTSDemuxer,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleNativeMP4Muxer,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleNativeMP4Demuxer

    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleRTSPStreamingServer,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleCloudServer,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleIMServer,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleDirectSharingServer,

    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleOPTCVideoEncoder,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleOPTCAudioEncoder,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleOPTCVideoDecoder,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleOPTCAudioDecoder,

    //dsp platform
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleDSPPlatform,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleDSPDec,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleDSPEnc,

    //scheduler
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleRunRobinScheduler,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModulePreemptiveScheduler,

    //thread pool
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleThreadPool,

    //data base
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleLightWeightDataBase,

    //account manager
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleAccountManager,

    //protocol agent
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleSACPAdminAgent,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogModuleSACPIMAgent,

    //app framework
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogAPPFrameworkSceneDirector,
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogAPPFrameworkSoundComposer

    //platform al
    {ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File)}, //ELogPlatformALSoundOutput,

};

volatile TUint *const pgConfigLogLevel = (volatile TUint *const) &gmModuleLogConfig[ELogGlobal].log_level;
volatile TUint *const pgConfigLogOption = (volatile TUint *const) &gmModuleLogConfig[ELogGlobal].log_option;
volatile TUint *const pgConfigLogOutput = (volatile TUint *const) &gmModuleLogConfig[ELogGlobal].log_output;

const TChar gcConfigLogLevelTag[ELogLevel_TotalCount][DMAX_LOGTAG_NAME_LEN] = {
    {"\t"},
    {"[Fatal ]"},
    {"[Error ]"},
    {"[Warn  ]"},
    {"[Notice]"},
    {"[Info  ]"},
    {"[Debug ]"},
    {"[Verbose]"},
};

const TChar gcConfigLogOptionTag[ELogOption_TotalCount][DMAX_LOGTAG_NAME_LEN] = {
    {"[State]"},
    {"[Config]"},
    {"[Flow]"},
    {"[PTS]"},
    {"[Resource]"},
    {"[Timing]"},
    {"[BinHeader]"},
    {"[BinWholeFile]"},
    {"[BinSepFile]"},
};

EECode gfGetLogLevelString(ELogLevel level, const TChar *&loglevel_string)
{
    EECode err = EECode_OK;

    switch (level) {
        case ELogLevel_Fatal:
            loglevel_string = "ELogLevel_Fatal";
            break;

        case ELogLevel_Error:
            loglevel_string = "ELogLevel_Error";
            break;

        case ELogLevel_Warn:
            loglevel_string = "ELogLevel_Warn";
            break;

        case ELogLevel_Notice:
            loglevel_string = "ELogLevel_Notice";
            break;

        case ELogLevel_Info:
            loglevel_string = "ELogLevel_Info";
            break;

        case ELogLevel_Debug:
            loglevel_string = "ELogLevel_Debug";
            break;

        case ELogLevel_Verbose:
            loglevel_string = "ELogLevel_Verbose";
            break;

        default:
            LOG_ERROR("Unknown log level %d\n", level);
            loglevel_string = "UnkwonLevelXX";
            err = EECode_BadParam;
            break;
    }

    return err;
}

EECode gfGetLogModuleString(ELogModule module, const TChar *&module_string)
{
    EECode err = EECode_OK;

    switch (module) {
        case ELogGlobal:
            module_string = "ELogGlobal";
            break;

        case ELogModuleGenericEngine:
            module_string = "ELogModuleGenericEngine";
            break;

        case ELogModuleStreamingServerManager:
            module_string = "ELogModuleStreamingServerManager";
            break;

        case ELogModuleCloudServerManager:
            module_string = "ELogModuleCloudServerManager";
            break;

        case ELogModuleIMServerManager:
            module_string = "ELogModuleIMServerManager";
            break;

        case ELogModuleDemuxerFilter:
            module_string = "ELogModuleDemuxerFilter";
            break;

        case ELogModuleAudioCapturerFilter:
            module_string = "ELogModuleAudioCapturerFilter";
            break;

        case ELogModuleVideoDecoderFilter:
            module_string = "ELogModuleVideoDecoderFilter";
            break;

        case ELogModuleAudioDecoderFilter:
            module_string = "ELogModuleAudioDecoderFilter";
            break;

        case ELogModuleVideoRendererFilter:
            module_string = "ELogModuleVideoRendererFilter";
            break;

        case ELogModuleAudioRendererFilter:
            module_string = "ELogModuleAudioRendererFilter";
            break;

        case ELogModuleVideoPostPFilter:
            module_string = "ELogModuleVideoPostPFilter";
            break;

        case ELogModuleAudioPostPFilter:
            module_string = "ELogModuleAudioPostPFilter";
            break;

        case ELogModuleMuxerFilter:
            module_string = "ELogModuleMuxerFilter";
            break;

        case ELogModuleStreamingTransmiterFilter:
            module_string = "ELogModuleStreamingTransmiterFilter";
            break;

        case ELogModuleVideoEncoderFilter:
            module_string = "ELogModuleVideoEncoderFilter";
            break;

        case ELogModuleAudioEncoderFilter:
            module_string = "ELogModuleAudioEncoderFilter";
            break;

        case ELogModuleConnecterPinmuxer:
            module_string = "ELogModuleConnecterPinmuxer";
            break;

        case ELogModuleCloudConnecterServerAgent:
            module_string = "ELogModuleCloudConnecterServerAgent";
            break;

        case ELogModuleCloudConnecterClientAgent:
            module_string = "ELogModuleCloudConnecterClientAgent";
            break;

        case ELogModuleFlowController:
            module_string = "ELogModuleFlowController";
            break;

        case ELogModulePrivateRTSPDemuxer:
            module_string = "ELogModulePrivateRTSPDemuxer";
            break;

        case ELogModulePrivateRTSPScheduledDemuxer:
            module_string = "ELogModulePrivateRTSPScheduledDemuxer";
            break;

        case ELogModuleUploadingReceiver:
            module_string = "ELogModuleUploadingReceiver";
            break;

        case ELogModuleAmbaDecoder:
            module_string = "ELogModuleAmbaDecoder";
            break;

        case ELogModuleAmbaVideoPostProcessor:
            module_string = "ELogModuleAmbaVideoPostProcessor";
            break;

        case ELogModuleAmbaVideoRenderer:
            module_string = "ELogModuleAmbaVideoRenderer";
            break;

        case ELogModuleAmbaEncoder:
            module_string = "ELogModuleAmbaEncoder";
            break;

        case ELogModuleStreamingTransmiter:
            module_string = "ELogModuleStreamingTransmiter";
            break;

        case ELogModuleFFMpegDemuxer:
            module_string = "ELogModuleFFMpegDemuxer";
            break;

        case ELogModuleFFMpegMuxer:
            module_string = "ELogModuleFFMpegMuxer";
            break;

        case ELogModuleFFMpegAudioDecoder:
            module_string = "ELogModuleFFMpegAudioDecoder";
            break;

        case ELogModuleFFMpegVideoDecoder:
            module_string = "ELogModuleFFMpegVideoDecoder";
            break;

        case ELogModuleAudioRenderer:
            module_string = "ELogModuleAudioRenderer";
            break;

        case ELogModuleAudioInput:
            module_string = "ELogModuleAudioInput";
            break;

        case ELogModuleAACAudioDecoder:
            module_string = "ELogModuleAACAudioDecoder";
            break;

        case ELogModuleAACAudioEncoder:
            module_string = "ELogModuleAACAudioEncoder";
            break;

        case ELogModuleCustomizedAudioDecoder:
            module_string = "ELogModuleCustomizedAudioDecoder";
            break;

        case ELogModuleCustomizedAudioEncoder:
            module_string = "ELogModuleCustomizedAudioEncoder";
            break;

        case ELogModuleNativeTSMuxer:
            module_string = "ELogModuleNativeTSMuxer";
            break;

        case ELogModuleRTSPStreamingServer:
            module_string = "ELogModuleRTSPStreamingServer";
            break;

        case ELogModuleCloudServer:
            module_string = "ELogModuleCloudServer";
            break;

        case ELogModuleDSPPlatform:
            module_string = "ELogModuleDSPPlatform";
            break;

        case ELogModuleDSPDec:
            module_string = "ELogModuleDSPDec";
            break;

        case ELogModuleDSPEnc:
            module_string = "ELogModuleDSPEnc";
            break;

        case ELogModuleRunRobinScheduler:
            module_string = "ELogModuleRunRobinScheduler";
            break;

        case ELogModulePreemptiveScheduler:
            module_string = "ELogModulePreemptiveScheduler";
            break;

        case ELogModuleThreadPool:
            module_string = "ELogModuleThreadPool";
            break;

        case ELogModuleLightWeightDataBase:
            module_string = "ELogModuleLightWeightDataBase";
            break;

        case ELogModuleAccountManager:
            module_string = "ELogModuleAccountManager";
            break;

        case ELogModuleSACPAdminAgent:
            module_string = "ELogModuleSACPAdminAgent";
            break;

        case ELogModuleSACPIMAgent:
            module_string = "ELogModuleSACPIMAgent";
            break;

        case ELogAPPFrameworkSceneDirector:
            module_string = "ELogAPPFrameworkSceneDirector";
            break;

        default:
            LOG_ERROR("Unknown module %d\n", module);
            module_string = "UnkwonModuleXX";
            err = EECode_BadParam;
            break;
    }

    return err;
}

EECode gfGetLogModuleIndex(const TChar *module_name, TUint &index)
{
    TUint i = 0;
    EECode err = EECode_OK;
    const TChar *module_string = NULL;
    TUint string_len = 0;

    if (!module_name) {
        LOG_FATAL("NULL module_name\n");
        return EECode_BadParam;
    }

    string_len = strlen(module_name);
    for (i = 0; i < ELogModuleListEnd; i ++) {
        err = gfGetLogModuleString((ELogModule)i, module_string);
        DASSERT_OK(err);
        if (EECode_OK == err) {
            DASSERT(module_string);
            if ((string_len == strlen(module_string)) && (!strncmp(module_string, module_name, strlen(module_string)))) {
                index = i;
                return EECode_OK;
            }
        } else {
            LOG_FATAL("please check code, i %d\n", i);
            return EECode_InternalLogicalBug;
        }
    }

    index = 0;
    return EECode_BadParam;
}

#ifdef DCONFIG_USE_NATIVE_LOG_SYSTEM

FILE *gpLogOutputFile = NULL;
FILE *gpSnapshotOutputFile = NULL;

TU32 isLogFileOpened()
{
    if (gpLogOutputFile) {
        return 1;
    }

    return 0;
}

void gfOpenLogFile(TChar *name)
{
    if (DLikely(name)) {
        //DASSERT(!gpLogOutputFile);
        if (DUnlikely(gpLogOutputFile)) {
            fclose(gpLogOutputFile);
            gpLogOutputFile = NULL;
        }

        gpLogOutputFile = fopen(name, "wt");
        if (DLikely(gpLogOutputFile)) {
            LOG_PRINTF("open log file(%s) success\n", name);
        } else {
            LOG_ERROR("open log file(%s) fail\n", name);
        }
    }
}

void gfCloseLogFile()
{
    if (DLikely(gpLogOutputFile)) {
        fclose(gpLogOutputFile);
        gpLogOutputFile = NULL;
    }
}

void gfSetLogLevel(ELogLevel level)
{
    gmModuleLogConfig[ELogGlobal].log_level = level;
}

void gfOpenSnapshotFile(TChar *name)
{
    if (DLikely(name)) {
        //DASSERT(!gpSnapshotOutputFile);
        if (DUnlikely(gpSnapshotOutputFile)) {
            fclose(gpSnapshotOutputFile);
            gpSnapshotOutputFile = NULL;
        }

        gpSnapshotOutputFile = fopen(name, "wt");
        if (DLikely(gpSnapshotOutputFile)) {
            LOG_PRINTF("open snapshot file(%s) success\n", name);
        } else {
            LOG_ERROR("open snapshot file(%s) fail\n", name);
        }
    }
}

void gfCloseSnapshotFile()
{
    if (DLikely(gpSnapshotOutputFile)) {
        fclose(gpSnapshotOutputFile);
        gpSnapshotOutputFile = NULL;
    }
}

#endif

void gfPrintMemory(TU8 *p, TU32 size)
{
    if (DLikely(p)) {
        while (size > 7) {
            __CLEAN_PRINT("%02x %02x %02x %02x %02x %02x %02x %02x\n", p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
            p += 8;
            size -= 8;
        }

        if (size) {
            while (size) {
                __CLEAN_PRINT("%02x ", p[0]);
                p ++;
                size --;
            }
            __CLEAN_PRINT("\n");
        }
    }
}

//check memory
void *gfDbgMalloc(TU32 size, const TChar *tag)
{
    void *p = malloc(size);
    LOG_PRINTF("[mem check, malloc], return %p, size %d, tag[%c%c%c%c]\n", p, size, tag[0], tag[1], tag[2], tag[3]);
    return p;
}

void gfDbgFree(void *p, const TChar *tag)
{
    LOG_PRINTF("[mem check, release], %p, tag[%c%c%c%c]\n", p, tag[0], tag[1], tag[2], tag[3]);
    free(p);
    return;
}

void *gfDbgCalloc(TU32 n, TU32 size, const TChar *tag)
{
    void *p = calloc(n, size);
    LOG_PRINTF("[mem check, calloc], return %p, size %d*%d, tag[%c%c%c%c]\n", p, n, size, tag[0], tag[1], tag[2], tag[3]);
    return p;
}

void *gfDbgRemalloc(void *p, TU32 size, const TChar *tag)
{
    void *pr = realloc(p, size);
    LOG_PRINTF("[mem check, remalloc], return %p, ori %p, size %d, tag[%c%c%c%c]\n", pr, p, size, tag[0], tag[1], tag[2], tag[3]);
    return pr;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

