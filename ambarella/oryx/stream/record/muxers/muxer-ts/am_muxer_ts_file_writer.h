/*
 * am_muxer_ts_file_writer.h
 *
 *  19/09/2012 [Hanbo Xiao]    [Created]
 *  17/12/2014 [Chengcai Jing] [Modified]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __AM_MUXER_TS_FILE_WRITER_H__
#define __AM_MUXER_TS_FILE_WRITER_H__

#ifndef DATA_CACHE_SIZE
#define DATA_CACHE_SIZE (1 << 21) /* 2MB */
#endif
#include "am_video_types.h"
class AMIFileWriter;
struct AMMuxerCodecTSConfig;
class AMTsFileWriter
{
public:
   static AMTsFileWriter *create (AMMuxerCodecTSConfig * muxer_ts_config,
                                  AM_VIDEO_INFO *video_info);

public:
   void destroy ();
   AM_STATE on_eof (AM_STREAM_DATA_TYPE stream_type);
   AM_STATE on_eos (AM_STREAM_DATA_TYPE stream_type);
   AM_STATE set_media_sink (const char *dest_str);
   AM_STATE write_data (uint8_t *data_ptr, int data_len,
                        AM_STREAM_DATA_TYPE data_type);
   void close_file();
   void set_begin_packet_pts(int64_t pts);
   void set_end_packet_pts(int64_t pts);
   AM_STATE create_next_split_file ();

private:
   AMTsFileWriter ();
   AM_STATE init (AMMuxerCodecTSConfig * muxer_ts_config,
                  AM_VIDEO_INFO *video_info);
   virtual ~AMTsFileWriter ();

private:
   bool get_current_time_string(char *time_str, int32_t len);
   AM_STATE create_m3u8_file();

private:
   int64_t        m_begin_packet_pts;
   int64_t        m_end_packet_pts;
   int64_t        m_file_duration;
   int            m_EOF_map;
   int            m_EOS_map;
   int            m_file_counter;
   int            m_dir_counter;
   uint32_t       m_max_file_number;
   uint32_t       m_file_target_duration;
   uint32_t       m_hls_sequence;
   char          *m_file_name;
   char          *m_tmp_name;
   char          *m_path_name;
   char          *m_base_name;
   AMIFileWriter *m_file_writer;
   AMMuxerCodecTSConfig *m_muxer_ts_config;
   AM_VIDEO_INFO         *m_video_info;
};

#endif
