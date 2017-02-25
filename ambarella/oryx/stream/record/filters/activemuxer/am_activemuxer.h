/*******************************************************************************
 * am_muxer.h
 *
 * History:
 *   2014-12-26 - [ypchang] created file
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
#ifndef ORYX_STREAM_RECORD_FILTERS_MUXER_AM_MUXER_H_
#define ORYX_STREAM_RECORD_FILTERS_MUXER_AM_MUXER_H_

#include <atomic>
#include <vector>

struct ActiveMuxerConfig;
struct AVQueueConfig;
class AMEvent;
class AMActiveMuxerInput;
class AMActiveMuxerConfig;
class AMActiveMuxerCodecObj;
class AMAVQueue;

class AMActiveMuxer: public AMPacketActiveFilter, public AMIMuxer
{
    friend class AMActiveMuxerCodecObj;
    friend class AMActiveMuxerInput;
    typedef AMPacketActiveFilter inherited;
    typedef std::vector<AMActiveMuxerInput*> AMMuxerInputList;

  public:
    static AMIMuxer* create(AMIEngine *engine, const std::string& config,
                            uint32_t input_num, uint32_t output_num);

  public:
    virtual void* get_interface(AM_REFIID ref_iid);
    virtual void destroy();
    virtual void get_info(INFO& info);
    virtual AMIPacketPin* get_input_pin(uint32_t index);
    virtual AMIPacketPin* get_output_pin(uint32_t index);
    virtual AM_STATE start();
    virtual AM_STATE stop();
    virtual bool start_send_normal_pkt();
    virtual bool start_file_recording(uint32_t muxer_id_bit_map);
    virtual bool stop_file_recording(uint32_t muxer_id_bit_map);
    virtual bool set_recording_file_num(uint32_t muxer_id_bit_map,
                                        uint32_t file_num);
    virtual bool set_recording_duration(uint32_t muxer_id_bit_map,
                                        int32_t duration);
    virtual bool set_file_duration(uint32_t muxer_id_bit_map,
                                   int32_t file_duration,
                                   bool apply_conf_file);
    virtual bool set_muxer_param(AMMuxerParam &param);
    virtual bool set_file_operation_callback(uint32_t muxer_id_bit_map,
                                             AM_FILE_OPERATION_CB_TYPE type,
                                             AMFileOperationCB callback);
    virtual bool is_ready_for_event(AMEventStruct& event);
    virtual bool update_image_3a_info(AMImage3AInfo *image_3a);
    virtual uint32_t version();
    virtual AM_MUXER_TYPE type();

  private:
    virtual void on_run();
    void work_loop();
    void avqueue_cb(AMPacket *packet);
  private:
    AM_STATE load_muxer_codec();
    void destroy_muxer_codec();
    void get_avqueue_config(ActiveMuxerConfig& muxer_config,
                            AVQueueConfig& avqueue_config);
    bool create_resource();
    void release_resource();

  private:
    AMActiveMuxer(AMIEngine *engine);
    virtual ~AMActiveMuxer();
    AM_STATE init(const std::string& config,
                  uint32_t input_num,
                  uint32_t output_num);

  private:
    ActiveMuxerConfig     *m_muxer_config      = nullptr; /* No need to delete */
    AMActiveMuxerConfig   *m_config            = nullptr;
    AMActiveMuxerCodecObj *m_muxer_codec       = nullptr;
    AMAVQueuePtr           m_avqueue           = nullptr;
    uint32_t               m_input_num         = 0;
    uint32_t               m_output_num        = 0;
    std::atomic<bool>      m_run               = {false};
    AMMuxerInputList       m_inputs;
    AVQueueConfig          m_avqueue_config;
};

class AMActiveMuxerInput: public AMPacketQueueInputPin
{
    typedef AMPacketQueueInputPin inherited;
    friend class AMActiveMuxer;

  public:
    static AMActiveMuxerInput* create(AMPacketFilter *filter)
    {
      AMActiveMuxerInput *result = new AMActiveMuxerInput(filter);
      if (AM_UNLIKELY(result && (AM_STATE_OK != result->init()))) {
        delete result;
        result = NULL;
      }
      return result;
    }

  private:
    AMActiveMuxerInput(AMPacketFilter *filter) :
      inherited(filter)
    {}
    virtual ~AMActiveMuxerInput(){}
    AM_STATE init()
    {
      return inherited::init(((AMActiveMuxer*)m_filter)->msgQ());
    }
};

#endif /* ORYX_STREAM_RECORD_FILTERS_MUXER_AM_MUXER_H_ */
