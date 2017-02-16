/*******************************************************************************
 * am_sip_ua_server.cpp
 *
 * History:
 *   2015-1-26 - [Shiming Dong] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
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
bool AMSipUAServer::m_busy = false;

struct ServerCtrlMsg
{
  uint32_t code;
  uint32_t data;
};

AMSipUAServer::MediaInfo::MediaInfo() :
    is_alive(false),
    is_supported(false),
    ssrc(0),
    rtp_port(0),
    session_id(0)
{
  media.clear();
  sdp.clear();
}

static bool is_fd_valid(int fd)
{
  return (fd >= 0) && ((fcntl(fd, F_GETFD) != -1) || (errno != EBADF));
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
  if (AM_LIKELY(m_run == false)) {
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
      ServerCtrlMsg msg;
      msg.code = CMD_STOP_ALL;
      msg.data = 0;
      write(UNIX_SOCK_CTRL_W, &msg, sizeof(msg));
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

void AMSipUAServer::inc_ref()
{
  ++ m_ref_count;
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

    INFO("Current connected client number is %d", m_connected_num);
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
        usernames.append(" " + m_sip_client_que.front()->username);
        if (is_str_equal(m_sip_client_que.front()->username.c_str(),
                         name->user_name)) {
          hang_up(m_sip_client_que.front());
          m_sip_client_que.pop();
          m_connected_num = m_connected_num ? (m_connected_num - 1) : 0;
          find = true;
          break;
        } else {
          SipClientInfo *sip_client = m_sip_client_que.front();
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

AMSipUAServer::AMSipUAServer() :
    m_context(nullptr),
    m_uac_event(nullptr),
    m_answer(nullptr),
    m_ref_count(0),
    m_server_thread(nullptr),
    m_sock_thread(nullptr),
    m_event(nullptr),
    m_event_support(nullptr),
    m_event_sdp(nullptr),
    m_event_sdps(nullptr),
    m_event_ssrc(nullptr),
    m_event_kill(nullptr),
    m_media_type(0),
    m_connected_num(0),
    m_rtp_port(0),
    m_run(false),
    m_unix_sock_run(false),
    m_unix_sock_fd(-1),
    m_media_service_sock_fd(-1),
    m_sip_config(nullptr),
    m_config(nullptr)
{
  UNIX_SOCK_CTRL_R = -1;
  UNIX_SOCK_CTRL_W = -1;
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
    set_media_map();
    signal(SIGPIPE, SIG_IGN);

  } while(0);

  return state;
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
          break;
        }
        delete_invalid_sip_client();

        INFO("Current connected client number is %d", m_connected_num);
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
          break;
       }
       eXosip_unlock(m_context);

       /* create a sip client */
       SipClientInfo *sip_client = new SipClientInfo(m_uac_event->did,
                                                     m_uac_event->cid);
       if (AM_UNLIKELY(!sip_client)) {
         ERROR("create sip client error!");
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
           sip_client->media_info[AM_RTP_MEDIA_AUDIO].is_supported = true;
           INFO("Audio is supported by the caller.");
         } else if (is_str_equal("video",
             ((sdp_media_t*)osip_list_get(&msg_req->m_medias, i))->m_media)) {
             video_con_req = eXosip_get_video_connection(msg_req);
             video_md_req  = eXosip_get_video_media(msg_req);
             sip_client->media_info[AM_RTP_MEDIA_VIDEO].is_supported = true;
             INFO("Video is supported by the caller.");
           }
       }

       if (AM_UNLIKELY(!get_supported_media_types())){
         delete sip_client;
         sdp_message_free(msg_req);
         break;
       }

       /* create audio RTP session info for communication */
       AMRtpClientMsgInfo client_audio_info;
       AMRtpClientInfo &audio_info = client_audio_info.info;
       if(AM_LIKELY(sip_client->media_info[AM_RTP_MEDIA_AUDIO].is_supported)) {
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
         sip_client->media_info[AM_RTP_MEDIA_AUDIO].session_id =
                                           client_audio_info.session_id;
         sip_client->media_info[AM_RTP_MEDIA_AUDIO].media =
                                           std::string(audio_info.media);
         sip_client->media_info[AM_RTP_MEDIA_AUDIO].rtp_port =
                                           audio_info.port_srv_rtp;
       }

       if (AM_UNLIKELY(m_rtp_port == m_sip_config->sip_port)) {
         m_rtp_port = m_rtp_port + 2;
       }

       /* create video RTP session info for communication */
       AMRtpClientMsgInfo client_video_info;
       AMRtpClientInfo &video_info = client_video_info.info;
       if(sip_client->media_info[AM_RTP_MEDIA_VIDEO].is_supported &&
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
         sip_client->media_info[AM_RTP_MEDIA_VIDEO].session_id =
                                           client_video_info.session_id;
         sip_client->media_info[AM_RTP_MEDIA_VIDEO].media =
                                           std::string(video_info.media);
         sip_client->media_info[AM_RTP_MEDIA_VIDEO].rtp_port =
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
       if (AM_LIKELY(sip_client)) {
         m_sip_client_que.push(sip_client);
       } else {
         delete sip_client;
         sdp_message_free(msg_req);
         ERROR("Create sip client error!");
         break;
       }

       if (AM_LIKELY(strlen(audio_info.media) &&
           (!is_str_equal(AM_MEDIA_UNSUPPORTED, audio_info.media)))) {
         if (AM_UNLIKELY(!send_rtp_control_data((uint8_t*)&client_audio_info,
                                                sizeof(client_audio_info)))) {
           ERROR("Send message to RTP muxer error!");
           sdp_message_free(msg_req);
           break;
         } else {
           INFO("Send audio INFO messages to RTP muxer.");
           if (AM_LIKELY(m_event_ssrc->wait(5000))) {
             NOTICE("RTP muxer has returned SSRC info!");
           } else {
             ERROR("Failed to get SSRC info from RTP muxer!");
             sdp_message_free(msg_req);
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
           break;
         } else {
           INFO("Send video INFO messages to RTP muxer.");
           if (AM_LIKELY(m_event_ssrc->wait(5000))) {
             NOTICE("RTP muxer has returned SSRC info!");
           } else {
             ERROR("Failed to get SSRC info from RTP muxer!");
             sdp_message_free(msg_req);
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
       if (m_sip_config->is_register &&
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
         break;
       } else {
         INFO("Send SDP messages to RTP muxer.");
         if (AM_LIKELY(m_event_sdp->wait(5000))) {
           NOTICE("RTP muxer has returned SDP info!");
         } else {
           ERROR("Failed to get SDP info from RTP muxer!");
           sdp_message_free(msg_req);
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
                m_sip_client_que.front()->media_info[AM_RTP_MEDIA_AUDIO].
                sdp.empty() ? "m=audio 0 RTP/AVP 0\r\n" :
                m_sip_client_que.front()->media_info[AM_RTP_MEDIA_AUDIO].
                sdp.c_str(),
                m_sip_client_que.front()->media_info[AM_RTP_MEDIA_VIDEO].
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

      }break;
      case EXOSIP_CALL_ACK: {
        for (uint32_t i = 0; i < m_sip_client_que.size(); ++i) {
          if (m_sip_client_que.front()->dialog_id == m_uac_event->did) {
            if (m_sip_client_que.front()->media_info[AM_RTP_MEDIA_AUDIO].ssrc) {
              AMRtpClientMsgControl audio_ctrl_msg(AM_RTP_CLIENT_MSG_START,
             m_sip_client_que.front()->media_info[AM_RTP_MEDIA_AUDIO].ssrc, 0);
              if (AM_UNLIKELY(!send_rtp_control_data((uint8_t*)&audio_ctrl_msg,
                                                     sizeof(audio_ctrl_msg)))) {
                ERROR("Send control start msg to RTP muxer error: %s",
                      strerror(errno));
                break;
              } else {
                INFO("Send audio start messages to RTP muxer.");
                m_sip_client_que.front()->media_info[AM_RTP_MEDIA_AUDIO].
                    is_alive = true;
              }
            }
            if (m_sip_client_que.front()->media_info[AM_RTP_MEDIA_VIDEO].ssrc) {
              AMRtpClientMsgControl video_ctrl_msg(AM_RTP_CLIENT_MSG_START,
              m_sip_client_que.front()->media_info[AM_RTP_MEDIA_VIDEO].ssrc, 0);
              if (AM_UNLIKELY(!send_rtp_control_data((uint8_t*)&video_ctrl_msg,
                                                     sizeof(video_ctrl_msg)))) {
                ERROR("Send control start msg to RTP muxer error: %s",
                      strerror(errno));
                break;
              } else {
                INFO("Send video start messages to RTP muxer.");
                m_sip_client_que.front()->media_info[AM_RTP_MEDIA_VIDEO].
                    is_alive = true;
              }
            }
            m_sip_client_que.front()->username = std::string(
                               m_uac_event->request->from->url->username);
            m_connected_num++;
            INFO("The connected number is now %d.", m_connected_num);
            if (AM_UNLIKELY(m_rtp_port == m_sip_config->sip_port)) {
              m_rtp_port = m_rtp_port + 2;
            }
            INFO("The RTP port is %d", m_rtp_port);
            break;
          } else {
            SipClientInfo *sip_client = m_sip_client_que.front();
            m_sip_client_que.pop();
            m_sip_client_que.push(sip_client);
          }
        }

        if (!is_str_equal(AM_MEDIA_UNSUPPORTED,
     m_sip_client_que.front()->media_info[AM_RTP_MEDIA_AUDIO].media.c_str())) {
          AMMediaServiceMsgBlock audio_info;
          audio_info.msg_info.base.type = AM_MEDIA_SERVICE_MSG_AUDIO_INFO;
          audio_info.msg_info.base.length = sizeof(AMMediaServiceMsgAudioINFO);
          if(AM_UNLIKELY(!parse_sdp(audio_info.msg_info,
         m_sip_client_que.front()->media_info[AM_RTP_MEDIA_AUDIO].sdp.c_str(),
         m_sip_client_que.front()->media_info[AM_RTP_MEDIA_AUDIO].sdp.size()))) {
            ERROR("Failed to parse sdp information.");
            break;
          }
          if(AM_UNLIKELY(!send_data_to_media_service(audio_info))) {
            ERROR("Failed to send info data to media service.");
            break;
          }
          NOTICE("Send audio info to media service successfully.");
        }
      }break;
      case EXOSIP_CALL_CLOSED: { /* a BYE was received for this call */
        for (uint32_t i = 0; i < m_sip_client_que.size(); i++) {
          if (m_uac_event->did == m_sip_client_que.front()->dialog_id) {
            if (m_sip_client_que.front()->media_info[AM_RTP_MEDIA_AUDIO].
                is_alive) {
              AMRtpClientMsgKill audio_msg_kill(
                 m_sip_client_que.front()->media_info[AM_RTP_MEDIA_AUDIO].ssrc,
                 0, m_unix_sock_fd);
              if (AM_UNLIKELY(!send_rtp_control_data((uint8_t* )&audio_msg_kill,
                                                     sizeof(audio_msg_kill)))) {
                ERROR("Failed to send kill message to RTP Muxer to kill client"
                      "SSRC: %08X!",
                 m_sip_client_que.front()->media_info[AM_RTP_MEDIA_AUDIO].ssrc);
                break;
              } else {
                INFO("Sent kill message to RTP Muxer to kill "
                     "SSRC: %08X!",
                 m_sip_client_que.front()->media_info[AM_RTP_MEDIA_AUDIO].ssrc);
              }

              if (AM_LIKELY(m_event_kill->wait(5000))) {
                NOTICE("Client with SSRC: %08X is killed!",
                m_sip_client_que.front()->media_info[AM_RTP_MEDIA_AUDIO].ssrc);
              } else {
                ERROR("Failed to receive response from RTP muxer!");
              }
            }

            if (m_sip_client_que.front()->media_info[AM_RTP_MEDIA_VIDEO].
                is_alive) {
              AMRtpClientMsgKill video_msg_kill(
                 m_sip_client_que.front()->media_info[AM_RTP_MEDIA_VIDEO].ssrc,
                 0, m_unix_sock_fd);
              if (AM_UNLIKELY(!send_rtp_control_data((uint8_t* )&video_msg_kill,
                                                     sizeof(video_msg_kill)))) {
                ERROR("Failed to send kill message to RTP Muxer to kill client"
                      "SSRC: %08X!",
                 m_sip_client_que.front()->media_info[AM_RTP_MEDIA_VIDEO].ssrc);
                break;
              } else {
                INFO("Sent kill message to RTP Muxer to kill "
                    "SSRC: %08X!",
                 m_sip_client_que.front()->media_info[AM_RTP_MEDIA_VIDEO].ssrc);
              }

              if (AM_LIKELY(m_event_kill->wait(5000))) {
                NOTICE("Client with SSRC: %08X is killed!",
                m_sip_client_que.front()->media_info[AM_RTP_MEDIA_AUDIO].ssrc);
              } else {
                ERROR("Failed to receive response from RTP muxer!");
              }
            }
            delete m_sip_client_que.front();
            m_sip_client_que.pop();
            NOTICE("Hanging up!");
            m_connected_num = m_connected_num ? (m_connected_num - 1) : 0;

            AMMediaServiceMsgBlock stop_msg;
            stop_msg.msg_stop.base.type = AM_MEDIA_SERVICE_MSG_STOP;
            stop_msg.msg_stop.base.length = sizeof(AMMediaServiceMsgSTOP);
            if (AM_UNLIKELY(!send_data_to_media_service(stop_msg))) {
              ERROR("Failed to send stop msg to media service.");
              break;
            }
            NOTICE("Send stop msg to media service successfully.");

            break;
          } else {
            SipClientInfo *sip_client = m_sip_client_que.front();
            m_sip_client_que.pop();
            m_sip_client_que.push(sip_client);
          }
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
        osip_free(str_sdp);

        /* create a sip client */
        SipClientInfo *sip_client = new SipClientInfo(m_uac_event->did,
                                                      m_uac_event->cid);
        if(AM_UNLIKELY(!sip_client)) {
          sdp_message_free(msg_rsp);
          ERROR("Create sip client error!");
          break;
        }
        for(int i = 0; i < msg_rsp->m_medias.nb_elt; ++i) {
          if (is_str_equal("audio",
              ((sdp_media_t*)osip_list_get(&msg_rsp->m_medias, i))->m_media)) {
            audio_con_rsp = eXosip_get_audio_connection(msg_rsp);
            audio_md_rsp  = eXosip_get_audio_media(msg_rsp);
            sip_client->media_info[AM_RTP_MEDIA_AUDIO].is_supported = true;
            INFO("Audio is supported by the callee.");
          } else if (is_str_equal("video",
              ((sdp_media_t*)osip_list_get(&msg_rsp->m_medias, i))->m_media)) {
              video_con_rsp = eXosip_get_video_connection(msg_rsp);
              video_md_rsp  = eXosip_get_video_media(msg_rsp);
              sip_client->media_info[AM_RTP_MEDIA_VIDEO].is_supported = true;
              INFO("Video is supported by the callee.");
            }
        }
        if (AM_UNLIKELY(!get_supported_media_types())){
          delete sip_client;
          sdp_message_free(msg_rsp);
          break;
        }
        /* create audio RTP session info for communication */
       AMRtpClientMsgInfo client_audio_info;
       AMRtpClientInfo &audio_info = client_audio_info.info;
       if(AM_LIKELY(sip_client->media_info[AM_RTP_MEDIA_AUDIO].is_supported)) {
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
         sip_client->media_info[AM_RTP_MEDIA_AUDIO].session_id =
                                           client_audio_info.session_id;
         sip_client->media_info[AM_RTP_MEDIA_AUDIO].media =
                                           std::string(audio_info.media);
         sip_client->media_info[AM_RTP_MEDIA_AUDIO].rtp_port =
                                           audio_info.port_srv_rtp;
       }

       /* create video RTP session info for communication */
       AMRtpClientMsgInfo client_video_info;
       AMRtpClientInfo &video_info = client_video_info.info;
       if(sip_client->media_info[AM_RTP_MEDIA_VIDEO].is_supported &&
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
         sip_client->media_info[AM_RTP_MEDIA_VIDEO].session_id =
                                           client_video_info.session_id;
         sip_client->media_info[AM_RTP_MEDIA_VIDEO].media =
                                           std::string(video_info.media);
         sip_client->media_info[AM_RTP_MEDIA_VIDEO].rtp_port =
                                           video_info.port_srv_rtp;
       }

       /* push sip_client into the sip client queue */
       if (AM_LIKELY(sip_client)) {
         m_sip_client_que.push(sip_client);
       } else {
         delete sip_client;
         sdp_message_free(msg_rsp);
         ERROR("Create sip client error!");
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
           break;
         } else {
           INFO("Send SDP messages to RTP muxer.");
           if (AM_LIKELY(m_event_sdp->wait(5000))) {
             NOTICE("RTP muxer has returned SDP info!");
           } else {
             ERROR("Failed to get SDP info from RTP muxer!");
             sdp_message_free(msg_rsp);
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
                  m_sip_client_que.front()->media_info[AM_RTP_MEDIA_AUDIO].
                  sdp.c_str(),
                  m_sip_client_que.front()->media_info[AM_RTP_MEDIA_VIDEO].
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
           break;
         } else {
           INFO("Send audio INFO messages to RTP muxer.");
           if (AM_LIKELY(m_event_ssrc->wait(5000))) {
             NOTICE("RTP muxer has returned SSRC info!");
           } else {
             ERROR("Failed to get SSRC info from RTP muxer!");
             sdp_message_free(msg_rsp);
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
           break;
         } else {
           INFO("Send video INFO messages to RTP muxer.");
           if (AM_LIKELY(m_event_ssrc->wait(5000))) {
             NOTICE("RTP muxer has returned SSRC info!");
           } else {
             ERROR("Failed to get SSRC info from RTP muxer!");
             sdp_message_free(msg_rsp);
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
         if (m_sip_client_que.front()->dialog_id == m_uac_event->did) {
           if (m_sip_client_que.front()->media_info[AM_RTP_MEDIA_AUDIO].ssrc) {
             AMRtpClientMsgControl audio_ctrl_msg(AM_RTP_CLIENT_MSG_START,
            m_sip_client_que.front()->media_info[AM_RTP_MEDIA_AUDIO].ssrc, 0);
             if (AM_UNLIKELY(!send_rtp_control_data((uint8_t*)&audio_ctrl_msg,
                                                    sizeof(audio_ctrl_msg)))) {
               ERROR("Send control start msg to RTP muxer error: %s",
                     strerror(errno));
               break;
             } else {
               INFO("Send audio start messages to RTP muxer.");
               m_sip_client_que.front()->media_info[AM_RTP_MEDIA_AUDIO].
                   is_alive = true;
             }
           }
           if (m_sip_client_que.front()->media_info[AM_RTP_MEDIA_VIDEO].ssrc) {
             AMRtpClientMsgControl video_ctrl_msg(AM_RTP_CLIENT_MSG_START,
             m_sip_client_que.front()->media_info[AM_RTP_MEDIA_VIDEO].ssrc, 0);
             if (AM_UNLIKELY(!send_rtp_control_data((uint8_t*)&video_ctrl_msg,
                                                    sizeof(video_ctrl_msg)))) {
               ERROR("Send control start msg to RTP muxer error: %s",
                     strerror(errno));
               break;
             } else {
               INFO("Send video start messages to RTP muxer.");
               m_sip_client_que.front()->media_info[AM_RTP_MEDIA_VIDEO].
                   is_alive = true;
             }
           }
           m_sip_client_que.front()->username = std::string(
                                     m_uac_event->request->to->url->username);
           m_connected_num++;
           INFO("The connected number is now %d.", m_connected_num);
           if (AM_UNLIKELY(m_rtp_port == m_sip_config->sip_port)) {
             m_rtp_port = m_rtp_port + 2;
           }
           INFO("The RTP port is %d", m_rtp_port);
           break;
         } else {
           SipClientInfo *sip_client = m_sip_client_que.front();
           m_sip_client_que.pop();
           m_sip_client_que.push(sip_client);
         }
       }
       sdp_message_free(msg_rsp);

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
  TRACE_INITIALIZE (6, nullptr);

  m_context = eXosip_malloc();
  if (AM_UNLIKELY(eXosip_init(m_context))) {
    ret = false;
    ERROR("Couldn't initialize eXosip!");
  }

  if (AM_UNLIKELY(eXosip_listen_addr(m_context, IPPROTO_UDP, nullptr,
                  m_sip_config->sip_port, AF_INET, 0))) {
    ret = false;
    ERROR("Couldn't initialize SIP transport layer!");
  }
  eXosip_set_user_agent(m_context, "Ambarella SIP Service");

  if (AM_LIKELY(m_sip_config->is_register)) {
    ret = register_to_server(m_sip_config->expires);
  }

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

void AMSipUAServer::hang_up(SipClientInfo* sip_client)
{
  int ret = -1;
  do {
    if (AM_UNLIKELY(!sip_client)) {
      ERROR("sip_client is null");
      break;
    }
    /* send stop message to RTP muxer */
    if (sip_client->media_info[AM_RTP_MEDIA_AUDIO].is_alive) {
      AMRtpClientMsgKill audio_msg_kill(
         sip_client->media_info[AM_RTP_MEDIA_AUDIO].ssrc,
         0, m_unix_sock_fd);
      if (AM_UNLIKELY(!send_rtp_control_data((uint8_t* )&audio_msg_kill,
                                             sizeof(audio_msg_kill)))) {
        ERROR("Failed to send kill message to RTP Muxer to kill client"
              "SSRC: %08X!", sip_client->media_info[AM_RTP_MEDIA_AUDIO].ssrc);
      } else {
        INFO("Sent kill message to RTP Muxer to kill SSRC: %08X!",
             sip_client->media_info[AM_RTP_MEDIA_AUDIO].ssrc);
      }

      if (AM_LIKELY(m_event_kill->wait(5000))) {
        NOTICE("Client with SSRC: %08X is killed!",
             sip_client->media_info[AM_RTP_MEDIA_AUDIO].ssrc);
      } else {
        ERROR("Failed to receive response from RTP muxer!");
      }
    }
    if (sip_client->media_info[AM_RTP_MEDIA_VIDEO].is_alive) {
      AMRtpClientMsgKill video_msg_kill(
         sip_client->media_info[AM_RTP_MEDIA_VIDEO].ssrc,
         0, m_unix_sock_fd);
      if (AM_UNLIKELY(!send_rtp_control_data((uint8_t* )&video_msg_kill,
                                             sizeof(video_msg_kill)))) {
        ERROR("Failed to send kill message to RTP Muxer to kill client"
              "SSRC: %08X!", sip_client->media_info[AM_RTP_MEDIA_VIDEO].ssrc);
      } else {
        INFO("Sent kill message to RTP Muxer to kill SSRC: %08X!",
         sip_client->media_info[AM_RTP_MEDIA_VIDEO].ssrc);
      }

      if (AM_LIKELY(m_event_kill->wait(5000))) {
        NOTICE("Client with SSRC: %08X is killed!",
             sip_client->media_info[AM_RTP_MEDIA_VIDEO].ssrc);
      } else {
        ERROR("Failed to receive response from RTP muxer!");
      }
    }

    /* send BYE message to client */
    eXosip_lock (m_context);
    ret = eXosip_call_terminate(m_context, sip_client->call_id,
                                sip_client->dialog_id);
    eXosip_unlock (m_context);
    if (0 != ret) {
      ERROR("hangup/terminate Failed!\n");
    } else {
      NOTICE("Hang up %s successfully!", sip_client->username.c_str());
    }

    delete sip_client;

  }while(0);
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
  }

  eXosip_lock (m_context);
  eXosip_clear_authentication_info(m_context);
  eXosip_add_authentication_info(m_context, m_sip_config->username.c_str(),
                                 m_sip_config->username.c_str(),
                                 m_sip_config->password.c_str(), "md5", nullptr);
  int reg_ret = eXosip_register_send_register(m_context, regid, reg);
  eXosip_unlock (m_context);

  if(AM_UNLIKELY(0 != reg_ret)) {
    ERROR("Register send failed!\n");
    ret = false;
  }

  return ret;
}

void AMSipUAServer::delete_invalid_sip_client()
{
  uint32_t queue_size = m_sip_client_que.size();
  for (uint32_t i = 0; i < queue_size; ++i) {
    if (AM_UNLIKELY(!m_sip_client_que.front()->media_info[AM_RTP_MEDIA_AUDIO].
        is_alive && !m_sip_client_que.front()->media_info[AM_RTP_MEDIA_VIDEO].
        is_alive)) {
      delete m_sip_client_que.front();
      m_sip_client_que.pop();
      NOTICE("Delete an invalid sip client in the queue!");
    } else {
      SipClientInfo *sip_client = m_sip_client_que.front();
      m_sip_client_que.pop();
      m_sip_client_que.push(sip_client);
    }
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
        uint32_t read_cnt = 0;
        ssize_t read_ret = 0;
        do {
          read_ret = read(UNIX_SOCK_CTRL_R, &msg, sizeof(msg));
        }while ((++ read_cnt < 5) && ((read_ret == 0) || ((read_ret < 0) &&
                ((errno == EAGAIN) || (errno == EWOULDBLOCK) ||
                 (errno == EINTR)))));

        if (AM_UNLIKELY(read_ret < 0)) {
          PERROR("read");
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
          case AM_MEDIA_NET_OK:
            break;
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

int AMSipUAServer::unix_socket_conn(int fd, const char *server_name)
{
  bool isok = true;
  do {
    socklen_t len = 0;
    sockaddr_un un;
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
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
      INFO("SIP.unix connect to RTP Muxer successfully!");
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
              if ((uint32_t)m_sip_client_que.front()->dialog_id ==
                  union_msg.msg_sdp.session_id) {
                m_sip_client_que.front()->media_info[AM_RTP_MEDIA_AUDIO].sdp.
                    clear();
                m_sip_client_que.front()->media_info[AM_RTP_MEDIA_AUDIO].sdp =
                    std::string(union_msg.msg_sdp.media[AM_RTP_MEDIA_AUDIO]);

                m_sip_client_que.front()->media_info[AM_RTP_MEDIA_VIDEO].sdp.
                    clear();
                m_sip_client_que.front()->media_info[AM_RTP_MEDIA_VIDEO].sdp =
                    std::string(union_msg.msg_sdp.media[AM_RTP_MEDIA_VIDEO]);
                m_event_sdp->signal();
                break;
              } else {
                SipClientInfo *client_info = m_sip_client_que.front();
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
                  if (m_sip_client_que.front()->media_info[j].ssrc
                      == union_msg.msg_ctrl.ssrc) {
                    m_sip_client_que.front()->media_info[j].is_alive = false;
                    if (!m_sip_client_que.front()->media_info
                        [AM_RTP_MEDIA_AUDIO].is_alive && !m_sip_client_que.
                        front()->media_info[AM_RTP_MEDIA_VIDEO].is_alive) {
                      NOTICE("Client is invalid, remove it from the queue!");
                      delete m_sip_client_que.front();
                      m_sip_client_que.pop();
                      m_connected_num = m_connected_num ? (m_connected_num - 1)
                                        : 0;
                      INFO("Current connected number is %d", m_connected_num);
                    }
                    is_found = true;
                    break;
                  }
                }
                if(is_found) {
                  break;
                } else {
                  SipClientInfo *client_info = m_sip_client_que.front();
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
                if (m_sip_client_que.front()->media_info[j].session_id ==
                    msg_ctrl.session_id) {
                  m_sip_client_que.front()->media_info[j].ssrc = msg_ctrl.ssrc;
                  NOTICE("get SSRC %X of %s",msg_ctrl.ssrc,
                         m_sip_client_que.front()->media_info[j].media.c_str());
                  is_found = true;
                  m_event_ssrc->signal();
                  break;
                }
              }
              if(is_found) {
                break;
              } else {
                SipClientInfo *client_info = m_sip_client_que.front();
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
      media_type = "g711";
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

bool AMSipUAServer::parse_sdp(AMMediaServiceMsgAudioINFO& info,
                              const char *audio_sdp, int32_t size)
{
  bool ret = true;
  std::string audio_type_indicator;
  const char *audio_fmt_indicator = "rtpmap:";
  const char *udp_port_indicator = "audio";
  const char *ip_domain_indicator = "c=IN IP";
  const char *indicator_uri = nullptr;
  char *end_ptr = nullptr;
  indicator_uri = (const char*) memmem(audio_sdp, size,
                   udp_port_indicator, strlen(udp_port_indicator));
  do {
    if(AM_LIKELY(indicator_uri)) {
      if(AM_LIKELY(strlen(indicator_uri) > strlen(udp_port_indicator))) {
        info.udp_port = strtol(indicator_uri + strlen(udp_port_indicator),
                          &end_ptr, 10);
        if(AM_LIKELY(info.udp_port > 0 &&  end_ptr)) {
          INFO("Get udp port from sdp, udp_port = %d", info.udp_port);
        } else {
          ERROR("Strtol udp_port error.");
          ret = false;
          break;
        }
      } else {
        ERROR("memmem udp port error.");
        ret = false;
        break;
      }
    } else {
      ERROR("Not find \"audio\" in the sdp: %s", audio_sdp);
      ret = false;
      break;
    }
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
    info.audio_format = AM_AUDIO_TYPE(m_media_map[audio_type_indicator.c_str()]);
    indicator_uri = (const char*) memmem(audio_sdp, size,
                                         audio_type_indicator.c_str(),
                                         audio_type_indicator.size());
    if(AM_LIKELY(indicator_uri)) {
      if(AM_LIKELY(strlen(indicator_uri) > audio_type_indicator.size())) {
        info.sample_rate = strtol(indicator_uri +
                         audio_type_indicator.size() + 1, &end_ptr, 10);
        if(AM_LIKELY(info.sample_rate > 0 &&  end_ptr)) {
          INFO("Get audio sample rate from sdp, sample rate = %d",
               info.sample_rate);
        } else {
          ERROR("Strtol sample rate error.");
          ret = false;
          break;
        }
        indicator_uri = end_ptr + 1;
        info.channel = strtol(indicator_uri, &end_ptr, 10);
        if(AM_LIKELY(info.channel > 0 && end_ptr)) {
          INFO("Get audio channel from sdp, channel = %d", info.channel);
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
    indicator_uri = (const char*)memmem(audio_sdp, size,
                                        ip_domain_indicator,
                                        strlen(ip_domain_indicator));
    if(AM_LIKELY(indicator_uri)) {
      if(AM_LIKELY(strlen(indicator_uri) > strlen(ip_domain_indicator))) {
        info.ip_domain = AM_PLAYBACK_IP_DOMAIN(strtol(indicator_uri +
                                strlen(ip_domain_indicator), &end_ptr, 10));
        if(AM_LIKELY(end_ptr)) {
          INFO("Get ip domain from sdp, ip domain is IPV%d",
               info.ip_domain);
        } else {
          ERROR("Strtol ip domain error.");
          ret = false;
          break;
        }
      }
    } else {
      ERROR("Not find %s.", ip_domain_indicator);
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

bool AMSipUAServer::send_data_to_media_service(AMMediaServiceMsgBlock& send_msg)
{
  bool ret = true;
  int count = 0;
  uint32_t send_ret = 0;
  if(!is_fd_valid(m_media_service_sock_fd)) {
    ERROR("m_media_service_sock_fd is not valid. "
        "Can not send data to media service.");
    return false;
  }
  AMMediaServiceMsgBase *base = (AMMediaServiceMsgBase*)&send_msg;
  while((++ count < 5) && (base->length !=
      (send_ret = write(m_media_service_sock_fd, &send_msg, base->length)))) {
    if(AM_LIKELY((errno != EAGAIN) &&
                 (errno != EWOULDBLOCK) &&
                 (errno != EINTR))) {
      ret = false;
      PERROR("write");
      break;
    }
  }
  return (ret && (send_ret == base->length) && (count < 5));
}

int AMSipUAServer::create_unix_socket_fd(const char *socket_name)
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
