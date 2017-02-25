/*******************************************************************************
 * am_muxer_mp4_base.h
 *
 * History:
 *   2015-12-28 - [ccjing] created file
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
#ifndef AM_MUXER_MP4_BASE_H_
#define AM_MUXER_MP4_BASE_H_
#include "am_file_operation_if.h"
#include "am_muxer_codec_if.h"
#include "am_audio_define.h"
#include "am_video_types.h"
#include "am_mutex.h"
#include "am_queue.h"

#include <deque>
#include <list>
#include <string>
#include "am_muxer_mp4_config.h"
class AMPacket;
class AMMuxerMp4Builder;
class AMMp4FileWriter;
class AMThread;
struct AMMuxerCodecMp4Config;
class AMMuxerMp4Config;
#define AUDIO_PACKET_QUEUE_NUM      2
#define ON_DATA_PKT_ERROR_NUM      50
#define CHECK_FREE_SPACE_FREQUENCY 50
#define PTS_TIME_FREQUENCY         90000
class AMMuxerMp4Base: public AMIMuxerCodec, public AMIFileOperation
{
    typedef AMSafeDeque<AMPacket*> packet_queue;
    typedef std::list<AMPacket*> packet_list;
    typedef std::list<std::string> string_list;
  public:
    /*interface of AMIMuxerCodec*/
    virtual AM_STATE start();
    virtual AM_STATE stop();
    virtual const char* name();
    virtual void* get_muxer_codec_interface(AM_REFIID refiid);
    virtual bool is_running();
    virtual AM_MUXER_CODEC_STATE get_state();
    virtual uint8_t get_muxer_codec_stream_id();
    virtual uint32_t get_muxer_id();
    virtual AM_STATE set_config(AMMuxerCodecConfig *config);
    virtual AM_STATE get_config(AMMuxerCodecConfig *config);
    /* Interface of AMIFileOperation */
    virtual bool stop_file_writing();
    virtual bool set_recording_file_num(uint32_t file_num);
    virtual bool set_recording_duration(int32_t duration);
    virtual bool set_file_duration(int32_t file_duration, bool apply_conf_file);
    virtual bool set_file_operation_callback(AM_FILE_OPERATION_CB_TYPE type,
                                             AMFileOperationCB callback);
    virtual bool set_muxer_param(AMMuxerParam &param);
  private:
    static void thread_entry(void *p);
    virtual void main_loop()                                    = 0;
    virtual AM_STATE generate_file_name(std::string &file_name) = 0;
    virtual void clear_params_for_new_file()                    = 0;
    virtual AM_STATE check_video_pts(AMPacket* packet)          = 0;
    virtual AM_STATE on_info_pkt(AMPacket* packet)              = 0;
    virtual AM_STATE on_data_pkt(AMPacket* packet)              = 0;
  protected :
    AM_STATE on_eos_pkt(AMPacket* packet);
    AM_STATE on_eof_pkt(AMPacket* packet); //close current file and create new file.
    AM_STATE write_video_data_pkt(AMPacket* packet);
    AM_STATE write_audio_data_pkt(AMPacket* packet);
  protected :
    AMMuxerMp4Base();
    AM_STATE init(const char* config_file);
    virtual ~AMMuxerMp4Base();
    AM_MUXER_CODEC_STATE create_resource();
    void release_resource();
    void clear_all_params();
    bool get_proper_file_location(std::string &file_location);
    bool get_current_time_string(char *time_str, int32_t len);
    bool is_h265_IDR_first_nalu(AMPacket* packet);
    void check_storage_free_space();
    std::string audio_type_to_string(AM_AUDIO_TYPE type);
    void update_config_param();
  protected:
    AM_PTS                 m_last_video_pts          = 0;
    AM_PTS                 m_stop_recording_pts      = 0;
    int64_t                m_curr_file_boundary      = 0;
    int64_t                m_first_video_pts         = 0;
    int64_t                m_frame_pts_diff          = 0;
    AMThread              *m_thread                  = nullptr;
    char                  *m_config_file             = nullptr;
    AMMuxerMp4Builder     *m_mp4_builder             = nullptr;
    AMMp4FileWriter       *m_file_writer             = nullptr;
    AMMuxerMp4Config      *m_config                  = nullptr;
    AMMuxerCodecMp4Config *m_muxer_mp4_config        = nullptr;/*do not need to delete*/
    AMFileOperationCB      m_file_create_cb          = nullptr;
    AMFileOperationCB      m_file_finish_cb          = nullptr;
    uint32_t               m_eos_map                 = 0;
    uint32_t               m_av_info_map             = 0;
    uint32_t               m_video_frame_count       = 0;
    uint32_t               m_last_frame_number       = 0;
    uint32_t               m_file_counter            = 0;
    AM_MUXER_CODEC_STATE   m_state                   = AM_MUXER_CODEC_INIT;
    bool                   m_is_audio_accepted       = false;
    bool                   m_is_video_arrived        = false;
    bool                   m_is_first_video          = false;
    bool                   m_need_splitted           = false;
    bool                   m_file_writing            = false;
    bool                   m_audio_sample_rate_set   = false;
    std::atomic_bool       m_run                     = {false};
    AM_AUDIO_INFO          m_audio_info;
    AM_VIDEO_INFO          m_video_info;
    std::string            m_muxer_name;
    std::string            m_file_location;
    AMMemLock              m_state_lock;
    AMMemLock              m_interface_lock;
    AMMemLock              m_file_param_lock;
    AMMemLock              m_param_set_lock;
    packet_queue           m_packet_queue;
    /*parameters for current file recording*/
    AMMuxerCodecMp4Config  m_curr_config;
    /*changed by muxer interface, used to update m_curr_config*/
    AMMuxerCodecMp4Config  m_set_config;
};

#endif /* AM_MUXER_MP4_BASE_H_ */
