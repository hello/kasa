/*******************************************************************************
 * am_media_service_instance.cpp
 *
 * History:
 *   2015-1-20 - [ccjing] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_file.h"
#include "am_thread.h"
#include "am_mutex.h"
#include "am_record_msg.h"
#include "am_playback_msg.h"
#include "am_media_service_instance.h"
#include "am_api_media.h"

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
    result = NULL;
  }
  return result;
}

bool AMMediaService::start_media()
{
  AUTO_SPIN_LOCK(m_lock);
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
    if(AM_UNLIKELY(m_record->is_recording())) {
      NOTICE("Record is already started.");
    } else if(!m_record->start()) {
      ERROR("Failed to start record.");
      m_is_started = false;
      if (m_run) {
        write(SERVICE_CTRL_WRITE, "s", 1);
      }
      NOTICE("Thread %s is already running, destroy it!",
             m_socket_thread->name());
      m_socket_thread->destroy();
      m_socket_thread = NULL;
      break;
    }
    m_is_started = true;
  }while(0);

  return m_is_started;
}

bool AMMediaService::stop_media()
{
  AUTO_SPIN_LOCK(m_lock);
  bool ret = true;
  if (m_run) {
    write(SERVICE_CTRL_WRITE, "s", 1);
  }
  if(m_socket_thread) {
    m_socket_thread->destroy();
    m_socket_thread = NULL;
  }
  if(AM_UNLIKELY(!m_record->stop())) {
    ERROR("Failed to stop record.");
    ret = false;
  }
  if(AM_UNLIKELY(!m_playback->stop())) {
    ERROR("Failed to stop playback.");
    ret = false;
  }
  m_is_started = false;

  return ret;
}

bool AMMediaService::send_event()
{
  bool ret = false;

  if (m_record->is_recording()) {
    if (m_record->is_ready_for_event()) {
      if (m_record->send_event()) {
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

  return ret;
}

void AMMediaService::destroy()
{
  delete this;
}

AMIPlaybackPtr& AMMediaService::get_playback_instance()
{
  return m_playback;
}

AMIRecordPtr& AMMediaService::get_record_instance()
{
  return m_record;
}

AMMediaService::AMMediaService() :
    m_media_callback(nullptr),
    m_playback(nullptr),
    m_record(nullptr),
    m_unix_socket_fd(-1),
    m_run(false),
    m_is_started(false),
    m_socket_thread(nullptr),
    m_lock(nullptr)
{
  SERVICE_CTRL_READ = -1;
  SERVICE_CTRL_WRITE = -1;
  for(uint32_t i = 0; i < AM_MEDIA_SERVICE_CLIENT_PROTO_NUM; ++i) {
    m_client_proto_fd[i] = -1;
  }
}

AMMediaService::~AMMediaService()
{
  stop_media();
  AM_DESTROY(m_lock);
  m_playback = nullptr;
  m_record   = nullptr;
  if (AM_LIKELY(SERVICE_CTRL_READ >= 0)) {
    close(SERVICE_CTRL_READ);
  }
  if (AM_LIKELY(SERVICE_CTRL_WRITE >= 0)) {
    close(SERVICE_CTRL_WRITE);
  }
}

bool AMMediaService::init(media_callback media_callback)
{
  bool ret = true;
  do{
    if(AM_UNLIKELY(pipe(m_service_ctrl) < 0)) {
      PERROR("pipe");
      ret = false;
      break;
    }
    m_media_callback = media_callback;
    if(AM_UNLIKELY(!m_media_callback)) {
      WARN("Media callback function is not set!");
    }
    m_record = AMIRecord::create();
    if(AM_UNLIKELY(!m_record)) {
      ERROR("Failed to create record.");
      ret = false;
      break;
    } else if(AM_UNLIKELY(!(m_record->init()))) {
      ERROR("Failed to initialize record.");
      ret = false;
      break;
    }
    m_lock = AMSpinLock::create();
    if (AM_UNLIKELY(!m_lock)) {
      ERROR("Failed to create lock!");
      break;
    }
    m_record->set_msg_callback(record_engine_callback, NULL);
    m_playback = AMIPlayback::create();
    if(AM_UNLIKELY(!m_playback)) {
      ERROR("Failed to create playback.");
      ret = false;
      break;
    } else if(AM_UNLIKELY(!(m_playback->init()))) {
      ERROR("Failed to initialize playback.");
      ret = false;
      break;
    }
    m_playback->set_msg_callback(playback_engine_callback, NULL);
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
    if(AM_LIKELY(select(max_fd + 1, &fdset, NULL, NULL, NULL) > 0)) {
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
            AMMediaServiceMsgAudioINFO* msg_info = &(client_msg.msg_info);
            AMPlaybackUri uri;
            uri.type = AM_PLAYBACK_URI_RTP;
            uri.media.rtp.audio_format = msg_info->audio_format;
            uri.media.rtp.channel = msg_info->channel;
            uri.media.rtp.sample_rate = msg_info->sample_rate;
            uri.media.rtp.udp_port = msg_info->udp_port;
            uri.media.rtp.ip_domain = msg_info->ip_domain;
            if(!m_playback->add_uri(uri)) {
              ERROR("Failed to add uri.");
              break;
            } else {
              if(m_playback->is_playing() || m_playback->is_paused()) {
                NOTICE("Playback is playing other audio, please stop it first.");
              } else {
                if(AM_UNLIKELY(!m_playback->play())) {
                  ERROR("Failed to start playing");
                  break;
                }
              }
            }
          }break;
          case AM_MEDIA_SERVICE_MSG_STOP :{
            INFO("Media service receives stop message from sip service.");
            if(!m_playback->stop()) {
              ERROR("Failed to stop playing.");
            } else {
              INFO("Stop playback successfully.");
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
