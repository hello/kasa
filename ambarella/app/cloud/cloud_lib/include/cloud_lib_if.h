/*******************************************************************************
 * cloud_lib_if.h
 *
 * History:
 *  2013/11/27 - [Zhi He] create file
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

#ifndef __CLOUD_LIB_IF_H__
#define __CLOUD_LIB_IF_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

#define DDynamicInputStringLength 8
#define DIdentificationStringLength 16

typedef enum {
    EAuthenticationType_Default = 0x00,

    EAuthenticationType_MD5 = 0x01,
    EAuthenticationType_SHA1 = 0x02,
    EAuthenticationType_SHA224 = 0x03,
    EAuthenticationType_SHA256 = 0x04,
    EAuthenticationType_SHA384 = 0x05,
    EAuthenticationType_SHA512 = 0x06,

} EAuthenticationType;

typedef enum {
    EEncryptionType_None = 0,

    EEncryptionType_DES = 0x01,
    EEncryptionType_3DES = 0x02,
    EEncryptionType_RSA = 0x03,
    EEncryptionType_AES = 0x04,

    EEncryptionType_Customized_1 = 0x80,
    EEncryptionType_Customized_2 = 0x81,
    EEncryptionType_Customized_3 = 0x82,
} EEncryptionType;

typedef enum {
    EProtocolHeaderExtentionType_Invalid = 0x00,
    EProtocolHeaderExtentionType_NoHeader = 0x01,
    EProtocolHeaderExtentionType_SACP_IM = 0x02,
    EProtocolHeaderExtentionType_SACP_ADMIN = 0x03,

    EProtocolHeaderExtentionType_Customized = 0x10,
} EProtocolHeaderExtentionType;

typedef enum {
    EProtocolType_Invalid = 0,
    EProtocolType_SACP,
    EProtocolType_ONVIF,
} EProtocolType;

typedef enum {
    ESACPDataChannelSubType_Invalid = 0x0,

    ESACPDataChannelSubType_H264_NALU = 0x01,
    ESACPDataChannelSubType_AAC = 0x02,
    ESACPDataChannelSubType_MPEG12Audio = 0x03,
    ESACPDataChannelSubType_G711_PCMU = 0x04,
    ESACPDataChannelSubType_G711_PCMA = 0x05,

    ESACPDataChannelSubType_RAW_PCM_S16 = 0x06,

    ESACPDataChannelSubType_Cusomized_ADPCM_S16 = 0x07,

    ESACPDataChannelSubType_AudioFlowControl_Pause = 0x90,
    ESACPDataChannelSubType_AudioFlowControl_EOS = 0x91,

} ESACPDataChannelSubType;

typedef enum {
    ESACPConnectResult_OK = 0x0,

    ESACPConnectResult_Reject_NoSuchChannel = 0x1,
    ESACPConnectResult_Reject_ChannelIsInUse = 0x2,
    ESACPConnectResult_Reject_BadRequestFormat = 0x3,
    ESACPConnectResult_Reject_CorruptedProtocol = 0x4,
    ESACPConnectResult_Reject_AuthenticationDataTooLong = 0x5,
    ESACPConnectResult_Reject_NotProperPassword = 0x6,
    ESACPConnectResult_Reject_NotSupported = 0x7,
    ESACPConnectResult_Reject_AuthenticationFail = 0x8,
    ESACPConnectResult_Reject_Unknown = 0x9,

    ESACPRequestResult_BadState = 0xa,
    ESACPRequestResult_NotExist = 0xb,
    ESACPRequestResult_BadParam = 0xc,
    ESACPRequestResult_ServerLogicalBug = 0xd,
    ESACPRequestResult_ServerNotSupport = 0xe,
    ESACPRequestResult_Server_NeedHardwareAuthenticate = 0xf,

#if 0
    ESACPConnectResult_Reject_NoUsername = 0x20,
    ESACPConnectResult_Reject_TooLongUsername = 0x21,
    ESACPConnectResult_Reject_NoPassword = 0x22,
    ESACPConnectResult_Reject_TooLongPassword = 0x23,
#endif

    ESACPRequestResult_NotOnline = 0x24,
    ESACPRequestResult_NoPermission = 0x25,
    ESACPRequestResult_AlreadyExist = 0x26,

    ESACPResult_NoRelatedComponent = 0x30,

    ESACPRequestResult_PossibleAttackerFromNetwork = 0x40,

    ESACPRequestResult_NeedSetOwner = 0x60,
    ESACPRequestResult_Reject_AnotherLogin = 0x61,
} ESACPConnectResult;

#define DSACPHeaderFlagBit_PacketStartIndicator (1 << 7)
#define DSACPHeaderFlagBit_PacketEndIndicator (1 << 6)
#define DSACPHeaderFlagBit_PacketKeyFrameIndicator (1 << 5)
#define DSACPHeaderFlagBit_PacketExtraDataIndicator (1 << 4)
#define DSACPHeaderFlagBit_ReplyResult (1 << 3)

//-----------------------------------------------------------------------
//
//  ICloudClientAgent
//
//-----------------------------------------------------------------------

typedef struct {
    TU16 native_major;
    TU8 native_minor;
    TU8 native_patch;

    TU16 native_date_year;
    TU8 native_date_month;
    TU8 native_date_day;

    TU16 peer_major;
    TU8 peer_minor;
    TU8 peer_patch;

    TU16 peer_date_year;
    TU8 peer_date_month;
    TU8 peer_date_day;
} SCloudAgentVersion;

#define DMaxCloudTagNameLength 32
#define DMaxCloudChannelNumber 64

typedef struct {
    TChar source_tag[DMaxCloudTagNameLength];

    TU16 channel_index;
    TU8 streaming_ready;
    TU8 remote_control_enabled;
} SCloudSourceItem;

typedef struct {
    TU16 items_number;
    TU8 reserved0;
    TU8 reserved1;

    SCloudSourceItem items[DMaxCloudChannelNumber];
} SCloudSourceItems;

typedef struct {
    //input
    float float_window_pos_x, float_window_pos_y;
    float float_window_width, float_window_height;

    //actual
    TU32 window_pos_x, window_pos_y;
    TU32 window_width, window_height;

    TU16 udec_index;
    TU8 render_index;
    TU8 display_disable;
} SDisplayItem;

typedef struct {
    //input
    float float_zoom_size_x, float_zoom_size_y;
    float float_zoom_input_center_x, float_zoom_input_center_y;

    //actual
    TU32 zoom_size_x, zoom_size_y;
    TU32 zoom_input_center_x, zoom_input_center_y;

    TU16 udec_index;
    TU8 is_valid;
    TU8 reserved1;
} SZoomItem;

typedef struct {
    TU32 pic_width, pic_height;
    TU32 bitrate, framerate;
} SSourceInfo;

#define DQUERY_MAX_DISPLAY_WINDOW_NUMBER 16
#define DQUERY_MAX_UDEC_NUMBER 16
#define DQUERY_MAX_SOURCE_NUMBER 16

typedef struct {
    float zoom_factor;
    float center_x, center_y;
} SZoomAccumulatedParameters;

typedef struct {
    SCloudAgentVersion version;//output only

    //NVR transcode related

    //return persist config
    TU8 channel_number_per_group;//output only
    TU8 total_group_number;//output only
    TU8 current_group_index;//output only
    TU8 have_dual_stream;//output only

    TU8 is_vod_enabled;//output only
    TU8 is_vod_ready;//output only
    TU8 update_group_info_flag;
    TU8 update_display_layout_flag;

    //input
    TU8 update_source_flag;
    TU8 update_display_flag;
    TU8 update_audio_flag;
    TU8 update_zoom_flag;

    //audio related
    TU16 audio_source_index;//in, out
    TU16 audio_target_index;//in, out

    //encoding parameters
    TU32 encoding_width;//in, out
    TU32 encoding_height;//in, out
    TU32 encoding_bitrate;//in, out
    TU32 encoding_framerate;//in, out

    //display related
    TU16 display_layout;//in, out
    TU16 display_hd_channel_index;//in, out

    //internal use
    TU8 show_other_windows;
    TU8 focus_window_id;
    TU8 current_group;
    TU8 reserved1;

    TU16 total_window_number;//output only
    TU16 current_display_window_number;
    SDisplayItem window[DQUERY_MAX_DISPLAY_WINDOW_NUMBER];
    SZoomItem zoom[DQUERY_MAX_UDEC_NUMBER];

    //blow not used yet
    SSourceInfo source_info[DQUERY_MAX_SOURCE_NUMBER];

    //history data
    SZoomAccumulatedParameters window_zoom_history[DQUERY_MAX_DISPLAY_WINDOW_NUMBER];
} SSyncStatus;

typedef enum {
    ERemoteTrickplay_Invalid = 0,
    ERemoteTrickplay_Pause,
    ERemoteTrickplay_Resume,
    ERemoteTrickplay_Step,
    ERemoteTrickplay_EOS,
} ERemoteTrickplay;

typedef enum {
    ERemoteFlowControl_Invalid = 0,
    ERemoteFlowControl_AudioPause = ESACPDataChannelSubType_AudioFlowControl_Pause,
    ERemoteFlowControl_AudioEOS = ESACPDataChannelSubType_AudioFlowControl_EOS,
} ERemoteFlowControl;

class ICloudClientAgent
{
public:
    virtual EECode ConnectToServer(TChar *server_url, TU64 &hardware_authenication_input, TU16 local_port = 0, TU16 server_port = 0, TU8 *authentication_buffer = NULL, TU16 authentication_length = 0) = 0;
    //virtual EECode HardwareAuthenticate(TU32 hash_value) = 0;
    virtual EECode DisconnectToServer() = 0;

    //misc api
public:
    virtual EECode QueryVersion(SCloudAgentVersion *version) = 0;

    //virtual EECode QuerySourceList(SCloudSourceItems* items) = 0;
    //virtual EECode AquireChannelControl(TU32 channel_index) = 0;
    //virtual EECode ReleaseChannelControl(TU32 channel_index) = 0;

    virtual EECode GetLastErrorHint(EECode &last_error_code, TU32 &error_hint1, TU32 &error_hint2) = 0;

public:
    virtual EECode Uploading(TU8 *p_data, TMemSize size, ESACPDataChannelSubType data_type, TU8 extra_flag = 0) = 0;

    //cmd channel to NVR
public:
    virtual EECode UpdateSourceBitrate(TChar *source_channel, TU32 bitrate, TU32 reply = 1) = 0;
    virtual EECode UpdateSourceFramerate(TChar *source_channel, TU32 framerate, TU32 reply = 1) = 0;
    virtual EECode UpdateSourceDisplayLayout(TChar *source_channel, TU32 layout, TU32 focus_index = 0, TU32 reply = 0) = 0;//layout: 0: single HD, 1 MxN, 2: highlighten view, 3: telepresence
    virtual EECode UpdateSourceAudioParams(TU8 audio_format, TU8 audio_channnel_number, TU32 audio_sample_frequency, TU32 audio_bitrate, TU8 *audio_extradata, TU16 audio_extradata_size, TU32 reply = 0) = 0;

    virtual EECode SwapWindowContent(TChar *source_channel, TU32 window_id_1, TU32 window_id_2, TU32 reply = 0) = 0;
    virtual EECode CircularShiftWindowContent(TChar *source_channel, TU32 shift_count = 1, TU32 is_ccw = 0, TU32 reply = 0) = 0;

    virtual EECode SwitchVideoHDSource(TChar *source_channel, TU32 hd_index, TU32 reply = 0) = 0;
    virtual EECode SwitchAudioSourceIndex(TChar *source_channel, TU32 audio_index, TU32 reply = 0) = 0;
    virtual EECode SwitchAudioTargetIndex(TChar *source_channel, TU32 audio_index, TU32 reply = 0) = 0;
    virtual EECode ZoomSource(TChar *source_channel, TU32 window_index, float zoom_factor, float zoom_input_center_x, float zoom_input_center_y, TU32 reply = 0) = 0;
    virtual EECode ZoomSource2(TChar *source_channel, TU32 window_index, float width_factor, float height_factor, float input_center_x, float input_center_y, TU32 reply = 0) = 0;
    virtual EECode UpdateSourceWindow(TChar *source_channel, TU32 window_index, float pos_x, float pos_y, float size_x, float size_y, TU32 reply = 0) = 0;
    virtual EECode SourceWindowToTop(TChar *source_channel, TU32 window_index, TU32 reply = 0) = 0;
    virtual EECode ShowOtherWindows(TChar *source_channel, TU32 window_index, TU32 show, TU32 reply = 0) = 0;

    virtual EECode DemandIDR(TChar *source_channel, TU32 demand_param = 0, TU32 reply = 0) = 0;

    virtual EECode SwitchGroup(TUint group = 0, TU32 reply = 0) = 0;
    virtual EECode SeekTo(SStorageTime *time, TU32 reply = 0) = 0;
    virtual EECode TrickPlay(ERemoteTrickplay trickplay, TU32 reply = 0) = 0;
    virtual EECode FastForward(TUint try_speed, TU32 reply = 0) = 0;//16.16 fixed point
    virtual EECode FastBackward(TUint try_speed, TU32 reply = 0) = 0;//16.16 fixed point

    //for extention
    //virtual EECode CustomizedCommand(TChar* source_channel, TU32 param1, TU32 param2, TU32 param3, TU32 param4, TU32 param5, TU32 reply = 0) = 0;

    //overall API
    virtual EECode QueryStatus(SSyncStatus *status, TU32 reply = 0) = 0;
    virtual EECode SyncStatus(SSyncStatus *status, TU32 reply = 0) = 0;

    //cmd from server
public:
    virtual EECode WaitCmd(TU32 &cmd_type) = 0;
    virtual EECode PeekCmd(TU32 &type, TU32 &payload_size, TU16 &header_ext_size) = 0;
    virtual EECode ReadData(TU8 *p_data, TMemSize size) = 0;
    virtual EECode WriteData(TU8 *p_data, TMemSize size) = 0;

public:
    virtual TInt GetHandle() const = 0;

public:
    virtual EECode DebugPrintCloudPipeline(TU32 index, TU32 param_0, TU32 reply = 0) = 0;
    virtual EECode DebugPrintStreamingPipeline(TU32 index, TU32 param_0, TU32 reply = 0) = 0;
    virtual EECode DebugPrintRecordingPipeline(TU32 index, TU32 param_0, TU32 reply = 0) = 0;
    virtual EECode DebugPrintChannel(TU32 index, TU32 param_0, TU32 reply = 0) = 0;
    virtual EECode DebugPrintAllChannels(TU32 param_0, TU32 reply = 0) = 0;

    //for externtion, textable API
    //    virtual EECode SendStringJson(TChar* input, TU32 input_size, TChar* output, TU32 output_size, TU32 reply = 1) = 0;
    //    virtual EECode SendStringXml(TChar* input, TU32 input_size, TChar* output, TU32 output_size, TU32 reply = 1) = 0;

public:
    virtual void Delete() = 0;

protected:
    virtual ~ICloudClientAgent() {};
};

typedef TU32(*TClientAgentAuthenticateCallBack)(TU8 *p_input, TU32 input_length);

#define DBuildupVideoFrameRate(den, framerate) (((TU32)(den & 0x00ffffff) << 8) & ((TU32)(framerate & 0xff)))
#define DParseVideoFrameRate(den, framerate, input) do { \
        framerate = input & 0xff; \
        den = (input >> 8) & 0xffffff; \
    } while (0)

//#define DServerNeedSync(x) (!(x & (0x3ff)))
#define DClientNeedSync(x) (!(x & (0xff)))
#define DBuildGOP(M, N, IDRIntervel) (((((TU32)IDRIntervel) & 0xff) << 24) | ((((TU32)N) & 0xffff) << 8) | (((TU32)M) & 0xff))
#define DParseGOP(M, N, IDRIntervel, gop) do { \
        M = gop & 0xff; \
        N = (gop >> 8) & 0xffff; \
        IDRIntervel = (gop >> 24) & 0xff; \
    } while (0)

class ICloudClientAgentV2
{
public:
    //connect, authenticate API
    virtual EECode ConnectToServer(TChar *server_url, TU16 server_port, TChar *account_name, TChar *password, TU16 local_port = 0) = 0;
    virtual EECode SetHardwareAuthenticateCallback(TClientAgentAuthenticateCallBack callback) = 0;
    virtual EECode DisconnectToServer() = 0;

public:
    //setup API
    virtual EECode SetupVideoParams(ESACPDataChannelSubType video_format, TU32 framerate, TU32 res_width, TU32 res_height, TU32 bitrate, TU8 *extradata, TU16 extradata_size, TU32 gop = 0) = 0;
    virtual EECode SetupAudioParams(ESACPDataChannelSubType audio_format, TU8 audio_channnel_number, TU32 audio_sample_frequency, TU32 audio_bitrate, TU8 *extradata, TU16 extradata_size) = 0;

public:
    //upload data
    virtual EECode Uploading(TU8 *p_data, TMemSize size, ESACPDataChannelSubType data_type, TU32 seq_number, TU8 extra_flag = 0) = 0;

public:
    //sync API
    virtual EECode VideoSync(TTime video_current_time, TU32 video_seq_number, ETimeUnit time_unit = ETimeUnit_native) = 0;
    virtual EECode AudioSync(TTime audio_current_time, TU32 audio_seq_number, ETimeUnit time_unit = ETimeUnit_native) = 0;
    virtual EECode DiscontiousIndicator() = 0;

    //cmd channel to data server
public:
    virtual EECode UpdateVideoBitrate(TU32 bitrate) = 0;
    virtual EECode UpdateVideoFramerate(TU32 framerate) = 0;

    //debug channel
public:
    virtual EECode DebugPrintCloudPipeline(TU32 index, TU32 param_0) = 0;
    virtual EECode DebugPrintStreamingPipeline(TU32 index, TU32 param_0) = 0;
    virtual EECode DebugPrintRecordingPipeline(TU32 index, TU32 param_0) = 0;
    virtual EECode DebugPrintChannel(TU32 index, TU32 param_0) = 0;
    virtual EECode DebugPrintAllChannels(TU32 param_0) = 0;

public:
    virtual void Delete() = 0;

protected:
    virtual ~ICloudClientAgentV2() {};
};

typedef EECode(*TServerAgentCmdCallBack)(void *owner, TU32 cmd_type, TU32 params0, TU32 params1, TU32 params2, TU32 params3, TU32 params4, TU8 *pcmd, TU32 cmdsize);
typedef EECode(*TServerAgentDataCallBack)(void *owner, TMemSize read_length, TU16 data_type, TU8 flag);

typedef EECode(*TServerAgentV2CmdCallBack)(void *owner, TU32 cmd_type, TU8 *pcmd, TU32 cmdsize);
typedef EECode(*TServerAgentV2DataCallBack)(void *owner, TU32 read_length, TU16 data_type, TU32 seq_num, TU8 flag);

class ICloudServerAgent: public IScheduledClient
{
public:
    virtual EECode AcceptClient(TInt socket) = 0;
    virtual EECode CloseConnection() = 0;

    virtual EECode ReadData(TU8 *p_data, TMemSize size) = 0;
    virtual EECode WriteData(TU8 *p_data, TMemSize size) = 0;

#ifdef DCONFIG_COMPILE_OLD_VERSION_FILE
    virtual EECode SetProcessCMDCallBack(void *owner, TServerAgentCmdCallBack callback) = 0; //to be obsolete
    virtual EECode SetProcessDataCallBack(void *owner, TServerAgentDataCallBack callback) = 0; //to be obsolete
#endif

    virtual EECode SetProcessCMDCallBackV2(void *owner, TServerAgentV2CmdCallBack callback) = 0;
    virtual EECode SetProcessDataCallBackV2(void *owner, TServerAgentV2DataCallBack callback) = 0;

public:
    virtual void Destroy() = 0;

protected:
    virtual ~ICloudServerAgent() {}
};

#ifdef DCONFIG_COMPILE_OLD_VERSION_FILE
extern ICloudClientAgent *gfCreateCloudClientAgent(EProtocolType type = EProtocolType_SACP, TU16 local_port = DDefaultClientPort, TU16 server_port = DDefaultSACPServerPort);
#endif

extern ICloudClientAgentV2 *gfCreateCloudClientAgentV2(EProtocolType type = EProtocolType_SACP);
extern ICloudServerAgent *gfCreateCloudServerAgent(EProtocolType type = EProtocolType_SACP, TU32 version = 1);

extern ECMDType gfSACPClientSubCmdTypeToGenericCmdType(TU16 sub_type);
extern EECode gfSACPConnectResultToErrorCode(ESACPConnectResult result);
extern ESACPConnectResult gfSACPErrorCodeToConnectResult(EECode code);

typedef enum {
    ESACPElementType_Invalid = 0x0000,

    ESACPElementType_GlobalSetting = 0x0001,
    ESACPElementType_SyncFlags = 0x0002,

    ESACPElementType_EncodingParameters = 0x0003,
    ESACPElementType_DisplaylayoutInfo = 0x0004,
    ESACPElementType_DisplayWindowInfo = 0x0005,
    ESACPElementType_DisplayZoomInfo = 0x0006,

} ESACPElementType;

typedef struct {
    TU8 type1;
    TU8 type2;
    TU8 size1;
    TU8 size2;
} SSACPElementHeader;

#define DSACP_FIX_POINT_DEN 0x01000000
#define DSACP_FIX_POINT_SYGN_FLAG 0x80000000

#define DSACP_MAX_PAYLOAD_LENGTH 1440

extern TMemSize gfWriteSACPElement(TU8 *p_src, TMemSize max_size, ESACPElementType type, TU32 param0, TU32 param1, TU32 param2, TU32 param3, TU32 param4, TU32 param5, TU32 param6, TU32 param7);
extern TMemSize gfReadSACPElement(TU8 *p_src, TMemSize max_size, ESACPElementType &type, TU32 &param0, TU32 &param1, TU32 &param2, TU32 &param3, TU32 &param4, TU32 &param5, TU32 &param6, TU32 &param7);

//version related
#define DCloudLibVesionMajor 0
#define DCloudLibVesionMinor 6
#define DCloudLibVesionPatch 11
#define DCloudLibVesionYear 2015
#define DCloudLibVesionMonth 9
#define DCloudLibVesionDay 7

void gfGetCloudLibVersion(TU32 &major, TU32 &minor, TU32 &patch, TU32 &year, TU32 &month, TU32 &day);

class IProtocolHeaderParser
{
public:
    virtual void Destroy() = 0;

protected:
    virtual ~IProtocolHeaderParser() {}

public:
    virtual TU32 GetFixedHeaderLength() const = 0;

    virtual void Parse(TU8 *header, TU32 &payload_size, TU32 &ext_type, TU32 &ext_size, TU32 &type, TU32 &cat, TU32 &sub_type) const = 0;

    virtual void Parse(TU8 *header, TU32 &payload_size, TU32 &reqres_bits, TU32 &cat, TU32 &sub_type) const = 0;

    virtual void ParseWithTargetID(TU8 *header, TU32 &total_size, TUniqueID &target_id) const = 0;

    virtual void ParseTargetID(TU8 *header, TUniqueID &target_id) const = 0;

    virtual void FillSourceID(TU8 *header, TUniqueID id) = 0;

    virtual void SetFlag(TU8 *header, TULong flag) = 0;

    virtual void SetReqResBits(TU8 *header, TU32 bits) = 0;

    virtual void SetPayloadSize(TU8 *header, TU32 size) = 0;
};

extern IProtocolHeaderParser *gfCreatePotocolHeaderParser(EProtocolType type = EProtocolType_SACP, EProtocolHeaderExtentionType extensiontype = EProtocolHeaderExtentionType_SACP_IM);

#define DIMServerPriviledgeRead (1 << 0)
#define DIMServerPriviledgeUpdate (1 << 1)

typedef void (*TReceiveMessageCallBack)(void *owner, TUniqueID id, TU8 *p_message, TU32 message_length, TU32 need_reply);
typedef void (*TSystemNotifyCallBack)(void *owner, ESystemNotify notify, void *notify_context);
typedef void (*TErrorCallBack)(void *owner, EECode err);

enum {
    EDeviceStatus_offline = 0x00,
    EDeviceStatus_standby = 0x01,
    EDeviceStatus_streaming = 0x02,
};

typedef struct {
    TChar name[DMAX_ACCOUNT_NAME_LENGTH_EX];
    TChar nickname[DMAX_ACCOUNT_NAME_LENGTH_EX];
    TChar dataserver_address[DMaxServerUrlLength];
    TU32 storage_capacity;
    TU8 dynamic_code[DDynamicInputStringLength];

    TSocketPort rtsp_streaming_port;
    TU8 status;
    TU8 reserved0;
} SDeviceProfile;

typedef struct {
    TU8 online;
    TU8 reserved0;
    TU8 reserved1;
    TU8 reserved2;
} SFriendInfo;

typedef struct {
    TUniqueID *p_id_list;
    TU32 id_count;

    TU32 max_id_count;
} SAgentIDListBuffer;

typedef struct {
    TChar question1[DMAX_SECURE_QUESTION_LENGTH];
    TChar question2[DMAX_SECURE_QUESTION_LENGTH];

    TChar answer1[DMAX_SECURE_ANSWER_LENGTH];
    TChar answer2[DMAX_SECURE_ANSWER_LENGTH];
} SSecureQuestions;

//-----------------------------------------------------------------------
//
//  IIMClientAgentAsync
//
//-----------------------------------------------------------------------
class IIMClientAgentAsync
{
public:
    virtual EECode Register(TChar *name, TChar *password, const TChar *server_url, TU16 serverport) = 0;

public:
    virtual EECode AcquireOwnership(TChar *device_code) = 0;
    virtual EECode DonateOwnership(TChar *donate_target, TChar *device_code) = 0;

public:
    virtual EECode GrantPrivilegeSingleDevice(TUniqueID device_id, TUniqueID usr_id, TU32 privilege = 0) = 0;
    virtual EECode GrantPrivilege(TUniqueID *device_id, TU32 device_count, TUniqueID usr_id, TU32 privilege = 0) = 0;
    virtual EECode AddFriend(TChar *friend_user_name) = 0;
    virtual EECode AcceptFriend(TUniqueID friend_id) = 0;
    virtual EECode RemoveFriend(TUniqueID friend_id) = 0;

    virtual EECode RetrieveFriendList() = 0;
    virtual EECode RetrieveAdminDeviceList() = 0;
    virtual EECode RetrieveSharedDeviceList() = 0;

    virtual EECode QueryDeivceInfo(TUniqueID device_id) = 0;

public:
    virtual void SetIdentificationString(TChar *p_id_string, TU32 length = DIdentificationStringLength) = 0;

public:
    virtual EECode Login(TUniqueID id, TChar *authencation_string, TChar *server_url, TU16 server_port, TU16 local_port, TChar *&cloud_server_url, TU16 &cloud_server_port, TU64 &dynamic_code_input) = 0;
    virtual EECode LoginUserAccount(TChar *name, TChar *password, const TChar *server_url, TU16 serverport) = 0;
    virtual EECode Logout() = 0;

public:
    virtual EECode PostMessage(TUniqueID id, TU8 *message, TU32 message_length) = 0;
    virtual EECode SendMessage(TUniqueID id, TU8 *message, TU32 message_length) = 0;

public:
    virtual EECode SetReceiveMessageCallBack(void *owner, TReceiveMessageCallBack callback) = 0;

public:
    virtual void Destroy() = 0;

protected:
    virtual ~IIMClientAgentAsync() {}
};

#ifdef DCONFIG_COMPILE_OLD_VERSION_FILE
extern IIMClientAgentAsync *gfCreateIMClientAgentAsync(EProtocolType type = EProtocolType_SACP);
#endif

//-----------------------------------------------------------------------
//
//  IIMClientAgent
//
//-----------------------------------------------------------------------
class IIMClientAgent
{
public:
    virtual EECode Register(TChar *name, TChar *password, const TChar *server_url, TSocketPort server_port, TUniqueID &id) = 0;
    virtual EECode GetSecurityQuestion(TChar *name, SSecureQuestions *p_questions, const TChar *server_url, TSocketPort serverport) = 0;
    virtual EECode ResetPasswordBySecurityAnswer(TChar *name, SSecureQuestions *p_questions, TChar *new_password, TUniqueID &id, const TChar *server_url, TU16 serverport) = 0;

public:
    virtual EECode AcquireOwnership(TChar *device_code, TUniqueID &device_id) = 0;
    virtual EECode DonateOwnership(TChar *donate_target, TUniqueID device_id) = 0;

public:
    virtual EECode GrantPrivilegeSingleDevice(TUniqueID device_id, TUniqueID usr_id, TU32 privilege) = 0;
    virtual EECode GrantPrivilege(TUniqueID *device_id, TU32 device_count, TUniqueID usr_id, TU32 privilege) = 0;
    virtual EECode AddFriend(TChar *friend_user_name, TUniqueID &friend_id) = 0;
    virtual EECode AddFriendByID(TUniqueID friend_id, TChar *friend_user_name) = 0;
    virtual EECode AcceptFriend(TUniqueID friend_id) = 0;
    virtual EECode RemoveFriend(TUniqueID friend_id) = 0;

    virtual EECode RetrieveFriendList(SAgentIDListBuffer *&p_id_list) = 0;
    virtual EECode RetrieveAdminDeviceList(SAgentIDListBuffer *&p_id_list) = 0;
    virtual EECode RetrieveSharedDeviceList(SAgentIDListBuffer *&p_id_list) = 0;
    //release SAgentIDListBuffer for upper three functions
    virtual void FreeIDList(SAgentIDListBuffer *id_list) = 0;

    virtual EECode QueryDeivceProfile(TUniqueID device_id, SDeviceProfile *p_profile) = 0;
    virtual EECode QueryFriendInfo(TUniqueID friend_id, SFriendInfo *info) = 0;

public:
    virtual EECode Login(TUniqueID id, TChar *password, TChar *server_url, TSocketPort server_port, TChar *&cloud_server_url, TSocketPort &cloud_server_port, TU64 &dynamic_code_input) = 0;
    virtual EECode LoginUserAccount(TChar *name, TChar *password, const TChar *server_url, TSocketPort server_port, TUniqueID &id) = 0;
    virtual EECode LoginByUserID(TUniqueID id, TChar *password, const TChar *server_url, TSocketPort server_port) = 0;
    virtual EECode Logout() = 0;

public:
    virtual EECode SetOwner(TChar *owner) = 0;

public:
    virtual EECode PostMessage(TUniqueID id, TU8 *message, TU32 message_length, TU8 user_protocol) = 0;
    virtual EECode SendMessage(TUniqueID id, TU8 *message, TU32 message_length, TU8 user_protocol) = 0;

public:
    virtual EECode UpdateNickname(TChar *nickname) = 0;
    virtual EECode UpdateDeviceNickname(TUniqueID device_id, TChar *nickname) = 0;
    virtual EECode UpdatePassword(TChar *ori_password, TChar *password) = 0;

    virtual EECode UpdateSecurityQuestion(SSecureQuestions *p_questions, TChar *password) = 0;

public:
    virtual EECode AcquireDynamicCode(TUniqueID device_id, TChar *password, TU32 &dynamic_code) = 0;
    virtual EECode ReleaseDynamicCode(TUniqueID device_id) = 0;

public:
    virtual void SetCallBack(void *owner, TReceiveMessageCallBack callback, TSystemNotifyCallBack notify_callback, TErrorCallBack error_callback) = 0;

public:
    virtual void Destroy() = 0;

protected:
    virtual ~IIMClientAgent() {}
};

extern IIMClientAgent *gfCreateIMClientAgent(EProtocolType type = EProtocolType_SACP, TU32 version = 2);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

