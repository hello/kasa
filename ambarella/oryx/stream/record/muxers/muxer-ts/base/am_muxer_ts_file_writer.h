/*
 * am_muxer_ts_file_writer.h
 *
 *  19/09/2012 [Hanbo Xiao]    [Created]
 *  17/12/2014 [Chengcai Jing] [Modified]
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
 */
#ifndef __AM_MUXER_TS_FILE_WRITER_H__
#define __AM_MUXER_TS_FILE_WRITER_H__

#ifndef DATA_CACHE_SIZE
#define DATA_CACHE_SIZE (1 << 21) /* 2MB */
#endif
#include "am_video_types.h"
#include "am_mutex.h"
#include "am_record_msg.h"
class AMIFileWriter;
struct AMMuxerCodecTSConfig;
class AMTsFileWriter
{
public:
   static AMTsFileWriter *create (AMMuxerCodecTSConfig * muxer_ts_config,
                                  AM_VIDEO_INFO *video_info);
public:
   void destroy ();
   AM_STATE set_media_sink (const char *dest_str);
   AM_STATE write_data (uint8_t *data_ptr, int data_len);
   AM_STATE close_file(bool need_sync = false);
   void set_begin_packet_pts(int64_t pts);
   void set_end_packet_pts(int64_t pts);
   AM_STATE create_next_file ();
   char* get_current_file_name();
   bool is_file_open();
   bool set_file_operation_cb(AM_FILE_OPERATION_CB_TYPE type,
                              AMFileOperationCB callback);
private:
   AMTsFileWriter ();
   AM_STATE init (AMMuxerCodecTSConfig * muxer_ts_config,
                  AM_VIDEO_INFO *video_info);
   virtual ~AMTsFileWriter ();

private:
   bool get_current_time_string(char *time_str, int32_t len);
   AM_STATE create_m3u8_file();
   bool file_finish_cb();
   bool file_create_cb();

private:
   int64_t               m_begin_packet_pts        = 0;
   int64_t               m_end_packet_pts          = 0;
   int64_t               m_file_duration           = 0;
   char                 *m_file_name               = nullptr;
   char                 *m_tmp_name                = nullptr;
   char                 *m_path_name               = nullptr;
   char                 *m_base_name               = nullptr;
   AMIFileWriter        *m_file_writer             = nullptr;
   AMMuxerCodecTSConfig *m_config                  = nullptr;
   AM_VIDEO_INFO        *m_video_info              = nullptr;
   AMFileOperationCB     m_file_finish_cb          = nullptr;
   AMFileOperationCB     m_file_create_cb          = nullptr;
   int32_t               m_file_counter            = 0;
   int32_t               m_dir_counter             = 0;
   uint32_t              m_max_file_num_per_folder = 0;
   uint32_t              m_hls_sequence            = 0;
   char                  m_time_string[32]         { 0 };
   AMMemLock             m_callback_lock;
};

#endif
