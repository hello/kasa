/*******************************************************************************
 * am_muxer_rtp.cpp
 *
 * History:
 *   2015-1-4 - [ypchang] created file
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
#include "am_fd.h"

#include "am_amf_types.h"
#include "am_amf_packet.h"

#include "am_rtp_client.h"
#include "am_muxer_rtp.h"
#include "am_rtp_session_if.h"
#include "am_muxer_rtp_config.h"

#include "am_audio_type.h"

#include "am_file.h"
#include "am_event.h"
#include "am_mutex.h"
#include "am_timer.h"
#include "am_thread.h"
#include "am_plugin.h"

#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <algorithm>

#ifdef BUILD_AMBARELLA_ORYX_CONF_DIR
#define MUXER_CONF_DIR ((const char*)BUILD_AMBARELLA_ORYX_CONF_DIR"/stream/muxer")
#else
#define MUXER_CONF_DIR ((const char*)"/etc/oryx/stream/muxer")
#endif

static const char* audio_type_to_string(AM_AUDIO_TYPE type)
{
  const char *name = "Unknown";
  switch(type) {
    case AM_AUDIO_NULL:   name = "NULL";       break;
    case AM_AUDIO_LPCM:   name = "PCM";        break;
    case AM_AUDIO_BPCM:   name = "BPCM";       break;
    case AM_AUDIO_G711A:  name = "G.711 Alaw"; break;
    case AM_AUDIO_G711U:  name = "G.711 Ulaw"; break;
    case AM_AUDIO_G726_40:
    case AM_AUDIO_G726_32:
    case AM_AUDIO_G726_24:
    case AM_AUDIO_G726_16: name = "G.726";     break;
    case AM_AUDIO_AAC:     name = "AAC";       break;
    case AM_AUDIO_OPUS:    name = "OPUS";      break;
    case AM_AUDIO_SPEEX:   name = "SPEEX";     break;
    default:break;
  }
  return name;
}

static const char* audio_codec_type_to_key(AM_AUDIO_CODEC_TYPE type)
{
  const char *name = "";
  switch(type) {
    case AM_AUDIO_CODEC_NONE:  name = "none"; break;
    case AM_AUDIO_CODEC_AAC:   name = "aac";  break;
    case AM_AUDIO_CODEC_OPUS:  name = "opus"; break;
    case AM_AUDIO_CODEC_LPCM:  name = "pcm";  break;
    case AM_AUDIO_CODEC_BPCM:  name = "bpcm"; break;
    case AM_AUDIO_CODEC_G726:  name = "g726"; break;
    case AM_AUDIO_CODEC_G711:  name = "g711"; break;
    case AM_AUDIO_CODEC_SPEEX: name = "speex"; break;
    default: break;
  }
  return name;
}

static const char* audio_type_to_key(AM_AUDIO_TYPE type)
{
  const char *name = "";
  switch(type) {
    case AM_AUDIO_NULL:   name = "null";  break;
    case AM_AUDIO_LPCM: {
      name = audio_codec_type_to_key(AM_AUDIO_CODEC_LPCM);
    }break;
    case AM_AUDIO_BPCM: {
      name = audio_codec_type_to_key(AM_AUDIO_CODEC_BPCM);
    }break;
    case AM_AUDIO_G711A:
    case AM_AUDIO_G711U: {
      name = audio_codec_type_to_key(AM_AUDIO_CODEC_G711);
    }break;
    case AM_AUDIO_G726_40:
    case AM_AUDIO_G726_32:
    case AM_AUDIO_G726_24:
    case AM_AUDIO_G726_16: {
      name = audio_codec_type_to_key(AM_AUDIO_CODEC_G726);
    }break;
    case AM_AUDIO_AAC: {
      name = audio_codec_type_to_key(AM_AUDIO_CODEC_AAC);
    }break;
    case AM_AUDIO_OPUS: {
      name = audio_codec_type_to_key(AM_AUDIO_CODEC_OPUS);
    }break;
    case AM_AUDIO_SPEEX: {
      name = audio_codec_type_to_key(AM_AUDIO_CODEC_SPEEX);
    }break;
    default:break;
  }
  return name;
}

static const char* proto_to_string(AM_RTP_CLIENT_PROTO proto)
{
  const char *str = "Unknown";
  switch(proto) {
    case AM_RTP_CLIENT_PROTO_RTSP: str = "RTSP"; break;
    case AM_RTP_CLIENT_PROTO_SIP:  str = "SIP";  break;
    default:break;
  }

  return str;
}

AMIMuxerCodec* get_muxer_codec(const char* config)
{
  return (AMIMuxerCodec*)AMMuxerRtp::create(config);
}

AMSessionPlugin* AMSessionPlugin::create(AMPlugin *so, AMIRtpSession *session)
{
  AMSessionPlugin *plugin = (so && session) ?
      new AMSessionPlugin(so, session) : nullptr;
  if (AM_LIKELY(plugin)) {
    plugin->add_ref();
  }
  return plugin;
}

AMIRtpSession*& AMSessionPlugin::session()
{
  return m_session;
}

void AMSessionPlugin::destroy(bool notify)
{
  std::lock_guard<AMMemLock> lock(m_lock);
  if (AM_LIKELY(-- m_ref_count == 0)) {
    m_session->del_clients_all(notify);
    delete this;
  }
}

void AMSessionPlugin::add_ref()
{
  std::lock_guard<AMMemLock> lock(m_lock);
  ++ m_ref_count;
}

AMSessionPlugin::AMSessionPlugin(AMPlugin *so, AMIRtpSession *session) :
  m_so(so),
  m_session(session)
{}

AMSessionPlugin::~AMSessionPlugin()
{
  delete m_session;
  AM_DESTROY(m_so);
}

AMMuxerRtp* AMMuxerRtp::create(const char *conf_file)
{
  AMMuxerRtp *result = new AMMuxerRtp();
  if (AM_UNLIKELY(result && (AM_STATE_OK != result->init(conf_file)))) {
    delete result;
    result = nullptr;
  }

  return result;
}

AM_STATE AMMuxerRtp::start()
{
  AM_STATE state = AM_STATE_ERROR;
  do {
    m_run = true;
    m_thread_work = AMThread::create("RTP.Work", static_work_thread, this);
    if (AM_UNLIKELY(!m_thread_work)) {
      ERROR("Failed to create RTP work thread!");
      m_run = false;
      break;
    }
    m_thread_server = AMThread::create("RTP.Server",
                                       static_server_thread, this);
    if (AM_UNLIKELY(!m_thread_server)) {
      ERROR("Failed to create RTP server thread!");
      m_run = false;
      break;
    }

    m_event_work->signal();
    while (!m_work_run.load()) {
      usleep(100000);
    }
    m_event_server->signal();
    while(!m_server_run.load()) {
      usleep(100000);
    }
    AUTO_MEM_LOCK(m_lock);
    m_muxer_state = AM_MUXER_CODEC_RUNNING;
    state = AM_STATE_OK;
  }while(0);

  return state;
}

AM_STATE AMMuxerRtp::stop()
{
  if (AM_LIKELY(AM_MUXER_CODEC_STOPPED != m_muxer_state)) {
    AMRtpClientMsgClose closecmd;
    /* Make server thread exit */
    send_client_msg(CTRL_FD_CLIENT, (uint8_t*)&closecmd, sizeof(closecmd));
    m_run = false; /* Make work thread exit */
    if (AM_LIKELY(AM_MUXER_CODEC_INIT == m_muxer_state)) {
      m_event_work->signal();
      m_event_server->signal();
    }
    m_lock.lock();
    m_muxer_state = AM_MUXER_CODEC_STOPPED;
    m_lock.unlock();
    AM_DESTROY(m_thread_work);
    AM_DESTROY(m_thread_server);
  }

  return AM_STATE_OK;
}

const char* AMMuxerRtp::name()
{
  return "muxer-rtp";
}

void* AMMuxerRtp::get_muxer_codec_interface(AM_REFIID refiid)
{
  return (refiid == IID_AMIMuxerCodec) ? ((AMIMuxerCodec*)this) :
      ((void*)nullptr);
}

bool AMMuxerRtp::is_running()
{
  return m_run.load();
}

AM_STATE AMMuxerRtp::set_config(AMMuxerCodecConfig *config)
{
  return AM_STATE_ERROR;
}

AM_STATE AMMuxerRtp::get_config(AMMuxerCodecConfig *config)
{
  return AM_STATE_ERROR;
}

AM_MUXER_ATTR AMMuxerRtp::get_muxer_attr()
{
  return AM_MUXER_NETWORK_NORMAL;
}

uint8_t AMMuxerRtp::get_muxer_codec_stream_id()
{
  return (0x0f);
}

uint32_t AMMuxerRtp::get_muxer_id()
{
  return m_muxer_config->muxer_id;
}

AM_MUXER_CODEC_STATE AMMuxerRtp::get_state()
{
  AUTO_MEM_LOCK(m_lock);
  return m_muxer_state;
}

void AMMuxerRtp::feed_data(AMPacket* packet)
{
  if (AM_LIKELY(packet)) {
    m_packet_q.push(packet);
  }
}

void AMMuxerRtp::kill_client(uint32_t client_ssrc)
{
  m_client_kill_q.push(client_ssrc);
}

AMMuxerRtp::AMMuxerRtp() :
    m_muxer_config(nullptr),
    m_config(nullptr),
    m_timer(nullptr),
    m_event_work(nullptr),
    m_event_server(nullptr),
    m_thread_work(nullptr),
    m_thread_server(nullptr),
    m_muxer_state(AM_MUXER_CODEC_INIT),
    m_rtp_unix_fd(-1),
    m_support_media_type(0),
    m_active_kill(false),
    m_run(true),
    m_work_run(false),
    m_server_run(false)
{
  CTRL_FD_CLIENT = -1;
  CTRL_FD_MUXER  = -1;
  for (int32_t i = 0; i < AM_RTP_CLIENT_PROTO_NUM; ++ i) {
    m_client_proto_fd[i] = -1;
  }
  memset(&m_client_msg, 0, sizeof(m_client_msg));
  m_unix_sock_name.clear();
}

AMMuxerRtp::~AMMuxerRtp()
{
  stop();
  clear_client_kill_queue();
  clear_all_resources();
  delete m_config;
  AM_DESTROY(m_thread_work);
  AM_DESTROY(m_thread_server);
  AM_DESTROY(m_event_work);
  AM_DESTROY(m_event_server);
  AM_DESTROY(m_timer);
  if (AM_LIKELY(m_rtp_unix_fd >= 0)) {
    close(m_rtp_unix_fd);
    unlink(m_unix_sock_name.c_str());
  }
  if (AM_LIKELY(CTRL_FD_CLIENT >= 0)) {
    close(CTRL_FD_CLIENT);
  }
  if (AM_LIKELY(CTRL_FD_MUXER >= 0)) {
    close(CTRL_FD_MUXER);
  }
}

AM_STATE AMMuxerRtp::init(const char *conf)
{
  AM_STATE state = AM_STATE_OK;

  do {
    m_config = new AMMuxerRtpConfig();
    if (AM_UNLIKELY(!m_config)) {
      ERROR("Failed to allocate memory for AMuxerRtpConfig!");
      state = AM_STATE_NO_MEMORY;
      break;
    }

    m_muxer_config = m_config->get_config(conf);
    if (AM_UNLIKELY(!m_muxer_config)) {
      ERROR("Failed to get RTP Muxer configuration from %s", conf);
      state = AM_STATE_ERROR;
      break;
    }

    m_unix_sock_name = RTP_CONTROL_SOCKET;
    m_rtp_unix_fd = rtp_unix_socket(m_unix_sock_name.c_str());
    if (AM_UNLIKELY(m_rtp_unix_fd < 0)) {
      ERROR("Failed to create server thread listening socket!");
      state = AM_STATE_ERROR;
      break;
    }

    if (AM_UNLIKELY(socketpair(AF_UNIX, SOCK_STREAM,
                               IPPROTO_IP, m_client_ctrl_fd) < 0)) {
      PERROR("socketpair");
      state = AM_STATE_ERROR;
      break;
    }

    m_event_work = AMEvent::create();
    if (AM_UNLIKELY(!m_event_work)) {
      ERROR("Failed to create AMEvent for work thread!");
      state = AM_STATE_NO_MEMORY;
      break;
    }

    m_event_server = AMEvent::create();
    if (AM_UNLIKELY(!m_event_server)) {
      ERROR("Failed to create AMEvent for server thread!");
      state = AM_STATE_NO_MEMORY;
      break;
    }

    m_timer = AMTimer::create();
    if (AM_UNLIKELY(!m_timer)) {
      ERROR("Failed to create AMTimer object for server thread!");
      state = AM_STATE_NO_MEMORY;
      break;
    }
  }while(0);

  return state;
}

void AMMuxerRtp::static_work_thread(void *data)
{
  ((AMMuxerRtp*)data)->work_thread();
}

void AMMuxerRtp::static_server_thread(void *data)
{
  ((AMMuxerRtp*)data)->server_thread();
}

bool AMMuxerRtp::static_timeout_kill_session(void *data)
{
  if (AM_LIKELY(data)) {
    std::string key = ((SessionDeleteData*)data)->key;
    AMMuxerRtp *muxer = ((SessionDeleteData*)data)->muxer;
    muxer->remove_session_del_data(key, ((SessionDeleteData*)data));
    muxer->kill_session(key);
  }

  return false;
}

void AMMuxerRtp::work_thread()
{
  m_event_work->wait();
  m_work_run = true;
  m_support_media_type = 0;
  while (m_run.load()) {
    AMPacket *packet = NULL;

    clear_client_kill_queue();
    if (AM_UNLIKELY(m_packet_q.empty())) {
      usleep(10000);
      continue;
    }
    packet = m_packet_q.front_pop();
    if (AM_LIKELY(packet)) {
      switch(packet->get_type()) {
        case AMPacket::AM_PAYLOAD_TYPE_INFO: {
          switch(packet->get_attr()) {
            case AMPacket::AM_PAYLOAD_ATTR_AUDIO: {
              AM_AUDIO_INFO *ainfo = (AM_AUDIO_INFO*)packet->get_data_ptr();
              RtpSessionTypeMap::iterator session_type_iter =
                  m_muxer_config->rtp_session_map.find((int32_t)ainfo->type);
              if (AM_UNLIKELY(session_type_iter ==
                              m_muxer_config->rtp_session_map.end())) {
                ERROR("RTP doesn't support audio type %s!",
                      audio_type_to_string(ainfo->type));
              } else {
                const RtpSessionType &session_type =
                    m_muxer_config->rtp_session_map[(int32_t)ainfo->type];
                std::string def_key = session_type.codec;
                std::string key = def_key + "-" +
                    std::to_string(ainfo->sample_rate / 1000) + "k";
                AMSessionPlugin *session_plugin =
                    load_session_plugin(key.c_str(), key.c_str(), ainfo);
                AMRtpSessionMap::iterator iter;

                /* Prevent the timer from deleting the session */
                stop_timer_kill_session(def_key);
                stop_timer_kill_session(key);

                m_session_lock.lock();
                iter = m_session_map.find(key);
                if ((ainfo->type == AM_AUDIO_G711A) ||
                    (ainfo->type == AM_AUDIO_G711U)) {
                  if (8000 == ainfo->sample_rate) {
                    m_support_media_type |= (1 << ainfo->type);
                  }
                } else {
                  m_support_media_type |= (1 << ainfo->type);
                }
                if (AM_LIKELY(session_plugin &&
                              (iter == m_session_map.end()))) {
                  AMRtpSessionMap::iterator def = m_session_map.find(def_key);
                  m_session_map[key] = session_plugin;
                  if (AM_LIKELY(def == m_session_map.end())) {
                    session_plugin->add_ref();
                    m_session_map[def_key] = session_plugin;
                    NOTICE("Use %s as the default RTP session for %s",
                           key.c_str(), def_key.c_str());
                  } else {
                    uint32_t session_clock =
                        session_plugin->session()->get_session_clock();
                    uint32_t def_session_clock =
                        def->second->session()->get_session_clock();
                    if (AM_LIKELY(session_clock > def_session_clock)) {
                      /* Always set the default audio session to the audio
                       * session with the highest audio sample rate
                       */
                      session_plugin->add_ref();
                      m_session_map[def_key]->destroy();
                      m_session_map[def_key] = session_plugin;
                      NOTICE("Use %s as the default RTP session for %s",
                             key.c_str(), def_key.c_str());
                    }
                  }
                  m_session_lock.unlock();
                  NOTICE("RTP session for %s is created!", key.c_str());
                  SupportAudioInfo *audio_info = new SupportAudioInfo;
                  if (AM_LIKELY(audio_info)) {
                    audio_info->audio_type = ainfo->type;
                    audio_info->sample_rate = ainfo->sample_rate;
                    m_rtp_audio_list.push_back(audio_info);
                  } else {
                    ERROR("Failed to create SupportAudioInfo structure!");
                  }
                } else if (AM_LIKELY(session_plugin)) {
                  NOTICE("RTP session for %s is already created!", key.c_str());
                  session_plugin->destroy();
                } else {
                  ERROR("Failed to create session for %s!",
                        session_type.name.c_str());
                }
              }
            }break;
            case AMPacket::AM_PAYLOAD_ATTR_VIDEO: {
              AM_VIDEO_INFO *vinfo = (AM_VIDEO_INFO*)packet->get_data_ptr();
              RtpSessionTypeMap::iterator session_type_iter =
                  m_muxer_config->rtp_session_map.find((int32_t)vinfo->type);
              if (AM_UNLIKELY(session_type_iter ==
                              m_muxer_config->rtp_session_map.end())) {
                ERROR("RTP doesn't support video type %d!", vinfo->type);
              } else {
                const RtpSessionType &session_type =
                    m_muxer_config->rtp_session_map[(int32_t)vinfo->type];
                std::string key = "video" + std::to_string(vinfo->stream_id);
                AMSessionPlugin *session_plugin =
                    load_session_plugin(session_type.codec.c_str(),
                                        key.c_str(),
                                        vinfo);
                AMRtpSessionMap::iterator iter;

                /* Prevent the timer from deleting the session */
                stop_timer_kill_session(key);

                m_session_lock.lock();
                iter = m_session_map.find(key);

                m_support_media_type |= (1 << vinfo->type);

                if (AM_LIKELY(session_plugin &&
                              (iter == m_session_map.end()))) {
                  m_session_map[key] = session_plugin;
                  NOTICE("Rtp session for %s stream%u{%s} is created!",
                         key.c_str(),
                         vinfo->stream_id,
                         session_type.name.c_str());
                  bool find = false;
                  for (uint8_t i = 0; i < m_rtp_video_list.size(); ++i) {
                    if (vinfo->type == m_rtp_video_list.at(i)->video_type) {
                      find = true;
                    }
                  }
                  if (!find) {
                    SupportVideoInfo *video_info = new SupportVideoInfo;
                    if (AM_LIKELY(video_info)) {
                      video_info->video_type = vinfo->type;
                      /* Always use stream ID 1(640*480) */
                      video_info->stream_id = 1; /* vinfo->stream_id */
                      m_rtp_video_list.push_back(video_info);
                    } else {
                      ERROR("Failed to create SupportVideoInfo struct!");
                    }
                  }
                } else if (AM_LIKELY(session_plugin && m_session_map[key])) {
                  if (AM_LIKELY(session_plugin->session()->type() ==
                      m_session_map[key]->session()->type())) {
                    NOTICE("RTP session for %s{%s} is already created!",
                           key.c_str(),
                           session_type.name.c_str());
                    session_plugin->destroy();
                  } else {
                    NOTICE("Video%u has change its type from %s to %s!",
                           m_session_map[key]->session()->type_string().c_str(),
                           session_plugin->session()->type_string().c_str());
                    m_session_map[key]->destroy();
                    m_session_map[key] = session_plugin;
                  }
                } else {
                  ERROR("Failed to create session for %s!",
                        session_type.name.c_str());
                }
                m_session_lock.unlock();
              }
            }break;
            default: break;
          }
        }break; /* case AMPacket::AM_PAYLOAD_TYPE_INFO: */
        case AMPacket::AM_PAYLOAD_TYPE_DATA: {
          AM_SESSION_STATE state = AM_SESSION_OK;
          AMRtpSessionMap::iterator iter;
          std::string key;
          switch(packet->get_attr()) {
            case AMPacket::AM_PAYLOAD_ATTR_AUDIO: {
              key = std::string(audio_type_to_key(
                  AM_AUDIO_TYPE(packet->get_frame_type()))) + "-" +
                  std::to_string(packet->get_frame_attr() / 1000) + "k";
            }break;
            case AMPacket::AM_PAYLOAD_ATTR_VIDEO: {
              key = "video" + std::to_string(packet->get_stream_id());
            }break;
            default: break;
          }
          m_session_lock.lock();
          iter = m_session_map.find(key);
          if (AM_LIKELY(iter != m_session_map.end())) {
            AMIRtpSession *&session = iter->second->session();
            state = session->process_data(*packet,
                                          m_muxer_config->max_tcp_payload_size);
          } else {
            state = AM_SESSION_ERROR;
          }
          if (AM_UNLIKELY(state == AM_SESSION_ERROR)) {
            if (AM_LIKELY(iter != m_session_map.end())) {
              ERROR("Error occurs in RTP session %s, delete it!",
                    iter->second->session()->type_string().c_str());
              iter->second->destroy();
            }
            m_session_map.erase(key);
          }
          m_session_lock.unlock();
        }break; /* case AMPacket::AM_PAYLOAD_TYPE_DATA: */
        case AMPacket::AM_PAYLOAD_TYPE_EOS: {
          std::string key;
          m_session_lock.lock();
          switch(packet->get_attr()) {
            case AMPacket::AM_PAYLOAD_ATTR_AUDIO: {
              std::string def_key =
                  audio_type_to_key(AM_AUDIO_TYPE(packet->get_frame_type()));
              AMRtpSessionMap::iterator def_iter;

              def_iter = m_session_map.find(def_key);
              uint32_t sample_rate = packet->get_frame_attr();

              key = def_key + "-" + std::to_string(sample_rate / 1000) + "k";
              if (AM_LIKELY((def_iter != m_session_map.end()) &&
                            (sample_rate == def_iter->second->session()->\
                                                      get_session_clock()))) {
                timer_kill_session(def_key);
              }
            }break;
            case AMPacket::AM_PAYLOAD_ATTR_VIDEO: {
              key = "video" + std::to_string(packet->get_stream_id());
            }break;
            default: break;
          }
          if (AM_LIKELY(!key.empty())) {
            timer_kill_session(key);
          }
          m_session_lock.unlock();

        }break; /* case AMPacket::AM_PAYLOAD_TYPE_EOS: */
        case AMPacket ::AM_PAYLOAD_TYPE_EVENT : {
          INFO("RTP muxer receive event packet, ignore.");
        } break;
        default: {
          ERROR("Unsupported packet type: %u!", packet->get_type());
        }break;
      }
      packet->release();
    }
  }
  clear_client_kill_queue();
  clear_all_resources();
  m_work_run = false;
  NOTICE("RTP Muxer work thread exits!");
}

static bool is_fd_valid(int fd)
{
  return (fd >= 0) && ((fcntl(fd, F_GETFD) != -1) || (errno != EBADF));
}

void AMMuxerRtp::server_thread()
{
  m_event_server->wait();
  m_server_run = true;
  while(m_server_run.load()) {
    int retval = -1;
    int maxfd = -1;
    int fd = -1;
    AM_NET_STATE ret = AM_NET_OK;
    fd_set fdset;

    FD_ZERO(&fdset);
    FD_SET(CTRL_FD_MUXER, &fdset);
    FD_SET(m_rtp_unix_fd, &fdset);
    maxfd = AM_MAX(CTRL_FD_MUXER, m_rtp_unix_fd);
    for (int32_t i = 0; i < AM_RTP_CLIENT_PROTO_NUM; ++ i) {
      if (AM_LIKELY(is_fd_valid(m_client_proto_fd[i]))) {
        FD_SET(m_client_proto_fd[i], &fdset);
        maxfd = AM_MAX(maxfd, m_client_proto_fd[i]);
      } else if (AM_LIKELY(m_client_proto_fd[i] >= 0)) {
        NOTICE("%s is now disconnected!",
               proto_to_string(AM_RTP_CLIENT_PROTO(i)));
        close(m_client_proto_fd[i]);
        m_client_proto_fd[i] = -1;
      }
    }
    if (AM_LIKELY((retval = select(maxfd + 1, &fdset, NULL, NULL, NULL)) > 0)) {
      if (FD_ISSET(CTRL_FD_MUXER, &fdset)) {
        fd = CTRL_FD_MUXER;
      } else if (FD_ISSET(m_rtp_unix_fd, &fdset)) {
        sockaddr_un clientaddr;
        socklen_t len = sizeof(clientaddr);
        uint32_t accept_cnt = 0;
        do {
          fd = accept(m_rtp_unix_fd, (sockaddr*)&clientaddr, &len);
        }while((++ accept_cnt < 5) && (fd < 0) && ((errno == EAGAIN) ||
            (errno == EWOULDBLOCK) || (errno == EINTR)));
        if (AM_UNLIKELY(fd < 0)) {
          PERROR("accept");
        } else {
          NOTICE("Client: %s connected, fd is %d!", clientaddr.sun_path, fd);
        }
      } else {
        for (uint32_t i = 0; i < AM_RTP_CLIENT_PROTO_NUM; ++ i) {
          if (AM_LIKELY((m_client_proto_fd[i] >= 0) &&
                        FD_ISSET(m_client_proto_fd[i], &fdset))) {
            if (AM_LIKELY(is_fd_valid(m_client_proto_fd[i]))) {
              NOTICE("Received message from %s(%d)...",
                     proto_to_string(AM_RTP_CLIENT_PROTO(i)),
                     m_client_proto_fd[i]);
              fd = m_client_proto_fd[i];
              break;
            } else {
              NOTICE("%s is now disconnected!",
                     proto_to_string(AM_RTP_CLIENT_PROTO(i)));
              close(m_client_proto_fd[i]);
              m_client_proto_fd[i] = -1;
            }
          }
        }
      }
      ret = recv_client_msg(fd, &m_client_msg);
      switch(ret) {
        case AM_NET_OK:
          if (AM_LIKELY(process_client_msg(fd, m_client_msg))) {
            break;
          }
          WARN("Failed to process client message!");
          /* no break */
        case AM_NET_ERROR:
        case AM_NET_PEER_SHUTDOWN:
          if (AM_LIKELY(fd != CTRL_FD_MUXER)) {
            NOTICE("close connection with %s", fd_to_proto_string(fd).c_str());
            close(fd);
          }
          /* no break */
        case AM_NET_BADFD: {
          for (uint32_t i = 0; i < AM_RTP_CLIENT_PROTO_NUM; ++ i) {
            if (AM_LIKELY(fd == m_client_proto_fd[i])) {
              m_client_proto_fd[i] = -1;
              break;
            }
          }
        }break;
        case AM_NET_AGAIN:
        default:break;
      }
    } else {
      if (AM_LIKELY(errno != EAGAIN)) {
        bool need_break = false;
        if (AM_LIKELY(errno == EBADF)) {
          for (int32_t i = 0; i < AM_RTP_CLIENT_PROTO_NUM; ++ i) {
            if (AM_LIKELY((m_client_proto_fd[i] >= 0) &&
                          !is_fd_valid(m_client_proto_fd[i]))) {
              NOTICE("%s is now disconnected!",
                     proto_to_string(AM_RTP_CLIENT_PROTO(i)));
              close(m_client_proto_fd[i]);
              m_client_proto_fd[i] = -1;
            }
          }
        } else {
          need_break = true;
          PERROR("select");
        }
        if (AM_LIKELY(need_break)) {
          break;
        }
      }
    }

  } /* while(m_run) */

  close_all_connections();
  m_server_run = false;
  NOTICE("RTP Muxer server thread exits!");
}

void AMMuxerRtp::kill_session(const std::string &key)
{
  m_session_lock.lock();
  /* timeout, start to kill all the sessions in the kill queue */
  if (AM_LIKELY(m_session_map.find(key) != m_session_map.end())) {
    m_session_map.at(key)->destroy();
    m_session_map.erase(key);
    NOTICE("RTP session %s received EOS!", key.c_str());
  } else {
    NOTICE("RTP session %s received EOS, but INFO was not received!",
           key.c_str());
  }
  if (AM_LIKELY(m_session_map.size() == 0)) {
    NOTICE("All streams are stopped!");
  }
  m_session_lock.unlock();
}

void AMMuxerRtp::timer_kill_session(const std::string &key)
{
  SessionDeleteData *data = new SessionDeleteData(this, key);
  bool kill_now = false;
  if (AM_LIKELY(data)) {
    std::string timer_name = key + ".del";
    if (AM_UNLIKELY(!m_timer->single_shot(
                     timer_name.c_str(),
                     m_muxer_config->kill_session_timeout,
                     static_timeout_kill_session,
                     data))) {
      kill_now = true;
      ERROR("Failed to delete session in timer!");
      delete data;
    } else {
      m_session_data_lock.lock();
      m_session_del_map[timer_name] = data;
      m_session_data_lock.unlock();
    }
  } else {
    kill_now = true;
    ERROR("Failed to create SessionDeleteData, kill sessin now!");
  }

  if (AM_LIKELY(kill_now)) {
    kill_session(key);
  }
}

void AMMuxerRtp::stop_timer_kill_session(const std::string &key)
{
  std::string timer_name = key + ".del";
  SessionDeleteData *data = (SessionDeleteData*)m_timer->
      single_stop(timer_name.c_str());
  remove_session_del_data(key, data);
}

void AMMuxerRtp::remove_session_del_data(const std::string &key,
                                         SessionDeleteData *data)
{
  std::string map_key = key + ".del";
  SessionDeleteMap::iterator iter;
  m_session_data_lock.lock();
  iter = m_session_del_map.find(map_key);
  if (AM_LIKELY(iter != m_session_del_map.end())) {
    if (AM_UNLIKELY(data != iter->second)) {
      WARN("Timer of %s has returned different data!", map_key.c_str());
      delete data;
    }
    delete iter->second;
    m_session_del_map.erase(iter);
  }
  m_session_data_lock.unlock();
}

void AMMuxerRtp::clear_client_kill_queue()
{
  while(!m_client_kill_q.empty()) {
    kill_client_by_ssrc(m_client_kill_q.front_pop());
  }
}

void AMMuxerRtp::clear_all_resources()
{
  while(!m_packet_q.empty()) {
    m_packet_q.front_pop()->release();
  }
  m_session_lock.lock();
  for (AMRtpSessionMap::iterator iter = m_session_map.begin();
      iter != m_session_map.end(); ++ iter) {
    iter->second->destroy(false); /* Dont' ask client to send notify back*/
  }
  m_session_map.clear();
  m_session_lock.unlock();

  m_session_data_lock.lock();
  for (SessionDeleteMap::iterator iter = m_session_del_map.begin();
       iter != m_session_del_map.end(); ++ iter) {
    m_timer->single_stop(iter->first.c_str());
    delete iter->second;
  }
  m_session_del_map.clear();
  m_session_data_lock.unlock();

  for (uint32_t i = 0; i < m_rtp_audio_list.size(); ++ i) {
    delete m_rtp_audio_list[i];
    m_rtp_audio_list[i] = nullptr;
  }
  m_rtp_audio_list.clear();

  for (uint32_t i = 0; i < m_rtp_video_list.size(); ++ i) {
    delete m_rtp_video_list[i];
    m_rtp_video_list[i] = nullptr;
  }
  m_rtp_video_list.clear();

}

void AMMuxerRtp::kill_client_by_ssrc(uint32_t ssrc)
{
  m_session_lock.lock();
  for (AMRtpSessionMap::iterator iter = m_session_map.begin();
      iter != m_session_map.end(); ++ iter) {
    iter->second->session()->del_client(ssrc);
  }
  m_session_lock.unlock();
}

void AMMuxerRtp::close_all_connections()
{
  AMRtpClientMsgClose msg_close;
  for (int32_t i = 0; i < AM_RTP_CLIENT_PROTO_NUM; ++ i) {
    if (AM_LIKELY(m_client_proto_fd[i] >= 0)) {
      send_client_msg(m_client_proto_fd[i], (uint8_t*)&msg_close,
                      sizeof(msg_close));
      close(m_client_proto_fd[i]);
      m_client_proto_fd[i] = -1;
    }
  }
}

AM_NET_STATE AMMuxerRtp::recv_client_msg(int fd,
                                         AMRtpClientMsgBlock *client_msg)
{
  AM_NET_STATE ret = AM_NET_OK;

  if (AM_LIKELY((fd >= 0) && client_msg)) {
    int read_ret = 0;
    int read_cnt = 0;
    uint32_t received = 0;

    do {
      read_ret = recv(fd, ((uint8_t *)client_msg) + received,
                      sizeof(AMRtpClientMsgBase) - received,
                      0);
      if (AM_LIKELY(read_ret > 0)) {
        received += read_ret;
      }
    }while((++ read_cnt < 5) &&
           (((read_ret >= 0) && (read_ret < (int)sizeof(AMRtpClientMsgBase))) ||
            ((read_ret < 0) && ((errno == EINTR) ||
                                (errno == EWOULDBLOCK) ||
                                (errno == EAGAIN)))));

    if (AM_LIKELY(received == sizeof(AMRtpClientMsgBase))) {
      AMRtpClientMsg *msg = (AMRtpClientMsg*)client_msg;
      read_ret = 0;
      read_cnt = 0;
      while ((received < msg->length) && (5 > read_cnt ++) &&
             ((read_ret == 0) || ((read_ret < 0) && ((errno == EINTR) ||
                                                     (errno == EWOULDBLOCK) ||
                                                     (errno == EAGAIN))))) {
        read_ret = recv(fd,
                        ((uint8_t *)client_msg) + received,
                        msg->length - received,
                        0);
        if (AM_LIKELY(read_ret > 0)) {
          received += read_ret;
        }
      }
    }

    if (AM_LIKELY(read_ret < 0)) {
      switch(errno) {
        case ECONNRESET: {
          NOTICE("%s closed its connection while reading data!",
                 fd_to_proto_string(fd).c_str());
          ret = AM_NET_PEER_SHUTDOWN;
        }break;
        case EBADF: {
          ret = AM_NET_BADFD;
          ERROR("Failed to receive message(%d): %s!",
                errno,  strerror(errno));
        }break;
        case EINTR:
        case EAGAIN: {
          ret = AM_NET_AGAIN;
          WARN("Failed to receive message(%d): %s!",
               errno,  strerror(errno));
        }break;
        default: {
          ret = AM_NET_ERROR;
          ERROR("Failed to receive message(%d): %s!",
                errno,  strerror(errno));
        }break;
      }
    } else if (read_ret == 0) {
      if (AM_LIKELY((errno == 0)) || (errno == EAGAIN)) {
        ret = AM_NET_PEER_SHUTDOWN;
      } else {
        ret = AM_NET_AGAIN;
        WARN("Received 0 bytes of data(%d: %s)!", errno, strerror(errno));
      }
    }
  }

  return ret;
}

int AMMuxerRtp::rtp_unix_socket(const char *name)
{
  int fd = -1;
  if (AM_LIKELY(name)) {
    bool isok = true;
    do {
      if (AM_UNLIKELY(AMFile::exists(name))) {
        NOTICE("%s already exists! Remove it first!", name);
        if (AM_UNLIKELY(unlink(name) < 0)) {
          PERROR("unlink");
          isok = false;
          break;
        }
      }
      if (AM_UNLIKELY(!AMFile::create_path(AMFile::dirname(name).c_str()))) {
        ERROR("Failed to create path %s", AMFile::dirname(name).c_str());
        isok = false;
        break;
      }

      if (AM_UNLIKELY((fd = socket(AF_UNIX, SOCK_STREAM, IPPROTO_IP)) < 0)) {
        PERROR("socket");
        isok = false;
        break;
      } else {
        struct sockaddr_un un;
        socklen_t socklen = offsetof(struct sockaddr_un, sun_path) +
                            strlen(name);
        memset(&un, 0, sizeof(un));
        un.sun_family = AF_UNIX;
        strcpy(un.sun_path, name);

        if (AM_UNLIKELY(bind(fd, (struct sockaddr*)&un, socklen) < 0)) {
          PERROR("bind");
          isok = false;
          break;
        }

        if (AM_UNLIKELY(listen(fd, m_muxer_config->max_proto_client) < 0)) {
          PERROR("listen");
          isok = false;
          break;
        }
      }

    }while(0);

    if (AM_UNLIKELY(!isok)) {
      if (AM_LIKELY(fd >= 0)) {
        close(fd);
        fd = -1;
      }
      if (AM_UNLIKELY(AMFile::exists(name) && (unlink(name) < 0))) {
        PERROR("unlink");
      }
    }
  }
  return fd;
}

bool AMMuxerRtp::process_client_msg(int fd, AMRtpClientMsgBlock &client_msg)
{
  bool ret = true;
  AMRtpClientMsg *msg = (AMRtpClientMsg*)&client_msg;

  switch(msg->type) {
    case AM_RTP_CLIENT_MSG_ACK: {
      AMRtpClientMsgACK &ack = client_msg.msg_ack;
      std::string media(ack.media);
      AMRtpSessionMap::iterator iter;

      std::transform(media.begin(), media.end(), media.begin(), ::tolower);
      m_session_lock.lock();
      iter = m_session_map.find(ack.media);
      if (AM_LIKELY(iter != m_session_map.end())) {
        AMIRtpSession *&session = iter->second->session();
        AMRtpClient *client = session->find_client_by_ssrc(ack.ssrc);
        if (AM_LIKELY(client)) {
          client->send_ack();
        } else {
          NOTICE("Strange, RTP muxer received ACK of client %08X "
                 "for session %s, but cannot find such client!",
                 ack.ssrc, media.c_str());
        }
      }
      m_session_lock.unlock();
    }break;
    case AM_RTP_CLIENT_MSG_SDP: {
      const char *addr = client_msg.msg_sdp.media[AM_RTP_MEDIA_NUM];
      std::string hostip = &addr[5];
      AM_IP_DOMAIN domain = AM_UNKNOWN;
      if (AM_LIKELY(is_str_n_equal(addr, "ipv4", strlen("ipv4")))) {
        domain = AM_IPV4;
      } else if (AM_LIKELY(is_str_n_equal(addr, "ipv6", strlen("ipv6")))) {
        domain = AM_IPV6;
      }
      uint16_t rtp_port[AM_RTP_MEDIA_NUM] = {0};
      memcpy(rtp_port, client_msg.msg_sdp.port, sizeof(rtp_port));
      NOTICE("Received SDP request from %s", hostip.c_str());
      for (uint32_t i = 0; i < AM_RTP_MEDIA_NUM; ++ i) {
        if (AM_LIKELY(strlen(client_msg.msg_sdp.media[i]))) {
          bool found = false;
          std::string media(client_msg.msg_sdp.media[i]);
          std::string media_name("Unknown");
          AMRtpSessionMap::iterator iter;

          switch(i) {
            case AM_RTP_MEDIA_AUDIO: media_name = "Audio"; break;
            case AM_RTP_MEDIA_VIDEO: media_name = "Video"; break;
            default:break;
          }

          NOTICE("Checking %s media type %s",
                 media_name.c_str(), media.c_str());
          client_msg.msg_sdp.media[i][0] = '\0';
          std::transform(media.begin(), media.end(), media.begin(), ::tolower);

          m_session_lock.lock();
          iter = m_session_map.find(media);
          if (AM_LIKELY(iter != m_session_map.end())) {
            AMIRtpSession *&session = iter->second->session();
            AM_RTP_MEDIA_TYPE media_type = AM_RTP_MEDIA_NUM;

            switch(session->type()) {
              case AM_SESSION_AAC:
              case AM_SESSION_OPUS:
              case AM_SESSION_G726:
              case AM_SESSION_SPEEX:
              case AM_SESSION_G711: media_type = AM_RTP_MEDIA_AUDIO; break;
              case AM_SESSION_H264:
              case AM_SESSION_H265:
              case AM_SESSION_MJPEG: media_type = AM_RTP_MEDIA_VIDEO; break;
            }
            if (AM_LIKELY(media_type == AM_RTP_MEDIA_TYPE(i))) {
              std::string &sdp = session->get_sdp(hostip, rtp_port[i], domain);
              strncpy(client_msg.msg_sdp.media[i], sdp.c_str(), sdp.size());
              NOTICE("Get session %s SDP:\n%s",
                     iter->first.c_str(),
                     sdp.c_str());
              found = true;
            }
          }
          m_session_lock.unlock();
          if (AM_UNLIKELY(!found)) {
            memset(client_msg.msg_sdp.media[i], 0, MAX_SDP_LEN);
            NOTICE("Can NOT find media: %s in %s session!",
                   media.c_str(), media_name.c_str());
          }
        }
      }
      ret = send_client_msg(fd, (uint8_t*)&client_msg.msg_sdp,
                            sizeof(client_msg.msg_sdp));
    }break;
    case AM_RTP_CLIENT_MSG_SDPS: {
      std::string payloads;
      std::string sdp_param;
      std::string sdps;
      for (uint8_t i = 0; i < m_rtp_audio_list.size(); ++i) {
        const char *type = audio_type_to_key(m_rtp_audio_list.at(i)->audio_type);
        std::string key = std::string(type) + "-" +
              std::to_string(m_rtp_audio_list.at(i)->sample_rate / 1000) + "k";
        m_session_lock.lock();
        if (m_session_map.find(key) != m_session_map.end()) {
          payloads.append(" ").
              append(m_session_map[key]->session()->get_payload_type());
          sdp_param.append(m_session_map[key]->session()->get_param_sdp());
        } else {
          ERROR("No matched RTP audio session %s was found!", key.c_str());
        }
        m_session_lock.unlock();
      }
      sdps = "m=audio " +
             std::to_string(client_msg.msg_sdps.port[AM_RTP_MEDIA_AUDIO]) +
             " RTP/AVP" + payloads + "\r\n" + sdp_param;
      if (client_msg.msg_sdps.enable_video && !m_rtp_video_list.empty()) {
        payloads.clear();
        sdp_param.clear();
        for (uint8_t i = 0; i < m_rtp_video_list.size(); ++i) {
          std::string key = "video" +
              std::to_string(m_rtp_video_list.at(i)->stream_id);
          m_session_lock.lock();
          if (m_session_map.find(key) != m_session_map.end()) {
            payloads.append(" ").
                append(m_session_map[key]->session()->get_payload_type());
            sdp_param.append(m_session_map[key]->session()->get_param_sdp());
            sdps.append("m=video " +
               std::to_string(client_msg.msg_sdps.port[AM_RTP_MEDIA_VIDEO]) +
               " RTP/AVP" + payloads + "\r\n" + "a=sendonly\r\n" + sdp_param);
          } else {
            ERROR("No matched RTP video session %s was found!", key.c_str());
          }
          m_session_lock.unlock();
        }
      }
      strncpy(client_msg.msg_sdps.media_sdp, sdps.c_str(), MAX_SDP_LEN);
      ret = send_client_msg(fd, (uint8_t*)&client_msg.msg_sdps,
                            sizeof(client_msg.msg_sdps));
      NOTICE("Return the SDPS info to SIP server.");
    }break;
    case AM_RTP_CLIENT_MSG_KILL: {
      if (AM_LIKELY(fd == CTRL_FD_MUXER)) {
        NOTICE("Session is asking to shut down client(SSRC: %08X) "
               "with session ID: %08X",
               client_msg.msg_kill.ssrc,
               client_msg.msg_kill.session_id);
        client_msg.msg_kill.active_kill = m_active_kill;
        ret = send_client_msg(client_msg.msg_kill.fd_from,
                              (uint8_t*)&client_msg.msg_kill,
                              sizeof(client_msg.msg_kill));
        m_active_kill = false;
      } else {
        NOTICE("Client(SSRC: %08X) with session ID: %08X is going to be closed",
               client_msg.msg_kill.ssrc,
               client_msg.msg_kill.session_id);
        m_active_kill = true;
        kill_client(client_msg.msg_kill.ssrc);
      }
    }break;
    case AM_RTP_CLIENT_MSG_INFO: {
      int client_tcp_fd = -1;
      if (AM_LIKELY(client_msg.msg_info.info.send_tcp_fd)) {
        if (AM_UNLIKELY(!recv_fd(fd, &client_tcp_fd))) {
          ERROR("Failed to get client TCP file descriptor!");
          ret = false;
          break;
        }
      }
      std::string rtp_client_conf =
          std::string(MUXER_CONF_DIR) + "/rtp-client.acs";
      AMRtpClientMsgInfo &msg_info = client_msg.msg_info;
      AMRtpSessionMap::iterator iter;
      AMIRtpSession *session = nullptr;

      m_session_lock.lock();
      iter = m_session_map.find(msg_info.info.media);
      session = (iter != m_session_map.end()) ?
          iter->second->session() : nullptr;
      m_session_lock.unlock();
      AMRtpClient *client = session ? AMRtpClient::create(rtp_client_conf,
                                                          fd,
                                                          client_tcp_fd,
                                                          CTRL_FD_CLIENT,
                                                          msg_info.session_id,
                                                          this,
                                                          msg_info.info) :
                                      nullptr;
      NOTICE("Received client information: session_id %08X!",
             msg_info.session_id);
      if (AM_LIKELY(session && client && session->add_client(client))) {
        AMRtpClientMsgControl control(AM_RTP_CLIENT_MSG_SSRC,
                                      client->ssrc(),
                                      msg_info.session_id);
        ret = send_client_msg(fd, (uint8_t*)&control, sizeof(control));
        NOTICE("Send SSRC %08X to client with session_id %08X!",
               client->ssrc(), msg_info.session_id);
      } else {
        if (AM_LIKELY(!session)) {
          ERROR("Cannot find such media type: %s", msg_info.info.media);
        } else if (!client) {
          ERROR("Failed to create client!");
        }
        ret = true;
      }
    }break;
    case AM_RTP_CLIENT_MSG_START: {
      m_session_lock.lock();
      for (AMRtpSessionMap::iterator iter = m_session_map.begin();
           iter != m_session_map.end(); ++ iter) {
        AMIRtpSession *&rtp = iter->second->session();
        std::string seq_ntp = rtp->get_client_seq_ntp(client_msg.msg_ctrl.ssrc);

        if (AM_LIKELY(!seq_ntp.empty())) {
          NOTICE("Session %s is going to enable client with SSRC %08X",
                 rtp->get_session_name().c_str(), client_msg.msg_ctrl.ssrc);
          strncpy(client_msg.msg_ctrl.seq_ntp, seq_ntp.c_str(), seq_ntp.size());
          ret = send_client_msg(fd, (uint8_t*)&client_msg.msg_ctrl,
                                sizeof(client_msg.msg_ctrl));
          if (AM_LIKELY(ret)) {
            rtp->set_client_enable(client_msg.msg_ctrl.ssrc, true);
          }
          break;
        }
      }
      m_session_lock.unlock();
    }break;
    case AM_RTP_CLIENT_MSG_STOP: {
      m_session_lock.lock();
      for (AMRtpSessionMap::iterator iter = m_session_map.begin();
          iter != m_session_map.end(); ++ iter) {
        iter->second->session()->set_client_enable(client_msg.msg_ctrl.ssrc,
                                                 false);
      }
      m_session_lock.unlock();
    }break;
    case AM_RTP_CLIENT_MSG_PROTO: {
      if (AM_LIKELY((client_msg.msg_proto.proto >= 0) &&
                    (client_msg.msg_proto.proto < AM_RTP_CLIENT_PROTO_NUM))) {
        m_client_proto_fd[client_msg.msg_proto.proto] = fd;
        NOTICE("%s server is connected with fd %d!",
               proto_to_string(client_msg.msg_proto.proto),
               m_client_proto_fd[client_msg.msg_proto.proto]);
      } else {
        ERROR("Invalid protocol!");
        ret = false;
      }
    }break;
    case AM_RTP_CLIENT_MSG_CLOSE: {
      if (AM_LIKELY(fd == CTRL_FD_MUXER)) {
        /* This command is from RTP muxer itself, send it to all the client */
        for (uint32_t i = 0; i < AM_RTP_CLIENT_PROTO_NUM; ++ i) {
          if (AM_LIKELY(m_client_proto_fd[i] >= 0)) {
            send_client_msg(m_client_proto_fd[i],
                            (uint8_t*)&client_msg.msg_close,
                            sizeof(client_msg.msg_close));
            NOTICE("Sent MessageClose to %s(%d)",
                   fd_to_proto_string(m_client_proto_fd[i]).c_str(),
                   m_client_proto_fd[i]);
          }
        }
      }
      m_server_run = false;
      NOTICE("Received close message!");
    }break;
    case AM_RTP_CLIENT_MSG_SUPPORT: {
      AMRtpClientMsgSupport &support = client_msg.msg_support;
      support.support_media_map = m_support_media_type;
      send_client_msg(fd, (uint8_t*)&client_msg.msg_support,
                      sizeof(client_msg.msg_support));
      NOTICE("Sent support media type %x to sip_ua_server.",
             support.support_media_map);
    }break;
    case AM_RTP_CLIENT_MSG_SSRC:
    default:break;
  }

  return ret;
}

bool AMMuxerRtp::send_client_msg(int fd, uint8_t *data, uint32_t size)
{
  int ret = 0;
  int count = 0;
  uint32_t send_ret = 0;

  do {
    ret = write(fd, data + send_ret, size - send_ret);
    if (AM_UNLIKELY(ret < 0)) {
      if (AM_LIKELY((errno != EAGAIN) &&
                    (errno != EWOULDBLOCK) &&
                    (errno != EINTR))) {
        uint32_t proto = 0;
        for (proto = 0; proto < AM_RTP_CLIENT_PROTO_NUM; ++ proto) {
          if (AM_LIKELY(fd == m_client_proto_fd[proto])) {
            NOTICE("%s has closed its connection!",
                   proto_to_string(AM_RTP_CLIENT_PROTO(proto)));
            break;
          }
        }
        if (AM_LIKELY(proto == AM_RTP_CLIENT_PROTO_NUM)) {
          ERROR("write: %s, fd is %d", strerror(errno), fd);
        }
        break;
      }
    } else {
      send_ret += ret;
    }
  }while((++ count < 5) && ((send_ret > 0) && (send_ret < size)));

  return (send_ret == size);
}

std::string AMMuxerRtp::fd_to_proto_string(int fd)
{
  std::string str = "Unknown";

  if (AM_LIKELY(fd >= 0)) {
    for (uint32_t i = 0; i < AM_RTP_CLIENT_PROTO_NUM; ++ i) {
      if (AM_LIKELY((fd == m_client_proto_fd[i]))) {
        str = proto_to_string(AM_RTP_CLIENT_PROTO(i));
      }
    }
  }

  return str;
}

AMSessionPlugin* AMMuxerRtp::load_session_plugin(
    const char *name,
    const char *session_name,
    void *data)
{
  AMSessionPlugin *session_plugin = nullptr;
  AMPlugin *so = nullptr;
  AMIRtpSession *session = nullptr;
  std::string session_so = ORYX_MUXER_DIR;
  session_so.append("/rtp-session/rtp-session-").append(name).append(".so");

  do {

    so = AMPlugin::create(session_so.c_str());
    if (AM_UNLIKELY(!so)) {
      ERROR("Failed to load RTP session plugin: rtp-session-%s.so", name);
      break;
    }
    CreateRtpSession create_rtp_session =
        (CreateRtpSession)so->get_symbol(CREATE_RTP_SESSION);
    if (AM_UNLIKELY(!create_rtp_session)) {
      ERROR("Invalid RTP session plugin: %s", session_so.c_str());
      AM_DESTROY(so);
      break;
    }
    session = create_rtp_session(session_name, m_muxer_config, data);
    if (AM_UNLIKELY(!session)) {
      ERROR("Failed to create object of rtp session plugin: %s!",
            session_so.c_str());
      AM_DESTROY(so);
      break;
    }

    session_plugin = AMSessionPlugin::create(so, session);
    if (AM_UNLIKELY(!session_plugin)) {
      ERROR("Failed to allocate memory for RTP SessionPlugin object!");
      delete session;
      AM_DESTROY(so);
      break;
    }
  }while(0);

  return session_plugin;
}
