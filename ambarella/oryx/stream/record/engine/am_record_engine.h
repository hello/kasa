/*******************************************************************************
 * am_record_engine.h
 *
 * History:
 *   2014-12-30 - [ypchang] created file
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
#ifndef ORYX_STREAM_RECORD_ENGINE_AM_RECORD_ENGINE_H_
#define ORYX_STREAM_RECORD_ENGINE_AM_RECORD_ENGINE_H_

#include "am_record_engine_if.h"
#include "am_mutex.h"
#include <vector>

typedef std::vector<void*> AMFilterVector;

struct EngineConfig;
struct EngineFilter;
struct ConnectionConfig;

class AMEvent;
class AMThread;
class AMRecordEngineConfig;
class AMIMuxer;
class AMIAudioSource;

class AMRecordEngine: public AMEngineFrame, public AMIRecordEngine
{
    typedef AMEngineFrame inherited;

    enum AM_RECORD_ENGINE_CMD
    {
      AM_ENGINE_CMD_START = 's',
      AM_ENGINE_CMD_STOP  = 'q',
      AM_ENGINE_CMD_ABORT = 'a',
      AM_ENGINE_CMD_EXIT  = 'e',
    };
  public:
    static AMRecordEngine* create(const std::string& config);

  public:
    virtual void* get_interface(AM_REFIID ref_iid);
    virtual void destroy();

  public:
    virtual AMIRecordEngine::AM_RECORD_ENGINE_STATUS get_engine_status();
    virtual bool create_graph();
    virtual bool record();
    virtual bool stop();
    virtual bool start_file_recording(uint32_t muxer_id_bit_map);
    virtual bool stop_file_recording(uint32_t muxer_id_bit_map);
    virtual bool start_file_muxer();
    virtual bool set_recording_file_num(uint32_t muxer_id_bit_map,
                                        uint32_t file_num);
    virtual bool set_recording_duration(uint32_t muxer_id_bit_map,
                                        int32_t duration);
    virtual bool set_file_duration(uint32_t muxer_id_bit_map,
                                   int32_t file_duration,
                                   bool apply_conf_file);
    virtual bool set_muxer_param(AMMuxerParam &param);
    virtual bool is_ready_for_event(AMEventStruct& event);
    virtual bool send_event(AMEventStruct& event);
    virtual bool enable_audio_codec(AM_AUDIO_TYPE type, uint32_t sample_rate,
                                    bool enable);
    virtual void set_app_msg_callback(AMRecordCallback callback, void *data);
    virtual void set_aec_enabled(bool enabled);
    virtual bool set_file_operation_callback(uint32_t muxer_id_bit_map,
                                             AM_FILE_OPERATION_CB_TYPE type,
                                             AMFileOperationCB callback);
    virtual bool update_image_3A_info(AMImage3AInfo *image_3A);

  private:
    static void static_app_msg_callback(void *context, AmMsg& msg);
    void app_msg_callback(void *context, AmMsg& msg);
    virtual void msg_proc(AmMsg& msg);
    bool change_engine_status(AMIRecordEngine::AM_RECORD_ENGINE_STATUS tStatus);

  private:
    AMRecordEngine();
    virtual ~AMRecordEngine();
    AM_STATE init(const std::string& config);
    AM_STATE load_all_filters();
    AMIPacketFilter* get_filter_by_name(std::string& name);
    AMIMuxer* get_muxer_filter_by_name(std::string& name);
    const char* get_filter_name_by_pointer(AMIInterface *filter);
    ConnectionConfig* get_connection_conf_by_name(std::string& name);
    void* get_filter_by_iid(AM_REFIID iid);
    AMFilterVector get_filter_vector_by_iid(AM_REFIID iid);
    static void static_mainloop(void *data);
    void mainloop();
    bool send_engine_cmd(AM_RECORD_ENGINE_CMD cmd, bool block = true);

  private:
    AMRecordEngineConfig   *m_config;
    EngineConfig           *m_engine_config; /* No need to delete */
    EngineFilter           *m_engine_filter;
    void                   *m_app_data;
    AMThread               *m_thread;
    AMEvent                *m_event;
    AMEvent                *m_sem;
    AMRecordCallback        m_app_callback;
    int                     m_msg_ctrl[2];
    AM_RECORD_ENGINE_STATUS m_status;
    bool                    m_graph_created;
    std::atomic_bool        m_mainloop_run;
    AMRecordMsg             m_app_msg;
    AMMemLock               m_lock;
#define MSG_R m_msg_ctrl[0]
#define MSG_W m_msg_ctrl[1]
};

#endif /* ORYX_STREAM_RECORD_ENGINE_AM_RECORD_ENGINE_H_ */
