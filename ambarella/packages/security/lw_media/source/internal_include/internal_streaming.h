/**
 * internal_streaming.h
 *
 * History:
 *  2015/07/28 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __INTERNAL_STREAMING_H__
#define __INTERNAL_STREAMING_H__

#define DMAX_ACCOUNT_NAME_LENGTH 24
#define DMAX_ACCOUNT_NAME_LENGTH_EX 32
#define DMAX_ACCOUNT_INFO_LENGTH 48

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
    IStreamingTransmiter *p_transmiter;
    TComponentIndex transmiter_input_pin_index;

    TU16 rtcp_sr_cooldown;
    TU16 rtcp_sr_current_count;

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



#endif

