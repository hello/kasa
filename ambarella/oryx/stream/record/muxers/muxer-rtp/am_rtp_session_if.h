/*******************************************************************************
 * am_rtp_session.h
 *
 * History:
 *   2015-1-4 - [ypchang] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_SESSION_IF_H_
#define ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_SESSION_IF_H_

#define NTP_OFFSET    2208988800ULL
#define NTP_OFFSET_US (NTP_OFFSET * 1000000ULL)

#include "am_rtp_msg.h"
#include "rtp.h"

#include <queue>
#include <vector>
#include <atomic>

enum AM_NET_PACKET_HDR_SIZE
{
  HEADER_SIZE_TCP = 40,
  HEADER_SIZE_UDP = 28,
};

enum AM_SESSION_TYPE
{
  AM_SESSION_AAC,
  AM_SESSION_OPUS,
  AM_SESSION_G726,
  AM_SESSION_G711,
  AM_SESSION_SPEEX,
  AM_SESSION_H264,
  AM_SESSION_H265,
  AM_SESSION_MJPEG,
};

enum AM_SESSION_STATE
{
  AM_SESSION_OK,
  AM_SESSION_ERROR,
  AM_SESSION_IO_SLOW,
};

struct timespec;
struct AMRtpData;
struct AM_VIDEO_INFO;
struct AM_AUDIO_INFO;
typedef std::queue<AMRtpData*> AMRtpDataQ;

class AMPacket;
class AMRtpClient;
class AMIRtpSession
{
  public:
    typedef std::vector<AMRtpClient*> AMRtpClientList;

  public:
    virtual std::string& get_session_name()                               = 0;
    virtual std::string& get_sdp(std::string& host_ip_addr, uint16_t port,
                                 AM_IP_DOMAIN domain)                     = 0;
    virtual std::string get_param_sdp()                                   = 0;
    virtual std::string get_payload_type()                                = 0;
    virtual bool add_client(AMRtpClient *client)                          = 0;
    virtual bool del_client(uint32_t ssrc)                                = 0;
    virtual void set_client_enable(uint32_t ssrc, bool enabled)           = 0;
    virtual std::string get_client_seq_ntp(uint32_t ssrc)                 = 0;
    virtual uint32_t get_session_clock()                                  = 0;
    virtual AM_SESSION_STATE process_data(AMPacket &packet,
                                          uint32_t max_packet_size)       = 0;
    virtual AM_SESSION_TYPE type()                                        = 0;
    virtual std::string type_string()                                     = 0;
    virtual AMRtpClient* find_client_by_ssrc(uint32_t ssrc)               = 0;
    virtual void set_config(void *config)                                 = 0;
    virtual ~AMIRtpSession(){}
};

#ifdef __cplusplus
extern "C" {
#endif
AMIRtpSession* create_h264_session(const char *name,
                                   uint32_t queue_size,
                                   void *vinfo);
AMIRtpSession* create_h265_session(const char *name,
                                   uint32_t queue_size,
                                   void *vinfo);
AMIRtpSession* create_mjpeg_session(const char *name,
                                    uint32_t queue_size,
                                    void *vinfo);
AMIRtpSession* create_aac_session(const char *name,
                                  uint32_t queue_size,
                                  void *ainfo);
AMIRtpSession* create_opus_session(const char *name,
                                   uint32_t queue_size,
                                   void *ainfo);
AMIRtpSession* create_speex_session(const char *name,
                                    uint32_t queue_size,
                                    void *ainfo);
AMIRtpSession* create_g726_session(const char *name,
                                   uint32_t queue_size,
                                   void *ainfo);
AMIRtpSession* create_g711_session(const char *name,
                                   uint32_t queue_size,
                                   void *ainfo);
#ifdef __cplusplus
}
#endif
#endif /* ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_SESSION_IF_H_ */
