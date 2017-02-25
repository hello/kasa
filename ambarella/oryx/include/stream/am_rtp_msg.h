/*******************************************************************************
 * am_rtp_msg.h
 *
 * History:
 *   2015-1-5 - [ypchang] created file
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
#ifndef ORYX_INCLUDE_STREAM_AM_RTP_MSG_H_
#define ORYX_INCLUDE_STREAM_AM_RTP_MSG_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define RTP_CONTROL_SOCKET ((const char*)"/run/oryx/rtp-server.socket")

#define MAX_SDP_LEN     1024
#define MAX_MEDIA_LEN   64

enum AM_RTP_CLIENT_MSG_TYPE
{
  AM_RTP_CLIENT_MSG_ACK,
  AM_RTP_CLIENT_MSG_SDP,
  AM_RTP_CLIENT_MSG_KILL,
  AM_RTP_CLIENT_MSG_INFO,
  AM_RTP_CLIENT_MSG_START,
  AM_RTP_CLIENT_MSG_STOP,
  AM_RTP_CLIENT_MSG_SSRC,
  AM_RTP_CLIENT_MSG_PROTO,
  AM_RTP_CLIENT_MSG_CLOSE,
  AM_RTP_CLIENT_MSG_SUPPORT,
  AM_RTP_CLIENT_MSG_SDPS,
};

enum AM_RTP_CLIENT_PROTO
{
  AM_RTP_CLIENT_PROTO_UNKNOWN = -1,
  AM_RTP_CLIENT_PROTO_RTSP    = 0,
  AM_RTP_CLIENT_PROTO_SIP     = 1,
  AM_RTP_CLIENT_PROTO_NUM,
};

struct AMRtpClientMsgBase
{
    uint32_t               length;
    uint32_t               session_id;
    AM_RTP_CLIENT_MSG_TYPE type;
};

struct AMRtpClientMsg
{
    uint32_t               length;
    uint32_t               session_id;
    AM_RTP_CLIENT_MSG_TYPE type;
    uint8_t                msg[];
};

enum AM_RTP_MEDIA_TYPE
{
  AM_RTP_MEDIA_VIDEO  = 0,
  AM_RTP_MEDIA_AUDIO  = 1,
  AM_RTP_MEDIA_NUM,
  AM_RTP_MEDIA_HOSTIP = AM_RTP_MEDIA_NUM,
  AM_RTP_MEDIA_SIZE,
};

struct AMRtpClientMsgACK
{
    uint32_t               length;
    uint32_t               session_id;
    AM_RTP_CLIENT_MSG_TYPE type;
    uint32_t               ssrc;
    char                   media[MAX_MEDIA_LEN];
    AMRtpClientMsgACK(uint32_t _session_id, uint32_t _ssrc) :
      length(sizeof(AMRtpClientMsgACK)),
      session_id(_session_id),
      type(AM_RTP_CLIENT_MSG_ACK),
      ssrc(_ssrc)
    {
      memset(media, 0, MAX_MEDIA_LEN);
    }
};

struct AMRtpClientMsgSDP
{
    uint32_t               length;
    uint32_t               session_id;
    AM_RTP_CLIENT_MSG_TYPE type;
    uint16_t               port[AM_RTP_MEDIA_NUM];
    char                   media[AM_RTP_MEDIA_SIZE][MAX_SDP_LEN];
    AMRtpClientMsgSDP() :
      length(sizeof(AMRtpClientMsgSDP)),
      session_id(0),
      type(AM_RTP_CLIENT_MSG_SDP)
    {
      memset(media, 0, sizeof(media));
      memset(port, 0, sizeof(port));
    }
};

struct AMRtpClientMsgKill
{
    uint32_t               length;
    uint32_t               session_id;
    AM_RTP_CLIENT_MSG_TYPE type;
    uint32_t               ssrc;
    int                    fd_from;
    bool                   active_kill;
    AMRtpClientMsgKill(uint32_t client_ssrc, uint32_t session, int fd) :
      length(sizeof(AMRtpClientMsgKill)),
      session_id(session),
      type(AM_RTP_CLIENT_MSG_KILL),
      ssrc(client_ssrc),
      fd_from(fd),
      active_kill(false)
    {}
};

enum AM_IP_DOMAIN
{
  AM_IPV4,
  AM_IPV6,
  AM_UNKNOWN,
};

enum AM_RTP_MODE
{
  AM_RTP_TCP,
  AM_RTP_UDP,
  AM_RAW_UDP,
};

struct AMRtpClientInfo
{
    AM_RTP_MODE  client_rtp_mode;     /* Client requested RTP transfer mode   */
    AM_IP_DOMAIN client_ip_domain;    /* IP address domain                    */
    uint16_t     tcp_channel_rtp;     /* Client TCP transfer channel for RTP  */
    uint16_t     tcp_channel_rtcp;    /* Client TCP transfer channel for RTCP */
    uint16_t     port_srv_rtp;        /* server side port assigned to client  */
    uint16_t     port_srv_rtcp;       /* server side port assigned to client  */
    bool         send_tcp_fd;
    char         media[MAX_MEDIA_LEN];
    struct {
        /* client UDP address info, to where server send data */
        /* This info is acquired through parsing RTSP Transport Header string */
        union {
            struct sockaddr_in  ipv4;
            struct sockaddr_in6 ipv6;
        }rtp;
        union {
            struct sockaddr_in  ipv4;
            struct sockaddr_in6 ipv6;
        }rtcp;
    }udp;
    union {
        /* client TCP address info, to where server send data */
        /* This info is acquired through "accept",
         * when client connects to RTSP server */
        struct sockaddr_in  ipv4;
        struct sockaddr_in6 ipv6;
    }tcp;
    AMRtpClientInfo() :
      client_rtp_mode(AM_RTP_UDP),
      client_ip_domain(AM_UNKNOWN),
      tcp_channel_rtp(0),
      tcp_channel_rtcp(0),
      port_srv_rtp(0),
      port_srv_rtcp(0),
      send_tcp_fd(false)
    {
      memset(media, 0, MAX_MEDIA_LEN);
    }
    AMRtpClientInfo(const AMRtpClientInfo& c) :
      client_rtp_mode(c.client_rtp_mode),
      client_ip_domain(c.client_ip_domain),
      tcp_channel_rtp(c.tcp_channel_rtp),
      tcp_channel_rtcp(c.tcp_channel_rtcp),
      port_srv_rtp(c.port_srv_rtp),
      port_srv_rtcp(c.port_srv_rtcp),
      send_tcp_fd(c.send_tcp_fd)
    {
      memcpy(media, c.media, MAX_MEDIA_LEN);
      memcpy(&udp, &c.udp, sizeof(udp));
      memcpy(&tcp, &c.tcp, sizeof(tcp));
    }
    void print()
    {
      switch(client_rtp_mode) {
        case AM_RTP_TCP: {
          INFO("Client        mode: TCP");
        }break;
        case AM_RTP_UDP: {
          INFO("Client        mode: UDP");
        }break;
        case AM_RAW_UDP: {
          INFO("Client        mode: RAW UDP");
        }break;
        default: {
          INFO("Client        mode: Unknown");
        }break;
      }
      switch(client_ip_domain) {
        case AM_IPV4: {
          char ipstr[INET_ADDRSTRLEN] = {0};
          INFO("Client IP protocol: IPv4");
          INFO("Client UDP address: %s:%hu-%hu",
               inet_ntop(AF_INET, (void*)&udp.rtp.ipv4.sin_addr,
                                   ipstr, sizeof(ipstr)) ? ipstr : "",
               ntohs(udp.rtp.ipv4.sin_port),
               ntohs(udp.rtcp.ipv4.sin_port));
          INFO("Client TCP address: %s:%hu",
               inet_ntop(AF_INET, (void*)&tcp.ipv4.sin_addr,
                         ipstr, sizeof(ipstr)) ? ipstr : "",
               ntohs(tcp.ipv4.sin_port));
          INFO("   TCP RTP Channel: %hu", tcp_channel_rtp);
          INFO("  TCP RTCP Channel: %hu", tcp_channel_rtcp);
          INFO("   RTP Port Server: %hu", port_srv_rtp);
          INFO("  RTCP Port Server: %hu", port_srv_rtcp);
        }break;
        case AM_IPV6: {
          char ipstr[INET6_ADDRSTRLEN] = {0};
          INFO("Client IP protocol: IPv6");
          INFO("Client UDP address: %s:%hu-%hu",
               inet_ntop(AF_INET6, (void*)&udp.rtp.ipv6.sin6_addr,
                         ipstr, sizeof(ipstr)) ? ipstr : "",
               ntohs(udp.rtp.ipv6.sin6_port),
               ntohs(udp.rtcp.ipv6.sin6_port));
          INFO("Client TCP address: %s:%hu",
               inet_ntop(AF_INET6, (void*)&tcp.ipv6.sin6_addr,
                         ipstr, sizeof(ipstr)) ? ipstr : "",
               ntohs(tcp.ipv6.sin6_port));
          INFO("   TCP RTP Channel: %hu", tcp_channel_rtp);
          INFO("  TCP RTCP Channel: %hu", tcp_channel_rtcp);
          INFO("   RTP Port Server: %hu", port_srv_rtp);
          INFO("  RTCP Port Server: %hu", port_srv_rtcp);
        }break;
        default: {
          INFO("Client IP protocol: Unknown");
        }break;
      }
    }
};

struct AMRtpClientMsgInfo
{
    uint32_t               length;
    uint32_t               session_id;
    AM_RTP_CLIENT_MSG_TYPE type;
    AMRtpClientInfo        info;
    AMRtpClientMsgInfo() :
      length(sizeof(AMRtpClientMsgInfo)),
      session_id(0),
      type(AM_RTP_CLIENT_MSG_INFO)
    {}
    void print()
    {
      INFO("Client  session ID: %08X", session_id);
      info.print();
    }
};


struct AMRtpClientMsgControl
{
    uint32_t               length;
    uint32_t               session_id;
    AM_RTP_CLIENT_MSG_TYPE type;
    uint32_t               ssrc;
    char                   seq_ntp[MAX_MEDIA_LEN];
    AMRtpClientMsgControl(AM_RTP_CLIENT_MSG_TYPE t,
                          uint32_t client_ssrc,
                          uint32_t client_session_id) :
      length(sizeof(AMRtpClientMsgControl)),
      session_id(client_session_id),
      type(t),
      ssrc(client_ssrc)
    {
      memset(seq_ntp, 0, sizeof(seq_ntp));
    }
};

struct AMRtpClientMsgProto
{
    uint32_t               length;
    uint32_t               session_id;
    AM_RTP_CLIENT_MSG_TYPE type;
    AM_RTP_CLIENT_PROTO    proto;
    AMRtpClientMsgProto() :
      length(sizeof(AMRtpClientMsgProto)),
      session_id(0),
      type(AM_RTP_CLIENT_MSG_PROTO),
      proto(AM_RTP_CLIENT_PROTO_UNKNOWN)
    {}
};

struct AMRtpClientMsgClose
{
    uint32_t               length;
    uint32_t               session_id;
    AM_RTP_CLIENT_MSG_TYPE type;
    uint32_t               reserved;
    AMRtpClientMsgClose() :
      length(sizeof(AMRtpClientMsgClose)),
      session_id(0),
      type(AM_RTP_CLIENT_MSG_CLOSE),
      reserved(0)
    {}
};

struct AMRtpClientMsgSupport
{
  uint32_t               length;
  uint32_t               session_id;
  AM_RTP_CLIENT_MSG_TYPE type;
  uint32_t               support_media_map;
  AMRtpClientMsgSupport() :
    length(sizeof(AMRtpClientMsgSupport)),
    session_id(0),
    type(AM_RTP_CLIENT_MSG_SUPPORT),
    support_media_map(0)
  {}
};

struct AMRtpClientMsgSDPS
{
    uint32_t               length;
    uint32_t               session_id;
    AM_RTP_CLIENT_MSG_TYPE type;
    uint16_t               port[AM_RTP_MEDIA_NUM];
    char                   media_sdp[MAX_SDP_LEN];
    bool                   enable_video;
    AMRtpClientMsgSDPS() :
      length(sizeof(AMRtpClientMsgSDPS)),
      session_id(0),
      type(AM_RTP_CLIENT_MSG_SDPS),
      enable_video(false)
    {
      memset(media_sdp, 0, sizeof(media_sdp));
      memset(port, 0, sizeof(port));
    }
};

union AMRtpClientMsgBlock
{
    struct AMRtpClientMsgACK     msg_ack;
    struct AMRtpClientMsgSDP     msg_sdp;
    struct AMRtpClientMsgKill    msg_kill;
    struct AMRtpClientMsgInfo    msg_info;
    struct AMRtpClientMsgControl msg_ctrl;
    struct AMRtpClientMsgProto   msg_proto;
    struct AMRtpClientMsgClose   msg_close;
    struct AMRtpClientMsgSupport msg_support;
    struct AMRtpClientMsgSDPS    msg_sdps;
    AMRtpClientMsgBlock() {}
};
#endif /* ORYX_INCLUDE_STREAM_AM_RTP_MSG_H_ */
