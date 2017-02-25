/*
 * am_muxer_ts_base.h
 *
 * 11/09/2012 [Hanbo Xiao] [Created]
 * 17/12/2014 [Chengcai Jing] [Modified]
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
#ifndef __AM_MUXER_TS_BASE_H__
#define __AM_MUXER_TS_BASE_H__
#include "am_file_operation_if.h"
#include "am_muxer_codec_if.h"
#include "am_muxer_ts_config.h"
#include "am_queue.h"
#include "am_mutex.h"

#include <list>
#include <deque>
#include <atomic>
#include <string>
#include "adts.h"

#define PTS_TIME_FREQUENCY         90000
#define ON_DATA_PKT_ERROR_NUM      3
#define CHECK_FREE_SPACE_FREQUENCY 50

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

class AMTsMuxerBase : public AMIMuxerCodec, AMIFileOperation
{
    typedef AMSafeDeque<AMPacket*> packet_queue;
    typedef std::list<std::string> string_list;
    typedef std::vector<ADTS>         AMAdtsList;
    /* Interface of AMIMuxerCodec*/
  public:
    virtual AM_STATE start();
    virtual AM_STATE stop();
    virtual const char* name();
    virtual void* get_muxer_codec_interface(AM_REFIID refiid);
    virtual bool is_running();
    virtual AM_STATE set_config(AMMuxerCodecConfig *config);
    virtual AM_STATE get_config(AMMuxerCodecConfig *config);
    virtual AM_MUXER_ATTR get_muxer_attr() = 0;
    virtual AM_MUXER_CODEC_STATE get_state();
    virtual uint8_t get_muxer_codec_stream_id();
    virtual uint32_t get_muxer_id();
    virtual bool stop_file_writing();
    virtual bool set_recording_file_num(uint32_t file_num);
    virtual bool set_recording_duration(int32_t duration);
    virtual bool set_file_duration(int32_t file_duration, bool apply_conf_file);
    virtual bool set_file_operation_callback(AM_FILE_OPERATION_CB_TYPE type,
                                             AMFileOperationCB callback);
    virtual bool set_muxer_param(AMMuxerParam &param);
  protected:
    AMTsMuxerBase();
    AM_STATE init(const char* config_file);
    virtual ~AMTsMuxerBase();
  private:
    static void thread_entry(void* p);
    virtual void main_loop() = 0;
    virtual AM_STATE generate_file_name(std::string &file_name) = 0;
    virtual void clear_params_for_new_file() = 0;
    virtual AM_STATE on_info_packet(AMPacket* packet) = 0;
    virtual AM_STATE on_data_packet(AMPacket* packet) = 0;
  protected :
    AM_MUXER_CODEC_STATE create_resource();
    void release_resource();
    bool get_proper_file_location(std::string& file_location);
    AM_STATE on_eos_packet(AMPacket *packet);
    AM_STATE on_eof_packet(AMPacket *packet);//close current file and create a new file.
    AM_STATE init_ts();
    void reset_parameter();
    AM_STATE build_and_write_audio(AMPacket* packet, uint32_t frame_num);
    AM_STATE build_and_write_video(uint8_t *data, uint32_t size, AM_PTS pts,
                                   bool frame_begin);
    AM_STATE build_and_write_pat_pmt();
    AM_STATE update_and_write_pat_pmt();
    AM_STATE process_h264_data_pkt(AMPacket* packet);
    AM_STATE process_h265_data_pkt(AMPacket* packet);
    AM_STATE process_audio_data_pkt(AMPacket* packet);
    void pcr_increment(int64_t pts);
    void pcr_calc_pkt_duration(uint32_t rate, uint32_t scale);
    AM_STATE set_audio_info(AM_AUDIO_INFO* audio_info);
     AM_STATE set_video_info(AM_VIDEO_INFO* video_info);
    bool get_current_time_string(char *time_str, int32_t len);
    std::string audio_type_to_string(AM_AUDIO_TYPE type);
    void check_storage_free_space();
    bool check_pcr_overflow(AMPacket* packet);
    void find_adts(AMPacket* packet);
    bool create_new_file();
    void update_config_param();
  protected :
    AM_PTS                      m_last_video_pts        = 0;
    AM_PTS                      m_first_video_pts       = 0;
    AM_PTS                      m_pcr_base              = 0;
    AM_PTS                      m_pcr_inc_base          = 0;
    AM_PTS                      m_stop_recording_pts    = 0;
    int64_t                     m_curr_file_boundary    = 0;
    int64_t                     m_pts_base_video        = 0;
    int64_t                     m_pts_base_audio        = 0;
    AMThread                   *m_thread                = nullptr;
    AMMuxerCodecTSConfig       *m_muxer_ts_config       = nullptr;/*do not need to delete*/
    AMMuxerTsConfig            *m_config                = nullptr;
    packet_queue               *m_packet_queue          = nullptr;
    packet_queue               *m_hevc_pkt_queue        = nullptr;
    AMTsBuilder                *m_ts_builder            = nullptr;
    AMTsFileWriter             *m_file_writer           = nullptr;
    char                       *m_config_file           = nullptr;
    AMFileOperationCB           m_file_create_cb        = nullptr;
    AMFileOperationCB           m_file_finish_cb        = nullptr;
    AM_MUXER_CODEC_STATE        m_state                 = AM_MUXER_CODEC_INIT;
    uint32_t                    m_av_info_map           = 0;
    uint32_t                    m_eos_map               = 0;
    uint32_t                    m_video_frame_count     = 0;
    uint32_t                    m_file_counter          = 0;
    uint16_t                    m_pcr_ext               = 0;
    uint16_t                    m_pcr_inc_ext           = 0;
    uint8_t                     m_lpcm_descriptor[8]    = { 0 };
    uint8_t                     m_pmt_descriptor[4]     = { 0 };
    uint8_t                     m_hevc_slice_num        = 0;
    uint8_t                     m_hevc_tile_num         = 0;
    bool                        m_audio_enable          = false;
    bool                        m_is_first_audio        = true;
    bool                        m_is_first_video        = true;
    bool                        m_need_splitted         = false;
    bool                        m_is_new_info           = false;
    bool                        m_file_writing          = true;
    bool                        m_audio_sample_rate_set = false;
    std::atomic_bool            m_run                   = {false};
    AM_TS_MUXER_PSI_PAT_INFO    m_pat_info;
    AM_TS_MUXER_PSI_PMT_INFO    m_pmt_info;
    AM_TS_MUXER_PSI_PRG_INFO    m_program_info;
    AM_TS_MUXER_PSI_STREAM_INFO m_video_stream_info;
    AM_TS_MUXER_PSI_STREAM_INFO m_audio_stream_info;
    AM_VIDEO_INFO               m_video_info;
    AM_AUDIO_INFO               m_audio_info;
    PACKET_BUF                  m_pat_buf;
    PACKET_BUF                  m_pmt_buf;
    PACKET_BUF                  m_video_pes_buf;
    PACKET_BUF                  m_audio_pes_buf;
    std::string                 m_muxer_name;
    std::string                 m_file_location;
    AMAdtsList                  m_adts;
    AMMemLock                   m_lock;
    AMMemLock                   m_state_lock;
    AMMemLock                   m_file_param_lock;
    AMMemLock                   m_param_set_lock;
    AMMuxerCodecTSConfig        m_curr_config;
    AMMuxerCodecTSConfig        m_set_config;
};

#endif
