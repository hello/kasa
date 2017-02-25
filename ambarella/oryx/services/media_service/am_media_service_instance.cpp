/*******************************************************************************
 * am_media_service_instance.cpp
 *
 * History:
 *   2015-1-20 - [ccjing] created file
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
#include "am_log.h"

#include "am_io.h"
#include "am_file.h"
#include "am_thread.h"
#include "am_record_msg.h"
#include "am_playback_msg.h"
#include "am_media_service_instance.h"
#include "am_api_media.h"
#include "am_image_quality_data.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

#define SERVICE_CTRL_READ       m_service_ctrl[0]
#define SERVICE_CTRL_WRITE      m_service_ctrl[1]
#define MAX_CONNECTION_NUMBER   AM_MEDIA_SERVICE_CLIENT_PROTO_NUM

static const char* proto_type_to_string(AM_MEDIA_SERVICE_CLIENT_PROTO proto)
{
  const char* name = "";
  switch (proto) {
    case AM_MEDIA_SERVICE_CLIENT_PROTO_UNKNOWN:
      name = "unknown";
      break;
    case AM_MEDIA_SERVICE_CLIENT_PROTO_SIP:
      name = "sip-service";
      break;
    default:
      break;
  }
  return name;
}

static bool is_fd_valid(int fd)
{
  return (fd >= 0) && ((fcntl(fd, F_GETFD) != -1) || (errno != EBADF));
}

AMMediaService* AMMediaService::create(media_callback media_callback)
{
  AMMediaService* result = new AMMediaService();
  if(AM_UNLIKELY(result && !result->init(media_callback))) {
    ERROR("Failed to create media service.");
    delete result;
    result = nullptr;
  }
  return result;
}

bool AMMediaService::start_media()
{
  AUTO_MEM_LOCK(m_lock);
  do {
    if (AM_UNLIKELY(m_is_started)) {
      NOTICE("Already started!");
      break;
    }
    m_socket_thread = AMThread::create("AMMediaService",
                                       socket_thread_entry, this);
    if(AM_UNLIKELY(!m_socket_thread)) {
      ERROR("Failed to create socket thread.");
      m_is_started = false;
      break;
    }
    if (m_record != nullptr) {
      if(AM_UNLIKELY(m_record->is_recording())) {
        NOTICE("Record is already started.");
      } else if(!m_record->start()) {
        ERROR("Failed to start record.");
        m_is_started = false;
        if (m_run) {
          ssize_t ret = am_write(SERVICE_CTRL_WRITE, "s", 1, 10);
          if (AM_UNLIKELY(ret != 1)) {
            ERROR("Failed to send stop command to control thread! %s",
                  strerror(errno));
          }
        }
        NOTICE("Thread %s is already running, destroy it!",
               m_socket_thread->name());
        m_socket_thread->destroy();
        m_socket_thread = nullptr;
        break;
      }
    } else {
      NOTICE("The stream record is not enabled in media service.");
    }
    m_is_started = true;
  }while(0);

  return m_is_started;
}

bool AMMediaService::stop_media()
{
  bool ret = true;

  if (AM_LIKELY(m_is_started)) {
    AUTO_MEM_LOCK(m_lock);
    if (m_run) {
      ssize_t ret = am_write(SERVICE_CTRL_WRITE, "s", 1, 10);
      if (AM_UNLIKELY(ret != 1)) {
        ERROR("Failed to send stop command to control thread! %s",
              strerror(errno));
      }
    }
    if(m_socket_thread) {
      m_socket_thread->destroy();
      m_socket_thread = nullptr;
    }
    if (m_record != nullptr) {
      if(AM_UNLIKELY(!m_record->stop())) {
        ERROR("Failed to stop record.");
        ret = false;
      }
    } else {
      NOTICE("The stream record is not enabled in media service");
    }
    if (m_playback != nullptr) {
      for (uint32_t i = 0; i < PLAYER_NUM; ++ i) {
        if(AM_UNLIKELY(!m_playback[i]->stop())) {
          ERROR("Failed to stop playback[%u].", i);
          ret = false;
        }
      }
    } else {
      NOTICE("The playback is not enabled in media service");
    }
    m_is_started = false;
  }

  return ret;
}

bool AMMediaService::send_event(AMEventStruct& event)
{
  bool ret = false;
  if (m_record != nullptr) {
    if (m_record->is_recording()) {
      if (m_record->is_ready_for_event(event)) {
        if (m_record->send_event(event)) {
          ret = true;
        } else {
          NOTICE("record send event failed.");
        }
      } else {
        NOTICE("record is not ready for event.");
      }
    } else {
      NOTICE("record is not recording.");
    }
    if (!ret) {
      ERROR("Failed to send event!");
    }
  } else {
    ERROR("The stream record is not enable in media service.");
    ret = false;
  }
  return ret;
}

void AMMediaService::destroy()
{
  delete this;
}

AM_PLAYER_INSTANCE_ID AMMediaService::get_valid_playback_id()
{
  AM_PLAYER_INSTANCE_ID id = PLAYER_NULL;
  if (m_playback != nullptr) {
    AUTO_MEM_LOCK(m_playback_lock);
    for (int32_t i = 0; i < PLAYER_NUM; ++ i) {
      if ((m_playback_ref[i] == 0) && (!m_playback[i]->is_paused()) &&
          (!m_playback[i]->is_playing())) {
        if (m_playback[i]->is_idle()) {
          if (!m_playback[i]->stop()) {
            ERROR("The playback status is idle, failed to stop it.");
            continue;
          }
        }
        id = AM_PLAYER_INSTANCE_ID(i);
        m_playback_ref[i] = 1;
        break;
      }
    }
  } else {
    ERROR("The Playback is not enabled in media service.");
  }
  return id;
}

AMIPlaybackPtr AMMediaService::get_playback_instance(AM_PLAYER_INSTANCE_ID id)
{
  AMIPlaybackPtr player = nullptr;
  do {
    if (m_playback == nullptr) {
      ERROR("Playback is not enabled in media service.");
      break;
    }
    if ((id <= PLAYER_NULL) || (id >= PLAYER_NUM)) {
      ERROR("The playback instance id is invalid.");
      break;
    }
    player = m_playback[id];
  } while(0);
  return player;
}

bool AMMediaService::release_playback_instance(AM_PLAYER_INSTANCE_ID id)
{
  bool ret = true;
  do {
    if (m_playback == nullptr) {
      ERROR("Playback is not enabled in media service.");
      ret = false;
      break;
    }
    if ((id <= PLAYER_NULL) || (id >= PLAYER_NUM)) {
      ERROR("The playback instance id is invalid.");
      ret = false;
      break;
    }
    AUTO_MEM_LOCK(m_playback_lock);
    if (!m_playback[id]->is_playing()) {
      INFO("Playback status is not playing now, stop it.");
      if (!m_playback[id]->stop()) {
        ERROR("Failed to stop playback instance[%d].", id);
        continue;
      }
    }
    m_playback_ref[id] = 0;
    NOTICE("release playback instance id is %d", id);
  } while(0);
  return ret;
}

AMIRecordPtr& AMMediaService::get_record_instance()
{
  if (m_record == nullptr) {
    ERROR("Stream record is not enabled in media service.");
  }
  return m_record;
}

AMMediaService::AMMediaService()
{}

AMMediaService::~AMMediaService()
{
  stop_media();
  m_record   = nullptr;
  if (m_playback != nullptr) {
    for (uint32_t i = 0; i < PLAYER_NUM; ++ i) {
      m_playback[i] = nullptr;
    }
    delete[] m_playback;
  }
  if (AM_LIKELY(SERVICE_CTRL_READ >= 0)) {
    close(SERVICE_CTRL_READ);
  }
  if (AM_LIKELY(SERVICE_CTRL_WRITE >= 0)) {
    close(SERVICE_CTRL_WRITE);
  }
}

bool AMMediaService::init(media_callback media_callback)
{
  bool ret = false;
  do{
    if(AM_UNLIKELY(pipe(m_service_ctrl) < 0)) {
      PERROR("pipe");
      break;
    }
    m_media_callback = media_callback;
    if(AM_UNLIKELY(!m_media_callback)) {
      NOTICE("Media callback function is not set!");
    }
#ifdef BUILD_AMBARELLA_ORYX_MEDIA_SERVICE_RECORD
    m_record = AMIRecord::create();
    if(AM_UNLIKELY(!m_record)) {
      ERROR("Failed to create record.");
      break;
    } else if(AM_UNLIKELY(!(m_record->init()))) {
      ERROR("Failed to initialize record.");
      break;
    }
    m_record->set_msg_callback(record_engine_callback, nullptr);
#endif
#ifdef BUILD_AMBARELLA_ORYX_MEDIA_SERVICE_PLAYBACK
    m_playback = new AMIPlaybackPtr[PLAYER_NUM];
    if (!m_playback) {
      ERROR("Failed to malloc memory in media service");
      break;
    }
    for (uint32_t i = 0; i < PLAYER_NUM; ++ i) {
      m_playback[i] = AMIPlayback::create();
      if(AM_UNLIKELY(!m_playback[i])) {
        ERROR("Failed to create playback[%u].", i);
        break;
      } else if(AM_UNLIKELY(!(m_playback[i]->init()))) {
        ERROR("Failed to initialize playback.");
        break;
      }
      m_playback[i]->set_msg_callback(playback_engine_callback, nullptr);
    }
#endif
    ret = true;
  }while(0);

  return ret;
}

void AMMediaService::record_engine_callback(AMRecordMsg& msg)
{
  switch (msg.msg) {
    case AM_RECORD_MSG_START_OK:
      NOTICE("Start OK!");
      break;
    case AM_RECORD_MSG_STOP_OK:
      NOTICE("Stop OK!");
      break;
    case AM_RECORD_MSG_ERROR:
      NOTICE("Error Occurred!");
      break;
    case AM_RECORD_MSG_ABORT:
      NOTICE("Recording Aborted!");
      break;
    case AM_RECORD_MSG_EOS:
      NOTICE("Recording Finished!");
      break;
    case AM_RECORD_MSG_TIMEOUT:
      NOTICE("Operation Timeout!");
      break;
    case AM_RECORD_MSG_OVER_FLOW:
    default:
      break;
  }
}

void AMMediaService::playback_engine_callback(AMPlaybackMsg& msg)
{
  switch (msg.msg) {
    case AM_PLAYBACK_MSG_START_OK:
      NOTICE("Start OK!");
      break;
    case AM_PLAYBACK_MSG_PAUSE_OK:
      NOTICE("Pauase OK!");
      break;
    case AM_PLAYBACK_MSG_STOP_OK:
      NOTICE("Stop OK!");
      break;
    case AM_PLAYBACK_MSG_ERROR:
      NOTICE("Error occurred!");
      break;
    case AM_PLAYBACK_MSG_ABORT:
      NOTICE("Abort due to error!");
      break;
    case AM_PLAYBACK_MSG_EOS:
      NOTICE("Playing back finished successfully!");
      break;
    case AM_PLAYBACK_MSG_TIMEOUT:
      NOTICE("Timeout!");
      break;
    case AM_PLAYBACK_MSG_EOF:
      NOTICE("One file is finished playing!");
      break;
    case AM_PLAYBACK_MSG_IDLE :
      NOTICE("Play list empty, enter idle status!");
      break;
    case AM_PLAYBACK_MSG_NULL:
    default:
      break;
  }
}

int AMMediaService::unix_socket_listen(const char* service_name)
{
  int listen_fd = -1;
  int sock_length = 0;
  struct sockaddr_un un;
  do {
    if(AM_UNLIKELY(((listen_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0))) {
      ERROR("Create unix socket domain error.");
      listen_fd = -1;
      break;
    }
    if(AMFile::exists(service_name)) {
      if(AM_UNLIKELY(unlink(service_name) < 0)) {
        PERROR("unlink");
        listen_fd = -1;
        break;
      }
    }
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    if(AM_UNLIKELY(strlen(service_name) >= 108)) {
      ERROR("service name is too long, the max length is 108.");
      close(listen_fd);
      listen_fd = -1;
      break;
    }
    if(AM_UNLIKELY(!AMFile::create_path(AMFile::dirname(service_name).c_str()))) {
      ERROR("Failed to create path socket domain in media service.");
      close(listen_fd);
      listen_fd = -1;
      break;
    }
    strcpy(un.sun_path, service_name);

    sock_length = offsetof(struct sockaddr_un, sun_path) + strlen(service_name);
    if(AM_UNLIKELY(bind(listen_fd, (struct sockaddr*)&un, sock_length) < 0)) {
      ERROR("Unix socket domain bind error, listen_fd = %d, "
          "service name :%s", listen_fd, service_name);
      PERROR("bind");
      close(listen_fd);
      listen_fd = -1;
      break;
    }
    if(AM_UNLIKELY(listen(listen_fd, MAX_CONNECTION_NUMBER) < 0)) {
      ERROR("Unix socket domain listen error.");
      close(listen_fd);
      listen_fd = -1;
      break;
    }
  }while(0);

  return listen_fd;
}
int AMMediaService::unix_socket_accept(int listen_fd)
{
  int child_fd = -1;
  struct sockaddr_un un;
  socklen_t socket_len = sizeof(un);
  do {
    if(AM_UNLIKELY((child_fd = accept(listen_fd, (struct sockaddr*)&un,
                                      &socket_len)) < 0)) {
      ERROR("Unix socket domain accept error.");
      child_fd = -1;
      break;
    }
  }while(0);

  return child_fd;
}

void AMMediaService::socket_thread_entry(void *p)
{
  ((AMMediaService*)p)->socket_thread_main_loop();
}

void AMMediaService::socket_thread_main_loop()
{
  fd_set fdset;
  int fd = -1;
  int max_fd = -1;
  m_run = true;
  if(AM_UNLIKELY(!create_resource())) {
    ERROR("Failed to create resource.");
    m_run = false;
  }

  while(m_run) {
    FD_ZERO(&fdset);
    FD_SET(SERVICE_CTRL_READ, &fdset);
    FD_SET(m_unix_socket_fd, &fdset);
    max_fd = AM_MAX(m_unix_socket_fd, SERVICE_CTRL_READ);
    for(uint32_t i = 0; i < AM_MEDIA_SERVICE_CLIENT_PROTO_NUM; ++ i) {
      if(is_fd_valid(m_client_proto_fd[i])) {
        FD_SET(m_client_proto_fd[i], &fdset);
        max_fd = AM_MAX(max_fd, m_client_proto_fd[i]);
      } else {
        m_client_proto_fd[i] = -1;
      }
    }
    if(AM_LIKELY(select(max_fd + 1, &fdset, nullptr, nullptr, nullptr) > 0)) {
      if(FD_ISSET(SERVICE_CTRL_READ, &fdset)) {
        char cmd[1] = { 0 };
        if(AM_UNLIKELY(read(SERVICE_CTRL_READ, cmd, sizeof(cmd)) < 0)) {
          PERROR("read");
          m_run = false;
          continue;
        } else {
          bool need_break = false;
          switch (cmd[0]) {
            case 's' :
              NOTICE("Thread %s received stop command!",
                     m_socket_thread->name());
              need_break = true;
              break;
            default:
              WARN("Unknown command %c", cmd[0]);
              break;
          }
          if (AM_LIKELY(need_break)) {
            break;
          }
        }
      }
      if(FD_ISSET(m_unix_socket_fd, &fdset)) {
        fd = unix_socket_accept(m_unix_socket_fd);
        if(AM_UNLIKELY(fd < 0)) {
          ERROR("Unix socket domain server accept error.");
          continue;
        }
      } else {
        for(uint32_t i = 0; i< AM_MEDIA_SERVICE_CLIENT_PROTO_NUM; ++i) {
          if(is_fd_valid(m_client_proto_fd[i]) &&
              FD_ISSET(m_client_proto_fd[i], &fdset)) {
            NOTICE("Received message from %s",
                   proto_type_to_string(AM_MEDIA_SERVICE_CLIENT_PROTO(i)));
            fd = m_client_proto_fd[i];
            break;
          }
        }
      }
      AM_MEDIA_NET_STATE  recv_ret = AM_MEDIA_NET_OK;
      AMMediaServiceMsgBlock client_msg;
      /*When a client connect to media service successfully,
       * an ack msg should be sent to media service immediatelly*/
      recv_ret = recv_client_msg(fd, &client_msg);
      switch(recv_ret) {
        case AM_MEDIA_NET_OK:
          if(AM_LIKELY(process_client_msg(fd, client_msg))) {
            break;
          }
          WARN("Failed to process client message!");
          /*no break*/
        case AM_MEDIA_NET_ERROR :
        case AM_MEDIA_NET_PEER_SHUTDOWN: {
          NOTICE("Close connection with %s", fd_to_proto_string(fd).c_str());
          close(fd);
        }
        case AM_MEDIA_NET_BADFD: {
          for(uint32_t i = 0; i < AM_MEDIA_SERVICE_CLIENT_PROTO_NUM; ++i) {
            if(fd == m_client_proto_fd[i]) {
              m_client_proto_fd[i] = -1;
              break;
            }
          }
        }break;
        default: break;
      }
    } else {
      if (AM_LIKELY(errno != EAGAIN)) {
        if (AM_LIKELY(errno == EBADF)) {
          for (int32_t i = 0; i < AM_MEDIA_SERVICE_CLIENT_PROTO_NUM; ++ i) {
            if (AM_LIKELY((m_client_proto_fd[i] >= 0) &&
                          !is_fd_valid(m_client_proto_fd[i]))) {
              NOTICE("%s is now disconnected!",
                     proto_type_to_string(AM_MEDIA_SERVICE_CLIENT_PROTO(i)));
              close(m_client_proto_fd[i]);
              m_client_proto_fd[i] = -1;
            }
          }
        } else {
          m_run = false;
          PERROR("select");
          break;
        }
      }
    }
  }/* end while(run)*/
  release_resource();
  if (AM_LIKELY(m_run)) {
    NOTICE("Exit %s mainloop!", m_socket_thread->name());
    m_run = false;
  } else {
    WARN("Thread %s exits due to error", m_socket_thread->name());
  }
}

bool AMMediaService::create_resource()
{
  bool ret = true;
  do {
    m_unix_socket_fd = unix_socket_listen(MEDIA_SERVICE_UNIX_DOMAIN_NAME);
    if(AM_UNLIKELY(m_unix_socket_fd < 0)) {
      ERROR("Unix socket domain listen error.");
      ret = false;
      break;
    }
  }while(0);
  return ret;
}
void AMMediaService::release_resource()
{
  for(int i = 0; i < AM_MEDIA_SERVICE_CLIENT_PROTO_NUM; i++) {
    if(m_client_proto_fd[i] >= 0) {
      close(m_client_proto_fd[i]);
      m_client_proto_fd[i] = -1;
    }
  }
  if (m_unix_socket_fd >= 0) {
    close(m_unix_socket_fd);
    m_unix_socket_fd = -1;
  }
  if (AMFile::exists(MEDIA_SERVICE_UNIX_DOMAIN_NAME)) {
    if (AM_UNLIKELY(unlink(MEDIA_SERVICE_UNIX_DOMAIN_NAME) < 0)) {
      PERROR("unlink");
    }
  }
}

AM_MEDIA_NET_STATE AMMediaService::recv_client_msg(int fd,
                                        AMMediaServiceMsgBlock* client_msg)
{
  AM_MEDIA_NET_STATE ret = AM_MEDIA_NET_OK;
  int read_ret = 0;
  int read_cnt = 0;
  uint32_t received = 0;
  do {
    if(AM_UNLIKELY(fd < 0)) {
      ret = AM_MEDIA_NET_BADFD;
      ERROR("fd is below zero.");
      break;
    }
    do{
      read_ret = recv(fd, ((uint8_t*)client_msg) + received,
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
      AMMediaServiceMsgBase* msg = (AMMediaServiceMsgBase*)client_msg;
      if(msg->length == sizeof(AMMediaServiceMsgBase)) {
        break;
      } else {
        read_ret = 0;
        read_cnt = 0;
        while((received < msg->length) && (5 > read_cnt ++) &&
            ((read_ret == 0) || ((read_ret < 0) && ((errno == EINTR) ||
                (errno == EWOULDBLOCK) ||
                (errno == EAGAIN))))) {
          read_ret = recv(fd, ((uint8_t*)client_msg) + received,
                          msg->length - received, 0);
          if(AM_LIKELY(read_ret > 0)) {
            received += read_ret;
          }
        }
      }
    }
    if(read_ret <= 0) {
      if(AM_LIKELY((errno == ECONNRESET) || (read_ret == 0))) {
        NOTICE("%s closed its connection while reading data.",
               fd_to_proto_string(fd).c_str());
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

bool AMMediaService::process_client_msg(int fd,
                                        AMMediaServiceMsgBlock& client_msg)
{
  bool ret = true;
  AM_MEDIA_SERVICE_CLIENT_PROTO proto = AM_MEDIA_SERVICE_CLIENT_PROTO_UNKNOWN;
  AMMediaServiceMsgBase *msg = (AMMediaServiceMsgBase*)&client_msg;
  do {
    if(msg->type == AM_MEDIA_SERVICE_MSG_ACK) {
      AMMediaServiceMsgACK* msg = &(client_msg.msg_ack);
      if(m_client_proto_fd[msg->proto] < 0) {
        m_client_proto_fd[msg->proto] = fd;
        INFO("receive ack msg, proto is %s", proto_type_to_string(msg->proto));
      } else {
        if(m_client_proto_fd[msg->proto] != fd) {
          if(is_fd_valid(m_client_proto_fd[msg->proto])) {
            ERROR("The m_client_proto_fd is already set and is valid,"
                "the new fd is not same as the old one. drop the new fd");
            ret = false;
            break;
          } else {
            NOTICE("The m_client_proto_fd is already set but is invalid,"
                "the new fd is not same as the old one, drop the old one");
            m_client_proto_fd[msg->proto] = fd;
          }
        }
      }
    }
    for(uint32_t i = 0; i < AM_MEDIA_SERVICE_CLIENT_PROTO_NUM; ++i) {
      if(m_client_proto_fd[i] == fd) {
        proto = AM_MEDIA_SERVICE_CLIENT_PROTO(i);
        break;
      }
    }
    switch (proto) {
      case AM_MEDIA_SERVICE_CLIENT_PROTO_SIP:{
        switch (msg->type) {
          case AM_MEDIA_SERVICE_MSG_ACK :{
            INFO("media service receives ack message from sip service.");
            if(AM_UNLIKELY(!send_ack(proto))) {
              ERROR("Media service failed to send ack to sip service.");
              ret = false;
            }
          }break;
          case AM_MEDIA_SERVICE_MSG_AUDIO_INFO :{
            AM_PLAYER_INSTANCE_ID id = PLAYER_NULL;
            if (AM_LIKELY((id = get_valid_playback_id()) != PLAYER_NULL)) {
              if(AM_UNLIKELY(!send_playback_id(proto, id))) {
                ERROR("Failed to send playback id to %s",
                       fd_to_proto_string(fd).c_str());
                ret = false;
              } else {
                INFO("Send playback id %d to %s successfully!", id,
                     fd_to_proto_string(fd).c_str());
              }
              AMMediaServiceMsgAudioINFO* msg_info = &(client_msg.msg_info);
              AMPlaybackUri uri;
              uri.type = AM_PLAYBACK_URI_RTP;
              uri.media.rtp.audio_type = msg_info->audio_format;
              uri.media.rtp.channel = msg_info->channel;
              uri.media.rtp.sample_rate = msg_info->sample_rate;
              uri.media.rtp.udp_port = msg_info->udp_port;
              uri.media.rtp.ip_domain = msg_info->ip_domain;
              if (!m_playback[id]->add_uri(uri)) {
                ERROR("Failed to add uri.");
                ret = false;
                break;
              }
              if (!m_playback[id]->play()) {
                ERROR("Failed to start playing!");
                ret = false;
                break;
              }
            } else {
              ERROR("Failed to get valid player.");
              ret = false;
            }
          }break;
          case AM_MEDIA_SERVICE_MSG_UDS_URI: {
            AM_PLAYER_INSTANCE_ID id = PLAYER_NULL;
            if (AM_LIKELY((id = get_valid_playback_id()) != PLAYER_NULL)) {
              if(AM_UNLIKELY(!send_playback_id(proto, id))) {
                ERROR("Failed to send playback id to %s",
                       fd_to_proto_string(fd).c_str());
                ret = false;
              } else {
                INFO("Send playback id %d to %s successfully!", id,
                     fd_to_proto_string(fd).c_str());
              }
              AMMediaServiceMsgUdsURI *uds_uri = &(client_msg.msg_uds);
              AMPlaybackUri uri;
              uri.type = AM_PLAYBACK_URI_UNIX_DOMAIN;
              uri.media.unix_domain.audio_type = uds_uri->audio_type;
              uri.media.unix_domain.sample_rate = uds_uri->sample_rate;
              uri.media.unix_domain.channel = uds_uri->channel;
              strncpy(uri.media.unix_domain.name, uds_uri->name,
                      sizeof(uri.media.unix_domain.name));
              if (!m_playback[id]->add_uri(uri)) {
                ERROR("Failed to add uri.");
                ret = false;
                break;
              }
              if (!m_playback[id]->play()) {
                ERROR("Failed to start playing!");
                ret = false;
                break;
              }
            } else {
              ERROR("Failed to get valid player.");
              ret = false;
            }
          } break;
          case AM_MEDIA_SERVICE_MSG_STOP :{
            AMMediaServiceMsgSTOP *msg_stop = &(client_msg.msg_stop);
            INFO("Media service receives stop playback %d message from %s",
                 msg_stop->playback_id, fd_to_proto_string(fd).c_str());
            if (m_playback == nullptr) {
              ERROR("Playback is not enabled in media service.");
              ret = false;
              break;
            }
            if (msg_stop->playback_id !=  PLAYER_NULL) {
              if(!m_playback[msg_stop->playback_id]->stop()) {
                ERROR("Failed to stop playing.");
                ret = false;
              } else {
                INFO("Stop playback %d successfully.", msg_stop->playback_id);
                if (!release_playback_instance(AM_PLAYER_INSTANCE_ID(
                                               msg_stop->playback_id))) {
                  ERROR("Failed to release playback instance. id is %d",
                         msg_stop->playback_id);
                  ret = false;
                }
              }
            }
          } break;
          default: {
            NOTICE("Client msg proto error.");
            ret = false;
            break;
          }
        }
      }break;
      default:
        NOTICE("proto type error.");
        ret = false;
        break;
    }
  }while(0);
  return ret;
}

std::string AMMediaService::fd_to_proto_string(int fd)
{
  std::string str = "Unknown";
  if(AM_LIKELY(fd >= 0)) {
    for(uint32_t i = 0; i< AM_MEDIA_SERVICE_CLIENT_PROTO_NUM; ++i) {
      if(AM_LIKELY(fd == m_client_proto_fd[i])) {
        str = proto_type_to_string(AM_MEDIA_SERVICE_CLIENT_PROTO(i));
      }
    }
  }
  return str;
}

bool AMMediaService::send_ack(AM_MEDIA_SERVICE_CLIENT_PROTO proto)
{
  struct AMMediaServiceMsgACK ack;
  bool ret = true;
  int count = 0;
  int send_ret = 0;
  ack.base.length = sizeof(AMMediaServiceMsgACK);
  ack.base.type = AM_MEDIA_SERVICE_MSG_ACK;
  ack.proto = proto;
  do{
    if(AM_UNLIKELY(m_client_proto_fd[proto] < 0)) {
      ERROR("the fd of %s is below zero.");
      ret = false;
      break;
    }
    while((++ count < 5) && (sizeof(AMMediaServiceMsgACK) !=
        (send_ret = write(m_client_proto_fd[proto], &ack, sizeof(ack))))) {
      if(AM_LIKELY((errno != EAGAIN) &&
                   (errno != EWOULDBLOCK) &&
                   (errno != EINTR))) {
        ret = false;
        PERROR("write");
        break;
      }
    }
  }while(0);
  return (ret && (send_ret == sizeof(ack)) && (count < 5));
}

bool AMMediaService::send_playback_id(AM_MEDIA_SERVICE_CLIENT_PROTO proto,
                                      int32_t id)
{
  bool ret = true;
  int count = 0;
  int send_ret = 0;
  AMMediaServiceMsgPlaybackID msg_id;
  msg_id.base.type = AM_MEDIA_SERVICE_MSG_PLAYBACK_ID;
  msg_id.base.length = sizeof(AMMediaServiceMsgPlaybackID);
  msg_id.playback_id = id;
  do {
    if(AM_UNLIKELY(m_client_proto_fd[proto] < 0)) {
      ERROR("the fd of %s is below zero.");
      ret = false;
      break;
    }
    while((++ count < 5) && (sizeof(AMMediaServiceMsgPlaybackID) !=
       (send_ret = write(m_client_proto_fd[proto], &msg_id, sizeof(msg_id))))) {
      if(AM_LIKELY((errno != EAGAIN) &&
                   (errno != EWOULDBLOCK) &&
                   (errno != EINTR))) {
        ret = false;
        PERROR("write");
        break;
      }
    }
  } while(0);
  return (ret && (send_ret == sizeof(msg_id)) && (count < 5));
}
