/*
 * am_muxer_ts.h
 *
 * 11/09/2012 [Hanbo Xiao] [Created]
 * 17/12/2014 [Chengcai Jing] [Modified]
 * Copyright (C) 2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __AM_MUXER_TS_H__
#define __AM_MUXER_TS_H__

#include "am_muxer_codec_if.h"
#include "am_muxer_ts_config.h"
#include <deque>

struct AM_TS_PID_INFO
{
    uint16_t PMT_PID;
    uint16_t VIDEO_PES_PID;
    uint16_t AUDIO_PES_PID;
    uint16_t reserved;
};

struct PACKET_BUF
{
    uint16_t pid;
    uint8_t  buf[MPEG_TS_TP_PACKET_SIZE];
};

enum
{
  TS_PACKET_SIZE = MPEG_TS_TP_PACKET_SIZE,
  TS_DATA_BUFFER_SIZE = MPEG_TS_TP_PACKET_SIZE * 1000,
  MAX_CODED_AUDIO_FRAME_SIZE = 8192,
  AUDIO_CHUNK_BUF_SIZE = MAX_CODED_AUDIO_FRAME_SIZE + MPEG_TS_TP_PACKET_SIZE * 4
};

enum
{
  TS_AUDIO_PACKET = 0x1,
  TS_VIDEO_PACKET = 0x2,
};

struct AMPacket;
struct AMMuxerCodecTSConfig;
class AMTsFileWriter;
class AMMuxerTsConfig;

class AMTsMuxer : public AMIMuxerCodec
{
    typedef std::deque<AMPacket*> packet_queue;
  public:
    static AMIMuxerCodec *create(const char* config_file);

  private:
    AMTsMuxer();
    AM_STATE init(const char* config_file);
    virtual ~AMTsMuxer();

    /* Interface of AMIMuxerCodec*/
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
  private:
    static void thread_entry(void* p);
    void main_loop();
    AM_MUXER_CODEC_STATE create_resource();
    void release_resource();

  public:
    void destroy();

  private:
    AM_STATE set_media_sink(char* file_name);
    AM_STATE set_split_duration(uint64_t duration_in90K_base = 0);

  private:
    AM_STATE on_eos_packet(AMPacket *);
    AM_STATE on_eof_packet(AMPacket *);
    AM_STATE on_data_packet(AMPacket *);
    AM_STATE on_info_packet(AMPacket *);
    AM_STATE init_ts();
    void reset_parameter();
    inline AM_STATE build_audio_ts_packet(uint8_t *data, uint32_t size, AM_PTS pts);
    inline AM_STATE build_and_flush_audio(AM_PTS pts);
    inline AM_STATE build_video_ts_packet(uint8_t *data, uint32_t size, AM_PTS pts);
    inline AM_STATE build_pat_pmt();
    inline AM_STATE update_pat_pmt();
    //inline void pcr_sync(AM_PTS);
    inline void pcr_increment(int64_t pts);
    inline void pcr_calc_pkt_duration(uint32_t rate, uint32_t scale);

    inline AM_STATE set_audio_info(AM_AUDIO_INFO* audio_info);
    inline AM_STATE set_video_info(AM_VIDEO_INFO* video_info);
    bool get_current_time_string(char *time_str, int32_t len);

  private:
    AM_VIDEO_INFO         m_h264_info;
    AM_AUDIO_INFO         m_audio_info;
    PACKET_BUF            m_pat_buf;
    PACKET_BUF            m_pmt_buf;
    PACKET_BUF            m_video_pes_buf;
    PACKET_BUF            m_audio_pes_buf;
    std::string           m_thread_name;
    std::string           m_file_location;
    AMThread             *m_thread;
    AMSpinLock           *m_lock;
    AMSpinLock           *m_state_lock;
    AMSpinLock           *m_file_writing_lock;
    AMMuxerCodecTSConfig *m_muxer_ts_config;/*do not need to delete*/
    AMMuxerTsConfig      *m_config;
    packet_queue         *m_packet_queue;
    AMTsBuilder          *m_ts_builder;
    AMTsFileWriter       *m_file_writer;


    AM_TS_MUXER_PSI_PAT_INFO    m_pat_info;
    AM_TS_MUXER_PSI_PMT_INFO    m_pmt_info;
    AM_TS_MUXER_PSI_PRG_INFO    m_program_info;
    AM_TS_MUXER_PSI_STREAM_INFO m_video_stream_info;
    AM_TS_MUXER_PSI_STREAM_INFO m_audio_stream_info;

    AM_PTS   m_last_video_pts;
    AM_PTS   m_pcr_base;
    AM_PTS   m_pcr_inc_base;
    uint64_t m_splitted_duration;
    int64_t  m_next_file_boundary;
    int64_t  m_pts_base_video;
    int64_t  m_pts_base_audio;
    uint32_t m_av_info_map;
    uint32_t m_eos_map;
    uint32_t m_audio_chunk_buf_avail;
    uint32_t m_file_video_frame_count;
    uint32_t m_video_frame_count;

    uint16_t m_stream_id;
    uint16_t m_pcr_ext;
    uint16_t m_pcr_inc_ext;
    uint8_t  m_lpcm_descriptor[8];
    uint8_t  m_pmt_descriptor[4];
    uint8_t *m_audio_chunk_buf;
    uint8_t *m_audio_chunk_buf_wr_ptr;
    char    *m_config_file;

    AM_AUDIO_TYPE        m_event_audio_type;
    AM_MUXER_ATTR        m_muxer_attr;
    AM_MUXER_CODEC_STATE m_state;

    bool m_run;
    bool m_audio_enable;
    bool m_is_first_audio;
    bool m_is_first_video;
    bool m_need_splitted;
    bool m_event_flag;
    bool m_event_normal_sync;
    bool m_is_new_info;
    bool m_start_file_writing;
};

#endif
