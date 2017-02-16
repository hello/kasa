/*******************************************************************************
 * am_muxer_export_uds.h
 *
 * History:
 *   2015-01-04 - [Zhi He] created file
 *
 * Copyright (C) 2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/

#ifndef __AM_MUXER_EXPORT_UDS_H__
#define __AM_MUXER_EXPORT_UDS_H__
#include "am_muxer_codec_info.h"

#define DMAX_CACHED_PACKET_NUMBER 64

using std::map;
using std::queue;
using std::pair;
class AMMuxerExportUDS : public AMIMuxerCodec
{
  protected:
    AMMuxerExportUDS();
    virtual ~AMMuxerExportUDS();

  public:
    static AMMuxerExportUDS *create();

  public:
    virtual AM_STATE start();
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
    void main_loop();

  private:
    bool init();
    bool send_info(int client_fd);
    bool send_packet(int client_fd);
    static void thread_entry(void *arg);
    void save_info(AMPacket *packet);
    void fill_export_packet(AMPacket *packet, AMExportPacket *export_packet);
    void clean_resource();
    void reset_resource();

  private:
    int m_export_state;

    int m_socket_fd;
    int m_connect_fd;
    int m_control_fd[2];
    int m_max_fd;

    bool m_running;
    bool m_thread_exit;
    bool m_client_connected;

    AMExportConfig m_config;

    fd_set m_all_set;
    fd_set m_read_set;

    uint32_t m_video_map;
    uint32_t m_audio_map;
    sockaddr_un m_addr;

    AMEvent                          *m_thread_wait;
    AMMutex                          *m_send_mutex;
    AMThread                         *m_accept_thread;
    AMCondition                      *m_send_cond;

    map<uint32_t, AM_VIDEO_INFO>      m_video_infos;
    map<uint32_t, AM_AUDIO_INFO>      m_audio_infos;
    map<uint32_t, AMExportVideoInfo>  m_video_export_infos;
    map<uint32_t, AMExportAudioInfo>  m_audio_export_infos;
    map<uint32_t, AMExportPacket>     m_video_export_packets;
    map<uint32_t, AMExportPacket>     m_audio_export_packets;
    map<uint32_t, bool>               m_video_info_send_flag;
    map<uint32_t, bool>               m_audio_info_send_flag;
    map<uint32_t, queue<AMPacket*>>   m_video_queue;
    map<uint32_t, queue<AMPacket*>>   m_audio_queue;
    queue<AMPacket*>                  m_packet_queue;

    bool                              m_send_block;
    pair<uint32_t, bool>              m_video_send_block;
    pair<uint32_t, bool>              m_audio_send_block;

    uint32_t                          m_audio_pts_increment;
    map<uint32_t, AM_PTS>             m_audio_last_pts;
    map<uint32_t, uint32_t>           m_audio_state;
};
#endif
