/*******************************************************************************
 * am_muxer_rtp.h
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
#ifndef ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_MUXER_RTP_H_
#define ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_MUXER_RTP_H_

#include "am_amf_types.h"
#include "am_muxer_codec_if.h"
#include "am_muxer_codec_info.h"
#include "am_rtp_msg.h"
#include "am_audio_define.h"
#include "am_video_types.h"
#include "am_queue.h"
#include "am_mutex.h"

#include <atomic>
#include <vector>
#include <map>

class AMEvent;
class AMTimer;
class AMPacket;
class AMThread;
class AMPlugin;
class AMRtpClient;
class AMIRtpSession;
class AMMuxerRtpConfig;

struct MuxerRtpConfig;

struct SessionDeleteData
{
    AMMuxerRtp *muxer;
    std::string key;
    SessionDeleteData(AMMuxerRtp *m, const std::string &k) :
      muxer(m),
      key(k)
    {
    }
};
typedef std::map<std::string, SessionDeleteData*> SessionDeleteMap;

class AMSessionPlugin
{
  public:
    static AMSessionPlugin* create(AMPlugin *so, AMIRtpSession *session);

  public:
    AMIRtpSession*& session();
    void destroy(bool notify = true);
    void add_ref();

  private:
    AMSessionPlugin(AMPlugin *so, AMIRtpSession *session);
    AMSessionPlugin(const AMSessionPlugin &a) = delete;
    AMSessionPlugin() = delete;
    virtual ~AMSessionPlugin();
  private:
    AMPlugin        *m_so        = nullptr;
    AMIRtpSession   *m_session   = nullptr;
    std::atomic<int> m_ref_count = {0};
    AMMemLock        m_lock;
};

class AMMuxerRtp: public AMIMuxerCodec
{
  struct SupportAudioInfo
  {
    AM_AUDIO_TYPE audio_type = AM_AUDIO_NULL;
    uint32_t sample_rate = 0;
  };
  struct SupportVideoInfo
  {
    AM_VIDEO_TYPE video_type = AM_VIDEO_NULL;
    uint32_t      stream_id = -1;
  };
  typedef AMSafeQueue<uint32_t> AMRtpSSRCQ;
  typedef AMSafeQueue<AMPacket*> AMRtpPacketQ;
  typedef std::vector<AMIRtpSession*> AMRtpSessionList;
  typedef std::map<std::string, AMSessionPlugin*> AMRtpSessionMap;
  typedef std::vector<SupportAudioInfo*> AMRtpAudioList;
  typedef std::vector<SupportVideoInfo*> AMRtpVideoList;

  public:
    static AMMuxerRtp* create(const char *conf_file);

  public:
    AM_STATE start();
    virtual AM_STATE stop();
    virtual const char* name();
    virtual void* get_muxer_codec_interface(AM_REFIID refiid);
    virtual bool is_running();
    virtual AM_STATE set_config(AMMuxerCodecConfig *config);
    virtual AM_STATE get_config(AMMuxerCodecConfig *config);
    virtual AM_MUXER_ATTR get_muxer_attr();
    virtual uint8_t get_muxer_codec_stream_id();
    virtual uint32_t get_muxer_id();
    virtual AM_MUXER_CODEC_STATE get_state();
    virtual void feed_data(AMPacket* packet);

  public:
    void kill_client(uint32_t ssrc);

  private:
    AMMuxerRtp();
    virtual ~AMMuxerRtp();
    AM_STATE init(const char *conf);

  private:
    static void static_work_thread(void *data);
    static void static_server_thread(void *data);
    static bool static_timeout_kill_session(void *data);

    void work_thread();
    void server_thread();
    void kill_session(const std::string &key);
    void timer_kill_session(const std::string &key);
    void stop_timer_kill_session(const std::string &key);
    void remove_session_del_data(const std::string &key,
                                 SessionDeleteData *data);
    void clear_client_kill_queue();
    void clear_all_resources();
    void kill_client_by_ssrc(uint32_t ssrc);
    void close_all_connections();
    int rtp_unix_socket(const char *name);
    AM_NET_STATE recv_client_msg(int fd, AMRtpClientMsgBlock *client_msg);
    bool process_client_msg(int fd, AMRtpClientMsgBlock &client_msg);
    bool send_client_msg(int fd, uint8_t *data, uint32_t size);
    std::string fd_to_proto_string(int fd);
    AMSessionPlugin* load_session_plugin(const char *name,
                                         const char *session_name,
                                         void *data);

  private:
    MuxerRtpConfig      *m_muxer_config; /* No need to delete */
    AMMuxerRtpConfig    *m_config;
    AMTimer             *m_timer;
    AMEvent             *m_event_work;
    AMEvent             *m_event_server;
    AMThread            *m_thread_work;
    AMThread            *m_thread_server;
    AM_MUXER_CODEC_STATE m_muxer_state;
    int                  m_client_proto_fd[AM_RTP_CLIENT_PROTO_NUM];
    int                  m_client_ctrl_fd[2];
    int                  m_rtp_unix_fd;
    uint32_t             m_support_media_type;
    bool                 m_active_kill;
    std::atomic<bool>    m_run;
    std::atomic<bool>    m_work_run;
    std::atomic<bool>    m_server_run;
    std::string          m_unix_sock_name;
    AMRtpClientMsgBlock  m_client_msg;
    AMMemLock            m_lock;
    AMMemLock            m_session_lock;
    AMMemLock            m_session_data_lock;
    AMRtpPacketQ         m_packet_q;
    AMRtpSSRCQ           m_client_kill_q;
    AMRtpSessionMap      m_session_map;
    SessionDeleteMap     m_session_del_map;
    AMRtpAudioList       m_rtp_audio_list;
    AMRtpVideoList       m_rtp_video_list;
#define CTRL_FD_CLIENT m_client_ctrl_fd[0]
#define CTRL_FD_MUXER  m_client_ctrl_fd[1]
};

#endif /* ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_MUXER_RTP_H_ */
