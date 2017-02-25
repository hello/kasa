/*******************************************************************************
 * am_muxer_periodic_mjpeg.h
 *
 * History:
 *   2016-04-20 - [ccjing] created file
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
#ifndef AM_MUXER_MJPEG_H_
#define AM_MUXER_MJPEG_H_
#include "am_file_operation_if.h"
#include "am_muxer_codec_if.h"
#include "am_video_types.h"
#include "am_mutex.h"

#include <deque>
#include <list>
#include <string>

#include "am_muxer_periodic_mjpeg_config.h"
#include "am_record_event_sender.h"

class AMPacket;
class AMPeriodicMjpegWriter;
class AMThread;
struct AMMuxerCodecPeriodicMjpegConfig;
class AMMuxerPeriodicMjpegConfig;

#define PERIODIC_MJPEG_DATA_PKT_ERROR_NUM             5
#define CHECK_FREE_SPACE_FREQUENCY_FOR_PERIODIC_MJPEG 50

struct MjpegFrameInfo
{
    uint32_t cur_second;
    uint32_t offset;
    uint32_t frame_size;
    uint32_t frame_counter;
};

class AMMuxerPeriodicMjpeg : public AMIMuxerCodec, public AMIFileOperation
{
    typedef std::list<std::string> string_list;
  public:
    typedef std::deque<AMPacket*> packet_queue;
    static AMMuxerPeriodicMjpeg* create(const char* config_name);
    /*interface of AMIMuxerCodec*/
    virtual AM_STATE start();
    virtual AM_STATE stop();
    virtual const char* name();
    virtual void* get_muxer_codec_interface(AM_REFIID refiid);
    virtual bool is_running();
    virtual AM_MUXER_CODEC_STATE get_state();
    virtual AM_MUXER_ATTR get_muxer_attr();
    virtual uint8_t get_muxer_codec_stream_id();
    virtual uint32_t get_muxer_id();
    virtual void     feed_data(AMPacket* packet);
    virtual AM_STATE set_config(AMMuxerCodecConfig *config);
    virtual AM_STATE get_config(AMMuxerCodecConfig *config);
    /* Interface of AMIFileOperation */
    virtual bool start_file_writing();
    virtual bool stop_file_writing();
    virtual bool set_recording_file_num(uint32_t file_num);
    virtual bool set_recording_duration(int32_t duration);
    virtual bool set_file_duration(int32_t file_duration, bool apply_conf_file);
    virtual bool set_file_operation_callback(AM_FILE_OPERATION_CB_TYPE type,
                                             AMFileOperationCB callback);
    virtual bool set_muxer_param(AMMuxerParam &param);
  private :
    AMMuxerPeriodicMjpeg();
    virtual ~AMMuxerPeriodicMjpeg();
    AM_STATE init(const char* config_file);
    virtual void main_loop();
    AM_MUXER_CODEC_STATE create_resource();
    virtual AM_STATE generate_file_name(char file_name[]);
    static void thread_entry(void *p);
    void release_resource();
    bool get_current_time_string(char *time_str, int32_t len);
    bool get_proper_file_location(std::string& file_location);
    void check_storage_free_space();
    AM_STATE on_info_packet(AMPacket* packet);
    virtual AM_STATE on_data_packet(AMPacket* packet);
    AM_STATE on_eos_packet(AMPacket* packet);
    bool parse_event_param(AMPacket* packet);
    bool check_file_size();
    AM_PTS get_current_pts();
    void update_config_param();
  private:
    int64_t                          m_start_pts             = 0;
    int64_t                          m_end_pts               = 0;
    int64_t                          m_interval_pts          = 0;
    int64_t                          m_next_period_pts       = 0;
    AMThread                        *m_thread                = nullptr;
    AMMuxerCodecPeriodicMjpegConfig *m_periodic_mjpeg_config = nullptr;/*do not need to delete*/
    AMMuxerPeriodicMjpegConfig      *m_config                = nullptr;
    packet_queue                    *m_packet_queue          = nullptr;
    char                            *m_config_file           = nullptr;
    AMPeriodicMjpegWriter           *m_file_writer           = nullptr;
    AMFileOperationCB                m_file_create_cb        = nullptr;
    AMFileOperationCB                m_file_finish_cb        = nullptr;
    uint32_t                         m_once_jpeg_num         = 0;
    uint32_t                         m_tmp_jpeg_num          = 0;
    uint32_t                         m_file_size             = 0;
    uint32_t                         m_frame_counter         = 0;
    uint32_t                         m_start_second          = 0;
    int                              m_hw_timer_fd           = -1;
    AM_MUXER_CODEC_STATE             m_state                 = AM_MUXER_CODEC_INIT;
    bool                             m_file_writing          = true;
    bool                             m_jpeg_enable           = false;
    std::atomic_bool                 m_run                   = {false};
    std::string                      m_file_location;
    std::string                      m_muxer_name;
    AMMemLock                        m_state_lock;
    AMMemLock                        m_interface_lock;
    AMMemLock                        m_file_writing_lock;
    AMMemLock                        m_param_set_lock;
    AMMuxerCodecPeriodicMjpegConfig  m_curr_config;
    AMMuxerCodecPeriodicMjpegConfig  m_set_config;
};
#endif /* AM_MUXER_MJPEG_H_ */
