/*******************************************************************************
 * am_rtp_client.cpp
 *
 * History:
 *   2015-1-6 - [ypchang] created file
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
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_rtp_client.h"
#include "am_rtp_client_config.h"

#include "am_thread.h"
#include "am_event.h"

#include "am_rtp_session_if.h"
#include "am_muxer_rtp.h"
#include "am_rtp_data.h"

struct RtcpByePacket
{
    AMRtpTcpHeader tcp_header;
    AMRtcpHeader   rtcp_command_head;
};

/*
 * AMRtpClientData
 */
AMRtpClientData::AMRtpClientData(int fd_proto,
                                 int fd_tcp,
                                 uint32_t session,
                                 const AMRtpClientInfo &info) :
    ssrc(random_number()),
    session_id(session),
    proto_fd(fd_proto),
    tcp_fd(fd_tcp),
    udp_fd_rtp(-1),
    udp_fd_rtcp(-1),
    client(info)
{
  client.print();
}

AMRtpClientData::~AMRtpClientData()
{
  if (AM_LIKELY(tcp_fd >= 0)) {
    close(tcp_fd);
  }
  if (AM_LIKELY(udp_fd_rtp >= 0)) {
    close(udp_fd_rtp);
  }
  if (AM_LIKELY(udp_fd_rtcp >= 0)) {
    close(udp_fd_rtcp);
  }
  DEBUG("~AMRtpClientData");
}

static const std::string uint_to_hex_string(uint32_t num)
{
  char str[16] = {0};
  sprintf(str, "%08X", num);
  return str;
}

static bool set_tcp_socket_opt(int fd, uint32_t sndbuf_size, uint32_t tmo_sec)
{
  bool ret = false;

  if (AM_LIKELY(fd >= 0)) {
    do {
      int32_t send_buf = sndbuf_size;
      int32_t no_delay = 1;
      struct timeval timeout = {0};
      int tos = 0;
      socklen_t tos_len = sizeof(tos);

      if (AM_UNLIKELY(getsockopt(fd, IPPROTO_IP, IP_TOS,
                                 (void*)&tos, &tos_len) != 0)) {
        PERROR("get IP_TOS");
        break;
      }
      tos = tos | IPTOS_LOWDELAY | IPTOS_RELIABILITY | IPTOS_THROUGHPUT;
      if (AM_UNLIKELY(setsockopt(fd, IPPROTO_IP, IP_TOS,
                                 (const void*)&tos, tos_len) != 0)) {
        PERROR("set IP_TOS");
        break;
      }
      if (AM_UNLIKELY((setsockopt(fd,
                                  IPPROTO_TCP, TCP_NODELAY,
                                  &no_delay, sizeof(no_delay)) != 0))) {
        PERROR("setsockopt");
        break;
      }
      if (AM_UNLIKELY((setsockopt(fd,
                                  SOL_SOCKET, SO_SNDBUF,
                                  &send_buf,
                                  sizeof(send_buf)) != 0))) {
        PERROR("setsockopt");
        break;
      }
      timeout.tv_sec = tmo_sec;
      if (AM_UNLIKELY((setsockopt(fd,
                                  SOL_SOCKET, SO_RCVTIMEO,
                                  (char*)&timeout,
                                  sizeof(timeout)) != 0))) {
        PERROR("setsockopt");
        break;
      }
      if (AM_UNLIKELY((setsockopt(fd,
                                  SOL_SOCKET, SO_SNDTIMEO,
                                  (char*)&timeout,
                                  sizeof(timeout)) != 0))) {
        PERROR("setsockopt");
        break;
      }
      ret = true;
    }while(0);
  }

  return ret;
}

bool AMRtpClientData::init(RtpClientConfig &config)
{
  name = uint_to_hex_string(ssrc);
  do {
    if (AM_LIKELY(tcp_fd >= 0)) {
      if (AM_UNLIKELY(!set_tcp_socket_opt(tcp_fd,
                                          config.send_buffer_size,
                                          config.block_timeout))) {
        ERROR("set TCP socket opt error!");
        break;
      }
    }
    udp_fd_rtp  = setup_udp_socket(config, client.port_srv_rtp);
    udp_fd_rtcp = setup_udp_socket(config, client.port_srv_rtcp);
  } while(0);

  return ((udp_fd_rtp >= 0) && (udp_fd_rtcp >= 0));
}

int AMRtpClientData::setup_udp_socket(RtpClientConfig &config, uint16_t port)
{
  int sock  = -1;
  struct sockaddr *addr = nullptr;
  struct sockaddr_in  addr4 = {0};
  struct sockaddr_in6 addr6 = {0};
  socklen_t socket_len = sizeof(struct sockaddr);
  switch(client.client_ip_domain) {
    case AM_IPV4: {
      sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      if (AM_LIKELY(sock >= 0)) {
        addr4.sin_family      = AF_INET;
        addr4.sin_addr.s_addr = htonl(INADDR_ANY);
        addr4.sin_port        = htons(port);
        addr = (struct sockaddr*)&addr4;
        socket_len = sizeof(addr4);
      } else {
        PERROR("socket");
      }
    }break;
    case AM_IPV6: {
      sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
      if (AM_LIKELY(sock >= 0)) {
        addr6.sin6_family = AF_INET6;
        addr6.sin6_addr   = in6addr_any;
        addr6.sin6_port   = htons(port);
        addr = (struct sockaddr*)&addr6;
        socket_len = sizeof(addr6);
      } else {
        PERROR("socket");
      }
    }break;
    default:break;
  }

  if (AM_LIKELY(sock >= 0)) {
    int reuse = 1;
    bool isok = false;
    do {
      if (AM_UNLIKELY(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
                                 &reuse, sizeof(reuse)) < 0)) {
        PERROR("SO_REUSEADDR");
        break;
      }
      if (AM_UNLIKELY(setsockopt(sock, SOL_SOCKET, SO_SNDBUF,
                                 &config.send_buffer_size,
                                 sizeof(config.send_buffer_size)) < 0)) {
        PERROR("SO_SNDBUF");
        break;
      }
      int tos = 0;
      socklen_t tos_len = sizeof(tos);
      if (AM_UNLIKELY(getsockopt(sock, IPPROTO_IP, IP_TOS,
                                 (void*)&tos, &tos_len) != 0)) {
        PERROR("get IP_TOS");
        break;
      }
      tos = tos | IPTOS_LOWDELAY | IPTOS_RELIABILITY | IPTOS_THROUGHPUT;
      if (AM_UNLIKELY(setsockopt(sock, IPPROTO_IP, IP_TOS,
                                 (const void*)&tos, tos_len) != 0)) {
        PERROR("set IP_TOS");
        break;
      }
      if (AM_LIKELY(config.blocked)) {
        timeval timeout = {0};
        timeout.tv_sec = config.block_timeout;
        if (AM_UNLIKELY(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
                                   (char*)&timeout, sizeof(timeout)) != 0)) {
          PERROR("SO_RCVTIMEO");
          break;
        }
        if (AM_UNLIKELY(setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO,
                                   (char*)&timeout, sizeof(timeout)) != 0)) {
          PERROR("SO_SNDTIMEO");
          break;
        }
      } else {
        int flag = fcntl(sock, F_GETFL, 0);
        flag |= O_NONBLOCK;
        if (AM_UNLIKELY(fcntl(sock, F_SETFL, flag) != 0)) {
          PERROR("fcntl");
          break;
        }
      }
      if (AM_UNLIKELY(bind(sock, addr, socket_len) < 0)) {
        PERROR("bind");
        break;
      }
      isok = true;
    } while(0);

    if (AM_UNLIKELY(!isok)) {
      close(sock);
      sock = -1;
    }
  }

  return sock;
}

struct sockaddr* AMRtpClientData::client_rtp_address(socklen_t *len)
{
  struct sockaddr *addr = NULL;
  switch(client.client_ip_domain) {
    case AM_IPV4: {
      addr = (struct sockaddr*)&(client.udp.rtp.ipv4);
      if (len) {*len = sizeof(sockaddr_in);}
    }break;
    case AM_IPV6: {
      addr = (struct sockaddr*)&(client.udp.rtp.ipv6);
      if (len) {*len = sizeof(sockaddr_in6);}
    }break;
    default: break;
  }
  return addr;
}

struct sockaddr* AMRtpClientData::client_rtcp_address(socklen_t *len)
{
  struct sockaddr *addr = NULL;
  switch(client.client_ip_domain) {
    case AM_IPV4: {
      addr = (struct sockaddr*)&(client.udp.rtcp.ipv4);
      if (len) {*len = sizeof(sockaddr_in);}
    }break;
    case AM_IPV6: {
      addr = (struct sockaddr*)&(client.udp.rtcp.ipv6);
      if (len) {*len = sizeof(sockaddr_in6);}
    }break;
    default: break;
  }
  return addr;
}

uint16_t AMRtpClientData::client_rtp_port()
{
  uint16_t port = 0;
  switch(client.client_ip_domain) {
    case AM_IPV4: {
      port = n2hs(client.udp.rtp.ipv4.sin_port);
    }break;
    case AM_IPV6: {
      port = n2hs(client.udp.rtp.ipv6.sin6_port);
    }break;
    default: break;
  }
  return port;
}

uint16_t AMRtpClientData::client_rtcp_port()
{
  uint16_t port = 0;
  switch(client.client_ip_domain) {
    case AM_IPV4: {
      port = n2hs(client.udp.rtcp.ipv4.sin_port);
    }break;
    case AM_IPV6: {
      port = n2hs(client.udp.rtcp.ipv6.sin6_port);
    }break;
    default: break;
  }
  return port;
}

/*
 * RtcpRRPacket
 */
uint32_t RtcpRRPacket::sender_ssrc()
{
  return rtcp_common_head.get_ssrc();
}

/*
 * RtcpSRPacket
 */
uint32_t RtcpSRPacket::sender_ssrc()
{
  return rtcp_common_head.get_ssrc();
}

/*
 * AMRtpClient
 */
AMRtpClient* AMRtpClient::create(const std::string &config,
                                 int proto_fd,
                                 int tcp_fd,
                                 int ctrl_fd,
                                 uint32_t session,
                                 AMMuxerRtp *muxer,
                                 const AMRtpClientInfo &clientInfo)
{
  AMRtpClient *result = new AMRtpClient(ctrl_fd, muxer);
  if (AM_UNLIKELY(result && !result->init(config, proto_fd, tcp_fd,
                                          session, clientInfo))) {
    delete result;
    result = NULL;
  }
  return result;
}

bool AMRtpClient::start()
{
  m_rtcp_event->signal();
  m_rtp_event->signal();
  return true;
}

void AMRtpClient::stop()
{
  if (AM_LIKELY(m_run)) {
    if (AM_LIKELY(send_thread_cmd(AM_CLIENT_CMD_STOP))) {
      if (AM_LIKELY(m_rtcp_event->wait(5000))) {
        NOTICE("RTCP thread of Client %s stopped!", m_client->name.c_str());
      } else {
        WARN("Failed to receive STOP response of RTCP thread of Client %s!",
             m_client->name.c_str());
      }
    }
  }
}

void AMRtpClient::set_enable(bool enable)
{
  AUTO_MEM_LOCK(m_lock);
  m_enable = enable;
}

bool AMRtpClient::is_alive()
{
  return m_run;
}

bool AMRtpClient::is_abort()
{
  return m_abort;
}

bool AMRtpClient::is_enable()
{
  AUTO_MEM_LOCK(m_lock);
  return m_enable;
}

bool AMRtpClient::is_new_client()
{
  return m_is_new.load();
}

void AMRtpClient::set_new_client(bool new_client)
{
  m_is_new = new_client;
}

void AMRtpClient::destroy(bool send_notify)
{
  if (-- m_ref_count <= 0) {
    stop();
    if (AM_LIKELY(send_notify)) {
      send_kill_client();
    }
    delete this;
  }
}

uint32_t AMRtpClient::ssrc()
{
  return (m_client ? m_client->ssrc : ((uint32_t)-1));
}

void AMRtpClient::inc_ref()
{
  ++ m_ref_count;
}

void AMRtpClient::set_session(AMIRtpSession *session)
{
  AUTO_MEM_LOCK(m_lock);
  m_session = session;
}

AMRtpClient::AMRtpClient(int ctrl_fd, AMMuxerRtp *muxer) :
  m_muxer(muxer),
  m_ctrl_fd(ctrl_fd)
{
}

AMRtpClient::~AMRtpClient()
{
  AM_DESTROY(m_rtcp_monitor);
  AM_DESTROY(m_packet_sender);
  AM_DESTROY(m_rtp_event);
  AM_DESTROY(m_rtcp_event);
  if (AM_LIKELY(RTP_CLI_R >= 0)) {
    close(RTP_CLI_R);
  }
  if (AM_LIKELY(RTP_CLI_W >= 0)) {
    close(RTP_CLI_W);
  }
  delete m_client;
  delete m_config;
}

bool AMRtpClient::init(const std::string &config,
                       int proto_fd,
                       int tcp_fd,
                       uint32_t session,
                       const AMRtpClientInfo &clientInfo)
{
  bool ret = false;
  do {
    m_config = new AMRtpClientConfig();
    if (AM_UNLIKELY(!m_config)) {
      ERROR("Failed to create AMRtpClientConfig object!");
      break;
    }
    m_client_config = m_config->get_config(config);
    if (AM_UNLIKELY(!m_client_config)) {
      ERROR("Failed to get RTP client configuration!");
      break;
    }
    if (AM_UNLIKELY(pipe(m_ctrl) < 0)) {
      PERROR("pipe");
      break;
    }
    m_client = new AMRtpClientData(proto_fd, tcp_fd, session, clientInfo);
    if (AM_UNLIKELY(!m_client)) {
      ERROR("Failed to create AMRtpClientData!");
      break;
    }
    if (AM_UNLIKELY(!m_client->init(*m_client_config))) {
      ERROR("Failed to initialize RTP client data!");
      break;
    }
    m_rtcp_sr_buf = new RtcpSRBuffer();
    if (AM_UNLIKELY(!m_rtcp_sr_buf)) {
      ERROR("Failed to allocate memory for RTCP SR packet!");
      break;
    } else {
      m_rtcp_hdr = &m_rtcp_sr_buf->rtcp_hdr;
      m_rtcp_sr  = &m_rtcp_sr_buf->rtcp_sr;
      m_rtcp_sr->set_packet_bytes(0);
      m_rtcp_sr->set_packet_count(0);
    }

    m_rtp_event = AMEvent::create();
    m_rtcp_event = AMEvent::create();
    if (AM_UNLIKELY(!m_rtp_event || !m_rtcp_event)) {
      ERROR("Failed to create event object!");
      break;
    }
    std::string packet_sender_name = m_client->name + ".rtp";
    m_packet_sender = AMThread::create(packet_sender_name,
                                       static_packet_sender,
                                       this);
    if (AM_UNLIKELY(!m_packet_sender)) {
      ERROR("Failed to create RTP packet sending thread for client %s!",
            m_client->name.c_str());
      break;
    }
    std::string rtcp_monitor_name = m_client->name + ".rtcp";
    m_rtcp_monitor = AMThread::create(rtcp_monitor_name,
                                      static_rtcp_monitor,
                                      this);
    if (AM_UNLIKELY(!m_rtcp_monitor)) {
      ERROR("Failed to create RTCP monitor thread for client %s!",
            m_client->name.c_str());
      break;
    }
    /* Don't let system handle SIGPIPE error */
    signal(SIGPIPE, SIG_IGN);
    ret = true;
  }while(0);

  return ret;
}

void AMRtpClient::static_rtcp_monitor(void *data)
{
  ((AMRtpClient*)data)->rtcp_monitor();
}

void AMRtpClient::static_packet_sender(void *data)
{
  ((AMRtpClient*)data)->packet_sender();
}

void AMRtpClient::rtcp_monitor()
{
  int maxfd = -1;
  bool rtcp_received = false;
  bool need_signal = false;
  uint32_t err_count = 0;
  uint32_t rr_count_threashold = 3;
  uint32_t rr_count = 0;
  uint64_t rr_recv_ntp_last = 0;
  uint64_t rr_recv_ntp_interval = 0;
  int32_t dynamic_client_timeout = m_client_config->client_timeout;
  timeval rtcp_sr_last_recv_time = {0,0};
  uint64_t rtcp_sr_interval = 0;
  uint32_t timeout_count = 0;

  fd_set allset;
  fd_set fdset;
  timeval timeout;

  FD_ZERO(&allset);
  FD_SET(RTP_CLI_R, &allset);
  FD_SET(m_client->udp_fd_rtcp, &allset);
  maxfd = AM_MAX(RTP_CLI_R, m_client->udp_fd_rtcp);

  m_rtcp_event->wait();

  while(m_run) {
    int retval = -1;
    /* Handle UDP mode only */
    timeval *tm = (m_client->client.client_rtp_mode != AM_RTP_TCP) ?
        &timeout : nullptr;

    timeout.tv_sec  = dynamic_client_timeout;
    timeout.tv_usec = 0;
    fdset = allset;
    if (AM_LIKELY((retval = select(maxfd + 1, &fdset, NULL, NULL, tm)) > 0)) {
      if (FD_ISSET(RTP_CLI_R, &fdset)) {
        char cmd[1] = {0};
        uint32_t read_count = 0;
        ssize_t  read_ret   = 0;
        do {
          read_ret = read(RTP_CLI_R, &cmd, sizeof(cmd));
        }while((++ read_count < 5) && ((read_ret == 0) ||
               ((read_ret < 0) && ((errno == EAGAIN) ||
                   (errno == EWOULDBLOCK) || (errno == EINTR)))));
        if (AM_UNLIKELY(read_ret <= 0)) {
          PERROR("read");
        } else {
          switch(cmd[0]) {
            case AM_CLIENT_CMD_STOP: {
              NOTICE("Client: %s received stop signal!",
                     m_client->name.c_str());
              m_run = false;
              need_signal = true;
            }break;
            case AM_CLIENT_CMD_ABORT: {
              NOTICE("Client: %s received abort signal!",
                     m_client->name.c_str());
              m_abort = true;
              need_signal = true;
            }break;
            case AM_CLIENT_CMD_ACK: {
              if (AM_LIKELY(!rtcp_received)) {
                NOTICE("Client: %s received ACK to keep alive...",
                       m_client->name.c_str());
              }
              timeout_count = 0;
            }break;
            default: break;
          }
          if (AM_UNLIKELY(m_abort || !m_run)) {
            /* Send RTCP BYE */
            RtcpByePacket rtcp_bye;
            rtcp_bye.tcp_header.set_channel_id(m_client->\
                                               client.tcp_channel_rtcp);
            rtcp_bye.tcp_header.set_magic_num();
            rtcp_bye.tcp_header.set_data_length(sizeof(AMRtcpHeader));
            rtcp_bye.rtcp_command_head.count = 1;
            rtcp_bye.rtcp_command_head.set_packet_type(AM_RTCP_BYE);
            rtcp_bye.rtcp_command_head.\
            set_packet_length(sizeof(AMRtcpHeader) / sizeof(uint32_t) - 1);
            switch(m_client->client.client_rtp_mode) {
              case AM_RTP_TCP: {
                send_rtcp_packet((uint8_t*)&rtcp_bye, sizeof(rtcp_bye));
              }break;
              case AM_RTP_UDP: {
                send_rtcp_packet((uint8_t*)&rtcp_bye.rtcp_command_head,
                                 sizeof(AMRtcpHeader));
              }break;
              default:break;
            }
          }
          if (AM_UNLIKELY(m_abort)) {
            break;
          }
        }
      } else if (FD_ISSET(m_client->udp_fd_rtcp, &fdset)) {
        socklen_t socklen = 0;
        sockaddr *addr = m_client->client_rtcp_address(&socklen);
        uint8_t recv_buffer[sizeof(AMRtcpHeader) + sizeof(AMRtcpSRPayload) +
                (sizeof(AMRtcpRRPayload) * m_client_config->report_block_cnt)];

        int read_ret = addr ? recvfrom(m_client->udp_fd_rtcp, &recv_buffer,
                                       sizeof(recv_buffer),
                                       0, addr, &socklen) : 0;
        if (AM_UNLIKELY(read_ret < 0)) {
          PERROR("recvfrom");
        } else if (read_ret > (int)sizeof(AMRtcpHeader)) {
          AMRtcpHeader *rtcp_header = (AMRtcpHeader*)&recv_buffer;
          switch (rtcp_header->pkt_type) {
            case AM_RTCP_RR: {
              RtcpRRPacket       *rtcp_rr = (RtcpRRPacket*)&recv_buffer;
              AMRtcpRRPayload *rr_payload = rtcp_rr->rtcp_rr_payload;
              uint8_t rr_pkg_count = rtcp_header->get_count();
              timeout_count = 0;
              for (uint8_t cnt = 0; cnt < rr_pkg_count; ++ cnt) {
                bool is_valid = ((int)(sizeof(AMRtcpHeader) +
                                 (cnt + 1) * sizeof(AMRtcpRRPayload)) <=
                                 read_ret);
                if (AM_LIKELY(is_valid)) {
                  INFO("%s.rtcp received RTCP packet(%u) of "
                       "source %08X from %08X",
                       m_client->name.c_str(),
                       rtcp_rr->rtcp_common_head.pkt_type,
                       rtcp_rr->rtcp_rr_payload[cnt].get_source_ssrc(),
                       rtcp_rr->rtcp_common_head.get_ssrc());
                }
                if (AM_LIKELY(is_valid && m_client_config->print_rtcp_stat)) {

                  INFO("\nReceived RR of source %08X:"
                       "\n              Fraction Lost: %hhu"
                       "\n         Total Packets Lost: %d"
                       "\nReceived Packets Seq Number: %u"
                       "\n        Interarrival Jitter: %u"
                       "\n                    Last SR: %u"
                       "\n        Delay Since Last SR: %u",
                       rr_payload[cnt].get_source_ssrc(),
                       rr_payload[cnt].get_fraction_lost(),
                       rr_payload[cnt].get_packet_lost(),
                       rr_payload[cnt].get_sequence_number(),
                       rr_payload[cnt].get_jitter(),
                       rr_payload[cnt].get_lsr(),
                       rr_payload[cnt].get_dlsr());
                }
                if (AM_LIKELY(is_valid && (m_client->ssrc ==
                              rr_payload[cnt].get_source_ssrc()))) {
                  uint64_t rr_recv_ntp = m_session->fake_ntp_time_us();
                  rtcp_received = true;
                  /*uint32_t sender_ssrc = rtcp_rr.sender_ssrc();*/
                  if (AM_LIKELY((rr_recv_ntp_last > 0) &&
                                (rr_recv_ntp_last < rr_recv_ntp))) {
                    rr_recv_ntp_interval += (rr_recv_ntp - rr_recv_ntp_last);
                  }
                  rr_recv_ntp_last = rr_recv_ntp;
                  rr_count += rr_recv_ntp_interval ? 1 : 0;
                  if (AM_UNLIKELY(rr_count >= rr_count_threashold)) {
                    /* (A/B + 1/2) == ((2A + B) / 2B) */
                    uint64_t avg_interval =
                        (2 * rr_recv_ntp_interval + rr_count) / (2 * rr_count);

                    dynamic_client_timeout = (int32_t)
                          ((2 * rr_recv_ntp_interval + (rr_count * 1000000)) /
                              (2 * (rr_count * 1000000))) + 2;
                    rr_count_threashold = (uint32_t)
                        ((2 * dynamic_client_timeout * 1000000ULL +
                            avg_interval) / (2 * avg_interval));
                    if (dynamic_client_timeout <= 2) {
                      WARN("Too many RTCP RR packets!");
                      dynamic_client_timeout  = (dynamic_client_timeout > 0) ?
                          dynamic_client_timeout * 5 : 10;
                    } else {
                      dynamic_client_timeout *= 4;
                    }
                    NOTICE("%s.rtcp got %u RR packets within "
                           "%llu microseconds!",
                           m_client->name.c_str(),
                           rr_count,
                           rr_recv_ntp_interval);
                    NOTICE("%s client timeout is changed to %u seconds, "
                        "RR packet statistics threshold changed to %u",
                        m_client->name.c_str(),
                        dynamic_client_timeout,
                        rr_count_threashold);
                    rr_count = 0;
                    rr_recv_ntp_interval = 0ULL;
                  }
                }
              }
            }break;
            case AM_RTCP_SR: {
              RtcpSRPacket *rtcp_sr = (RtcpSRPacket*)&recv_buffer;
              timeout_count = 0;
              if (AM_LIKELY(m_client_config->print_rtcp_stat)) {
                INFO("\nReceived SR sender info of SSRC %08X:"
                     "\n           NTP  Timestamp MSW: %u"
                     "\n           NTP  Timestamp LSW: %u"
                     "\n                RTP Timestamp: %u"
                     "\n        Sender's packet count: %u"
                     "\n         Sender's octet count: %u",
                     rtcp_sr->sender_ssrc(),
                     rtcp_sr->rtcp_sr_payload.get_ntp_time_h32(),
                     rtcp_sr->rtcp_sr_payload.get_ntp_time_l32(),
                     rtcp_sr->rtcp_sr_payload.get_time_stamp(),
                     rtcp_sr->rtcp_sr_payload.get_packet_count(),
                     rtcp_sr->rtcp_sr_payload.get_packet_bytes());
              }
              for (uint32_t cnt = 0; cnt < rtcp_header->get_count(); ++ cnt) {
                timeval rtcp_recv_time;

                INFO("%s.rtcp received RTCP packet(%u) of "
                     "source %08X from %08X",
                     m_client->name.c_str(),
                     rtcp_sr->rtcp_common_head.pkt_type,
                     rtcp_sr->rtcp_rr_payload[cnt].get_source_ssrc(),
                     rtcp_sr->rtcp_common_head.get_ssrc());
                if (AM_LIKELY(m_client_config->print_rtcp_stat)) {
                  AMRtcpRRPayload *rr_payload = rtcp_sr->rtcp_rr_payload;
                  INFO("\nReceived SR report block of source %08X:"
                       "\n              Fraction Lost: %hhu"
                       "\n         Total Packets Lost: %u"
                       "\nReceived Packets Seq Number: %u"
                       "\n        Interarrival Jitter: %u"
                       "\n                    Last SR: %u"
                       "\n        Delay Since Last SR: %u",
                       rr_payload[cnt].get_source_ssrc(),
                       rr_payload[cnt].get_fraction_lost(),
                       rr_payload[cnt].get_packet_lost(),
                       rr_payload[cnt].get_sequence_number(),
                       rr_payload[cnt].get_jitter(),
                       rr_payload[cnt].get_lsr(),
                       rr_payload[cnt].get_dlsr());
                }

                gettimeofday(&rtcp_recv_time, NULL);
                if (AM_LIKELY(m_client->ssrc == rtcp_sr->rtcp_rr_payload[cnt].\
                              get_source_ssrc())) {
                  if (AM_LIKELY((rtcp_sr_last_recv_time.tv_sec > 0) &&
                  (rtcp_recv_time.tv_sec > rtcp_sr_last_recv_time.tv_sec))) {
                    rtcp_sr_interval = rtcp_recv_time.tv_sec -
                                               rtcp_sr_last_recv_time.tv_sec;
                  }
                  memcpy(&rtcp_sr_last_recv_time, &rtcp_recv_time,
                         sizeof(timeval));
                  dynamic_client_timeout = rtcp_sr_interval ?
                    (rtcp_sr_interval * 3) : m_client_config->client_timeout;
                  if (AM_LIKELY(rtcp_sr_interval)) {
                    NOTICE("%s.rtcp got one SR packet within %llu seconds",
                           m_client->name.c_str(),
                           rtcp_sr_interval);
                  }
                  NOTICE("%s.rtcp client timeout is changed to %d seconds now.",
                          m_client->name.c_str(),
                          dynamic_client_timeout);
                }
              }
            }break;
            case AM_RTCP_BYE: {
              NOTICE("%s.rtcp got RTCP BYE packet.", m_client->name.c_str());
            }break;
            case AM_RTCP_SDES: {
              NOTICE("%s.rtcp got RTCP SDES packet.", m_client->name.c_str());
            }break;
            default:break;
          }
        }
      }
    } else if (retval == 0) {
      WARN("Client: %s is not responding within %d seconds!",
           m_client->name.c_str(), dynamic_client_timeout);
      ++ timeout_count;
      if (AM_LIKELY(timeout_count > m_client_config->client_timeout_count)) {
        WARN("Client: %s failed to report RTCP RR packet %d times! shutdown!",
             m_client->name.c_str(), timeout_count);
        m_abort = true;
        break;
      }
    } else {
      PERROR("select");
      if (AM_LIKELY(++ err_count > 5)) {
        m_abort = true;
        break;
      }
    }
  }

  if (AM_LIKELY(m_run && m_abort)) {
    abort_client(false);
    m_run = false;
  }
  if (AM_LIKELY(need_signal)) {
    m_rtcp_event->signal();
  }
}

void AMRtpClient::packet_sender()
{
  uint64_t first_ntp_time_stamp = 0ULL;
  uint64_t last_ntp_time_stamp  = 0ULL;
  uint64_t curr_ntp_time        = 0ULL;

  uint64_t  first_sent_pkt_pts  = 0ULL;
  uint64_t  last_sent_pkt_pts   = 0ULL;
  uint64_t  curr_clock_cnt_90k  = 0ULL;

  uint32_t first_rtp_time_stamp = 0;
  uint32_t last_rtp_time_stamp  = 0;

  uint32_t last_sent_rtcp_bytes = 0;
  bool need_send_rtcp_sr = false;
  bool first_rtcp = true;

  m_rtp_event->wait();
  while(m_run) {
    AMRtpData *rtp_data = nullptr;
    AMRtpPacket *packet = nullptr;
    uint32_t    pkt_num = 0;
    uint32_t payload_size = 0;
    if (AM_UNLIKELY(!m_session)) {
      usleep(m_client_config->wait_interval * 1000);
      continue;
    }

    if (AM_UNLIKELY(!m_session->is_client_ready(m_client->ssrc))) {
      NOTICE("Client %s is already removed, abort!", m_client->name.c_str());
      m_abort = true;
      break;
    }

    rtp_data = m_session->get_rtp_data_by_ssrc(m_client->ssrc);
    if (AM_UNLIKELY(!rtp_data)) {
      usleep(m_client_config->wait_interval * 1000);
      continue;
    }
    m_lock.lock();

    last_sent_pkt_pts = rtp_data->get_pts();
    packet = rtp_data->get_packet();

    if (AM_UNLIKELY(!m_abort && first_rtcp)) {
      AM_NET_STATE rtcp_send_state = AM_NET_OK;
      AMRtpHeader *rtp_header = (AMRtpHeader*)packet[0].udp_data;

      first_rtcp = false;
      first_ntp_time_stamp = m_session->fake_ntp_time_us();
      first_rtp_time_stamp = n2hl(rtp_header->timestamp);
      first_sent_pkt_pts = last_sent_pkt_pts;

      /* This is the very first RTCP SR packet */
      build_rtcp_sr_packet(*m_rtcp_sr_buf,
                           m_client->ssrc,
                           first_rtp_time_stamp,
                           first_ntp_time_stamp);
      last_sent_rtcp_bytes = m_rtcp_sr_buf->rtcp_sr.get_packet_bytes();
      last_ntp_time_stamp = first_ntp_time_stamp;

      switch(m_client->client.client_rtp_mode) {
        case AM_RTP_TCP: {
          rtcp_send_state = send_rtcp_packet((uint8_t*)m_rtcp_sr_buf,
                                             sizeof(RtcpSRBuffer));
        }break;
        case AM_RTP_UDP: {
          rtcp_send_state = send_rtcp_packet((uint8_t*)m_rtcp_hdr,
                                             sizeof(AMRtcpHeader) +
                                             sizeof(AMRtcpSRPayload));
        }break;
        default:break;
      }

      switch(rtcp_send_state) {
        case AM_NET_ERROR: m_abort = true; break;
        case AM_NET_SLOW: /*todo:*/break;
        default: break;
      }
    }

    payload_size = rtp_data->get_payload_size();
    pkt_num = rtp_data->get_packet_number();
    /* Send data */
    for (uint32_t i = 0; i < pkt_num && !m_abort; ++ i) {
      AM_NET_STATE state = AM_NET_OK;
      if ((i + 1) == pkt_num) {
        /* save the last RTP packet's timestamp */
        last_rtp_time_stamp =
            n2hl(((AMRtpHeader*)packet[i].udp_data)->timestamp);
      }
      switch(m_client->client.client_rtp_mode) {
        case AM_RTP_TCP: {
          uint32_t len = packet[i].total_size;
          uint8_t  data[len];
          uint8_t *ssrc = &data[sizeof(AMRtpTcpHeader) + 8];
          memcpy(data, packet[i].tcp_data, len);

          data[1] = m_client->client.tcp_channel_rtp;
          ssrc[0] = (m_client->ssrc & 0xff000000) >> 24;
          ssrc[1] = (m_client->ssrc & 0x00ff0000) >> 16;
          ssrc[2] = (m_client->ssrc & 0x0000ff00) >>  8;
          ssrc[3] = (m_client->ssrc & 0x000000ff);
          state = tcp_send(m_client->tcp_fd, data, len);
        }break;
        case AM_RTP_UDP: {
          uint32_t len = packet[i].total_size - sizeof(AMRtpTcpHeader);
          uint8_t data[len];
          uint8_t *ssrc = &data[8];
          sockaddr *addr = m_client->client_rtp_address(nullptr);

          memcpy(data, packet[i].udp_data, len);
          ssrc[0] = (m_client->ssrc & 0xff000000) >> 24;
          ssrc[1] = (m_client->ssrc & 0x00ff0000) >> 16;
          ssrc[2] = (m_client->ssrc & 0x0000ff00) >>  8;
          ssrc[3] = (m_client->ssrc & 0x000000ff);
          state = addr ? udp_send(m_client->udp_fd_rtp, addr, data, len) :
              AM_NET_ERROR;
        }break;
        case AM_RAW_UDP:
        default: {
          ERROR("RTP mode[%u] not supported!",
                m_client->client.client_rtp_mode);
        }break;
      }

      switch(state) {
        case AM_NET_ERROR: {
          m_abort = true;
        }break;
        case AM_NET_SLOW: {
          /* todo: */
        }break;
        default: break;
      }

      if (AM_UNLIKELY(m_abort)) {
        break;
      }
    }
    update_rtcp_sr_pkt_num_bytes(*m_rtcp_sr_buf, pkt_num, payload_size);

    /* release this AMRtpData */
    rtp_data->release();

    curr_ntp_time = m_session->fake_ntp_time_us(&curr_clock_cnt_90k);

    need_send_rtcp_sr = ((((m_rtcp_sr->get_packet_bytes() -
                            last_sent_rtcp_bytes) * 5 / 1000) >=
                         (sizeof(AMRtcpHeader) + sizeof(AMRtcpSRPayload))) &&
                        ((curr_ntp_time - last_ntp_time_stamp) >=
                            m_client_config->rtcp_sr_interval * 1000000ULL));

    if (AM_LIKELY(!m_abort && need_send_rtcp_sr)) {
      AM_NET_STATE rtcp_send_state = AM_NET_OK;
      uint64_t clock_diff = m_client_config->rtcp_incremental_ts ?
                            (curr_clock_cnt_90k - last_sent_pkt_pts) :
                            (curr_clock_cnt_90k - first_sent_pkt_pts);
      int32_t tsdiff = (int32_t)
          ((clock_diff * m_session->get_session_clock() + 45000ULL) / 90000ULL);

      last_rtp_time_stamp = m_client_config->rtcp_incremental_ts ?
          (last_rtp_time_stamp + tsdiff) :
          (first_rtp_time_stamp + tsdiff);

      build_rtcp_sr_packet(*m_rtcp_sr_buf,
                           m_client->ssrc,
                           last_rtp_time_stamp,
                           curr_ntp_time);

      last_sent_rtcp_bytes = m_rtcp_sr_buf->rtcp_sr.get_packet_bytes();
      last_ntp_time_stamp = curr_ntp_time;

      switch(m_client->client.client_rtp_mode) {
        case AM_RTP_TCP: {
          rtcp_send_state = send_rtcp_packet((uint8_t*)m_rtcp_sr_buf,
                                             sizeof(RtcpSRBuffer));
        }break;
        case AM_RTP_UDP: {
          rtcp_send_state = send_rtcp_packet((uint8_t*)m_rtcp_hdr,
                                             sizeof(AMRtcpHeader) +
                                             sizeof(AMRtcpSRPayload));
        }break;
        default:break;
      }

      switch(rtcp_send_state) {
        case AM_NET_ERROR: m_abort = true; break;
        case AM_NET_SLOW: /*todo:*/break;
        default: break;
      }
    }
    m_lock.unlock();
    if (AM_UNLIKELY(m_abort)) {
      break;
    }
  }
  if (AM_LIKELY(m_abort && m_run)) {
    abort_client(true);
    m_run = false;
  }
}

AM_NET_STATE AMRtpClient::tcp_send(int fd, uint8_t *data, uint32_t len)
{
  AM_NET_STATE    state = AM_NET_OK;
  bool              ret = false;
  int          send_ret = -1;
  uint32_t   total_sent = 0;
  uint32_t resend_count = 0;
  timeval         start = {0, 0};
  timeval           end = {0, 0};
  bool        need_stat = true;

  if (AM_UNLIKELY(gettimeofday(&start, NULL)) < 0) {
    PERROR("gettimeofday");
    need_stat = false;
  }

  do {
    send_ret = send(fd, data + total_sent,
                    len - total_sent,
                    m_client_config->blocked ? 0 : MSG_DONTWAIT);
    if (AM_UNLIKELY(send_ret < 0)) {
      bool need_break = true;
      switch(errno) {
        case EAGAIN:
        case EINTR: {
          need_break = false;
        }break;
        case EPIPE: {
          WARN("Connection to client(%s) has been shut down!",
               m_client->name.c_str());
        }break;
        case ECONNRESET: {
          WARN("Client(%s) closed it's connection!",
               m_client->name.c_str());
        }break;
        default: {
          PERROR("send");
        }break;
      }
      if (AM_LIKELY(need_break)) {
        state = AM_NET_ERROR;
        break;
      }
    } else {
      total_sent += send_ret;
    }
    ret = (len == total_sent);
    if (AM_UNLIKELY(!ret)) {
      if (AM_LIKELY(++ resend_count >= m_client_config->resend_count)) {
        ERROR("Network IO is too slow, tried %d times, "
              "%d bytes sent, %u bytes dropped!",
              resend_count, total_sent, (len - total_sent));
        state = AM_NET_SLOW;
        break;
      }
      usleep(m_client_config->resend_interval * 1000);
    }
  }while(!ret);

  if (AM_UNLIKELY(gettimeofday(&end, NULL)) < 0) {
    PERROR("gettimeofday");
    need_stat = false;
  }
  if (AM_LIKELY(need_stat)) {
    uint64_t timediff = (end.tv_sec * 1000000 + end.tv_usec) -
        (start.tv_sec * 1000000 + start.tv_usec) -
        ((m_client_config->resend_interval * 1000) *
            ((resend_count >= m_client_config->resend_count) ?
                (resend_count - 1) : resend_count));
    uint64_t curr_speed = total_sent * 1000000 / timediff / 1024;
    ++ m_tcp_send_count;
    m_tcp_send_size += total_sent;
    m_tcp_send_time += timediff;
    m_tcp_max_speed = (curr_speed > m_tcp_max_speed) ?
        curr_speed : m_tcp_max_speed;
    m_tcp_min_speed = (curr_speed < m_tcp_min_speed) ?
        curr_speed : m_tcp_min_speed;
    if (AM_UNLIKELY(timediff >= 200000)) {
      WARN("Send %d bytes takes %llu ms!", total_sent, timediff / 1000);
    }
    if (AM_UNLIKELY(m_tcp_send_count >= m_client_config->statistics_count)) {
      m_tcp_avg_speed = m_tcp_send_size * 1000000 / m_tcp_send_time / 1024;
      STAT("\nSend %u times, %llu bytes data sent through TCP, takes %llu ms"
           "\n        Average speed %llu KB/sec"
           "\nInstant minimum speed %llu KB/sec"
           "\nInstant maximum speed %llu KB/sec",
           m_tcp_send_count, m_tcp_send_size, m_tcp_send_time / 1000,
           m_tcp_avg_speed, m_tcp_min_speed, m_tcp_max_speed);
      m_tcp_send_count = 0;
      m_tcp_send_size = 0ULL;
      m_tcp_send_time = 0ULL;
      m_tcp_max_speed = 0ULL;
      m_tcp_min_speed = ((uint64_t)-1);
    }
  }

  return state;
}

AM_NET_STATE AMRtpClient::udp_send(int fd, sockaddr *addr,
                                   uint8_t *data, uint32_t len)
{
  AM_NET_STATE      state = AM_NET_OK;
  bool                ret = false;
  int            send_ret = -1;
  uint32_t     total_sent = 0;
  uint32_t   resend_count = 0;
  timeval           start = {0, 0};
  timeval             end = {0, 0};
  bool          need_stat = true;

  if (AM_UNLIKELY(gettimeofday(&start, NULL)) < 0) {
    PERROR("gettimeofday");
    need_stat = false;
  }

  do {
    send_ret = sendto(fd, data + total_sent, len - total_sent,
                      (m_client_config->blocked ? 0 : MSG_DONTWAIT),
                      addr, sizeof(sockaddr));
    if (AM_UNLIKELY(send_ret < 0)) {
      bool need_break = true;
      switch(errno) {
        case EAGAIN:
        case EINTR: {
          need_break = false;
        }break;
        case EPIPE: {
          WARN("Connection to client(%s) has been shut down!",
               m_client->name.c_str());
        }break;
        case ECONNRESET: {
          WARN("Client(%s) closed it's connection!",
               m_client->name.c_str());
        }break;
        default: {
          PERROR("send");
        }break;
      }
      if (AM_LIKELY(need_break)) {
        state = AM_NET_ERROR;
        break;
      }
    } else {
      total_sent += send_ret;
    }
    ret = (len == total_sent);
    if (AM_UNLIKELY(!ret)) {
      if (AM_LIKELY(++ resend_count >= m_client_config->resend_count)) {
        ERROR("Network IO is too slow, tried %d times, "
              "%d bytes sent, %u bytes dropped!",
              resend_count, total_sent, (len - total_sent));
        state = AM_NET_SLOW;
        break;
      }
      usleep(m_client_config->resend_interval * 1000);
    }
  }while(!ret);

  if (AM_UNLIKELY(gettimeofday(&end, NULL)) < 0) {
    PERROR("gettimeofday");
    need_stat = false;
  }
  if (AM_LIKELY(need_stat)) {
    uint64_t timediff = (end.tv_sec * 1000000 + end.tv_usec) -
        (start.tv_sec * 1000000 + start.tv_usec) -
        ((m_client_config->resend_interval * 1000) *
            ((resend_count >= m_client_config->resend_count) ?
                (resend_count - 1) : resend_count));
    uint64_t curr_speed = total_sent * 1000000 / timediff / 1024;
    ++ m_udp_send_count;
    m_udp_send_size += total_sent;
    m_udp_send_time += timediff;
    m_udp_max_speed = (curr_speed > m_udp_max_speed) ?
        curr_speed : m_udp_max_speed;
    m_udp_min_speed = (curr_speed < m_udp_min_speed) ?
        curr_speed : m_udp_min_speed;
    if (AM_UNLIKELY(timediff >= 200000)) {
      WARN("Send %d bytes takes %llu ms!", total_sent, timediff / 1000);
    }
    if (AM_UNLIKELY(m_udp_send_count >= m_client_config->statistics_count)) {
      m_udp_avg_speed = m_udp_send_size * 1000000 / m_udp_send_time / 1024;
      STAT("\nSend %u times, %llu bytes data sent through UDP, takes %llu ms"
           "\n        Average speed %llu KB/sec"
           "\nInstant minimum speed %llu KB/sec"
           "\nInstant maximum speed %llu KB/sec",
           m_udp_send_count, m_udp_send_size, m_udp_send_time / 1000,
           m_udp_avg_speed, m_udp_min_speed, m_udp_max_speed);
      m_udp_send_count = 0;
      m_udp_send_size = 0ULL;
      m_udp_send_time = 0ULL;
      m_udp_max_speed = 0ULL;
      m_udp_min_speed = ((uint64_t)-1);
    }
  }

  return state;
}

AM_NET_STATE AMRtpClient::send_rtcp_packet(uint8_t *data, uint32_t len)
{
  AM_NET_STATE state = AM_NET_OK;
  AMRtcpHeader *rtcp_header = nullptr;
  switch(m_client->client.client_rtp_mode) {
    case AM_RTP_TCP: {
      rtcp_header = (AMRtcpHeader*)&data[sizeof(AMRtpTcpHeader)];
      state = tcp_send(m_client->tcp_fd, data, len);
      INFO("Send RTCP packet(%u) %u bytes over TCP to client %s",
           rtcp_header->pkt_type,
           len,
           m_client->name.c_str());
    }break;
    case AM_RTP_UDP: {
      sockaddr *addr = m_client->client_rtcp_address(nullptr);
      rtcp_header = (AMRtcpHeader*)data;
      state = udp_send(m_client->udp_fd_rtcp, addr, data, len);
      INFO("Send RTCP packet(%u) %u bytes over UDP to client %s:%u",
           rtcp_header->pkt_type,
           len,
           m_client->name.c_str(),
           m_client->client_rtcp_port());
    }break;
    case AM_RAW_UDP:
    default:{
      ERROR("RTP mode[%u] not supported!", m_client->client.client_rtp_mode);
    }break;
  }

  return state;
}

inline void AMRtpClient::update_rtcp_sr_pkt_num_bytes(RtcpSRBuffer &sr,
                                                      uint32_t pkt_num,
                                                      uint32_t payload_size)
{
  AMRtcpSRPayload &rtcp_sr = sr.rtcp_sr;
  rtcp_sr.add_packet_count(pkt_num);
  rtcp_sr.add_packet_bytes(payload_size);
}

void AMRtpClient::build_rtcp_sr_packet(RtcpSRBuffer &sr,
                                       uint32_t ssrc,
                                       uint32_t rtp_ts,
                                       uint64_t ntp_ts)
{
  AMRtcpHeader   &rtcp_hdr = sr.rtcp_hdr;
  AMRtcpSRPayload &rtcp_sr = sr.rtcp_sr;
  uint8_t *tcp_header = (uint8_t*)&sr.tcp_hdr;
  tcp_header[0] = 0x24;
  tcp_header[1] = m_client->client.tcp_channel_rtcp;
  tcp_header[2] = (RTCP_SR_PACKET_SIZE & 0xff00) >> 8;
  tcp_header[3] = (RTCP_SR_PACKET_SIZE & 0x00ff);

  rtcp_hdr.padding = 0;
  rtcp_hdr.version = 2;
  rtcp_hdr.count   = 0;
  rtcp_hdr.set_packet_type(AM_RTCP_SR);
  rtcp_hdr.set_ssrc(ssrc);
  rtcp_hdr.set_packet_length((uint16_t)
                                ((sizeof(AMRtcpHeader) +
                                    sizeof(AMRtcpSRPayload)) /
                                    sizeof(uint32_t) - 1));
  rtcp_sr.set_time_stamp(rtp_ts);
  rtcp_sr.set_ntp_time_h32((uint32_t)(ntp_ts / 1000000ULL));
  rtcp_sr.set_ntp_time_l32((uint32_t)(((ntp_ts % 1000000ULL)<<32)/1000000ULL));
  INFO("%s RTCP TS: %u, NTP H32: %u, NTP L32: %u",
       m_client->name.c_str(),
       rtp_ts,
       (uint32_t)(ntp_ts / 1000000ULL),
       (uint32_t)(((ntp_ts % 1000000ULL) << 32) / 1000000ULL));
}

void AMRtpClient::abort_client(bool send_cmd)
{
  if (AM_LIKELY(send_cmd)) {
    if (AM_LIKELY(send_thread_cmd(AM_CLIENT_CMD_ABORT))) {
      if (AM_LIKELY(m_rtcp_event->wait(5000))) {
        NOTICE("RTCP thread of Client %s aborted!", m_client->name.c_str());
      } else {
        WARN("Failed to receive ABORT response of RTCP thread of Client %s!",
             m_client->name.c_str());
      }
    }
  }
  m_muxer->kill_client(m_client->ssrc);
}

void AMRtpClient::send_ack()
{
  send_thread_cmd(AM_CLIENT_CMD_ACK);
}

bool AMRtpClient::send_kill_client()
{
  AMRtpClientMsgKill killmsg(m_client->ssrc,
                             m_client->session_id,
                             m_client->proto_fd);
  int ret = 0;
  int count = 0;
  uint32_t sent_ret = 0;
  uint8_t *data = (uint8_t*)&killmsg;
  do {
    ret = write(m_ctrl_fd, data + sent_ret, (sizeof(killmsg) - sent_ret));
    if (AM_UNLIKELY(ret < 0)) {
      if (AM_LIKELY((errno != EAGAIN) &&
                    (errno != EWOULDBLOCK) &&
                    (errno != EINTR))) {
        PERROR("write");
        break;
      }
    } else {
      sent_ret += ret;
    }
  }while((++ count < 5) && ((sent_ret >= 0) && (sent_ret < sizeof(killmsg))));

  return (sent_ret == sizeof(killmsg));
}

bool AMRtpClient::send_thread_cmd(AM_CLIENT_CMD cmd)
{
  bool ret = true;
  char command[1] = {cmd};
  int count = 0;
  int send_ret = 0;

  while ((++count < 5) && (sizeof(command) !=
      (send_ret = write(RTP_CLI_W, command, sizeof(command))))) {
    if (AM_LIKELY((errno != EAGAIN) &&
                  (errno != EWOULDBLOCK) &&
                  (errno != EINTR))) {
      ret = false;
      PERROR("write");
      break;
    }
  }

  return (ret && (send_ret == sizeof(command)) && (count < 5));
}
