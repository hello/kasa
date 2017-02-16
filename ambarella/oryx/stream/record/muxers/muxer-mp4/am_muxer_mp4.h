/*******************************************************************************
 * am_muxer_mp4.h
 *
 * History:
 *   2015-1-9 - [ccjing] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_MUXER_MP4_H_
#define AM_MUXER_MP4_H_
#include "am_muxer_codec_if.h"
#include "am_muxer_mp4_config.h"
#include "am_audio_define.h"
#include "am_video_types.h"
#include <deque>

class AMPacket;
class AMMuxerMp4Builder;
class AMMp4FileWriter;
class AMThread;
struct AMMuxerCodecMp4Config;
class AMMuxerMp4Config;

class AMMuxerMp4 : public AMIMuxerCodec
{
    typedef std::deque<AMPacket*> packet_queue;
  public:
    static AMMuxerMp4* create(const char* config_file);

  public:
    /*interface of AMIMuxerCodec*/
    virtual AM_STATE start();
    virtual AM_STATE stop();
    virtual bool start_file_writing();
    virtual bool stop_file_writing();
    virtual bool is_running();
    virtual AM_MUXER_CODEC_STATE get_state();
    virtual AM_MUXER_ATTR get_muxer_attr();
    virtual void     feed_data(AMPacket* packet);
    virtual AM_STATE set_config(AMMuxerCodecConfig *config);
    virtual AM_STATE get_config(AMMuxerCodecConfig *config);

  private:
    AMMuxerMp4();
    AM_STATE init(const char* config_file);
    virtual ~AMMuxerMp4();

  public:
    void destroy();

  private:
    static void thread_entry(void *p);
    void main_loop();
    AM_MUXER_CODEC_STATE create_resource();
    void release_resource();

  private:
    AM_STATE set_media_sink(char* file_name);
    AM_STATE set_split_duration(uint64_t duration = 0);
    void reset_parameter();
    bool get_current_time_string(char *time_str, int32_t len);

  private:
    AM_STATE on_info_packet(AMPacket* packet);
    AM_STATE on_data_packet(AMPacket* packet);
    AM_STATE on_eos_packet(AMPacket* packet);
    AM_STATE on_eof(AMPacket* packet);//close current file and create new file.

  private:
    AM_AUDIO_INFO          m_audio_info;
    AM_VIDEO_INFO          m_video_info;
    std::string            m_thread_name;
    AMThread              *m_thread;
    AMSpinLock            *m_state_lock;
    AMSpinLock            *m_lock;
    AMSpinLock            *m_file_writing_lock;
    AMMuxerCodecMp4Config *m_muxer_mp4_config;/*do not need to delete*/
    AMMuxerMp4Config      *m_config;
    packet_queue          *m_packet_queue;
    packet_queue          *m_sei_queue;
    char                  *m_config_file;
    AMMuxerMp4Builder     *m_mp4_builder;
    AMMp4FileWriter       *m_file_writer;
    std::string            m_file_location;

    AM_PTS   m_last_video_pts;
    uint64_t m_splitted_duration;
    int64_t  m_next_file_boundary;
    uint32_t m_eos_map;
    uint32_t m_av_info_map;
    uint32_t m_file_video_frame_count;
    uint32_t m_video_frame_count;
    uint16_t m_stream_id;

    AM_MUXER_CODEC_STATE m_state;
    AM_MUXER_ATTR m_muxer_attr;
    AM_AUDIO_TYPE m_event_audio_type;

    bool m_run;
    bool m_audio_enable;
    bool m_is_first_audio;
    bool m_is_first_video;
    bool m_need_splitted;
    bool m_is_new_info;
    bool m_start_file_writing;
};
#endif /* AM_MUXER_MP4_H_ */
