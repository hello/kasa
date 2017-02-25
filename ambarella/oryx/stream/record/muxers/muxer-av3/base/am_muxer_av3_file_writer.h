/*******************************************************************************
 * am_muxer_av3_file_writer.h
 *
 * History:
 *   2016-08-30 - [ccjing] created file
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
#ifndef AM_MUXER_AV3_FILE_WRITER_H_
#define AM_MUXER_AV3_FILE_WRITER_H_
#include "am_video_types.h"
#include "am_mutex.h"
#include "am_record_msg.h"
class AV3FileWriter;
struct AMMuxerCodecAv3Config;
struct AV3FileTypeBox;
struct AV3MediaDataBox;
struct AV3MovieBox;
struct AV3UUIDBox;
class AMAv3FileWriter
{
  public:
    static AMAv3FileWriter* create(AMMuxerCodecAv3Config *muxer_AV3_config,
                                   AM_VIDEO_INFO *video_info);
  private:
    AMAv3FileWriter();
    AM_STATE init(AMMuxerCodecAv3Config *muxer_av3_config,
                  AM_VIDEO_INFO *video_info);
    virtual ~AMAv3FileWriter();
  public:
    void destroy();
    AM_STATE set_media_sink(const char* file_name);
    AM_STATE write_data(const uint8_t* data, uint32_t data_len);
    AM_STATE write_data_direct(uint8_t *data, uint32_t data_len);
    AM_STATE seek_data(uint32_t offset, uint32_t whence); //SEEK_SET ...
    bool is_file_open();
    AM_STATE write_file_type_box(AV3FileTypeBox& box);
    AM_STATE write_media_data_box(AV3MediaDataBox& box);
    AM_STATE write_movie_box(AV3MovieBox& box, bool copy_to_mem = false);
    AM_STATE write_uuid_box(AV3UUIDBox& box, bool copy_to_mem = false);
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
    AM_STATE create_next_file();
    AM_STATE close_file(bool need_sync = false);
    void set_begin_packet_pts(int64_t pts);
    void set_end_packet_pts(int64_t pts);
    bool set_video_info(AM_VIDEO_INFO *vinfo);
    char* get_current_file_name();
    AV3FileWriter* get_file_writer();
    bool set_file_operation_cb(AM_FILE_OPERATION_CB_TYPE type,
                               AMFileOperationCB callback);
    const uint8_t* get_data_buffer();
    uint32_t get_data_size();
    void clear_buffer(bool free_mem = false);
  private :
    bool get_current_time_string(char *time_str, int32_t len);
    bool file_finish_cb();
    bool file_create_cb();

  private:
    uint64_t               m_cur_file_offset         = 0;
    int64_t                m_begin_packet_pts        = 0;
    int64_t                m_end_packet_pts          = 0;
    int64_t                m_file_duration           = 0;
    AMMuxerCodecAv3Config *m_config                  = nullptr;
    char                  *m_file_name               = nullptr;
    char                  *m_tmp_file_name           = nullptr;
    char                  *m_path_name               = nullptr;
    char                  *m_base_name               = nullptr;
    AV3FileWriter         *m_file_writer             = nullptr;
    AMFileOperationCB      m_file_finish_cb          = nullptr;
    AMFileOperationCB      m_file_create_cb          = nullptr;
    int32_t                m_file_counter            = 0;
    int32_t                m_dir_counter             = 0;
    char                   m_time_string[32]         = { 0 };
    AM_VIDEO_INFO          m_video_info;
    AMMemLock              m_callback_lock;
};

#endif /* AM_MUXER_AV3_FILE_WRITER_H_ */
