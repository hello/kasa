/*
 * rtsp_demuxer.cpp
 *
 * History:
 *    2012/08/03 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "lwmd_if.h"
#include "lwmd_log.h"

#include "internal.h"

#include "network_al.h"

#include "internal_streaming.h"

#include "osal.h"
#include "framework.h"
#include "modules_interface.h"
#include "codec_interface.h"

#ifdef BUILD_MODULE_DEMUXER_RTSP

#include "rtp_h264_scheduled_receiver.h"
#include "rtp_aac_scheduled_receiver.h"
#include "rtp_pcm_scheduled_receiver.h"
#include "rtp_mpeg12audio_scheduled_receiver.h"

#include "rtp_scheduled_receiver_tcp.h"

#include "rtsp_demuxer.h"

//#define DLOCAL_DEBUG_VERBOSE

static EECode __release_ring_buffer(CIBuffer *pBuffer)
{
    DASSERT(pBuffer);
    if (pBuffer) {
        if (EBufferCustomizedContentType_RingBuffer == pBuffer->mCustomizedContentType) {
            IMemPool *pool = (IMemPool *)pBuffer->mpCustomizedContent;
            DASSERT(pool);
            if (pool) {
                pool->ReleaseMemBlock((TULong)(pBuffer->GetDataMemorySize()), pBuffer->GetDataPtrBase());
            }
        }
    } else {
        LOG_FATAL("NULL pBuffer!\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

IDemuxer *gfCreatePrivateRTSPDemuxerScheduledModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index)
{
    return CScheduledRTSPDemuxer::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

//strings for RTSP
static const TChar *_rtsp_option_request_fmt =
    "OPTIONS rtsp://%s:%d/%s RTSP/1.0\r\n"
    "CSeq: %d\r\n"
    DSRTING_RTSP_CLIENT_TAG
    "\r\n"
    ;

static const TChar *_rtsp_describe_request_fmt =
    "DESCRIBE rtsp://%s:%d/%s RTSP/1.0\r\n"
    "CSeq: %d\r\n"
    DSRTING_RTSP_CLIENT_TAG
    "Accept: application/sdp\r\n\r\n"
    ;

static const TChar *_rtsp_setup_request_fmt =
    "SETUP rtsp://%s:%d/%s/%s RTSP/1.0\r\n"
    "CSeq: %d\r\n"
    DSRTING_RTSP_CLIENT_TAG
    "Transport: RTP/AVP;unicast;client_port=%d-%d\r\n\r\n"
    ;

static const TChar *_rtsp_setup_request_with_sessionid_fmt =
    "SETUP rtsp://%s:%d/%s/%s RTSP/1.0\r\n"
    "CSeq: %d\r\n"
    DSRTING_RTSP_CLIENT_TAG
    "Transport: RTP/AVP;unicast;client_port=%d-%d\r\n"
    "Session: %llx\r\n\r\n"
    ;

static const TChar *_rtsp_setup_request_tcp_fmt =
    "SETUP rtsp://%s:%d/%s/%s RTSP/1.0\r\n"
    "CSeq: %d\r\n"
    DSRTING_RTSP_CLIENT_TAG
    "Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n\r\n"
    ;

static const TChar *_rtsp_setup_request_tcp_with_sessionid_fmt =
    "SETUP rtsp://%s:%d/%s/%s RTSP/1.0\r\n"
    "CSeq: %d\r\n"
    DSRTING_RTSP_CLIENT_TAG
    "Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n"
    "Session: %llx\r\n\r\n"
    ;

static const TChar *_rtsp_play_request_fmt =
    "PLAY rtsp://%s:%d/%s RTSP/1.0\r\n"
    "CSeq: %d\r\n"
    DSRTING_RTSP_CLIENT_TAG
    "Session: %llx\r\n"
    "Range: npt=0.000-\r\n\r\n"
    ;

static const TChar *_rtsp_teardown_request_fmt =
    "TEARDOWN rtsp://%s:%d/%s RTSP/1.0\r\n"
    "CSeq: %d\r\n"
    DSRTING_RTSP_CLIENT_TAG
    "Session: %llx\r\n\r\n"
    ;

typedef struct {
    TUint offset_x;
    TUint offset_y;
    TUint width;
    TUint height;
    TUint bitrate;

    //VideoFrameRate framerate;
    float framerate;

    StreamFormat format;

    TU8 rtp_track_index;
    TU8 rtp_payload_type;
    TU8 reserved1;
    TU8 reserved2;

    TChar track_string[64];

    TU8 *p_extra_data_sps_base64;
    TMemSize extra_data_sps_base64_size;
    TU8 *p_extra_data_pps_base64;
    TMemSize extra_data_pps_base64_size;
} SVideoRTPParams;

typedef struct {
    TUint channel_number;
    TUint sample_rate;
    TUint bitrate;

    StreamFormat format;

    TU8 is_sample_interleave;
    TU8 sample_format;
    TU8 rtp_track_index;
    TU8 rtp_payload_type;

    TChar track_string[16];

    TU8 *p_extra_data_base16;
    TMemSize extra_data_base16_size;
} SAudioRTPParams;

static EECode _try_open_rtprtcp_port(TU16 try_rtpport_start, TU16 try_rtpport_end, TU16 &current_rtp_port, TSocketHandler &rtpsocket, TSocketHandler &rtcpsocket, TU32 request_receive_buffer_size, TU32 request_send_buffer_size)
{
    if (try_rtpport_start & 0x1) {
        try_rtpport_start ++;
    }

    LOG_INFO("try find rtp/rtcp port pair in %hu-%hu.\n", try_rtpport_start, try_rtpport_end);

    while (try_rtpport_start < try_rtpport_end) {
        rtpsocket = gfNet_SetupDatagramSocket(INADDR_ANY, try_rtpport_start, 0, request_receive_buffer_size, request_send_buffer_size);
        if (rtpsocket < 0) {
            try_rtpport_start += 2;
            continue;
        }
        rtcpsocket = gfNet_SetupDatagramSocket(INADDR_ANY, try_rtpport_start + 1, 0, request_receive_buffer_size, request_send_buffer_size);
        if (rtcpsocket < 0) {
            gfNetCloseSocket(rtpsocket);
            try_rtpport_start += 2;
            continue;
        }
        current_rtp_port = try_rtpport_start;
        LOG_INFO("find rtp/rtcp port pair, %hu/%hu.\n", try_rtpport_start, try_rtpport_start + 1);
        return EECode_OK;
    }

    LOG_INFO("cannot find rtp/rtcp port till %hu.\n", try_rtpport_end);
    return EECode_Error;
}

static TChar *_find_string(TChar *buffer, TInt src_len, const TChar *target, TInt target_len)
{
    char *ptmp = buffer;
    DASSERT(buffer);
    DASSERT(target);
    DASSERT(target_len);

    while ((0x0 != ptmp) && (0 < src_len)) {
        if (!strncmp(ptmp, target, target_len)) {
            return ptmp;
        }
        src_len --;
        ptmp ++;
    }
    return NULL;
}

static StreamFormat __guess_stream_format_from_sdp(TChar *p_string, StreamType &type, TChar *&next_p)
{
    if (p_string) {
        if (!strncmp("H264", p_string, strlen("H264"))) {
            type = StreamType_Video;
            next_p = p_string + strlen("H264");
            return StreamFormat_H264;
        } else if (!strncmp("PCMU", p_string, strlen("PCMU"))) {
            type = StreamType_Audio;
            next_p = p_string + strlen("PCMU");
            return StreamFormat_PCMU;
        } else if (!strncmp("PCMA", p_string, strlen("PCMA"))) {
            type = StreamType_Audio;
            next_p = p_string + strlen("PCMA");
            return StreamFormat_PCMA;
        } else if (!strncmp("MPA", p_string, strlen("MPA"))) {
            type = StreamType_Audio;
            next_p = p_string + strlen("MPA");
            return StreamFormat_MPEG12Audio;
        } else if (!strncmp("mpeg4-generic", p_string, strlen("mpeg4-generic"))) {
            type = StreamType_Audio;
            next_p = p_string + strlen("mpeg4-generic");
            return StreamFormat_AAC;
        } else {
            TChar ppp[32] = {0};
            strncpy(ppp, p_string, 30);
            LOG_ERROR("unknown format string:%s\n", ppp);
        }
    }

    type = StreamType_Invalid;
    next_p = NULL;
    return StreamFormat_Invalid;
}

#if 0
static TUint __guess_framerate(float value)
{
    if ((value < (15 + 0.02)) && (value > (15 - 0.02))) {
        return VideoFrameRate_15;
    } else if ((value < (24 + 0.02)) && (value > (24 - 0.02))) {
        return VideoFrameRate_24;
    } else if ((value < (23.96 + 0.02)) && (value > (23.96 - 0.02))) {
        return VideoFrameRate_23dot96;
    } else if ((value < (29.97 + 0.02)) && (value > (29.97 - 0.02))) {
        return VideoFrameRate_29dot97;
    } else if ((value < (30 + 0.02)) && (value > (30 - 0.02))) {
        return VideoFrameRate_30;
    } else if ((value < (59.94 + 0.02)) && (value > (59.94 - 0.02))) {
        return VideoFrameRate_59dot94;
    } else if ((value < (60 + 0.02)) && (value > (60 - 0.02))) {
        return VideoFrameRate_60;
    }

    return (TUint)value;
}
#endif

static TInt _has_username_password_in_rtsp_url(TChar *ptmp, TChar *&p_server_addr)
{
    TChar *ptmp1 = NULL, *ptmp2 = NULL;
    if (DLikely(ptmp)) {
        ptmp1 = strchr(ptmp, '/');
        if (DLikely(ptmp1)) {
            ptmp2 = strchr(ptmp, '@');
            if (ptmp2 && (((TULong)ptmp1) > ((TULong)(ptmp2 + 1)))) {
                p_server_addr = ptmp2 + 1;
                return 1;
            }
        }
    }

    return 0;
}

static EECode _parse_rtsp_url(const TChar *total_url, TChar *server_addr, TChar *item_name, TU16 &port_number)
{
    TChar *ptmp, *ptmp1;
    TUint len;
    TChar port_string[8];
    TChar *p_server_addr_1 = NULL;

    if (!total_url || !server_addr || !item_name) {
        LOG_ERROR("NULL params.\n");
        return EECode_BadParam;
    }

    ptmp = (TChar *)total_url + 7; //skip 'rtsp://'

    if (1 == _has_username_password_in_rtsp_url(ptmp, p_server_addr_1)) {
        DASSERT(p_server_addr_1);
        LOG_NOTICE("url has username and password, %s\n", p_server_addr_1);
        ptmp = p_server_addr_1;
    }

    ptmp1 = strchr(ptmp, ':');
    if (ptmp1) {
        //has port number's case
        len = ptmp1 - ptmp;

        memcpy(server_addr, ptmp, len);
        server_addr[len] = 0x0;

        ptmp = ptmp1 + 1;//skip ':'

        ptmp1 = strchr(ptmp, '/');
        DASSERT(ptmp1);
        if (!ptmp1) {
            LOG_ERROR("rtsp string has error, %s.\n", ptmp);
            return EECode_Error;
        }
        len = ptmp1 - ptmp;
        DASSERT(len < 8);
        if (len < 8) {
            memcpy(port_string, ptmp, len);
            port_string[len] = 0x0;
            port_number = atoi(port_string);
        } else {
            LOG_ERROR("why port number string greater than 8, %s?\n", ptmp);
            return EECode_Error;
        }

        ptmp = ptmp1 + 1;//skip '/'
        ptmp1 = strchr(ptmp, '/');
        if (1 || (!ptmp1)) {
            strcpy(item_name, ptmp);
        } else {
            //remove '/'
            len = ptmp1 - ptmp;
            memcpy(item_name, ptmp, len);
            item_name[len] = 0x0;
        }
    } else {
        //no port number's case
        port_number = DefaultRTSPServerPort;

        ptmp1 = strchr(ptmp, '/');
        if (ptmp1) {
            len = ptmp1 - ptmp;
            memcpy(server_addr, ptmp, len);
            server_addr[len] = 0x0;
        } else {
            LOG_ERROR("no item's case, guess it to 'stream_0'?\n");
            strcpy(server_addr, ptmp);
            strcpy(item_name, "stream_0");
            return EECode_OK;
        }

        ptmp = ptmp1 + 1;//skip '/'
        ptmp1 = strchr(ptmp, '/');
        if (1 || (!ptmp1)) {
            strcpy(item_name, ptmp);
        } else {
            //remove '/'
            len = ptmp1 - ptmp;
            memcpy(item_name, ptmp, len);
            item_name[len] = 0x0;
        }
    }

    return EECode_OK;
}

static void __parseSDP(TChar *p_sdp, TInt len, TU8 &has_video, TU8 &has_audio, TU8 &has_video_extradata, TU8 &has_audio_extradata, SVideoRTPParams *video_params, SAudioRTPParams *audio_params)
{
    TChar *ptmp = NULL;
    TChar *ptmp1 = NULL;
    TChar *ptmp2 = NULL;
    TInt length = 0;
    //float valuef = 0;
    TUint value = 0;

    TUint play_load_type = 0;

    StreamFormat format = StreamFormat_Invalid;
    StreamType type = StreamType_Invalid;

    if (DLikely(p_sdp && len)) {

        ptmp = _find_string(p_sdp, len, "sprop-parameter-sets=", strlen("sprop-parameter-sets="));
        if (ptmp) {
            ptmp += strlen("sprop-parameter-sets=");
            ptmp1 = strchr(ptmp, ',');
            if (DLikely(ptmp1)) {
                has_video_extradata = 1;
                video_params->p_extra_data_sps_base64 = (TU8 *)ptmp;
                video_params->extra_data_sps_base64_size = (TMemSize)(ptmp1 - ptmp);
                LOG_INFO("found 'sprop-parameter-sets=' (video extra data, sps), %p, %ld\n", video_params->p_extra_data_sps_base64, video_params->extra_data_sps_base64_size);
                if (DLikely(ptmp1)) {
                    ptmp1 ++;
                    //ptmp2 = strchr(ptmp1, '\n');
                    ptmp2 = ptmp1;
                    TUint max_search_count = 16;
                    //LOG_NOTICE("%02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x\n", ptmp1[0], ptmp1[1], ptmp1[2], ptmp1[3], ptmp1[4], ptmp1[5], ptmp1[6], ptmp1[7], ptmp1[8], ptmp1[9], ptmp1[10], ptmp1[11]);
                    while (max_search_count) {
                        //LOG_NOTICE("ptmp2[0] %02x, %p\n", ptmp2[0], ptmp2);
                        if ((';' != ptmp2[0]) && (0x0d != ptmp2[0]) && (0x0a != ptmp2[0])) {
                            ptmp2 ++;
                        } else {
                            max_search_count = 16;
                            break;
                        }
                        max_search_count --;
                    }
                    if (DLikely(16 == max_search_count)) {

                    } else {
                        LOG_ERROR("do not find pps end\n");
                    }
                    video_params->p_extra_data_pps_base64 = (TU8 *)ptmp1;
                    video_params->extra_data_pps_base64_size = (TMemSize)(ptmp2 - ptmp1);
                    LOG_INFO("found 'sprop-parameter-sets=' (video extra data, pps), %p, %ld\n", video_params->p_extra_data_pps_base64, video_params->extra_data_pps_base64_size);
                }
            }
        }

        ptmp = _find_string(p_sdp, len, "a=cliprect:", strlen("a=cliprect:"));
        if (ptmp) {
            TUint num_cnt = 0, offset_x = 0, offset_y = 0;
            num_cnt = sscanf(ptmp, "a=cliprect:%d,%d,%d,%d", &offset_x, &offset_y, &video_params->height, &video_params->width);
            DASSERT(4 == num_cnt);
            LOG_NOTICE("get video width %d, height %d\n", video_params->width, video_params->height);
        }

        ptmp = _find_string(p_sdp, len, "a=framerate:", strlen("a=framerate:"));
        if (ptmp) {
            TUint num_cnt = 0;
            float framerate = 29.97;
            num_cnt = sscanf(ptmp, "a=framerate:%f", &framerate);
            DASSERT(1 == num_cnt);
            video_params->framerate = framerate;
            LOG_NOTICE("get video framerate %f\n", framerate);
        }

        ptmp = _find_string(p_sdp, len, "a=rtpmap:", strlen("a=rtpmap:"));
        if (DLikely(ptmp)) {

            ptmp1 = ptmp + strlen("a=rtpmap:");
            play_load_type = atoi(ptmp1);
            ptmp1 = strchr(ptmp1, ' ');
            if (ptmp1) {

                while (' ' == *ptmp1) {
                    ptmp1++;
                }

                format = __guess_stream_format_from_sdp(ptmp1, type, ptmp);
                if (StreamType_Video == type) {
                    DASSERT(StreamFormat_H264 == format);
                    DASSERT(ptmp);

                    has_video = 0;
                    video_params->format = format;
                    video_params->rtp_payload_type = play_load_type;

                    length = len - (ptmp - p_sdp);
                    DASSERT(length > 8);

#if 0
                    ptmp1 = _find_string(ptmp, length, "a=control:track", strlen("a=control:track"));
                    if (ptmp1) {
                        //LOG_NOTICE("video ptmp1 %s\n", ptmp1);
                        sscanf(ptmp1, "a=control:track%d", &value);
                        //LOG_NOTICE("get track id %d\n", value);
                        video_params->rtp_track_index = value;
                    }
#else
                    ptmp1 = _find_string(ptmp, length, "a=control:", strlen("a=control:"));
                    if (DLikely(ptmp1)) {
                        ptmp2 = _find_string(ptmp1, length, "track", strlen("track"));
                        if (DLikely(ptmp2)) {
                            if (!strncmp(ptmp2, "trackID=", strlen("trackID="))) {
                                sscanf(ptmp2, "trackID=%d", &value);
                                video_params->rtp_track_index = value;
                                snprintf(&video_params->track_string[0], 15, "%s%d", "trackID=", value);
                            } else {
                                sscanf(ptmp2, "track%d", &value);
                                video_params->rtp_track_index = value;
                                snprintf(&video_params->track_string[0], 15, "%s%d", "track", value);
                            }
                        }
                    }
#endif

#if 0
                    ptmp1 = _find_string(ptmp, length, "a=framerate:", strlen("a=framerate:"));
                    if (ptmp1) {
                        sscanf(ptmp1, "a=framerate:%f", &valuef);
                        video_params->framerate = valuef;
                    }
#endif

                    ptmp1 = _find_string(ptmp, length, "a=cliprect:", strlen("a=cliprect:"));
                    if (ptmp1) {
                        sscanf(ptmp1, "a=cliprect:%d,%d,%d,%d", &video_params->offset_x, &video_params->offset_y, &video_params->height, &video_params->width);
                    } else {
                        if ((0 == video_params->rtp_track_index) || (1 == video_params->rtp_track_index)) {
                            video_params->width = 0;
                            video_params->height = 0;
                        } else {
                            video_params->width = 0;
                            video_params->height = 0;
                        }
                    }
                    has_video = 1;
                    //LOG_NOTICE("\n");

                    ptmp1 = ptmp + strlen("a=rtpmap");
                    length = len - (ptmp1 - p_sdp);
                    ptmp = _find_string(ptmp1, length, "a=rtpmap:", strlen("a=rtpmap:"));
                    if (DLikely(ptmp)) {
                        ptmp1 = ptmp + strlen("a=rtpmap:");
                        play_load_type = atoi(ptmp1);
                        ptmp1 = strchr(ptmp, ' ');
                        while (' ' == *ptmp1) {
                            ptmp1++;
                        }

                        format = __guess_stream_format_from_sdp(ptmp1, type, ptmp);

                        if (StreamType_Audio == type) {
                            DASSERT(ptmp);

                            has_audio = 0;
                            audio_params->format = format;
                            audio_params->rtp_payload_type = play_load_type;

                            length = len - (ptmp - p_sdp);
                            DASSERT(length > 8);

                            sscanf(ptmp, "/%d/%d", &audio_params->sample_rate, &audio_params->channel_number);
#if 0
                            //LOG_FATAL("audio ptmp %s, audio_params->sample_rate %d, audio_params->channel_number %d\n", ptmp, audio_params->sample_rate, audio_params->channel_number);
                            ptmp1 = _find_string(ptmp, length, "a=control:track", strlen("a=control:track"));
                            if (ptmp1) {
                                sscanf(ptmp1, "a=control:track%d", &value);
                                //LOG_NOTICE("get track id %d\n", value);
                                audio_params->rtp_track_index = value;
                            }
#else

                            if (StreamFormat_AAC == format) {
                                ptmp1 = _find_string(ptmp, length, "config=", strlen("config="));
                                if (DLikely(ptmp1)) {
                                    ptmp2 = ptmp1 + strlen("config=");
                                    TInt remain_length = length - strlen("config=");
                                    do {
                                        if ((';' == ptmp2[0]) || (0x0d == ptmp2[0]) || (0x0a == ptmp2[0])) {
                                            remain_length = length - strlen("config=") - remain_length;
                                            if (DLikely(0 < remain_length)) {
                                                audio_params->p_extra_data_base16 = (TU8 *)ptmp1 + strlen("config=");
                                                audio_params->extra_data_base16_size = (TMemSize)remain_length;
                                                has_audio_extradata = 1;
                                            } else {
                                                LOG_FATAL("internal error!\n");
                                            }
                                            break;
                                        }
                                        ptmp2 ++;
                                        remain_length --;
                                    } while (remain_length > 0);
                                } else {
                                    LOG_NOTICE("do not found 'config=' in aac's sdp\n");
                                }
                            }

                            ptmp1 = _find_string(ptmp, length, "a=control:", strlen("a=control:"));
                            if (DLikely(ptmp1)) {
                                ptmp2 = _find_string(ptmp1, length, "track", strlen("track"));
                                if (DLikely(ptmp2)) {
                                    if (!strncmp(ptmp2, "trackID=", strlen("trackID="))) {
                                        sscanf(ptmp2, "trackID=%d", &value);
                                        audio_params->rtp_track_index = value;
                                        snprintf(&audio_params->track_string[0], 15, "%s%d", "trackID=", value);
                                    } else {
                                        sscanf(ptmp2, "track%d", &value);
                                        audio_params->rtp_track_index = value;
                                        snprintf(&audio_params->track_string[0], 15, "%s%d", "track", value);
                                    }
                                }
                            }
#endif
                            has_audio = 1;

                        } else {
                            LOG_WARN("not audio?\n");
                        }
                    } else {
                        LOG_INFO("do not find audio's 'a=rtpmap:'\n");
                    }

                } else if (StreamType_Audio == type) {

                    DASSERT(ptmp);

                    has_audio = 0;
                    audio_params->format = format;
                    audio_params->rtp_payload_type = play_load_type;

                    length = len - (ptmp - p_sdp);
                    DASSERT(length > 8);

                    sscanf(ptmp, "/%d/%d", &audio_params->sample_rate, &audio_params->channel_number);
#if 0
                    //LOG_FATAL("audio ptmp %s, audio_params->sample_rate %d, audio_params->channel_number %d\n", ptmp, audio_params->sample_rate, audio_params->channel_number);
                    ptmp1 = _find_string(ptmp, length, "a=control:track", strlen("a=control:track"));
                    if (ptmp1) {
                        sscanf(ptmp1, "a=control:track%d", &value);
                        //LOG_NOTICE("get track id %d\n", value);
                        audio_params->rtp_track_index = value;
                    }
#else
                    if (StreamFormat_AAC == format) {
                        ptmp1 = _find_string(ptmp, length, "config=", strlen("config="));
                        if (DLikely(ptmp1)) {
                            ptmp2 = ptmp1 + strlen("config=");
                            TInt remain_length = length - strlen("config=");
                            do {
                                if ((';' == ptmp2[0]) || (0x0d == ptmp2[0]) || (0x0a == ptmp2[0])) {
                                    remain_length = length - strlen("config=") - remain_length;
                                    if (DLikely(0 < remain_length)) {
                                        audio_params->p_extra_data_base16 = (TU8 *)ptmp1 + strlen("config=");
                                        audio_params->extra_data_base16_size = (TMemSize)remain_length;
                                        has_audio_extradata = 1;
                                    } else {
                                        LOG_FATAL("internal error!\n");
                                    }
                                    break;
                                }
                                ptmp2 ++;
                                remain_length --;
                            } while (remain_length > 0);
                        } else {
                            LOG_NOTICE("do not found 'config=' in aac's sdp\n");
                        }
                    }

                    ptmp1 = _find_string(ptmp, length, "a=control:", strlen("a=control:"));
                    if (DLikely(ptmp1)) {
                        ptmp2 = _find_string(ptmp1, length, "track", strlen("track"));
                        if (DLikely(ptmp2)) {
                            if (!strncmp(ptmp2, "trackID=", strlen("trackID="))) {
                                sscanf(ptmp2, "trackID=%d", &value);
                                audio_params->rtp_track_index = value;
                                snprintf(&audio_params->track_string[0], 15, "%s%d", "trackID=", value);
                            } else {
                                sscanf(ptmp2, "track%d", &value);
                                audio_params->rtp_track_index = value;
                                snprintf(&audio_params->track_string[0], 15, "%s%d", "track", value);
                            }
                        }
                    }
#endif
                    has_audio = 1;

                    ptmp = _find_string(ptmp, length, "a=rtpmap:", strlen("a=rtpmap:"));
                    if (DLikely(ptmp)) {
                        ptmp1 = ptmp + strlen("a=rtpmap:");
                        play_load_type = atoi(ptmp1);
                        ptmp1 = strchr(ptmp, ' ');
                        while (' ' == *ptmp1) {
                            ptmp1++;
                        }

                        format = __guess_stream_format_from_sdp(ptmp1, type, ptmp);

                        if (DLikely(StreamType_Video == type)) {

                            DASSERT(ptmp);
                            has_video = 0;
                            video_params->format = format;
                            video_params->rtp_payload_type = play_load_type;

                            length = len - (ptmp - p_sdp);
                            DASSERT(length > 8);

#if 0
                            ptmp1 = _find_string(ptmp, length, "a=control:track", strlen("a=control:track"));
                            if (ptmp1) {
                                //LOG_NOTICE("video ptmp1 %s\n", ptmp1);
                                sscanf(ptmp1, "a=control:track%d", &value);
                                //LOG_NOTICE("get track id %d\n", value);
                                video_params->rtp_track_index = value;
                            }
#else
                            ptmp1 = _find_string(ptmp, length, "a=control:", strlen("a=control:"));
                            if (DLikely(ptmp1)) {
                                ptmp2 = _find_string(ptmp1, length, "track", strlen("track"));
                                if (DLikely(ptmp2)) {
                                    if (!strncmp(ptmp2, "trackID=", strlen("trackID="))) {
                                        sscanf(ptmp2, "trackID=%d", &value);
                                        video_params->rtp_track_index = value;
                                        snprintf(&video_params->track_string[0], 15, "%s%d", "trackID=", value);
                                    } else {
                                        sscanf(ptmp2, "track%d", &value);
                                        video_params->rtp_track_index = value;
                                        snprintf(&video_params->track_string[0], 15, "%s%d", "track", value);
                                    }
                                }
                            }
#endif

#if 0
                            ptmp1 = _find_string(ptmp, length, "a=framerate:", strlen("a=framerate:"));
                            if (ptmp1) {
                                sscanf(ptmp1, "a=framerate:%f", &valuef);
                                video_params->framerate = __guess_framerate(valuef);
                            }
#endif

                            ptmp1 = _find_string(ptmp, length, "a=cliprect:", strlen("a=cliprect:"));
                            if (ptmp1) {
                                sscanf(ptmp1, "a=cliprect:%d,%d,%d,%d", &video_params->offset_x, &video_params->offset_y, &video_params->height, &video_params->width);
                            } else {
                                if ((0 == video_params->rtp_track_index) || (1 == video_params->rtp_track_index)) {
                                    video_params->width = 0;
                                    video_params->height = 0;
                                } else {
                                    video_params->width = 0;
                                    video_params->height = 0;
                                }
                            }
                            has_video = 1;

                        } else {
                            LOG_WARN("not video?\n");
                        }
                    } else {
                        LOG_INFO("do not find video's 'a=rtpmap:'\n");
                    }
                } else {
                    LOG_WARN("BAD type\n");
                }
            }
        } else {
            LOG_WARN("do not found sdp content\n");
        }
    } else {
        LOG_FATAL("NULL pointer p_dsp or zero len\n");
    }
}

EECode gfCreateRTPScheduledReceiver(StreamFormat format, TUint index, IScheduledRTPReceiver *&receceiver, IScheduledClient *&client, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    switch (format) {
        case StreamFormat_H264: {
                CRTPH264ScheduledReceiver *p = new CRTPH264ScheduledReceiver(index, pPersistMediaConfig, pMsgSink);
                if (p) {
                    receceiver = (IScheduledRTPReceiver *)p;
                    client = (IScheduledClient *)p;
                } else {
                    LOG_FATAL("new CRTPH264ScheduledReceiver(%d) fail.\n", index);
                    return EECode_NoMemory;
                }
            }
            break;

        case StreamFormat_PCMA:
        case StreamFormat_PCMU: {
                CRTPPCMScheduledReceiver *p = new CRTPPCMScheduledReceiver(index, pPersistMediaConfig, pMsgSink);
                if (p) {
                    receceiver = (IScheduledRTPReceiver *)p;
                    client = (IScheduledClient *)p;
                } else {
                    LOG_FATAL("new CRTPPCMScheduledReceiver(%d) fail.\n", index);
                    return EECode_NoMemory;
                }
            }
            break;

        case StreamFormat_AAC: {
                CRTPAACScheduledReceiver *p = new CRTPAACScheduledReceiver(index, pPersistMediaConfig, pMsgSink);
                if (p) {
                    receceiver = (IScheduledRTPReceiver *)p;
                    client = (IScheduledClient *)p;
                } else {
                    LOG_FATAL("new CRTPAACScheduledReceiver(%d) fail.\n", index);
                    return EECode_NoMemory;
                }
            }
            break;

        case StreamFormat_MPEG12Audio: {
                CRTPMpeg12AudioScheduledReceiver *p = new CRTPMpeg12AudioScheduledReceiver(index, pPersistMediaConfig, pMsgSink);
                if (p) {
                    receceiver = (IScheduledRTPReceiver *)p;
                    client = (IScheduledClient *)p;
                } else {
                    LOG_FATAL("new CRTPMpeg12AudioScheduledReceiver(%d) fail.\n", index);
                    return EECode_NoMemory;
                }
            }
            break;

        default:
            LOG_FATAL("not supported streamformat %d.\n", format);
            return EECode_NotSupported;
            break;
    }

    return EECode_OK;
}

EECode gfCreateRTPScheduledReceiverTCP(TUint index, IScheduledRTPReceiver *&receceiver, IScheduledClient *&client, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    CRTPScheduledReceiverTCP *p = new CRTPScheduledReceiverTCP(index, pPersistMediaConfig, pMsgSink);
    if (p) {
        receceiver = (IScheduledRTPReceiver *)p;
        client = (IScheduledClient *)p;
    } else {
        LOG_FATAL("new CRTPScheduledReceiverTCP(%d) fail.\n", index);
        return EECode_NoMemory;
    }
    return EECode_OK;
}

//-----------------------------------------------------------------------
//
// CScheduledRTSPDemuxer
//
//-----------------------------------------------------------------------
CScheduledRTSPDemuxer::CScheduledRTSPDemuxer(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TUint index)
    : inherited(pname, index)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpMsgSink(pMsgSink)
    , mpScheduler(NULL)
    , mpMutex(NULL)
    , mbRTSPServerConnected(0)
    , mbOutputMsgSinkSet(0)
    , mbContextCreated(0)
    , mbContextRegistered(0)
    , mbEnableAudio(0)
    , mbEnableVideo(0)
    , mReconnectCount(0)
    , mMaxReconnectCount(32)
    , mPriority(1)
    , mbPreSetReceiveBufferSize(1)
    , mbPreSetSendBufferSize(0)
    , mbUseTCPMode(0)
    , mReceiveBufferSize(0)
    , mSendBufferSize(0)
    , mpServerAddr(NULL)
    , mpItemName(NULL)
    , mnUrlBufferSize(0)
    , mServerRTSPPort(DefaultRTSPServerPort)
    , mRTSPSeq(2)
    , mSessionID(0)
    , mClientRTPPortRangeBegin(DefaultRTPClientPortBegin)
    , mClientRTPPortRangeEnd(DefaultRTPClientPortEnd)
    , mClientRTPPortRangeBeginBase(DefaultRTPClientPortBegin)
    , mRTSPSocket(-1)
    , mpSourceUrl(NULL)
    , mSourceUrlLength(0)
    , mpVideoExtraDataFromSDP(NULL)
    , mVideoExtraDataFromSDPBufferSize(0)
    , mVideoExtraDataFromSDPSize(0)
    , mpAudioExtraDataFromSDP(NULL)
    , mAudioExtraDataFromSDPBufferSize(0)
    , mAudioExtraDataFromSDPSize(0)
{
    mbSendTeardown = 0;

    mpVideoPostProcessingCallback = NULL;
    mpVideoPostProcessingCallbackContext = NULL;
    mpAudioPostProcessingCallback = NULL;
    mpAudioPostProcessingCallbackContext = NULL;
}

IDemuxer *CScheduledRTSPDemuxer::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TUint index)
{
    CScheduledRTSPDemuxer *result = new CScheduledRTSPDemuxer(pname, pPersistMediaConfig, pMsgSink, index);
    if (result && result->Construct() != EECode_OK) {
        LOG_ERROR("CRTSPDemuxer->Construct() fail\n");
        delete result;
        result = NULL;
    }
    return result;
}

CObject *CScheduledRTSPDemuxer::GetObject0() const
{
    return (CObject *) this;
}

EECode CScheduledRTSPDemuxer::Construct()
{
    TUint i = 0;

    DSET_MODULE_LOG_CONFIG(ELogModulePrivateRTSPScheduledDemuxer);
    memset(mRTPContext, 0x0, sizeof(mRTPContext));

    mpReceiverTCP = NULL;
    mpScheduledClientTCP = NULL;

    for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        mpReceiver[i] = NULL;
        mpScheduledClient[i] = NULL;
        mbRTPReceiverCreated[i] = 0;
        mbRTPReceiverRegistered[i] = 0;

        mRTPContext[i].rtp_socket = -1;
        mRTPContext[i].rtcp_socket = -1;
    }

    mClientRTPPortRangeBegin += 128 * mIndex;
    mClientRTPPortRangeBeginBase = mClientRTPPortRangeBegin;

    mReceiveBufferSize = 4 * 1024 * 1024;

    mpScheduler = gfGetNetworkReceiverScheduler((const volatile SPersistMediaConfig *)mpPersistMediaConfig, mIndex);
    DASSERT(mpScheduler);

    mpMutex = gfCreateMutex();
    DASSERT(mpMutex);

    return EECode_OK;
}

CScheduledRTSPDemuxer::~CScheduledRTSPDemuxer()
{
    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    mSourceUrlLength = 0;
    if (mpSourceUrl) {
        DDBG_FREE(mpSourceUrl, "DMSU");
        mpSourceUrl = NULL;
    }

    if (mpVideoExtraDataFromSDP) {
        DDBG_FREE(mpVideoExtraDataFromSDP, "DMve");
        mpVideoExtraDataFromSDP = NULL;
    }
    mVideoExtraDataFromSDPBufferSize = 0;

    if (mpAudioExtraDataFromSDP) {
        DDBG_FREE(mpAudioExtraDataFromSDP, "DMae");
        mpAudioExtraDataFromSDP = NULL;
    }
    mAudioExtraDataFromSDPBufferSize = 0;

    closeAllSockets();

}

void CScheduledRTSPDemuxer::closeAllSockets()
{
    TUint i = 0;

    if (DIsSocketHandlerValid(mRTSPSocket)) {
        gfNetCloseSocket(mRTSPSocket);
        mRTSPSocket = DInvalidSocketHandler;
    }

    for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        if (mRTPContext[i].rtp_socket >= 0) {
            gfNetCloseSocket(mRTPContext[i].rtp_socket);
            mRTPContext[i].rtp_socket = DInvalidSocketHandler;
        }

        if (mRTPContext[i].rtcp_socket >= 0) {
            gfNetCloseSocket(mRTPContext[i].rtcp_socket);
            mRTPContext[i].rtcp_socket = DInvalidSocketHandler;
        }
    }

}

EECode CScheduledRTSPDemuxer::createRTPDataReceivers()
{
    TUint i = 0;
    EECode err;

    for (i = 0; i < 2; i ++) {
        if (!mbRTPReceiverCreated[i]) {

            if (!mRTPContext[i].enabled || !mRTPContext[i].initialized) {
                continue;
            }

            DASSERT(!mpReceiver[i]);
            DASSERT(!mpScheduledClient[i]);

            err = gfCreateRTPScheduledReceiver(mRTPContext[i].format, mIndex, mpReceiver[i], mpScheduledClient[i], mpPersistMediaConfig, mpMsgSink);
            DASSERT_OK(err);
            if (EECode_OK == err) {
                mbRTPReceiverCreated[i] = 1;
                DASSERT(mpReceiver[i]);
                DASSERT(mpScheduledClient[i]);

                mRTPContext[i].server_addr = mpServerAddr;
                if (StreamType_Video == mRTPContext[i].type) {
                    mRTPContext[i].priority = 2;
                } else {
                    mRTPContext[i].priority = 0;
                }
                if (mpReceiver[i]) {
                    err = mpReceiver[i]->Initialize(&mRTPContext[i], &mStreamCodecInfos.info[i]);
                    DASSERT_OK(err);

                    if ((StreamType_Video == mRTPContext[i].type) && (mpVideoPostProcessingCallback && mpVideoPostProcessingCallbackContext)) {
                        mpReceiverTCP->SetVideoDataPostProcessingCallback(mpVideoPostProcessingCallbackContext, mpVideoPostProcessingCallback);
                    }

                    if ((StreamType_Audio == mRTPContext[i].type) && (mpAudioPostProcessingCallback && mpAudioPostProcessingCallbackContext)) {
                        mpReceiverTCP->SetAudioDataPostProcessingCallback(mpAudioPostProcessingCallbackContext, mpAudioPostProcessingCallback);
                    }
                }
            } else {
                LOGM_ERROR("gfCreateRTPScheduledReceiver fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        } else {
            LOGM_WARN("mbRTPReceiverCreated[%d] already created?\n", i);
            DASSERT(mpReceiver[i]);
            DASSERT(mpScheduledClient[i]);
        }
    }

    mbContextCreated = 1;

    return EECode_OK;
}

EECode CScheduledRTSPDemuxer::createRTPDataReceiversTCP()
{
    //LOGM_NOTICE("createRTPDataReceiversTCP() begin\n");

    EECode err = gfCreateRTPScheduledReceiverTCP(mIndex, mpReceiverTCP, mpScheduledClientTCP, mpPersistMediaConfig, mpMsgSink);
    if (EECode_OK == err) {
        if (mpReceiverTCP) {
            err = mpReceiverTCP->Initialize(&mRTPContext[0], &mStreamCodecInfos.info[0], 2);
            DASSERT_OK(err);
            if (mpVideoPostProcessingCallback && mpVideoPostProcessingCallbackContext) {
                mpReceiverTCP->SetVideoDataPostProcessingCallback(mpVideoPostProcessingCallbackContext, mpVideoPostProcessingCallback);
            }

            if (mpAudioPostProcessingCallback && mpAudioPostProcessingCallbackContext) {
                mpReceiverTCP->SetAudioDataPostProcessingCallback(mpAudioPostProcessingCallbackContext, mpAudioPostProcessingCallback);
            }
        }
    } else {
        LOGM_ERROR("gfCreateRTPScheduledReceiverTCP fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }
    mbContextCreated = 1;
    //LOGM_NOTICE("createRTPDataReceiversTCP() end\n");

    return EECode_OK;
}

void CScheduledRTSPDemuxer::reinitializeRTPDataReceivers()
{
    TUint i = 0;

    for (i = 0; i < 2; i ++) {
        if (mpReceiver[i]) {
            mpReceiver[i]->ReInitialize(&mRTPContext[i], &mStreamCodecInfos.info[i]);
        }
    }

}

void CScheduledRTSPDemuxer::reinitializeRTPDataReceiversTCP()
{
    if (mpReceiverTCP) {
        LOGM_NOTICE("reinitializeRTPDataReceiversTCP() begin\n");
        mpReceiverTCP->ReInitialize(&mRTPContext[0], &mStreamCodecInfos.info[0], 2);
        LOGM_NOTICE("reinitializeRTPDataReceiversTCP() end\n");
    }
}

void CScheduledRTSPDemuxer::destroyRTPDataReceivers()
{
    TUint i = 0;

    for (i = 0; i < 2; i ++) {
        if (mbRTPReceiverCreated[i]) {

            DASSERT(mpReceiver[i]);
            DASSERT(mpScheduledClient[i]);

            mpReceiver[i]->Destroy();
            mpReceiver[i] = NULL;
            mpScheduledClient[i] = NULL;

            mbRTPReceiverCreated[i] = 0;

        } else {
            DASSERT(!mpReceiver[i]);
            DASSERT(!mpScheduledClient[i]);
        }
    }

    mbContextCreated = 0;
}

void CScheduledRTSPDemuxer::destroyRTPDataReceiversTCP()
{
    if (mpReceiverTCP) {
        mpReceiverTCP->Destroy();
        mpReceiverTCP = NULL;
    }
    mpScheduledClientTCP = NULL;

    mbContextCreated = 0;
}

EECode CScheduledRTSPDemuxer::startRTPDataReceiver()
{
    TUint i = 0;
    EECode err;

    LOGM_INFO("[CScheduledRTSPDemuxer flow]: startRTPDataReceiver(), start\n");

    if (!mpScheduler) {
        LOGM_ERROR("mpScheduler is NULL\n");
        return EECode_BadState;
    }

    if (!mbContextCreated) {
        createRTPDataReceivers();
    }

    DASSERT(!mbContextRegistered);

    for (i = 0; i < 2; i ++) {
        if (mpScheduledClient[i] && mRTPContext[i].enabled && mRTPContext[i].initialized && !mbRTPReceiverRegistered[i]) {
            DASSERT(mpReceiver[i]);
            LOGM_INFO("%d, mpReceiver[i] %p, mpVideoExtraDataFromSDP %p, mVideoExtraDataFromSDPSize %ld\n", i, mpReceiver[i], mpVideoExtraDataFromSDP, mVideoExtraDataFromSDPSize);
            if (DLikely((EDemuxerVideoOutputPinIndex == i) && (mpReceiver[i]) && mpVideoExtraDataFromSDP && mVideoExtraDataFromSDPSize)) {
                mpReceiver[i]->SetExtraData(mpVideoExtraDataFromSDP, mVideoExtraDataFromSDPSize, i);
            } else if (DLikely((EDemuxerAudioOutputPinIndex == i) && (mpReceiver[i]) && mpAudioExtraDataFromSDP && mAudioExtraDataFromSDPSize)) {
                mpReceiver[i]->SetExtraData(mpAudioExtraDataFromSDP, mAudioExtraDataFromSDPSize, i);
            }

            err = mpScheduler->AddScheduledCilent(mpScheduledClient[i]);
            mbRTPReceiverRegistered[i] = 1;
            DASSERT_OK(err);
        }
    }

    LOGM_INFO("[CScheduledRTSPDemuxer flow]: startRTPDataReceiver(), end\n");
    mbContextRegistered = 1;

    return EECode_OK;
}

EECode CScheduledRTSPDemuxer::stopRTPDataReceiver()
{
    TUint i = 0;
    EECode err;

    LOGM_INFO("[CScheduledRTSPDemuxer flow]: stopRTPDataReciever(), start\n");

    if (!mpScheduler) {
        LOGM_ERROR("mpScheduler is NULL\n");
        return EECode_BadState;
    }

    for (i = 0; i < 2; i ++) {
        //DASSERT(mpScheduledClient[i]);
        //DASSERT(mRTPContext[i].enabled);
        //DASSERT(mRTPContext[i].initialized);
        if (mpScheduledClient[i] && mRTPContext[i].enabled && mRTPContext[i].initialized && mbRTPReceiverRegistered[i]) {
            err = mpScheduler->RemoveScheduledCilent(mpScheduledClient[i]);
            //mpScheduler->PrintStatus();
            mbRTPReceiverRegistered[i] = 0;
            DASSERT_OK(err);
        }
    }

    mbContextRegistered = 0;

    LOGM_INFO("[CScheduledRTSPDemuxer flow]: stopRTPDataReciever(), end\n");

    return EECode_OK;
}

EECode CScheduledRTSPDemuxer::startRTPDataReceiverTCP()
{
    //LOGM_NOTICE("startRTPDataReceiverTCP() begin\n");

    if (!mpScheduler) {
        LOGM_ERROR("mpScheduler is NULL\n");
        return EECode_BadState;
    }

    if (!mbContextCreated) {
        createRTPDataReceiversTCP();
    }

    if (DUnlikely(mbContextRegistered)) {
        LOGM_ERROR("Already registered?\n");
        return EECode_BadState;
    }

    mpScheduler->AddScheduledCilent(mpScheduledClientTCP);

    if (mpVideoExtraDataFromSDP && mVideoExtraDataFromSDPSize) {
        mpReceiverTCP->SetExtraData(mpVideoExtraDataFromSDP, mVideoExtraDataFromSDPSize, EDemuxerVideoOutputPinIndex);
    }
    if (mpAudioExtraDataFromSDP && mAudioExtraDataFromSDPSize) {
        mpReceiverTCP->SetExtraData(mpAudioExtraDataFromSDP, mAudioExtraDataFromSDPSize, EDemuxerAudioOutputPinIndex);
    }

    mbContextRegistered = 1;
    //LOGM_NOTICE("startRTPDataReceiverTCP() end\n");

    return EECode_OK;
}

EECode CScheduledRTSPDemuxer::stopRTPDataReceiverTCP()
{
    //LOGM_NOTICE("stopRTPDataReceiverTCP() begin\n");

    if (!mpScheduler) {
        LOGM_ERROR("mpScheduler is NULL\n");
        return EECode_BadState;
    }

    if (DUnlikely(!mbContextRegistered)) {
        LOGM_ERROR("Not registered?\n");
        return EECode_BadState;
    }

    mpScheduler->RemoveScheduledCilent(mpScheduledClientTCP);

    mbContextRegistered = 0;

    //LOGM_NOTICE("stopRTPDataReceiverTCP() end\n");

    return EECode_OK;
}

EECode CScheduledRTSPDemuxer::connectToRTSPServer(TChar *pFileName)
{
    const TChar *rtsp_prefix = "rtsp://";
    char *ptmp = NULL;
    TInt ret = 0, ret1 = 0;
    EECode err;
    TInt retry_count = 5;

    SVideoRTPParams video_param;
    SAudioRTPParams audio_param;
    TU8 has_video = 0;
    TU8 has_audio = 0;
    TU8 setup_video = 0;
    TU8 setup_audio = 0;
    TMemSize str_length = 0;

    TU8 *p_video_extra_data = NULL;
    TMemSize video_extra_data_size = 0;
    TU8 has_video_extradata = 0;
    TU8 has_audio_extradata = 0;

    LOGM_INFO("[rtsp flow]: parse url start.\n");

    if (!pFileName) {
        LOGM_ERROR("NULL pFileName\n");
        return EECode_BadParam;
    }

    if ((strlen(pFileName) < strlen(rtsp_prefix)) || strncmp(rtsp_prefix, pFileName, strlen(rtsp_prefix))) {
        LOGM_ERROR("BAD rtsp url: %s, should be like '%s10.0.0.2/stream_0'.\n", pFileName, rtsp_prefix);
        return EECode_BadParam;
    }

    str_length = strlen(pFileName);
    if (DUnlikely(mpServerAddr && (str_length > mnUrlBufferSize))) {
        DDBG_FREE(mpServerAddr, "DMSU");
        mpServerAddr = NULL;
    }

    if (DUnlikely(!mpServerAddr)) {
        mpServerAddr = (char *)DDBG_MALLOC(str_length + 1, "DMSU");
        if (DUnlikely(!mpServerAddr)) {
            LOGM_FATAL("NO memory, request size %ld\n", str_length + 1);
            return EECode_NoMemory;
        }
        mnUrlBufferSize = str_length;
    }
    memset(mpServerAddr, 0x0, str_length + 1);

    if (DUnlikely(!mpItemName)) {
        mpItemName = (char *)DDBG_MALLOC(str_length + 1, "DMIU");
        if (DUnlikely(!mpItemName)) {
            LOGM_FATAL("NO memory, request size %ld\n", str_length + 1);
            return EECode_NoMemory;
        }
        mnUrlBufferSize = str_length;
    }
    memset(mpItemName, 0x0, str_length + 1);

    err = _parse_rtsp_url(pFileName, mpServerAddr, mpItemName, mServerRTSPPort);
    if (DUnlikely(EECode_OK != err)) {
        LOGM_ERROR("_parse_rtsp_url %s fail.\n", pFileName);
        return err;
    }

    LOGM_INFO("[rtsp flow]: Parse rtsp url done, server addr %s, port %d, item name %s, start connect to server....\n", mpServerAddr, mServerRTSPPort, mpItemName);

retry_connect:

    //connect to server, use tcp
    mRTSPSocket = gfNet_ConnectTo((const char *) mpServerAddr, mServerRTSPPort, SOCK_STREAM, IPPROTO_TCP);

    if (mRTSPSocket >= 0) {
        LOGM_INFO("[rtsp flow]: connect to server done.\n");
    } else {
        LOGM_ERROR("[rtsp flow]: connect to server fail.\n");
        return EECode_Error;
    }

    //send OPTIONS request
    memset(mRTSPBuffer, 0x0, DMAX_RTSP_STRING_LEN);
    snprintf(mRTSPBuffer, DMAX_RTSP_STRING_LEN - 1, _rtsp_option_request_fmt, mpServerAddr, mServerRTSPPort, mpItemName, mRTSPSeq ++);
    LOGM_INFO("[rtsp flow]: before send OPTIONS request....\r\n    %s, length %lu\n", mRTSPBuffer, (TULong)strlen(mRTSPBuffer));
    ret = gfNet_Send(mRTSPSocket, (TU8 *)mRTSPBuffer, strlen(mRTSPBuffer), 0);
    if (DLikely(ret == ((TInt)strlen(mRTSPBuffer)))) {
        LOGM_INFO("[rtsp flow]: send OPTIONS request done, sent len %d, read responce from server\n", ret);
    } else {
        if (!ret) {
            LOGM_ERROR("peer close? retry\n");
            gfNetCloseSocket(mRTSPSocket);
            mRTSPSocket = DInvalidSocketHandler;
            if (retry_count > 0) {
                retry_count --;
                goto retry_connect;
            } else {
                LOGM_ERROR("max retry count reaches, return error\n");
                return EECode_Error;
            }
        } else {
            LOGM_ERROR("socket error, ret %d\n", ret);
            return EECode_Error;
        }
    }

    ret = gfNet_Recv(mRTSPSocket, (TU8 *)mRTSPBuffer, DMAX_RTSP_STRING_LEN, 0);
    if (DUnlikely(!ret)) {
        LOGM_ERROR("peer close? retry\n");
        gfNetCloseSocket(mRTSPSocket);
        mRTSPSocket = DInvalidSocketHandler;
        if (retry_count > 0) {
            retry_count --;
            goto retry_connect;
        } else {
            LOGM_ERROR("max retry count reaches, return error\n");
            return EECode_Error;
        }
    } else if (DUnlikely(0 > ret)) {
        LOGM_ERROR("socket error, ret %d\n", ret);
        return EECode_Error;
    } else {
        LOGM_INFO("[rtsp flow]: OPTIONS's responce from server:   %s, length %d\n", mRTSPBuffer, ret);
    }

    //send DESCRIBE request
    memset(mRTSPBuffer, 0x0, DMAX_RTSP_STRING_LEN);
    snprintf(mRTSPBuffer, DMAX_RTSP_STRING_LEN - 1, _rtsp_describe_request_fmt, mpServerAddr, mServerRTSPPort, mpItemName, mRTSPSeq ++);
    LOGM_INFO("[rtsp flow]: before send DESCRIBE request....\r\n    %s, length %lu\n", mRTSPBuffer, (TULong)strlen(mRTSPBuffer));
    ret = gfNet_Send(mRTSPSocket, (TU8 *)mRTSPBuffer, strlen(mRTSPBuffer), 0);
    if (DLikely(ret == ((TInt)strlen(mRTSPBuffer)))) {
        LOGM_INFO("[rtsp flow]: send DESCRIBE request done, sent len %d, read responce from server\n", ret);
    } else {
        if (!ret) {
            LOGM_ERROR("peer close? retry\n");
            gfNetCloseSocket(mRTSPSocket);
            mRTSPSocket = DInvalidSocketHandler;
            if (retry_count > 0) {
                retry_count --;
                goto retry_connect;
            } else {
                LOGM_ERROR("max retry count reaches, return error\n");
                return EECode_Error;
            }
        } else {
            LOGM_ERROR("socket error, ret %d\n", ret);
            return EECode_Error;
        }
    }

    memset(&video_param, 0x0, sizeof(video_param));
    memset(&audio_param, 0x0, sizeof(audio_param));

    ret = gfNet_Recv(mRTSPSocket, (TU8 *)mRTSPBuffer, DMAX_RTSP_STRING_LEN, 0);
    if (DUnlikely(!ret)) {
        LOGM_ERROR("peer close? retry\n");
        gfNetCloseSocket(mRTSPSocket);
        mRTSPSocket = DInvalidSocketHandler;
        if (retry_count > 0) {
            retry_count --;
            goto retry_connect;
        } else {
            LOGM_ERROR("max retry count reaches, return error\n");
            return EECode_Error;
        }
    } else if (DUnlikely(0 > ret)) {
        LOGM_ERROR("socket error, ret %d\n", ret);
        return EECode_Error;
    } else {
        LOGM_INFO("[rtsp flow]: DESCRIBE's responce from server:   %s, length %d\n", mRTSPBuffer, ret);
    }

    __parseSDP(mRTSPBuffer, ret, has_video, has_audio, has_video_extradata, has_audio_extradata, &video_param, &audio_param);

    if ((!has_video) && (!has_audio)) {
        LOGM_NOTICE("not receive sdp yet, try read again\n");
        ret1 = gfNet_Recv(mRTSPSocket, (TU8 *)mRTSPBuffer + ret, DMAX_RTSP_STRING_LEN - ret, 0);
        if (DUnlikely(!ret)) {
            LOGM_ERROR("peer close? retry\n");
            gfNetCloseSocket(mRTSPSocket);
            mRTSPSocket = DInvalidSocketHandler;
            if (retry_count > 0) {
                retry_count --;
                goto retry_connect;
            } else {
                LOGM_ERROR("max retry count reaches, return error\n");
                return EECode_Error;
            }
        } else if (DUnlikely(0 > ret)) {
            LOGM_ERROR("socket error, ret %d\n", ret);
            return EECode_Error;
        } else {
            LOGM_INFO("[rtsp flow]: DESCRIBE's responce from server(part 2):   %s, length %d\n", mRTSPBuffer, ret + ret1);
        }
        __parseSDP(mRTSPBuffer, ret + ret1, has_video, has_audio, has_video_extradata, has_audio_extradata, &video_param, &audio_param);
    }

    if (has_video_extradata) {
        if (DLikely(video_param.p_extra_data_sps_base64 && video_param.extra_data_sps_base64_size && video_param.p_extra_data_pps_base64 && video_param.extra_data_pps_base64_size)) {
            video_extra_data_size = video_param.extra_data_sps_base64_size + video_param.extra_data_pps_base64_size + 64;
            if (mpVideoExtraDataFromSDP) {
                if (mVideoExtraDataFromSDPBufferSize < video_extra_data_size) {
                    DDBG_FREE(mpVideoExtraDataFromSDP, "DMve");
                    mpVideoExtraDataFromSDP = NULL;
                    mVideoExtraDataFromSDPBufferSize = 0;
                }
            }

            if (!mpVideoExtraDataFromSDP) {
                mpVideoExtraDataFromSDP = (TU8 *)DDBG_MALLOC(video_extra_data_size, "DMve");
                if (DLikely(mpVideoExtraDataFromSDP)) {
                    mVideoExtraDataFromSDPBufferSize = video_extra_data_size;
                } else {
                    LOGM_FATAL("DDBG_MALLOC(%ld) fail\n", video_extra_data_size);
                    return EECode_NoMemory;
                }
            }

            p_video_extra_data = mpVideoExtraDataFromSDP;
            mVideoExtraDataFromSDPSize = 0;

            p_video_extra_data[0] = 0x0;
            p_video_extra_data[1] = 0x0;
            p_video_extra_data[2] = 0x0;
            p_video_extra_data[3] = 0x01;
            //p_video_extra_data[4] = 0x67;
            p_video_extra_data += 4;
            mVideoExtraDataFromSDPSize += 4;

            TU8 *ptmp00 = NULL;
            //gfDecodingBase64(video_param.p_extra_data_sps_base64, p_video_extra_data, video_param.extra_data_sps_base64_size, video_extra_data_size);
            video_extra_data_size = (video_param.extra_data_sps_base64_size / 4) * 3;
            ptmp00 = video_param.p_extra_data_sps_base64 + video_param.extra_data_sps_base64_size - 1;
            gfDecodingBase64(p_video_extra_data, video_param.p_extra_data_sps_base64, video_extra_data_size);
            while ('=' == ptmp00[0]) {
                video_extra_data_size --;
                ptmp00 --;
                //LOGM_INFO("minor 1\n");
            }
            p_video_extra_data += video_extra_data_size;
            mVideoExtraDataFromSDPSize += video_extra_data_size;

            p_video_extra_data[0] = 0x0;
            p_video_extra_data[1] = 0x0;
            p_video_extra_data[2] = 0x0;
            p_video_extra_data[3] = 0x01;
            //p_video_extra_data[4] = 0x68;
            p_video_extra_data += 4;
            mVideoExtraDataFromSDPSize += 4;

            //gfDecodingBase64(video_param.p_extra_data_pps_base64, p_video_extra_data, video_param.extra_data_pps_base64_size, video_extra_data_size);
            video_extra_data_size = (video_param.extra_data_pps_base64_size / 4) * 3;
            ptmp00 = video_param.p_extra_data_pps_base64 + video_param.extra_data_pps_base64_size - 1;
            gfDecodingBase64(p_video_extra_data, video_param.p_extra_data_pps_base64, video_extra_data_size);
            while ('=' == ptmp00[0]) {
                video_extra_data_size --;
                ptmp00 --;
                //LOGM_INFO("minor 1\n");
            }
            mVideoExtraDataFromSDPSize += video_extra_data_size;
#if 0
            LOGM_INFO("mVideoExtraDataFromSDPSize %ld\n", mVideoExtraDataFromSDPSize);
            LOGM_INFO("%02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x\n", \
                      mpVideoExtraDataFromSDP[0], mpVideoExtraDataFromSDP[1], mpVideoExtraDataFromSDP[2], mpVideoExtraDataFromSDP[3], \
                      mpVideoExtraDataFromSDP[4], mpVideoExtraDataFromSDP[5], mpVideoExtraDataFromSDP[6], mpVideoExtraDataFromSDP[7], \
                      mpVideoExtraDataFromSDP[8], mpVideoExtraDataFromSDP[9], mpVideoExtraDataFromSDP[10], mpVideoExtraDataFromSDP[11], \
                      mpVideoExtraDataFromSDP[12], mpVideoExtraDataFromSDP[13], mpVideoExtraDataFromSDP[14], mpVideoExtraDataFromSDP[15]);
#endif

            //check video width.height
            if (0 == video_param.width && 0 == video_param.height) {
                EECode ret = EECode_OK;
                SCodecVideoCommon *p_video_parser = gfGetVideoCodecParser(mpVideoExtraDataFromSDP + 5, mVideoExtraDataFromSDPSize - 5, StreamFormat_H264, ret);
                if (!p_video_parser || EECode_OK != ret) {
                    LOGM_ERROR("gfGetVideoCodecParser failed, ret=%d\n", ret);
                } else {
                    video_param.width = p_video_parser->max_width;
                    video_param.height = p_video_parser->max_height;
                    LOGM_NOTICE("gfGetVideoCodecParser, got video width=%u, height=%u\n", video_param.width, video_param.height);
                }
            }

        } else {
            LOGM_WARN("no video extra data, %p, %ld\n", p_video_extra_data, video_extra_data_size);
            mVideoExtraDataFromSDPSize = 0;
        }
    } else {
        mVideoExtraDataFromSDPSize = 0;
    }

    if (has_audio_extradata) {
        if (DLikely(audio_param.p_extra_data_base16 && audio_param.extra_data_base16_size)) {

            if (mpAudioExtraDataFromSDP) {
                if (mAudioExtraDataFromSDPBufferSize < (audio_param.extra_data_base16_size / 2)) {
                    DDBG_FREE(mpAudioExtraDataFromSDP, "DMve");
                    mpAudioExtraDataFromSDP = NULL;
                    mVideoExtraDataFromSDPBufferSize = 0;
                }
            }

            if (!mpAudioExtraDataFromSDP) {
                mpAudioExtraDataFromSDP = (TU8 *)DDBG_MALLOC(audio_param.extra_data_base16_size / 2, "DMve");
                if (DLikely(mpAudioExtraDataFromSDP)) {
                    mAudioExtraDataFromSDPBufferSize = (audio_param.extra_data_base16_size / 2);
                } else {
                    LOGM_FATAL("DDBG_MALLOC(%ld) fail\n", (audio_param.extra_data_base16_size / 2));
                    return EECode_NoMemory;
                }
            }

            mAudioExtraDataFromSDPSize = audio_param.extra_data_base16_size / 2;
            gfDecodingBase16(mpAudioExtraDataFromSDP, (const TU8 *)audio_param.p_extra_data_base16, audio_param.extra_data_base16_size);

            LOGM_NOTICE("mAudioExtraDataFromSDPSize %ld\n", mAudioExtraDataFromSDPSize);
            LOGM_ALWAYS("%02x %02x\n", mpAudioExtraDataFromSDP[0], mpAudioExtraDataFromSDP[1]);
        } else {
            LOGM_WARN("no audio extra data, %p, %ld\n", audio_param.p_extra_data_base16, audio_param.extra_data_base16_size);
            mAudioExtraDataFromSDPSize = 0;
        }
    } else {
        mAudioExtraDataFromSDPSize = 0;
    }

    if (video_param.width && video_param.height) {
        postVideoSizeMsg(video_param.width, video_param.height);
    }

    //need parse responce here, fix me
    //LOGM_WARN("please implement parsing DESCRIBE's responce, todo...\n");

    if (has_video) {
        if (!video_param.width || !video_param.height) {
            video_param.width = 720;
            video_param.height = 480;
            LOGM_NOTICE("do not get video width height, guess default width %d, height %d\n", video_param.width, video_param.height);
        } else {
            LOGM_NOTICE("get video width %d, height %d\n", video_param.width, video_param.height);
        }

        if (!video_param.framerate) {
            video_param.framerate = 30;
            LOGM_NOTICE("do not get video framerate, guess default framerate %f\n", video_param.framerate);
        } else {
            LOGM_NOTICE("get video framerate %f, %p\n", video_param.framerate, &video_param);
        }
        mStreamCodecInfos.info[EDemuxerVideoOutputPinIndex].spec.video.pic_width = video_param.width;
        mStreamCodecInfos.info[EDemuxerVideoOutputPinIndex].spec.video.pic_height = video_param.height;
        mStreamCodecInfos.info[EDemuxerVideoOutputPinIndex].spec.video.framerate = video_param.framerate;
        if (DLikely(video_param.framerate)) {
            mStreamCodecInfos.info[EDemuxerVideoOutputPinIndex].spec.video.framerate_num = DDefaultVideoFramerateNum;
            mStreamCodecInfos.info[EDemuxerVideoOutputPinIndex].spec.video.framerate_den = DDefaultVideoFramerateNum / video_param.framerate;
        } else {
            mStreamCodecInfos.info[EDemuxerVideoOutputPinIndex].spec.video.framerate_num = DDefaultVideoFramerateNum;
            mStreamCodecInfos.info[EDemuxerVideoOutputPinIndex].spec.video.framerate_den = DDefaultVideoFramerateDen;
        }
        mStreamCodecInfos.info[EDemuxerVideoOutputPinIndex].stream_enabled = mbEnableVideo;
        mStreamCodecInfos.info[EDemuxerVideoOutputPinIndex].stream_presence = 1;
        mStreamCodecInfos.info[EDemuxerVideoOutputPinIndex].stream_type = StreamType_Video;
        DASSERT(StreamFormat_H264 == video_param.format);
        mStreamCodecInfos.info[EDemuxerVideoOutputPinIndex].stream_format = video_param.format;
        mStreamCodecInfos.info[EDemuxerVideoOutputPinIndex].payload_type = video_param.rtp_payload_type;

        mRTPContext[EDemuxerVideoOutputPinIndex].enabled = mbEnableVideo;
        mRTPContext[EDemuxerVideoOutputPinIndex].track_id = video_param.rtp_track_index;
        mRTPContext[EDemuxerVideoOutputPinIndex].format = StreamFormat_H264;
        mRTPContext[EDemuxerVideoOutputPinIndex].type = StreamType_Video;
    }

    if (has_audio) {
        mStreamCodecInfos.info[EDemuxerAudioOutputPinIndex].spec.audio.sample_rate = audio_param.sample_rate;
        mStreamCodecInfos.info[EDemuxerAudioOutputPinIndex].spec.audio.channel_number = audio_param.channel_number;
        mStreamCodecInfos.info[EDemuxerAudioOutputPinIndex].stream_enabled = mbEnableAudio;
        mStreamCodecInfos.info[EDemuxerAudioOutputPinIndex].stream_presence = 1;
        mStreamCodecInfos.info[EDemuxerAudioOutputPinIndex].stream_type = StreamType_Audio;
        mStreamCodecInfos.info[EDemuxerAudioOutputPinIndex].stream_format = audio_param.format;
        mStreamCodecInfos.info[EDemuxerAudioOutputPinIndex].payload_type = audio_param.rtp_payload_type;

        mRTPContext[EDemuxerAudioOutputPinIndex].enabled = mbEnableAudio;
        mRTPContext[EDemuxerAudioOutputPinIndex].track_id = audio_param.rtp_track_index;
        mRTPContext[EDemuxerAudioOutputPinIndex].format = audio_param.format;
        mRTPContext[EDemuxerAudioOutputPinIndex].type = StreamType_Audio;
    }

    LOGM_INFO("has_video %d, mRTPContext[EDemuxerVideoOutputPinIndex].enabled %d\n", has_video, mRTPContext[EDemuxerVideoOutputPinIndex].enabled);

    if (has_video && mRTPContext[EDemuxerVideoOutputPinIndex].enabled) {
        //build video rtp/rtcp socket
        LOGM_INFO("mClientRTPPortRangeBegin %d\n", mClientRTPPortRangeBegin);
        err = _try_open_rtprtcp_port(mClientRTPPortRangeBegin, mClientRTPPortRangeEnd, mRTPContext[EDemuxerVideoOutputPinIndex].rtp_port, mRTPContext[EDemuxerVideoOutputPinIndex].rtp_socket, mRTPContext[EDemuxerVideoOutputPinIndex].rtcp_socket, mReceiveBufferSize, mSendBufferSize);
        if (EECode_OK != err) {
            LOGM_ERROR("cannot alloc client RTP/RTCP(video) port.\n");
            return EECode_Error;
        }
        mRTPContext[EDemuxerVideoOutputPinIndex].rtcp_port = mRTPContext[EDemuxerVideoOutputPinIndex].rtp_port + 1;
        LOGM_INFO("mClientRTPPortRangeBegin %d, mRTPContext[EDemuxerVideoOutputPinIndex].rtp_port %d, rtp socket %d, rtcp socket %d\n", mClientRTPPortRangeBegin, mRTPContext[EDemuxerVideoOutputPinIndex].rtp_port, mRTPContext[EDemuxerVideoOutputPinIndex].rtp_socket, mRTPContext[EDemuxerVideoOutputPinIndex].rtcp_socket);

        //send SETUP request
        memset(mRTSPBuffer, 0x0, DMAX_RTSP_STRING_LEN);
        snprintf(mRTSPBuffer, DMAX_RTSP_STRING_LEN - 1, _rtsp_setup_request_fmt, mpServerAddr, mServerRTSPPort, mpItemName, video_param.track_string, mRTSPSeq ++, mRTPContext[EDemuxerVideoOutputPinIndex].rtp_port, mRTPContext[EDemuxerVideoOutputPinIndex].rtcp_port);
        LOGM_INFO("[rtsp flow]: before send SETUP request....\r\n    %s, length %lu\n", mRTSPBuffer, (TULong)strlen(mRTSPBuffer));
        ret = gfNet_Send(mRTSPSocket, (TU8 *)mRTSPBuffer, strlen(mRTSPBuffer), 0);

        if (DLikely(ret == ((TInt)strlen(mRTSPBuffer)))) {
            LOGM_INFO("[rtsp flow]: send SETUP request done, sent len %d, read responce from server\n", ret);
        } else {
            if (!ret) {
                LOGM_ERROR("peer close? retry\n");
                gfNetCloseSocket(mRTSPSocket);
                mRTSPSocket = DInvalidSocketHandler;
                if (retry_count > 0) {
                    retry_count --;
                    goto retry_connect;
                } else {
                    LOGM_ERROR("max retry count reaches, return error\n");
                    return EECode_Error;
                }
            } else {
                LOGM_ERROR("socket error, ret %d\n", ret);
                return EECode_Error;
            }
        }

        ret = gfNet_Recv(mRTSPSocket, (TU8 *)mRTSPBuffer, DMAX_RTSP_STRING_LEN, 0);
        if (DUnlikely(!ret)) {
            LOGM_ERROR("peer close? retry\n");
            gfNetCloseSocket(mRTSPSocket);
            mRTSPSocket = DInvalidSocketHandler;
            if (retry_count > 0) {
                retry_count --;
                goto retry_connect;
            } else {
                LOGM_ERROR("max retry count reaches, return error\n");
                return EECode_Error;
            }
        } else if (DUnlikely(0 > ret)) {
            LOGM_ERROR("socket error, ret %d\n", ret);
            return EECode_Error;
        } else {
            LOGM_INFO("[rtsp flow]: SETUP's responce from server:   %s, length %d\n", mRTSPBuffer, ret);
        }

        //find server's RTP/RTCP port(video)
        if ((ptmp = _find_string((char *)mRTSPBuffer, DMAX_RTSP_STRING_LEN, "server_port=", strlen("server_port=")))) {
            sscanf(ptmp, "server_port=%hu-%hu", &mRTPContext[EDemuxerVideoOutputPinIndex].server_rtp_port, &mRTPContext[EDemuxerVideoOutputPinIndex].server_rtcp_port);
            LOGM_INFO("[rtsp flow]: in SETUP's responce, server's video RTP port %d, RTCP port %d.\n", mRTPContext[EDemuxerVideoOutputPinIndex].server_rtp_port, mRTPContext[EDemuxerVideoOutputPinIndex].server_rtcp_port);
        } else {
            LOGM_ERROR("cannot find server RTP/RTCP(video) port.\n");
            return EECode_Error;
        }

        //find sessionID
        if ((ptmp = _find_string((char *)mRTSPBuffer, DMAX_RTSP_STRING_LEN, "Session: ", strlen("Session: ")))) {
            sscanf(ptmp, "Session: %llx", &mSessionID);
            LOGM_INFO("[rtsp flow]: in SETUP's responce, get session ID %llx.\n", mSessionID);
        } else {
            LOGM_ERROR("cannot find session ID.\n");
            return EECode_Error;
        }

        setup_video = 1;
    }

    LOGM_INFO("has_audio %d, mRTPContext[EDemuxerAudioOutputPinIndex].enabled %d\n", has_audio, mRTPContext[EDemuxerAudioOutputPinIndex].enabled);

    if (has_audio && mRTPContext[EDemuxerAudioOutputPinIndex].enabled) {
        err = _try_open_rtprtcp_port(mClientRTPPortRangeBegin + 2, mClientRTPPortRangeEnd, mRTPContext[EDemuxerAudioOutputPinIndex].rtp_port, mRTPContext[EDemuxerAudioOutputPinIndex].rtp_socket, mRTPContext[EDemuxerAudioOutputPinIndex].rtcp_socket, 0, 0);
        if (EECode_OK != err) {
            LOGM_ERROR("cannot alloc client RTP/RTCP(audio) port.\n");
            return EECode_Error;
        }
        mRTPContext[EDemuxerAudioOutputPinIndex].rtcp_port = mRTPContext[EDemuxerAudioOutputPinIndex].rtp_port + 1;

        //send SETUP request
        memset(mRTSPBuffer, 0x0, DMAX_RTSP_STRING_LEN);
        snprintf(mRTSPBuffer, DMAX_RTSP_STRING_LEN - 1, _rtsp_setup_request_with_sessionid_fmt, mpServerAddr, mServerRTSPPort, mpItemName, audio_param.track_string, mRTSPSeq ++, mRTPContext[EDemuxerAudioOutputPinIndex].rtp_port, mRTPContext[EDemuxerAudioOutputPinIndex].rtcp_port, mSessionID);
        LOGM_INFO("[rtsp flow]: before send SETUP request....\r\n    %s, length %lu\n", mRTSPBuffer, (TULong)strlen(mRTSPBuffer));
        ret = gfNet_Send(mRTSPSocket, (TU8 *)mRTSPBuffer, strlen(mRTSPBuffer), 0);

        if (DLikely(ret == ((TInt)strlen(mRTSPBuffer)))) {
            LOGM_INFO("[rtsp flow]: send SETUP request done, sent len %d, read responce from server\n", ret);
        } else {
            if (!ret) {
                LOGM_ERROR("peer close? retry\n");
                gfNetCloseSocket(mRTSPSocket);
                mRTSPSocket = DInvalidSocketHandler;
                if (retry_count > 0) {
                    retry_count --;
                    goto retry_connect;
                } else {
                    LOGM_ERROR("max retry count reaches, return error\n");
                    return EECode_Error;
                }
            } else {
                LOGM_ERROR("socket error, ret %d\n", ret);
                return EECode_Error;
            }
        }

        ret = gfNet_Recv(mRTSPSocket, (TU8 *)mRTSPBuffer, DMAX_RTSP_STRING_LEN, 0);
        if (DUnlikely(!ret)) {
            LOGM_ERROR("peer close? retry\n");
            gfNetCloseSocket(mRTSPSocket);
            mRTSPSocket = DInvalidSocketHandler;
            if (retry_count > 0) {
                retry_count --;
                goto retry_connect;
            } else {
                LOGM_ERROR("max retry count reaches, return error\n");
                return EECode_Error;
            }
        } else if (DUnlikely(0 > ret)) {
            LOGM_ERROR("socket error, ret %d\n", ret);
            return EECode_Error;
        } else {
            LOGM_INFO("[rtsp flow]: SETUP's responce from server:   %s, length %d\n", mRTSPBuffer, ret);
        }

        //find server's RTP/RTCP port(audio)
        if ((ptmp = _find_string((char *)mRTSPBuffer, DMAX_RTSP_STRING_LEN, "server_port=", strlen("server_port=")))) {
            sscanf(ptmp, "server_port=%hu-%hu", &mRTPContext[EDemuxerAudioOutputPinIndex].server_rtp_port, &mRTPContext[EDemuxerAudioOutputPinIndex].server_rtcp_port);
            LOGM_INFO("[rtsp flow]: in SETUP's responce, server's audio RTP port %d, RTCP port %d.\n", mRTPContext[EDemuxerAudioOutputPinIndex].server_rtp_port, mRTPContext[EDemuxerAudioOutputPinIndex].server_rtcp_port);
            setup_audio = 1;
        } else {
            LOGM_ERROR("cannot find server RTP/RTCP(audio) port. responce %s\n", mRTSPBuffer);
            //return EECode_Error;
            setup_audio = 0;
        }

    }

    if (setup_video || setup_audio) {
        //send PLAY request
        memset(mRTSPBuffer, 0x0, DMAX_RTSP_STRING_LEN);
        snprintf(mRTSPBuffer, DMAX_RTSP_STRING_LEN - 1, _rtsp_play_request_fmt, mpServerAddr, mServerRTSPPort, mpItemName, mRTSPSeq ++, mSessionID);
        LOGM_INFO("[rtsp flow]: before send PLAY request....\r\n    %s, length %lu\n", mRTSPBuffer, (TULong)strlen(mRTSPBuffer));
        ret = gfNet_Send(mRTSPSocket, (TU8 *)mRTSPBuffer, strlen(mRTSPBuffer), 0);
        if (DLikely(ret == ((TInt)strlen(mRTSPBuffer)))) {
            LOGM_INFO("[rtsp flow]: send PLAY request done, sent len %d, read responce from server\n", ret);
        } else {
            if (!ret) {
                LOGM_ERROR("peer close? retry\n");
                gfNetCloseSocket(mRTSPSocket);
                mRTSPSocket = DInvalidSocketHandler;
                if (retry_count > 0) {
                    retry_count --;
                    goto retry_connect;
                } else {
                    LOGM_ERROR("max retry count reaches, return error\n");
                    return EECode_Error;
                }
            } else {
                LOGM_ERROR("socket error, ret %d\n", ret);
                return EECode_Error;
            }
        }

        ret = gfNet_Recv(mRTSPSocket, (TU8 *)mRTSPBuffer, DMAX_RTSP_STRING_LEN, 0);
        if (DUnlikely(!ret)) {
            LOGM_ERROR("peer close? retry\n");
            gfNetCloseSocket(mRTSPSocket);
            mRTSPSocket = DInvalidSocketHandler;
            if (retry_count > 0) {
                retry_count --;
                goto retry_connect;
            } else {
                LOGM_ERROR("max retry count reaches, return error\n");
                return EECode_Error;
            }
        } else if (DUnlikely(0 > ret)) {
            LOGM_ERROR("socket error, ret %d\n", ret);
            return EECode_Error;
        } else {
            LOGM_INFO("[rtsp flow]: PLAY's responce from server:   %s, length %d\n", mRTSPBuffer, ret);
        }

        mbRTSPServerConnected = 1;

        mRTPContext[EDemuxerVideoOutputPinIndex].index = mIndex;
        mRTPContext[EDemuxerAudioOutputPinIndex].index = mIndex;

        mRTPContext[EDemuxerVideoOutputPinIndex].rtsp_socket = mRTSPSocket;
        mRTPContext[EDemuxerAudioOutputPinIndex].rtsp_socket = mRTSPSocket;

        mRTPContext[EDemuxerVideoOutputPinIndex].initialized = 1;
        mRTPContext[EDemuxerAudioOutputPinIndex].initialized = 1;
    }

    return EECode_OK;
}

EECode CScheduledRTSPDemuxer::connectToRTSPServerTCP(TChar *pFileName)
{
    //LOGM_NOTICE("connectToRTSPServerTCP() begin\n");

    const TChar *rtsp_prefix = "rtsp://";
    char *ptmp = NULL;
    TInt ret = 0, ret1 = 0;
    EECode err;
    TInt retry_count = 5;

    SVideoRTPParams video_param;
    SAudioRTPParams audio_param;
    TU8 has_video = 0;
    TU8 has_audio = 0;
    TU8 setup_video = 0;
    TU8 setup_audio = 0;
    TMemSize str_length = 0;

    TU8 *p_video_extra_data = NULL;
    TMemSize video_extra_data_size = 0;
    TU8 has_video_extradata = 0;
    TU8 has_audio_extradata = 0;

    LOGM_INFO("[rtsp flow]: parse url start.\n");

    if (!pFileName) {
        LOGM_ERROR("NULL pFileName\n");
        return EECode_BadParam;
    }

    if ((strlen(pFileName) < strlen(rtsp_prefix)) || strncmp(rtsp_prefix, pFileName, strlen(rtsp_prefix))) {
        LOGM_ERROR("BAD rtsp url: %s, should be like '%s10.0.0.2/stream_0'.\n", pFileName, rtsp_prefix);
        return EECode_BadParam;
    }

    str_length = strlen(pFileName);
    if (DUnlikely(mpServerAddr && (str_length > mnUrlBufferSize))) {
        DDBG_FREE(mpServerAddr, "DMSU");
        mpServerAddr = NULL;
    }

    if (DUnlikely(!mpServerAddr)) {
        mpServerAddr = (char *)DDBG_MALLOC(str_length + 1, "DMSU");
        if (DUnlikely(!mpServerAddr)) {
            LOGM_FATAL("NO memory, request size %ld\n", str_length + 1);
            return EECode_NoMemory;
        }
        mnUrlBufferSize = str_length;
    }
    memset(mpServerAddr, 0x0, str_length + 1);

    if (DUnlikely(!mpItemName)) {
        mpItemName = (char *)DDBG_MALLOC(str_length + 1, "DMIU");
        if (DUnlikely(!mpItemName)) {
            LOGM_FATAL("NO memory, request size %ld\n", str_length + 1);
            return EECode_NoMemory;
        }
        mnUrlBufferSize = str_length;
    }
    memset(mpItemName, 0x0, str_length + 1);

    err = _parse_rtsp_url(pFileName, mpServerAddr, mpItemName, mServerRTSPPort);
    if (DUnlikely(EECode_OK != err)) {
        LOGM_ERROR("_parse_rtsp_url %s fail.\n", pFileName);
        return err;
    }

    LOGM_INFO("[rtsp flow]: Parse rtsp url done, server addr %s, port %d, item name %s, start connect to server....\n", mpServerAddr, mServerRTSPPort, mpItemName);

retry_connect_tcp:

    //connect to server, use tcp
    mRTSPSocket = gfNet_ConnectTo((const char *) mpServerAddr, mServerRTSPPort, SOCK_STREAM, IPPROTO_TCP);

    if (mRTSPSocket >= 0) {
        LOGM_INFO("[rtsp flow]: connect to server done.\n");
    } else {
        LOGM_ERROR("[rtsp flow]: connect to server fail.\n");
        return EECode_Error;
    }

    //send OPTIONS request
    memset(mRTSPBuffer, 0x0, DMAX_RTSP_STRING_LEN);
    snprintf(mRTSPBuffer, DMAX_RTSP_STRING_LEN - 1, _rtsp_option_request_fmt, mpServerAddr, mServerRTSPPort, mpItemName, mRTSPSeq ++);
    LOGM_INFO("[rtsp flow]: before send OPTIONS request....\r\n    %s, length %lu\n", mRTSPBuffer, (TULong)strlen(mRTSPBuffer));
    ret = gfNet_Send(mRTSPSocket, (TU8 *)mRTSPBuffer, strlen(mRTSPBuffer), 0);
    if (DLikely(ret == ((TInt)strlen(mRTSPBuffer)))) {
        LOGM_INFO("[rtsp flow]: send OPTIONS request done, sent len %d, read responce from server\n", ret);
    } else {
        if (!ret) {
            LOGM_ERROR("peer close? retry\n");
            gfNetCloseSocket(mRTSPSocket);
            mRTSPSocket = DInvalidSocketHandler;
            if (retry_count > 0) {
                retry_count --;
                goto retry_connect_tcp;
            } else {
                LOGM_ERROR("max retry count reaches, return error\n");
                return EECode_Error;
            }
        } else {
            LOGM_ERROR("socket error, ret %d\n", ret);
            return EECode_Error;
        }
    }

    ret = gfNet_Recv(mRTSPSocket, (TU8 *)mRTSPBuffer, DMAX_RTSP_STRING_LEN, 0);
    if (DUnlikely(!ret)) {
        LOGM_ERROR("peer close? retry\n");
        gfNetCloseSocket(mRTSPSocket);
        mRTSPSocket = DInvalidSocketHandler;
        if (retry_count > 0) {
            retry_count --;
            goto retry_connect_tcp;
        } else {
            LOGM_ERROR("max retry count reaches, return error\n");
            return EECode_Error;
        }
    } else if (DUnlikely(0 > ret)) {
        LOGM_ERROR("socket error, ret %d\n", ret);
        return EECode_Error;
    } else {
        LOGM_INFO("[rtsp flow]: OPTIONS's responce from server:   %s, length %d\n", mRTSPBuffer, ret);
    }

    //send DESCRIBE request
    memset(mRTSPBuffer, 0x0, DMAX_RTSP_STRING_LEN);
    snprintf(mRTSPBuffer, DMAX_RTSP_STRING_LEN - 1, _rtsp_describe_request_fmt, mpServerAddr, mServerRTSPPort, mpItemName, mRTSPSeq ++);
    LOGM_INFO("[rtsp flow]: before send DESCRIBE request....\r\n    %s, length %lu\n", mRTSPBuffer, (TULong)strlen(mRTSPBuffer));
    ret = gfNet_Send(mRTSPSocket, (TU8 *)mRTSPBuffer, strlen(mRTSPBuffer), 0);
    if (DLikely(ret == ((TInt)strlen(mRTSPBuffer)))) {
        LOGM_INFO("[rtsp flow]: send DESCRIBE request done, sent len %d, read responce from server\n", ret);
    } else {
        if (!ret) {
            LOGM_ERROR("peer close? retry\n");
            gfNetCloseSocket(mRTSPSocket);
            mRTSPSocket = DInvalidSocketHandler;
            if (retry_count > 0) {
                retry_count --;
                goto retry_connect_tcp;
            } else {
                LOGM_ERROR("max retry count reaches, return error\n");
                return EECode_Error;
            }
        } else {
            LOGM_ERROR("socket error, ret %d\n", ret);
            return EECode_Error;
        }
    }

    memset(&video_param, 0x0, sizeof(video_param));
    memset(&audio_param, 0x0, sizeof(audio_param));

    ret = gfNet_Recv(mRTSPSocket, (TU8 *)mRTSPBuffer, DMAX_RTSP_STRING_LEN, 0);
    if (DUnlikely(!ret)) {
        LOGM_ERROR("peer close? retry\n");
        gfNetCloseSocket(mRTSPSocket);
        mRTSPSocket = DInvalidSocketHandler;
        if (retry_count > 0) {
            retry_count --;
            goto retry_connect_tcp;
        } else {
            LOGM_ERROR("max retry count reaches, return error\n");
            return EECode_Error;
        }
    } else if (DUnlikely(0 > ret)) {
        LOGM_ERROR("socket error, ret %d\n", ret);
        return EECode_Error;
    } else {
        LOGM_INFO("[rtsp flow]: DESCRIBE's responce from server:   %s, length %d\n", mRTSPBuffer, ret);
    }

    __parseSDP(mRTSPBuffer, ret, has_video, has_audio, has_video_extradata, has_audio_extradata, &video_param, &audio_param);

    if ((!has_video) && (!has_audio)) {
        LOGM_NOTICE("not receive sdp yet, try read again\n");
        ret1 = gfNet_Recv(mRTSPSocket, (TU8 *)mRTSPBuffer + ret, DMAX_RTSP_STRING_LEN - ret, 0);
        if (DUnlikely(!ret)) {
            LOGM_ERROR("peer close? retry\n");
            gfNetCloseSocket(mRTSPSocket);
            mRTSPSocket = DInvalidSocketHandler;
            if (retry_count > 0) {
                retry_count --;
                goto retry_connect_tcp;
            } else {
                LOGM_ERROR("max retry count reaches, return error\n");
                return EECode_Error;
            }
        } else if (DUnlikely(0 > ret)) {
            LOGM_ERROR("socket error, ret %d\n", ret);
            return EECode_Error;
        } else {
            LOGM_INFO("[rtsp flow]: DESCRIBE's responce from server(part 2):   %s, length %d\n", mRTSPBuffer, ret + ret1);
        }
        __parseSDP(mRTSPBuffer, ret + ret1, has_video, has_audio, has_video_extradata, has_audio_extradata, &video_param, &audio_param);
    }

    if (has_video_extradata) {
        if (DLikely(video_param.p_extra_data_sps_base64 && video_param.extra_data_sps_base64_size && video_param.p_extra_data_pps_base64 && video_param.extra_data_pps_base64_size)) {
            video_extra_data_size = video_param.extra_data_sps_base64_size + video_param.extra_data_pps_base64_size + 64;
            if (mpVideoExtraDataFromSDP) {
                if (mVideoExtraDataFromSDPBufferSize < video_extra_data_size) {
                    DDBG_FREE(mpVideoExtraDataFromSDP, "DMve");
                    mpVideoExtraDataFromSDP = NULL;
                    mVideoExtraDataFromSDPBufferSize = 0;
                }
            }

            if (!mpVideoExtraDataFromSDP) {
                mpVideoExtraDataFromSDP = (TU8 *)DDBG_MALLOC(video_extra_data_size, "DMve");
                if (DLikely(mpVideoExtraDataFromSDP)) {
                    mVideoExtraDataFromSDPBufferSize = video_extra_data_size;
                } else {
                    LOGM_FATAL("DDBG_MALLOC(%ld) fail\n", video_extra_data_size);
                    return EECode_NoMemory;
                }
            }

            p_video_extra_data = mpVideoExtraDataFromSDP;
            mVideoExtraDataFromSDPSize = 0;

            p_video_extra_data[0] = 0x0;
            p_video_extra_data[1] = 0x0;
            p_video_extra_data[2] = 0x0;
            p_video_extra_data[3] = 0x01;
            //p_video_extra_data[4] = 0x67;
            p_video_extra_data += 4;
            mVideoExtraDataFromSDPSize += 4;

            TU8 *ptmp00 = NULL;
            //gfDecodingBase64(video_param.p_extra_data_sps_base64, p_video_extra_data, video_param.extra_data_sps_base64_size, video_extra_data_size);
            video_extra_data_size = (video_param.extra_data_sps_base64_size / 4) * 3;
            ptmp00 = video_param.p_extra_data_sps_base64 + video_param.extra_data_sps_base64_size - 1;
            gfDecodingBase64(p_video_extra_data, video_param.p_extra_data_sps_base64, video_extra_data_size);
            while ('=' == ptmp00[0]) {
                video_extra_data_size --;
                ptmp00 --;
                //LOGM_INFO("minor 1\n");
            }
            p_video_extra_data += video_extra_data_size;
            mVideoExtraDataFromSDPSize += video_extra_data_size;

            p_video_extra_data[0] = 0x0;
            p_video_extra_data[1] = 0x0;
            p_video_extra_data[2] = 0x0;
            p_video_extra_data[3] = 0x01;
            //p_video_extra_data[4] = 0x68;
            p_video_extra_data += 4;
            mVideoExtraDataFromSDPSize += 4;

            //gfDecodingBase64(video_param.p_extra_data_pps_base64, p_video_extra_data, video_param.extra_data_pps_base64_size, video_extra_data_size);
            video_extra_data_size = (video_param.extra_data_pps_base64_size / 4) * 3;
            ptmp00 = video_param.p_extra_data_pps_base64 + video_param.extra_data_pps_base64_size - 1;
            gfDecodingBase64(p_video_extra_data, video_param.p_extra_data_pps_base64, video_extra_data_size);
            while ('=' == ptmp00[0]) {
                video_extra_data_size --;
                ptmp00 --;
                //LOGM_INFO("minor 1\n");
            }
            mVideoExtraDataFromSDPSize += video_extra_data_size;
#if 0
            LOGM_INFO("mVideoExtraDataFromSDPSize %ld\n", mVideoExtraDataFromSDPSize);
            LOGM_INFO("%02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x\n", \
                      mpVideoExtraDataFromSDP[0], mpVideoExtraDataFromSDP[1], mpVideoExtraDataFromSDP[2], mpVideoExtraDataFromSDP[3], \
                      mpVideoExtraDataFromSDP[4], mpVideoExtraDataFromSDP[5], mpVideoExtraDataFromSDP[6], mpVideoExtraDataFromSDP[7], \
                      mpVideoExtraDataFromSDP[8], mpVideoExtraDataFromSDP[9], mpVideoExtraDataFromSDP[10], mpVideoExtraDataFromSDP[11], \
                      mpVideoExtraDataFromSDP[12], mpVideoExtraDataFromSDP[13], mpVideoExtraDataFromSDP[14], mpVideoExtraDataFromSDP[15]);
#endif

            //check video width.height
            if (0 == video_param.width && 0 == video_param.height) {
                EECode ret = EECode_OK;
                SCodecVideoCommon *p_video_parser = gfGetVideoCodecParser(mpVideoExtraDataFromSDP + 5, mVideoExtraDataFromSDPSize - 5, StreamFormat_H264, ret);
                if (!p_video_parser || EECode_OK != ret) {
                    LOGM_ERROR("gfGetVideoCodecParser failed, ret=%d\n", ret);
                } else {
                    video_param.width = p_video_parser->max_width;
                    video_param.height = p_video_parser->max_height;
                    LOGM_NOTICE("gfGetVideoCodecParser, got video width=%u, height=%u\n", video_param.width, video_param.height);
                }
            }

        } else {
            LOGM_WARN("no video extra data, %p, %ld\n", p_video_extra_data, video_extra_data_size);
            mVideoExtraDataFromSDPSize = 0;
        }
    } else {
        mVideoExtraDataFromSDPSize = 0;
    }

    if (video_param.width && video_param.height) {
        postVideoSizeMsg(video_param.width, video_param.height);
    }

    if (has_audio_extradata) {
        if (DLikely(audio_param.p_extra_data_base16 && audio_param.extra_data_base16_size)) {

            if (mpAudioExtraDataFromSDP) {
                if (mAudioExtraDataFromSDPBufferSize < (audio_param.extra_data_base16_size / 2)) {
                    DDBG_FREE(mpAudioExtraDataFromSDP, "DMve");
                    mpAudioExtraDataFromSDP = NULL;
                    mVideoExtraDataFromSDPBufferSize = 0;
                }
            }

            if (!mpAudioExtraDataFromSDP) {
                mpAudioExtraDataFromSDP = (TU8 *)DDBG_MALLOC(audio_param.extra_data_base16_size / 2, "DMve");
                if (DLikely(mpAudioExtraDataFromSDP)) {
                    mAudioExtraDataFromSDPBufferSize = (audio_param.extra_data_base16_size / 2);
                } else {
                    LOGM_FATAL("DDBG_MALLOC(%ld) fail\n", (audio_param.extra_data_base16_size / 2));
                    return EECode_NoMemory;
                }
            }

            mAudioExtraDataFromSDPSize = audio_param.extra_data_base16_size / 2;
            gfDecodingBase16(mpAudioExtraDataFromSDP, (const TU8 *)audio_param.p_extra_data_base16, audio_param.extra_data_base16_size);

            LOGM_NOTICE("mAudioExtraDataFromSDPSize %ld\n", mAudioExtraDataFromSDPSize);
            LOGM_ALWAYS("%02x %02x\n", mpAudioExtraDataFromSDP[0], mpAudioExtraDataFromSDP[1]);
        } else {
            LOGM_WARN("no audio extra data, %p, %ld\n", audio_param.p_extra_data_base16, audio_param.extra_data_base16_size);
            mAudioExtraDataFromSDPSize = 0;
        }
    } else {
        mAudioExtraDataFromSDPSize = 0;
    }

    //need parse responce here, fix me
    //LOGM_WARN("please implement parsing DESCRIBE's responce, todo...\n");

    if (has_video) {
        if (!video_param.width || !video_param.height) {
            video_param.width = 720;
            video_param.height = 480;
            LOGM_NOTICE("do not get video width height, guess default width %d, height %d\n", video_param.width, video_param.height);
        } else {
            LOGM_NOTICE("get video width %d, height %d\n", video_param.width, video_param.height);
        }

        if (!video_param.framerate) {
            video_param.framerate = 30;
            LOGM_NOTICE("do not get video framerate, guess default framerate %f\n", video_param.framerate);
        } else {
            LOGM_NOTICE("get video framerate %f\n", video_param.framerate);
        }
        mStreamCodecInfos.info[EDemuxerVideoOutputPinIndex].spec.video.pic_width = video_param.width;
        mStreamCodecInfos.info[EDemuxerVideoOutputPinIndex].spec.video.pic_height = video_param.height;
        mStreamCodecInfos.info[EDemuxerVideoOutputPinIndex].spec.video.framerate = video_param.framerate;
        if (DLikely(video_param.framerate)) {
            mStreamCodecInfos.info[EDemuxerVideoOutputPinIndex].spec.video.framerate_num = DDefaultVideoFramerateNum;
            mStreamCodecInfos.info[EDemuxerVideoOutputPinIndex].spec.video.framerate_den = DDefaultVideoFramerateNum / video_param.framerate;
        } else {
            mStreamCodecInfos.info[EDemuxerVideoOutputPinIndex].spec.video.framerate_num = DDefaultVideoFramerateNum;
            mStreamCodecInfos.info[EDemuxerVideoOutputPinIndex].spec.video.framerate_den = DDefaultVideoFramerateDen;
        }
        mStreamCodecInfos.info[EDemuxerVideoOutputPinIndex].stream_enabled = mbEnableVideo;
        mStreamCodecInfos.info[EDemuxerVideoOutputPinIndex].stream_presence = 1;
        mStreamCodecInfos.info[EDemuxerVideoOutputPinIndex].stream_type = StreamType_Video;
        DASSERT(StreamFormat_H264 == video_param.format);
        mStreamCodecInfos.info[EDemuxerVideoOutputPinIndex].stream_format = video_param.format;
        mStreamCodecInfos.info[EDemuxerVideoOutputPinIndex].payload_type = video_param.rtp_payload_type;

        mRTPContext[EDemuxerVideoOutputPinIndex].enabled = mbEnableVideo;
        mRTPContext[EDemuxerVideoOutputPinIndex].track_id = video_param.rtp_track_index;
        mRTPContext[EDemuxerVideoOutputPinIndex].format = StreamFormat_H264;
        mRTPContext[EDemuxerVideoOutputPinIndex].type = StreamType_Video;
    }

    if (has_audio) {
        mStreamCodecInfos.info[EDemuxerAudioOutputPinIndex].spec.audio.sample_rate = audio_param.sample_rate;
        mStreamCodecInfos.info[EDemuxerAudioOutputPinIndex].spec.audio.channel_number = audio_param.channel_number;
        mStreamCodecInfos.info[EDemuxerAudioOutputPinIndex].stream_enabled = mbEnableAudio;
        mStreamCodecInfos.info[EDemuxerAudioOutputPinIndex].stream_presence = 1;
        mStreamCodecInfos.info[EDemuxerAudioOutputPinIndex].stream_type = StreamType_Audio;
        mStreamCodecInfos.info[EDemuxerAudioOutputPinIndex].stream_format = audio_param.format;
        mStreamCodecInfos.info[EDemuxerAudioOutputPinIndex].payload_type = audio_param.rtp_payload_type;

        mRTPContext[EDemuxerAudioOutputPinIndex].enabled = mbEnableAudio;
        mRTPContext[EDemuxerAudioOutputPinIndex].track_id = audio_param.rtp_track_index;
        mRTPContext[EDemuxerAudioOutputPinIndex].format = audio_param.format;
        mRTPContext[EDemuxerAudioOutputPinIndex].type = StreamType_Audio;
    }

    LOGM_INFO("has_video %d, mRTPContext[EDemuxerVideoOutputPinIndex].enabled %d\n", has_video, mRTPContext[EDemuxerVideoOutputPinIndex].enabled);

    if (has_video && mRTPContext[EDemuxerVideoOutputPinIndex].enabled) {

        //send SETUP request
        memset(mRTSPBuffer, 0x0, DMAX_RTSP_STRING_LEN);
        snprintf(mRTSPBuffer, DMAX_RTSP_STRING_LEN - 1, _rtsp_setup_request_tcp_fmt, mpServerAddr, mServerRTSPPort, mpItemName, video_param.track_string, mRTSPSeq ++);
        LOGM_INFO("[rtsp flow]: before send SETUP request....\r\n    %s, length %lu\n", mRTSPBuffer, (TULong)strlen(mRTSPBuffer));
        ret = gfNet_Send(mRTSPSocket, (TU8 *)mRTSPBuffer, strlen(mRTSPBuffer), 0);

        if (DLikely(ret == ((TInt)strlen(mRTSPBuffer)))) {
            LOGM_INFO("[rtsp flow]: send SETUP request done, sent len %d, read responce from server\n", ret);
        } else {
            if (!ret) {
                LOGM_ERROR("peer close? retry\n");
                gfNetCloseSocket(mRTSPSocket);
                mRTSPSocket = DInvalidSocketHandler;
                if (retry_count > 0) {
                    retry_count --;
                    goto retry_connect_tcp;
                } else {
                    LOGM_ERROR("max retry count reaches, return error\n");
                    return EECode_Error;
                }
            } else {
                LOGM_ERROR("socket error, ret %d\n", ret);
                return EECode_Error;
            }
        }

        ret = gfNet_Recv(mRTSPSocket, (TU8 *)mRTSPBuffer, DMAX_RTSP_STRING_LEN, 0);
        if (DUnlikely(!ret)) {
            LOGM_ERROR("peer close? retry\n");
            gfNetCloseSocket(mRTSPSocket);
            mRTSPSocket = DInvalidSocketHandler;
            if (retry_count > 0) {
                retry_count --;
                goto retry_connect_tcp;
            } else {
                LOGM_ERROR("max retry count reaches, return error\n");
                return EECode_Error;
            }
        } else if (DUnlikely(0 > ret)) {
            LOGM_ERROR("socket error, ret %d\n", ret);
            return EECode_Error;
        } else {
            LOGM_INFO("[rtsp flow]: SETUP's responce from server:   %s, length %d\n", mRTSPBuffer, ret);
        }

        //find sessionID
        if ((ptmp = _find_string((char *)mRTSPBuffer, DMAX_RTSP_STRING_LEN, "Session: ", strlen("Session: ")))) {
            sscanf(ptmp, "Session: %llx", &mSessionID);
            LOGM_INFO("[rtsp flow]: in SETUP's responce, get session ID %llx.\n", mSessionID);
        } else {
            LOGM_ERROR("cannot find session ID.\n");
            return EECode_Error;
        }

        setup_video = 1;
    }

    LOGM_INFO("has_audio %d, mRTPContext[EDemuxerAudioOutputPinIndex].enabled %d\n", has_audio, mRTPContext[EDemuxerAudioOutputPinIndex].enabled);

    if (has_audio && mRTPContext[EDemuxerAudioOutputPinIndex].enabled) {

        //send SETUP request
        memset(mRTSPBuffer, 0x0, DMAX_RTSP_STRING_LEN);
        snprintf(mRTSPBuffer, DMAX_RTSP_STRING_LEN - 1, _rtsp_setup_request_tcp_with_sessionid_fmt, mpServerAddr, mServerRTSPPort, mpItemName, audio_param.track_string, mRTSPSeq ++, mSessionID);
        LOGM_INFO("[rtsp flow]: before send SETUP request....\r\n    %s, length %lu\n", mRTSPBuffer, (TULong)strlen(mRTSPBuffer));
        ret = gfNet_Send(mRTSPSocket, (TU8 *)mRTSPBuffer, strlen(mRTSPBuffer), 0);

        if (DLikely(ret == ((TInt)strlen(mRTSPBuffer)))) {
            LOGM_INFO("[rtsp flow]: send SETUP request done, sent len %d, read responce from server\n", ret);
        } else {
            if (!ret) {
                LOGM_ERROR("peer close? retry\n");
                gfNetCloseSocket(mRTSPSocket);
                mRTSPSocket = DInvalidSocketHandler;
                if (retry_count > 0) {
                    retry_count --;
                    goto retry_connect_tcp;
                } else {
                    LOGM_ERROR("max retry count reaches, return error\n");
                    return EECode_Error;
                }
            } else {
                LOGM_ERROR("socket error, ret %d\n", ret);
                return EECode_Error;
            }
        }

        ret = gfNet_Recv(mRTSPSocket, (TU8 *)mRTSPBuffer, DMAX_RTSP_STRING_LEN, 0);
        if (DUnlikely(!ret)) {
            LOGM_ERROR("peer close? retry\n");
            gfNetCloseSocket(mRTSPSocket);
            mRTSPSocket = DInvalidSocketHandler;
            if (retry_count > 0) {
                retry_count --;
                goto retry_connect_tcp;
            } else {
                LOGM_ERROR("max retry count reaches, return error\n");
                return EECode_Error;
            }
        } else if (DUnlikely(0 > ret)) {
            LOGM_ERROR("socket error, ret %d\n", ret);
            return EECode_Error;
        } else {
            LOGM_INFO("[rtsp flow]: SETUP's responce from server:   %s, length %d\n", mRTSPBuffer, ret);
        }

        setup_audio = 1;
    }

    if (setup_video || setup_audio) {
        //send PLAY request
        memset(mRTSPBuffer, 0x0, DMAX_RTSP_STRING_LEN);
        snprintf(mRTSPBuffer, DMAX_RTSP_STRING_LEN - 1, _rtsp_play_request_fmt, mpServerAddr, mServerRTSPPort, mpItemName, mRTSPSeq ++, mSessionID);
        LOGM_INFO("[rtsp flow]: before send PLAY request....\r\n    %s, length %lu\n", mRTSPBuffer, (TULong)strlen(mRTSPBuffer));
        ret = gfNet_Send(mRTSPSocket, (TU8 *)mRTSPBuffer, strlen(mRTSPBuffer), 0);
        if (DLikely(ret == ((TInt)strlen(mRTSPBuffer)))) {
            LOGM_INFO("[rtsp flow]: send PLAY request done, sent len %d, read responce from server\n", ret);
        } else {
            if (!ret) {
                LOGM_ERROR("peer close? retry\n");
                gfNetCloseSocket(mRTSPSocket);
                mRTSPSocket = DInvalidSocketHandler;
                if (retry_count > 0) {
                    retry_count --;
                    goto retry_connect_tcp;
                } else {
                    LOGM_ERROR("max retry count reaches, return error\n");
                    return EECode_Error;
                }
            } else {
                LOGM_ERROR("socket error, ret %d\n", ret);
                return EECode_Error;
            }
        }

        ret = gfNet_Recv(mRTSPSocket, (TU8 *)mRTSPBuffer, DMAX_RTSP_STRING_LEN, 0);
        if (DUnlikely(!ret)) {
            LOGM_ERROR("peer close? retry\n");
            gfNetCloseSocket(mRTSPSocket);
            mRTSPSocket = DInvalidSocketHandler;
            if (retry_count > 0) {
                retry_count --;
                goto retry_connect_tcp;
            } else {
                LOGM_ERROR("max retry count reaches, return error\n");
                return EECode_Error;
            }
        } else if (DUnlikely(0 > ret)) {
            LOGM_ERROR("socket error, ret %d\n", ret);
            return EECode_Error;
        } else {
            LOGM_INFO("[rtsp flow]: PLAY's responce from server:   %s, length %d\n", mRTSPBuffer, ret);
        }

        mbRTSPServerConnected = 1;

        mRTPContext[EDemuxerVideoOutputPinIndex].index = mIndex;
        mRTPContext[EDemuxerAudioOutputPinIndex].index = mIndex;

        mRTPContext[EDemuxerVideoOutputPinIndex].rtsp_socket = mRTSPSocket;
        mRTPContext[EDemuxerAudioOutputPinIndex].rtsp_socket = mRTSPSocket;

        mRTPContext[EDemuxerVideoOutputPinIndex].initialized = 1;
        mRTPContext[EDemuxerAudioOutputPinIndex].initialized = 1;
    }

    //LOGM_NOTICE("connectToRTSPServerTCP() end\n");

    return EECode_OK;
}

EECode CScheduledRTSPDemuxer::SetupOutput(COutputPin *p_output_pins[], CBufferPool *p_bufferpools[], IMemPool *p_mempools[], IMsgSink *p_msg_sink)
{
    //debug assert
    DASSERT(!mRTPContext[EDemuxerVideoOutputPinIndex].bufferpool);
    DASSERT(!mRTPContext[EDemuxerAudioOutputPinIndex].bufferpool);
    DASSERT(!mRTPContext[EDemuxerVideoOutputPinIndex].outpin);
    DASSERT(!mRTPContext[EDemuxerAudioOutputPinIndex].outpin);
    DASSERT(p_msg_sink);

    DASSERT(!mbOutputMsgSinkSet);

    mRTPContext[EDemuxerVideoOutputPinIndex].outpin = p_output_pins[EDemuxerVideoOutputPinIndex];
    mRTPContext[EDemuxerAudioOutputPinIndex].outpin = p_output_pins[EDemuxerAudioOutputPinIndex];
    mRTPContext[EDemuxerVideoOutputPinIndex].bufferpool = p_bufferpools[EDemuxerVideoOutputPinIndex];
    mRTPContext[EDemuxerAudioOutputPinIndex].bufferpool = p_bufferpools[EDemuxerAudioOutputPinIndex];
    mRTPContext[EDemuxerVideoOutputPinIndex].mempool = p_mempools[EDemuxerVideoOutputPinIndex];
    mRTPContext[EDemuxerAudioOutputPinIndex].mempool = p_mempools[EDemuxerAudioOutputPinIndex];

    if (p_msg_sink) {
        mpMsgSink = p_msg_sink;
    }

    if (mRTPContext[EDemuxerVideoOutputPinIndex].outpin && mRTPContext[EDemuxerVideoOutputPinIndex].bufferpool) {
        mRTPContext[EDemuxerVideoOutputPinIndex].bufferpool->SetReleaseBufferCallBack(__release_ring_buffer);
        mbEnableVideo = 1;
    }

    if (mRTPContext[EDemuxerAudioOutputPinIndex].outpin && mRTPContext[EDemuxerAudioOutputPinIndex].bufferpool) {
        mRTPContext[EDemuxerAudioOutputPinIndex].bufferpool->SetReleaseBufferCallBack(__release_ring_buffer);
        mbEnableAudio = 1;
    }

    if (!mbEnableVideo && !mbEnableAudio) {
        LOGM_FATAL("no output?\n");
        return EECode_BadParam;
    }

    mbOutputMsgSinkSet = 1;

    return EECode_OK;
}

EECode CScheduledRTSPDemuxer::SetupContext(TChar *url, void *p_agent, TU8 priority, TU32 request_receive_buffer_size, TU32 request_send_buffer_size)
{
    AUTO_LOCK(mpMutex);

    DASSERT(mbOutputMsgSinkSet);

    if (url) {
        DASSERT(!mbRTSPServerConnected);
        DASSERT(mbEnableVideo || mbEnableAudio);

        if (priority) {
            mPriority = priority;
        }

        if (DLikely(mpPersistMediaConfig)) {
            mbUseTCPMode = mpPersistMediaConfig->rtsp_client_config.use_tcp_mode_first;
            LOGM_NOTICE("mbUseTCPMode %d\n", mbUseTCPMode);
        }

        if (request_receive_buffer_size) {
            mbPreSetReceiveBufferSize = 1;
            mReceiveBufferSize = request_receive_buffer_size;
        }

        if (request_send_buffer_size) {
            mbPreSetSendBufferSize = 1;
            mSendBufferSize = request_send_buffer_size;
        }

        if ((!mpSourceUrl) || (strlen(url) > mSourceUrlLength)) {
            mSourceUrlLength = strlen(url);
            if (mpSourceUrl) {
                DDBG_FREE(mpSourceUrl, "DMSU");
                mpSourceUrl = NULL;
            }
            mpSourceUrl = (TChar *)DDBG_MALLOC(mSourceUrlLength + 4, "DMSU");
            if (!mpSourceUrl) {
                LOGM_FATAL("Not enough memory\n");
                return EECode_NoMemory;
            }
        }
        memset(mpSourceUrl, 0x0, mSourceUrlLength);
        strcpy(mpSourceUrl, url);

        if (!mbUseTCPMode) {
            return connectToRTSPServer(url);
        }

        return connectToRTSPServerTCP(url);
    } else {
        LOGM_FATAL("NULL input in CRTSPDemuxer::SetupContext\n");
        return EECode_BadParam;
    }
}

EECode CScheduledRTSPDemuxer::DestroyContext()
{
    AUTO_LOCK(mpMutex);

    clearContext();

    return EECode_OK;
}

EECode CScheduledRTSPDemuxer::ReconnectServer()
{
    EECode err = EECode_OK;
    AUTO_LOCK(mpMutex);

    mReconnectCount ++;
    if (mReconnectCount >= mMaxReconnectCount) {
        mReconnectCount = 0;
    }

    mClientRTPPortRangeBegin = mClientRTPPortRangeBeginBase + mReconnectCount * 4;

    LOGM_NOTICE("ReconnectServer? before stopRTPDataReceiver(), mReconnectCount %d, mClientRTPPortRangeBegin %d\n", mReconnectCount, mClientRTPPortRangeBegin);

    if (!mbSendTeardown) {
        mbSendTeardown = 1;
        sendTeardown();
    }

    if (!mbUseTCPMode) {
        err = stopRTPDataReceiver();
    } else {
        err = stopRTPDataReceiverTCP();
    }
    DASSERT_OK(err);

    if (mbRTSPServerConnected) {
        mbRTSPServerConnected = 0;
    }

    closeAllSockets();

    LOGM_NOTICE("before connectToRTSPServer()\n");

    if (!mbUseTCPMode) {
        err = connectToRTSPServer(mpSourceUrl);
    } else {
        err = connectToRTSPServerTCP(mpSourceUrl);
    }

    if (EECode_OK != err) {
        LOGM_ERROR("connectToRTSPServer(%s) fail, return %s\n", mpSourceUrl, gfGetErrorCodeString(err));
        return err;
    }

    if (!mbUseTCPMode) {
        reinitializeRTPDataReceivers();
    } else {
        reinitializeRTPDataReceiversTCP();
    }

    if (!mbUseTCPMode) {
        err = startRTPDataReceiver();
    } else {
        err = startRTPDataReceiverTCP();
    }

    mbSendTeardown = 0;

    return EECode_OK;
}

EECode CScheduledRTSPDemuxer::Start()
{
    AUTO_LOCK(mpMutex);
    DASSERT(mbOutputMsgSinkSet);
    DASSERT(mbRTSPServerConnected);

    if (mbOutputMsgSinkSet && mbRTSPServerConnected) {
        if (!mbContextRegistered) {
            if (!mbUseTCPMode) {
                startRTPDataReceiver();
            } else {
                startRTPDataReceiverTCP();
            }
        }
    } else {
        LOGM_ERROR("BAD state in CRTSPDemuxer::Start(), mbOutputMsgSinkSet %d, mbRTSPServerConnected %d\n", mbOutputMsgSinkSet, mbRTSPServerConnected);
        return EECode_BadState;
    }

    return EECode_BadState;
}

EECode CScheduledRTSPDemuxer::Stop()
{
    EECode err = EECode_OK;
    AUTO_LOCK(mpMutex);

    if (mbContextRegistered) {
        if (!mbSendTeardown) {
            mbSendTeardown = 1;
            sendTeardown();
        }
        if (!mbUseTCPMode) {
            err = stopRTPDataReceiver();
        } else {
            err = stopRTPDataReceiverTCP();
        }
    }

    return err;
}

EECode CScheduledRTSPDemuxer::Suspend()
{
    LOGM_WARN("Suspend() is not implemented yet\n");
    return EECode_OK;
}

EECode CScheduledRTSPDemuxer::Pause()
{
    LOGM_WARN("Pause() is not implemented yet\n");
    return EECode_OK;
}

EECode CScheduledRTSPDemuxer::Resume()
{
    LOGM_WARN("Resume() is not implemented yet\n");
    return EECode_OK;
}

EECode CScheduledRTSPDemuxer::Purge()
{
    LOGM_WARN("Purge() is not implemented yet\n");
    return EECode_OK;
}

EECode CScheduledRTSPDemuxer::Flush()
{
    LOGM_NOTICE("not implemented in this module.\n");
    return EECode_NoImplement;
}

EECode CScheduledRTSPDemuxer::ResumeFlush()
{
    LOGM_NOTICE("not implemented in this module.\n");
    return EECode_NoImplement;
}

EECode CScheduledRTSPDemuxer::SetPbRule(TU8 direction, TU8 feeding_rule, TU16 speed)
{
    LOGM_NOTICE("not implemented in this module.\n");
    return EECode_NoImplement;
}

EECode CScheduledRTSPDemuxer::SetPbLoopMode(TU32 *p_loop_mode)
{
    LOGM_NOTICE("not implemented in this module.\n");
    return EECode_NoImplement;
}

EECode CScheduledRTSPDemuxer::EnableVideo(TU32 enable)
{
    LOGM_ERROR("not implemented in this module.\n");
    return EECode_NoImplement;
}

EECode CScheduledRTSPDemuxer::EnableAudio(TU32 enable)
{
    LOGM_ERROR("not implemented in this module.\n");
    return EECode_NoImplement;
}

void CScheduledRTSPDemuxer::Delete()
{
    LOGM_INFO("CScheduledRTSPDemuxer::Delete().\n");

    clearContext();

    closeAllSockets();

    inherited::Delete();
}

EECode CScheduledRTSPDemuxer::clearContext()
{
    if (mbRTSPServerConnected) {
        LOGM_INFO("tear down request\n");
        if (!mbSendTeardown) {
            mbSendTeardown = 1;
            sendTeardown();
        }
        LOGM_INFO("tear down request send finished\n");
        mbRTSPServerConnected = 0;
    }

    if (!mbUseTCPMode) {
        if (mbContextRegistered) {
            stopRTPDataReceiver();
        }

        destroyRTPDataReceivers();
    } else {
        if (mbContextRegistered) {
            stopRTPDataReceiverTCP();
        }

        destroyRTPDataReceiversTCP();
    }



    return EECode_OK;
}

EECode CScheduledRTSPDemuxer::GetInfo(SStreamCodecInfos *&pinfos)
{
    pinfos = &mStreamCodecInfos;
    return EECode_OK;
}

EECode CScheduledRTSPDemuxer::UpdateContext(SContextInfo *pContext)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CScheduledRTSPDemuxer::GetExtraData(SStreamingSessionContent *pContent)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CScheduledRTSPDemuxer::NavigationSeek(SContextInfo *info)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CScheduledRTSPDemuxer::ResumeChannel()
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

void CScheduledRTSPDemuxer::sendTeardown()
{
    TInt ret = 0;
    memset(mRTSPBuffer, 0x0, DMAX_RTSP_STRING_LEN);
    snprintf(mRTSPBuffer, DMAX_RTSP_STRING_LEN - 1, _rtsp_teardown_request_fmt, mpServerAddr, mServerRTSPPort, mpItemName, mRTSPSeq ++, mSessionID);
    LOGM_INFO("[rtsp flow]: before send TEARDWON request....\r\n    %s, length %lu\n", mRTSPBuffer, (TULong)strlen(mRTSPBuffer));

    ret = gfNet_Send_timeout(mRTSPSocket, (TU8 *)mRTSPBuffer, strlen(mRTSPBuffer), 0, 2);
    LOGM_INFO("[rtsp flow]: send TEARDWON request done, sent len %d, read responce from server\n", ret);
    //DASSERT(ret == ((TInt)strlen(mRTSPBuffer)));

    //ret = gfNet_Recv_timeout(mRTSPSocket, (TU8*)mRTSPBuffer, DMAX_RTSP_STRING_LEN, 0, 2);
    //LOGM_INFO("[rtsp flow]: TEARDWON's responce from server:   %s, length %d\n", mRTSPBuffer, ret);
    //need parse responce here, fix me
    //LOGM_WARN("please implement parsing TEARDWON's responce, todo...\n");
}

void CScheduledRTSPDemuxer::postVideoSizeMsg(TU32 width, TU32 height)
{
    SMSG msg;
    memset(&msg, 0x0, sizeof(msg));
    msg.code = EMSGType_VideoSize;
    msg.p1 = width;
    msg.p2 = height;
    if (mpMsgSink) {
        mpMsgSink->MsgProc(msg);
    }
}

EECode CScheduledRTSPDemuxer::Seek(TTime &ms, ENavigationPosition position)
{
    LOGM_INFO("RTSP demuxer not support seek now\n");
    return EECode_OK;
}

EECode CScheduledRTSPDemuxer::QueryContentInfo(const SStreamCodecInfos *&pinfos) const
{
    pinfos = &mStreamCodecInfos;
    return EECode_OK;
}

EECode CScheduledRTSPDemuxer::SetVideoPostProcessingCallback(void *callback_context, void *callback)
{
    mpVideoPostProcessingCallback = callback;
    mpVideoPostProcessingCallbackContext = callback_context;

    return EECode_OK;
}

EECode CScheduledRTSPDemuxer::SetAudioPostProcessingCallback(void *callback_context, void *callback)
{
    mpAudioPostProcessingCallback = callback;
    mpAudioPostProcessingCallbackContext = callback_context;

    return EECode_OK;
}

void CScheduledRTSPDemuxer::PrintStatus()
{
    if (!mbUseTCPMode) {
        TU32 i = 0;

        for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
            if (mpReceiver[i]) {
                //mpReceiver[i]->PrintStatus();
            }
        }
    } else {
        //mpReceiverTCP->PrintStatus();
    }
}

#endif

