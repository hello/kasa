/*******************************************************************************
 * am_muxer_rtp.h
 *
 * History:
 *   2015-1-4 - [ypchang] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
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

#include <vector>
#include <queue>
#include <map>

class AMEvent;
class AMPacket;
class AMThread;
class AMSpinLock;
class AMRtpClient;
class AMIRtpSession;
class AMMuxerRtpConfig;

struct MuxerRtpConfig;

class AMMuxerRtp: public AMIMuxerCodec
{
  struct SupportAudioInfo
  {
    AM_AUDIO_TYPE audio_type;
    SupportAudioInfo() :
      audio_type(AM_AUDIO_NULL)
    {}
  };
  struct SupportVideoInfo
  {
    AM_VIDEO_TYPE video_type;
    uint32_t      stream_id;
    SupportVideoInfo() :
      video_type(AM_VIDEO_NULL),
      stream_id(-1)
    {}
  };

    typedef std::queue<uint32_t> AMRtpSSRCQ;
    typedef std::queue<AMPacket*> AMRtpPacketQ;
    typedef std::vector<AMIRtpSession*> AMRtpSessionList;
    typedef std::map<std::string, AMIRtpSession*> AMRtpSessionMap;
    typedef std::vector<SupportAudioInfo*> AMRtpAudioList;
    typedef std::vector<SupportVideoInfo*> AMRtpVideoList;

  public:
    static AMMuxerRtp* create(const char *conf_file);

  public:
    AM_STATE start();
    virtual AM_STATE stop();
    virtual bool start_file_writing();
    virtual bool stop_file_writing();
    virtual bool is_running();
    virtual AM_STATE set_config(AMMuxerCodecConfig *config);
    virtual AM_STATE get_config(AMMuxerCodecConfig *config);
    virtual AM_MUXER_ATTR get_muxer_attr();
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

    void work_thread();
    void server_thread();
    void clear_client_kill_queue();
    void clear_all_resources();
    void kill_client_by_ssrc(uint32_t ssrc);
    void close_all_connections();
    int rtp_unix_socket(const char *name);
    AM_NET_STATE recv_client_msg(int fd, AMRtpClientMsgBlock *client_msg);
    bool process_client_msg(int fd, AMRtpClientMsgBlock &client_msg);
    bool send_client_msg(int fd, uint8_t *data, uint32_t size);
    std::string fd_to_proto_string(int fd);

  private:
    MuxerRtpConfig      *m_muxer_config; /* No need to delete */
    AMMuxerRtpConfig    *m_config;
    AMSpinLock          *m_lock;
    AMEvent             *m_event_work;
    AMEvent             *m_event_server;
    AMThread            *m_thread_work;
    AMThread            *m_thread_server;
    std::string          m_unix_sock_name;
    AMRtpClientMsgBlock  m_client_msg;
    AMRtpPacketQ         m_packet_q;
    AMRtpSSRCQ           m_client_kill_q;
    AMRtpSessionMap      m_session_map;
    bool                 m_run;
    bool                 m_work_run;
    bool                 m_server_run;
    bool                 m_active_kill;
    AM_MUXER_CODEC_STATE m_muxer_state;
    int                  m_client_proto_fd[AM_RTP_CLIENT_PROTO_NUM];
    int                  m_client_ctrl_fd[2];
    int                  m_rtp_unix_fd;
    uint32_t             m_support_media_type;
    AMRtpAudioList       m_rtp_audio_list;
    AMRtpVideoList       m_rtp_video_list;
#define CTRL_FD_CLIENT m_client_ctrl_fd[0]
#define CTRL_FD_MUXER  m_client_ctrl_fd[1]
};

#endif /* ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_MUXER_RTP_H_ */
