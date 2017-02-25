/*******************************************************************************
 * am_sip_ua_server.h
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

#ifndef AM_SIP_UA_SERVER_H_
#define AM_SIP_UA_SERVER_H_

#include "am_sip_ua_server_if.h"
#include "eXosip2/eXosip.h"
#include "osip2/osip_mt.h"
#include "osipparser2/osip_message.h"
#include "osipparser2/osip_parser.h"
#include "am_media_service_msg.h"
#include "am_jitter_buffer.h"

#include <atomic>
#include <mutex>
#include <map>

#include <string>
#include <queue>

const char* AM_MEDIA_UNSUPPORTED = "unsupported";
#define DEFAULT_IO_RETRY_COUNT 5
#define SOCKET_BUFFER_SIZE 8192
#define  SOCK_CTRL_R  m_ctrl_fd[0]
#define  SOCK_CTRL_W  m_ctrl_fd[1]

class AMEvent;
class AMMutex;
class AMThread;

struct SipUAServerConfig;
struct AMMediaServiceMsgINFO;

typedef std::map<std::string, int> SipMediaMap;

#define DEFAULT_AUDIO_FRAME_MS 160
#define DEFAULT_RECV_RTP_PACKET_SIZE 1024
#define DEFAULT_MAX_JITTERBUFFER 100000
#define DEFAULT_RESYNCH_THRESHOLD -1
#define DEFAULT_MAX_CONTIG_INTERP 10

class AMSipClientInfo
{
  struct MediaInfo
  {
    uint32_t    ssrc           = 0;
    uint32_t    session_id     = 0;
    uint16_t    rtp_port       = 0;
    bool        is_supported   = false;
    std::atomic<bool> is_alive = {false};
    std::string media;
    std::string sdp;
  };
  public:
    bool init(bool jitter_buffer, uint16_t frames_remain);
    bool setup_rtp_client_socket();
    static void start_process_rtp_thread(void *data);
    void process_rtp_thread();
    AMSipClientInfo() :
      m_dialog_id(-1),
      m_call_id(-1)
    { SOCK_CTRL_R = -1; SOCK_CTRL_W = -1; }
    AMSipClientInfo(int dialog, int call) :
      m_dialog_id(dialog),
      m_call_id(call)
    { SOCK_CTRL_R = -1; SOCK_CTRL_W = -1; }
    ~AMSipClientInfo();
  public:
    AMJitterBuffer *m_jitterbuf   = nullptr;
    AMThread   *m_rtp_thread      = nullptr;
    uint32_t    m_audio_frame_len = DEFAULT_AUDIO_FRAME_MS;
    int         m_playback_id     = -1;
    int         m_recv_rtp_fd     = -1;
    int         m_send_rtp_uds_fd = -1;
    int         m_ctrl_fd[2];
    int         m_dialog_id;
    int         m_call_id;
    std::atomic<bool> m_running   = {false};
    std::string m_username;
    std::string m_uds_server_name;
    std::string m_uds_client_name;
    MediaInfo   m_media_info[AM_RTP_MEDIA_NUM];
};

class AMSipUAServer: public AMISipUAServer
{
  enum {
    WAIT_TIMER  = 300,
    REG_TIMER   = 300*1000,
    BUSY_TIMER  = 10,
  };

  typedef std::queue<AMSipClientInfo*> SipClientInfoQue;

  public:
    static AMISipUAServer* get_instance();
    virtual bool start();
    virtual void stop();
    virtual uint32_t version();
    virtual bool set_sip_registration_parameter(AMSipRegisterParameter*);
    virtual bool set_sip_config_parameter(AMSipConfigParameter*);
    virtual bool set_sip_media_priority_list(AMMediaPriorityList*);
    virtual bool initiate_sip_call(AMSipCalleeAddress *address);
    virtual bool hangup_sip_call(AMSipHangupUsername *name);

  protected:
    virtual void inc_ref();
    virtual void release();

  public:
    void hang_up_all();
    void hang_up(AMSipClientInfo* sip_client);
    bool register_to_server(int expires);
    static void busy_status_timer(int arg);
    void delete_invalid_sip_client();

  private:
    AMSipUAServer() {};
    virtual ~AMSipUAServer();
    bool construct();

    uint32_t get_random_number();
    bool init_sip_ua_server();
    bool get_supported_media_types();

    bool on_sip_call_invite_recv();
    bool on_sip_call_ack_recv();
    bool on_sip_call_closed_recv();
    bool on_sip_call_answered_recv();

    bool start_sip_ua_server_thread();
    static void static_sip_ua_server_thread(void *data);
    void sip_ua_server_thread();

    bool start_connect_unix_thread();
    static void static_unix_thread(void *data);
    void connect_unix_thread();
    void release_resource();

    bool recv_rtp_control_data();
    bool send_rtp_control_data(uint8_t *data, size_t len);
    const char* select_media_type(uint32_t uas_type, sdp_media_t *uac_type);
    void set_media_map();
    bool parse_sdp(AMMediaServiceMsgUdsURI& uri, const char *audio_sdp,
                   int32_t size);
    AM_MEDIA_NET_STATE recv_data_from_media_service(
                                 AMMediaServiceMsgBlock& service_msg);
    bool process_msg_from_media_service(AMMediaServiceMsgBlock *service_msg);
    bool send_data_to_media_service(AMMediaServiceMsgBlock& send_msg);

  private:
    static AMSipUAServer    *m_instance;
    static std::mutex        m_lock;
    static std::atomic<bool> m_busy;
    eXosip_t             *m_context               = nullptr;
    eXosip_event_t       *m_uac_event             = nullptr;
    osip_message         *m_answer                = nullptr;
    AMThread             *m_server_thread         = nullptr;
    AMThread             *m_sock_thread           = nullptr;
    AMEvent              *m_event                 = nullptr;
    AMEvent              *m_event_support         = nullptr;
    AMEvent              *m_event_sdp             = nullptr;
    AMEvent              *m_event_sdps            = nullptr;
    AMEvent              *m_event_ssrc            = nullptr;
    AMEvent              *m_event_kill            = nullptr;
    AMEvent              *m_event_playid          = nullptr;
    SipUAServerConfig    *m_sip_config            = nullptr;
    AMSipUAServerConfig  *m_config                = nullptr;

    int                   m_ctrl_unix_fd[2]       = {-1};
    uint32_t              m_media_type            = 0;
    int                   m_unix_sock_fd          = -1;
    int                   m_media_service_sock_fd = -1;
    uint16_t              m_connected_num         = 0;
    uint16_t              m_rtp_port              = 0;
    std::atomic<bool>     m_run                   = {false};
    std::atomic<bool>     m_unix_sock_run         = {false};

    std::atomic_int       m_ref_count             = {0};
    std::string           m_sdps;
    std::string           m_config_file;

    SipClientInfoQue      m_sip_client_que;
    SipMediaMap           m_media_map;

};

#endif /* AM_SIP_UA_SERVER_H_ */
