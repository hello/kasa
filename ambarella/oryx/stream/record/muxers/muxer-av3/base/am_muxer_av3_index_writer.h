/*******************************************************************************
 * am_muxer_av3_index_writer.h
 *
 * History:
 *   2016-08-31 - [ccjing] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents (“Software”) are protected by intellectual
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
#ifndef AM_MUXER_AV3_INDEX_WRITER_H_
#define AM_MUXER_AV3_INDEX_WRITER_H_
struct AM_VIDEO_INFO;
struct AM_AUDIO_INFO;
class AMAv3IndexWriter
{
  public :
    static AMAv3IndexWriter* create(uint32_t write_frequency, bool event = false);
  private :
    AMAv3IndexWriter();
    virtual ~AMAv3IndexWriter();
    bool init(uint32_t write_frequency, bool event = false);
  public :
    void destroy();
    bool open_file();
    void close_file(bool need_sync = false);
    bool set_AV3_file_name(char *file_name, uint32_t length);
    bool set_video_info(AM_VIDEO_INFO *video_info);
    bool set_audio_info(AM_AUDIO_INFO *audio_info);
    bool set_vps(uint8_t *vps, uint32_t size);
    bool set_sps(uint8_t *sps, uint32_t size);
    bool set_pps(uint8_t *pps, uint32_t size);
    bool write_video_information(int64_t delta_pts, uint64_t offset,
                                 uint64_t size, uint8_t sync_sample);
    bool write_audio_information(uint16_t aac_spec_config, int64_t delta_pts,
                   uint32_t packet_frame_count, uint64_t offset, uint64_t size);
    bool write_gps_information(int64_t delta_pts, uint64_t offset, uint64_t size);
  private :
    int32_t        m_fd;
    uint32_t       m_write_frequency;
    uint32_t       m_vps_size;
    uint32_t       m_sps_size;
    uint32_t       m_pps_size;
    AM_VIDEO_INFO *m_video_info;
    AM_AUDIO_INFO *m_audio_info;
    uint8_t       *m_vps;
    uint8_t       *m_sps;
    uint8_t       *m_pps;
    uint8_t       *m_index_buf;
    char           m_file_name[128];
    uint32_t       m_buf_offset;
    uint32_t       m_frame_count;
    uint8_t        m_info_map;
    uint8_t        m_param_map;
    bool           m_event;
};

#endif /* AM_MUXER_AV3_INDEX_WRITER_H_ */
