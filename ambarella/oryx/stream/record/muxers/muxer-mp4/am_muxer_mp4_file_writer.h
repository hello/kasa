/*******************************************************************************
 * am_muxer_mp4_file_writer.h
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
#ifndef AM_MUXER_MP4_FILE_WRITER_H_
#define AM_MUXER_MP4_FILE_WRITER_H_
#include "am_video_types.h"
class AMIFileWriter;
struct AMMuxerCodecMp4Config;

class AMMp4FileWriter
{
  public:
    static AMMp4FileWriter* create(AMMuxerCodecMp4Config *muxer_mp4_config,
                                   AM_VIDEO_INFO *video_info);

  private:
    AMMp4FileWriter();
    AM_STATE init(AMMuxerCodecMp4Config *muxer_mp4_config,
                  AM_VIDEO_INFO *video_info);
    virtual ~AMMp4FileWriter();

  public:
    void destroy();
    AM_STATE deinit();
    AM_STATE on_eof();
    AM_STATE on_eos();
    AM_STATE set_media_sink(const char* file_name);
    AM_STATE write_data(uint8_t* data, uint32_t data_len);
    AM_STATE write_data_direct(uint8_t *data, uint32_t data_len);
    AM_STATE seek_data(uint32_t offset, uint32_t whence); //SEEK_SET ...
    bool set_file_offset(uint32_t offset, uint32_t whence);
    bool  is_file_open();
    AM_STATE write_u8(uint8_t value);
    AM_STATE write_s8(int8_t value);
    AM_STATE write_be_u16(uint16_t value);
    AM_STATE write_be_s16(int16_t value);
    AM_STATE write_be_u24(uint32_t value);
    AM_STATE write_be_s24(int32_t value);
    AM_STATE write_be_u32(uint32_t value);
    AM_STATE write_be_s32(int32_t value);
    AM_STATE write_be_u64(uint64_t value);
    AM_STATE write_be_s64(int64_t value);
    uint64_t get_file_offset();
    AM_STATE create_next_split_file();
    AM_STATE create_m3u8_file();
    AM_STATE close_file();
    void set_begin_packet_pts(int64_t pts);
    void set_end_packet_pts(int64_t pts);
  private:

  private:
    uint64_t       m_cur_file_offset;
    int64_t        m_begin_packet_pts;
    int64_t        m_end_packet_pts;
    int64_t        m_file_duration;
    uint32_t       m_max_file_number;
    uint32_t       m_file_target_duration;
    uint32_t       m_hls_sequence;
    int             m_file_counter;
    int             m_dir_counter;
    char           *m_file_name;
    char           *m_tmp_file_name;
    char           *m_path_name;
    char           *m_base_name;
    AMIFileWriter *m_file_writer;
    AMMuxerCodecMp4Config *m_muxer_mp4_config;
    AM_VIDEO_INFO          *m_video_info;
};

#endif /* AM_MUXER_MP4_FILE_WRITER_H_ */
