/*******************************************************************************
 * am_rtsp_client_session.h
 *
 * History:
 *   2014-12-29 - [Shiming Dong] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#ifndef AM_RTSP_CLIENT_SESSION_H_
#define AM_RTSP_CLIENT_SESSION_H_

#include "am_rtsp_requests.h"

#define  CLIENT_TIMEOUT_THRESHOLD  30

struct RtspRequest;
struct RtspAuthHeader;
struct RtspTransHeader;

class AMEvent;
class AMThread;
class AMRtspServer;
class AMRtspAuthenticator;
class AMRtspClientSession
{
  friend class AMRtspServer;
  enum PARSE_STATE {
    PARSE_INCOMPLETE,
    PARSE_COMPLETE,
    PARSE_RTCP
  };

  enum CLIENT_SESSION_STATE {
    CLIENT_SESSION_INIT,
    CLIENT_SESSION_THREAD_RUN,
    CLIENT_SESSION_OK,
    CLIENT_SESSION_FAILED,
    CLIENT_SESSION_STOPPED
  };

  enum {
    PORT_RTP  = 0,
    PORT_RTCP = 1,
    PORT_NUM  = 2
  };

  struct MediaInfo
  {
      bool        is_alive;
      AMEvent    *event_msg_seqntp;
      AMEvent    *event_ssrc;
      AMEvent    *event_kill;
      std::string media;
      std::string seq_ntp;
      std::string sdp;
      uint32_t    ssrc;
      uint16_t    udp_port[PORT_NUM];
      uint16_t    stream_id;
      MediaInfo();
      ~MediaInfo();
  };

  public:
    AMRtspClientSession(AMRtspServer *server, uint16_t udp_rtp_port);
    virtual ~AMRtspClientSession();

  public:
    bool init_client_session(int32_t server_tcp_sock);
    void abort_client();
    bool kill_client();
    void set_client_name(const char *name);

  private:
    static void static_client_thread(void *data);
    void client_thread();
    void close_all_socket();
    bool parse_transport_header(RtspTransHeader& header,
                                char*            req_str,
                                uint32_t         req_len);
    bool parse_authentication_header(RtspAuthHeader& header,
                                     char*           req_str,
                                     uint32_t        req_len);
    void handle_rtcp_packet(char *rtcp);
    bool parse_client_request(RtspRequest     &request,
                              PARSE_STATE     &state,
                              char*            req_str,
                              uint32_t         req_len);
    bool handle_client_request(RtspRequest& request);
    bool cmd_options (RtspRequest& req, char* buf, uint32_t size);
    bool cmd_describe(RtspRequest& req, char* buf, uint32_t size);
    bool cmd_setup(RtspRequest& req, char* buf, uint32_t size);
    bool cmd_teardown(RtspRequest& req, char* buf, uint32_t size);
    bool cmd_play(RtspRequest& req, char* buf, uint32_t size);
    bool cmd_get_parameter(RtspRequest& req, char* buf, uint32_t size);
    bool cmd_set_patameter(RtspRequest& req, char* buf, uint32_t size);
    bool cmd_not_supported(RtspRequest& req, char* buf, uint32_t size);
    bool cmd_bad_requests(RtspRequest& req, char* buf, uint32_t size);
    bool authenticate_ok(RtspRequest& req, char* buf, uint32_t size);
    void not_found(RtspRequest& req, char* buf, uint32_t size);

    char* get_date_string(char* buf, uint32_t size);
    void  teardown();
    std::string get_sdp();
    void sdp_ack();
    void seq_ntp_ack(AM_RTP_MEDIA_TYPE type);
    void ssrc_ack(AM_RTP_MEDIA_TYPE type);
    void kill_ack(AM_RTP_MEDIA_TYPE type);
    void set_media_sdp(AM_RTP_MEDIA_TYPE type, const char *sdp);
    void set_media_seq_ntp(AM_RTP_MEDIA_TYPE type, const char *seq_ntp);
    void set_media_ssrc(AM_RTP_MEDIA_TYPE type, uint32_t ssrc);
    uint16_t media_stream_id(AM_RTP_MEDIA_TYPE type);
    bool media_has_sdp();
    bool media_is_empty();
    bool is_alive();

  private:
    AMRtspAuthenticator  *m_authenticator;
    AMRtspServer         *m_rtsp_server;
    AMThread             *m_client_thread;
    AMEvent              *m_event_msg_sdp;
    CLIENT_SESSION_STATE  m_session_state;
    int32_t               m_tcp_sock;
    uint32_t              m_client_session_id;
    uint32_t              m_rtcp_rr_ssrc;
    uint64_t              m_rtcp_rr_interval;
    uint32_t              m_identify;
    uint16_t              m_client_id;
    int32_t               m_dynamic_timeout_sec;
    int32_t               m_client_ctrl_sock[2];
#define CLIENT_CTRL_READ  m_client_ctrl_sock[0]
#define CLIENT_CTRL_WRITE m_client_ctrl_sock[1]

    sockaddr_in           m_client_addr;
    timeval               m_rtcp_rr_last_recv_time;
    std::string           m_client_name;
    MediaInfo             m_media_info[AM_RTP_MEDIA_NUM];

    RtspTransHeader       m_rtsp_trans_hdr;
    RtspAuthHeader        m_rtsp_auth_hdr;
};

#endif /* AM_RTSP_CLIENT_SESSION_H_ */
