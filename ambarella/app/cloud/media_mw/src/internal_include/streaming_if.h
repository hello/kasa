/*******************************************************************************
 * streaming_if.h
 *
 * History:
 *    2012/12/23 - [Zhi He] create file
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

#ifndef __STREAMING_IF_H__
#define __STREAMING_IF_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

#define RTSP_MAX_BUFFER_SIZE 4096
#define RTSP_MAX_DATE_BUFFER_SIZE 512

#ifdef DCONFIG_COMPILE_OLD_VERSION_FILE

//can be used or flags
typedef enum {
    ESessionMethod_INVALID = 0,
    ESessionMethod_RTSP_OPTIONS = (0x1 << 1),
    ESessionMethod_RTSP_DESCRIBE = (0x1 << 2),
    ESessionMethod_RTSP_ANNOUNCE = (0x1 << 3),

    ESessionMethod_RTSP_SETUP = (0x1 << 4),
    ESessionMethod_RTSP_PLAY = (0x1 << 5),
    ESessionMethod_RTSP_PAUSE = (0x1 << 6),
    ESessionMethod_RTSP_TEARDOWN = (0x1 << 7),
    ESessionMethod_RTSP_GET_PARAMETER = (0x1 << 8),
    ESessionMethod_RTSP_SET_PARAMETER = (0x1 << 9),

    ESessionMethod_RTSP_REDIRECT = (0x1 << 10),
    ESessionMethod_RTSP_RECORD = (0x1 << 11),

    //ONLY USED when RTSP is carrided over RTP
    ESessionMethod_RTSP_EmbeddedBinaryData = (0x1 << 12),
}  ESessionMethod;

typedef enum {
    ESessionStatusCode_Invalid = 0,

    //success 2xx
    ESessionStatusCode_LowOnStorageSpace = 250,

    //redirection 3xx

    //client error 4xx
    ESessionStatusCode_MethodNotAllowed = 405,
    ESessionStatusCode_ParametersNotUnderstood = 451,
    ESessionStatusCode_ConferenceNotFound = 452,
    ESessionStatusCode_NotEnoughBandwidth = 453,
    ESessionStatusCode_SessionNotFound = 454,
    ESessionStatusCode_MethodNotValidInThisState = 455,
    ESessionStatusCode_HeaderFieldNotValidForResource = 456,
    ESessionStatusCode_InvalidRange = 457,
    ESessionStatusCode_ParametersIsReadOnly = 458,
    ESessionStatusCode_AggregateOperationNotAllowed = 459,
    ESessionStatusCode_OnlyAggregateOperationAllowed = 460,
    ESessionStatusCode_UnsupportedTransport = 461,
    ESessionStatusCode_DestnationUnreachable = 462,
    ESessionStatusCode_OptionNotSupported = 551,
}  ESessionStatusCode;

//refer to rfc2326, 12 Header Field Definitions
typedef enum {
    ERTSPHeaderType_general = (0x1 << 0),
    ERTSPHeaderType_request = (0x1 << 1),
    ERTSPHeaderType_response = (0x1 << 2),
    ERTSPHeaderType_entity = (0x1 << 3),
} ERTSPHeaderType;

typedef enum {
    ERTSPHeaderSupport_required,
    ERTSPHeaderSupport_optional,
} ERTSPHeaderSupport;

typedef enum {
    ERTSPHeaderName_Accept = 0,
    ERTSPHeaderName_Accept_Encoding,
    ERTSPHeaderName_Accept_Language,
    ERTSPHeaderName_Allow,
    ERTSPHeaderName_Authorization,
    ERTSPHeaderName_Bandwidth,
    ERTSPHeaderName_Blocksize,
    ERTSPHeaderName_Cache_Control,
    ERTSPHeaderName_Conference,
    ERTSPHeaderName_Connection,
    ERTSPHeaderName_Content_Base,
    ERTSPHeaderName_Content_Encoding_1,
    ERTSPHeaderName_Content_Encoding_2,
    ERTSPHeaderName_Content_Language,
    ERTSPHeaderName_Content_Length_1,
    ERTSPHeaderName_Content_Length_2,
    ERTSPHeaderName_Content_Location,
    ERTSPHeaderName_Content_Type_1,
    ERTSPHeaderName_Content_Type_2,
    ERTSPHeaderName_CSeq,
    ERTSPHeaderName_Date,
    ERTSPHeaderName_Expires,
    ERTSPHeaderName_From,
    ERTSPHeaderName_If_Modified_Since,
    ERTSPHeaderName_Last_Modified,
    ERTSPHeaderName_Proxy_Authenticate,
    ERTSPHeaderName_Proxy_Require,
    ERTSPHeaderName_Public,
    ERTSPHeaderName_Range_1,
    ERTSPHeaderName_Range_2,
    ERTSPHeaderName_Referer,
    ERTSPHeaderName_Require,
    ERTSPHeaderName_Retry_After,
    ERTSPHeaderName_RTP_Info,
    ERTSPHeaderName_Scale,
    ERTSPHeaderName_Session,
    ERTSPHeaderName_Server,
    ERTSPHeaderName_Speed,
    ERTSPHeaderName_Transport,
    ERTSPHeaderName_Unsupported,
    ERTSPHeaderName_User_Agent,
    ERTSPHeaderName_Via,
    ERTSPHeaderName_WWW_Authenticate,

    //NOT modify blow!
    ERTSPHeaderName_COUNT,
} ERTSPHeaderName;

#endif

typedef struct {
    TU16 first_seq_num;
    TU16 cur_seq_num;

    TU32 octet_count;
    TU32 packet_count;
    TU32 last_octet_count;
    TTime last_rtcp_ntp_time;
    TTime first_rtcp_ntp_time;
    TU32 base_timestamp;
    TU32 timestamp;
} SRTCPServerStatistics;

//-----------------------------------------------------------------------
//
// SStreamingContent
//
//-----------------------------------------------------------------------
#define DMaxStreamContentUrlLength 64

class IStreamingTransmiter;
class IStreamingServer;
typedef struct {
    TComponentIndex transmiter_input_pin_index;
    TU8 type;//StreamType;
    TU8 format;//StreamFormat format;

    TU8 enabled;
    TU8 data_comes;
    TU8 parameters_setup;
    TU8 reserved1;

    //video info
    TU32 video_width, video_height;
    TU32 video_bit_rate;//, video_framerate;
    TU32 video_framerate_num, video_framerate_den;
    float video_framerate;

    //audio info
    TU32 audio_channel_number, audio_sample_rate;
    TU32 audio_bit_rate, audio_sample_format;
    TU32 audio_frame_size;

    //port_base
    TU16 server_rtp_port;
    TU16 server_rtcp_port;

    TChar profile_level_id_base16[8];
    TChar sps_base64[256];
    TChar pps_base64[32];

    TChar aac_config_base16[32];

    IStreamingTransmiter *p_transmiter;
} SStreamingSubSessionContent;

typedef struct {
    TGenericID id;

    TComponentIndex content_index;
    TU8 b_content_setup;
    TU8 sub_session_count;

    TU8 enabled;
    TU8 has_video;
    TU8 has_audio;
    TU8 enable_remote_control;

    //SStreamingSubSessionContent sub_session_content[ESubSession_tot_count];
    SStreamingSubSessionContent *sub_session_content[ESubSession_tot_count];

    TChar session_name[DMaxStreamContentUrlLength];

    IStreamingServer *p_server;

    CIDoubleLinkedList *p_client_list;

    IFilter *p_streaming_transmiter_filter;
    IFilter *p_flow_controller_filter;
    IFilter *p_video_source_filter;
    IFilter *p_audio_source_filter;

    //vod
    TU8 content_type;// 0: streaming; 1: vod file from storage; 2: vod file seted by '-f filname'
    TU8 reserved_0;
    TU8 reserved_1;
    TU8 reserved_2;
} SStreamingSessionContent;

//-----------------------------------------------------------------------
//
// IStreamingServerManager
//
//-----------------------------------------------------------------------

class IStreamingServerManager
{
public:
    virtual void PrintStatus0() = 0;
    virtual void Destroy() = 0;

public:
    virtual EECode RunServerManager() = 0;
    virtual EECode ExitServerManager() = 0;

    virtual EECode Start() = 0;
    virtual EECode Stop() = 0;
    virtual IStreamingServer *AddServer(StreamingServerType type, StreamingServerMode mode, TU16 server_port, TU8 enable_video, TU8 enable_audio) = 0;
    virtual EECode RemoveServer(IStreamingServer *server) = 0;

    virtual EECode AddStreamingContent(IStreamingServer *server_info, SStreamingSessionContent *content) = 0;

    virtual EECode RemoveSession(IStreamingServer *server_info, void *session) = 0;
};

struct rtcp_stat_t {
    TInt  first_packet;
    TU32 octet_count;
    TU32 packet_count;
    TU32 last_octet_count;
    TTime last_rtcp_ntp_time;
    TTime first_rtcp_ntp_time;
    TU32 base_timestamp;
    TU32 timestamp;
};

#define DMaxRtspAuthenRealmLength 64
#define DMaxRtspAuthenNonceLength 64
#define DMaxRtspAuthenPasswordLength 16
struct SAuthenticator {
    const TChar *realm;
    TChar nonce[DMaxRtspAuthenNonceLength];
    TChar username[DMAX_ACCOUNT_NAME_LENGTH_EX];
    TChar devicename[DMAX_ACCOUNT_NAME_LENGTH_EX];
    TChar password[DMaxRtspAuthenPasswordLength];
};

struct SClientSessionInfo;
struct SSubSessionInfo {
    TU16 seq_number;
    TU8 track_id;
    TU8 is_started;

    //server info
    TSocketPort server_rtp_port;
    TSocketPort server_rtcp_port;

    //client info
    TU32 addr;
    TSocketPort client_rtp_port;
    TSocketPort client_rtcp_port;

    struct sockaddr_in addr_port;//RTP
    struct sockaddr_in addr_port_ext;//RTCP

    TInt socket;
    TInt rtcp_socket;

    TU32 rtp_ssrc;
    TTime start_time;
    TTime cur_time;

    TTime ntp_time;
    TTime ntp_time_base;

    //av sync
    TTime avsync_begin_native_timestamp;
    TTime avsync_begin_linear_timestamp;
    TTime avsync_initial_drift;

    SClientSessionInfo *parent;
    SStreamingSubSessionContent *p_content;
    //for vod transmiter, copy from SStreamingSubSessionContent
    IStreamingTransmiter *p_transmiter;
    TComponentIndex transmiter_input_pin_index;

    TU16 rtcp_sr_cooldown;
    TU16 rtcp_sr_current_count;
    rtcp_stat_t rtcp_stat;//absolete

    //for rtp over rtsp
    TU8 rtp_over_rtsp;
    TU8 rtp_channel;
    TU8 rtcp_channel;
    TU8 send_extradata;

    TInt  rtsp_fd;

    TU8 wait_first_key_frame;
    TU8 key_frame_comes;
    TU8 need_resync;
    TU8 is_closed;

    //debug real time print
#ifdef DCONFIG_ENABLE_REALTIME_PRINT
    TU64 debug_sub_session_packet_count;
    TTime debug_sub_session_duration_num;
    TTime debug_sub_session_duration_den;
    TTime debug_sub_session_begin_time;
#endif

    SRTCPServerStatistics statistics;

    STransferDataChannel *p_transfer_channel;
};

struct SClientSessionInfo {
    struct sockaddr_in client_addr;//with RTSP port
    TSocketHandler tcp_socket;

    TSocketPort server_port;
    TU8 master_index;
    TU8 get_host_string;
    TChar host_addr[32];

    IStreamingServer *p_server;

    //info
    TChar *p_sdp_info;
    TU32 sdp_info_len;
    TChar *p_session_name;
    TU32 session_name_len;

    TUint session_id;

    SSubSessionInfo sub_session[ESubSession_tot_count];

    TU8 enable_video;
    TU8 enable_audio;
    TU8 setup_context;
    TU8 add_to_transmiter;

    TChar mRecvBuffer[RTSP_MAX_BUFFER_SIZE];
    TInt mRecvLen;

    TTime last_cmd_time;

    TU8 rtp_over_rtsp;
    TU8 vod_mode;
    TU8 vod_has_end;
    TU8 speed_combo;

    IFilter *p_streaming_transmiter_filter;
    SStreamingSessionContent *p_content;

    ESessionServerState state;
    ProtocolType protocol_type;

    void *p_pipeline;
    SDateTime starttime;
    SDateTime seektime;
    SDateTime endtime;

    float current_position;
    float vod_content_length;

    TTime current_time;

    SAuthenticator authenticator;
} ;

extern TU32 gfRandom32(void);

extern IStreamingServerManager *gfCreateStreamingServerManager(const volatile SPersistMediaConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);

TUint getTrackID(char *string, char *string_end);
void apend_string(char *&ori, TUint size, char *append_str);
TChar *generate_sdp_description_h264(TChar const *pStreamName, TChar const *pInfo, TChar const *pDescription, TChar const *ipAddressStr, TChar *rtspURL, TUint tv_sec, TUint tv_usec, TUint payload_type, TUint width, TUint height, float framerate, TU64 length);
TChar *generate_sdp_description_h264_aac(TChar const *pStreamName, TChar const *pInfo, TChar const *pDescription, TChar const *ipAddressStr, TChar *rtspURL, TUint tv_sec, TUint tv_usec, TUint video_payload_type, TUint width, TUint height, float framerate, TUint audio_payload_type, TUint samplerate, TUint channel_number, TChar *p_aac_config, TU64 length);
TChar *generate_sdp_description_h264_pcm_g711_mu(TChar const *pStreamName, TChar const *pInfo, TChar const *pDescription, TChar const *ipAddressStr, TChar *rtspURL, TUint tv_sec, TUint tv_usec, TUint payload_type, TUint width, TUint height, float framerate, TU64 length);
TChar *generate_sdp_description_h264_pcm_g711_a(TChar const *pStreamName, TChar const *pInfo, TChar const *pDescription, TChar const *ipAddressStr, TChar *rtspURL, TUint tv_sec, TUint tv_usec, TUint payload_type, TUint width, TUint height, float framerate, TU64 length);
TChar *generate_sdp_description_aac(TChar const *pStreamName, TChar const *pInfo, TChar const *pDescription, TChar const *ipAddressStr, TChar *rtspURL, TUint tv_sec, TUint tv_usec, TUint audio_payload_type, TUint samplerate, TUint channel_number, TChar *p_aac_config, TU64 length);
TChar *generate_sdp_description_pcm_g711_mu(TChar const *pStreamName, TChar const *pInfo, TChar const *pDescription, TChar const *ipAddressStr, TChar *rtspURL, TUint tv_sec, TUint tv_usec, TU64 length);
TChar *generate_sdp_description_pcm_g711_a(TChar const *pStreamName, TChar const *pInfo, TChar const *pDescription, TChar const *ipAddressStr, TChar *rtspURL, TUint tv_sec, TUint tv_usec, TU64 length);
TChar *generate_sdp_description_h264_sps_params(TChar const *pStreamName, TChar const *pInfo, TChar const *pDescription, TChar const *ipAddressStr, TChar *rtspURL, TUint tv_sec, TUint tv_usec, TUint payload_type, TUint width, TUint height, float framerate, TChar *profile_level_id, TChar *sps_base64, TChar *pps_base64, TU64 length);
TChar *generate_sdp_description_h264_aac_sps_params(TChar const *pStreamName, TChar const *pInfo, TChar const *pDescription, TChar const *ipAddressStr, TChar *rtspURL, TUint tv_sec, TUint tv_usec, TUint video_payload_type, TUint width, TUint height, float framerate, TUint audio_payload_type, TUint samplerate, TUint channel_number, TChar *profile_level_id, TChar *sps_base64, TChar *pps_base64, TChar *p_aac_config, TU64 length);
TChar *generate_sdp_description_h264_pcm_g711_mu_sps_params(TChar const *pStreamName, TChar const *pInfo, TChar const *pDescription, TChar const *ipAddressStr, TChar *rtspURL, TUint tv_sec, TUint tv_usec, TUint payload_type, TUint width, TUint height, float framerate, TChar *profile_level_id, TChar *sps_base64, TChar *pps_base64, TU64 length);
TChar *generate_sdp_description_h264_pcm_g711_a_sps_params(TChar const *pStreamName, TChar const *pInfo, TChar const *pDescription, TChar const *ipAddressStr, TChar *rtspURL, TUint tv_sec, TUint tv_usec, TUint payload_type, TUint width, TUint height, float framerate, TChar *profile_level_id, TChar *sps_base64, TChar *pps_base64, TU64 length);

bool parseTransportHeader(TChar const *buf, ProtocolType &streamingMode, TChar *streamingModeString, TChar *destinationAddressStr,
                          TChar *destinationTTL, TU16 *clientRTPPortNum, TU16 *clientRTCPPortNum, TChar *rtpChannelId, TChar *rtcpChannelId);
bool parseSessionHeader(char const *buf, unsigned short &sessionId);

EECode gfSDPProcessVideoExtraData(SStreamingSubSessionContent *subsession, TU8 *p_extra_data, TU32 size);
EECode gfSDPProcessAudioExtraData(SStreamingSubSessionContent *subsession, TU8 *p_extra_data, TU32 size);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

