/*******************************************************************************
 * media_mw_utils.h
 *
 * History:
 *  2013/04/19 - [Zhi He] create file
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

#ifndef __MEDIA_MW_UTILS_H__
#define __MEDIA_MW_UTILS_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

#define MaxStreamNumber  8
#define DMaxFileExterntionLength 32
#define DMaxFileIndexLength 32

//some constant
enum {
    EConstMaxDemuxerMuxerStreamNumber = 4,

    EConstDemxerMaxOutputPinNumber = 8,

    EConstVideoEncoderMaxInputPinNumber = 4,
    EConstVideoDecoderMaxInputPinNumber = 16,
    EConstAudioDecoderMaxInputPinNumber = 32,
    EConstAudioRendererMaxInputPinNumber = 8,
    EConstMuxerMaxInputPinNumber = 4,
    EConstVideoPostProcessorMaxInputPinNumber = 16,
    EConstVideoRendererMaxInputPinNumber = 16,

    EConstStreamingTransmiterMaxInputPinNumber = 128,

    EConstPinmuxerMaxInputPinNumber = 32,
};

//struct

typedef struct {
    TU8 total_stream_number;
    TU8 reserved0, reserved1, reserved2;
    SStreamCodecInfo info[EConstMaxDemuxerMuxerStreamNumber];
} SStreamCodecInfos;

EECode gfUpdateHighlightenDisplayLayout(SVideoPostPGlobalSetting *p_global_setting, SVideoPostPDisplayLayout *cur_config, SVideoPostPDisplayHighLightenWindowSize *highlighten_window_size, SVideoPostPConfiguration *p_result);

const TChar *gfGetContainerStringFromType(ContainerType type);
const TChar *gfGetMediaDeviceWorkingModeString(EMediaWorkingMode mode);
const TChar *gfGetPlaybackModeString(EPlaybackMode mode);
const TChar *gfGetDSPOperationModeString(EDSPOperationMode mode);

StreamFormat gfGetStreamFormatFromString(TChar *str);
EntropyType gfGetEntropyTypeFromString(char *str);
ContainerType gfGetContainerTypeFromString(char *str);
AudioSampleFMT gfGetSampleFormatFromString(char *str);

void gfLoadDefaultMediaConfig(volatile SPersistMediaConfig *pconfig);
void gfLoadDefaultMultipleWindowPlaybackConfig(volatile SPersistMediaConfig *pconfig);
void gfLoadDefaultLogConfig(SLogConfig *p_config, TUint max_element_number);

void gfSetLogConfigLevel(ELogLevel level, SLogConfig *p_config, TUint max_element_number);
EECode gfPrintLogConfig(SLogConfig *p_config, TUint max_element_number);

EECode gfGenerateMediaConfigFile(ERunTimeConfigType config_type, const volatile SPersistMediaConfig *p_config, const TChar *config_file_name);
EECode gfSaveLogConfigFile(ERunTimeConfigType config_type, const TChar *config_file_name, SLogConfig *p_config, TUint max_element_number);
EECode gfLoadLogConfigFile(ERunTimeConfigType config_type, const TChar *config_file_name, SLogConfig *p_config, TUint max_element_number);
EECode gfLoadSimpleLogConfigFile(TChar *simple_configfile, TUint &loglevel, TUint &logoption, TUint &logoutput);
EECode gfReadLogConfigFile(ERunTimeConfigType config_type, const TChar *config_file_name);
EECode gfReadMediaConfigFile(ERunTimeConfigType config_type, const TChar *config_file_name, SPersistMediaConfig *p_config);
EECode gfWriteMediaConfigFile(ERunTimeConfigType config_type, const volatile SPersistMediaConfig *p_config, const TChar *config_file_name);
EECode gfWriteLogConfigFile(ERunTimeConfigType config_type, const TChar *config_file_name);

IScheduler *gfGetNetworkReceiverScheduler(const volatile SPersistMediaConfig *p, TUint index);
IScheduler *gfGetNetworkReceiverTCPScheduler(const volatile SPersistMediaConfig *p, TUint index);
IScheduler *gfGetFileIOWriterScheduler(const volatile SPersistMediaConfig *p, TU32 index);
ITCPPulseSender *gfGetTCPPulseSender(const volatile SPersistMediaConfig *p, TUint index);

void gfPrintCodecInfoes(SStreamCodecInfos *pInfo);

#define SPS_PPS_LEN (57+8+6)
bool gfGetSpsPps(TU8 *pBuffer, TUint size, TU8 *&p_spspps, TUint &spspps_size, TU8 *&p_IDR, TU8 *&p_ret_data);

TMemSize gfSkipDelimter(TU8 *p);
TU32 gfSkipSEI(TU8 *p, TU32 len);

//StreamFormat gfGetStreamingFormatFromSACPSubType(ESACPDataChannelSubType sub_type);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif


