/*******************************************************************************
 * am_sip_ua_server.cpp
 *
 * History:
 *   2015-1-26 - [Shiming Dong] created file
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
#include "am_base_include.h"
#include "am_define.h"
#include "am_file.h"
#include "am_log.h"
#include "am_configure.h"

#include <sys/un.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "am_event.h"
#include "am_thread.h"
#include "am_mutex.h"
#include "am_audio_define.h"
#include "am_video_types.h"
#include "am_io.h"

#include "am_rtp_msg.h"
#include "am_sip_ua_server_config.h"
#include "am_sip_ua_server.h"
#include "am_sip_ua_server_protocol_data_structure.h"
#include "rtp.h"

#define  UNIX_SOCK_CTRL_R       m_ctrl_unix_fd[0]
#define  UNIX_SOCK_CTRL_W       m_ctrl_unix_fd[1]

#define  CMD_ABRT_ALL              'a'
#define  CMD_STOP_ALL              'e'

#ifdef BUILD_AMBARELLA_ORYX_CONF_DIR
#define SIP_CONF_DIR ((const char*)BUILD_AMBARELLA_ORYX_CONF_DIR"/apps")
#else
#define SIP_CONF_DIR ((const char*)"/etc/oryx/apps")
#endif

AMSipUAServer *AMSipUAServer::m_instance = nullptr;
std::mutex AMSipUAServer::m_lock;
std::atomic<bool> AMSipUAServer::m_busy(false);

struct ServerCtrlMsg
{
  uint32_t code;
  uint32_t data;
};

static bool is_fd_valid(int fd)
{
  return (fd >= 0) && ((fcntl(fd, F_GETFD) != -1) || (errno != EBADF));
}

static int create_unix_socket_fd(const char *socket_name)
{
  socklen_t len = 0;
  sockaddr_un un;
  int fd = -1;
  do{
    if (AM_UNLIKELY((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)) {
      PERROR("unix domain socket");
      break;
    }

    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, socket_name);
    len = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);
    if (AM_UNLIKELY(AMFile::exists(socket_name))) {
      NOTICE("%s already exists! Remove it first!", socket_name);
      if (AM_UNLIKELY(unlink(socket_name) < 0)) {
        PERROR("unlink");
        close(fd);
        fd = -1;
        break;
      }
    }
    if (AM_UNLIKELY(!AMFile::create_path(AMFile::dirname(socket_name).
                                         c_str()))) {
      ERROR("Failed to create path %s",
            AMFile::dirname(socket_name).c_str());
      close(fd);
      fd = -1;
      break;
    }

    if (AM_UNLIKELY(bind(fd, (struct sockaddr*)&un, len) < 0)) {
      PERROR("unix domain bind");
      close(fd);
      fd = -1;
      break;
    }
  }while(0);
  return fd;
}

static int unix_socket_conn(int fd, const char *server_name)
{
  bool isok = true;
  do {
    socklen_t len = 0;
    sockaddr_un un;
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    if (AM_UNLIKELY(!server_name)) {
      ERROR("Server name is invalid");
      isok = false;
      break;
    }
    strcpy(un.sun_path, server_name);
    len = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);
    if (AM_UNLIKELY(connect(fd, (struct sockaddr*)&un, len) < 0)) {
      if (AM_LIKELY(errno == ENOENT)) {
        NOTICE("RTP Muxer is not working, wait and try again...");
      } else {
        PERROR("connect");
      }
      isok = false;
      break;
    } else {
      INFO("SIP.unix connect to %s successfully!", server_name);
    }
  } while(0);

  if (AM_UNLIKELY(!isok)) {
    if (AM_LIKELY(fd >= 0)) {
      close(fd);
      fd = -1;
    }
  }

  return fd;
}

bool AMSipClientInfo::init(bool jitter_buffer, uint16_t frames_remain)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY(pipe(m_ctrl_fd) < 0)) {
      PERROR("pipe");
      ret = false;
      break;
    }
    if (jitter_buffer) {
      m_jitterbuf = new AMJitterBuffer(frames_remain);

      if (m_jitterbuf && (AM_JB_OK != m_jitterbuf->jb_create())) {
        ERROR("Failed to create jitter buffer!");
        ret = false;
        break;
      }
      AM_JB_CONFIG config = {
          .max_jitterbuf = DEFAULT_MAX_JITTERBUFFER,
          .resync_threshold = DEFAULT_RESYNCH_THRESHOLD,
          .max_contig_interp = DEFAULT_MAX_CONTIG_INTERP,
          .target_extra = 0,
      };
      m_jitterbuf->jb_setconf(&config);
      INFO("Jitter Buffer was enabled!");
    }
  } while(0);

  return ret;
}

bool AMSipClientInfo::setup_rtp_client_socket()
{
  bool ret = true;
  do {
    struct sockaddr_in addr4 = {0};
    struct sockaddr *addr = nullptr;
    socklen_t socket_len = sizeof(struct sockaddr);

    if (AM_UNLIKELY(m_recv_rtp_fd >= 0)) {
      close(m_recv_rtp_fd);
      m_recv_rtp_fd = -1;
    }
    m_recv_rtp_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (AM_LIKELY(m_recv_rtp_fd >= 0)) {
      addr4.sin_family = AF_INET;
      addr4.sin_addr.s_addr = htonl(INADDR_ANY);
      addr4.sin_port = htons(m_media_info[AM_RTP_MEDIA_AUDIO].rtp_port);
      addr = (struct sockaddr*)&addr4;
      socket_len = sizeof(addr4);
    } else {
      PERROR("socket");
      ret = false;
      break;
    }
    if(AM_LIKELY(m_recv_rtp_fd >= 0)) {
      int recv_buf = 4 * SOCKET_BUFFER_SIZE;
      if (AM_UNLIKELY(setsockopt(m_recv_rtp_fd, SOL_SOCKET, SO_RCVBUF,
                                 &recv_buf, sizeof(int)) < 0)) {
        ERROR("Failed to set socket receive buf!");
        ret = false;
        break;
      }
      int reuse = 1;
      if (AM_UNLIKELY(setsockopt(m_recv_rtp_fd, SOL_SOCKET, SO_REUSEADDR,
                                 &reuse, sizeof(reuse)) < 0)) {
        ERROR("Failed to set addr reuse!");
        ret = false;
        break;
      }
      int flag = fcntl(m_recv_rtp_fd, F_GETFL, 0);
      flag |= O_NONBLOCK;
      if (AM_UNLIKELY(fcntl(m_recv_rtp_fd, F_SETFL, flag) != 0)) {
        PERROR("fcntl");
        ret = false;
        break;
      }
      if (AM_UNLIKELY(bind(m_recv_rtp_fd, addr, socket_len) != 0)) {
        close(m_recv_rtp_fd);
        m_recv_rtp_fd = -1;
        ERROR("Failed to bind socket");
        ret = false;
        break;
      }
    } else {
      ERROR("set up udp socket error.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

void AMSipClientInfo::start_process_rtp_thread(void *data)
{
  ((AMSipClientInfo*)data)->process_rtp_thread();
}

void AMSipClientInfo::process_rtp_thread()
{
  uint32_t count = 0;
  bool connected = false;
  while((m_send_rtp_uds_fd < 0) && (count < 10)) {
    m_send_rtp_uds_fd =  create_unix_socket_fd(m_uds_client_name.c_str());
    if (AM_LIKELY(m_send_rtp_uds_fd > 0)) {
      count ++;
      m_send_rtp_uds_fd = unix_socket_conn(m_send_rtp_uds_fd,
                                           m_uds_server_name.c_str());
      if (AM_LIKELY(m_send_rtp_uds_fd > 0)) {
        INFO("Connect to uds server successfully!");
        connected = true;
        break;
      } else {
        NOTICE("Connect to uds server error! try %d times", count);
        usleep(50000);
      }
    } else {
      ERROR("Create uds client fd error!");
    }
  }

  fd_set rfds;
  fd_set zfds;
  int max_fd =-1;
  uint32_t pre_ts = 0;
  uint16_t pre_sn = 0;
  bool get_frame_len = false;

  if (AM_LIKELY(connected && (m_recv_rtp_fd > 0))) {
    m_running = true;
    FD_ZERO(&zfds);
    FD_SET(m_recv_rtp_fd, &zfds);
    FD_SET(SOCK_CTRL_R, &zfds);
    max_fd = AM_MAX(m_recv_rtp_fd, SOCK_CTRL_R);
  } else if (connected){
    ERROR("Setup recv rpt socket error!");
  } else {
    ERROR("Conected to uds server error!");
  }

  while (m_running) {
    rfds = zfds;
    int ret = select(max_fd +1, &rfds, nullptr, nullptr, nullptr);
    if (AM_UNLIKELY(ret <= 0)) {
      PERROR("select");
      continue;
    }
    if (FD_ISSET(SOCK_CTRL_R, &rfds)) {
      ServerCtrlMsg msg = {0};
      ssize_t ret = am_read(SOCK_CTRL_R, &msg, sizeof(msg),
                            DEFAULT_IO_RETRY_COUNT);
      if (ret != sizeof(msg)) {
        PERROR("Failed to read msg from SOCK_CTRL_R!");
      } else {
        switch(msg.code) {
          case CMD_ABRT_ALL:
            ERROR("Fatal error occurred, rtp_thread abort!");
            break;
          case CMD_STOP_ALL:
            m_running = false;
            NOTICE("process rtp thread received stop signal!");
            break;
          default:
            WARN("Unknown msg received!");
            break;
        }
      }
    } else if (AM_LIKELY(FD_ISSET(m_recv_rtp_fd, &rfds))) {
      uint8_t *recv_buf = new uint8_t[DEFAULT_RECV_RTP_PACKET_SIZE];
      int recv_len = recv(m_recv_rtp_fd, recv_buf + 8,
                          DEFAULT_RECV_RTP_PACKET_SIZE - 8, 0);
      if(AM_UNLIKELY(recv_len < 0)) {
        if((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
          delete []recv_buf;
          continue;
        } else {
          PERROR("recvfrom");
          m_running = false;
          delete []recv_buf;
          break;
        }
      }
      uint32_t send_size = (uint32_t)recv_len;

      recv_buf[0] = 'R';
      recv_buf[1] = 'T';
      recv_buf[2] = 'P';
      recv_buf[3] = ' ';
      recv_buf[4] = (send_size & 0xff000000) >> 24;
      recv_buf[5] = (send_size & 0x00ff0000) >> 16;
      recv_buf[6] = (send_size & 0x0000ff00) >> 8;
      recv_buf[7] = (send_size & 0x000000ff);

      if (nullptr != m_jitterbuf) { /* enable jitter buffer */
        AMRtpHeader *rtp_header = (AMRtpHeader*)(recv_buf + 8);

        if (!get_frame_len) {
          if ((0 == pre_ts) && (0 == pre_sn)) {
            pre_ts = ntohl(rtp_header->timestamp);
            pre_sn = ntohs(rtp_header->sequencenumber);
          } else {
            if ((pre_sn + 1) == ntohs(rtp_header->sequencenumber)) {
              m_audio_frame_len = ntohl(rtp_header->timestamp) - pre_ts;
              get_frame_len = true;
              NOTICE("Get audio frame length %d", m_audio_frame_len);
            } else {
              pre_ts = 0;
              pre_sn = 0;
            }
          }
        }
        uint32_t timestamp = ntohl(rtp_header->timestamp);
        uint32_t now = timestamp + 100;

        AM_JB_STATE ret_jb;
        ret_jb = m_jitterbuf->jb_put(recv_buf, AM_JB_TYPE_VOICE,
                                     m_audio_frame_len, timestamp, now);
        switch(ret_jb) {
          case AM_JB_OK: {
            /* Frame added */
          }break;
          case AM_JB_DROP: {
            ERROR("Drop this frame immediately!");
            delete []recv_buf;
          }break;
          case AM_JB_SCHED: {
            NOTICE("Frame added into head of queue.");
          }break;
          default : {
            WARN("This should not happened in jb_put!");
            delete []recv_buf;
          }break;
        }

        while (m_jitterbuf->jb_ready_to_get()) {
          AM_JB_FRAME frame_out;
          frame_out.data = nullptr;
          uint32_t interpl = m_jitterbuf->jb_next();
          ret_jb = m_jitterbuf->jb_get(&frame_out, interpl, m_audio_frame_len);
          switch(ret_jb) {
            case AM_JB_OK: {
              /* deliver the frame */
              if (AM_LIKELY(nullptr != frame_out.data)) {
                uint32_t pkt_size =
                  ((((uint8_t*)frame_out.data)[4] & 0x000000ff) << 24) |
                  ((((uint8_t*)frame_out.data)[5] & 0x000000ff) << 16) |
                  ((((uint8_t*)frame_out.data)[6] & 0x000000ff) << 8)  |
                  ((((uint8_t*)frame_out.data)[7] & 0x000000ff));
                ssize_t ret = am_write(m_send_rtp_uds_fd, frame_out.data,
                                       pkt_size + 8, DEFAULT_IO_RETRY_COUNT);
                if (AM_UNLIKELY(ret != ssize_t(pkt_size + 8))) {
                  PERROR("Send RTP data to uds error!");
                }
                delete [](uint8_t*)frame_out.data;
              }
            }break;
            case AM_JB_DROP: {
              NOTICE("Here's an audio frame you should just drop");
              delete [](uint8_t*)frame_out.data;
            }break;
            case AM_JB_NOFRAME: {
              NOTICE("There's no frame scheduled for this time");
            }break;
            case AM_JB_INTERP: {
              NOTICE("There was a lost frame, interpolate next one");
              if(AM_LIKELY(frame_out.data == nullptr)) {
                frame_out.data = m_jitterbuf->jb_get_frame()->data;
                uint32_t pkt_size =
                  ((((uint8_t*)frame_out.data)[4] & 0x000000ff) << 24) |
                  ((((uint8_t*)frame_out.data)[5] & 0x000000ff) << 16) |
                  ((((uint8_t*)frame_out.data)[6] & 0x000000ff) << 8)  |
                  ((((uint8_t*)frame_out.data)[7] & 0x000000ff));
                ssize_t ret = am_write(m_send_rtp_uds_fd, frame_out.data,
                                       pkt_size + 8, DEFAULT_IO_RETRY_COUNT);
                 if (AM_UNLIKELY(ret != ssize_t(pkt_size + 8))) {
                   PERROR("Send RTP data to uds error!");
                 }
              }
            }break;
            case AM_JB_EMPTY: {
              NOTICE("The jitter buffer is empty");
            }break;
            default : {
              ERROR("Invalid return value of jb_get");
            }break;
          }
        }
      } else { /* Not enable jitter buffer */
        ssize_t ret = am_write(m_send_rtp_uds_fd, recv_buf,
                               send_size + 8, DEFAULT_IO_RETRY_COUNT);
         if (AM_UNLIKELY(ret != ssize_t(send_size + 8))) {
           PERROR("Send RTP data to uds error!");
         }
         delete []recv_buf;
      }
    }
  } /* while */

  if (m_recv_rtp_fd > 0) {
    close(m_recv_rtp_fd);
    m_recv_rtp_fd = -1;
  }
  if (m_send_rtp_uds_fd > 0) {
    close(m_send_rtp_uds_fd);
    m_send_rtp_uds_fd = -1;
  }

}

AMSipClientInfo::~AMSipClientInfo()
{
  while (m_running) {
    INFO("Sending stop to thread %s", m_rtp_thread->name());
    if (AM_LIKELY(SOCK_CTRL_W >= 0)) {
      ServerCtrlMsg msg = {
        .code = CMD_STOP_ALL,
        .data = 0
      };
      ssize_t ret = am_write(SOCK_CTRL_W, &msg, sizeof(msg),
                             DEFAULT_IO_RETRY_COUNT);
      if (AM_UNLIKELY(ret != (ssize_t)sizeof(msg))) {
        ERROR("Failed to send stop command! %s", strerror(errno));
      }
      usleep(30000);
    }
  }
  if (SOCK_CTRL_W >= 0) {
    close(SOCK_CTRL_W);
  }
  if (SOCK_CTRL_R >= 0) {
    close(SOCK_CTRL_R);
  }
  AM_DESTROY(m_rtp_thread);
  if (m_recv_rtp_fd > 0) {
    close(m_recv_rtp_fd);
    m_recv_rtp_fd = -1;
  }
  if (m_send_rtp_uds_fd > 0) {
    close(m_send_rtp_uds_fd);
    m_send_rtp_uds_fd = -1;
  }
  if (AMFile::exists(m_uds_client_name.c_str())) {
    if (AM_UNLIKELY(unlink(m_uds_client_name.c_str()) < 0)) {
      PERROR("unlink");
    }
  }
  if (nullptr != m_jitterbuf) {
    AM_JB_FRAME frame;
    while (m_jitterbuf->jb_getall(&frame) == AM_JB_OK) {
        delete [](uint8_t*)frame.data;
    }
    m_jitterbuf->jb_destroy();
    delete m_jitterbuf;
    m_jitterbuf = nullptr;
  }
}

AMISipUAServerPtr AMISipUAServer::create()
{
  return AMSipUAServer::get_instance();
}

AMISipUAServer* AMSipUAServer::get_instance()
{
  m_lock.lock();
  if (AM_LIKELY(!m_instance)) {
    m_instance = new AMSipUAServer();
    if (AM_UNLIKELY(m_instance && (!m_instance->construct()))) {
      delete m_instance;
      m_instance = nullptr;
    }
  }
  m_lock.unlock();
  return m_instance;
}

bool AMSipUAServer::start()
{
  if (AM_LIKELY(!m_run)) {
    m_run = (init_sip_ua_server() && start_connect_unix_thread()
             && start_sip_ua_server_thread());
    m_event->signal();
  }
  return m_run;
}

void AMSipUAServer::stop()
{
  if (AM_LIKELY(m_sip_config->is_register && m_context)) {
    register_to_server(0); /* log out */
  }
  if (AM_LIKELY(m_run && m_server_thread)) {
    INFO("Sending stop to %s", m_server_thread->name());
  }
  m_run = false; /* exit sip_ua_server thread */
  hang_up_all();

  while (m_unix_sock_run) {
    INFO("Sending stop to %s", m_sock_thread->name());
    if (AM_LIKELY(UNIX_SOCK_CTRL_W >= 0)) {
      ServerCtrlMsg msg = {
        .code = CMD_STOP_ALL,
        .data = 0
      };
      ssize_t ret = am_write(UNIX_SOCK_CTRL_W, &msg, sizeof(msg), 10);
      if (AM_UNLIKELY(ret != (ssize_t)sizeof(msg))) {
        ERROR("Failed to send stop command! %s", strerror(errno));
      }
      usleep(30000);
    }
  }
  AM_DESTROY(m_server_thread);
  AM_DESTROY(m_sock_thread);
  if (AM_LIKELY(m_context)) {
    eXosip_quit(m_context);
    m_context = nullptr;
  }
}

uint32_t AMSipUAServer::version()
{
  return SIP_LIB_VERSION;
}

bool AMSipUAServer::set_sip_registration_parameter(AMSipRegisterParameter
                                                   *regis_para)
{
  bool ret = true;
  if (AM_UNLIKELY(nullptr == regis_para)) {
    ret = false;
    ERROR("SIP registration parameter is nullptr!");
  } else {
    AMConfig *config = AMConfig::create(m_config_file.c_str());
    AMConfig &sip_config = *config;
    if (AM_LIKELY(regis_para->m_modify_flag &
        (1 << AMSipRegisterParameter::USERNAME_FLAG))) {
      sip_config["username"] = std::string(regis_para->m_username);
    }
    if (AM_LIKELY(regis_para->m_modify_flag &
        (1 << AMSipRegisterParameter::PASSWORD_FLAG))) {
      sip_config["password"] = std::string(regis_para->m_password);
    }
    if (AM_LIKELY(regis_para->m_modify_flag &
        (1 << AMSipRegisterParameter::SERVER_URL_FLAG))) {
      sip_config["server_url"] = std::string(regis_para->m_server_url);
    }
    if (AM_LIKELY(regis_para->m_modify_flag &
        (1 << AMSipRegisterParameter::SERVER_PORT_FLAG))) {
      sip_config["server_port"] = regis_para->m_server_port;
    }
    if (AM_LIKELY(regis_para->m_modify_flag &
        (1 << AMSipRegisterParameter::SIP_PORT_FLAG))) {
      sip_config["sip_port"] = regis_para->m_sip_port;
    }
    if (AM_LIKELY(regis_para->m_modify_flag &
        (1 << AMSipRegisterParameter::EXPIRES_FLAG))) {
      sip_config["expires"] = regis_para->m_expires;
    }
    if (AM_LIKELY(regis_para->m_modify_flag &
        (1 << AMSipRegisterParameter::IS_REGISTER_FLAG))) {
      sip_config["is_register"] = regis_para->m_is_register;
    }
    if (AM_LIKELY(regis_para->m_modify_flag)) {
      sip_config.save();
    }
    delete config;
    NOTICE("Rewrite the SIP registration parameters!");
  }
  /* Reread the sip configuration file */
  if (AM_UNLIKELY(!(m_sip_config = m_config->get_config(m_config_file)))) {
    ERROR("Reread the sip configuration file error!");
    ret = -1;
  } else {
    if (m_sip_config->is_register && regis_para->m_is_apply &&
        register_to_server(m_sip_config->expires)) {
      INFO("Re-register to SIP server successfully!");
    }
  }
  return ret;
}

bool AMSipUAServer::set_sip_config_parameter(AMSipConfigParameter
                                             *config_para)
{
  bool ret = true;
  if (AM_UNLIKELY(nullptr == config_para)) {
    ret = false;
    ERROR("SIP config parameter is nullptr!");
  } else {
    AMConfig *config = AMConfig::create(m_config_file.c_str());
    AMConfig &sip_config = *config;
    if (AM_LIKELY(config_para->m_modify_flag &
        (1 << AMSipConfigParameter::RTP_PORT_FLAG))) {
      sip_config["rtp_stream_port_base"] = config_para->m_rtp_stream_port_base;
    }
    if (AM_LIKELY(config_para->m_modify_flag &
        (1 << AMSipConfigParameter::MAX_CONN_NUM_FLAG))) {
      sip_config["max_conn_num"] = config_para->m_max_conn_num;
    }
    if (AM_LIKELY(config_para->m_modify_flag &
        (1 << AMSipConfigParameter::ENABLE_VIDEO_FLAG))) {
      sip_config["enable_video"] = config_para->m_enable_video;
    }
    if (AM_LIKELY(config_para->m_modify_flag)) {
      sip_config.save();
    }
    delete config;
    NOTICE("Rewrite the SIP config parameters!");
  }
  return ret;
}

bool AMSipUAServer::set_sip_media_priority_list(AMMediaPriorityList
                                                *media_prio)
{
  bool ret = true;
  if (AM_UNLIKELY(nullptr == media_prio || (0 == media_prio->m_media_num))) {
    ret = false;
    ERROR("SIP audio_priority list is nullptr!");
  } else {
    AMConfig *config = AMConfig::create(m_config_file.c_str());
    AMConfig &sip_config = *config;
    if (AM_LIKELY(sip_config["audio_priority_list"].exists())) {
      sip_config["audio_priority_list"].remove();
    }
    for(uint32_t pos = 0; pos < media_prio->m_media_num; ++pos) {
      sip_config["audio_priority_list"][pos] =
                      std::string(media_prio->m_media_list[pos]);
    }
    sip_config.save();
    delete config;
    NOTICE("Rewrite the SIP media priority list!");
  }
  return ret;
}

bool AMSipUAServer::initiate_sip_call(AMSipCalleeAddress *address)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY(m_busy)) {
      WARN("User is busy now, wait a moment!");
      break;
    } else {
      m_busy = true;
      signal(SIGALRM, AMSipUAServer::busy_status_timer);
      INFO("Enter busy status");
      alarm(BUSY_TIMER);
    }

    if (AM_UNLIKELY(nullptr == address)) {
      ret = false;
      ERROR("SIP callee address parameter is nullptr!");
      break;
    }
    delete_invalid_sip_client();

    char localip[128] = {0};
    char uac_sdp[2048] = {0};
    char source_call[64] = {0};
    char dest_call[64] = {0};
    osip_message_t *invite= nullptr;

    INFO("Initiate a call to %s", address->callee_address);

    /* Reread the sip configuration file */
    if (AM_UNLIKELY(!(m_sip_config = m_config->get_config(m_config_file)))) {
      ERROR("Reread the sip configuration file error!");
      ret = false;
      break;
    }

    INFO("Current connected client number is %u", m_connected_num);
    if (AM_UNLIKELY(m_connected_num >= m_sip_config->max_conn_num)) {
      WARN("Max connected number is reached!");
      break;
    } else if (0 == m_connected_num) {
      m_rtp_port = m_sip_config->rtp_stream_port_base;
    }
    uint16_t rtp_port = m_rtp_port;

    AMRtpClientMsgSDPS sdps_msg;
    sdps_msg.port[AM_RTP_MEDIA_AUDIO] = rtp_port;
    if (m_sip_config->enable_video) {
      sdps_msg.enable_video = true;
      sdps_msg.port[AM_RTP_MEDIA_VIDEO] = rtp_port + 2;
    }

    /* Send query SDPS message to RTP muxer*/
    if (AM_UNLIKELY(!send_rtp_control_data((uint8_t*)&sdps_msg,
                                           sizeof(sdps_msg)))) {
      ERROR("Send message to RTP muxer error!");
      ret = false;
      break;
    } else {
      INFO("Send SDPS messages to RTP muxer.");
      if (AM_LIKELY(m_event_sdps->wait(5000))) {
        NOTICE("RTP muxer has returned SDPS info!");
      } else {
        ERROR("Failed to get SDPS info from RTP muxer!");
        ret = false;
        break;
      }
    }

    if (is_str_start_with(address->callee_address, "sip:")) {
      strncpy(dest_call, address->callee_address, sizeof(dest_call));
    } else {
      snprintf(dest_call, sizeof(dest_call), "sip:%s", address->callee_address);
    }
    eXosip_guess_localip(m_context, AF_INET, localip, sizeof(localip));
    snprintf(source_call, sizeof(source_call), "sip:%s@%s:%d",
             m_sip_config->username.c_str(),
             localip, m_sip_config->sip_port);
    int result = eXosip_call_build_initial_invite(m_context, &invite,
                                dest_call, source_call, nullptr,
                                "This is a SIP call");
    if (result != 0) {
      ERROR("Intial INVITE failed!\n");
      ret = false;
      break;
    }

    snprintf(uac_sdp, 2048,
             "v=0\r\n"
             "o=Ambarella 1 1 IN IP4 %s\r\n"
             "s=Ambarella SIP Phone\r\n"
             "c=IN IP4 %s\r\n"
             "t=0 0\r\n"
             "%s",
             localip, localip, m_sdps.c_str());

    INFO("The UAC's SDP is\n%s",uac_sdp);

    osip_message_set_body(invite, uac_sdp, strlen(uac_sdp));
    osip_message_set_content_type(invite, "application/sdp");

    eXosip_lock(m_context);
    eXosip_call_send_initial_invite(m_context, invite);
    eXosip_unlock(m_context);

  }while(0);

  return ret;
}

bool AMSipUAServer::hangup_sip_call(AMSipHangupUsername *name)
{
  bool ret = true;
  bool find = false;
  std::string usernames;
  do {
    if (AM_UNLIKELY(nullptr == name)) {
      ret = false;
      ERROR("SIP hangup username is nullptr!");
      break;
    }
    if (is_str_equal(name->user_name, "disconnect_all")) {
      hang_up_all();
    } else {
      for (uint32_t i = 0; i < m_sip_client_que.size(); ++i) {
        usernames.append(" " + m_sip_client_que.front()->m_username);
        if (is_str_equal(m_sip_client_que.front()->m_username.c_str(),
                         name->user_name)) {
          hang_up(m_sip_client_que.front());
          m_sip_client_que.pop();
          m_connected_num = m_connected_num ? (m_connected_num - 1) : 0;
          find = true;
          break;
        } else {
          AMSipClientInfo *sip_client = m_sip_client_que.front();
          m_sip_client_que.pop();
          m_sip_client_que.push(sip_client);
        }
      }
      if (AM_UNLIKELY(!find)) {
        WARN("%s is not in the sip client queue! The queue now contains:%s",
             name->user_name, usernames.c_str());
      }
    }
  }while(0);

  return ret;
}

void AMSipUAServer::inc_ref()
{
  ++ m_ref_count;
}

void AMSipUAServer::release()
{
  if ((m_ref_count >= 0) && (--m_ref_count <= 0)) {
    NOTICE("Last reference of AMSipUAServer's object %p, release it!",
           m_instance);
    delete m_instance;
    m_instance = nullptr;
    m_ref_count = 0;
  }
}

void AMSipUAServer::hang_up_all()
{
  delete_invalid_sip_client();
  int queue_size = m_sip_client_que.size();
  NOTICE("Hang up all the %d calls!", queue_size);
  for (int i = 0; i < queue_size; ++i) {
    hang_up(m_sip_client_que.front());
    m_sip_client_que.pop();
    m_connected_num = m_connected_num ? (m_connected_num - 1) : 0;
  }
  if (AM_LIKELY(m_sip_client_que.empty())) {
    INFO("Hang up all calls successfully!");
  } else {
    WARN("Not hang up all calls successfully!");
  }
}

void AMSipUAServer::hang_up(AMSipClientInfo* sip_client)
{
  int ret = -1;
  do {
    if (AM_UNLIKELY(!sip_client)) {
      ERROR("sip_client is null");
      break;
    }
    /* send stop message to RTP muxer */
    if (sip_client->m_media_info[AM_RTP_MEDIA_AUDIO].is_alive) {
      AMRtpClientMsgKill audio_msg_kill(
         sip_client->m_media_info[AM_RTP_MEDIA_AUDIO].ssrc,
         0, m_unix_sock_fd);
      if (AM_UNLIKELY(!send_rtp_control_data((uint8_t* )&audio_msg_kill,
                                             sizeof(audio_msg_kill)))) {
        ERROR("Failed to send kill message to RTP Muxer to kill client"
              "SSRC: %08X!", sip_client->m_media_info[AM_RTP_MEDIA_AUDIO].ssrc);
      } else {
        INFO("Sent kill message to RTP Muxer to kill SSRC: %08X!",
             sip_client->m_media_info[AM_RTP_MEDIA_AUDIO].ssrc);
      }

      if (AM_LIKELY(m_event_kill->wait(5000))) {
        NOTICE("Client with SSRC: %08X is killed!",
             sip_client->m_media_info[AM_RTP_MEDIA_AUDIO].ssrc);
      } else {
        ERROR("Failed to receive response from RTP muxer!");
      }
    }
    if (sip_client->m_media_info[AM_RTP_MEDIA_VIDEO].is_alive) {
      AMRtpClientMsgKill video_msg_kill(
         sip_client->m_media_info[AM_RTP_MEDIA_VIDEO].ssrc,
         0, m_unix_sock_fd);
      if (AM_UNLIKELY(!send_rtp_control_data((uint8_t* )&video_msg_kill,
                                             sizeof(video_msg_kill)))) {
        ERROR("Failed to send kill message to RTP Muxer to kill client"
              "SSRC: %08X!", sip_client->m_media_info[AM_RTP_MEDIA_VIDEO].ssrc);
      } else {
        INFO("Sent kill message to RTP Muxer to kill SSRC: %08X!",
         sip_client->m_media_info[AM_RTP_MEDIA_VIDEO].ssrc);
      }

      if (AM_LIKELY(m_event_kill->wait(5000))) {
        NOTICE("Client with SSRC: %08X is killed!",
             sip_client->m_media_info[AM_RTP_MEDIA_VIDEO].ssrc);
      } else {
        ERROR("Failed to receive response from RTP muxer!");
      }
    }

    /* send BYE message to client before send stop to media service
     * in case the RTP write error through unix domain socket */
    eXosip_lock (m_context);
    ret = eXosip_call_terminate(m_context, sip_client->m_call_id,
                                sip_client->m_dialog_id);
    eXosip_unlock (m_context);
    if (0 != ret) {
      ERROR("hangup/terminate Failed!\n");
    } else {
      NOTICE("Hang up %s successfully!", sip_client->m_username.c_str());
    }

    /* Send stop msg to media service to release playback instance */
    AMMediaServiceMsgBlock stop_msg;
    stop_msg.msg_stop.base.type = AM_MEDIA_SERVICE_MSG_STOP;
    stop_msg.msg_stop.base.length = sizeof(AMMediaServiceMsgSTOP);
    stop_msg.msg_stop.playback_id = sip_client->m_playback_id;
    if (AM_UNLIKELY(!send_data_to_media_service(stop_msg))) {
      ERROR("Failed to send stop msg to media service.");
      break;
    } else {
      NOTICE("Send stop msg to media service successfully.");
    }

    delete sip_client;

  }while(0);
}

bool AMSipUAServer::register_to_server(int expires)
{
  bool ret = true;
  char identity[100];
  char registerer[100];

  snprintf(identity, sizeof(identity), "sip:%s@%s:%d",
           m_sip_config->username.c_str(),
           m_sip_config->server_url.c_str(),
           m_sip_config->sip_port);
  snprintf(registerer, sizeof(registerer), "sip:%s:%d",
           m_sip_config->server_url.c_str(),
           m_sip_config->server_port);
  do {
    osip_message_t *reg = nullptr;
    eXosip_lock (m_context);
    int regid = eXosip_register_build_initial_register(m_context, identity,
                                                       registerer,
                                                       nullptr, /* contact */
                                                       expires, &reg);
    eXosip_unlock (m_context);

    if(AM_UNLIKELY(regid < 1)) {
      ERROR("register build init Failed!\n");
      ret = false;
      break;
    }

    eXosip_lock (m_context);
    eXosip_clear_authentication_info(m_context);
    eXosip_add_authentication_info(m_context, m_sip_config->username.c_str(),
                                   m_sip_config->username.c_str(),
                                   m_sip_config->password.c_str(), "md5",
                                   nullptr);
    int reg_ret = eXosip_register_send_register(m_context, regid, reg);
    eXosip_unlock (m_context);

    if(AM_UNLIKELY(0 != reg_ret)) {
      ERROR("Register send failed!\n");
      ret = false;
      break;
    }
  } while(0);

  return ret;
}

void AMSipUAServer::busy_status_timer(int arg)
{
  if (AM_UNLIKELY(m_busy)) {
    m_busy = false;
    WARN("SIP connection has not established in %d seconds!", BUSY_TIMER);
  } else {
      INFO("Connection established!");
  }
}

void AMSipUAServer::delete_invalid_sip_client()
{
  uint32_t queue_size = m_sip_client_que.size();
  for (uint32_t i = 0; i < queue_size; ++i) {
    if (AM_UNLIKELY(!m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_AUDIO].
        is_alive && !m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_VIDEO].
        is_alive)) {
      delete m_sip_client_que.front();
      m_sip_client_que.pop();
      NOTICE("Delete an invalid sip client in the queue!");
    } else {
      AMSipClientInfo *sip_client = m_sip_client_que.front();
      m_sip_client_que.pop();
      m_sip_client_que.push(sip_client);
    }
  }
}

AMSipUAServer::~AMSipUAServer()
{
  if (AM_LIKELY(m_sip_config->is_register && m_context)) {
    register_to_server(0); /* log out */
  }
  m_run = false; /* exit sip_ua_server thread */

  if (AM_LIKELY(UNIX_SOCK_CTRL_R >= 0)) {
    close(UNIX_SOCK_CTRL_R);
  }
  if (AM_LIKELY(UNIX_SOCK_CTRL_W >= 0)) {
    close(UNIX_SOCK_CTRL_W);
  }
  AM_DESTROY(m_server_thread);
  AM_DESTROY(m_sock_thread);
  AM_DESTROY(m_event);
  AM_DESTROY(m_event_support);
  AM_DESTROY(m_event_sdp);
  AM_DESTROY(m_event_sdps);
  AM_DESTROY(m_event_ssrc);
  AM_DESTROY(m_event_kill);
  AM_DESTROY(m_event_playid);
  m_media_map.clear();
  if (AM_LIKELY(m_context)) {
    eXosip_quit(m_context);
    m_context = nullptr;
  }

}

bool AMSipUAServer::construct()
{
  bool state = true;
  do {
    m_config = new AMSipUAServerConfig();
    if (AM_UNLIKELY(!m_config)) {
      ERROR("Failed to create AMSipUAServerConfig object!");
      state = false;
      break;
    }
    m_config_file = std::string(SIP_CONF_DIR) + "/sip_ua_server.acs";
    m_sip_config = m_config->get_config(m_config_file);
    if (AM_UNLIKELY(!m_sip_config)) {
      ERROR("Failed to load sip_ua server config!");
      state = false;
      break;
    }
    if (AM_UNLIKELY(nullptr == (m_event = AMEvent::create()))) {
      ERROR("Failed to create event!");
      state = false;
      break;
    }
    if (AM_UNLIKELY(pipe(m_ctrl_unix_fd) < 0)) {
      PERROR("pipe");
      state = false;
      break;
    }
    if (AM_UNLIKELY(nullptr == (m_event_support = AMEvent::create()))) {
      ERROR("Failed to create event support!");
      state = false;
      break;
    }
    if (AM_UNLIKELY(nullptr == (m_event_sdp = AMEvent::create()))) {
      ERROR("Failed to create event sdp!");
      state = false;
      break;
    }
    if (AM_UNLIKELY(nullptr == (m_event_sdps = AMEvent::create()))) {
      ERROR("Failed to create event sdps!");
      state = false;
      break;
    }
    if (AM_UNLIKELY(nullptr == (m_event_ssrc = AMEvent::create()))) {
      ERROR("Failed to create event ssrc!");
      state = false;
      break;
    }
    if (AM_UNLIKELY(nullptr == (m_event_kill = AMEvent::create()))) {
      ERROR("Failed to create event kill!");
      state = false;
      break;
    }
    if (AM_UNLIKELY(nullptr == (m_event_playid = AMEvent::create()))) {
      ERROR("Failed to create event playid!");
      state = false;
      break;
    }
    set_media_map();
    signal(SIGPIPE, SIG_IGN);

  } while(0);

  return state;
}

uint32_t AMSipUAServer::get_random_number()
{
  struct timeval current = {0};
  gettimeofday(&current, nullptr);
  srand(current.tv_usec);
  return (rand() % ((uint32_t)-1));
}

bool AMSipUAServer::init_sip_ua_server()
{
  bool ret = true;
  TRACE_INITIALIZE(6, nullptr);
  do {
    m_context = eXosip_malloc();
    if (AM_UNLIKELY(eXosip_init(m_context))) {
      ERROR("Couldn't initialize eXosip!");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(eXosip_listen_addr(m_context, IPPROTO_UDP, nullptr,
                    m_sip_config->sip_port, AF_INET, 0))) {
      ERROR("Couldn't initialize SIP transport layer!");
      ret = false;
      break;
    }
    eXosip_set_user_agent(m_context, "Ambarella SIP Service");

    if (AM_LIKELY(m_sip_config->is_register)) {
      ret = register_to_server(m_sip_config->expires);
    }
  } while(0);

  return ret;
}

bool AMSipUAServer::get_supported_media_types()
{
  bool ret = true;
  /* communicate with RTP-muxer to get supported media type */
  AMRtpClientMsgSupport support_msg;
  support_msg.length = sizeof(support_msg);

  /* Send query support media type message to RTP muxer */
  if (AM_UNLIKELY(!send_rtp_control_data((uint8_t*)&support_msg,
                                         sizeof(support_msg)))) {
    ERROR("Send message to RTP muxer error!");
    ret = false;
  } else {
    INFO("Send query support media type message to RTP muxer.");
    if (AM_UNLIKELY(m_event_support->wait(5000))) {
      NOTICE("RTP muxer has returned support media type.");
    } else {
      ERROR("Failed to get support media type from RTP muxer!");
      ret = false;
    }
  }
  return ret;
}

bool AMSipUAServer::on_sip_call_invite_recv()
{
  bool ret = true;
  do {
    if (AM_UNLIKELY(m_busy)) {
      eXosip_lock(m_context);
      eXosip_call_send_answer(m_context, m_uac_event->tid, SIP_BUSY_HERE,
                              nullptr);
      eXosip_unlock(m_context);
      WARN("User is busy!");
      break;
    }
    INFO("Received a call from %s@%s:%s",
          m_uac_event->request->from->url->username,
          m_uac_event->request->from->url->host,
          m_uac_event->request->from->url->port);

    /* Reread the sip configuration file */
    if (AM_UNLIKELY(!(m_sip_config = m_config->get_config(m_config_file)))) {
      ERROR("Reread the sip configuration file error!");
      ret = false;
      break;
    }
    delete_invalid_sip_client();

    INFO("Current connected client number is %u", m_connected_num);
    if (AM_UNLIKELY(m_connected_num >= m_sip_config->max_conn_num)) {
      eXosip_lock(m_context);
      eXosip_call_send_answer(m_context, m_uac_event->tid, SIP_BUSY_HERE,
                              nullptr);
      eXosip_unlock(m_context);
      WARN("Refuse the call, user is busy!");
      break;
    } else if (0 == m_connected_num) {
      m_rtp_port = m_sip_config->rtp_stream_port_base;
    }

    eXosip_lock(m_context);
    eXosip_call_send_answer(m_context, m_uac_event->tid,
                            SIP_RINGING, nullptr);
    if(AM_UNLIKELY(0 != eXosip_call_build_answer(m_context,
                                                 m_uac_event->tid,
                                                 SIP_OK, &m_answer))) {
      eXosip_call_send_answer(m_context, m_uac_event->tid,
                              SIP_DECLINE, nullptr);
      ERROR("error build answer!\n");
      eXosip_unlock(m_context);
      ret = false;
      break;
    }
    eXosip_unlock(m_context);

    /* create a sip client */
    AMSipClientInfo *sip_client = new AMSipClientInfo(m_uac_event->did,
                                                      m_uac_event->cid);
    if (AM_UNLIKELY(nullptr == sip_client)) {
      ERROR("Create sip client error!");
      ret = false;
      break;
    } else if (AM_UNLIKELY(!sip_client->init(m_sip_config->jitter_buffer,
                                        m_sip_config->frames_remain_in_jb))) {
      ERROR("Init sip client error!");
      delete sip_client;
      ret = false;
      break;
    }
    /* Request SDP(for UAS) */
    sdp_message_t     *msg_req = nullptr;
    sdp_connection_t  *audio_con_req = nullptr;
    sdp_media_t       *audio_md_req  = nullptr;
    sdp_connection_t  *video_con_req = nullptr;
    sdp_media_t       *video_md_req  = nullptr;

    /* parse the SDP message */
    eXosip_lock(m_context);
    msg_req = eXosip_get_remote_sdp(m_context, m_uac_event->did);
    eXosip_unlock(m_context);

    char *str_sdp = nullptr;
    sdp_message_to_str(msg_req, &str_sdp);
    INFO("The caller's SDP message is: \n%s", str_sdp);
    osip_free(str_sdp);

    for(int i = 0; i < msg_req->m_medias.nb_elt; ++i) {
      if (is_str_equal("audio",
          ((sdp_media_t*)osip_list_get(&msg_req->m_medias, i))->m_media)) {
        audio_con_req = eXosip_get_audio_connection(msg_req);
        audio_md_req  = eXosip_get_audio_media(msg_req);
        sip_client->m_media_info[AM_RTP_MEDIA_AUDIO].is_supported = true;
        INFO("Audio is supported by the caller.");
      } else if (is_str_equal("video",
          ((sdp_media_t*)osip_list_get(&msg_req->m_medias, i))->m_media)) {
          video_con_req = eXosip_get_video_connection(msg_req);
          video_md_req  = eXosip_get_video_media(msg_req);
          sip_client->m_media_info[AM_RTP_MEDIA_VIDEO].is_supported = true;
          INFO("Video is supported by the caller.");
        }
    }

    if (AM_UNLIKELY(!get_supported_media_types())){
      delete sip_client;
      sdp_message_free(msg_req);
      ret = false;
      break;
    }

    /* create audio RTP session info for communication */
    AMRtpClientMsgInfo client_audio_info;
    AMRtpClientInfo &audio_info = client_audio_info.info;
    if(AM_LIKELY(sip_client->m_media_info[AM_RTP_MEDIA_AUDIO].is_supported)) {
      /* TODO: Add IPv6 support */
      audio_info.client_ip_domain = AM_IPV4;/* default */
      audio_info.tcp_channel_rtp  = 0xff;   /* default UDP mode */
      audio_info.tcp_channel_rtcp = 0xff;
      audio_info.send_tcp_fd = false;
      audio_info.port_srv_rtp  = m_rtp_port++;
      audio_info.port_srv_rtcp = m_rtp_port++;
      strcpy(audio_info.media, select_media_type(m_media_type,audio_md_req));

      sockaddr_in uac_audio_addr;
      uac_audio_addr.sin_family = AF_INET;
      uac_audio_addr.sin_addr.s_addr = inet_addr(audio_con_req->c_addr);
      uac_audio_addr.sin_port = htons(atoi(audio_md_req->m_port));
      memcpy(&audio_info.udp.rtp.ipv4, &uac_audio_addr,
             sizeof(uac_audio_addr));
      audio_info.udp.rtp.ipv4.sin_port = htons(atoi(audio_md_req->m_port));

      memcpy(&audio_info.udp.rtcp.ipv4, &uac_audio_addr,
             sizeof(uac_audio_addr));
      audio_info.udp.rtcp.ipv4.sin_port = htons(atoi(audio_md_req->m_port)
                                                + 1);

      memcpy(&audio_info.tcp.ipv4, &uac_audio_addr, sizeof(uac_audio_addr));
      client_audio_info.print();

      client_audio_info.session_id = get_random_number();
      sip_client->m_media_info[AM_RTP_MEDIA_AUDIO].session_id =
                                        client_audio_info.session_id;
      sip_client->m_media_info[AM_RTP_MEDIA_AUDIO].media =
                                        std::string(audio_info.media);
      sip_client->m_media_info[AM_RTP_MEDIA_AUDIO].rtp_port =
                                        audio_info.port_srv_rtp;
    }

    if (AM_UNLIKELY(m_rtp_port == m_sip_config->sip_port)) {
      m_rtp_port = m_rtp_port + 2;
    }

    /* create video RTP session info for communication */
    AMRtpClientMsgInfo client_video_info;
    AMRtpClientInfo &video_info = client_video_info.info;
    if(sip_client->m_media_info[AM_RTP_MEDIA_VIDEO].is_supported &&
        m_sip_config->enable_video) {
      /* TODO: Add IPv6 support */
      video_info.client_ip_domain = AM_IPV4;/* default */
      video_info.tcp_channel_rtp  = 0xff;   /* default UDP mode */
      video_info.tcp_channel_rtcp = 0xff;
      video_info.send_tcp_fd = false;
      video_info.port_srv_rtp  = m_rtp_port++;
      video_info.port_srv_rtcp = m_rtp_port++;
      strcpy(video_info.media, select_media_type(m_media_type,video_md_req));

      sockaddr_in uac_video_addr;
      uac_video_addr.sin_family = AF_INET;
      uac_video_addr.sin_addr.s_addr = inet_addr(video_con_req->c_addr);
      uac_video_addr.sin_port = htons(atoi(video_md_req->m_port));

      memcpy(&video_info.udp.rtp.ipv4, &uac_video_addr,
             sizeof(uac_video_addr));
      video_info.udp.rtp.ipv4.sin_port = htons(atoi(video_md_req->m_port));

      memcpy(&video_info.udp.rtcp.ipv4, &uac_video_addr,
             sizeof(uac_video_addr));
      video_info.udp.rtcp.ipv4.sin_port = htons(atoi(video_md_req->m_port)
                                                + 1);

      memcpy(&video_info.tcp.ipv4, &uac_video_addr, sizeof(uac_video_addr));
      client_video_info.print();

      client_video_info.session_id = get_random_number();
      sip_client->m_media_info[AM_RTP_MEDIA_VIDEO].session_id =
                                        client_video_info.session_id;
      sip_client->m_media_info[AM_RTP_MEDIA_VIDEO].media =
                                        std::string(video_info.media);
      sip_client->m_media_info[AM_RTP_MEDIA_VIDEO].rtp_port =
                                        video_info.port_srv_rtp;

    }

    /*if not support the audio and video type then refuse the call */
    if (AM_UNLIKELY((is_str_equal(AM_MEDIA_UNSUPPORTED, audio_info.media) ||
                     (0 == strlen(audio_info.media))) &&
                     (is_str_equal(AM_MEDIA_UNSUPPORTED, video_info.media) ||
                     (0 == strlen(video_info.media))))) {
      eXosip_lock (m_context);
      eXosip_call_send_answer(m_context, m_uac_event->tid,
                              SIP_UNSUPPORTED_MEDIA_TYPE, nullptr);
      eXosip_unlock (m_context);
      WARN("Refuse the call, not supported media type!");
      delete sip_client;
      sdp_message_free(msg_req);
      break;
    }

    /* push sip_client into the sip client queue */
    if (AM_LIKELY(nullptr != sip_client)) {
      m_sip_client_que.push(sip_client);
    } else {
      sdp_message_free(msg_req);
      ERROR("Create sip client error!");
      ret = false;
      break;
    }

    if (AM_LIKELY(strlen(audio_info.media) &&
        (!is_str_equal(AM_MEDIA_UNSUPPORTED, audio_info.media)))) {
      if (AM_UNLIKELY(!send_rtp_control_data((uint8_t*)&client_audio_info,
                                             sizeof(client_audio_info)))) {
        ERROR("Send message to RTP muxer error!");
        sdp_message_free(msg_req);
        ret = false;
        break;
      } else {
        INFO("Send audio INFO messages to RTP muxer.");
        if (AM_LIKELY(m_event_ssrc->wait(5000))) {
          NOTICE("RTP muxer has returned SSRC info!");
        } else {
          ERROR("Failed to get SSRC info from RTP muxer!");
          sdp_message_free(msg_req);
          ret = false;
          break;
        }
      }
    } else {
      WARN("Audio type is not supported!");
    }

    if (strlen(video_info.media) &&
        (!is_str_equal(AM_MEDIA_UNSUPPORTED, video_info.media))) {
      if (AM_UNLIKELY(!send_rtp_control_data((uint8_t*)&client_video_info,
                                             sizeof(client_video_info)))) {
        ERROR("Send message to RTP muxer error!");
        sdp_message_free(msg_req);
        ret = false;
        break;
      } else {
        INFO("Send video INFO messages to RTP muxer.");
        if (AM_LIKELY(m_event_ssrc->wait(5000))) {
          NOTICE("RTP muxer has returned SSRC info!");
        } else {
          ERROR("Failed to get SSRC info from RTP muxer!");
          sdp_message_free(msg_req);
          ret = false;
          break;
        }
      }
    }

    char localip[128]  = {0};
    char uas_sdp[2048] = {0};
    eXosip_guess_localip(m_context, AF_INET, localip, 128);
    /* communicate with RTP-muxer to get media SDP info */
    AMRtpClientMsgSDP sdp_msg;
    sdp_msg.session_id = m_uac_event->did;/* use the did as session_id */
    char full_ip[256] = {0};
    if (m_sip_config->is_register && audio_con_req &&
        !is_str_equal(audio_con_req->c_addr, msg_req->o_addr)) {
      snprintf(full_ip, sizeof(full_ip), "%s:%s", "IPV4",
               audio_con_req->c_addr);
    } else {
      snprintf(full_ip, sizeof(full_ip), "%s:%s", "IPV4", localip);
    }
    strcpy(sdp_msg.media[AM_RTP_MEDIA_HOSTIP], full_ip);
    if(!is_str_equal(audio_info.media, AM_MEDIA_UNSUPPORTED) &&
       strlen(audio_info.media)) {
      strcpy(sdp_msg.media[AM_RTP_MEDIA_AUDIO], audio_info.media);
      sdp_msg.port[AM_RTP_MEDIA_AUDIO] = audio_info.port_srv_rtp;
    }
    if(!is_str_equal(video_info.media, AM_MEDIA_UNSUPPORTED) &&
       strlen(video_info.media)) {
      strcpy(sdp_msg.media[AM_RTP_MEDIA_VIDEO],video_info.media);
      sdp_msg.port[AM_RTP_MEDIA_VIDEO] = video_info.port_srv_rtp;
    }
    sdp_msg.length = sizeof(sdp_msg);

    /* Send query SDP message to RTP muxer*/
    if (AM_UNLIKELY(!send_rtp_control_data((uint8_t*)&sdp_msg,
                                           sizeof(sdp_msg)))) {
      ERROR("Send message to RTP muxer error!");
      sdp_message_free(msg_req);
      ret = false;
      break;
    } else {
      INFO("Send SDP messages to RTP muxer.");
      if (AM_LIKELY(m_event_sdp->wait(5000))) {
        NOTICE("RTP muxer has returned SDP info!");
      } else {
        ERROR("Failed to get SDP info from RTP muxer!");
        sdp_message_free(msg_req);
        ret = false;
        break;
      }
    }

    /* build and send local SDP message */
    snprintf(uas_sdp, 2048,
             "v=0\r\n"
             "o=Ambarella 1 1 IN IP4 %s\r\n"
             "s=Ambarella SIP UA Server\r\n"
             "i=Session Information\r\n"
            //  "c=IN IP4 %s\r\n"
             "t=0 0\r\n"
             "%s%s",
             localip,//localip,
             m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_AUDIO].
             sdp.empty() ? "m=audio 0 RTP/AVP 0\r\n" :
             m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_AUDIO].
             sdp.c_str(),
             m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_VIDEO].
             sdp.c_str());

    INFO("The UAS's SDP is \n%s",uas_sdp);

    eXosip_lock(m_context);
    osip_message_set_body(m_answer, uas_sdp, strlen(uas_sdp));
    osip_message_set_content_type(m_answer, "application/sdp");
    eXosip_unlock(m_context);

    eXosip_lock(m_context);
    eXosip_call_send_answer(m_context, m_uac_event->tid, SIP_OK, m_answer);
    eXosip_unlock(m_context);

    sdp_message_free(msg_req);

  } while(0);

  return ret;
}

bool AMSipUAServer::on_sip_call_ack_recv()
{
  bool ret = true;
  do {
    for (uint32_t i = 0; i < m_sip_client_que.size(); ++i) {
      if (m_sip_client_que.front()->m_dialog_id == m_uac_event->did) {
        if (m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_AUDIO].ssrc) {
          AMRtpClientMsgControl audio_ctrl_msg(AM_RTP_CLIENT_MSG_START,
         m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_AUDIO].ssrc, 0);
          if (AM_UNLIKELY(!send_rtp_control_data((uint8_t*)&audio_ctrl_msg,
                                                 sizeof(audio_ctrl_msg)))) {
            ERROR("Send control start msg to RTP muxer error: %s",
                  strerror(errno));
            ret = false;
            break;
          } else {
            INFO("Send audio start messages to RTP muxer.");
            m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_AUDIO].
                is_alive = true;
          }
        }
        if (m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_VIDEO].ssrc) {
          AMRtpClientMsgControl video_ctrl_msg(AM_RTP_CLIENT_MSG_START,
          m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_VIDEO].ssrc, 0);
          if (AM_UNLIKELY(!send_rtp_control_data((uint8_t*)&video_ctrl_msg,
                                                 sizeof(video_ctrl_msg)))) {
            ERROR("Send control start msg to RTP muxer error: %s",
                  strerror(errno));
            ret = false;
            break;
          } else {
            INFO("Send video start messages to RTP muxer.");
            m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_VIDEO].
                is_alive = true;
          }
        }
        m_sip_client_que.front()->m_username = std::string(
                           m_uac_event->request->from->url->username);
        m_connected_num++;
        INFO("The connected number is now %u.", m_connected_num);
        if (AM_UNLIKELY(m_rtp_port == m_sip_config->sip_port)) {
          m_rtp_port = m_rtp_port + 2;
        }
        INFO("The RTP port is %d", m_rtp_port);
        break;
      } else {
        AMSipClientInfo *sip_client = m_sip_client_que.front();
        m_sip_client_que.pop();
        m_sip_client_que.push(sip_client);
      }
    }
    /* Start to receive RTP packet and send to media service */
    if (is_str_equal(AM_MEDIA_UNSUPPORTED,
                     m_sip_client_que.front()->m_media_info
                     [AM_RTP_MEDIA_AUDIO].media.c_str())) {
      WARN("Audio is unsupported!");
      break;
    }
    AMMediaServiceMsgBlock uds_uri;
    if(AM_UNLIKELY(!parse_sdp(uds_uri.msg_uds,
   m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_AUDIO].sdp.c_str(),
   m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_AUDIO].sdp.size()))) {
      ERROR("Failed to parse sdp information.");
      ret = false;
      break;
    }
    uds_uri.msg_uds.base.type = AM_MEDIA_SERVICE_MSG_UDS_URI;
    uds_uri.msg_uds.base.length = sizeof(AMMediaServiceMsgUdsURI);
    uds_uri.msg_uds.playback_id = -1;
    /* Use base server name plus dialog id as UDS server name */
    m_sip_client_que.front()->m_uds_server_name =
        m_sip_config->sip_media_rtp_server_name.append(
        std::to_string(m_sip_client_que.front()->m_dialog_id)).c_str();
    /*Use base client name plus dialog id as UDS client name */
    m_sip_client_que.front()->m_uds_client_name =
        m_sip_config->sip_media_rtp_client_name.append(
        std::to_string(m_sip_client_que.front()->m_dialog_id)).c_str();
    strncpy(uds_uri.msg_uds.name,
            m_sip_client_que.front()->m_uds_server_name.c_str(),
            sizeof(uds_uri.msg_uds.name));
    if(AM_UNLIKELY(!send_data_to_media_service(uds_uri))) {
      ERROR("Failed to send uri data to media service.");
      ret = false;
      break;
    } else {
      NOTICE("Send uds uri to media service successfully.");
      if (AM_LIKELY(m_event_playid->wait(5000))) {
        NOTICE("Media service has returned playback id!");
      } else {
        ERROR("Failed to get playback id from media service!");
        ret = false;
        break;
      }
    }

    if(AM_UNLIKELY(!m_sip_client_que.front()->setup_rtp_client_socket())) {
      ERROR("Failed to setup rtp client socket!");
      ret = false;
      break;
    }

    m_sip_client_que.front()->m_rtp_thread = AMThread::create(
        (m_sip_client_que.front()->m_username +
        std::to_string(m_sip_client_que.front()->m_dialog_id)).c_str(),
        AMSipClientInfo::start_process_rtp_thread, m_sip_client_que.front());
    if (AM_UNLIKELY(!m_sip_client_que.front()->m_rtp_thread)) {
      ERROR("Failed to create rtp_thread!");
      m_sip_client_que.front()->m_rtp_thread = nullptr;
      ret = false;
      break;
    }
  } while(0);

  return ret;
}

bool AMSipUAServer::on_sip_call_closed_recv()
{
  bool ret = true;
  do {
    for (uint32_t i = 0; i < m_sip_client_que.size(); i++) {
      if (m_uac_event->did == m_sip_client_que.front()->m_dialog_id) {
        if (m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_AUDIO].
            is_alive) {
          AMRtpClientMsgKill audio_msg_kill(
             m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_AUDIO].ssrc,
             0, m_unix_sock_fd);
          if (AM_UNLIKELY(!send_rtp_control_data((uint8_t* )&audio_msg_kill,
                                                 sizeof(audio_msg_kill)))) {
            ERROR("Failed to send kill message to RTP Muxer to kill client"
                  "SSRC: %08X!",
             m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_AUDIO].ssrc);
            ret = false;
            break;
          } else {
            INFO("Sent kill message to RTP Muxer to kill "
                 "SSRC: %08X!",
             m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_AUDIO].ssrc);
          }

          if (AM_LIKELY(m_event_kill->wait(5000))) {
            NOTICE("Client with SSRC: %08X is killed!",
            m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_AUDIO].ssrc);
          } else {
            ERROR("Failed to receive response from RTP muxer!");
            ret = false;
          }
        }

        if (m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_VIDEO].
            is_alive) {
          AMRtpClientMsgKill video_msg_kill(
             m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_VIDEO].ssrc,
             0, m_unix_sock_fd);
          if (AM_UNLIKELY(!send_rtp_control_data((uint8_t* )&video_msg_kill,
                                                 sizeof(video_msg_kill)))) {
            ERROR("Failed to send kill message to RTP Muxer to kill client"
                  "SSRC: %08X!",
             m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_VIDEO].ssrc);
            ret = false;
            break;
          } else {
            INFO("Sent kill message to RTP Muxer to kill "
                "SSRC: %08X!",
             m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_VIDEO].ssrc);
          }

          if (AM_LIKELY(m_event_kill->wait(5000))) {
            NOTICE("Client with SSRC: %08X is killed!",
            m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_AUDIO].ssrc);
          } else {
            ERROR("Failed to receive response from RTP muxer!");
            ret = false;
          }
        }

        AMMediaServiceMsgBlock stop_msg;
        stop_msg.msg_stop.base.type = AM_MEDIA_SERVICE_MSG_STOP;
        stop_msg.msg_stop.base.length = sizeof(AMMediaServiceMsgSTOP);
        stop_msg.msg_stop.playback_id =
            m_sip_client_que.front()->m_playback_id;
        if (AM_UNLIKELY(!send_data_to_media_service(stop_msg))) {
          ERROR("Failed to send stop msg to media service.");
          ret = false;
          break;
        } else {
          NOTICE("Send stop msg to media service successfully.");
        }

        delete m_sip_client_que.front();
        m_sip_client_que.pop();
        NOTICE("Hanging up!");
        m_connected_num = m_connected_num ? (m_connected_num - 1) : 0;

        break; /* break for loop */
      } else {
        AMSipClientInfo *sip_client = m_sip_client_que.front();
        m_sip_client_que.pop();
        m_sip_client_que.push(sip_client);
      }
    }
  } while (0);

  return ret;
}

bool AMSipUAServer::on_sip_call_answered_recv()
{
  bool ret = true;
  do {
    INFO("The callee has answered!");
    sdp_message_t *msg_rsp = nullptr;
    sdp_connection_t *audio_con_rsp = nullptr;
    sdp_media_t *audio_md_rsp = nullptr;
    sdp_connection_t *video_con_rsp = nullptr;
    sdp_media_t *video_md_rsp = nullptr;

    eXosip_lock(m_context);
    msg_rsp = eXosip_get_sdp_info(m_uac_event->response);
    eXosip_unlock(m_context);
    char *str_sdp = nullptr;
    sdp_message_to_str(msg_rsp, &str_sdp);
    INFO("The callee's response SDP message is: \n%s", str_sdp);
    std::string tmp_sdp_str = std::string(str_sdp);
    osip_free(str_sdp);

    /* create a sip client */
    AMSipClientInfo *sip_client = new AMSipClientInfo(m_uac_event->did,
                                                      m_uac_event->cid);
    if(AM_UNLIKELY(nullptr == sip_client)) {
      sdp_message_free(msg_rsp);
      ERROR("Create sip client error!");
      ret = false;
      break;
    } else if (AM_UNLIKELY(!sip_client->init(m_sip_config->jitter_buffer,
                                        m_sip_config->frames_remain_in_jb))) {
      ERROR("Init sip client error!");
      delete sip_client;
      ret = false;
      break;
    }
    for(int i = 0; i < msg_rsp->m_medias.nb_elt; ++i) {
      if (is_str_equal("audio",
          ((sdp_media_t*)osip_list_get(&msg_rsp->m_medias, i))->m_media)) {
        audio_con_rsp = eXosip_get_audio_connection(msg_rsp);
        audio_md_rsp  = eXosip_get_audio_media(msg_rsp);
        sip_client->m_media_info[AM_RTP_MEDIA_AUDIO].is_supported = true;
        INFO("Audio is supported by the callee.");
      } else if (is_str_equal("video",
          ((sdp_media_t*)osip_list_get(&msg_rsp->m_medias, i))->m_media)) {
          video_con_rsp = eXosip_get_video_connection(msg_rsp);
          video_md_rsp  = eXosip_get_video_media(msg_rsp);
          sip_client->m_media_info[AM_RTP_MEDIA_VIDEO].is_supported = true;
          INFO("Video is supported by the callee.");
        }
    }
    if (AM_UNLIKELY(!get_supported_media_types())){
      delete sip_client;
      sdp_message_free(msg_rsp);
      ret = false;
      break;
    }
    /* create audio RTP session info for communication */
   AMRtpClientMsgInfo client_audio_info;
   AMRtpClientInfo &audio_info = client_audio_info.info;
   if(AM_LIKELY(sip_client->m_media_info[AM_RTP_MEDIA_AUDIO].is_supported)) {
     /* TODO: Add IPv6 support */
     audio_info.client_ip_domain = AM_IPV4;/* default */
     audio_info.tcp_channel_rtp  = 0xff;   /* default UDP mode */
     audio_info.tcp_channel_rtcp = 0xff;
     audio_info.send_tcp_fd = false;
     audio_info.port_srv_rtp  = m_rtp_port++;
     audio_info.port_srv_rtcp = m_rtp_port++;
     strcpy(audio_info.media,select_media_type(m_media_type,audio_md_rsp));

     sockaddr_in uac_audio_addr;
     uac_audio_addr.sin_family = AF_INET;
     uac_audio_addr.sin_addr.s_addr = inet_addr(audio_con_rsp->c_addr);
     uac_audio_addr.sin_port = htons(atoi(audio_md_rsp->m_port));
     memcpy(&audio_info.udp.rtp.ipv4, &uac_audio_addr,
            sizeof(uac_audio_addr));
     audio_info.udp.rtp.ipv4.sin_port = htons(atoi(audio_md_rsp->m_port));

     memcpy(&audio_info.udp.rtcp.ipv4, &uac_audio_addr,
            sizeof(uac_audio_addr));
     audio_info.udp.rtcp.ipv4.sin_port = htons(atoi(audio_md_rsp->m_port)
                                               + 1);
     memcpy(&audio_info.tcp.ipv4, &uac_audio_addr, sizeof(uac_audio_addr));
     client_audio_info.print();
     client_audio_info.session_id = get_random_number();
     sip_client->m_media_info[AM_RTP_MEDIA_AUDIO].session_id =
                                       client_audio_info.session_id;
     sip_client->m_media_info[AM_RTP_MEDIA_AUDIO].media =
                                       std::string(audio_info.media);
     sip_client->m_media_info[AM_RTP_MEDIA_AUDIO].rtp_port =
                                       audio_info.port_srv_rtp;
   }

   /* create video RTP session info for communication */
   AMRtpClientMsgInfo client_video_info;
   AMRtpClientInfo &video_info = client_video_info.info;
   if(sip_client->m_media_info[AM_RTP_MEDIA_VIDEO].is_supported &&
       m_sip_config->enable_video) {
     /* TODO: Add IPv6 support */
     video_info.client_ip_domain = AM_IPV4;/* default */
     video_info.tcp_channel_rtp  = 0xff;   /* default UDP mode */
     video_info.tcp_channel_rtcp = 0xff;
     video_info.send_tcp_fd = false;
     video_info.port_srv_rtp  = m_rtp_port++;
     video_info.port_srv_rtcp = m_rtp_port++;
     strcpy(video_info.media, select_media_type(m_media_type,video_md_rsp));

     sockaddr_in uac_video_addr;
     uac_video_addr.sin_family = AF_INET;
     uac_video_addr.sin_addr.s_addr = inet_addr(video_con_rsp->c_addr);
     uac_video_addr.sin_port = htons(atoi(video_md_rsp->m_port));
     memcpy(&video_info.udp.rtp.ipv4, &uac_video_addr,
            sizeof(uac_video_addr));
     video_info.udp.rtp.ipv4.sin_port = htons(atoi(video_md_rsp->m_port));

     memcpy(&video_info.udp.rtcp.ipv4, &uac_video_addr,
            sizeof(uac_video_addr));
     video_info.udp.rtcp.ipv4.sin_port = htons(atoi(video_md_rsp->m_port)
                                               + 1);
     memcpy(&video_info.tcp.ipv4, &uac_video_addr, sizeof(uac_video_addr));
     client_video_info.print();
     client_video_info.session_id = get_random_number();
     sip_client->m_media_info[AM_RTP_MEDIA_VIDEO].session_id =
                                       client_video_info.session_id;
     sip_client->m_media_info[AM_RTP_MEDIA_VIDEO].media =
                                       std::string(video_info.media);
     sip_client->m_media_info[AM_RTP_MEDIA_VIDEO].rtp_port =
                                       video_info.port_srv_rtp;
   }

   /* push sip_client into the sip client queue */
   if (AM_LIKELY(nullptr != sip_client)) {
     m_sip_client_que.push(sip_client);
   } else {
     sdp_message_free(msg_rsp);
     ERROR("Create sip client error!");
     ret = false;
     break;
   }
   /* if UAS returns more than one payload type in 200 OK, re-INVITE */
   if (AM_UNLIKELY(audio_md_rsp && (audio_md_rsp->m_payloads.nb_elt > 1))) {
     WARN("%d kinds of supported payload types are received, re-INVITE.",
          audio_md_rsp->m_payloads.nb_elt);

     char localip[128]  = {0};
     char re_uac_sdp[2048] = {0};
     eXosip_guess_localip(m_context, AF_INET, localip, 128);
     /* communicate with RTP-muxer to get media SDP info */
     AMRtpClientMsgSDP sdp_msg;
     sdp_msg.session_id = m_uac_event->did;/* use the did as session_id */
     char full_ip[256] = {0};
     if (m_sip_config->is_register &&
         !is_str_equal(audio_con_rsp->c_addr, msg_rsp->o_addr)) {
       snprintf(full_ip, sizeof(full_ip), "%s:%s", "IPV4",
                audio_con_rsp->c_addr);
     } else {
       snprintf(full_ip, sizeof(full_ip), "%s:%s", "IPV4", localip);
     }
     strcpy(sdp_msg.media[AM_RTP_MEDIA_HOSTIP], full_ip);
     if(!is_str_equal(audio_info.media, AM_MEDIA_UNSUPPORTED) &&
        strlen(audio_info.media)) {
       strcpy(sdp_msg.media[AM_RTP_MEDIA_AUDIO], audio_info.media);
       sdp_msg.port[AM_RTP_MEDIA_AUDIO] = audio_info.port_srv_rtp;
     }
     if(!is_str_equal(video_info.media, AM_MEDIA_UNSUPPORTED) &&
        strlen(video_info.media)) {
       strcpy(sdp_msg.media[AM_RTP_MEDIA_VIDEO],video_info.media);
       sdp_msg.port[AM_RTP_MEDIA_VIDEO] = video_info.port_srv_rtp;
     }
     sdp_msg.length = sizeof(sdp_msg);

     /* Send query SDP message to RTP muxer*/
     if (AM_UNLIKELY(!send_rtp_control_data((uint8_t*)&sdp_msg,
                                            sizeof(sdp_msg)))) {
       ERROR("Send message to RTP muxer error!");
       sdp_message_free(msg_rsp);
       ret = false;
       break;
     } else {
       INFO("Send SDP messages to RTP muxer.");
       if (AM_LIKELY(m_event_sdp->wait(5000))) {
         NOTICE("RTP muxer has returned SDP info!");
       } else {
         ERROR("Failed to get SDP info from RTP muxer!");
         sdp_message_free(msg_rsp);
         ret = false;
         break;
       }
     }

     /* build and send local SDP message */
     snprintf(re_uac_sdp, 2048,
              "v=0\r\n"
              "o=Ambarella 1 1 IN IP4 %s\r\n"
              "s=Ambarella SIP UA Server\r\n"
              "i=Session Information\r\n"
            //  "c=IN IP4 %s\r\n"
              "t=0 0\r\n"
              "%s%s",
              localip,//localip,
              m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_AUDIO].
              sdp.c_str(),
              m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_VIDEO].
              sdp.c_str());

     INFO("The re-UAC's SDP is \n%s",re_uac_sdp);

     /* Send ACK to confirm this SIP session */
     osip_message_t *ack = nullptr;
     eXosip_lock(m_context);
     eXosip_call_build_ack(m_context, m_uac_event->did, &ack);
     eXosip_call_send_ack(m_context, m_uac_event->did, ack);
     eXosip_unlock(m_context);

     /* Send re-INVITE method */
     osip_message_t *re_invite = nullptr;
     eXosip_lock(m_context);
     eXosip_call_build_request(m_context, m_uac_event->did, "INVITE",
                               &re_invite);
     osip_message_set_body(re_invite, re_uac_sdp, strlen(re_uac_sdp));
     osip_message_set_content_type(re_invite, "application/sdp");

     eXosip_call_send_request(m_context, m_uac_event->did, re_invite);
     eXosip_unlock(m_context);

     sdp_message_free(msg_rsp);
     break;
   }

   if (AM_LIKELY(strlen(audio_info.media) &&
       (!is_str_equal(AM_MEDIA_UNSUPPORTED, audio_info.media)))) {
     if (AM_UNLIKELY(!send_rtp_control_data((uint8_t*)&client_audio_info,
                                            sizeof(client_audio_info)))) {
       ERROR("Send message to RTP muxer error!");
       sdp_message_free(msg_rsp);
       ret = false;
       break;
     } else {
       INFO("Send audio INFO messages to RTP muxer.");
       if (AM_LIKELY(m_event_ssrc->wait(5000))) {
         NOTICE("RTP muxer has returned SSRC info!");
       } else {
         ERROR("Failed to get SSRC info from RTP muxer!");
         sdp_message_free(msg_rsp);
         ret = false;
         break;
       }
     }
   }
   if (strlen(video_info.media) &&
       (!is_str_equal(AM_MEDIA_UNSUPPORTED, video_info.media))) {
     if (AM_UNLIKELY(!send_rtp_control_data((uint8_t*)&client_video_info,
                                            sizeof(client_video_info)))) {
       ERROR("Send message to RTP muxer error!");
       sdp_message_free(msg_rsp);
       ret = false;
       break;
     } else {
       INFO("Send video INFO messages to RTP muxer.");
       if (AM_LIKELY(m_event_ssrc->wait(5000))) {
         NOTICE("RTP muxer has returned SSRC info!");
       } else {
         ERROR("Failed to get SSRC info from RTP muxer!");
         sdp_message_free(msg_rsp);
         ret = false;
         break;
       }
     }
   }

   /* Send ACK to confirm this SIP session */
   osip_message_t *ack = nullptr;
   eXosip_lock(m_context);
   eXosip_call_build_ack(m_context, m_uac_event->did, &ack);
   eXosip_call_send_ack(m_context, m_uac_event->did, ack);
   eXosip_unlock(m_context);

   /* Connection established, exit busy status */
   m_busy = false;

   for (uint32_t i = 0; i < m_sip_client_que.size(); ++i) {
     if (m_sip_client_que.front()->m_dialog_id == m_uac_event->did) {
       if (m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_AUDIO].ssrc) {
         AMRtpClientMsgControl audio_ctrl_msg(AM_RTP_CLIENT_MSG_START,
        m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_AUDIO].ssrc, 0);
         if (AM_UNLIKELY(!send_rtp_control_data((uint8_t*)&audio_ctrl_msg,
                                                sizeof(audio_ctrl_msg)))) {
           ERROR("Send control start msg to RTP muxer error: %s",
                 strerror(errno));
           ret = false;
           break;
         } else {
           INFO("Send audio start messages to RTP muxer.");
           m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_AUDIO].
               is_alive = true;
         }
       }
       if (m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_VIDEO].ssrc) {
         AMRtpClientMsgControl video_ctrl_msg(AM_RTP_CLIENT_MSG_START,
         m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_VIDEO].ssrc, 0);
         if (AM_UNLIKELY(!send_rtp_control_data((uint8_t*)&video_ctrl_msg,
                                                sizeof(video_ctrl_msg)))) {
           ERROR("Send control start msg to RTP muxer error: %s",
                 strerror(errno));
           ret = false;
           break;
         } else {
           INFO("Send video start messages to RTP muxer.");
           m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_VIDEO].
               is_alive = true;
         }
       }
       m_sip_client_que.front()->m_username = std::string(
                                 m_uac_event->request->to->url->username);
       m_connected_num++;
       INFO("The connected number is now %u.", m_connected_num);
       if (AM_UNLIKELY(m_rtp_port == m_sip_config->sip_port)) {
         m_rtp_port = m_rtp_port + 2;
       }
       INFO("The RTP port is %d", m_rtp_port);
       break;
     } else {
       AMSipClientInfo *sip_client = m_sip_client_que.front();
       m_sip_client_que.pop();
       m_sip_client_que.push(sip_client);
     }
   }
   sdp_message_free(msg_rsp);

   /* Start to receive RTP packet and send to media service */
   AMMediaServiceMsgBlock uds_uri;
   if(AM_UNLIKELY(!parse_sdp(uds_uri.msg_uds, tmp_sdp_str.c_str(),
                             tmp_sdp_str.size()))) {
     ERROR("Failed to parse sdp information.");
     ret = false;
     break;
   }
   uds_uri.msg_uds.base.type = AM_MEDIA_SERVICE_MSG_UDS_URI;
   uds_uri.msg_uds.base.length = sizeof(AMMediaServiceMsgUdsURI);
   uds_uri.msg_uds.playback_id = -1;
   /* Use base server name plus dialog id as UDS server name */
   m_sip_client_que.front()->m_uds_server_name =
       m_sip_config->sip_media_rtp_server_name.append(
       std::to_string(m_sip_client_que.front()->m_dialog_id)).c_str();
   /*Use base client name plus dialog id as UDS client name */
   m_sip_client_que.front()->m_uds_client_name =
       m_sip_config->sip_media_rtp_client_name.append(
       std::to_string(m_sip_client_que.front()->m_dialog_id)).c_str();
   strncpy(uds_uri.msg_uds.name,
           m_sip_client_que.front()->m_uds_server_name.c_str(),
           sizeof(uds_uri.msg_uds.name));
   if(AM_UNLIKELY(!send_data_to_media_service(uds_uri))) {
     ERROR("Failed to send uri data to media service.");
     ret = false;
     break;
   } else {
     NOTICE("Send uds uri to media service successfully.");
     if (AM_LIKELY(m_event_playid->wait(5000))) {
       NOTICE("Media service has returned playback id!");
     } else {
       ERROR("Failed to get playback id from media service!");
       ret = false;
       break;
     }
   }

   if(AM_UNLIKELY(!m_sip_client_que.front()->setup_rtp_client_socket())) {
     ERROR("Failed to setup rtp client socket!");
     ret = false;
     break;
   }

   m_sip_client_que.front()->m_rtp_thread = AMThread::create(
       (m_sip_client_que.front()->m_username +
       std::to_string(m_sip_client_que.front()->m_dialog_id)).c_str(),
       AMSipClientInfo::start_process_rtp_thread, m_sip_client_que.front());
   if (AM_UNLIKELY(!m_sip_client_que.front()->m_rtp_thread)) {
     ERROR("Failed to create rtp_thread!");
     m_sip_client_que.front()->m_rtp_thread = nullptr;
     ret = false;
     break;
   }
  } while (0);

  return ret;
}

bool AMSipUAServer::start_sip_ua_server_thread()
{
  bool ret = true;
  AM_DESTROY(m_server_thread);
  m_server_thread = AMThread::create("SIP.server", static_sip_ua_server_thread,
                                     this);
  if (AM_UNLIKELY(!m_server_thread)) {
    ERROR("Failed to create sip ua server thread!");
    ret = false;
  }
  return ret;
}

void AMSipUAServer::static_sip_ua_server_thread(void *data)
{
  ((AMSipUAServer*)data)->sip_ua_server_thread();
}

void AMSipUAServer::sip_ua_server_thread()
{
  m_event->wait();
  int32_t reg_remain = REG_TIMER;

  eXosip_lock(m_context);
  eXosip_automatic_action(m_context);
  eXosip_unlock(m_context);

  while (m_run) {
    m_uac_event = eXosip_event_wait(m_context, 0, WAIT_TIMER);

    if (AM_LIKELY(m_sip_config->is_register)) {
      reg_remain = reg_remain - WAIT_TIMER;
      if(AM_UNLIKELY(reg_remain < 0)) {
        eXosip_lock (m_context);
        /* automatic send register every 900s */
        eXosip_automatic_refresh(m_context);
        eXosip_unlock (m_context);
        reg_remain = REG_TIMER;
      }
    }

    if (AM_LIKELY(nullptr == m_uac_event)) {
      continue;
    }

    NOTICE(m_uac_event->textinfo);

    eXosip_lock(m_context);
    eXosip_default_action(m_context, m_uac_event); /* handle 401/407 */
    eXosip_unlock(m_context);

    switch(m_uac_event->type) {
      case EXOSIP_CALL_INVITE: { /* new INVITE method coming */
        if (AM_UNLIKELY(!on_sip_call_invite_recv())) {
          ERROR("Process invite sip call error!");
        }
      }break;
      case EXOSIP_CALL_ACK: {
        if (AM_UNLIKELY(!on_sip_call_ack_recv())) {
          ERROR("Process ack sip call error!");
        }
      }break;
      case EXOSIP_CALL_CLOSED: { /* a BYE was received for this call */
        if (AM_UNLIKELY(!on_sip_call_closed_recv())) {
          ERROR("Process closed sip call error!");
        }
      }break;
      case EXOSIP_MESSAGE_NEW: {
        if(AM_LIKELY(MSG_IS_MESSAGE(m_uac_event->request))) {
           osip_body_t *body = nullptr;
           if(AM_LIKELY(osip_message_get_body(m_uac_event->request, 0, &body)
               != -1)) {
             INFO("Get a message: \"%s\" from %s\n",body->body,
                  m_uac_event->request->from->url->username);
           }
        }
        eXosip_message_build_answer(m_context, m_uac_event->tid, SIP_OK,
                                    &m_answer);
        eXosip_message_send_answer(m_context, m_uac_event->tid, SIP_OK,
                                   m_answer);
      }break;
      case EXOSIP_CALL_CANCELLED: {
        NOTICE("Call has been cancelled");
      }break;
      case EXOSIP_CALL_PROCEEDING: {
        INFO("Proceeding...");
      }break;
      case EXOSIP_CALL_RINGING: {
        INFO("Ringing...");
      }break;
      case EXOSIP_CALL_ANSWERED: {
        if (AM_UNLIKELY(!on_sip_call_answered_recv())) {
          ERROR("Process answered sip call error!");
        }
      }break;
      case EXOSIP_CALL_NOANSWER: {
        WARN("No answer within the timeout");
      }break;
      case EXOSIP_CALL_REQUESTFAILURE:  /* 4xx received */
      case EXOSIP_CALL_SERVERFAILURE:   /* 5xx received */
      case EXOSIP_CALL_GLOBALFAILURE: { /* 6xx received */
        if (AM_LIKELY(m_uac_event->response->reason_phrase)) {
          WARN("%s",m_uac_event->response->reason_phrase);
        }
      }break;
      default :
        break;
    }
    eXosip_event_free(m_uac_event);
  }
}

bool AMSipUAServer::start_connect_unix_thread()
{
  bool ret = true;
  AM_DESTROY(m_sock_thread);
  m_sock_thread = AMThread::create("SIP.unix", static_unix_thread, this);
  if (AM_UNLIKELY(!m_sock_thread)) {
    ERROR("Failed to create connect unix thread!");
    ret = false;
  }
  return ret;
}

void AMSipUAServer::static_unix_thread(void *data)
{
  ((AMSipUAServer*)data)->connect_unix_thread();
}

void AMSipUAServer::connect_unix_thread()
{
  m_unix_sock_run = true;
  uint32_t unix_socket_create_cnt = 0;
  uint32_t media_service_sock_create_cnt = 0;

  while (m_unix_sock_run) {
    int retval      = -1;
    timeval *tm     = nullptr;
    timeval timeout = {0};
    int max_fd = -1;
    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(UNIX_SOCK_CTRL_R, &fdset);
    if((m_unix_sock_fd < 0) && (unix_socket_create_cnt < 5)) {
      m_unix_sock_fd = create_unix_socket_fd(
          m_sip_config->sip_unix_domain_name.c_str());
      unix_socket_create_cnt ++;
      if(m_unix_sock_fd >= 0) {
        m_unix_sock_fd = unix_socket_conn(m_unix_sock_fd, RTP_CONTROL_SOCKET);
        if(m_unix_sock_fd >= 0) {
          int send_bytes = -1;
          AMRtpClientMsgProto proto_msg;
          proto_msg.proto = AM_RTP_CLIENT_PROTO_SIP;

          if (AM_UNLIKELY((send_bytes = send(m_unix_sock_fd,
                                             &proto_msg,
                                             sizeof(proto_msg),
                                             0)) == -1)) {
            ERROR("Send message to RTP muxer error!");
            close(m_unix_sock_fd);
            m_unix_sock_fd = -1;
          } else {
            unix_socket_create_cnt = 0;
            INFO("Send %d bytes protocol messages to RTP muxer.", send_bytes);
          }
        } else {
          WARN("Failed to connect to rtp service.");
        }
      } else {
        WARN("Create unix domain socket error.");
      }
    }
    if((m_media_service_sock_fd < 0) && (media_service_sock_create_cnt < 5)) {
      m_media_service_sock_fd = create_unix_socket_fd(
          m_sip_config->sip_to_media_service_name.c_str());
      media_service_sock_create_cnt ++;
      if(m_media_service_sock_fd >= 0) {
        m_media_service_sock_fd = unix_socket_conn(m_media_service_sock_fd,
                                           MEDIA_SERVICE_UNIX_DOMAIN_NAME);
        if(m_media_service_sock_fd < 0) {
          WARN("Failed to connect to media service error.");
        } else {
          NOTICE("Connect to media service successfully.");
          /*When connect to media service successfully,
           * an ack msg should be sent to media service immediately*/
          AMMediaServiceMsgBlock msg;
          msg.msg_ack.base.length = sizeof(AMMediaServiceMsgACK);
          msg.msg_ack.base.type = AM_MEDIA_SERVICE_MSG_ACK;
          msg.msg_ack.proto = AM_MEDIA_SERVICE_CLIENT_PROTO_SIP;
          if(AM_UNLIKELY(!send_data_to_media_service(msg))) {
            ERROR("Failed to send ack msg to media service.");
            close(m_media_service_sock_fd);
            m_media_service_sock_fd = -1;
          } else {
            media_service_sock_create_cnt = 0;
            INFO("sip send an ack to media service successfully.");
          }
        }
      } else {
        WARN("Failed to create unix socket domain fd");
      }
    }
    if(m_unix_sock_fd < 0 || m_media_service_sock_fd < 0) {
      timeout.tv_sec = m_sip_config->sip_retry_interval / 1000;
      tm = &timeout;
    }
    if(is_fd_valid(m_unix_sock_fd)) {
      FD_SET(m_unix_sock_fd, &fdset);
    }
    if(is_fd_valid(m_media_service_sock_fd)) {
      FD_SET(m_media_service_sock_fd, &fdset);
    }
    max_fd = AM_MAX(AM_MAX(m_media_service_sock_fd, m_unix_sock_fd),
                    UNIX_SOCK_CTRL_R);
    retval = select((max_fd + 1), &fdset, nullptr, nullptr, tm);
    if (AM_LIKELY(retval > 0)) {
      if (FD_ISSET(UNIX_SOCK_CTRL_R, &fdset)) {
        ServerCtrlMsg msg = {0};
        ssize_t ret = am_read(UNIX_SOCK_CTRL_R, &msg, sizeof(msg),
                              DEFAULT_IO_RETRY_COUNT);
        if (ret != sizeof(msg)) {
          PERROR("Failed to read msg from SOCK_CTRL_R!");
        } else if (AM_LIKELY((msg.code == CMD_ABRT_ALL) ||
                             (msg.code == CMD_STOP_ALL))) {
          switch (msg.code) {
            case CMD_ABRT_ALL:
              ERROR("Fatal error occurred, SIP.unix abort!");
              break;
            case CMD_STOP_ALL:
              m_unix_sock_run = false;
              NOTICE("SIP.unix received stop signal!");
              break;
            default:
              break;
          }
          break; /* break while */
        }
      } else if (FD_ISSET(m_unix_sock_fd, &fdset)) {
        if(AM_UNLIKELY(!recv_rtp_control_data())) {
          NOTICE("Recv rtp control data error,"
              "close unix socket connection with RTP Muxer!");
          close(m_unix_sock_fd);
          m_unix_sock_fd = -1;
        }
      } else if(FD_ISSET(m_media_service_sock_fd, &fdset)) {
        AMMediaServiceMsgBlock recv_msg;
        AM_MEDIA_NET_STATE  recv_ret = AM_MEDIA_NET_OK;
        recv_ret = recv_data_from_media_service(recv_msg);
        switch(recv_ret) {
          case AM_MEDIA_NET_OK: {
            if (AM_UNLIKELY(!process_msg_from_media_service(&recv_msg))) {
              ERROR("Process msg from media service error!");
            }
          }break;
          case AM_MEDIA_NET_ERROR :
          case AM_MEDIA_NET_PEER_SHUTDOWN: {
            NOTICE("Close connection with media_service");
            close(m_media_service_sock_fd);
            m_media_service_sock_fd = -1;
          } break;
          case AM_MEDIA_NET_BADFD:
          default: break;
        }
      }
    } else if (retval < 0) {
      if (AM_LIKELY(errno != EINTR)) {
        PERROR("select");
      }
      if (AM_UNLIKELY(!is_fd_valid(m_media_service_sock_fd))) {
        close(m_media_service_sock_fd);
        m_media_service_sock_fd = -1;
      }
      if (AM_UNLIKELY(!is_fd_valid(m_unix_sock_fd))) {
        close(m_unix_sock_fd);
        m_unix_sock_fd = -1;
      }
    }
  } /* while */
  release_resource();
  if (AM_LIKELY(m_unix_sock_run)) {
    m_unix_sock_run = false;
    INFO("SIP.unix exits due to server abort!");
  } else {
    INFO("SIP.unix exits mainloop!");
  }

}

void AMSipUAServer::release_resource()
{
  if (AM_LIKELY(m_unix_sock_fd >= 0)) {
    close(m_unix_sock_fd);
    m_unix_sock_fd = -1;
  }
  if(AM_LIKELY(m_media_service_sock_fd >= 0)) {
    close(m_media_service_sock_fd);
    m_media_service_sock_fd = -1;
  }
  unlink(m_sip_config->sip_unix_domain_name.c_str());
  unlink(m_sip_config->sip_to_media_service_name.c_str());
}

bool AMSipUAServer::recv_rtp_control_data()
{
  bool isok = true;
  if (AM_LIKELY(m_unix_sock_fd >= 0)) {
    int read_ret = 0;
    int read_cnt = 0;
    uint32_t received = 0;
    AMRtpClientMsgBlock union_msg;

    do {
      read_ret = recv(m_unix_sock_fd,
                      ((uint8_t*)&union_msg) + received,
                      sizeof(AMRtpClientMsgBase) - received,
                      0);
      if (AM_LIKELY(read_ret > 0)) {
        received += read_ret;
      }
    } while ((++ read_cnt < 5) &&
             (((read_ret >= 0) &&
               ((size_t)read_ret < sizeof(AMRtpClientMsgBase))) ||
               ((read_ret < 0) && ((errno == EINTR) ||
                                   (errno == EWOULDBLOCK) ||
                                   (errno == EAGAIN)))));
    if (AM_LIKELY(received == sizeof(AMRtpClientMsgBase))) {
      AMRtpClientMsg *msg = (AMRtpClientMsg*)&union_msg;
      read_ret = 0;
      read_cnt = 0;
      while ((received < msg->length) && (5 > read_cnt ++) &&
             ((read_ret == 0) || ((read_ret < 0) && ((errno == EINTR) ||
                                                     (errno == EWOULDBLOCK) ||
                                                     (errno == EAGAIN))))) {
        read_ret = recv(m_unix_sock_fd,
                        ((uint8_t*) &union_msg) + received,
                        msg->length - received,
                        0);
        if (AM_LIKELY(read_ret > 0)) {
          received += read_ret;
        }
      }
      if (AM_LIKELY(received == msg->length)) {
        switch (msg->type) {
          case AM_RTP_CLIENT_MSG_SUPPORT: {
            m_media_type = union_msg.msg_support.support_media_map;
            m_event_support->signal();
          }break;
          case AM_RTP_CLIENT_MSG_SDP: {
            for (uint32_t i = 0; i < m_sip_client_que.size(); i++) {
              if ((uint32_t)m_sip_client_que.front()->m_dialog_id ==
                  union_msg.msg_sdp.session_id) {
                m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_AUDIO].sdp.
                    clear();
                m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_AUDIO].sdp =
                    std::string(union_msg.msg_sdp.media[AM_RTP_MEDIA_AUDIO]);

                m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_VIDEO].sdp.
                    clear();
                m_sip_client_que.front()->m_media_info[AM_RTP_MEDIA_VIDEO].sdp =
                    std::string(union_msg.msg_sdp.media[AM_RTP_MEDIA_VIDEO]);
                m_event_sdp->signal();
                break;
              } else {
                AMSipClientInfo *client_info = m_sip_client_que.front();
                m_sip_client_que.pop();
                m_sip_client_que.push(client_info);
              }
            }
          }break;
          case AM_RTP_CLIENT_MSG_SDPS: {
            m_sdps.clear();
            m_sdps = std::string(union_msg.msg_sdps.media_sdp);
            NOTICE("Get SDP from RTP muxer:\n%s",m_sdps.c_str());
            m_event_sdps->signal();
          }break;
          case AM_RTP_CLIENT_MSG_KILL: {
            NOTICE("Client with identify %08X, SSRC %08X is killed.",
                   union_msg.msg_kill.session_id,
                   union_msg.msg_kill.ssrc);
            if (AM_UNLIKELY(!union_msg.msg_kill.active_kill)) {
              WARN("The RTP session was killed passively!");
              bool is_found = false;
              for (uint32_t i = 0; i < m_sip_client_que.size(); ++i) {
                for (uint32_t j = 0; j < AM_RTP_MEDIA_NUM; ++j) {
                  if (m_sip_client_que.front()->m_media_info[j].ssrc
                      == union_msg.msg_ctrl.ssrc) {
                    m_sip_client_que.front()->m_media_info[j].is_alive = false;
                    if (!m_sip_client_que.front()->m_media_info
                        [AM_RTP_MEDIA_AUDIO].is_alive && !m_sip_client_que.
                        front()->m_media_info[AM_RTP_MEDIA_VIDEO].is_alive) {
                      NOTICE("Client is invalid, remove it from the queue!");
                      delete m_sip_client_que.front();
                      m_sip_client_que.pop();
                      m_connected_num = m_connected_num ? (m_connected_num - 1)
                                        : 0;
                      INFO("Current connected number is %u", m_connected_num);
                    }
                    is_found = true;
                    break;
                  }
                }
                if(is_found) {
                  break;
                } else {
                  AMSipClientInfo *client_info = m_sip_client_que.front();
                  m_sip_client_que.pop();
                  m_sip_client_que.push(client_info);
                }
              }
              break;
            }
            m_event_kill->signal();
          }break;
          case AM_RTP_CLIENT_MSG_SSRC: {
            bool is_found = false;
            AMRtpClientMsgControl &msg_ctrl = union_msg.msg_ctrl;
            for (uint32_t i = 0; i < m_sip_client_que.size(); ++i) {
              for (uint32_t j = 0; j < AM_RTP_MEDIA_NUM; ++j) {
                if (m_sip_client_que.front()->m_media_info[j].session_id ==
                    msg_ctrl.session_id) {
                  m_sip_client_que.front()->m_media_info[j].ssrc = msg_ctrl.ssrc;
                  NOTICE("get SSRC %X of %s",msg_ctrl.ssrc,
                         m_sip_client_que.front()->m_media_info[j].media.c_str());
                  is_found = true;
                  m_event_ssrc->signal();
                  break;
                }
              }
              if(is_found) {
                break;
              } else {
                AMSipClientInfo *client_info = m_sip_client_que.front();
                m_sip_client_que.pop();
                m_sip_client_que.push(client_info);
              }
            }
            if (AM_UNLIKELY(!is_found)) {
              WARN("No matched media was found in receiving SSRC!");
            }
          }break;
          case AM_RTP_CLIENT_MSG_CLOSE: {
            NOTICE("RTP Muxer is stopped, disconnect with it!");
            isok = false;
          }break;
          case AM_RTP_CLIENT_MSG_ACK:
          case AM_RTP_CLIENT_MSG_PROTO:
          case AM_RTP_CLIENT_MSG_INFO:
          case AM_RTP_CLIENT_MSG_START:
          case AM_RTP_CLIENT_MSG_STOP: break;
          default: {
            ERROR("Invalid message type received!");
          }break;
        }
      } else {
        ERROR("Failed to receive message(%u bytes), type: %u",
              msg->length, msg->type);
      }
    } else {
      if (AM_LIKELY((errno != EINTR) &&
                    (errno != EWOULDBLOCK) &&
                    (errno != EAGAIN))) {
        ERROR("Failed to receive message(recv: %s)!", strerror(errno));
        isok = false;
      }
    }
  }
  return isok;
}

bool AMSipUAServer::send_rtp_control_data(uint8_t *data, size_t len)
{
  bool ret = false;

  if (AM_LIKELY((m_unix_sock_fd >= 0) && data && (len > 0))) {
    int      send_ret     = -1;
    uint32_t total_sent   = 0;
    uint32_t resend_count = 0;
    do {
      send_ret = write(m_unix_sock_fd, data + total_sent, len - total_sent);
      if (AM_UNLIKELY(send_ret < 0)) {
        if (AM_LIKELY((errno != EAGAIN) &&
                      (errno != EWOULDBLOCK) &&
                      (errno != EINTR))) {
          if (AM_LIKELY(errno == ECONNRESET)) {
            WARN("RTP muxer closed connection while sending data!");
          } else {
            PERROR("send");
          }
          close(m_unix_sock_fd);
          m_unix_sock_fd = -1;
          break;
        }
      } else {
        total_sent += send_ret;
      }
      ret = (len == total_sent);
      if (AM_UNLIKELY(!ret)) {
        if (AM_LIKELY(++ resend_count >= m_sip_config->sip_send_retry_count)) {
          ERROR("Connecttion to RTP muxer is unstable!");
          break;
        }
        usleep(m_sip_config->sip_send_retry_interval * 1000);
      }
    }while (!ret);
  } else if (AM_LIKELY(m_unix_sock_fd < 0)) {
    ERROR("Connection to RTP muxer is already closed!");
  }

  return ret;
}

const char* AMSipUAServer::select_media_type(uint32_t uas_type,
                                             sdp_media_t *uac_type)
{
  const char *media_type = AM_MEDIA_UNSUPPORTED;
  if (is_str_equal("audio", uac_type->m_media)) {
    for (uint32_t i = 0; i < m_sip_config->audio_priority_list.size(); ++i) {
      /* if uas support this audio type */
      if(m_media_map.find(m_sip_config->audio_priority_list.at(i)) ==
          m_media_map.end()) {
        WARN("%s is not supported in media map!",
             m_sip_config->audio_priority_list.at(i).c_str());
        continue;
      }
      if ((1 << m_media_map[m_sip_config->audio_priority_list.at(i)]) &
          uas_type) {
        /* if uac support this audio type */
        if(is_str_equal("PCMA",
                        m_sip_config->audio_priority_list.at(i).c_str())) {
          uint8_t payload_num = uac_type->m_payloads.nb_elt;
          for (uint8_t pos = 0; pos < payload_num; pos++) {
            if(is_str_equal("8",(char*)osip_list_get(&uac_type->m_payloads,
                                                     pos))) {
              media_type = m_sip_config->audio_priority_list.at(i).c_str();
              NOTICE("choose audio type %s as the RTP payload type!",
                     media_type);
              break;
            }
          }
          if(!is_str_equal(AM_MEDIA_UNSUPPORTED,media_type)) break;
        } else if(is_str_equal(m_sip_config->audio_priority_list.at(i).c_str()
                               ,"PCMU")) {
          uint8_t payload_num = uac_type->m_payloads.nb_elt;
          for (uint8_t pos = 0; pos < payload_num; pos++) {
            if(is_str_equal("0",(char*)osip_list_get(&uac_type->m_payloads,
                                                     pos))) {
              media_type = m_sip_config->audio_priority_list.at(i).c_str();
              NOTICE("choose audio type %s as the RTP payload type!",
                     media_type);
              break;
            }
          }
          if(!is_str_equal(AM_MEDIA_UNSUPPORTED,media_type)) break;
        } else { /* dynamic payload types */
          uint8_t attr_num = uac_type->a_attributes.nb_elt;
          sdp_attribute_t *sdp_att = nullptr;
          for (uint8_t pos = 0; pos < attr_num; pos++) {
            sdp_att = (sdp_attribute_t *)osip_list_get(&uac_type->a_attributes
                                                       ,pos);
            if (is_str_equal("rtpmap", sdp_att->a_att_field)) {
              if (nullptr != strstr(sdp_att->a_att_value,
                std::string(" " + m_sip_config->audio_priority_list.at(i)
                            + "/").c_str())) {
                media_type = m_sip_config->audio_priority_list.at(i).c_str();
                NOTICE("choose audio type %s as the RTP payload type!",
                       media_type);
                break;
              }
            }
          }
          if(!is_str_equal(AM_MEDIA_UNSUPPORTED,media_type)) break;
        }
      }
    }
    if (is_str_equal(media_type, "PCMA") || is_str_equal(media_type,"PCMU")) {
      media_type = "g711-8k";
    } else if (is_str_equal(media_type, "G726-40") ||
               is_str_equal(media_type, "G726-32") ||
               is_str_equal(media_type, "G726-24") ||
               is_str_equal(media_type, "G726-16")) {
      media_type = "g726";
    } else if (is_str_equal(media_type, "mpeg4-generic")) {
      media_type = "aac";
    }
  } else if(is_str_equal("video", uac_type->m_media)) {
    for (uint32_t i = 0; i < m_sip_config->video_priority_list.size(); ++i) {
      /* if uas support this video type */
      if(m_media_map.find(m_sip_config->video_priority_list.at(i)) ==
          m_media_map.end()) {
        WARN("%s is not supported in media map!",
             m_sip_config->video_priority_list.at(i).c_str());
        continue;
      }
      if ((1 << m_media_map[m_sip_config->video_priority_list.at(i)]) &
          uas_type) {
        /* if uac support this video type */
        uint8_t attr_num = uac_type->a_attributes.nb_elt;
        sdp_attribute_t *sdp_att = nullptr;
        for (uint8_t pos = 0; pos < attr_num; pos++) {
          sdp_att = (sdp_attribute_t *)osip_list_get(&uac_type->a_attributes,
                                                     pos);
          if (is_str_equal("rtpmap", sdp_att->a_att_field)) {
            if (nullptr != strstr(sdp_att->a_att_value,
                std::string(" " + m_sip_config->video_priority_list.at(i)
                            + "/").c_str())) {
              media_type = m_sip_config->video_priority_list.at(i).c_str();
              NOTICE("choose video type %s as the RTP payload type!",
                     media_type);
              break;
            }
          }

        }
        if(!is_str_equal(AM_MEDIA_UNSUPPORTED,media_type)) break;
      }
    }
    if (is_str_equal(media_type, "H264")) {
      media_type = "video1";
    }
  }

  return media_type;
}

void AMSipUAServer::set_media_map()
{
  m_media_map["PCMA"]          = AM_AUDIO_G711A;
  m_media_map["PCMU"]          = AM_AUDIO_G711U;
  m_media_map["G726-40"]       = AM_AUDIO_G726_40;
  m_media_map["G726-32"]       = AM_AUDIO_G726_32;
  m_media_map["G726-24"]       = AM_AUDIO_G726_24;
  m_media_map["G726-16"]       = AM_AUDIO_G726_16;
  m_media_map["mpeg4-generic"] = AM_AUDIO_AAC;
  m_media_map["opus"]          = AM_AUDIO_OPUS;
  m_media_map["H264"]          = AM_VIDEO_H264;
  m_media_map["H265"]          = AM_VIDEO_H265;
  m_media_map["JPEG"]          = AM_VIDEO_MJPEG;
}

bool AMSipUAServer::parse_sdp(AMMediaServiceMsgUdsURI& uri,
                              const char *audio_sdp, int32_t size)
{
  bool ret = true;
  std::string audio_type_indicator;
  const char *audio_fmt_indicator = "rtpmap:";
  const char *indicator_uri = nullptr;
  char *end_ptr = nullptr;

  do {
    indicator_uri = (const char*) memmem(audio_sdp, size,
                           audio_fmt_indicator, strlen(audio_fmt_indicator));
    int audio_type = -1;
    if(AM_LIKELY(indicator_uri)) {
      if(AM_LIKELY(strlen(indicator_uri) > strlen(audio_fmt_indicator))) {
        audio_type = strtol(indicator_uri + strlen(audio_fmt_indicator),
                            &end_ptr, 10);
        if(AM_LIKELY(audio_type > 0 &&  end_ptr)) {
          INFO("Get audio type from sdp, audio type = %d", audio_type);
        } else {
          ERROR("Strtol audio type error.");
          ret = false;
          break;
        }
      } else {
        ERROR("memmem audio format error.");
        ret = false;
        break;
      }
    } else {
      ERROR("Not find \"rtpmap:\" in the sdp: %s", audio_sdp);
      ret = false;
      break;
    }
    switch(audio_type) {
      case RTP_PAYLOAD_TYPE_PCMU:{
        audio_type_indicator = "PCMU";
      }break;
      case RTP_PAYLOAD_TYPE_PCMA: {
        audio_type_indicator = "PCMA";
      }break;
      case RTP_PAYLOAD_TYPE_OPUS: {
        audio_type_indicator = "opus";
      }break;
      case RTP_PAYLOAD_TYPE_AAC: {
        audio_type_indicator = "aac";
      }break;
      case RTP_PAYLOAD_TYPE_G726: {
        audio_type_indicator = "G726";
      }break;
      case RTP_PAYLOAD_TYPE_G726_16: {
        audio_type_indicator = "G726-16";
      }break;
      case RTP_PAYLOAD_TYPE_G726_24: {
        audio_type_indicator = "G726-24";
      }break;
      case RTP_PAYLOAD_TYPE_G726_32: {
        audio_type_indicator = "G726-32";
      }break;
      case RTP_PAYLOAD_TYPE_G726_40: {
        audio_type_indicator = "G726-40";
      }break;
      default :
        NOTICE("audio type is not supported.");
        ret = false;
        break;
    }
    if(AM_UNLIKELY(!ret)) {
      break;
    }
    uri.audio_type = AM_AUDIO_TYPE(m_media_map[audio_type_indicator.c_str()]);
    indicator_uri = (const char*) memmem(audio_sdp, size,
                                         audio_type_indicator.c_str(),
                                         audio_type_indicator.size());
    if(AM_LIKELY(indicator_uri)) {
      if(AM_LIKELY(strlen(indicator_uri) > audio_type_indicator.size())) {
        uri.sample_rate = strtol(indicator_uri +
                          audio_type_indicator.size() + 1, &end_ptr, 10);
        if(AM_LIKELY(uri.sample_rate > 0 &&  end_ptr)) {
          INFO("Get audio sample rate from sdp, sample rate = %d",
               uri.sample_rate);
        } else {
          ERROR("Strtol sample rate error.");
          ret = false;
          break;
        }
        indicator_uri = end_ptr + 1;
        uri.channel = strtol(indicator_uri, &end_ptr, 10);
        if(AM_LIKELY(uri.channel > 0 && end_ptr)) {
          INFO("Get audio channel from sdp, channel = %d", uri.channel);
        } else {
          ERROR("Strtol channel error.");
          ret = false;
          break;
        }
      } else {
        ERROR("memmem %s error.", audio_type_indicator.c_str());
        ret = false;
        break;
      }
    } else {
      ERROR("Not find: %s", audio_type_indicator.c_str());
      ret = false;
      break;
    }
  }while(0);
  return ret;
}

AM_MEDIA_NET_STATE AMSipUAServer::recv_data_from_media_service(
                               AMMediaServiceMsgBlock& service_msg)
{
  AM_MEDIA_NET_STATE ret = AM_MEDIA_NET_OK;
  int read_ret = 0;
  int read_cnt = 0;
  uint32_t received = 0;
  do {
    if(AM_UNLIKELY(m_media_service_sock_fd < 0)) {
      ret = AM_MEDIA_NET_BADFD;
      ERROR("fd is below zero.");
      break;
    }
    do{
      read_ret = recv(m_media_service_sock_fd,
                      ((uint8_t*)&service_msg) + received,
                      sizeof(AMMediaServiceMsgBase) - received, 0);
      if(AM_LIKELY(read_ret > 0)) {
        received += read_ret;
      }
    }while((++ read_cnt < 5) &&
        (((read_ret >= 0) && (read_ret < (int)sizeof(AMMediaServiceMsgBase))) ||
            ((read_ret < 0) && ((errno == EINTR) ||
                (errno == EWOULDBLOCK) ||
                (errno == EAGAIN)))));
    if(AM_LIKELY(received == sizeof(AMMediaServiceMsgBase))) {
      AMMediaServiceMsgBase *msg = (AMMediaServiceMsgBase*)&service_msg;
      if(msg->length ==  sizeof(AMMediaServiceMsgBase)) {
        break;
      }else {
        read_ret = 0;
        read_cnt = 0;
        while((received < msg->length) && (5 > read_cnt ++) &&
            ((read_ret == 0) || ((read_ret < 0) && ((errno == EINTR) ||
                (errno == EWOULDBLOCK) ||
                (errno == EAGAIN))))) {
          read_ret = recv(m_media_service_sock_fd,
                          ((uint8_t*)&service_msg) + received,
                          msg->length - received, 0);
          if(AM_LIKELY(read_ret > 0)) {
            received += read_ret;
          }
        }
      }
    }
    if(read_ret <= 0) {
      if(AM_LIKELY(errno == ECONNRESET)) {
        NOTICE("Media service closed its connection while reading data."
            "read_ret = %d, errno = %d", read_ret, errno);
        ret = AM_MEDIA_NET_PEER_SHUTDOWN;
        break;
      } else {
        if(AM_LIKELY(errno == EBADF)) {
          ret = AM_MEDIA_NET_BADFD;
        } else {
          ret = AM_MEDIA_NET_ERROR;
        }
        ERROR("Failed to receive message(%d): %s!", errno, strerror(errno));
        break;
      }
    }
  }while(0);
  return ret;
}

bool AMSipUAServer::process_msg_from_media_service(AMMediaServiceMsgBlock
                                                   *service_msg)
{
  bool ret = true;
  do {
    AMMediaServiceMsgBase *msg_base = (AMMediaServiceMsgBase*)service_msg;
    switch(msg_base->type) {
      case AM_MEDIA_SERVICE_MSG_ACK: {
        INFO("Recv ACK msg from media service.");
      }break;
      case AM_MEDIA_SERVICE_MSG_PLAYBACK_ID: {
        AMMediaServiceMsgPlaybackID *play_id =
            (AMMediaServiceMsgPlaybackID*)service_msg;
        m_sip_client_que.front()->m_playback_id = play_id->playback_id;
        INFO("Recv playback id %d from media service.", play_id->playback_id);
        m_event_playid->signal();
      }break;
      default :
        break;
    }
  } while(0);

  return ret;
}

bool AMSipUAServer::send_data_to_media_service(AMMediaServiceMsgBlock& send_msg)
{
  bool ret = true;
  do {
    if(!is_fd_valid(m_media_service_sock_fd)) {
      ERROR("m_media_service_sock_fd is not valid. "
            "Can not send data to media service.");
      ret = false;
      break;
    }
    AMMediaServiceMsgBase *base = (AMMediaServiceMsgBase*)&send_msg;
    ssize_t send_ret = am_write(m_media_service_sock_fd, &send_msg,
                                base->length,
                                DEFAULT_IO_RETRY_COUNT);
    if (AM_UNLIKELY(send_ret != ssize_t(base->length))) {
      PERROR("Failed to send data to media service!");
      ret = false;
      break;
    }

  } while(0);

  return ret;
}
